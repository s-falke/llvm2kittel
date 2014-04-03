// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "Version.h"

#include "StrengthIncreaser.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Constants.h>
  #include <llvm/BasicBlock.h>
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Constants.h>
  #include <llvm/IR/BasicBlock.h>
  #include <llvm/IR/Instructions.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <iostream>
#include <inttypes.h>

char StrengthIncreaser::ID = 0;

StrengthIncreaser::StrengthIncreaser()
  : llvm::BasicBlockPass(ID)
{}

static uint64_t getPower(uint64_t arg)
{
    uint64_t res = 1;
    while (arg > 0) {
        res <<= 1;
        arg -= 1;
    }
    return res;
}

bool StrengthIncreaser::runOnBasicBlock(llvm::BasicBlock &bb)
{
    bool res = false;
    for (llvm::BasicBlock::iterator inst = bb.begin(), inste = bb.end(); inst != inste; ++inst) {
        if (inst->isShift()) {
            llvm::BinaryOperator *shift = llvm::cast<llvm::BinaryOperator>(inst);
            llvm::BinaryOperator::BinaryOps opcode = shift->getOpcode();
            if (opcode == llvm::Instruction::Shl || opcode == llvm::Instruction::AShr || opcode == llvm::Instruction::LShr) {
                if (llvm::isa<llvm::ConstantInt>(shift->getOperand(1))) {
                    llvm::ConstantInt *c = llvm::cast<llvm::ConstantInt>(shift->getOperand(1));
                    llvm::ConstantInt *power = llvm::ConstantInt::get(c->getType(), getPower(c->getZExtValue()));
                    llvm::BinaryOperator *arith = NULL;
                    if (opcode == llvm::Instruction::Shl) {
                        arith = llvm::BinaryOperator::CreateNSWMul(shift->getOperand(0), power, "", shift);
                        arith->setHasNoSignedWrap(false);
                    } else if (opcode == llvm::Instruction::AShr) {
                        arith = llvm::BinaryOperator::CreateExactSDiv(shift->getOperand(0), power, "", shift);
                        arith->setIsExact(false);
                    } else {
                        arith = llvm::BinaryOperator::CreateExactUDiv(shift->getOperand(0), power, "", shift);
                        arith->setIsExact(false);
                    }
                    shift->moveBefore(arith);
                    shift->replaceAllUsesWith(arith);
                    inst = llvm::BasicBlock::iterator(arith);
                    shift->eraseFromParent();
                }
            }
        }
    }
    return res;
}

llvm::BasicBlockPass *createStrengthIncreaserPass()
{
    return new StrengthIncreaser();
}
