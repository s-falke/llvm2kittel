// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/BasicBlockSorter.h"

BasicBlockSorter::BasicBlockSorter()
  : llvm::FunctionPass(ID)
{}

BasicBlockSorter::~BasicBlockSorter()
{}

bool BasicBlockSorter::runOnFunction(llvm::Function &function)
{
    std::vector<llvm::BasicBlock*> revSorted = sortBasicBlocks(function);

    if (revSorted.empty()) {
        return false;
    } else {
        for (std::vector<llvm::BasicBlock*>::iterator i = revSorted.begin(), e = revSorted.end(); i + 1 != e; ++i) {
            (*(i + 1))->moveBefore(*i);
        }
        return true;
    }
}

// recursive function for depth-first post-order iteration over a directed graph of basic blocks
void BasicBlockSorter::visitBasicBlock(std::vector<llvm::BasicBlock *> &blockList, std::set<llvm::BasicBlock *> &visitedBlocks, llvm::BasicBlock *BB)
{
    const llvm::TerminatorInst *term = BB->getTerminator();

    visitedBlocks.insert(BB);

    // for each successor, from the last to the first, call visitBasicBlock in that successor
    unsigned int idx = term->getNumSuccessors();
    while (idx != 0) {
        --idx;
        if (visitedBlocks.find(term->getSuccessor(idx)) == visitedBlocks.end()) {
            visitBasicBlock(blockList, visitedBlocks, term->getSuccessor(idx));
        }
    }

    blockList.push_back(BB);
}

std::vector<llvm::BasicBlock*> BasicBlockSorter::sortBasicBlocks(llvm::Function &function)
{
    std::vector<llvm::BasicBlock*> ret;
    std::set<llvm::BasicBlock*> visited;

    llvm::BasicBlock &entryBlock = function.getEntryBlock();

    visitBasicBlock(ret, visited, &entryBlock);

    return ret;
}

char BasicBlockSorter::ID = 0;

llvm::FunctionPass *createBasicBlockSorterPass()
{
    return new BasicBlockSorter();
}
