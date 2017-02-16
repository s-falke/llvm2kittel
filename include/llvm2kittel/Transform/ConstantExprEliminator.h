// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONSTANT_EXPR_ELIMINATOR_H
#define CONSTANT_EXPR_ELIMINATOR_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Constants.h>
  #include <llvm/Function.h>
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Constants.h>
  #include <llvm/IR/Function.h>
  #include <llvm/IR/Instructions.h>
#endif
#include <llvm/Pass.h>
#include "WARN_ON.h"

class ConstantExprEliminator : public llvm::FunctionPass
{

public:
    ConstantExprEliminator();

    bool runOnFunction(llvm::Function &function);

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "ConstantExpr elimination pass";
    }

    static char ID;

private:
    llvm::Instruction *replaceConstantExpr(llvm::ConstantExpr *constExpr, llvm::Instruction *before);

    llvm::Instruction *getBeforePoint(llvm::Instruction *instr, unsigned int j);

};

llvm::FunctionPass *createConstantExprEliminatorPass();

#endif // CONSTANT_EXPR_ELIMINATOR_H
