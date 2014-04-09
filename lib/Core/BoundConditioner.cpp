// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/BoundConditioner.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <iostream>
#include <sstream>
#include <cstdlib>
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

static Constraint *getBoundConstraints(std::string var, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    std::map<std::string, unsigned int>::iterator found = bitwidthMap.find(var);
    if (found == bitwidthMap.end()) {
        std::cerr << "Did not find bitwidth information for " << var << " in \"getBoundConstraints\"!" << std::endl;
        exit(777);
    }
    unsigned int bitwidth = found->second;
    Polynomial *lower = NULL;
    Polynomial *upper = NULL;
    Polynomial *pvar = new Polynomial(var);
    if (unsignedEncoding) {
        lower = Polynomial::null;
        upper = Polynomial::uimax(bitwidth);
    } else {
        lower = Polynomial::simin(bitwidth);
        upper = Polynomial::simax(bitwidth);
    }
    Constraint *lowerCheck = new Atom(pvar, lower, Atom::Geq);
    Constraint *upperCheck = new Atom(pvar, upper, Atom::Leq);
    return new Operator(lowerCheck, upperCheck, Operator::And);
}

static Constraint *getBoundConstraints(std::set<std::string> &vars, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    if (vars.size() == 0) {
        return Constraint::_true;
    }
    Constraint *res = getBoundConstraints(*(vars.begin()), bitwidthMap, unsignedEncoding);
    for (std::set<std::string>::iterator i = ++(vars.begin()), e = vars.end(); i != e; ++i) {
        res = new Operator(res, getBoundConstraints(*i, bitwidthMap, unsignedEncoding), Operator::And);
    }
    return res;
}

static std::list<Polynomial*> getNormArgs(std::string fun, std::list<Rule*> rules)
{
    if (fun.substr(fun.length() - 4) == "stop") {
        for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
            Rule *rule = *i;
            Term *rhs = rule->getRight();
            if (rhs->getFunctionSymbol() == fun) {
                return rhs->getArgs();
            }
        }
        std::cerr << "Did not find rule with matching RHS!" << std::endl;
        exit(1212);
    }
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *rule = *i;
        Term *lhs = rule->getLeft();
        if (lhs->getFunctionSymbol() == fun) {
            return lhs->getArgs();
        }
    }
    std::cerr << "Did not find rule with matching LHS!" << std::endl;
    exit(1212);
}

bool isNormal(Polynomial *p)
{
    return p->isVar() || p->isConst();
}

static std::map<unsigned int, Polynomial*> getNonNormalArgPositions(Term *rhs)
{
    std::map<unsigned int, Polynomial*> res;
    std::list<Polynomial*> args = rhs->getArgs();
    unsigned int c = 0;
    for (std::list<Polynomial*>::iterator i = args.begin(), e = args.end(); i != e; ++i) {
        Polynomial *p = *i;
        if (!isNormal(p)) {
            res.insert(std::make_pair(c, p));
        }
        ++c;
    }
    return res;
}

static std::list<Polynomial*> getNonNormalAtomPolynomials(Constraint *c)
{
    std::list<Polynomial*> res;
    std::list<Constraint*> atomics = c->getAtomics();
    for (std::list<Constraint*>::iterator i = atomics.begin(), e = atomics.end(); i != e; ++i) {
        Constraint *cc = *i;
        if (cc->getCType() == Constraint::CAtom) {
            Atom *atom = static_cast<Atom*>(cc);
            Polynomial *lhs = atom->getLeft();
            if (!isNormal(lhs)) {
                res.push_back(lhs);
            }
            Polynomial *rhs = atom->getRight();
            if (!isNormal(rhs)) {
                res.push_back(rhs);
            }
        }
    }
    return res;
}

static std::string get(std::list<Polynomial*> args, unsigned int c)
{
    std::list<Polynomial*>::iterator tmp = args.begin();
    for (unsigned int i = 0; i < c; ++i) {
        ++tmp;
    }
    return *((*tmp)->getVariables()->begin());
}

static Term *getNormRhs(Term *lhs, unsigned int c, Polynomial *addTerm, bool doAdd)
{
    std::list<Polynomial*> args = lhs->getArgs();
    std::list<Polynomial*> newargs;
    unsigned int cc = 0;
    for (std::list<Polynomial*>::iterator i = args.begin(), e = args.end(); i != e; ++i) {
        Polynomial *p = *i;
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
    return new Term(lhs->getFunctionSymbol(), newargs);
}

static std::list<Rule*> getNormRules(Term *normLhs, unsigned int c, std::map<std::string, unsigned int> &bitwidthMap, bool unsignedEncoding)
{
    std::list<Rule*> res;
    std::string var = get(normLhs->getArgs(), c);
    std::map<std::string, unsigned int>::iterator found = bitwidthMap.find(var);
    if (found == bitwidthMap.end()) {
        std::cerr << "Did not find bitwidth information for " << var << " in \"getNormRules\"!" << std::endl;
        exit(777);
    }
    unsigned int bitwidth = found->second;
    Polynomial *lower = NULL;
    Polynomial *upper = NULL;
    Polynomial *adder = NULL;
    Polynomial *pvar = new Polynomial(var);
    if (unsignedEncoding) {
        lower = Polynomial::null;
        upper = Polynomial::uimax(bitwidth);
    } else {
        lower = Polynomial::simin(bitwidth);
        upper = Polynomial::simax(bitwidth);
    }
    adder = Polynomial::power_of_two(bitwidth);
    Constraint *tooSmall = new Atom(pvar, lower, Atom::Lss);
    Constraint *tooBig = new Atom(pvar, upper, Atom::Gtr);
    Term *rhs1 = getNormRhs(normLhs, c, adder, true);
    Rule *rule1 = new Rule(normLhs, rhs1, tooSmall);
    Term *rhs2 = getNormRhs(normLhs, c, adder, false);
    Rule *rule2 = new Rule(normLhs, rhs2, tooBig);
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

static Rule *chainRules(Rule *rule1, Rule *rule2)
{
    Term *rhs1 = rule1->getRight();
    Term *lhs2 = rule2->getLeft();
    std::map<std::string, Polynomial*> subby;
    std::list<Polynomial*> rhs1args = rhs1->getArgs();
    std::list<Polynomial*> lhs2args = lhs2->getArgs();
    for (std::list<Polynomial*>::iterator i1 = rhs1args.begin(), e1 = rhs1args.end(), i2 = lhs2args.begin(); i1 != e1; ++i1, ++i2) {
        Polynomial *p = *i1;
        std::string var = *((*i2)->getVariables()->begin());
        subby.insert(std::make_pair(var, p));
    }
    return new Rule(rule1->getLeft(), rule2->getRight()->instantiate(&subby), new Operator(rule1->getConstraint(), rule2->getConstraint()->instantiate(&subby), Operator::And));
}

static std::pair<Rule*, Rule*> getTheNormRules(std::string rhsFun, unsigned int argPos, std::list<Rule*> &rules)
{
    std::list<Rule*> normRules;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *tmp = *i;
        if (tmp->getLeft()->getFunctionSymbol() == rhsFun && tmp->getRight()->getFunctionSymbol() == rhsFun) {
            Polynomial *theArg = tmp->getRight()->getArg(argPos);
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

static std::list<Rule*> getChainedNormRules(Rule *rule, std::list<Rule*> &rules)
{
    std::list<Rule*> res;
    std::list<Rule*> thusFar;
    Term *rhs = rule->getRight();
    std::list<Polynomial*> args = rhs->getArgs();
    thusFar.push_back(rule);
    std::string rhsFun = rhs->getFunctionSymbol();
    unsigned int argCount = 0;
    for (std::list<Polynomial*>::iterator ia = args.begin(), ea = args.end(); ia != ea; ++ia) {
        Polynomial *p = *ia;
        if (isNormal(p)) {
            ++ argCount;
            continue;
        }
        std::list<Rule*> tmp;
        for (std::list<Rule*>::iterator ir = thusFar.begin(), er = thusFar.end(); ir != er; ++ir) {
            Rule *toChain = *ir;
            // no chaining
            tmp.push_back(toChain);
            long int normStepsNeeded = p->normStepsNeeded();
            std::pair<Rule*, Rule*> theNormRules = getTheNormRules(rhsFun, argCount, rules);
            Rule *rule1 = toChain;
            Rule *rule2 = toChain;
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
    Rule *exitRule = NULL;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *tmp = *i;
        if (tmp->getLeft()->getFunctionSymbol() == rhsFun && tmp->getRight()->getFunctionSymbol() != rhsFun) {
            exitRule = tmp;
            break;
        }
    }
    if (exitRule == NULL) {
        std::cerr << "Did not find exit rule!" << std::endl;
        exit(777);
    }
    for (std::list<Rule*>::iterator i = thusFar.begin(), e = thusFar.end(); i != e; ++i) {
        res.push_back(chainRules(*i, exitRule));
    }
    return res;
}

static std::list<Rule*> eliminateUnneededNorms(std::list<Rule*> rules, std::set<std::string> &haveToKeep)
{
    std::list<Rule*> res;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *rule = *i;
        Term *lhs = rule->getLeft();
        Term *rhs = rule->getRight();
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
                std::list<Rule*> chainedRules = getChainedNormRules(rule, rules);
                res.insert(res.end(), chainedRules.begin(), chainedRules.end());
            }
        } else {
            // unrelated to norm
            res.push_back(rule);
        }
    }
    return res;
}

static std::list<Rule*> eliminateBlocks(std::list<Rule*> rules)
{
    std::list<Rule*> res;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *rule = *i;
        Term *lhs = rule->getLeft();
        Term *rhs = rule->getRight();
        std::string lhsFun = lhs->getFunctionSymbol();
        std::string rhsFun = rhs->getFunctionSymbol();
        if (isBlockSymbol(rhsFun)) {
            // chain
            for (std::list<Rule*>::iterator ii = rules.begin(), ee = rules.end(); ii != ee; ++ii) {
                Rule *inner = *ii;
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
static void printRules(std::string header, std::list<Rule*> rules)
{
    std::cout << header << std::endl;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        std::cout << (*i)->toString() << std::endl;
    }
    std::cout << header << std::endl;
}
*/

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

static Constraint *mapPolysToVars(Constraint *c, std::map<Polynomial*, std::string> &atomPolyToVarMap)
{
    if (atomPolyToVarMap.empty()) {
        return c;
    }
    std::list<Atom*> atoms = getAtoms(c);
    std::list<Atom*> newAtoms;
    for (std::list<Atom*>::iterator i = atoms.begin(), e = atoms.end(); i != e; ++i) {
        Atom *a = *i;
        Polynomial *lhs = a->getLeft();
        Polynomial *rhs = a->getRight();
        Polynomial *newLhs = NULL;
        for (std::map<Polynomial*, std::string>::iterator ii = atomPolyToVarMap.begin(), ee = atomPolyToVarMap.end(); ii != ee; ++ii) {
            if (ii->first->equals(lhs)) {
                newLhs = new Polynomial(ii->second);
            }
        }
        if (newLhs == NULL) {
            newLhs = lhs;
        }
        Polynomial *newRhs = NULL;
        for (std::map<Polynomial*, std::string>::iterator ii = atomPolyToVarMap.begin(), ee = atomPolyToVarMap.end(); ii != ee; ++ii) {
            if (ii->first->equals(rhs)) {
                newRhs = new Polynomial(ii->second);
            }
        }
        if (newRhs == NULL) {
            newRhs = rhs;
        }
        if (newLhs != lhs || newRhs != rhs) {
            newAtoms.push_back(new Atom(newLhs, newRhs, a->getAType()));
        } else {
            newAtoms.push_back(a);
        }
    }
    return makeConjunction(newAtoms);
}

std::list<Rule*> addBoundConditions(std::list<Rule*> rules, std::map<std::string, unsigned int> bitwidthMap, bool unsignedEncoding)
{
    std::list<Rule*> res;
    std::set<std::string> haveToKeep;
    unsigned int normCount = 0;
    unsigned int blockCount = 0;
    for (std::list<Rule*>::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
        Rule *rule = *i;
        Term *lhs = rule->getLeft();
        Term *rhs = rule->getRight();
        Constraint *c = rule->getConstraint();
        std::string lhsFun = lhs->getFunctionSymbol();
        std::string rhsFun = rhs->getFunctionSymbol();
        std::map<unsigned int, Polynomial*> nonNormal = getNonNormalArgPositions(rhs);
        std::list<Polynomial*> nonNormalAtomPolys = getNonNormalAtomPolynomials(c);
        if (nonNormal.empty() && nonNormalAtomPolys.empty()) {
            // rule only needs bound conditions
            std::set<std::string> *lhsVars = lhs->getVariables();
            std::set<std::string> *rhsVars = rhs->getVariables();
            std::set<std::string> vars;
            vars.insert(lhsVars->begin(), lhsVars->end());
            vars.insert(rhsVars->begin(), rhsVars->end());
            Constraint *bounds = getBoundConstraints(vars, bitwidthMap, unsignedEncoding);
            Constraint *conj = new Operator(c, bounds, Operator::And);
            Rule *newRule = new Rule(lhs, rhs, conj);
            res.push_back(newRule);
        } else {
            std::list<Polynomial*> ruleNormArgs = getNormArgs(rhsFun, rules);
            std::list<Polynomial*> condNormArgs;
            std::list<Polynomial*> rhsArgs = rhs->getArgs();

            std::string cond_norm = getNormFun(rhsFun, normCount);
            ++normCount;
            std::string rule_norm = getNormFun(rhsFun, normCount);
            ++normCount;
            std::string blocker = getBlockFun(rhsFun, blockCount);
            ++blockCount;

            // set up dummy vars for atom polys
            std::map<Polynomial*, std::string> atomPolyToVarMap;
            unsigned int counter = 0;
            for (std::list<Polynomial*>::iterator pi = nonNormalAtomPolys.begin(), pe = nonNormalAtomPolys.end(); pi != pe; ++pi) {
                Polynomial *pol = *pi;
                if (pol->normStepsNeeded() == -1) {
                    haveToKeep.insert(cond_norm);
                }
                std::string var = getNewVar(counter++);
                condNormArgs.push_back(new Polynomial(var));
                atomPolyToVarMap.insert(std::make_pair(pol, var));
                std::string tmpvar = *(pol->getVariables()->begin());
                std::map<std::string, unsigned int>::iterator tmpvari = bitwidthMap.find(tmpvar);
                if (tmpvari == bitwidthMap.end()) {
                    std::cerr << "Did not find bitwidth information for " << var << " in \"getBoundConstraints\"!" << std::endl;
                    exit(777);
                }
                bitwidthMap.insert(std::make_pair(var, tmpvari->second));
            }

            // rule needs bound conditions and normalization

            // lhs -> rhs_cond_norm [ bounds ]
            Constraint *bounds1 = getBoundConstraints(*(lhs->getVariables()), bitwidthMap, unsignedEncoding);
            std::list<Polynomial*> cond_norm_args = lhs->getArgs();
            cond_norm_args.insert(cond_norm_args.end(), nonNormalAtomPolys.begin(), nonNormalAtomPolys.end());
            Term *rhs_cond_norm = new Term(cond_norm, cond_norm_args);
            Rule *rule1 = new Rule(lhs, rhs_cond_norm, bounds1);
            res.push_back(rule1);

            // rhs_cond_norm -> blockrhs_rule_norm [ bounds /\ mapped(c) ]
            std::list<Polynomial*> cond_norm_done_args = lhs->getArgs();
            cond_norm_done_args.insert(cond_norm_done_args.end(), condNormArgs.begin(), condNormArgs.end());
            Term *rhs_cond_norm_done = new Term(cond_norm, cond_norm_done_args);
            Term *block = new Term(blocker, lhs->getArgs());
            Constraint *bounds2 = getBoundConstraints(*(rhs_cond_norm_done->getVariables()), bitwidthMap, unsignedEncoding);
            Constraint *newC = new Operator(mapPolysToVars(c, atomPolyToVarMap), bounds2, Operator::And);
            Rule *rule2 = new Rule(rhs_cond_norm_done, block, newC);

            // block -> rhs_rule_norm
            Term *rhs_rule_norm = new Term(rule_norm, rhsArgs);
            Rule *rule3 = new Rule(block, rhs_rule_norm, Constraint::_true);

            // rhs_rule_norm -> rhs [ bounds ]
            Term *rhs_rule_norm_done = new Term(rule_norm, ruleNormArgs);
            Term *normRhs = new Term(rhsFun, ruleNormArgs);
            Constraint *bounds3 = getBoundConstraints(*(rhs_rule_norm_done->getVariables()), bitwidthMap, unsignedEncoding);
            Rule *rule4 = new Rule(rhs_rule_norm_done, normRhs, bounds3);

            // do normalization for both
            unsigned int count = 0;
            for (std::list<Polynomial*>::iterator it = cond_norm_done_args.begin(), et = cond_norm_done_args.end(); it != et; ++it) {
                Polynomial *pol = *it;
                if (pol->isConst()) {
                    continue;
                }
                std::string var = *(pol->getVariables()->begin());
                if (isCNorm(var)) {
                    std::list<Rule*> normRules = getNormRules(rhs_cond_norm_done, count, bitwidthMap, unsignedEncoding);
                    res.insert(res.end(), normRules.begin(), normRules.end());
                }
                ++count;
            }
            res.push_back(rule2);
            res.push_back(rule3);
            count = 0;
            for (std::list<Polynomial*>::iterator ii = rhsArgs.begin(), ee = rhsArgs.end(); ii != ee; ++ii) {
                std::map<unsigned int, Polynomial*>::iterator it = nonNormal.find(count);
                if (it != nonNormal.end()) {
                    // normalize it!
                    std::list<Rule*> normRules = getNormRules(rhs_rule_norm_done, count, bitwidthMap, unsignedEncoding);
                    res.insert(res.end(), normRules.begin(), normRules.end());
                    Polynomial *p = it->second;
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
