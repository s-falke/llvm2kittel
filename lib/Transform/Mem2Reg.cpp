// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

// Contains code from LLVM's Mem2Reg.cpp

#include "llvm2kittel/Transform/Mem2Reg.h"

// llvm includes
#include <llvm/Transforms/Utils/PromoteMemToReg.h>

void Mem2RegPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
#if LLVM_VERSION < VERSION(3, 5)
    AU.addRequired<llvm::DominatorTree>();
#else
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
#endif
    AU.setPreservesCFG();
    AU.addPreserved<llvm::UnifyFunctionExitNodes>();
    AU.addPreservedID(llvm::LowerSwitchID);
    AU.addPreservedID(llvm::LowerInvokePassID);
}

char Mem2RegPass::ID = 0;

bool Mem2RegPass::runOnFunction(llvm::Function &F)
{
    std::vector<llvm::AllocaInst*> Allocas;
    std::vector<llvm::Instruction*> nondefCalls;

    llvm::BasicBlock &BB = F.getEntryBlock();

    bool Changed = false;

#if LLVM_VERSION < VERSION(3, 5)
    llvm::DominatorTree &DT = getAnalysis<llvm::DominatorTree>();
#else
    llvm::DominatorTree &DT = getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
#endif

    while (true) {
        Allocas.clear();

        // Find allocas that are safe to promote, by looking at all instructions in
        // the entry node
        for (llvm::BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I) {
            // Is it an alloca?
            if (llvm::AllocaInst *AI = llvm::dyn_cast<llvm::AllocaInst>(I)) {
                if (isAllocaPromotable(AI)) {
                    Allocas.push_back(AI);
                }
            }
        }

        if (Allocas.empty()) {
            break;
        }

        // explicitely initialize them
        for (std::vector<llvm::AllocaInst*>::iterator i = Allocas.begin(), e = Allocas.end(); i != e; ++i) {
            llvm::AllocaInst *alloca = *i;
            llvm::Function *nondefFunction = m_nondefFactory.getNondefFunction(alloca->getAllocatedType());
            llvm::Instruction *nondefValue = llvm::CallInst::Create(nondefFunction, "", alloca);
            nondefCalls.push_back(nondefValue);
            alloca->moveBefore(nondefValue);
            llvm::Instruction *store = new llvm::StoreInst(nondefValue, alloca, nondefValue);
            nondefValue->moveBefore(store);
        }

        PromoteMemToReg(Allocas, DT);
        Changed = true;
    }

    // remove nondef calls that are not used
    for (std::vector<llvm::Instruction*>::iterator i = nondefCalls.begin(), e = nondefCalls.end(); i != e; ++i) {
        llvm::Instruction *inst = *i;
        if (inst->getNumUses() == 0) {
            inst->eraseFromParent();
        }
    }

    return Changed;
}

llvm::FunctionPass *createMem2RegPass(NondefFactory &nondefFactory)
{
#if LLVM_VERSION < VERSION(3, 5)
    llvm::initializeDominatorTreePass(*llvm::PassRegistry::getPassRegistry());
#else
    llvm::initializeDominatorTreeWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
#endif
    return new Mem2RegPass(nondefFactory);
}
