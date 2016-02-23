// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/ConstantExprEliminator.h"
#include "llvm2kittel/Util/Version.h"

char ConstantExprEliminator::ID = 0;

ConstantExprEliminator::ConstantExprEliminator()
  : llvm::FunctionPass(ID)
{}

bool ConstantExprEliminator::runOnFunction(llvm::Function &function)
{
    bool res = false;
    bool doLoop = true;
    while (doLoop) {
        doLoop = false;
        for (llvm::Function::iterator i = function.begin(), e = function.end(); i != e; ++i) {
            llvm::BasicBlock *bb = &*i;
            bool bbchanged = false;
            for (llvm::BasicBlock::iterator ibb = bb->begin(), ebb = bb->end(); ibb != ebb; ++ibb) {
                llvm::Instruction *inst = &*ibb;
                unsigned int n = inst->getNumOperands();
                for (unsigned int j = 0; j < n; ++j) {
                    llvm::Value *arg = inst->getOperand(j);
                    if (llvm::isa<llvm::ConstantExpr>(arg)) {
                        llvm::ConstantExpr *constExpr = llvm::cast<llvm::ConstantExpr>(arg);
                        llvm::Instruction *before = getBeforePoint(inst, j);
                        llvm::Instruction *newArg = replaceConstantExpr(constExpr, before);
                        if (newArg != NULL) {
                            res = true;
                            doLoop = true;
                            bbchanged = true;
                            inst->setOperand(j, newArg);
                        }
                    }
                }
                if (bbchanged) {
                    break;
                }
            }
        }
    }
    return res;
}

llvm::Instruction *ConstantExprEliminator::getBeforePoint(llvm::Instruction *instr, unsigned int j)
{
    if (llvm::isa<llvm::PHINode>(instr)) {
        llvm::PHINode *phi = llvm::cast<llvm::PHINode>(instr);
#if LLVM_VERSION <= VERSION(2, 9)
        llvm::BasicBlock *predBB = llvm::cast<llvm::BasicBlock>(phi->getOperand(j + 1));
#else
        llvm::BasicBlock *predBB = phi->getIncomingBlock(j);
#endif
        return predBB->getTerminator();
    } else {
        return instr;
    }
}

llvm::Instruction *ConstantExprEliminator::replaceConstantExpr(llvm::ConstantExpr *constantExpr, llvm::Instruction *before)
{
    std::string opcodeName = constantExpr->getOpcodeName();
    if (opcodeName == "getelementptr") {
        std::vector<llvm::Value*> args;
        for (unsigned int i = 1; i < constantExpr->getNumOperands(); ++i) {
            args.push_back(constantExpr->getOperand(i));
        }
#if LLVM_VERSION < VERSION(3, 0)
        llvm::Instruction *instr = llvm::GetElementPtrInst::Create(constantExpr->getOperand(0), args.begin(), args.end(), constantExpr->getName(), before);
#elif LLVM_VERSION < VERSION(3, 7)
        llvm::Instruction *instr = llvm::GetElementPtrInst::Create(constantExpr->getOperand(0), args, constantExpr->getName(), before);
#else
        llvm::Type *sourceElementType = llvm::cast<llvm::SequentialType>(constantExpr->getOperand(0)->getType()->getScalarType())->getElementType();
        llvm::Instruction *instr = llvm::GetElementPtrInst::Create(sourceElementType, constantExpr->getOperand(0), args, constantExpr->getName(), before);
#endif
        return instr;
    } else if (opcodeName == "ptrtoint") {
        llvm::Instruction *instr = new llvm::PtrToIntInst(constantExpr->getOperand(0), constantExpr->getType(), constantExpr->getName(), before);
        return instr;
    } else if (opcodeName == "inttoptr") {
        llvm::Instruction *instr = new llvm::IntToPtrInst(constantExpr->getOperand(0), constantExpr->getType(), constantExpr->getName(), before);
        return instr;
    } else if (opcodeName == "bitcast") {
        llvm::Instruction *instr = new llvm::BitCastInst(constantExpr->getOperand(0), constantExpr->getType(), constantExpr->getName(), before);
        return instr;
#if LLVM_VERSION >= VERSION(3, 4)
    } else if (opcodeName == "addrspacecast") {
        llvm::Instruction *instr = new llvm::AddrSpaceCastInst(constantExpr->getOperand(0), constantExpr->getType(), constantExpr->getName(), before);
        return instr;
#endif
    } else if (opcodeName == "add") {
        if (constantExpr == llvm::ConstantExpr::getAdd(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // no flag
            llvm::BinaryOperator *add = llvm::BinaryOperator::CreateNUWAdd(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            add->setHasNoUnsignedWrap(false);
            return add;
        } else if (constantExpr == llvm::ConstantExpr::getNUWAdd(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nuw flag
            llvm::BinaryOperator *add = llvm::BinaryOperator::CreateNUWAdd(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return add;
        } else if (constantExpr == llvm::ConstantExpr::getNSWAdd(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nsw flag
            llvm::BinaryOperator *add = llvm::BinaryOperator::CreateNSWAdd(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return add;
        } else {
            return NULL;
        }
    } else if (opcodeName == "sub") {
        if (constantExpr == llvm::ConstantExpr::getSub(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // no flag
            llvm::BinaryOperator *sub = llvm::BinaryOperator::CreateNUWSub(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            sub->setHasNoUnsignedWrap(false);
            return sub;
        } else if (constantExpr == llvm::ConstantExpr::getNUWSub(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nuw flag
            llvm::BinaryOperator *sub = llvm::BinaryOperator::CreateNUWSub(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return sub;
        } else if (constantExpr == llvm::ConstantExpr::getNSWSub(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nsw flag
            llvm::BinaryOperator *sub = llvm::BinaryOperator::CreateNSWSub(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return sub;
        } else {
            return NULL;
        }
    } else if (opcodeName == "mul") {
        if (constantExpr == llvm::ConstantExpr::getMul(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // no flag
            llvm::BinaryOperator *mul = llvm::BinaryOperator::CreateNUWMul(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            mul->setHasNoUnsignedWrap(false);
            return mul;
        } else if (constantExpr == llvm::ConstantExpr::getNUWMul(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nuw flag
            llvm::BinaryOperator *mul = llvm::BinaryOperator::CreateNUWMul(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return mul;
        } else if (constantExpr == llvm::ConstantExpr::getNSWMul(constantExpr->getOperand(0), constantExpr->getOperand(1))) {
            // nsw flag
            llvm::BinaryOperator *mul = llvm::BinaryOperator::CreateNSWMul(constantExpr->getOperand(0), constantExpr->getOperand(1), constantExpr->getName(), before);
            return mul;
        } else {
            return NULL;
        }
    } else if (opcodeName == "trunc") {
        llvm::Instruction *instr = new llvm::TruncInst(constantExpr->getOperand(0), constantExpr->getType(), constantExpr->getName(), before);
        return instr;
    } else {
        return NULL;
    }
}

llvm::FunctionPass *createConstantExprEliminatorPass()
{
    return new ConstantExprEliminator();
}
