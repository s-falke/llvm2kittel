// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/ExtremeInliner.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Type.h>
#else
  #include <llvm/IR/Type.h>
#endif
#include "WARN_ON.h"

ExtremeInliner::ExtremeInliner(llvm::Function *function, bool inlineVoids)
  : llvm::Inliner(ID),
    m_function(function),
    m_inlineVoids(inlineVoids)
{}

ExtremeInliner::~ExtremeInliner()
{}

llvm::InlineCost ExtremeInliner::getInlineCost(llvm::CallSite callSite)
{
    llvm::Function *caller = callSite.getCaller();

    if (caller != m_function) {
        return llvm::InlineCost::getNever();
    }

    const llvm::Type *type = callSite.getType();
    if (!m_inlineVoids && (type->isVoidTy() || type->isFloatTy() || type->isDoubleTy() || llvm::isa<llvm::PointerType>(type))) {
        return llvm::InlineCost::getNever();
    }

    return llvm::InlineCost::getAlways();
}

float ExtremeInliner::getInlineFudgeFactor(llvm::CallSite)
{
    return 1.0;
}

void ExtremeInliner::resetCachedCostInfo(llvm::Function*)
{}

void ExtremeInliner::growCachedCostInfo(llvm::Function *, llvm::Function *)
{}

llvm::Pass *createExtremeInlinerPass(llvm::Function *function, bool inlineVoids)
{
#if LLVM_VERSION < VERSION(3, 4)
    llvm::initializeCallGraphAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
#elif LLVM_VERSION == VERSION(3, 4)
    llvm::initializeCallGraphPass(*llvm::PassRegistry::getPassRegistry());
#else
    llvm::initializeCallGraphWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
#endif
#if LLVM_VERSION >= VERSION(3, 6)
    llvm::initializeAliasAnalysisAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
    llvm::initializeAssumptionTrackerPass(*llvm::PassRegistry::getPassRegistry());
#endif
    return new ExtremeInliner(function, inlineVoids);
}

char ExtremeInliner::ID = 0;
