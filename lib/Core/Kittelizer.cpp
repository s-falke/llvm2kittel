// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Kittelizer.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Term.h"
#include "llvm2kittel/IntTRS/Rule.h"

// C++ includes
#include <iostream>
#include <cstdlib>

ref<Constraint> simplify(ref<Constraint> c)
{
    Constraint::CType type = c->getCType();
    if (type == Constraint::CTrue || type == Constraint::CFalse || type == Constraint::CNondef || type == Constraint::CAtom) {
        return c;
    }
    ref<Operator> op = static_cast<Operator*>(c.get());
    if (op->getOType() == Operator::And) {
        std::list<ref<Constraint> > atomics;
        op->addAtomicsToList(atomics);
        std::list<ref<Constraint> > usedAtomics;
        for (std::list<ref<Constraint> >::iterator i = atomics.begin(), e = atomics.end(); i != e; ++i) {
            ref<Constraint> cc = *i;
            Constraint::CType ccType = cc->getCType();
            if (ccType == Constraint::CFalse) {
                return Constraint::_false;
            } else if (ccType == Constraint::CTrue || ccType == Constraint::CNondef) {
                // don't add to usedAtomics
            } else {
                usedAtomics.push_back(cc);
            }
        }
        if (usedAtomics.size() == 0) {
            return Constraint::_true;
        }
        ref<Constraint> res = *usedAtomics.rbegin();
        usedAtomics.erase(--usedAtomics.end());
        for (std::list<ref<Constraint> >::reverse_iterator ri = usedAtomics.rbegin(), re = usedAtomics.rend(); ri != re; ++ri) {
            res = Operator::create(*ri, res, Operator::And);
        }
        return res;
    } else {
        std::cerr << "Internal error in Kittelizer::simplify (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xBABE);
    }
}

std::list<ref<Rule> > kittelize(std::list<ref<Rule> > rules, SMTSolver solver)
{
    std::list<ref<Rule> > res;
    EliminateClass *elim = eliminateClassFactory(solver);
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        ref<Term> lhs = rule->getLeft();
        ref<Term> rhs = rule->getRight();
        ref<Constraint> con = rule->getConstraint()->evaluateTrivialAtoms()->eliminateNeq()->toDNF(elim);
        std::list<ref<Constraint> > dcs;
        con->addDualClausesToList(dcs);
        for (std::list<ref<Constraint> >::iterator ci = dcs.begin(), ce = dcs.end(); ci != ce; ++ci) {
            ref<Constraint> c = simplify(*ci);
            Constraint::CType type = c->getCType();
            if (type == Constraint::CFalse) {
                // no rule
            } else if (c->getCType() == Constraint::CNondef) {
                // rule with "TRUE"
                res.push_back(Rule::create(lhs, rhs, Constraint::_true));
            } else {
                // rule with c
                res.push_back(Rule::create(lhs, rhs, c));
            }
        }
    }
    delete elim;
    return res;
}
