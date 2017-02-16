// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef STRENGTH_INCREASER_H
#define STRENGTH_INCREASER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Instructions.h>
#endif
#include <llvm/Pass.h>
#include "WARN_ON.h"

class StrengthIncreaser : public llvm::BasicBlockPass
{

public:
    StrengthIncreaser();

    bool runOnBasicBlock(llvm::BasicBlock &bb);

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Strength increaser pass";
    }

    static char ID;
};

llvm::BasicBlockPass *createStrengthIncreaserPass();

#endif // STRENGHT_INCREASER_H
