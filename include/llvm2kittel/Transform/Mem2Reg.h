// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef MEM_2_REG_H
#define MEM_2_REG_H

#include "llvm2kittel/Transform/NondefFactory.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Analysis/Dominators.h>
#else
  #include <llvm/IR/Dominators.h>
#endif
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Function.h>
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Function.h>
  #include <llvm/IR/Instructions.h>
#endif
#include <llvm/Pass.h>
#include "WARN_ON.h"

class Mem2RegPass : public llvm::FunctionPass
{

public:
    Mem2RegPass(NondefFactory &nondefFactory)
      : FunctionPass(ID), m_nondefFactory(nondefFactory)
    {}

    bool runOnFunction(llvm::Function &function);

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "llvm2KITTeL's mem2reg";
    }

    static char ID;

private:
    NondefFactory &m_nondefFactory;

};

llvm::FunctionPass *createMem2RegPass(NondefFactory &factory);

#endif // MEM_2_REG_H
