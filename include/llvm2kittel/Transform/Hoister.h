// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef HOISTER_H
#define HOISTER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Function.h>
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Function.h>
  #include <llvm/IR/Instructions.h>
#endif
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Analysis/Dominators.h>
#else
  #include <llvm/IR/Dominators.h>
#endif
#include <llvm/Pass.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include "WARN_ON.h"

class Hoister : public llvm::LoopPass
{

public:
    Hoister();

    bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM);

#if LLVM_VERSION >= VERSION(3, 3)
    using llvm::Pass::doFinalization;
#endif
    bool doFinalization();

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Selective Hoister";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

    static char ID;

private:
    bool changed;

    llvm::AliasSetTracker *CurAST;
    std::map<llvm::Loop*, llvm::AliasSetTracker*> LoopToAliasMap;

    void HoistRegion(llvm::Loop *L, llvm::DomTreeNode *N, llvm::AliasAnalysis *AA);

    bool doesNotAccessMemory(llvm::Function *F);

    bool canHoistInst(llvm::Instruction &I, llvm::AliasAnalysis *AA);

    bool inSubLoop(llvm::Loop *L, llvm::BasicBlock *BB);

    bool isLoopInvariantInst(llvm::Loop *L, llvm::Instruction &I);

    void hoist(llvm::BasicBlock *preheader, llvm::Instruction &I);

private:
    Hoister(const Hoister &);
    Hoister &operator=(const Hoister &);

};

llvm::LoopPass *createHoisterPass();

#endif // HOISTER_H
