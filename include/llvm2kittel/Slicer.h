// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef SLICER_H
#define SLICER_H

#include "llvm2kittel/Util/Ref.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Function.h>
#else
  #include <llvm/IR/Function.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <list>
#include <map>
#include <set>

class Rule;

class Slicer
{

public:
    Slicer(llvm::Function *F, std::set<std::string> phiVars);
    ~Slicer();

    std::list<ref<Rule> > sliceUsage(std::list<ref<Rule> > rules);

    std::list<ref<Rule> > sliceConstraint(std::list<ref<Rule> > rules);

    std::list<ref<Rule> > sliceDefined(std::list<ref<Rule> > rules);

    std::list<ref<Rule> > sliceStillUsed(std::list<ref<Rule> > rules, bool conservative);

    std::list<ref<Rule> > sliceTrivialNondefConstraints(std::list<ref<Rule> > rules);

    std::list<ref<Rule> > sliceDuplicates(std::list<ref<Rule> > rules);

private:
    llvm::Function *m_F;

    std::set<unsigned int> getSet(unsigned int size);

    std::string getVar(std::string name);
    std::string getEval(std::string startstop);

    std::map<std::string, unsigned int> m_functionIdx;
    std::map<unsigned int, std::string> m_idxFunction;
    unsigned int getIdxFunction(std::string f);
    std::string getFunction(unsigned int idx);

    unsigned int m_numFunctions;
    std::set<std::string> m_functions;
    bool *m_preceeds;
    void setUpPreceeds(std::list<ref<Rule> > rules);
    void makePreceedsTransitive(void);
    bool *m_calls;
    void setUpCalls(std::list<ref<Rule> > rules);
    void makeCallsTransitive(void);

    std::map<std::string, unsigned int> m_varIdx;
    std::map<unsigned int, std::string> m_idxVar;
    unsigned int getIdxVar(std::string v);
    std::string getVar(unsigned int idx);

    unsigned int m_numVars;
    std::set<std::string> m_vars;
    bool *m_depends;
    void makeDependsTransitive(void);

    std::map<std::string, std::set<std::string> > m_defined;
    std::map<std::string, std::set<std::string> > m_stillUsed;

    std::set<std::string> getKnownVars(std::string f);

    std::set<std::string> getStillUsed(std::string f);

    std::set<unsigned int> getNotNeeded(std::string f, std::list<std::string> vars);

    std::set<std::string> computeReachableFuns(std::list<ref<Rule> > rules);

    bool isRecursiveCall(std::string f);
    bool isNondef(std::string v);

    std::set<std::string> m_phiVars;

private:
    Slicer(const Slicer &);
    Slicer &operator=(const Slicer &);

};

#endif // SLICER_H
