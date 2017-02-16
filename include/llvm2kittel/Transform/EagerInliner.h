// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef EAGER_INLINER_H
#define EAGER_INLINER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Function.h>
#else
  #include <llvm/IR/Function.h>
#endif
#include <llvm/Analysis/InlineCost.h>
#if LLVM_VERSION < VERSION(4, 0)
#include <llvm/Transforms/IPO/InlinerPass.h>
#else
#include <llvm/Transforms/IPO/Inliner.h>
#endif
#include "WARN_ON.h"

#if LLVM_VERSION < VERSION(4, 0)
class EagerInliner : public llvm::Inliner
#else
class EagerInliner : public llvm::LegacyInlinerBase
#endif
{

public:
    EagerInliner();
    ~EagerInliner();

    llvm::InlineCost getInlineCost(llvm::CallSite callSite);

    float getInlineFudgeFactor(llvm::CallSite callSite);

    void resetCachedCostInfo(llvm::Function *function);
    void growCachedCostInfo(llvm::Function *caller, llvm::Function *callee);

    static char ID;

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Eager function inliner";
    }

private:
    EagerInliner(const EagerInliner &);
    EagerInliner &operator=(const EagerInliner &);

};

llvm::Pass *createEagerInlinerPass();

#endif // EAGER_INLINER_H
