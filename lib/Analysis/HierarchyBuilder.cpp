// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/HierarchyBuilder.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/CallSite.h>
#else
  #include <llvm/IR/CallSite.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <iostream>
#include <cstdlib>

HierarchyBuilder::HierarchyBuilder()
  : m_functionIdx(),
    m_idxFunction(),
    m_numFunctions(0),
    m_functions(),
    m_calls(NULL)
{}

HierarchyBuilder::~HierarchyBuilder()
{
    delete [] m_calls;
}

void HierarchyBuilder::computeHierarchy(llvm::Module *module)
{
    m_functionIdx.clear();
    m_functions.clear();
    if (m_calls != NULL) {
        delete [] m_calls;
    }
    for (llvm::Module::iterator i = module->begin(), e = module->end(); i != e; ++i) {
        if (!i->isDeclaration()) {
            m_functionIdx.insert(std::make_pair(&*i, m_numFunctions));
            m_idxFunction.insert(std::make_pair(m_numFunctions, &*i));
            m_functions.push_back(&*i);
            ++m_numFunctions;
        }
    }
    m_calls = new bool[m_numFunctions * m_numFunctions];
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        for (unsigned int j = 0; j < m_numFunctions; ++j) {
            m_calls[i + m_numFunctions * j] = false;
        }
    }
    visit(module);
    makeTransitive();
/*
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        std::cout << getFunction(i)->getName().str() << " calls ";
        std::list<llvm::Function*> called = getTransitivelyCalledFunctions(getFunction(i));
        if (called.size() == 0) {
            std::cout << "NONE" << std::endl;
        } else {
            for (std::list<llvm::Function*>::iterator fi = called.begin(), fe = called.end(); fi != fe; ) {
                std::cout << (*fi)->getName().str();
                if (++fi != fe) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
    }
*/
}

void HierarchyBuilder::printHierarchy(void)
{
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        for (unsigned int j = 0; j < m_numFunctions; ++j) {
            std::cout << m_calls[i + m_numFunctions * j];
            if (j + 1 != m_numFunctions) {
                std::cout << ' ';
            }
        }
        std::cout << std::endl;
    }
}

void HierarchyBuilder::makeTransitive(void)
{
    for (unsigned int y = 0; y < m_numFunctions; ++y) {
        for (unsigned int x = 0; x < m_numFunctions; ++x) {
            if (m_calls[x + m_numFunctions * y]) {
                // x calls y
                for (unsigned int j = 0; j < m_numFunctions; ++j) {
                    if (m_calls[y + m_numFunctions * j]) {
                        // y calls j --> x calls j
                        m_calls[x + m_numFunctions * j] = true;
                    }
                }
            }
        }
    }
}

bool HierarchyBuilder::isCyclic(void)
{
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        if (m_calls[i + m_numFunctions * i]) {
            return true;
        }
    }
    return false;
}

std::list<std::list<llvm::Function*> > HierarchyBuilder::getSccs()
{
    std::map<unsigned int, unsigned int> indexMap;
    std::map<unsigned int, unsigned int> lowlinkMap;
    std::list<std::list<llvm::Function*> > res;

    unsigned int index = 0;
    std::list<unsigned int> S;
    for (unsigned int v = 0; v < m_numFunctions; ++v) {
        if (indexMap.find(v) == indexMap.end()) {
            tarjan(v, indexMap, lowlinkMap, index, S, res);
        }
    }
    return res;
}

static bool contains(std::list<unsigned int> &S, unsigned int v)
{
    for (std::list<unsigned int>::iterator i = S.begin(), e = S.end(); i != e; ++i) {
        if (*i == v) {
            return true;
        }
    }
    return false;
}

void HierarchyBuilder::tarjan(unsigned int v, std::map<unsigned int, unsigned int> &indexMap, std::map<unsigned int, unsigned int> &lowlinkMap, unsigned int &index, std::list<unsigned int> &S, std::list<std::list<llvm::Function*> > &SCCs)
{
    indexMap.insert(std::make_pair(v, index));
    lowlinkMap.insert(std::make_pair(v, index));
    index = index + 1;
    S.push_front(v);
    for (unsigned int vv = 0; vv < m_numFunctions; ++vv) {
        if (m_calls[v + m_numFunctions * vv]) {
            if (indexMap.find(vv) == indexMap.end()) {
                tarjan(vv, indexMap, lowlinkMap, index, S, SCCs);
                unsigned int vlow = lowlinkMap.find(v)->second;
                unsigned int vvlow = lowlinkMap.find(vv)->second;
                if (vvlow < vlow) {
                    lowlinkMap.erase(v);
                    lowlinkMap.insert(std::make_pair(v, vvlow));
                }
            } else if (contains(S, vv)) {
                unsigned int vlow = lowlinkMap.find(v)->second;
                unsigned int vvlow = lowlinkMap.find(vv)->second;
                if (vvlow < vlow) {
                    lowlinkMap.erase(v);
                    lowlinkMap.insert(std::make_pair(v, vvlow));
                }
            }
        }
    }
    if (lowlinkMap.find(v)->second == indexMap.find(v)->second) {
        std::list<llvm::Function*> component;
        unsigned int n;
        do {
            n = *(S.begin());
            S.pop_front();
            component.push_front(getFunction(n));
        } while(n != v);
        SCCs.push_back(component);
    }
}

std::list<llvm::Function*> HierarchyBuilder::getTransitivelyCalledFunctions(llvm::Function *f)
{
    std::list<llvm::Function*> res;
    unsigned int idx = getIdx(f);
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        if (m_calls[idx + m_numFunctions * i]) {
            res.push_back(getFunction(i));
        }
    }
    return res;
}

llvm::Function *HierarchyBuilder::getFunction(unsigned int idx)
{
    std::map<unsigned int, llvm::Function*>::iterator found = m_idxFunction.find(idx);
    if (found == m_idxFunction.end()) {
        std::cerr << "Internal error in HierarchyBuilder::getFunction (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

unsigned int HierarchyBuilder::getIdx(llvm::Function *f)
{
    std::map<llvm::Function*, unsigned int>::iterator found = m_functionIdx.find(f);
    if (found == m_functionIdx.end()) {
        std::cerr << "Internal error in HierarchyBuilder::getIdx (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

void HierarchyBuilder::visitCallInst(llvm::CallInst &I)
{
    llvm::CallSite callSite(&I);
    llvm::Function *calledFunction = callSite.getCalledFunction();
    if (calledFunction != NULL) {
        if (calledFunction->isDeclaration()) {
            return;
        }
        m_calls[getIdx(I.getParent()->getParent()) + m_numFunctions * getIdx(calledFunction)] = true;
        return;
    }
    // calledFunction == NULL
    llvm::Value *calledValue = I.getCalledValue();
    const llvm::Type *calledValueType = calledValue->getType();
    for (std::list<llvm::Function*>::iterator f = m_functions.begin(), fe = m_functions.end(); f != fe; ++f) {
        llvm::Function *F = *f;
        if (F->getType() == calledValueType) {
            m_calls[getIdx(I.getParent()->getParent()) + m_numFunctions * getIdx(F)] = true;
        }
    }
}
