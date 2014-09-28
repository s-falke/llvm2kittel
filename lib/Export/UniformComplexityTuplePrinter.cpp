// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Export/UniformComplexityTuplePrinter.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <list>
#include <map>

static void printVars(std::list<std::string> &vars, std::ostream &stream, std::string sep)
{
    for (std::list<std::string>::iterator i = vars.begin(), e = vars.end(); i != e; ) {
        stream << *i;
        if (++i != e) {
            stream << sep;
        }
    }
}

static std::string getLeftString(ref<Rule> rule, std::list<std::string> &vars)
{
    std::ostringstream res;
    std::string lhsFun = rule->getLeft()->getFunctionSymbol();
    res << lhsFun << '(';
    printVars(vars, res, ", ");
    res << ')';
    return res.str();
}

static ref<Polynomial> getArg(std::string &var, std::list<std::string> &lhsNames, std::list<ref<Polynomial> > &args)
{
    std::list<ref<Polynomial> >::iterator a = args.begin();
    for (std::list<std::string>::iterator i = lhsNames.begin(), e = lhsNames.end(); i != e; ++i, ++a) {
        if (*i == var) {
            return *a;
        }
    }
    return ref<Polynomial>();
}

static std::string getRightString(ref<Rule> rule, std::list<std::string> &vars, std::map<std::string, std::list<std::string> > &argNames)
{
    std::ostringstream res;
    std::string rhsFun = rule->getRight()->getFunctionSymbol();
    std::map<std::string, std::list<std::string> >::iterator found = argNames.find(rhsFun);
    if (found == argNames.end()) {
        res << rhsFun << '(';
        printVars(vars, res, ", ");
        res << ')';
    } else {
        std::list<ref<Polynomial> > args = rule->getRight()->getArgs();
        res << rhsFun << '(';
        for (std::list<std::string>::iterator i = vars.begin(), e = vars.end(); i != e; ) {
            std::string var = *i;
            ref<Polynomial> arg = getArg(var, found->second, args);
            if (arg.isNull()) {
                res << var;
            } else {
                res << arg->toString();
            }
            if (++i != e) {
                res << ", ";
            }
        }
        res << ')';
    }
    return res.str();
}

static std::string toCIntString(ref<Rule> rule, std::list<std::string> &vars, std::map<std::string, std::list<std::string> > &argNames)
{
    std::ostringstream res;
    res << getLeftString(rule, vars) << " -> " << "Com_1(" << getRightString(rule, vars, argNames) << ")";
    if (rule->getConstraint()->getCType() != Constraint::CTrue) {
        res << " :|: " << rule->getConstraint()->toCIntString();
    }
    return res.str();
}

static std::string toCIntString(std::list<ref<Rule> > &rules, std::list<std::string> &vars, std::map<std::string, std::list<std::string> > &argNames)
{
    size_t numRules = rules.size();
    if (numRules == 0) {
        std::cerr << "Internal error in UniformComplexityTuplePrinter (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xCAFE);
    }
    std::ostringstream res;
    bool first = true;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ) {
        // the iterator get increased inside the loop
        ref<Rule> tmp = *i;
        if (tmp->getConstraint()->getCType() != Constraint::CTrue) {
            std::cerr << "Cannot print nontrivial complexity tuple with nontrivial constraints (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        }
        if (first) {
            res << getLeftString(tmp, vars) << " -> " << "Com_" << numRules << '(';
            first = false;
        }
        res << getRightString(tmp, vars, argNames);
        if (++i != e) {
            res << ", ";
        }
    }
    res << ')';
    return res.str();
}

static std::list<std::string> getVars(std::list<ref<Rule> > &rules)
{
    std::set<std::string> varsSet;

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        std::set<std::string> tmp;
        (*i)->addVariablesToSet(tmp);
        for (std::set<std::string>::iterator ii = tmp.begin(), ee = tmp.end(); ii != ee; ++ii) {
            if (ii->substr(0, 7) != "nondef.") {
                varsSet.insert(*ii);
            }
        }
    }

    std::list<std::string> res;
    res.insert(res.begin(), varsSet.begin(), varsSet.end());
    return res;
}

static std::list<std::string> getAllVars(std::list<ref<Rule> > &rules)
{
    std::set<std::string> varsSet;

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        (*i)->addVariablesToSet(varsSet);
    }

    std::list<std::string> res;
    res.insert(res.begin(), varsSet.begin(), varsSet.end());
    return res;
}

static std::map<std::string, std::list<std::string> > getArgNames(std::list<ref<Rule> > &rules)
{
    std::map<std::string, std::list<std::string> > res;

    for (std::list<ref<Rule> >::iterator i= rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string lhsFun = rule->getLeft()->getFunctionSymbol();
        std::list<std::string> argNames;
        std::list<ref<Polynomial> > args = rule->getLeft()->getArgs();
        for (std::list<ref<Polynomial> >::iterator ii = args.begin(), ee = args.end(); ii != ee; ++ii) {
            if (!(*ii)->isVar()) {
                std::cerr << "Internal error in UniformComplexityTuplePrinter (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
                exit(0xAAAA);
            }
            std::set<std::string> vars;
            (*ii)->addVariablesToSet(vars);
            argNames.push_back(*vars.begin());
        }
        std::map<std::string, std::list<std::string> >::iterator found = res.find(lhsFun);
        if (found == res.end()) {
            res.insert(std::make_pair(lhsFun, argNames));
        } else if (found->second != argNames) {
            std::cerr << "Internal error in UniformComplexityTuplePrinter (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(0xBBBB);
        }
    }

    return res;
}

void printUniformComplexityTuples(std::list<ref<Rule> > &rules, std::set<std::string> &complexityLHSs, std::string &startFun, std::ostream &stream)
{
    std::set<std::string> todoComplexityLHSs;
    todoComplexityLHSs.insert(complexityLHSs.begin(), complexityLHSs.end());

    std::list<std::string> vars = getVars(rules);
    std::list<std::string> allVars = getAllVars(rules);
    std::map<std::string, std::list<std::string> > argNames = getArgNames(rules);

    stream << "(GOAL COMPLEXITY)" << std::endl;
    stream << "(STARTTERM (FUNCTIONSYMBOLS " << startFun << "))" << std::endl;
    stream << "(VAR ";
    printVars(allVars, stream, " ");
    stream << ')' << std::endl;
    stream << "(RULES" << std::endl;

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string lhsFun = rule->getLeft()->getFunctionSymbol();
        if (complexityLHSs.find(lhsFun) == complexityLHSs.end()) {
            stream << "  " << toCIntString(rule, vars, argNames) << std::endl;
        } else {
            if (todoComplexityLHSs.find(lhsFun) == todoComplexityLHSs.end()) {
                // already taken care of
            } else {
                // collect and print in one complexity tuple
                std::list<ref<Rule> > combineRules;
                for (std::list<ref<Rule> >::iterator ii = rules.begin(); ii != e; ++ii) {
                    ref<Rule> tmp = *ii;
                    if (tmp->getLeft()->getFunctionSymbol() == lhsFun) {
                        combineRules.push_back(tmp);
                    }
                }
                stream << "  " << toCIntString(combineRules, vars, argNames) << std::endl;
                todoComplexityLHSs.erase(lhsFun);
            }
        }
    }

    stream << ")" << std::endl;
}
