// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef BITCAST_CALL_ELIMINATOR_H
#define BITCAST_CALL_ELIMINATOR_H

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

class BitcastCallEliminator : public llvm::FunctionPass
{

public:
    BitcastCallEliminator();

    bool runOnFunction(llvm::Function &function);

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Bitcast call elimination pass";
    }

    static char ID;

private:
    bool argsMatch(const llvm::FunctionType *calleeType, llvm::CallInst *inst);
};

llvm::FunctionPass *createBitcastCallEliminatorPass();

#endif // BITCAST_CALL_ELIMINATOR_H
