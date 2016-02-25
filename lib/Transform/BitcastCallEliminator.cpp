// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/BitcastCallEliminator.h"
#include "llvm2kittel/Util/Version.h"

char BitcastCallEliminator::ID = 0;

BitcastCallEliminator::BitcastCallEliminator()
  : llvm::FunctionPass(ID)
{}

bool BitcastCallEliminator::runOnFunction(llvm::Function &function)
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
                if (llvm::isa<llvm::CallInst>(inst) && llvm::cast<llvm::CallInst>(inst)->getCalledFunction() == NULL) {
                    llvm::CallInst *callInst = llvm::cast<llvm::CallInst>(inst);
                    llvm::Value *calledValue = callInst->getCalledValue();
                    llvm::Value *bareCalledValue = calledValue->stripPointerCasts();
                    if (llvm::isa<llvm::Function>(bareCalledValue)) {
                        const llvm::FunctionType *calledType = llvm::cast<llvm::FunctionType>(llvm::cast<llvm::PointerType>(calledValue->getType())->getContainedType(0));
                        const llvm::FunctionType *calleeType = llvm::cast<llvm::Function>(bareCalledValue)->getFunctionType();
                        if (calledType->getReturnType() == calleeType->getReturnType()) {
                            if (argsMatch(calleeType, callInst)) {
                                std::vector<llvm::Value*> args;
                                unsigned int numArgs = callInst->getNumArgOperands();
                                for (unsigned int k = 0; k < numArgs; ++k) {
                                    args.push_back(callInst->getArgOperand(k));
                                }
#if LLVM_VERSION < VERSION(3, 0)
                                llvm::CallInst *newCall = llvm::CallInst::Create(bareCalledValue, args.begin(), args.end(), "", inst);
#else
                                llvm::CallInst *newCall = llvm::CallInst::Create(bareCalledValue, args, "", inst);
#endif
                                inst->replaceAllUsesWith(newCall);
                                llvm::StringRef name = inst->getName();
                                inst->eraseFromParent();
                                newCall->setName(name);
                                res = true;
                                doLoop = true;
                                bbchanged = true;
                            }
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

bool BitcastCallEliminator::argsMatch(const llvm::FunctionType *calleeType, llvm::CallInst *inst)
{
    if (calleeType->getNumParams() != inst->getNumArgOperands()) {
        return false;
    }

    unsigned int i = 0;
    for (llvm::FunctionType::param_iterator it = calleeType->param_begin(), et = calleeType->param_end(); it != et; ++it, ++i) {
        if (*it != inst->getArgOperand(i)->getType()) {
            return false;
        }
    }
    return true;
}

llvm::FunctionPass *createBitcastCallEliminatorPass()
{
    return new BitcastCallEliminator();
}
