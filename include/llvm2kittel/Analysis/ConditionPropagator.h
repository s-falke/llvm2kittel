// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONDITION_PROPAGATOR_H
#define CONDITION_PROPAGATOR_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/BasicBlock.h>
  #include <llvm/Function.h>
  #include <llvm/Value.h>
#else
  #include <llvm/IR/BasicBlock.h>
  #include <llvm/IR/Function.h>
  #include <llvm/IR/Value.h>
#endif
#include <llvm/Pass.h>
#include "WARN_ON.h"

// C++ includes
#include <map>
#include <set>
#include <utility>

typedef std::pair<std::set<llvm::Value*>, std::set<llvm::Value*> > TrueFalsePair;
typedef std::map<llvm::BasicBlock*, TrueFalsePair> TrueFalseMap;

class ConditionPropagator : public llvm::FunctionPass
{

public:
    ConditionPropagator(bool debug, bool onlyLoopConditions, std::set<llvm::BasicBlock*> lcbs);

    bool runOnFunction(llvm::Function &F);

    TrueFalseMap getTrueFalseMap()
    {
        return m_map;
    }

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "ConditionPropagator";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

    static char ID;

private:
    void printSet(std::set<llvm::Value*> X);

    bool isPred(llvm::BasicBlock *bb1, llvm::BasicBlock *bb2);

    std::set<llvm::Value*> intersect(std::set<llvm::Value*> X, std::set<llvm::Value*> Y);

    TrueFalseMap m_map;

    bool m_debug;

    bool m_onlyLoopConditions;
    std::set<llvm::BasicBlock*> m_lcbs;

private:
    ConditionPropagator(const ConditionPropagator &);
    ConditionPropagator &operator=(const ConditionPropagator &);

};

ConditionPropagator *createConditionPropagatorPass(bool debug, bool onlyLoopConditions, std::set<llvm::BasicBlock*> lcbs);

#endif // CONDITION_PROPAGATOR_H
