// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/BoundConstrainer.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stack>

static std::string getNormFun(std::string f, unsigned int i)
{
    std::ostringstream tmp;
    tmp << f << "_" << i << "_norm";
    return tmp.str();
}

static std::string getBlockFun(std::string f, unsigned int i)
{
    std::ostringstream tmp;
    tmp << f << "_" << i << "_block";
    return tmp.str();
}

static ref<Constraint> getBoundConstraints(std::string var, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    std::map<std::string, unsigned int>::iterator found = bitwidthMap.find(var);
    if (found == bitwidthMap.end()) {
        std::cerr << "Did not find bitwidth information for " << var << " in \"getBoundConstraints\"!" << std::endl;
        exit(777);
    }
    unsigned int bitwidth = found->second;
    ref<Polynomial> lower;
    ref<Polynomial> upper;
    ref<Polynomial> pvar = Polynomial::create(var);
    if (unsignedEncoding) {
        lower = Polynomial::null;
        upper = Polynomial::uimax(bitwidth);
    } else {
        lower = Polynomial::simin(bitwidth);
        upper = Polynomial::simax(bitwidth);
    }
    ref<Constraint> lowerCheck = Atom::create(pvar, lower, Atom::Geq);
    ref<Constraint> upperCheck = Atom::create(pvar, upper, Atom::Leq);
    return Operator::create(lowerCheck, upperCheck, Operator::And);
}

static ref<Constraint> getBoundConstraints(std::set<std::string> &vars, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    if (vars.size() == 0) {
        return Constraint::_true;
    }
    ref<Constraint> res = getBoundConstraints(*(vars.begin()), bitwidthMap, unsignedEncoding);
    for (std::set<std::string>::iterator i = ++(vars.begin()), e = vars.end(); i != e; ++i) {
        res = Operator::create(res, getBoundConstraints(*i, bitwidthMap, unsignedEncoding), Operator::And);
    }
    return res;
}

static std::list<ref<Polynomial> > getNormArgs(std::string fun, std::list<ref<Rule> > rules)
{
    if (fun.substr(fun.length() - 4) == "stop") {
        for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
            ref<Rule> rule = *i;
            ref<Term> rhs = rule->getRight();
            if (rhs->getFunctionSymbol() == fun) {
                return rhs->getArgs();
            }
        }
        std::cerr << "Did not find rule with matching RHS!" << std::endl;
        exit(1212);
    }
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        ref<Term> lhs = rule->getLeft();
        if (lhs->getFunctionSymbol() == fun) {
            return lhs->getArgs();
        }
    }
    std::cerr << "Did not find rule with matching LHS!" << std::endl;
    exit(1212);
}

bool isNormal(ref<Polynomial> p)
{
    return p->isVar() || p->isConst();
}

static std::map<unsigned int, ref<Polynomial> > getNonNormalArgPositions(ref<Term> rhs)
{
    std::map<unsigned int, ref<Polynomial> > res;
    std::list<ref<Polynomial> > args = rhs->getArgs();
    unsigned int c = 0;
    for (std::list<ref<Polynomial> >::iterator i = args.begin(), e = args.end(); i != e; ++i) {
        ref<Polynomial> p = *i;
        if (!isNormal(p)) {
            res.insert(std::make_pair(c, p));
        }
        ++c;
    }
    return res;
}

static std::list<ref<Polynomial> > getNonNormalAtomPolynomials(ref<Constraint> c)
{
    std::list<ref<Polynomial> > res;
    std::list<ref<Constraint> > atomics;
    c->addAtomicsToList(atomics);
    for (std::list<ref<Constraint> >::iterator i = atomics.begin(), e = atomics.end(); i != e; ++i) {
        ref<Constraint> cc = *i;
        if (cc->getCType() == Constraint::CAtom) {
            ref<Atom> atom = static_cast<Atom*>(cc.get());
            ref<Polynomial> lhs = atom->getLeft();
            if (!isNormal(lhs)) {
                res.push_back(lhs);
            }
            ref<Polynomial> rhs = atom->getRight();
            if (!isNormal(rhs)) {
                res.push_back(rhs);
            }
        }
    }
    return res;
}

static std::string get(std::list<ref<Polynomial> > args, unsigned int c)
{
    std::list<ref<Polynomial> >::iterator tmp = args.begin();
    for (unsigned int i = 0; i < c; ++i) {
        ++tmp;
    }
    std::set<std::string> vars;
    (*tmp)->addVariablesToSet(vars);
    return *(vars.begin());
}

static ref<Term> getNormRhs(ref<Term> lhs, unsigned int c, ref<Polynomial> addTerm, bool doAdd)
{
    std::list<ref<Polynomial> > args = lhs->getArgs();
    std::list<ref<Polynomial> > newargs;
    unsigned int cc = 0;
    for (std::list<ref<Polynomial> >::iterator i = args.begin(), e = args.end(); i != e; ++i) {
        ref<Polynomial> p = *i;
        if (c == cc) {
            if (doAdd) {
                newargs.push_back(p->add(addTerm));
            } else  {
                newargs.push_back(p->sub(addTerm));
            }
        } else {
            newargs.push_back(p);
        }
        ++cc;
    }
    return Term::create(lhs->getFunctionSymbol(), newargs);
}

static std::list<ref<Rule> > getNormRules(ref<Term> normLhs, unsigned int c, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    std::list<ref<Rule> > res;
    std::string var = get(normLhs->getArgs(), c);
    std::map<std::string, unsigned int>::iterator found = bitwidthMap.find(var);
    if (found == bitwidthMap.end()) {
        std::cerr << "Did not find bitwidth information for " << var << " in \"getNormRules\"!" << std::endl;
        exit(777);
    }
    unsigned int bitwidth = found->second;
    ref<Polynomial> lower;
    ref<Polynomial> upper;
    ref<Polynomial> adder;
    ref<Polynomial> pvar = Polynomial::create(var);
    if (unsignedEncoding) {
        lower = Polynomial::null;
        upper = Polynomial::uimax(bitwidth);
    } else {
        lower = Polynomial::simin(bitwidth);
        upper = Polynomial::simax(bitwidth);
    }
    adder = Polynomial::power_of_two(bitwidth);
    ref<Constraint> tooSmall = Atom::create(pvar, lower, Atom::Lss);
    ref<Constraint> tooBig = Atom::create(pvar, upper, Atom::Gtr);
    ref<Term> rhs1 = getNormRhs(normLhs, c, adder, true);
    ref<Rule> rule1 = Rule::create(normLhs, rhs1, tooSmall);
    ref<Term> rhs2 = getNormRhs(normLhs, c, adder, false);
    ref<Rule> rule2 = Rule::create(normLhs, rhs2, tooBig);
    res.push_back(rule1);
    res.push_back(rule2);
    return res;
}

static bool isNormSymbol(std::string fun)
{
    return (fun.substr(fun.length() - 5) == "_norm");
}

static bool isBlockSymbol(std::string fun)
{
    return (fun.substr(fun.length() - 6) == "_block");
}

static ref<Rule> chainRules(ref<Rule> rule1, ref<Rule> rule2)
{
    ref<Term> rhs1 = rule1->getRight();
    ref<Term> lhs2 = rule2->getLeft();
    std::map<std::string, ref<Polynomial> > subby;
    std::list<ref<Polynomial> > rhs1args = rhs1->getArgs();
    std::list<ref<Polynomial> > lhs2args = lhs2->getArgs();
    for (std::list<ref<Polynomial> >::iterator i1 = rhs1args.begin(), e1 = rhs1args.end(), i2 = lhs2args.begin(); i1 != e1; ++i1, ++i2) {
        ref<Polynomial> p = *i1;
        std::set<std::string> vars;
        (*i2)->addVariablesToSet(vars);
        std::string var = *(vars.begin());
        subby.insert(std::make_pair(var, p));
    }
    return Rule::create(rule1->getLeft(), rule2->getRight()->instantiate(&subby), Operator::create(rule1->getConstraint(), rule2->getConstraint()->instantiate(&subby), Operator::And));
}

static std::pair<ref<Rule>, ref<Rule> > getTheNormRules(std::string rhsFun, unsigned int argPos, std::list<ref<Rule> > &rules)
{
    std::list<ref<Rule> > normRules;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> tmp = *i;
        if (tmp->getLeft()->getFunctionSymbol() == rhsFun && tmp->getRight()->getFunctionSymbol() == rhsFun) {
            ref<Polynomial> theArg = tmp->getRight()->getArg(argPos);
            if (!theArg->isVar()) {
                normRules.push_back(tmp);
            }
        }
    }
    if (normRules.size() != 2) {
        std::cerr << "Did not find exactly two norm rules!" << std::endl;
        exit(767400);
    }
    return std::make_pair(*normRules.begin(), *(++normRules.begin()));
}

static std::list<ref<Rule> > getChainedNormRules(ref<Rule> rule, std::list<ref<Rule> > &rules)
{
    std::list<ref<Rule> > res;
    std::list<ref<Rule> > thusFar;
    ref<Term> rhs = rule->getRight();
    std::list<ref<Polynomial> > args = rhs->getArgs();
    thusFar.push_back(rule);
    std::string rhsFun = rhs->getFunctionSymbol();
    unsigned int argCount = 0;
    for (std::list<ref<Polynomial> >::iterator ia = args.begin(), ea = args.end(); ia != ea; ++ia) {
        ref<Polynomial> p = *ia;
        if (isNormal(p)) {
            ++ argCount;
            continue;
        }
        std::list<ref<Rule> > tmp;
        for (std::list<ref<Rule> >::iterator ir = thusFar.begin(), er = thusFar.end(); ir != er; ++ir) {
            ref<Rule> toChain = *ir;
            // no chaining
            tmp.push_back(toChain);
            long int normStepsNeeded = p->normStepsNeeded();
            std::pair<ref<Rule>, ref<Rule> > theNormRules = getTheNormRules(rhsFun, argCount, rules);
            ref<Rule> rule1 = toChain;
            ref<Rule> rule2 = toChain;
            for (long int c = 1; c <= normStepsNeeded; ++c) {
                rule1 = chainRules(rule1, theNormRules.first);
                rule2 = chainRules(rule2, theNormRules.second);
                tmp.push_back(rule1);
                tmp.push_back(rule2);
            }
        }
        thusFar = tmp;
        ++argCount;
    }
    // now chain with exit
    ref<Rule> exitRule;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> tmp = *i;
        if (tmp->getLeft()->getFunctionSymbol() == rhsFun && tmp->getRight()->getFunctionSymbol() != rhsFun) {
            exitRule = tmp;
            break;
        }
    }
    if (exitRule.isNull()) {
        std::cerr << "Did not find exit rule!" << std::endl;
        exit(777);
    }
    for (std::list<ref<Rule> >::iterator i = thusFar.begin(), e = thusFar.end(); i != e; ++i) {
        res.push_back(chainRules(*i, exitRule));
    }
    return res;
}

static std::list<ref<Rule> > eliminateUnneededNorms(std::list<ref<Rule> > rules, std::set<std::string> &haveToKeep)
{
    std::list<ref<Rule> > res;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        ref<Term> lhs = rule->getLeft();
        ref<Term> rhs = rule->getRight();
        std::string lhsFun = lhs->getFunctionSymbol();
        std::string rhsFun = rhs->getFunctionSymbol();
        bool leftIsNorm = isNormSymbol(lhsFun);
        bool rightIsNorm = isNormSymbol(rhsFun);
        if (leftIsNorm && rightIsNorm) {
            if (haveToKeep.find(lhsFun) != haveToKeep.end()) {
                // keep if needed
                res.push_back(rule);
            }
        } else if (leftIsNorm && !rightIsNorm) {
            if (haveToKeep.find(lhsFun) != haveToKeep.end()) {
                // keep if needed
                res.push_back(rule);
            }
        } else if (!leftIsNorm && rightIsNorm) {
            if (haveToKeep.find(rhsFun) != haveToKeep.end()) {
                // keep if needed
                res.push_back(rule);
            } else {
                // chain
                std::list<ref<Rule> > chainedRules = getChainedNormRules(rule, rules);
                res.insert(res.end(), chainedRules.begin(), chainedRules.end());
            }
        } else {
            // unrelated to norm
            res.push_back(rule);
        }
    }
    return res;
}

static std::list<ref<Rule> > eliminateBlocks(std::list<ref<Rule> > rules)
{
    std::list<ref<Rule> > res;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        ref<Term> lhs = rule->getLeft();
        ref<Term> rhs = rule->getRight();
        std::string lhsFun = lhs->getFunctionSymbol();
        std::string rhsFun = rhs->getFunctionSymbol();
        if (isBlockSymbol(rhsFun)) {
            // chain
            for (std::list<ref<Rule> >::iterator ii = rules.begin(), ee = rules.end(); ii != ee; ++ii) {
                ref<Rule> inner = *ii;
                if (inner->getLeft()->getFunctionSymbol() == rhsFun) {
                   res.push_back(chainRules(rule, inner));
                }
            }
        } else if (isBlockSymbol(lhsFun)) {
            // nada
        } else {
            // unrelated to blocking
            res.push_back(rule);
        }
    }
    return res;
}

static std::string getNewVar(unsigned int c)
{
    std::ostringstream tmp;
    tmp << "c_norm_" << c;
    return tmp.str();
}

static bool isCNorm(std::string v)
{
    return (v.substr(0, 7) == "c_norm_");
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

static ref<Constraint> mapPolysToVars(ref<Constraint> c, std::map<Polynomial*, std::string> &atomPolyToVarMap)
{
    if (atomPolyToVarMap.empty()) {
        return c;
    }
    std::list<ref<Atom> > atoms = getAtoms(c);
    std::list<ref<Atom> > newAtoms;
    for (std::list<ref<Atom> >::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        ref<Atom> a = *i;
        ref<Polynomial> lhs = a->getLeft();
        ref<Polynomial> rhs = a->getRight();
        ref<Polynomial> newLhs;
        for (std::map<Polynomial*, std::string>::iterator ii = atomPolyToVarMap.begin(), ee = atomPolyToVarMap.end(); ii != ee; ++ii) {
            if (ii->first->equals(lhs)) {
                newLhs = Polynomial::create(ii->second);
            }
        }
        if (newLhs.isNull()) {
            newLhs = lhs;
        }
        ref<Polynomial> newRhs;
        for (std::map<Polynomial*, std::string>::iterator ii = atomPolyToVarMap.begin(), ee = atomPolyToVarMap.end(); ii != ee; ++ii) {
            if (ii->first->equals(rhs)) {
                newRhs = Polynomial::create(ii->second);
            }
        }
        if (newRhs.isNull()) {
            newRhs = rhs;
        }
        if (newLhs.get() != lhs.get() || newRhs.get() != rhs.get()) {
            ref<Atom> na = static_cast<Atom*>(Atom::create(newLhs, newRhs, a->getAType()).get());
            newAtoms.push_back(na);
        } else {
            newAtoms.push_back(a);
        }
    }
    return makeConjunction(newAtoms);
}

std::list<ref<Rule> > addBoundConstraints(std::list<ref<Rule> > rules, std::map<std::string, unsigned int> bitwidthMap, bool unsignedEncoding)
{
    std::list<ref<Rule> > res;
    std::set<std::string> haveToKeep;
    unsigned int normCount = 0;
    unsigned int blockCount = 0;
    for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        ref<Term> lhs = rule->getLeft();
        ref<Term> rhs = rule->getRight();
        ref<Constraint> c = rule->getConstraint();
        std::string lhsFun = lhs->getFunctionSymbol();
        std::string rhsFun = rhs->getFunctionSymbol();
        std::map<unsigned int, ref<Polynomial> > nonNormal = getNonNormalArgPositions(rhs);
        std::list<ref<Polynomial> > nonNormalAtomPolys = getNonNormalAtomPolynomials(c);
        if (nonNormal.empty() && nonNormalAtomPolys.empty()) {
            // rule only needs bound conditions
            std::set<std::string> vars;
            lhs->addVariablesToSet(vars);
            rhs->addVariablesToSet(vars);
            ref<Constraint> bounds = getBoundConstraints(vars, bitwidthMap, unsignedEncoding);
            ref<Constraint> conj = Operator::create(c, bounds, Operator::And);
            ref<Rule> newRule = Rule::create(lhs, rhs, conj);
            res.push_back(newRule);
        } else {
            std::list<ref<Polynomial> > ruleNormArgs = getNormArgs(rhsFun, rules);
            std::list<ref<Polynomial> > condNormArgs;
            std::list<ref<Polynomial> > rhsArgs = rhs->getArgs();

            std::string cond_norm = getNormFun(rhsFun, normCount);
            ++normCount;
            std::string rule_norm = getNormFun(rhsFun, normCount);
            ++normCount;
            std::string blocker = getBlockFun(rhsFun, blockCount);
            ++blockCount;

            // set up dummy vars for atom polys
            std::map<Polynomial*, std::string> atomPolyToVarMap;
            unsigned int counter = 0;
            for (std::list<ref<Polynomial> >::iterator pi = nonNormalAtomPolys.begin(), pe = nonNormalAtomPolys.end(); pi != pe; ++pi) {
                ref<Polynomial> pol = *pi;
                if (pol->normStepsNeeded() == -1) {
                    haveToKeep.insert(cond_norm);
                }
                std::string var = getNewVar(counter++);
                condNormArgs.push_back(Polynomial::create(var));
                atomPolyToVarMap.insert(std::make_pair(pol.get(), var));
                std::set<std::string> tmpVars;
                pol->addVariablesToSet(tmpVars);
                std::string tmpvar = *(tmpVars.begin());
                std::map<std::string, unsigned int>::iterator tmpvari = bitwidthMap.find(tmpvar);
                if (tmpvari == bitwidthMap.end()) {
                    std::cerr << "Did not find bitwidth information for " << var << " in \"getBoundConstraints\"!" << std::endl;
                    exit(777);
                }
                bitwidthMap.insert(std::make_pair(var, tmpvari->second));
            }

            // rule needs bound constraints and normalization

            // lhs -> rhs_cond_norm [ bounds ]
            std::set<std::string> bounds1Vars;
            lhs->addVariablesToSet(bounds1Vars);
            ref<Constraint> bounds1 = getBoundConstraints(bounds1Vars, bitwidthMap, unsignedEncoding);
            std::list<ref<Polynomial> > cond_norm_args = lhs->getArgs();
            cond_norm_args.insert(cond_norm_args.end(), nonNormalAtomPolys.begin(), nonNormalAtomPolys.end());
            ref<Term> rhs_cond_norm = Term::create(cond_norm, cond_norm_args);
            ref<Rule> rule1 = Rule::create(lhs, rhs_cond_norm, bounds1);
            res.push_back(rule1);

            // rhs_cond_norm -> blockrhs_rule_norm [ bounds /\ mapped(c) ]
            std::list<ref<Polynomial> > cond_norm_done_args = lhs->getArgs();
            cond_norm_done_args.insert(cond_norm_done_args.end(), condNormArgs.begin(), condNormArgs.end());
            ref<Term> rhs_cond_norm_done = Term::create(cond_norm, cond_norm_done_args);
            ref<Term> block = Term::create(blocker, lhs->getArgs());
            std::set<std::string> bounds2Vars;
            rhs_cond_norm_done->addVariablesToSet(bounds2Vars);
            ref<Constraint> bounds2 = getBoundConstraints(bounds2Vars, bitwidthMap, unsignedEncoding);
            ref<Constraint> newC = Operator::create(mapPolysToVars(c, atomPolyToVarMap), bounds2, Operator::And);
            ref<Rule> rule2 = Rule::create(rhs_cond_norm_done, block, newC);

            // block -> rhs_rule_norm
            ref<Term> rhs_rule_norm = Term::create(rule_norm, rhsArgs);
            ref<Rule> rule3 = Rule::create(block, rhs_rule_norm, Constraint::_true);

            // rhs_rule_norm -> rhs [ bounds ]
            ref<Term> rhs_rule_norm_done = Term::create(rule_norm, ruleNormArgs);
            ref<Term> normRhs = Term::create(rhsFun, ruleNormArgs);
            std::set<std::string> bounds3Vars;
            rhs_rule_norm_done->addVariablesToSet(bounds3Vars);
            ref<Constraint> bounds3 = getBoundConstraints(bounds3Vars, bitwidthMap, unsignedEncoding);
            ref<Rule> rule4 = Rule::create(rhs_rule_norm_done, normRhs, bounds3);

            // do normalization for both
            unsigned int count = 0;
            for (std::list<ref<Polynomial> >::iterator it = cond_norm_done_args.begin(), et = cond_norm_done_args.end(); it != et; ++it) {
                ref<Polynomial> pol = *it;
                if (pol->isConst()) {
                    continue;
                }
                std::set<std::string> vars;
                pol->addVariablesToSet(vars);
                std::string var = *(vars.begin());
                if (isCNorm(var)) {
                    std::list<ref<Rule> > normRules = getNormRules(rhs_cond_norm_done, count, bitwidthMap, unsignedEncoding);
                    res.insert(res.end(), normRules.begin(), normRules.end());
                }
                ++count;
            }
            res.push_back(rule2);
            res.push_back(rule3);
            count = 0;
            for (std::list<ref<Polynomial> >::iterator ii = rhsArgs.begin(), ee = rhsArgs.end(); ii != ee; ++ii) {
                std::map<unsigned int, ref<Polynomial> >::iterator it = nonNormal.find(count);
                if (it != nonNormal.end()) {
                    // normalize it!
                    std::list<ref<Rule> > normRules = getNormRules(rhs_rule_norm_done, count, bitwidthMap, unsignedEncoding);
                    res.insert(res.end(), normRules.begin(), normRules.end());
                    ref<Polynomial> p = it->second;
                    if (p->normStepsNeeded() == -1) {
                        haveToKeep.insert(getNormFun(rhsFun, normCount));
                    }
                }
                ++count;
            }
            res.push_back(rule4);
        }
    }
    return eliminateBlocks(eliminateUnneededNorms(res, haveToKeep));
}
