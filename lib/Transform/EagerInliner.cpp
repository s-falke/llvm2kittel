// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/EagerInliner.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Type.h>
#else
  #include <llvm/IR/Type.h>
#endif

EagerInliner::EagerInliner()
  : llvm::Inliner(ID)
{}

EagerInliner::~EagerInliner()
{}

llvm::InlineCost EagerInliner::getInlineCost(llvm::CallSite)
{
    return llvm::InlineCost::getAlways();
}

float EagerInliner::getInlineFudgeFactor(llvm::CallSite)
{
    return 1.0;
}

void EagerInliner::resetCachedCostInfo(llvm::Function*)
{}

void EagerInliner::growCachedCostInfo(llvm::Function *, llvm::Function *)
{}

llvm::Pass *createEagerInlinerPass()
{
#if LLVM_VERSION < VERSION(3, 4)
    llvm::initializeCallGraphAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
#elif LLVM_VERSION == VERSION(3, 4)
    llvm::initializeCallGraphPass(*llvm::PassRegistry::getPassRegistry());
#else
    llvm::initializeCallGraphWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
#endif
#if LLVM_VERSION >= VERSION(3, 6) && LLVM_VERSION < VERSION(3, 8)
    llvm::initializeAliasAnalysisAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
    llvm::initializeAssumptionCacheTrackerPass(*llvm::PassRegistry::getPassRegistry());
#elif LLVM_VERSION >= VERSION(3, 8)
    llvm::initializeAAResultsWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
    llvm::initializeAssumptionCacheTrackerPass(*llvm::PassRegistry::getPassRegistry());
#endif
    return new EagerInliner();
}

char EagerInliner::ID = 0;
