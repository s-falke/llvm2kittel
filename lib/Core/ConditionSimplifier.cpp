// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/ConditionSimplifier.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <iostream>
#include <cstdlib>
#include <stack>

static std::list<Atom*> getAtoms(Constraint *c)
{
    std::list<Atom*> res;
    std::stack<Constraint*> todo;
    todo.push(c);
    while (!todo.empty()) {
        Constraint *cc = todo.top();
        todo.pop();
        switch (cc->getCType()) {
        case Constraint::COperator: {
            Operator *op = static_cast<Operator*>(cc);
            if (op->getOType() != Operator::And) {
                std::cerr << "Unexpected constraint!" << std::endl;
                exit(76767);
            }
            todo.push(op->getLeft());
            todo.push(op->getRight());
            break;
        }
        case Constraint::CAtom:
            res.push_back(static_cast<Atom*>(cc));
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

static bool alreadyThere(Atom *a, std::list<Atom*> &atoms)
{
    for (std::list<Atom*>::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        Atom *aa = *i;
        if (a->equals(aa)) {
            return true;
        }
    }
    return false;
}

static std::list<Atom*> filterAtoms(std::list<Atom*> &atoms, std::set<std::string> &vars)
{
    std::list<Atom*> res;
    for (std::list<Atom*>::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        Atom *a = *i;
        std::set<std::string> *avars = a->getVariables();
        if (!disjoint(vars, avars) && !alreadyThere(a, res)) {
            res.push_back(a);
        }
    }
    return res;
}

static Constraint *makeConjunction(std::list<Atom*> &atoms)
{
    Constraint *res = *(atoms.begin());
    for (std::list<Atom*>::iterator i = ++(atoms.begin()), e = atoms.end(); i != e; ++i) {
        res = new Operator(res, *i, Operator::And);
    }
    return res;
}

static Rule *simplifyConditions(Rule *rule)
{
    Term *lhs = rule->getLeft();
    Term *rhs = rule->getRight();
    Constraint *c = rule->getConstraint();
    if (c->getCType() == Constraint::CTrue) {
        return rule;
    }
    std::set<std::string> vars;
    std::set<std::string> *tmp1 = lhs->getVariables();
    std::set<std::string> *tmp2 = rhs->getVariables();
    vars.insert(tmp1->begin(), tmp1->end());
    vars.insert(tmp2->begin(), tmp2->end());
    std::list<Atom*> atoms = getAtoms(c);
    std::list<Atom*> newAtoms = filterAtoms(atoms, vars);
    if (newAtoms.size() == atoms.size()) {
        return rule;
    } else if (newAtoms.size() == 0) {
        return new Rule(lhs, rhs, Constraint::_true);
    } else {
        return new Rule(lhs, rhs, makeConjunction(newAtoms));
    }
}

std::list<Rule*> simplifyConditions(std::list<Rule*> rules)
{
    std::list<Rule*> res;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        res.push_back(simplifyConditions(*i));
    }
    return res;
}
