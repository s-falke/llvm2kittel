// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/ConstraintSimplifier.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <iostream>
#include <cstdlib>
#include <stack>

static std::list<ref<Atom> > getAtoms(ref<Constraint> c)
{
    std::list<ref<Atom> > res;
    std::stack<ref<Constraint> > todo;
    todo.push(c);
    while (!todo.empty()) {
        ref<Constraint> cc = todo.top();
        todo.pop();
        switch (cc->getCType()) {
        case Constraint::COperator: {
            ref<Operator> op = static_cast<Operator*>(cc.get());
            if (op->getOType() != Operator::And) {
                std::cerr << "Unexpected constraint!" << std::endl;
                exit(76767);
            }
            todo.push(op->getLeft());
            todo.push(op->getRight());
            break;
        }
        case Constraint::CAtom:
            res.push_back(static_cast<Atom*>(cc.get()));
            break;
        case Constraint::CTrue:
        case Constraint::CFalse:
        case Constraint::CNondef:
        case Constraint::CNegation:
        default:
            std::cerr << "Unexpected constraint!" << std::endl;
            exit(76767);
            break;
        }
    }
    return res;
}

static bool disjoint(std::set<std::string> &vars, std::set<std::string> *avars)
{
    for (std::set<std::string>::iterator i = avars->begin(), e = avars->end(); i != e; ++i) {
        std::string v = *i;
        if (vars.find(v) != vars.end()) {
            return false;
        }
    }
    return true;
}

static bool alreadyThere(ref<Atom> a, std::list<ref<Atom> > &atoms)
{
    for (std::list<ref<Atom> >::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        ref<Atom> aa = *i;
        if (a->equals(aa)) {
            return true;
        }
    }
    return false;
}

static std::list<ref<Atom> > filterAtoms(std::list<ref<Atom> > &atoms, std::set<std::string> &vars)
{
    std::list<ref<Atom> > res;
    for (std::list<ref<Atom> >::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        ref<Atom> a = *i;
        std::set<std::string> *avars = a->getVariables();
        if (!disjoint(vars, avars) && !alreadyThere(a, res)) {
            res.push_back(a);
        }
    }
    return res;
}

static ref<Constraint> makeConjunction(std::list<ref<Atom> > &atoms)
{
    ref<Constraint> res = *(atoms.begin());
    for (std::list<ref<Atom> >::iterator i = ++(atoms.begin()), e = atoms.end(); i != e; ++i) {
      res = Operator::create(res, *i, Operator::And);
    }
    return res;
}

static ref<Rule> simplifyConstraints(ref<Rule> rule)
{
    ref<Term> lhs = rule->getLeft();
    ref<Term> rhs = rule->getRight();
    ref<Constraint> c = rule->getConstraint();
    if (c->getCType() == Constraint::CTrue) {
        return rule;
    }
    std::set<std::string> vars;
    std::set<std::string> *tmp1 = lhs->getVariables();
    std::set<std::string> *tmp2 = rhs->getVariables();
    vars.insert(tmp1->begin(), tmp1->end());
    vars.insert(tmp2->begin(), tmp2->end());
    std::list<ref<Atom> > atoms = getAtoms(c);
    std::list<ref<Atom> > newAtoms = filterAtoms(atoms, vars);
    if (newAtoms.size() == atoms.size()) {
        return rule;
    } else if (newAtoms.size() == 0) {
        return Rule::create(lhs, rhs, Constraint::_true);
    } else {
        return Rule::create(lhs, rhs, makeConjunction(newAtoms));
    }
}

std::list<ref<Rule> > simplifyConstraints(std::list<ref<Rule> > rules)
{
    std::list<ref<Rule> > res;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        res.push_back(simplifyConstraints(*i));
    }
    return res;
}
