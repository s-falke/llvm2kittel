// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/LoopConditionBlocksCollector.h"

// C++ includes
#include <iostream>

LoopConditionBlocksCollector::LoopConditionBlocksCollector()
  : llvm::LoopPass(ID),
    m_conditionBlocks()
{}

LoopConditionBlocksCollector::~LoopConditionBlocksCollector()
{}

void LoopConditionBlocksCollector::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
#if LLVM_VERSION < VERSION(3, 7)
    AU.addRequired<llvm::LoopInfo>();
#else
    AU.addRequired<llvm::LoopInfoWrapperPass>();
#endif
}

bool LoopConditionBlocksCollector::runOnLoop(llvm::Loop *L, llvm::LPPassManager&)
{
    llvm::BasicBlock *header = L->getHeader();
    llvm::BasicBlock *latch = L->getLoopLatch();
    if (header != NULL && L->isLoopExiting(header)) {
        m_conditionBlocks.insert(header);
    }
    if (latch != NULL && L->isLoopExiting(latch)) {
        m_conditionBlocks.insert(latch);
    }
    return false;
}

char LoopConditionBlocksCollector::ID = 0;

LoopConditionBlocksCollector *createLoopConditionBlocksCollectorPass()
{
    return new LoopConditionBlocksCollector();
}
