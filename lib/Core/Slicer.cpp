// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Slicer.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/InstrTypes.h>
  #include <llvm/Module.h>
#else
  #include <llvm/IR/InstrTypes.h>
  #include <llvm/IR/Module.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <queue>
#include <vector>

Slicer::Slicer(llvm::Function *F, std::set<std::string> phiVars)
  : m_F(F),
    m_functionIdx(),
    m_idxFunction(),
    m_numFunctions(0),
    m_functions(),
    m_preceeds(NULL),
    m_calls(NULL),
    m_varIdx(),
    m_idxVar(),
    m_numVars(0),
    m_vars(),
    m_depends(NULL),
    m_defined(),
    m_stillUsed(),
    m_phiVars(phiVars)
{}

Slicer::~Slicer()
{
    delete [] m_preceeds;
    delete [] m_calls;
    delete [] m_depends;
}

/*
static void printRules(std::string header, std::list<ref<Rule> > rules)
{
    std::cout << header << std::endl;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        std::cout << (*i)->toString() << std::endl;
    }
    std::cout << header << std::endl;
}
*/

// Usage
std::set<unsigned int> Slicer::getSet(unsigned int size)
{
    std::set<unsigned int> res;
    for (unsigned int i = 0; i < size; ++i) {
        res.insert(i);
    }
    return res;
}

std::list<ref<Rule> > Slicer::sliceUsage(std::list<ref<Rule> > rules)
{
    if (rules.empty()) {
        return rules;
    }
    std::list<ref<Rule> > res;
    std::vector<std::string> vars;
    std::list<ref<Polynomial> > var_args = (*rules.begin())->getLeft()->getArgs();
    for (std::list<ref<Polynomial> >::iterator i = var_args.begin(), e = var_args.end(); i != e; ++i) {
        ref<Polynomial> tmp = *i;
        vars.push_back(*(tmp->getVariables()->begin()));
    }
    unsigned int arity = static_cast<unsigned int>(vars.size());
    std::set<unsigned int> notNeeded = getSet(arity);
    for (std::list<ref<Rule> >::iterator it = rules.begin(), et = rules.end(); it != et; ++it) {
        ref<Rule> tmp = *it;
        std::set<std::string> *c_vars = tmp->getConstraint()->getVariables();
        std::vector<std::set<std::string>*> rhsVars;
        std::set<std::string> *allRhsVars = NULL;
        if (isRecursiveCall(tmp->getRight()->getFunctionSymbol())) {
            allRhsVars = tmp->getRight()->getVariables();
        } else {
            for (unsigned int i = 0; i < arity; ++i) {
                rhsVars.push_back(tmp->getRight()->getVariables(i));
            }
        }
        unsigned int argc = 0;
        for (std::vector<std::string>::iterator vi = vars.begin(), ve = vars.end(); vi != ve; ++vi, ++argc) {
            std::string var = *vi;
            if (c_vars->find(var) != c_vars->end()) {
                // needed because it occurs in the constraint
                notNeeded.erase(argc);
            } else {
                if (allRhsVars != NULL) {
                    if (allRhsVars->find(var) != allRhsVars->end()) {
                        // needed because it occurs in a recursive call
                        notNeeded.erase(argc);
                    }
                } else {
                    unsigned int rhsc = 0;
                    for (std::vector<std::set<std::string>*>::iterator ri = rhsVars.begin(), re = rhsVars.end(); ri != re; ++ri, ++rhsc) {
                        if (rhsc != argc) {
                            std::set<std::string> *r_vars = *ri;
                            if (r_vars->find(var) != r_vars->end()) {
                                // needed because it occurs in different position in rhs
                                notNeeded.erase(argc);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    // Keep all inputs of integer type
    unsigned int intarg = 0;
    for (llvm::Function::arg_iterator i = m_F->arg_begin(), e = m_F->arg_end(); i != e; ++i) {
        llvm::Argument *arg = i;
        if (llvm::isa<llvm::IntegerType>(arg->getType())) {
            notNeeded.erase(intarg);
            intarg++;
        }
    }
    // keep all globals of integer type
    llvm::Module *module = m_F->getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (llvm::isa<llvm::IntegerType>(globalType)) {
            notNeeded.erase(intarg);
            intarg++;
        }
    }

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        if (!isRecursiveCall(rule->getRight()->getFunctionSymbol())) {
            res.push_back(rule->dropArgs(notNeeded));
        } else {
          res.push_back(Rule::create(rule->getLeft()->dropArgs(notNeeded), rule->getRight(), rule->getConstraint()));
        }
    }
    return res;
}

// Constraint
std::list<ref<Rule> > Slicer::sliceConstraint(std::list<ref<Rule> > rules)
{
    if (rules.empty()) {
        return rules;
    }
    std::list<ref<Rule> > res;
    std::vector<std::string> vars;
    std::list<ref<Polynomial> > var_args = (*rules.begin())->getLeft()->getArgs();
    for (std::list<ref<Polynomial> >::iterator i = var_args.begin(), e = var_args.end(); i != e; ++i) {
        ref<Polynomial> tmp = *i;
        vars.push_back(*(tmp->getVariables()->begin()));
    }
    m_numVars = static_cast<unsigned int>(vars.size());
    std::set<unsigned int> notNeeded = getSet(m_numVars);
    // Keep all inputs of integer type
    unsigned int intarg = 0;
    for (llvm::Function::arg_iterator i = m_F->arg_begin(), e = m_F->arg_end(); i != e; ++i) {
        llvm::Argument *arg = i;
        if (llvm::isa<llvm::IntegerType>(arg->getType())) {
            notNeeded.erase(intarg);
            intarg++;
        }
    }
    // keep all globals of integer type
    llvm::Module *module = m_F->getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (llvm::isa<llvm::IntegerType>(globalType)) {
            notNeeded.erase(intarg);
            intarg++;
        }
    }
    // prepare
    std::set<std::string> c_vars;
    m_depends = new bool[m_numVars * m_numVars];
    for (unsigned int i = 0; i < m_numVars; ++i) {
        for (unsigned int j = 0; j < m_numVars; ++j) {
            m_depends[i + m_numVars * j] = false;
        }
    }
    unsigned int idx = 0;
    for (std::vector<std::string>::iterator i = vars.begin(), e = vars.end(); i != e; ++i) {
        std::string v = *i;
        m_varIdx.insert(std::make_pair(v, idx));
        m_idxVar.insert(std::make_pair(idx, v));
        ++idx;
    }
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::set<std::string> *vv = rule->getConstraint()->getVariables();
        c_vars.insert(vv->begin(), vv->end());
        if (isRecursiveCall(rule->getRight()->getFunctionSymbol())) {
            std::set<std::string> *rv = rule->getRight()->getVariables();
            c_vars.insert(rv->begin(), rv->end());
            continue;
        }
        std::list<ref<Polynomial> > rhsArgs = rule->getRight()->getArgs();
        std::list<ref<Polynomial> >::iterator ri = rhsArgs.begin();
        for (std::vector<std::string>::iterator it = vars.begin(), et = vars.end(); it != et; ++it, ++ri) {
            std::string lvar = *it;
            unsigned int lvarIdx = getIdxVar(lvar);
            ref<Polynomial> inRhs = *ri;
            std::set<std::string> *tmp = inRhs->getVariables();
            for (std::set<std::string>::iterator ii = tmp->begin(), ee = tmp->end(); ii != ee; ++ii) {
                if (!isNondef(*ii)) {
                    m_depends[lvarIdx + m_numVars * getIdxVar(*ii)] = true;
                }
            }
        }
    }
    makeDependsTransitive();

/*
    for (std::vector<std::string>::iterator i = vars.begin(), e = vars.end(); i != e; ++i) {
        std::cout << *i << " depends on ";
        unsigned int iIdx = getIdxVar(*i);
        for (unsigned int ii = 0; ii < m_numVars; ++ii) {
            if (m_depends[iIdx + m_numVars * ii]) {
                std::cout << getVar(ii) << " ";
            }
        }
        std::cout << std::endl;
    }
*/

    for (std::set<std::string>::iterator i = c_vars.begin(), e = c_vars.end(); i != e; ++i) {
        std::string v = *i;
        if (isNondef(v)) {
            continue;
        }
        unsigned int vidx = getIdxVar(v);
        for (unsigned int ii = 0; ii < m_numVars; ++ii) {
            if (m_depends[vidx + m_numVars * ii]) {
                notNeeded.erase(ii);
            }
        }
    }

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        if (!isRecursiveCall(rule->getRight()->getFunctionSymbol())) {
            res.push_back(rule->dropArgs(notNeeded));
        } else {
          res.push_back(Rule::create(rule->getLeft()->dropArgs(notNeeded), rule->getRight(), rule->getConstraint()));
        }
    }
    return res;
}

// Defined
std::string Slicer::getVar(std::string name)
{
    std::ostringstream tmp;
    tmp << "v_" << name;
    return tmp.str();
}

std::string Slicer::getEval(std::string startstop)
{
    std::ostringstream tmp;
    tmp << "eval_" << m_F->getName().str() << "_" << startstop;
    return tmp.str();
}

void Slicer::setUpPreceeds(std::list<ref<Rule> > rules)
{
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        m_functions.insert((*i)->getLeft()->getFunctionSymbol());
        m_functions.insert((*i)->getRight()->getFunctionSymbol());
    }
    m_numFunctions = static_cast<unsigned int>(m_functions.size());
    unsigned int idx = 0;
    for (std::set<std::string>::iterator i = m_functions.begin(), e = m_functions.end(); i != e; ++i) {
        std::string f = *i;
        m_functionIdx.insert(std::make_pair(f, idx));
        m_idxFunction.insert(std::make_pair(idx, f));
        ++idx;
    }
    m_preceeds = new bool[m_numFunctions * m_numFunctions];
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        for (unsigned int j = 0; j < m_numFunctions; ++j) {
            m_preceeds[i + m_numFunctions * j] = false;
        }
    }
    std::set<std::string> visited;
    std::queue<std::string> todo;
    todo.push(getEval("start"));

    do {
        std::string v = todo.front();
        todo.pop();
        visited.insert(v);
        std::list<std::string> succs;
        for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
            ref<Rule> rule = *i;
            if (rule->getLeft()->getFunctionSymbol() == v) {
                bool have = false;
                std::string succ = rule->getRight()->getFunctionSymbol();
                for (std::list<std::string>::iterator si = succs.begin(), se = succs.end(); si != se; ++si) {
                    if (*si == succ) {
                        have = true;
                    }
                }
                if (!have) {
                    succs.push_back(succ);
                }
            }
        }
        for (std::list<std::string>::iterator i = succs.begin(), e = succs.end(); i != e; ++i) {
            std::string child = *i;
            if (visited.find(child) == visited.end()) {
                // not yet visited
                m_preceeds[getIdxFunction(v) + m_numFunctions * getIdxFunction(child)] = true;
                todo.push(child);
            }
        }
    } while (!todo.empty());

    makePreceedsTransitive();

/*
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        std::cout << getFunction(i) << " preceeds ";
        for (unsigned int ii = 0; ii < m_numFunctions; ++ii) {
            if (m_preceeds[i + m_numFunctions * ii]) {
                std::cout << getFunction(ii) << " ";
            }
        }
        std::cout << std::endl;
    }
*/
}

std::set<std::string> Slicer::computeReachableFuns(std::list<ref<Rule> > rules)
{
    std::set<std::string> res;
    std::queue<std::string> todo;
    todo.push(getEval("start"));

    do {
        std::string v = todo.front();
        todo.pop();
        res.insert(v);
        std::list<std::string> succs;
        for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
            ref<Rule> rule = *i;
            if (rule->getLeft()->getFunctionSymbol() == v) {
                bool have = false;
                std::string succ = rule->getRight()->getFunctionSymbol();
                for (std::list<std::string>::iterator si = succs.begin(), se = succs.end(); si != se; ++si) {
                    if (*si == succ) {
                        have = true;
                    }
                }
                if (!have) {
                    succs.push_back(succ);
                }
            }
        }
        for (std::list<std::string>::iterator i = succs.begin(), e = succs.end(); i != e; ++i) {
            std::string child = *i;
            if (res.find(child) == res.end()) {
                // not yet visited
                todo.push(child);
            }
        }
    } while (!todo.empty());

    return res;
}

std::list<ref<Rule> > Slicer::sliceDefined(std::list<ref<Rule> > rules)
{
    std::set<std::string> reachableFuns = computeReachableFuns(rules);
    std::list<ref<Rule> > reachable;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        if (reachableFuns.find((*i)->getLeft()->getFunctionSymbol()) != reachableFuns.end()) {
            reachable.push_back(*i);
        }
    }
    if (reachable.empty()) {
        return reachable;
    }
    setUpPreceeds(reachable);
    std::set<std::string> initial;
    for (llvm::Function::arg_iterator i = m_F->arg_begin(), e = m_F->arg_end(); i != e; ++i) {
        if (llvm::isa<llvm::IntegerType>(i->getType())) {
            initial.insert(getVar(i->getName()));
        }
    }
    llvm::Module *module = m_F->getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (llvm::isa<llvm::IntegerType>(globalType)) {
            initial.insert(getVar(global->getName()));
        }
    }
    m_defined.insert(std::make_pair(getEval("start"), initial));
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> tmp = *i;
        ref<Term> left = tmp->getLeft();
        ref<Term> right = tmp->getRight();
        if (m_defined.find(right->getFunctionSymbol()) != m_defined.end() || isRecursiveCall(right->getFunctionSymbol())) {
            // already have this one
            continue;
        }
        std::list<ref<Polynomial> > largs = left->getArgs();
        std::list<ref<Polynomial> > rargs = right->getArgs();
        std::set<std::string> defs;
        for (std::list<ref<Polynomial> >::iterator li = largs.begin(), le = largs.end(), ri = rargs.begin(); li != le; ++li, ++ri) {
            ref<Polynomial> lpol = *li;
            ref<Polynomial> rpol = *ri;
            std::string lvar = *lpol->getVariables()->begin();
            if (!rpol->isVar()) {
                defs.insert(lvar);
            } else {
                std::string rvar = *rpol->getVariables()->begin();
                if (lvar != rvar) {
                    defs.insert(lvar);
                }
            }
        }
        m_defined.insert(std::make_pair(right->getFunctionSymbol(), defs));
    }

/*
    for (std::set<std::string>::iterator i = m_functions.begin(), e = m_functions.end(); i != e; ++i) {
        std::map<std::string, std::set<std::string> >::iterator found = m_defined.find(*i);
        if (found == m_defined.end()) {
            continue;
        }
        std::cout << "Defined by " << *i << ": ";
        std::set<std::string> defd = found->second;
        for (std::set<std::string>::iterator vi = defd.begin(), ve = defd.end(); vi != ve;) {
            std::cout << *vi;
            if (++vi != ve) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "Known by " << *i << ": ";
        std::set<std::string> known = getKnownVars(*i);
        for (std::set<std::string>::iterator vi = known.begin(), ve = known.end(); vi != ve;) {
            std::cout << *vi;
            if (++vi != ve) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
*/

    std::list<ref<Rule> > res;
    std::list<std::string> vars;
    std::list<ref<Polynomial> > polys = (*reachable.begin())->getLeft()->getArgs();
    for (std::list<ref<Polynomial> >::iterator i = polys.begin(), e = polys.end(); i != e; ++i) {
        vars.push_back(*(*i)->getVariables()->begin());
    }
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::set<unsigned int> lnotneeded = getNotNeeded(rule->getLeft()->getFunctionSymbol(), vars);
        std::set<unsigned int> rnotneeded;
        if (rule->getRight()->getFunctionSymbol() == getEval("stop")) {
            rnotneeded = getSet(static_cast<unsigned int>(rule->getRight()->getArgs().size()));
        } else if (isRecursiveCall(rule->getRight()->getFunctionSymbol())) {
            // keep everything
        } else {
            rnotneeded = getNotNeeded(rule->getRight()->getFunctionSymbol(), vars);
        }
      ref<Rule> newRule = Rule::create(rule->getLeft()->dropArgs(lnotneeded), rule->getRight()->dropArgs(rnotneeded), rule->getConstraint());
        res.push_back(newRule);
    }

    return res;
}

std::set<unsigned int> Slicer::getNotNeeded(std::string f, std::list<std::string> vars)
{
    std::set<unsigned int> res;
    unsigned int tmp = 0;
    std::set<std::string> known = getKnownVars(f);
    for (std::list<std::string>::iterator vi = vars.begin(), ve = vars.end(); vi != ve; ++vi) {
        if (known.find(*vi) == known.end()) {
            res.insert(tmp);
        }
        ++tmp;
    }
    return res;
}

std::set<std::string> Slicer::getKnownVars(std::string f)
{
    std::set<std::string> res;
    std::set<std::string> fdefines = m_defined.find(f)->second;
    res.insert(fdefines.begin(), fdefines.end());
    unsigned int fidx = getIdxFunction(f) * m_numFunctions;
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        if (m_preceeds[i + fidx]) {
            std::set<std::string> pdefines = m_defined.find(getFunction(i))->second;
            res.insert(pdefines.begin(), pdefines.end());
        }
    }
    return res;
}

// Still Used
void Slicer::setUpCalls(std::list<ref<Rule> > rules)
{
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        m_functions.insert((*i)->getLeft()->getFunctionSymbol());
        m_functions.insert((*i)->getRight()->getFunctionSymbol());
    }
    m_numFunctions = static_cast<unsigned int>(m_functions.size());
    unsigned int idx = 0;
    for (std::set<std::string>::iterator i = m_functions.begin(), e = m_functions.end(); i != e; ++i) {
        std::string f = *i;
        m_functionIdx.insert(std::make_pair(f, idx));
        m_idxFunction.insert(std::make_pair(idx, f));
        ++idx;
    }
    m_calls = new bool[m_numFunctions * m_numFunctions];
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        for (unsigned int j = 0; j < m_numFunctions; ++j) {
            m_calls[i + m_numFunctions * j] = false;
        }
    }
    std::set<std::string> visited;
    std::queue<std::string> todo;
    todo.push(getEval("start"));

    do {
        std::string v = todo.front();
        todo.pop();
        visited.insert(v);
        std::list<std::string> succs;
        for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
            ref<Rule> rule = *i;
            if (rule->getLeft()->getFunctionSymbol() == v) {
                bool have = false;
                std::string succ = rule->getRight()->getFunctionSymbol();
                for (std::list<std::string>::iterator si = succs.begin(), se = succs.end(); si != se; ++si) {
                    if (*si == succ) {
                        have = true;
                    }
                }
                if (!have) {
                    succs.push_back(succ);
                }
            }
        }
        for (std::list<std::string>::iterator i = succs.begin(), e = succs.end(); i != e; ++i) {
            std::string child = *i;
            m_calls[getIdxFunction(v) + m_numFunctions * getIdxFunction(child)] = true;
            if (visited.find(child) == visited.end()) {
                // not yet visited
                todo.push(child);
            }
        }
    } while (!todo.empty());

    makeCallsTransitive();

/*
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        std::cout << getFunction(i) << " calls ";
        for (unsigned int ii = 0; ii < m_numFunctions; ++ii) {
            if (m_calls[i + m_numFunctions * ii]) {
                std::cout << getFunction(ii) << " ";
            }
        }
        std::cout << std::endl;
    }
*/
}

std::list<ref<Rule> > Slicer::sliceStillUsed(std::list<ref<Rule> > rules, bool conservative)
{
    std::set<std::string> reachableFuns = computeReachableFuns(rules);
    std::list<ref<Rule> > reachable;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        if (reachableFuns.find((*i)->getLeft()->getFunctionSymbol()) != reachableFuns.end()) {
            reachable.push_back(*i);
        }
    }
    if (reachable.empty()) {
        return reachable;
    }
    setUpCalls(reachable);

    // simple cases first
    m_stillUsed.insert(std::make_pair(getEval("stop"), std::set<std::string>()));
    std::set<std::string> initial;
    for (llvm::Function::arg_iterator i = m_F->arg_begin(), e = m_F->arg_end(); i != e; ++i) {
        if (llvm::isa<llvm::IntegerType>(i->getType())) {
            initial.insert(getVar(i->getName()));
        }
    }
    llvm::Module *module = m_F->getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (llvm::isa<llvm::IntegerType>(globalType)) {
            initial.insert(getVar(global->getName()));
        }
    }
    m_stillUsed.insert(std::make_pair(getEval("start"), initial));

    // rules
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> tmp = *i;
        ref<Term> left = tmp->getLeft();
        ref<Term> right = tmp->getRight();
        std::set<std::string> *c_vars = tmp->getConstraint()->getVariables();
        std::list<ref<Polynomial> > largs = left->getArgs();
        size_t largsSize = largs.size();
        std::list<ref<Polynomial> > rargs = right->getArgs();
        std::set<std::string> used;
        std::set<std::string> interestingVars;
        std::set<std::string> seenVars;
        size_t counter = 0;
        if (isRecursiveCall(right->getFunctionSymbol())) {
            std::set<std::string> *vars = right->getVariables();
            interestingVars.insert(vars->begin(), vars->end());
        } else {
            for (std::list<ref<Polynomial> >::iterator ri = rargs.begin(), re = rargs.end(), li = largs.begin(); ri != re; ++ri, ++li, ++counter) {
                ref<Polynomial> rpol = *ri;
                if (rpol->isVar()) {
                    std::string rvar = *rpol->getVariables()->begin();
                    if (counter >= largsSize) {
                        // "new"
                        interestingVars.insert(rvar);
                    } else {
                        std::string lvar = *(*li)->getVariables()->begin();
                        if (lvar != rvar) {
                            // in different position
                            interestingVars.insert(rvar);
                        } else if (conservative && m_phiVars.find(rvar) != m_phiVars.end()) {
                            // it's a phi and thus always interesting
                            interestingVars.insert(rvar);
                        } else if (seenVars.find(rvar) != seenVars.end()) {
                            // more than once
                            interestingVars.insert(rvar);
                        } else {
                            // seen it once now
                            seenVars.insert(rvar);
                        }
                    }
                } else {
                    std::set<std::string> *rvars = rpol->getVariables();
                    interestingVars.insert(rvars->begin(), rvars->end());
                }
            }
        }
        for (std::list<ref<Polynomial> >::iterator li = largs.begin(), le = largs.end(); li != le; ++li) {
            ref<Polynomial> lpol = *li;
            std::string lvar = *lpol->getVariables()->begin();
            if (c_vars->find(lvar) != c_vars->end() || interestingVars.find(lvar) != interestingVars.end()) {
                used.insert(lvar);
            }
        }
        std::map<std::string, std::set<std::string> >::iterator found = m_stillUsed.find(left->getFunctionSymbol());
        if (found == m_stillUsed.end()) {
            m_stillUsed.insert(std::make_pair(left->getFunctionSymbol(), used));
        } else {
            used.insert(found->second.begin(), found->second.end());
            m_stillUsed.erase(found);
            m_stillUsed.insert(std::make_pair(left->getFunctionSymbol(), used));
        }
    }

/*
    for (std::set<std::string>::iterator i = m_functions.begin(), e = m_functions.end(); i != e; ++i) {
        std::map<std::string, std::set<std::string> >::iterator found = m_stillUsed.find(*i);
        if (found == m_stillUsed.end()) {
            continue;
        }
        std::cout << "Directly still used by " << *i << ": ";
        std::set<std::string> used = found->second;
        for (std::set<std::string>::iterator vi = used.begin(), ve = used.end(); vi != ve;) {
            std::cout << *vi;
            if (++vi != ve) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
        std::cout << "Transitively still used by " << *i << ": ";
        std::set<std::string> stillUsed = getStillUsed(*i);
        for (std::set<std::string>::iterator vi = stillUsed.begin(), ve = stillUsed.end(); vi != ve;) {
            std::cout << *vi;
            if (++vi != ve) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
*/

    std::list<ref<Rule> > res;
    std::map<std::string, std::set<std::string> > stillusedMap;
    std::map<std::string, std::set<unsigned int> > notneededMap;
    std::map<std::string, std::list<std::string> > varsMap;
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string leftF = rule->getLeft()->getFunctionSymbol();
        if (stillusedMap.find(leftF) == stillusedMap.end()) {
            stillusedMap.insert(std::make_pair(leftF, getStillUsed(leftF)));
        }
        if (varsMap.find(leftF) == varsMap.end()) {
            std::list<ref<Polynomial> > polys = rule->getLeft()->getArgs();
            std::list<std::string> vars;
            for (std::list<ref<Polynomial> >::iterator it = polys.begin(), et = polys.end(); it != et; ++it) {
                vars.push_back(*(*it)->getVariables()->begin());
            }
            varsMap.insert(std::make_pair(leftF, vars));
        }
    }
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string rightF = rule->getRight()->getFunctionSymbol();
        if (stillusedMap.find(rightF) == stillusedMap.end()) {
            if (isRecursiveCall(rightF) && notneededMap.find(rightF) == notneededMap.end()) {
                // keep everything
                notneededMap.insert(std::make_pair(rightF, std::set<unsigned int>()));
            } else {
                stillusedMap.insert(std::make_pair(rightF, getStillUsed(rightF)));
            }
        }
    }
    for (std::map<std::string, std::set<std::string> >::iterator it = stillusedMap.begin(), et = stillusedMap.end(); it != et; ++it) {
        std::string f = it->first;
        if (notneededMap.find(f) == notneededMap.end()) {
            std::set<std::string> stillused = it->second;
            std::map<std::string, std::list<std::string> >::iterator varsi = varsMap.find(f);
            std::set<unsigned int> notneeded;
            if (varsi != varsMap.end()) {
                unsigned int i = 0;
                std::list<std::string> vars = varsi->second;
                for (std::list<std::string>::iterator vi = vars.begin(), ve = vars.end(); vi != ve; ++vi, ++i) {
                    std::string var = *vi;
                    if (stillused.find(var) == stillused.end()) {
                        notneeded.insert(i);
                    }
                }
            }
            notneededMap.insert(std::make_pair(f, notneeded));
        }
    }
    for (std::list<ref<Rule> >::iterator i = reachable.begin(), e = reachable.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::set<unsigned int> lnotneeded = notneededMap.find(rule->getLeft()->getFunctionSymbol())->second;
        std::set<unsigned int> rnotneeded = notneededMap.find(rule->getRight()->getFunctionSymbol())->second;
      ref<Rule> newRule = Rule::create(rule->getLeft()->dropArgs(lnotneeded), rule->getRight()->dropArgs(rnotneeded), rule->getConstraint());
        res.push_back(newRule);
    }

    return res;
}

std::set<std::string> Slicer::getStillUsed(std::string f)
{
    std::set<std::string> res;
    std::set<std::string> fstillused = m_stillUsed.find(f)->second;
    res.insert(fstillused.begin(), fstillused.end());
    unsigned int fidx = getIdxFunction(f);
    for (unsigned int i = 0; i < m_numFunctions; ++i) {
        if (m_calls[fidx + i * m_numFunctions] && !isRecursiveCall(getFunction(i))) {
            std::set<std::string> sstillused = m_stillUsed.find(getFunction(i))->second;
            res.insert(sstillused.begin(), sstillused.end());
        }
    }
    return res;
}

// Helpers
void Slicer::makePreceedsTransitive(void)
{
    for (unsigned int y = 0; y < m_numFunctions; ++y) {
        for (unsigned int x = 0; x < m_numFunctions; ++x) {
            if (m_preceeds[x + m_numFunctions * y]) {
                // x calls y
                for (unsigned int j = 0; j < m_numFunctions; ++j) {
                    if (m_preceeds[y + m_numFunctions * j]) {
                        // y calls j --> x calls j
                        m_preceeds[x + m_numFunctions * j] = true;
                    }
                }
            }
        }
    }
}

void Slicer::makeCallsTransitive(void)
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

void Slicer::makeDependsTransitive(void)
{
    for (unsigned int y = 0; y < m_numVars; ++y) {
        for (unsigned int x = 0; x < m_numVars; ++x) {
            if (m_depends[x + m_numVars * y]) {
                // x depends on y
                for (unsigned int j = 0; j < m_numVars; ++j) {
                    if (m_depends[y + m_numVars * j]) {
                        // y depends on j --> x depends on j
                        m_depends[x + m_numVars * j] = true;
                    }
                }
            }
        }
    }
}

std::string Slicer::getFunction(unsigned int idx)
{
    std::map<unsigned int, std::string>::iterator found = m_idxFunction.find(idx);
    if (found == m_idxFunction.end()) {
        std::cerr << "Internal error in Slicer::getFunction (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

unsigned int Slicer::getIdxFunction(std::string f)
{
    std::map<std::string, unsigned int>::iterator found = m_functionIdx.find(f);
    if (found == m_functionIdx.end()) {
        std::cerr << "Internal error in Slicer::getIdxFunction (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

std::string Slicer::getVar(unsigned int idx)
{
    std::map<unsigned int, std::string>::iterator found = m_idxVar.find(idx);
    if (found == m_idxVar.end()) {
        std::cerr << "Internal error in Slicer::getVar (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

unsigned int Slicer::getIdxVar(std::string v)
{
    std::map<std::string, unsigned int>::iterator found = m_varIdx.find(v);
    if (found == m_varIdx.end()) {
        std::cerr << "Internal error in Slicer::getIdxVar (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(123);
    } else {
        return found->second;
    }
}

bool Slicer::isRecursiveCall(std::string f)
{
    std::string end = f.substr(f.length() - 5);
    return (end == "start");
}

bool Slicer::isNondef(std::string v)
{
    unsigned int len = static_cast<unsigned int>(v.length());
    if (len < 6) {
        return false;
    }
    std::string begin = v.substr(0, 6);
    return (begin == "nondef");
}
