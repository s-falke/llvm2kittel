// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef MEMORY_ANALYZER_H
#define MEMORY_ANALYZER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Support/InstVisitor.h>
#elif LLVM_VERSION < VERSION(3, 5)
  #include <llvm/InstVisitor.h>
#else
  #include <llvm/IR/InstVisitor.h>
#endif
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/GlobalVariable.h>
  #include <llvm/InstrTypes.h>
  #include <llvm/Function.h>
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/GlobalVariable.h>
  #include <llvm/IR/InstrTypes.h>
  #include <llvm/IR/Function.h>
  #include <llvm/IR/Instructions.h>
#endif
#include <llvm/Pass.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include "WARN_ON.h"

// C++ includes
#include <map>
#include <set>
#include <utility>

typedef std::pair<std::set<llvm::GlobalVariable*>, std::set<llvm::GlobalVariable*> > MayMustPair;
typedef std::map<llvm::Instruction*, MayMustPair> MayMustMap;

#include "WARN_OFF.h"

class MemoryAnalyzer : public llvm::InstVisitor<MemoryAnalyzer>, public llvm::FunctionPass
{

#include "WARN_ON.h"

public:
    MemoryAnalyzer();
    ~MemoryAnalyzer();

    bool runOnFunction(llvm::Function &function);

    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);

#if LLVM_VERSION < VERSION(4, 0)
    virtual const char *getPassName() const
#else
    virtual llvm::StringRef getPassName() const
#endif
    {
        return "Obtain memory alias analysis results";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

    static char ID;

    MayMustMap getMayMustMap()
    {
        return m_map;
    }

    std::set<llvm::GlobalVariable*> getMayZap()
    {
        return m_mayZap;
    }

private:
    std::set<llvm::GlobalVariable*> m_globals;
    llvm::AliasAnalysis *m_aa;
    MayMustMap m_map;
    std::set<llvm::GlobalVariable*> m_mayZap;

private:
    MemoryAnalyzer(const MemoryAnalyzer &);
    MemoryAnalyzer &operator=(const MemoryAnalyzer &);

};

MemoryAnalyzer *createMemoryAnalyzerPass();

#endif // MEMORY_ANALYZER_H
