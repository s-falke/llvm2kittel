// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef LOOP_CONDITION_EXPLICITIZER_H
#define LOOP_CONDITION_EXPLICITIZER_H

#include "llvm2kittel/Util/quadruple.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/BasicBlock.h>
  #include <llvm/Value.h>
#else
  #include <llvm/IR/BasicBlock.h>
  #include <llvm/IR/Value.h>
#endif
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include "WARN_ON.h"

// C++ includes
#include <map>
#include <set>
#include <utility>

typedef std::map<llvm::BasicBlock*, std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > > ConditionMap;

class LoopConditionExplicitizer : public llvm::LoopPass
{

public:
    LoopConditionExplicitizer(bool debug);

    bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &);

    ConditionMap getConditionMap()
    {
        return m_map;
    }

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "LoopConditionExplicitizer";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

    static char ID;

private:
    void printSet(std::set<llvm::Value*> X);

    ConditionMap m_map;

    bool m_debug;

    void insertForAll(llvm::Value* val1, llvm::CmpInst::Predicate, llvm::Value* val2, llvm::Value* off, std::vector<llvm::Loop*> &loops);
    void insertForAll(llvm::Value* val1, llvm::CmpInst::Predicate, llvm::Value* val2, llvm::Value* off, llvm::Loop* loop);
    void insert(llvm::Value* val1, llvm::CmpInst::Predicate, llvm::Value* val2, llvm::Value* off, llvm::BasicBlock* bb);

private:
    LoopConditionExplicitizer(const LoopConditionExplicitizer &);
    LoopConditionExplicitizer &operator=(const LoopConditionExplicitizer &);

};

LoopConditionExplicitizer *createLoopConditionExplicitizerPass(bool debug);

#endif // LOOP_CONDITION_EXPLICITIZER_H
