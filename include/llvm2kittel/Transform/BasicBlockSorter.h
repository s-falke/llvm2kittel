// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef BASIC_BLOCK_SORTER_H
#define BASIC_BLOCK_SORTER_H

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
#include <llvm/Pass.h>
#include "WARN_ON.h"

// C++ includes
#include <vector>
#include <set>

class BasicBlockSorter : public llvm::FunctionPass
{

public:
    BasicBlockSorter();
    ~BasicBlockSorter();

    bool runOnFunction(llvm::Function &function);

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Bring basic blocks into topological order";
    }

    static char ID;

private:
    void visitBasicBlock(std::vector<llvm::BasicBlock*> &blockList, std::set<llvm::BasicBlock*> &visitedBlocks, llvm::BasicBlock *BB);

    std::vector<llvm::BasicBlock*> sortBasicBlocks(llvm::Function &function);

};

llvm::FunctionPass *createBasicBlockSorterPass();

#endif // BASIC_BLOCK_SORTER_H
