// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Export/ComplexityTuplePrinter.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>

static std::string toCIntString(ref<Rule> rule)
{
    std::ostringstream res;
    res << rule->getLeft()->toString() << " -> " << "Com_1(" << rule->getRight()->toString() << ")";
    if (rule->getConstraint()->getCType() != Constraint::CTrue) {
        res << " :|: " << rule->getConstraint()->toCIntString();
    }
    return res.str();
}

static std::string toCIntString(std::list<ref<Rule> > &rules)
{
    size_t numRules = rules.size();
    if (numRules == 0) {
        std::cerr << "Internal error in ComplexityTuplePrinter (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
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
            res << tmp->getLeft()->toString() << " -> " << "Com_" << numRules << '(';
            first = false;
        }
        res << tmp->getRight()->toString();
        if (++i != e) {
            res << ", ";
        }
    }
    res << ')';
    return res.str();
}

static void printVars(std::list<ref<Rule> > &rules, std::ostream &stream)
{
    std::set<std::string> varsSet;

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        (*i)->addVariablesToSet(varsSet);
    }

    for (std::set<std::string>::iterator i = varsSet.begin(), e = varsSet.end(); i != e; ++i) {
        stream << ' ' << *i;
    }
}

void printComplexityTuples(std::list<ref<Rule> > &rules, std::set<std::string> &complexityLHSs, std::ostream &stream)
{
    std::set<std::string> todoComplexityLHSs;
    todoComplexityLHSs.insert(complexityLHSs.begin(), complexityLHSs.end());

    stream << "(GOAL COMPLEXITY)\n(STARTTERM CONSTRUCTOR-BASED)" << std::endl;
    stream << "(VAR";
    printVars(rules, stream);
    stream << ')' << std::endl;
    stream << "(RULES" << std::endl;

    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string lhsFun = rule->getLeft()->getFunctionSymbol();
        if (complexityLHSs.find(lhsFun) == complexityLHSs.end()) {
            stream << "  " << toCIntString(rule) << std::endl;
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
                stream << "  " << toCIntString(combineRules) << std::endl;
                todoComplexityLHSs.erase(lhsFun);
            }
        }
    }

    stream << ")" << std::endl;
}
