// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef HIERARCHY_BUILDER_H
#define HIERARCHY_BUILDER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Support/InstVisitor.h>
  #include <llvm/InstrTypes.h>
#else
  #include <llvm/InstVisitor.h>
  #include <llvm/IR/InstrTypes.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <list>
#include <map>

#include "WARN_OFF.h"

class HierarchyBuilder : public llvm::InstVisitor<HierarchyBuilder>
{

#include "WARN_ON.h"

public:
    HierarchyBuilder();
    ~HierarchyBuilder();

    void computeHierarchy(llvm::Module *module);

    bool isCyclic(void);
    std::list<std::list<llvm::Function*> > getSccs();

    std::list<llvm::Function*> getTransitivelyCalledFunctions(llvm::Function *f);

    void visitCallInst(llvm::CallInst &I);

private:
    std::map<llvm::Function*, unsigned int> m_functionIdx;
    std::map<unsigned int, llvm::Function*> m_idxFunction;
    unsigned int getIdx(llvm::Function *f);
    llvm::Function *getFunction(unsigned int idx);

    unsigned int m_numFunctions;
    std::list<llvm::Function*> m_functions;

    bool *m_calls;

    void makeTransitive(void);

    void printHierarchy(void);

    void tarjan(unsigned int v, std::map<unsigned int, unsigned int> &indexMap, std::map<unsigned int, unsigned int> &lowlinkMap, unsigned int &index, std::list<unsigned int> &S, std::list<std::list<llvm::Function*> > &SCCs);

private:
    HierarchyBuilder(const HierarchyBuilder &);
    HierarchyBuilder &operator=(const HierarchyBuilder &);

};

#endif // HIERARCHY_BUILDER_H
