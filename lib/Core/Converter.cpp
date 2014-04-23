// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Converter.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Constants.h>
#else
  #include <llvm/IR/Constants.h>
#endif
#include <llvm/ADT/SmallVector.h>
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/CallSite.h>
#else
  #include <llvm/IR/CallSite.h>
#endif
#if LLVM_VERSION < VERSION(3, 4)
  #include <llvm/Transforms/Utils/BasicBlockUtils.h>
#else
  #include <llvm/Analysis/CFG.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>

#define SMALL_VECTOR_SIZE 8

Converter::Converter(const llvm::Type *boolType, bool assumeIsControl, bool selectIsControl, bool onlyMultiPredIsControl, bool boundedIntegers, bool unsignedEncoding, bool onlyLoopConditions, DivRemConstraintType divisionConstraintType, bool bitwiseConditions, bool complexityTuples)
  : m_entryBlock(NULL),
    m_boolType(boolType),
    m_blockRules(),
    m_rules(),
    m_vars(),
    m_lhs(),
    m_counter(0),
    m_phase1(true),
    m_globals(),
    m_mmMap(),
    m_funcMayZap(),
    m_tfMap(),
    m_elcMap(),
    m_returns(),
    m_idMap(),
    m_nondef(0),
    m_assumeIsControl(assumeIsControl),
    m_selectIsControl(selectIsControl),
    m_onlyMultiPredIsControl(onlyMultiPredIsControl),
    m_controlPoints(),
    m_trivial(false),
    m_function(NULL),
    m_scc(),
    m_phiVars(),
    m_boundedIntegers(boundedIntegers),
    m_unsignedEncoding(unsignedEncoding),
    m_bitwidthMap(),
    m_onlyLoopConditions(onlyLoopConditions),
    m_loopConditionBlocks(),
    m_divisionConstraintType(divisionConstraintType),
    m_bitwiseConditions(bitwiseConditions),
    m_complexityTuples(complexityTuples),
    m_complexityLHSs()
{}

bool Converter::isTrivial(void)
{
    llvm::SmallVector<std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*>, SMALL_VECTOR_SIZE> res;
    llvm::FindFunctionBackedges(*m_function, res);

    if (res.empty()) {
        // no backedge
        for (llvm::Function::iterator bb = m_function->begin(), end = m_function->end(); bb != end; ++bb) {
            for (llvm::BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i) {
                if (llvm::isa<llvm::CallInst>(i)) {
                    llvm::CallInst *ci = llvm::cast<llvm::CallInst>(i);
                    llvm::Function *calledFunction = ci->getCalledFunction();
                    std::list<llvm::Function*> callees;
                    if (calledFunction != NULL) {
                        callees.push_back(calledFunction);
                    } else {
                        callees = getMatchingFunctions(*ci);
                    }
                    for (std::list<llvm::Function*>::iterator cf = callees.begin(), cfe = callees.end(); cf != cfe; ++cf) {
                        llvm::Function *callee = *cf;
                        if (m_scc.find(callee) != m_scc.end()) {
                            // call within SCC
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    } else {
        // backedge
        return false;
    }
}

void Converter::phase1(llvm::Function *function, std::set<llvm::Function*> &scc, MayMustMap &mmMap, std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > &funcMayZap, TrueFalseMap &tfMap, std::set<llvm::BasicBlock*> &lcbs, ConditionMap &elcMap)
{
    m_function = function;
    m_scc = scc;
    m_mmMap = mmMap;
    m_tfMap = tfMap;
    m_funcMayZap = funcMayZap;
    m_loopConditionBlocks = lcbs;
    m_elcMap = elcMap;
    m_globals.clear();
    m_blockRules.clear();
    m_rules.clear();
    m_vars.clear();
    m_lhs.clear();
    m_returns.clear();
    m_idMap.clear();
    m_phiVars.clear();
    m_controlPoints.clear();
    m_controlPoints.insert(getEval(m_function, "start"));
    m_controlPoints.insert(getEval(m_function, "stop"));
    m_counter = 0;
    m_nondef = 0;
    m_entryBlock = &function->getEntryBlock();
    m_trivial = isTrivial();

    for (llvm::Function::arg_iterator i = function->arg_begin(), e = function->arg_end(); i != e; ++i) {
        if (i->getType()->isIntegerTy() && i->getType() != m_boolType) {
            m_vars.push_back(getVar(i));
        }
    }
    llvm::Module *module = function->getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (globalType->isIntegerTy() && globalType != m_boolType) {
            std::string var = getVar(global);
            m_globals.push_back(global);
            m_vars.push_back(var);
            if (m_boundedIntegers) {
                m_bitwidthMap.insert(std::make_pair(var, llvm::cast<llvm::IntegerType>(globalType)->getBitWidth()));
            }
        }
    }
    if (!m_trivial) {
        m_phase1 = true;
        visit(function);
    }

    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i) {
        m_lhs.push_back(Polynomial::create(*i));
    }
}

void Converter::phase2(llvm::Function *function, std::set<llvm::Function*> &scc, MayMustMap &mmMap, std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > &funcMayZap, TrueFalseMap &tfMap, std::set<llvm::BasicBlock*> &lcbs, ConditionMap &elcMap)
{
    if (m_trivial) {
        m_rules.push_back(Rule::create(Term::create(getEval(m_function, "start"), m_lhs), Term::create(getEval(m_function, "stop"), m_lhs), Constraint::_true));
        return;
    }
    m_function = function;
    m_scc = scc;
    m_mmMap = mmMap;
    m_tfMap = tfMap;
    m_funcMayZap = funcMayZap;
    m_loopConditionBlocks = lcbs;
    m_elcMap = elcMap;
    m_phase1 = false;

    for (llvm::Function::iterator bb = m_function->begin(), end = m_function->end(); bb != end; ++bb) {
        visitBB(bb);
    }

    // add rules from returns to stop
    for (std::list<llvm::BasicBlock*>::iterator i = m_returns.begin(), e = m_returns.end(); i != e; ++i) {
        ref<Term> lhs = Term::create(getEval(*i, "out"), m_lhs);
        ref<Term> rhs = Term::create(getEval(m_function, "stop"), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    }
}

std::list<ref<Rule> > Converter::getRules()
{
    return m_rules;
}

std::list<ref<Rule> > Converter::getCondensedRules()
{
    std::list<ref<Rule> > good;
    std::list<ref<Rule> > junk;
    std::list<ref<Rule> > res;
    for (std::list<ref<Rule> >::iterator i = m_rules.begin(), e = m_rules.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::string f = rule->getLeft()->getFunctionSymbol();
        if (m_controlPoints.find(f) != m_controlPoints.end()) {
            good.push_back(rule);
        } else {
            junk.push_back(rule);
        }
    }
    for (std::list<ref<Rule> >::iterator i = good.begin(), e = good.end(); i != e; ++i) {
        ref<Rule> rule = *i;
        std::vector<ref<Rule> > todo;
        todo.push_back(rule);
        while (!todo.empty()) {
            ref<Rule> r = *todo.begin();
            todo.erase(todo.begin());
            ref<Term> rhs = r->getRight();
            std::string f = rhs->getFunctionSymbol();
            if (m_controlPoints.find(f) != m_controlPoints.end()) {
                res.push_back(r);
            } else {
                std::list<ref<Rule> > newtodo;
                for (std::list<ref<Rule> >::iterator ii = junk.begin(), ee = junk.end(); ii != ee; ++ii) {
                    ref<Rule> junkrule = *ii;
                    if (junkrule->getLeft()->getFunctionSymbol() == f) {
                        std::map<std::string, ref<Polynomial> > subby;
                        std::list<ref<Polynomial> > rhsargs = rhs->getArgs();
                        std::list<ref<Polynomial> >::iterator ai = rhsargs.begin();
                        for (std::list<std::string>::iterator vi = m_vars.begin(), ve = m_vars.end(); vi != ve; ++vi, ++ai) {
                            subby.insert(std::make_pair(*vi, *ai));
                        }
                        ref<Rule> newRule = Rule::create(r->getLeft(), junkrule->getRight()->instantiate(&subby), Operator::create(r->getConstraint(), junkrule->getConstraint()->instantiate(&subby), Operator::And));
                        newtodo.push_back(newRule);
                    }
                }
                todo.insert(todo.begin(), newtodo.begin(), newtodo.end());
            }
        }
    }
    return res;
}

std::string Converter::getVar(llvm::Value *V)
{
    std::string name = V->getName();
    std::ostringstream tmp;
    tmp << "v_" << name;
    std::string res = tmp.str();
    if (m_boundedIntegers && V->getType()->isIntegerTy()) {
        m_bitwidthMap.insert(std::make_pair(res, llvm::cast<llvm::IntegerType>(V->getType())->getBitWidth()));
    }
    return res;
}

std::string Converter::getEval(unsigned int i)
{
    std::ostringstream tmp;
    tmp << "eval_" << m_function->getName().str() << "_" << i;
    return tmp.str();
}

std::string Converter::getEval(llvm::BasicBlock *bb, std::string inout)
{
    std::ostringstream tmp;
    tmp << "eval_" << bb->getName().str() << '_' << inout;
    std::string res = tmp.str();
    if (inout == "in" && (!m_onlyMultiPredIsControl || bb->getUniquePredecessor() == NULL)) {
        m_controlPoints.insert(res);
    }
    return res;
}

std::string Converter::getEval(llvm::Function *func, std::string startstop)
{
    std::ostringstream tmp;
    tmp << "eval_" << func->getName().str() << "_" << startstop;
    return tmp.str();
}

ref<Polynomial> Converter::getPolynomial(llvm::Value *V)
{
    if (llvm::isa<llvm::ConstantInt>(V)) {
        mpz_t value;
        mpz_init(value);
        if (m_boundedIntegers && m_unsignedEncoding) {
            uint64_t cv = static_cast<llvm::ConstantInt*>(V)->getZExtValue();
            std::ostringstream tmp;
            tmp << cv;
            gmp_sscanf(tmp.str().data(), "%Zu", value);
        } else {
            int64_t cv = static_cast<llvm::ConstantInt*>(V)->getSExtValue();
            std::ostringstream tmp;
            tmp << cv;
            gmp_sscanf(tmp.str().data(), "%Zd", value);
        }
        ref<Polynomial> res = Polynomial::create(value);
        mpz_clear(value);
        return res;
    } else if (llvm::isa<llvm::Instruction>(V) || llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::GlobalVariable>(V)) {
        return Polynomial::create(getVar(V));
    } else {
        return Polynomial::create(getNondef(V));
    }
}

std::list<ref<Polynomial> > Converter::getNewArgs(llvm::Value &V, ref<Polynomial> p)
{
    std::list<ref<Polynomial> > res;
    std::string Vname = getVar(&V);
    std::list<ref<Polynomial> >::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        if (Vname == *i) {
            res.push_back(p);
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

std::list<ref<Polynomial> > Converter::getZappedArgs(std::set<llvm::GlobalVariable*> toZap)
{
    std::list<ref<Polynomial> > res;
    std::list<ref<Polynomial> >::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        bool doZap = false;
        const llvm::Type *zapType = NULL;
        for (std::set<llvm::GlobalVariable*>::iterator zap = toZap.begin(), zape = toZap.end(); zap != zape; ++zap) {
            if (getVar(*zap) == *i) {
                doZap = true;
                zapType = llvm::cast<llvm::PointerType>((*zap)->getType())->getContainedType(0);
                break;
            }
        }
        if (doZap) {
            std::string nondef = getNondef(NULL);
            if (m_boundedIntegers) {
                m_bitwidthMap.insert(std::make_pair(nondef, llvm::cast<llvm::IntegerType>(zapType)->getBitWidth()));
            }
            res.push_back(Polynomial::create(nondef));
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

std::list<ref<Polynomial> > Converter::getZappedArgs(std::set<llvm::GlobalVariable*> toZap, llvm::Value &V, ref<Polynomial> p)
{
    std::list<ref<Polynomial> > res;
    std::string Vname = getVar(&V);
    std::list<ref<Polynomial> >::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        bool doZap = false;
        const llvm::Type *zapType = NULL;
        for (std::set<llvm::GlobalVariable*>::iterator zap = toZap.begin(), zape = toZap.end(); zap != zape; ++zap) {
            if (getVar(*zap) == *i) {
                doZap = true;
                zapType = llvm::cast<llvm::PointerType>((*zap)->getType())->getContainedType(0);
                break;
            }
        }
        if (doZap) {
            std::string nondef = getNondef(NULL);
            if (m_boundedIntegers) {
                m_bitwidthMap.insert(std::make_pair(nondef, llvm::cast<llvm::IntegerType>(zapType)->getBitWidth()));
            }
            res.push_back(Polynomial::create(nondef));
        } else if (Vname == *i) {
            res.push_back(p);
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

ref<Constraint> Converter::getConditionFromValue(llvm::Value *cond)
{
    if (llvm::isa<llvm::ConstantInt>(cond)) {
        return Atom::create(getPolynomial(cond), Polynomial::null, Atom::Neq);
    }
    if (llvm::isa<llvm::Argument>(cond) && cond->getType() == m_boolType) {
        return Nondef::create();
    }
    if (!llvm::isa<llvm::Instruction>(cond)) {
        cond->dump();
        std::cerr << "Cannot handle non-instructions!" << std::endl;
        exit(5);
    }
    llvm::Instruction *I = llvm::cast<llvm::Instruction>(cond);
    return getConditionFromInstruction(I);
}

ref<Constraint> Converter::getConditionFromInstruction(llvm::Instruction *I)
{
    if (llvm::isa<llvm::ZExtInst>(I)) {
        return getConditionFromValue(I->getOperand(0));
    }
    if (I->getType() != m_boolType) {
        return Atom::create(getPolynomial(I), Polynomial::null, Atom::Neq);
    }
    if (llvm::isa<llvm::CmpInst>(I)) {
        llvm::CmpInst *cmp = llvm::cast<llvm::CmpInst>(I);
        if (cmp->getOperand(0)->getType()->isPointerTy()) {
            return Nondef::create();
        } else if (cmp->getOperand(0)->getType()->isFloatingPointTy()) {
            return Nondef::create();
        } else {
            llvm::CmpInst::Predicate pred = cmp->getPredicate();
            if (m_boundedIntegers && !m_unsignedEncoding && (pred == llvm::CmpInst::ICMP_UGT || pred == llvm::CmpInst::ICMP_UGE || pred == llvm::CmpInst::ICMP_ULT || pred == llvm::CmpInst::ICMP_ULE)) {
                return getUnsignedComparisonForSignedBounded(pred, getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)));
            } else if (m_boundedIntegers && m_unsignedEncoding && (pred == llvm::CmpInst::ICMP_SGT || pred == llvm::CmpInst::ICMP_SGE || pred == llvm::CmpInst::ICMP_SLT || pred == llvm::CmpInst::ICMP_SLE)) {
                unsigned int bitwidth = llvm::cast<llvm::IntegerType>(cmp->getOperand(0)->getType())->getBitWidth();
                return getSignedComparisonForUnsignedBounded(pred, getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)), bitwidth);
            } else {
                return Atom::create(getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)), getAtomType(pred));
            }
        }
    }
    std::string opcodeName = I->getOpcodeName();
    if (opcodeName == "and") {
        return Operator::create(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(1)), Operator::And);
    } else if (opcodeName == "or") {
        return Operator::create(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(1)), Operator::Or);
    } else if (opcodeName == "xor") {
        llvm::ConstantInt *ci;
        unsigned int realIdx;
        if (llvm::isa<llvm::ConstantInt>(I->getOperand(0))) {
            ci = llvm::cast<llvm::ConstantInt>(I->getOperand(0));
            realIdx = 1;
        } else {
            ci = llvm::cast<llvm::ConstantInt>(I->getOperand(1));
            realIdx = 0;
        }
        if (ci->isOne()) {
            return Negation::create(getConditionFromValue(I->getOperand(realIdx)));
        } else {
            return getConditionFromValue(I->getOperand(realIdx));
        }
    } else if (opcodeName == "select") {
        llvm::ConstantInt *opOne = llvm::dyn_cast<llvm::ConstantInt>(I->getOperand(1));
        llvm::ConstantInt *opTwo = llvm::dyn_cast<llvm::ConstantInt>(I->getOperand(2));
        if (opOne != NULL && opOne->isOne() && opTwo != NULL && !opTwo->isOne()) {
            // select ... true false (this actually occurs in some bitcode)
            return getConditionFromValue(I->getOperand(0));
        } else if (opOne != NULL && opOne->isOne()) {
            // select ... true ...
            // true --> or
            return Operator::create(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(2)), Operator::Or);
        } else if (opTwo != NULL && !opTwo->isOne()) {
            // select ... ... false
            return Operator::create(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(1)), Operator::And);
        } else {
            // select ... ... ...
            ref<Constraint> test = getConditionFromValue(I->getOperand(0));
            ref<Constraint> negTest = Negation::create(test);
            ref<Constraint> truePart = Operator::create(test, getConditionFromValue(I->getOperand(1)), Operator::And);
            ref<Constraint> falsePart = Operator::create(negTest, getConditionFromValue(I->getOperand(2)), Operator::And);
            return Operator::create(truePart, falsePart, Operator::Or);
        }
    } else {
        return Nondef::create();
    }
}

ref<Constraint> Converter::getUnsignedComparisonForSignedBounded(llvm::CmpInst::Predicate pred, ref<Polynomial> x, ref<Polynomial> y)
{
    switch (pred) {
    case llvm::CmpInst::ICMP_UGT:
    case llvm::CmpInst::ICMP_UGE: {
        ref<Constraint> xge = Atom::create(x, Polynomial::null, Atom::Geq);
        ref<Constraint> yge = Atom::create(y, Polynomial::null, Atom::Geq);
        ref<Constraint> xlt = Atom::create(x, Polynomial::null, Atom::Lss);
        ref<Constraint> ylt = Atom::create(y, Polynomial::null, Atom::Lss);
        ref<Constraint> gege = Operator::create(xge, yge, Operator::And);
        ref<Constraint> ltlt = Operator::create(xlt, ylt, Operator::And);
        ref<Constraint> ltge = Operator::create(xlt, yge, Operator::And);
        ref<Constraint> disj1 = Operator::create(gege, Atom::create(x, y, getAtomType(pred)), Operator::And);
        ref<Constraint> disj2 = Operator::create(ltlt, Atom::create(x, y, getAtomType(pred)), Operator::And);
        ref<Constraint> disj3 = ltge;
        ref<Constraint> tmp = Operator::create(disj1, disj2, Operator::Or);
        return Operator::create(tmp, disj3, Operator::Or);
    }
    case llvm::CmpInst::ICMP_ULT:
        return getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_UGT, y, x);
    case llvm::CmpInst::ICMP_ULE:
        return getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_UGE, y, x);
    case llvm::CmpInst::ICMP_EQ:
    case llvm::CmpInst::ICMP_NE:
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::ICMP_SLT:
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::BAD_ICMP_PREDICATE:
    case llvm::CmpInst::FCMP_FALSE:
    case llvm::CmpInst::FCMP_OEQ:
    case llvm::CmpInst::FCMP_OGT:
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_OLT:
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_ONE:
    case llvm::CmpInst::FCMP_ORD:
    case llvm::CmpInst::FCMP_UNO:
    case llvm::CmpInst::FCMP_UEQ:
    case llvm::CmpInst::FCMP_UGT:
    case llvm::CmpInst::FCMP_UGE:
    case llvm::CmpInst::FCMP_ULT:
    case llvm::CmpInst::FCMP_ULE:
    case llvm::CmpInst::FCMP_UNE:
    case llvm::CmpInst::FCMP_TRUE:
    case llvm::CmpInst::BAD_FCMP_PREDICATE:
    default:
        std::cerr << "Unexpected unsigned comparison predicate" << std::endl;
        exit(7);
    }
}

ref<Constraint> Converter::getSignedComparisonForUnsignedBounded(llvm::CmpInst::Predicate pred, ref<Polynomial> x, ref<Polynomial> y, unsigned int bitwidth)
{
    ref<Polynomial> maxpos = Polynomial::simax(bitwidth);
    switch (pred) {
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::ICMP_SGE: {
        ref<Constraint> xle = Atom::create(x, maxpos, Atom::Leq);
        ref<Constraint> yle = Atom::create(y, maxpos, Atom::Leq);
        ref<Constraint> xgt = Atom::create(x, maxpos, Atom::Gtr);
        ref<Constraint> ygt = Atom::create(y, maxpos, Atom::Gtr);
        ref<Constraint> lele = Operator::create(xle, yle, Operator::And);
        ref<Constraint> gtgt = Operator::create(xgt, ygt, Operator::And);
        ref<Constraint> legt = Operator::create(xle, ygt, Operator::And);
        ref<Constraint> disj1 = Operator::create(lele, Atom::create(x, y, getAtomType(pred)), Operator::And);
        ref<Constraint> disj2 = Operator::create(gtgt, Atom::create(x, y, getAtomType(pred)), Operator::And);
        ref<Constraint> disj3 = legt;
        ref<Constraint> tmp = Operator::create(disj1, disj2, Operator::Or);
        return Operator::create(tmp, disj3, Operator::Or);
    }
    case llvm::CmpInst::ICMP_SLT:
        return getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, y, x, bitwidth);
    case llvm::CmpInst::ICMP_SLE:
        return getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGE, y, x, bitwidth);
    case llvm::CmpInst::ICMP_EQ:
    case llvm::CmpInst::ICMP_NE:
    case llvm::CmpInst::ICMP_UGT:
    case llvm::CmpInst::ICMP_UGE:
    case llvm::CmpInst::ICMP_ULT:
    case llvm::CmpInst::ICMP_ULE:
    case llvm::CmpInst::BAD_ICMP_PREDICATE:
    case llvm::CmpInst::FCMP_FALSE:
    case llvm::CmpInst::FCMP_OEQ:
    case llvm::CmpInst::FCMP_OGT:
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_OLT:
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_ONE:
    case llvm::CmpInst::FCMP_ORD:
    case llvm::CmpInst::FCMP_UNO:
    case llvm::CmpInst::FCMP_UEQ:
    case llvm::CmpInst::FCMP_UGT:
    case llvm::CmpInst::FCMP_UGE:
    case llvm::CmpInst::FCMP_ULT:
    case llvm::CmpInst::FCMP_ULE:
    case llvm::CmpInst::FCMP_UNE:
    case llvm::CmpInst::FCMP_TRUE:
    case llvm::CmpInst::BAD_FCMP_PREDICATE:
    default:
        std::cerr << "Unexpected signed comparison predicate" << std::endl;
        exit(7);
    }
}

Atom::AType Converter::getAtomType(llvm::CmpInst::Predicate pred)
{
    switch (pred) {
    case llvm::CmpInst::ICMP_EQ:
        return Atom::Equ;
    case llvm::CmpInst::ICMP_NE:
        return Atom::Neq;
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::ICMP_UGT:
        return Atom::Gtr;
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::ICMP_UGE:
        return Atom::Geq;
    case llvm::CmpInst::ICMP_SLT:
    case llvm::CmpInst::ICMP_ULT:
        return Atom::Lss;
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::ICMP_ULE:
        return Atom::Leq;
    case llvm::CmpInst::BAD_ICMP_PREDICATE:
    case llvm::CmpInst::FCMP_FALSE:
    case llvm::CmpInst::FCMP_OEQ:
    case llvm::CmpInst::FCMP_OGT:
    case llvm::CmpInst::FCMP_OGE:
    case llvm::CmpInst::FCMP_OLT:
    case llvm::CmpInst::FCMP_OLE:
    case llvm::CmpInst::FCMP_ONE:
    case llvm::CmpInst::FCMP_ORD:
    case llvm::CmpInst::FCMP_UNO:
    case llvm::CmpInst::FCMP_UEQ:
    case llvm::CmpInst::FCMP_UGT:
    case llvm::CmpInst::FCMP_UGE:
    case llvm::CmpInst::FCMP_ULT:
    case llvm::CmpInst::FCMP_ULE:
    case llvm::CmpInst::FCMP_UNE:
    case llvm::CmpInst::FCMP_TRUE:
    case llvm::CmpInst::BAD_FCMP_PREDICATE:
    default:
        std::cerr << "Unknown comparison predicate" << std::endl;
        exit(7);
    }
}

ref<Constraint> Converter::buildConjunction(std::set<llvm::Value*> &trues, std::set<llvm::Value*> &falses)
{
    ref<Constraint> res = Constraint::_true;
    for (std::set<llvm::Value*>::iterator i = trues.begin(), e = trues.end(); i != e; ++i) {
        ref<Constraint> c = getConditionFromValue(*i)->toNNF(false);
        res = Operator::create(res, c, Operator::And);
    }
    for (std::set<llvm::Value*>::iterator i = falses.begin(), e = falses.end(); i != e; ++i) {
        ref<Constraint> c = getConditionFromValue(*i)->toNNF(true);
        res = Operator::create(res, c, Operator::And);
    }
    return res;
}

ref<Constraint> Converter::buildBoundConjunction(std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > &bounds)
{
    ref<Constraint> res = Constraint::_true;
    for (std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> >::iterator i = bounds.begin(), e = bounds.end(); i != e; ++i) {
        ref<Polynomial> p = getPolynomial(i->first);
        ref<Polynomial> q1 = getPolynomial(i->third);
        ref<Polynomial> q2 = getPolynomial(i->fourth);
        ref<Constraint> c = Atom::create(p, q1->add(q2), getAtomType(i->second));
        res = Operator::create(res, c, Operator::And);
    }
    return res;
}

void Converter::visitBB(llvm::BasicBlock *bb)
{
    // start
    if (bb == m_entryBlock) {
        ref<Term> lhs = Term::create(getEval(m_function, "start"), m_lhs);
        ref<Term> rhs = Term::create(getEval(bb, "in"), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    }

    // return
    if (llvm::isa<llvm::ReturnInst>(bb->getTerminator())) {
        m_returns.push_back(bb);
    }

    // visit instructions
    m_blockRules.clear();
    visit(*bb);

    // jump from bb_in to first instruction or bb_out
    // get condition
    ref<Constraint> cond;
    TrueFalseMap::iterator it = m_tfMap.find(bb);
    if (it != m_tfMap.end()) {
        TrueFalsePair tfp = it->second;
        std::set<llvm::Value*> trues = tfp.first;
        std::set<llvm::Value*> falses = tfp.second;
        cond = buildConjunction(trues, falses);
    } else {
        cond = Constraint::_true;
    }
    ConditionMap::iterator it2 = m_elcMap.find(bb);
    if (it2 != m_elcMap.end()) {
        std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > bounds = it2->second;
        ref<Constraint> c_new = buildBoundConjunction(bounds);
        cond = Operator::create(cond, c_new, Operator::And);
    }
    // find first instruction
    llvm::Instruction *first = NULL;
    unsigned int firstID = 0;
    for (llvm::BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i) {
        std::map<llvm::Instruction*, unsigned int>::iterator found = m_idMap.find(i);
        if (found != m_idMap.end()) {
            first = i;
            firstID = found->second;
            break;
        }
    }
    if (first != NULL) {
        ref<Term> lhs = Term::create(getEval(bb, "in"), m_lhs);
        ref<Term> rhs = Term::create(getEval(firstID), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, cond);
        m_rules.push_back(rule);
    } else {
        ref<Term> lhs = Term::create(getEval(bb, "in"), m_lhs);
        ref<Term> rhs = Term::create(getEval(bb, "out"), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, cond);
        m_rules.push_back(rule);
    }

    // insert rules for instructions
    m_rules.insert(m_rules.end(), m_blockRules.begin(), m_blockRules.end());

    // jump from last instruction in bb to bb_out
    llvm::Instruction *last = NULL;
    unsigned int lastID = 0;
    for (llvm::BasicBlock::iterator i = bb->begin(), e = bb->end(); i != e; ++i) {
        // terminators are not in the map
        std::map<llvm::Instruction*, unsigned int>::iterator found = m_idMap.find(i);
        if (found != m_idMap.end()) {
            last = i;
            lastID = found->second;
        }
    }
    if (last != NULL) {
        ref<Term> lhs = Term::create(getEval(lastID+1), m_lhs);
        m_counter++;
        ref<Term> rhs = Term::create(getEval(bb, "out"), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    }

    // jump from bb_out to succs
    llvm::TerminatorInst *terminator = bb->getTerminator();
    if (llvm::isa<llvm::ReturnInst>(terminator)) {
    } else if (llvm::isa<llvm::UnreachableInst>(terminator)) {
        ref<Term> lhs = Term::create(getEval(bb, "out"), m_lhs);
        ref<Term> rhs = Term::create(getEval(m_function, "stop"), m_lhs);
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    } else {
        llvm::BranchInst *branch = llvm::cast<llvm::BranchInst>(terminator);
        bool useCondition = (!m_onlyLoopConditions || m_loopConditionBlocks.find(bb) != m_loopConditionBlocks.end());
        if (branch->isUnconditional()) {
            ref<Term> lhs = Term::create(getEval(bb, "out"), m_lhs);
            ref<Term> rhs = Term::create(getEval(branch->getSuccessor(0), "in"), getArgsWithPhis(bb, branch->getSuccessor(0)));
            ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
            m_rules.push_back(rule);
        } else {
            ref<Term> lhs = Term::create(getEval(bb, "out"), m_lhs);
            ref<Term> rhs1 = Term::create(getEval(branch->getSuccessor(0), "in"), getArgsWithPhis(bb, branch->getSuccessor(0)));
            ref<Term> rhs2 = Term::create(getEval(branch->getSuccessor(1), "in"), getArgsWithPhis(bb, branch->getSuccessor(1)));
            ref<Constraint> c = getConditionFromValue(branch->getCondition());
            ref<Rule> rule1 = Rule::create(lhs, rhs1, useCondition ? c->toNNF(false) : Constraint::_true);
            m_rules.push_back(rule1);
            ref<Rule> rule2 = Rule::create(lhs, rhs2, useCondition ? c->toNNF(true) : Constraint::_true);
            m_rules.push_back(rule2);
        }
    }
}

std::list<ref<Polynomial> > Converter::getArgsWithPhis(llvm::BasicBlock *from, llvm::BasicBlock *to)
{
    std::list<ref<Polynomial> > res;
    std::map<std::string, ref<Polynomial> > phiValues;
    for (llvm::BasicBlock::iterator i = to->begin(), e = to->end(); i != e; ++i) {
        if (!llvm::isa<llvm::PHINode>(*i)) {
            break;
        }
        llvm::PHINode *phi = llvm::cast<llvm::PHINode>(i);
        if (phi->getType() == m_boolType || !phi->getType()->isIntegerTy()) {
            continue;
        }
        std::string PHIName = getVar(phi);
        phiValues.insert(std::make_pair(PHIName, getPolynomial(phi->getIncomingValueForBlock(from))));
    }
    std::list<ref<Polynomial> >::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        std::map<std::string, ref<Polynomial> >::iterator found = phiValues.find(*i);
        if (found == phiValues.end()) {
            res.push_back(*pp);
        } else {
            res.push_back(found->second);
        }
    }
    return res;
}

void Converter::visitTerminatorInst(llvm::TerminatorInst&)
{}

void Converter::visitGenericInstruction(llvm::Instruction &I, std::list<ref<Polynomial> > newArgs, ref<Constraint> c)
{
    m_idMap.insert(std::make_pair(&I, m_counter));
    ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
    ++m_counter;
    ref<Term> rhs = Term::create(getEval(m_counter), newArgs);
    ref<Rule> rule = Rule::create(lhs, rhs, c);
    m_blockRules.push_back(rule);
}

void Converter::visitGenericInstruction(llvm::Instruction &I, ref<Polynomial> value, ref<Constraint> c)
{
    visitGenericInstruction(I, getNewArgs(I, value), c);
}

void Converter::visitAdd(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType || I.getType()->isVectorTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> p1 = getPolynomial(I.getOperand(0));
        ref<Polynomial> p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->add(p2));
    }
}

void Converter::visitSub(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType || I.getType()->isVectorTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> p1 = getPolynomial(I.getOperand(0));
        ref<Polynomial> p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->sub(p2));
    }
}

void Converter::visitMul(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType || I.getType()->isVectorTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> p1 = getPolynomial(I.getOperand(0));
        ref<Polynomial> p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->mult(p2));
    }
}

ref<Constraint> Converter::getSDivConstraint(DivConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    ref<Constraint> case1 = Operator::create(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = x
    ref<Constraint> case2 = Operator::create(store.yEQone, store.zEQx, Operator::And);
    // 3. y = -1 /\ z = -x
    ref<Constraint> case3 = Operator::create(store.yEQnegone, store.zEQnegx, Operator::And);
    // 4. y > 1 /\ x > 0 /\ z >= 0 /\ z < x
    ref<Constraint> case41 = Operator::create(store.yGTRone, store.xGTRnull, Operator::And);
    ref<Constraint> case42 = Operator::create(case41, store.zGEQnull, Operator::And);
    ref<Constraint> case4 = Operator::create(case42, store.zLSSx, Operator::And);
    // 5. y > 1 /\ x < 0 /\ z <= 0 /\ z > x
    ref<Constraint> case51 = Operator::create(store.yGTRone, store.xLSSnull, Operator::And);
    ref<Constraint> case52 = Operator::create(case51, store.zLEQnull, Operator::And);
    ref<Constraint> case5 = Operator::create(case52, store.zGTRx, Operator::And);
    // 6. y < -1 /\ x > 0 /\ z <= 0 /\ z > -x
    ref<Constraint> case61 = Operator::create(store.yLSSnegone, store.xGTRnull, Operator::And);
    ref<Constraint> case62 = Operator::create(case61, store.zLEQnull, Operator::And);
    ref<Constraint> case6 = Operator::create(case62, store.zGTRnegx, Operator::And);
    // 7. y < -1 /\ x < 0 /\ z >= 0 /\ z < -x
    ref<Constraint> case71 = Operator::create(store.yLSSnegone, store.xLSSnull, Operator::And);
    ref<Constraint> case72 = Operator::create(case71, store.zGEQnull, Operator::And);
    ref<Constraint> case7 = Operator::create(case72, store.zLSSnegx, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    disj = Operator::create(disj, case4, Operator::Or);
    disj = Operator::create(disj, case5, Operator::Or);
    disj = Operator::create(disj, case6, Operator::Or);
    disj = Operator::create(disj, case7, Operator::Or);
    return disj;
}

ref<Constraint> Converter::getSDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;
    ref<Polynomial> negone = Polynomial::negone;
    ref<Polynomial> negupper = upper->constMult(Polynomial::_negone);

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;
    ref<Polynomial> negx = negupper;

    DivConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.yEQnegone = Atom::create(y, negone, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.zEQx = Atom::create(z, x, Atom::Equ);
    store.zEQnegx = Atom::create(z, negx, Atom::Equ);
    store.yGTRone = Atom::create(y, one, Atom::Gtr);
    store.xGTRnull = Atom::create(x, null, Atom::Gtr);
    store.zGEQnull = Atom::create(z, null, Atom::Geq);
    store.zLSSx = Atom::create(z, x, Atom::Lss);
    store.xLSSnull = Atom::create(x, null, Atom::Lss);
    store.zLEQnull = Atom::create(z, null, Atom::Leq);
    store.zGTRx = Atom::create(z, x, Atom::Gtr);
    store.yLSSnegone = Atom::create(y, negone, Atom::Lss);
    store.zGTRnegx = Atom::create(z, negx, Atom::Gtr);
    store.zLSSnegx = Atom::create(z, negx, Atom::Lss);

    return getSDivConstraint(store);
}

ref<Constraint> Converter::getSDivConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    return getSDivConstraintForUnbounded(upper, lower, res);
}

ref<Constraint> Converter::getSDivConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res, unsigned int bitwidth)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;
    ref<Polynomial> negone = Polynomial::uimax(bitwidth);
    ref<Polynomial> maxpos = Polynomial::simax(bitwidth);
    ref<Polynomial> minneg = Polynomial::simin_as_ui(bitwidth);
    ref<Polynomial> negupper = upper->constMult(Polynomial::_negone);

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;
    ref<Polynomial> negx = negupper;

    DivConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.yEQnegone = Atom::create(y, negone, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.zEQx = Atom::create(z, x, Atom::Equ);
    store.zEQnegx = Atom::create(z, negx, Atom::Equ);
    ref<Constraint> yGTRone1 = Atom::create(y, one, Atom::Gtr);
    ref<Constraint> yGTRone2 = Atom::create(y, maxpos, Atom::Leq);
    store.yGTRone = Operator::create(yGTRone1, yGTRone2, Operator::And);
    ref<Constraint> xGTRnull1 = Atom::create(x, null, Atom::Gtr);
    ref<Constraint> xGTRnull2 = Atom::create(x, maxpos, Atom::Leq);
    store.xGTRnull = Operator::create(xGTRnull1, xGTRnull2, Operator::And);
    store.zGEQnull = Atom::create(z, maxpos, Atom::Leq);
    store.zLSSx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, x, bitwidth);
    store.xLSSnull = Atom::create(x, minneg, Atom::Geq);
    ref<Constraint> zLEQnull1 = Atom::create(z, minneg, Atom::Geq);
    ref<Constraint> zLEQnull2 = Atom::create(z, null, Atom::Equ);
    store.zLEQnull = Operator::create(zLEQnull1, zLEQnull2, Operator::Or);
    store.zGTRx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, x, bitwidth);
    ref<Constraint> yLSSnegone1 = Atom::create(y, minneg, Atom::Geq);
    ref<Constraint> yLSSnegone2 = Atom::create(y, negone, Atom::Lss);
    store.yLSSnegone = Operator::create(yLSSnegone1, yLSSnegone2, Operator::And);
    store.zGTRnegx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, negx, bitwidth);
    store.zLSSnegx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, negx, bitwidth);

    return getSDivConstraint(store);
}

ref<Constraint> Converter::getExactSDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    // x/y = z
    ref<Polynomial> px = upper;
    ref<Polynomial> py = lower;
    ref<Polynomial> pz = res;
    ref<Polynomial> pnegx = px->constMult(Polynomial::_negone);
    ref<Polynomial> pnegy = py->constMult(Polynomial::_negone);
    ref<Polynomial> pnull = Polynomial::null;
    // 1. x = 0 /\ z = 0
    ref<Constraint> pxnull = Atom::create(px, pnull, Atom::Equ);
    ref<Constraint> pznull = Atom::create(pz, pnull, Atom::Equ);
    ref<Constraint> case1 = Operator::create(pxnull, pznull, Operator::And);
    // 2. y > 0 /\ x > 0 /\ z >= 0 /\ x - y*z >= 0 /\ x - y*z < y
    ref<Constraint> pygtrnull = Atom::create(py, pnull, Atom::Gtr);
    ref<Constraint> pxgtrnull = Atom::create(px, pnull, Atom::Gtr);
    ref<Constraint> pzgeqnull = Atom::create(pz, pnull, Atom::Geq);
    ref<Polynomial> term2 = px->sub(py->mult(pz));
    ref<Constraint> term2geqnull = Atom::create(term2, pnull, Atom::Geq);
    ref<Constraint> term2lssy = Atom::create(term2, py, Atom::Lss);
    ref<Constraint> case21 = Operator::create(pygtrnull, pxgtrnull, Operator::And);
    ref<Constraint> case22 = Operator::create(case21, pzgeqnull, Operator::And);
    ref<Constraint> case23 = Operator::create(case22, term2geqnull, Operator::And);
    ref<Constraint> case2 = Operator::create(case23, term2lssy, Operator::And);
    // 3. y < 0 /\ x > 0 /\ z <= 0 /\ x - y*z >= 0 /\ x - y*z < -y
    ref<Constraint> pylssnull = Atom::create(py, pnull, Atom::Lss);
    ref<Constraint> pzleqnull = Atom::create(pz, pnull, Atom::Leq);
    ref<Constraint> term2lssnegy = Atom::create(term2, pnegy, Atom::Lss);
    ref<Constraint> case31 = Operator::create(pylssnull, pxgtrnull, Operator::And);
    ref<Constraint> case32 = Operator::create(case31, pzleqnull, Operator::And);
    ref<Constraint> case33 = Operator::create(case32, term2geqnull, Operator::And);
    ref<Constraint> case3 = Operator::create(case33, term2lssnegy, Operator::And);
    // 4. y > 0 /\ x < 0 /\ z <= 0 /\ -x + y*z >= 0 /\ -x + y*z < y
    ref<Constraint> pxlssnull = Atom::create(px, pnull, Atom::Lss);
    ref<Polynomial> term4 = pnegx->add(py->mult(pz));
    ref<Constraint> term4geqnull = Atom::create(term4, pnull, Atom::Geq);
    ref<Constraint> term4lssy = Atom::create(term4, py, Atom::Lss);
    ref<Constraint> case41 = Operator::create(pygtrnull, pxlssnull, Operator::And);
    ref<Constraint> case42 = Operator::create(case41, pzleqnull, Operator::And);
    ref<Constraint> case43 = Operator::create(case42, term4geqnull, Operator::And);
    ref<Constraint> case4 = Operator::create(case43, term4lssy, Operator::And);
    // 5. y < 0 /\ x < 0 /\ z >= 0 /\ -x + y*z >= 0 /\ -x + y*z < -y
    ref<Constraint> term4lssnegy = Atom::create(term4, pnegy, Atom::Lss);
    ref<Constraint> case51 = Operator::create(pylssnull, pxlssnull, Operator::And);
    ref<Constraint> case52 = Operator::create(case51, pzgeqnull, Operator::And);
    ref<Constraint> case53 = Operator::create(case52, term4geqnull, Operator::And);
    ref<Constraint> case5 = Operator::create(case53, term4lssnegy, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    disj = Operator::create(disj, case4, Operator::Or);
    disj = Operator::create(disj, case5, Operator::Or);
    return disj;
}

void Converter::visitSDiv(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType || I.getType()->isVectorTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        ref<Constraint> divC;
        ref<Polynomial> upper = getPolynomial(I.getOperand(0));
        ref<Polynomial> lower = getPolynomial(I.getOperand(1));
        if (m_divisionConstraintType != None) {
            if (m_boundedIntegers) {
                unsigned int bitwidth = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
                divC = m_unsignedEncoding ? getSDivConstraintForUnsignedBounded(upper, lower, nondef, bitwidth) : getSDivConstraintForSignedBounded(upper, lower, nondef);
            } else {
                divC = (m_divisionConstraintType == Exact) ? getExactSDivConstraintForUnbounded(upper, lower, nondef) : getSDivConstraintForUnbounded(upper, lower, nondef);
            }
        } else {
            divC = Constraint::_true;
        }
        visitGenericInstruction(I, nondef, divC);
    }
}

ref<Constraint> Converter::getUDivConstraint(DivConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    ref<Constraint> case1 = Operator::create(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = x
    ref<Constraint> case2 = Operator::create(store.yEQone, store.zEQx, Operator::And);
    // 3. y > 1 /\ x > 0 /\ z < x
    ref<Constraint> case31 = Operator::create(store.yGTRone, store.xGTRnull, Operator::And);
    ref<Constraint> case3 = Operator::create(case31, store.zLSSx, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    return disj;
}

ref<Constraint> Converter::getUDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    return getSDivConstraintForUnbounded(upper, lower, res);
}

ref<Constraint> Converter::getUDivConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;

    DivConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.zEQx = Atom::create(z, x, Atom::Equ);
    ref<Constraint> yGTRone1 = Atom::create(y, one, Atom::Gtr);
    ref<Constraint> yGTRone2 = Atom::create(y, null, Atom::Lss);
    store.yGTRone = Operator::create(yGTRone1, yGTRone2, Operator::Or);
    ref<Constraint> xGTRnull1 = Atom::create(x, null, Atom::Gtr);
    ref<Constraint> xGTRnull2 = Atom::create(x, null, Atom::Lss);
    store.xGTRnull = Operator::create(xGTRnull1, xGTRnull2, Operator::Or);
    store.zLSSx = getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_ULT, z, x);

    return getUDivConstraint(store);
}

ref<Constraint> Converter::getUDivConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;

    DivConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.zEQx = Atom::create(z, x, Atom::Equ);
    store.yGTRone = Atom::create(y, one, Atom::Gtr);
    store.xGTRnull = Atom::create(x, null, Atom::Gtr);
    store.zLSSx = Atom::create(z, x, Atom::Lss);

    return getUDivConstraint(store);
}

ref<Constraint> Converter::getExactUDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    return getExactSDivConstraintForUnbounded(upper, lower, res);
}

void Converter::visitUDiv(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType || I.getType()->isVectorTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        ref<Constraint> divC;
        ref<Polynomial> upper = getPolynomial(I.getOperand(0));
        ref<Polynomial> lower = getPolynomial(I.getOperand(1));
        if (m_divisionConstraintType != None) {
            if (m_boundedIntegers) {
                divC = m_unsignedEncoding ? getUDivConstraintForUnsignedBounded(upper, lower, nondef) : getUDivConstraintForSignedBounded(upper, lower, nondef);
            } else {
                divC = (m_divisionConstraintType == Exact) ? getExactUDivConstraintForUnbounded(upper, lower, nondef) : getUDivConstraintForUnbounded(upper, lower, nondef);
            }
        } else {
            divC = Constraint::_true;
        }
        visitGenericInstruction(I, nondef, divC);
    }
}

ref<Constraint> Converter::getSRemConstraint(RemConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    ref<Constraint> case1 = Operator::create(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = 0
    ref<Constraint> case2 = Operator::create(store.yEQone, store.zEQnull, Operator::And);
    // 3. y = -1 /\ z = 0
    ref<Constraint> case3 = Operator::create(store.yEQnegone, store.zEQnull, Operator::And);
    // 4. y > 1 /\ x > 0 /\ z >= 0 /\ z < y
    ref<Constraint> case41 = Operator::create(store.yGTRone, store.xGTRnull, Operator::And);
    ref<Constraint> case42 = Operator::create(case41, store.zGEQnull, Operator::And);
    ref<Constraint> case4 = Operator::create(case42, store.zLSSy, Operator::And);
    // 5. y > 1 /\ x < 0 /\ z <= 0 /\ z > -y
    ref<Constraint> case51 = Operator::create(store.yGTRone, store.xLSSnull, Operator::And);
    ref<Constraint> case52 = Operator::create(case51, store.zLEQnull, Operator::And);
    ref<Constraint> case5 = Operator::create(case52, store.zGTRnegy, Operator::And);
    // 6. y < -1 /\ x > 0 /\ z >= 0 /\ z < -y
    ref<Constraint> case61 = Operator::create(store.yLSSnegone, store.xGTRnull, Operator::And);
    ref<Constraint> case62 = Operator::create(case61, store.zGEQnull, Operator::And);
    ref<Constraint> case6 = Operator::create(case62, store.zLSSnegy, Operator::And);
    // 7. y < -1 /\ x < 0 /\ z <= 0 /\ z > y
    ref<Constraint> case71 = Operator::create(store.yLSSnegone, store.xLSSnull, Operator::And);
    ref<Constraint> case72 = Operator::create(case71, store.zLEQnull, Operator::And);
    ref<Constraint> case7 = Operator::create(case72, store.zGTRy, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    disj = Operator::create(disj, case4, Operator::Or);
    disj = Operator::create(disj, case5, Operator::Or);
    disj = Operator::create(disj, case6, Operator::Or);
    disj = Operator::create(disj, case7, Operator::Or);
    return disj;
}

ref<Constraint> Converter::getSRemConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;
    ref<Polynomial> negone = Polynomial::negone;
    ref<Polynomial> neglower = lower->constMult(Polynomial::_negone);

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;
    ref<Polynomial> negy = neglower;

    RemConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.yEQnegone = Atom::create(y, negone, Atom::Equ);
    store.yGTRone = Atom::create(y, one, Atom::Gtr);
    store.xGTRnull = Atom::create(x, null, Atom::Gtr);
    store.zGEQnull = Atom::create(z, null, Atom::Geq);
    store.zLSSy = Atom::create(z, y, Atom::Lss);
    store.xLSSnull = Atom::create(x, null, Atom::Lss);
    store.zLEQnull = Atom::create(z, null, Atom::Leq);
    store.zGTRnegy = Atom::create(z, negy, Atom::Gtr);
    store.yLSSnegone = Atom::create(y, negone, Atom::Lss);
    store.zLSSnegy = Atom::create(z, negy, Atom::Lss);
    store.zGTRy = Atom::create(z, y, Atom::Gtr);

    return getSRemConstraint(store);
}

ref<Constraint> Converter::getSRemConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    return getSRemConstraintForUnbounded(upper, lower, res);
}

ref<Constraint> Converter::getSRemConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res, unsigned int bitwidth)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;
    ref<Polynomial> negone = Polynomial::uimax(bitwidth);
    ref<Polynomial> maxpos = Polynomial::simax(bitwidth);
    ref<Polynomial> minneg = Polynomial::simin_as_ui(bitwidth);
    ref<Polynomial> neglower = lower->constMult(Polynomial::_negone);

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;
    ref<Polynomial> negy = neglower;

    RemConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.yEQnegone = Atom::create(y, negone, Atom::Equ);
    ref<Constraint> yGTRone1 = Atom::create(y, one, Atom::Gtr);
    ref<Constraint> yGTRone2 = Atom::create(y, maxpos, Atom::Leq);
    store.yGTRone = Operator::create(yGTRone1, yGTRone2, Operator::And);
    ref<Constraint> xGTRnull1 = Atom::create(x, null, Atom::Gtr);
    ref<Constraint> xGTRnull2 = Atom::create(x, maxpos, Atom::Leq);
    store.xGTRnull = Operator::create(xGTRnull1, xGTRnull2, Operator::And);
    store.zGEQnull = Atom::create(z, maxpos, Atom::Leq);
    store.zLSSy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, y, bitwidth);
    store.xLSSnull = Atom::create(x, minneg, Atom::Geq);
    ref<Constraint> zLEQnull1 = Atom::create(z, minneg, Atom::Geq);
    ref<Constraint> zLEQnull2 = Atom::create(z, null, Atom::Equ);
    store.zLEQnull = Operator::create(zLEQnull1, zLEQnull2, Operator::Or);
    store.zGTRnegy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, negy, bitwidth);
    ref<Constraint> yLSSnegone1 = Atom::create(y, minneg, Atom::Geq);
    ref<Constraint> yLSSnegone2 = Atom::create(y, negone, Atom::Lss);
    store.yLSSnegone = Operator::create(yLSSnegone1, yLSSnegone2, Operator::And);
    store.zLSSnegy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, negy, bitwidth);
    store.zGTRy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, y, bitwidth);

    ref<Constraint> ret = getSRemConstraint(store);
    return ret;
}

void Converter::visitSRem(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        ref<Constraint> remC;
        ref<Polynomial> upper = getPolynomial(I.getOperand(0));
        ref<Polynomial> lower = getPolynomial(I.getOperand(1));
        if (m_divisionConstraintType != None) {
            if (m_boundedIntegers) {
                unsigned int bitwidth = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
                remC = m_unsignedEncoding ? getSRemConstraintForUnsignedBounded(upper, lower, nondef, bitwidth) : getSRemConstraintForSignedBounded(upper, lower, nondef);
            } else {
                remC = getSRemConstraintForUnbounded(upper, lower, nondef);
            }
        } else {
            remC = Constraint::_true;
        }
        visitGenericInstruction(I, nondef, remC);
    }
}

ref<Constraint> Converter::getURemConstraint(RemConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    ref<Constraint> case1 = Operator::create(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = 0
    ref<Constraint> case2 = Operator::create(store.yEQone, store.zEQnull, Operator::And);
    // 3. y > 1 /\ x > 0 /\ z < y
    ref<Constraint> case31 = Operator::create(store.yGTRone, store.xGTRnull, Operator::And);
    ref<Constraint> case3 = Operator::create(case31, store.zLSSy, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    return disj;
}

ref<Constraint> Converter::getURemConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    return getSRemConstraintForUnbounded(upper, lower, res);
}

ref<Constraint> Converter::getURemConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;

    RemConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    ref<Constraint> yGTRone1 = Atom::create(y, one, Atom::Gtr);
    ref<Constraint> yGTRone2 = Atom::create(y, null, Atom::Lss);
    store.yGTRone = Operator::create(yGTRone1, yGTRone2, Operator::Or);
    ref<Constraint> xGTRnull1 = Atom::create(x, null, Atom::Gtr);
    ref<Constraint> xGTRnull2 = Atom::create(x, null, Atom::Lss);
    store.xGTRnull = Operator::create(xGTRnull1, xGTRnull2, Operator::Or);
    store.zLSSy = getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_ULT, z, y);

    return getURemConstraint(store);
}

ref<Constraint> Converter::getURemConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res)
{
    ref<Polynomial> null = Polynomial::null;
    ref<Polynomial> one = Polynomial::one;

    ref<Polynomial> x = upper;
    ref<Polynomial> y = lower;
    ref<Polynomial> z = res;

    RemConstraintStore store;

    store.xEQnull = Atom::create(x, null, Atom::Equ);
    store.zEQnull = Atom::create(z, null, Atom::Equ);
    store.yEQone = Atom::create(y, one, Atom::Equ);
    store.yGTRone = Atom::create(y, one, Atom::Gtr);
    store.xGTRnull = Atom::create(x, null, Atom::Gtr);
    store.zLSSy = Atom::create(z, y, Atom::Lss);

    return getURemConstraint(store);
}

void Converter::visitURem(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        ref<Constraint> remC;
        ref<Polynomial> upper = getPolynomial(I.getOperand(0));
        ref<Polynomial> lower = getPolynomial(I.getOperand(1));
        if (m_divisionConstraintType != None) {
            if (m_boundedIntegers) {
                remC = m_unsignedEncoding ? getURemConstraintForUnsignedBounded(upper, lower, nondef) : getURemConstraintForSignedBounded(upper, lower, nondef);
            } else {
                remC = getURemConstraintForUnbounded(upper, lower, nondef);
            }
        } else {
            remC = Constraint::_true;
        }
        visitGenericInstruction(I, nondef, remC);
    }
}

ref<Constraint> Converter::getAndConstraintForBounded(ref<Polynomial> x, ref<Polynomial> y, ref<Polynomial> res)
{
    ref<Constraint> resLEQx = Atom::create(res, x, Atom::Leq);
    ref<Constraint> resLEQy = Atom::create(res, y, Atom::Leq);
    if (m_unsignedEncoding) {
        return Operator::create(resLEQx, resLEQy, Operator::And);
    }
    ref<Polynomial> null = Polynomial::null;
    ref<Constraint> xGEQnull = Atom::create(x, null, Atom::Geq);
    ref<Constraint> yGEQnull = Atom::create(y, null, Atom::Geq);
    ref<Constraint> resGEQnull = Atom::create(res, null, Atom::Geq);
    ref<Constraint> xLSSnull = Atom::create(x, null, Atom::Lss);
    ref<Constraint> yLSSnull = Atom::create(y, null, Atom::Lss);
    ref<Constraint> resLSSnull = Atom::create(res, null, Atom::Lss);
    // case 1: x >= 0 /\ y >= 0 /\ res >= 0 /\ res <= x /\ res <= y
    ref<Constraint> case1 = Operator::create(xGEQnull, yGEQnull, Operator::And);
    case1 = Operator::create(case1, resGEQnull, Operator::And);
    case1 = Operator::create(case1, resLEQx, Operator::And);
    case1 = Operator::create(case1, resLEQy, Operator::And);
    // case 2: x >= 0 /\ y < 0 /\ res >= 0 /\ res <= x
    ref<Constraint> case2 = Operator::create(xGEQnull, yLSSnull, Operator::And);
    case2 = Operator::create(case2, resGEQnull, Operator::And);
    case2 = Operator::create(case2, resLEQx, Operator::And);
    // case 3: x < 0 /\ y >= 0 /\ res >= 0 /\ res <= y
    ref<Constraint> case3 = Operator::create(xLSSnull, yGEQnull, Operator::And);
    case3 = Operator::create(case3, resGEQnull, Operator::And);
    case3 = Operator::create(case3, resLEQy, Operator::And);
    // case 4: x < 0 /\ y < 0 /\ res < 0 /\ res <= x /\ res <= y
    ref<Constraint> case4 = Operator::create(xLSSnull, yLSSnull, Operator::And);
    case4 = Operator::create(case4, resLSSnull, Operator::And);
    case4 = Operator::create(case4, resLEQx, Operator::And);
    case4 = Operator::create(case4, resLEQy, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    disj = Operator::create(disj, case4, Operator::Or);

    return disj;
}

void Converter::visitAnd(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        if (m_boundedIntegers && m_bitwiseConditions) {
            ref<Polynomial> x = getPolynomial(I.getOperand(0));
            ref<Polynomial> y = getPolynomial(I.getOperand(1));
            ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
            ref<Constraint> c = getAndConstraintForBounded(x, y, nondef);
            visitGenericInstruction(I, nondef, c);
        } else {
            ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
            visitGenericInstruction(I, nondef);
        }
    }
}

ref<Constraint> Converter::getOrConstraintForBounded(ref<Polynomial> x, ref<Polynomial> y, ref<Polynomial> res)
{
    ref<Constraint> resGEQx = Atom::create(res, x, Atom::Leq);
    ref<Constraint> resGEQy = Atom::create(res, y, Atom::Leq);
    if (m_unsignedEncoding) {
        return Operator::create(resGEQx, resGEQy, Operator::And);
    }
    ref<Polynomial> null = Polynomial::null;
    ref<Constraint> xGEQnull = Atom::create(x, null, Atom::Geq);
    ref<Constraint> yGEQnull = Atom::create(y, null, Atom::Geq);
    ref<Constraint> resGEQnull = Atom::create(res, null, Atom::Geq);
    ref<Constraint> xLSSnull = Atom::create(x, null, Atom::Lss);
    ref<Constraint> yLSSnull = Atom::create(y, null, Atom::Lss);
    ref<Constraint> resLSSnull = Atom::create(res, null, Atom::Lss);
    // case 1: x >= 0 /\ y >= 0 /\ res >= 0 /\ res >= x /\ res >= y
    ref<Constraint> case1 = Operator::create(xGEQnull, yGEQnull, Operator::And);
    case1 = Operator::create(case1, resGEQnull, Operator::And);
    case1 = Operator::create(case1, resGEQx, Operator::And);
    case1 = Operator::create(case1, resGEQy, Operator::And);
    // case 2: x >= 0 /\ y < 0 /\ res < 0 /\ res >= y
    ref<Constraint> case2 = Operator::create(xGEQnull, yLSSnull, Operator::And);
    case2 = Operator::create(case2, resLSSnull, Operator::And);
    case2 = Operator::create(case2, resGEQy, Operator::And);
    // case 3: x < 0 /\ y >= 0 /\ res < 0 /\ res >= x
    ref<Constraint> case3 = Operator::create(xLSSnull, yGEQnull, Operator::And);
    case3 = Operator::create(case3, resLSSnull, Operator::And);
    case3 = Operator::create(case3, resGEQx, Operator::And);
    // case 4: x < 0 /\ y < 0 /\ res < 0 /\ res >= x /\ res >= y
    ref<Constraint> case4 = Operator::create(xLSSnull, yLSSnull, Operator::And);
    case4 = Operator::create(case4, resLSSnull, Operator::And);
    case4 = Operator::create(case4, resGEQx, Operator::And);
    case4 = Operator::create(case4, resGEQy, Operator::And);
    // disjunct...
    ref<Constraint> disj = Operator::create(case1, case2, Operator::Or);
    disj = Operator::create(disj, case3, Operator::Or);
    disj = Operator::create(disj, case4, Operator::Or);

    return disj;
}

void Converter::visitOr(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        if (m_boundedIntegers && m_bitwiseConditions) {
            ref<Polynomial> x = getPolynomial(I.getOperand(0));
            ref<Polynomial> y = getPolynomial(I.getOperand(1));
            ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
            ref<Constraint> c = getOrConstraintForBounded(x, y, nondef);
            visitGenericInstruction(I, nondef, c);
        } else {
            ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
            visitGenericInstruction(I, nondef);
        }
    }
}

void Converter::visitXor(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        if (llvm::isa<llvm::ConstantInt>(I.getOperand(1)) && llvm::cast<llvm::ConstantInt>(I.getOperand(1))->isAllOnesValue()) {
            // it is xor %i, -1 --> actually, it is -%i - 1
            ref<Polynomial> p1 = getPolynomial(I.getOperand(0));
            ref<Polynomial> p2 = Polynomial::one;
            visitGenericInstruction(I, p1->constMult(Polynomial::_negone)->sub(p2));
        } else {
            ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
            visitGenericInstruction(I, nondef);
        }
    }
}

void Converter::visitCallInst(llvm::CallInst &I)
{
    if (m_phase1) {
        if (I.getType() != m_boolType && I.getType()->isIntegerTy()) {
            m_vars.push_back(getVar(&I));
        }
    } else {
        llvm::CallSite callSite(&I);
        llvm::Function *calledFunction = callSite.getCalledFunction();
        if (calledFunction != NULL) {
            llvm::StringRef functionName = calledFunction->getName();
            if (functionName == "__kittel_assume") {
                if (m_assumeIsControl) {
                    m_controlPoints.insert(getEval(m_counter));
                }
                ref<Constraint> c = m_onlyLoopConditions ? Constraint::_true : getConditionFromValue(callSite.getArgument(0));
                visitGenericInstruction(I, m_lhs, c->toNNF(false));
                return;
            } else if (functionName.startswith("__kittel_nondef")) {
                ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
                visitGenericInstruction(I, nondef);
                return;
            }
        }
        if (m_complexityTuples) {
            m_controlPoints.insert(getEval(m_counter));
            m_controlPoints.insert(getEval(m_counter + 1));
            m_complexityLHSs.insert(getEval(m_counter));
        }
        // "random" functions
        if (I.getType()->isIntegerTy() || I.getType()->isVoidTy() || I.getType()->isFloatingPointTy() || I.getType()->isPointerTy() || I.getType()->isVectorTy() || I.getType()->isStructTy() || I.getType()->isArrayTy()) {
            std::list<llvm::Function*> callees;
            if (calledFunction != NULL) {
                callees.push_back(calledFunction);
            } else {
                callees = getMatchingFunctions(I);
            }
            std::set<llvm::GlobalVariable*> toZap;
            m_idMap.insert(std::make_pair(&I, m_counter));
            ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
            for (std::list<llvm::Function*>::iterator cf = callees.begin(), cfe = callees.end(); cf != cfe; ++cf) {
                llvm::Function *callee = *cf;
                if (m_scc.find(callee) != m_scc.end() || m_complexityTuples) {
                    std::list<ref<Polynomial> > callArgs;
                    for (llvm::CallSite::arg_iterator i = callSite.arg_begin(), e = callSite.arg_end(); i != e; ++i) {
                        llvm::Value *arg = *i;
                        if (arg->getType() != m_boolType && arg->getType()->isIntegerTy()) {
                            callArgs.push_back(getPolynomial(arg));
                        }
                    }
                    for (std::list<llvm::GlobalVariable*>::iterator i = m_globals.begin(), e = m_globals.end(); i != e; ++i) {
                        callArgs.push_back(getPolynomial(*i));
                    }
                    m_controlPoints.insert(getEval(callee, "start"));
                    ref<Term> rhs2 = Term::create(getEval(callee, "start"), callArgs);
                    ref<Rule> rule2 = Rule::create(lhs, rhs2, Constraint::_true);
                    m_blockRules.push_back(rule2);
                }
                if (callee->isDeclaration()) {
                    // they don't mess with globals
                    continue;
                }
                std::map<llvm::Function*, std::set<llvm::GlobalVariable*> >::iterator it = m_funcMayZap.find(callee);
                if (it == m_funcMayZap.end()) {
                    std::cerr << "Could not find alias information (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
                    exit(767);
                }
                std::set<llvm::GlobalVariable*> zapped = it->second;
                toZap.insert(zapped.begin(), zapped.end());
            }
            m_counter++;
            // zap!
            std::list<ref<Polynomial> > newArgs;
            if (I.getType()->isIntegerTy() && I.getType() != m_boolType) {
                ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
                newArgs = getZappedArgs(toZap, I, nondef);
            } else {
                newArgs = getZappedArgs(toZap);
            }
            ref<Term> rhs1 = Term::create(getEval(m_counter), newArgs);
            ref<Rule> rule1 = Rule::create(lhs, rhs1, Constraint::_true);
            m_blockRules.push_back(rule1);
            return;
        } else {
            std::cerr << "Unsupported function return type encountered!" << std::endl;
            exit(1);
        }
    }
}

std::list<llvm::Function*> Converter::getMatchingFunctions(llvm::CallInst &I)
{
    std::list<llvm::Function*> res;
    const llvm::Type *type = I.getCalledValue()->getType();
    llvm::Module *module = I.getParent()->getParent()->getParent();
    for (llvm::Module::iterator i = module->begin(), e = module->end(); i != e; ++i) {
        if (!i->isDeclaration()) {
            if (i->getType() == type) {
                res.push_back(i);
            }
        }
    }
    return res;
}

std::string Converter::getNondef(llvm::Value *V)
{
    std::ostringstream tmp;
    tmp << "nondef." << m_nondef;
    m_nondef++;
    std::string res = tmp.str();
    if (m_boundedIntegers && V != NULL && V->getType()->isIntegerTy()) {
        m_bitwidthMap.insert(std::make_pair(res, llvm::cast<llvm::IntegerType>(V->getType())->getBitWidth()));
    }
    return res;
}

void Converter::visitSelectInst(llvm::SelectInst &I)
{
    if (I.getType() == m_boolType || !I.getType()->isIntegerTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        if (m_selectIsControl) {
            m_controlPoints.insert(getEval(m_counter));
        }
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        m_counter++;
        ref<Term> rhs1 = Term::create(getEval(m_counter), getNewArgs(I, getPolynomial(I.getTrueValue())));
        ref<Term> rhs2 = Term::create(getEval(m_counter), getNewArgs(I, getPolynomial(I.getFalseValue())));
        ref<Constraint> c = m_onlyLoopConditions ? Constraint::_true : getConditionFromValue(I.getCondition());
        ref<Rule> rule1 = Rule::create(lhs, rhs1, c->toNNF(false));
        m_blockRules.push_back(rule1);
        ref<Rule> rule2 = Rule::create(lhs, rhs2, c->toNNF(true));
        m_blockRules.push_back(rule2);
    }
}

void Converter::visitPHINode(llvm::PHINode &I)
{
    if (I.getType() == m_boolType || !I.getType()->isIntegerTy()) {
        return;
    }
    if (m_phase1) {
        std::string phiVar = getVar(&I);
        m_vars.push_back(phiVar);
        m_phiVars.insert(phiVar);
    } else {
    }
}

void Converter::visitGetElementPtrInst(llvm::GetElementPtrInst &)
{}

void Converter::visitIntToPtrInst(llvm::IntToPtrInst &)
{}

void Converter::visitBitCastInst(llvm::BitCastInst &I)
{
    if (!I.getType()->isIntegerTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> value = getPolynomial(&I);
        visitGenericInstruction(I, value);
    }
}

void Converter::visitPtrToIntInst(llvm::PtrToIntInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitLoadInst(llvm::LoadInst &I)
{
    if (!I.getType()->isIntegerTy() || I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        MayMustMap::iterator it = m_mmMap.find(&I);
        if (it == m_mmMap.end()) {
            std::cerr << "Could not find alias information (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(321);
        }
        MayMustPair mmp = it->second;
        std::set<llvm::GlobalVariable*> mays = mmp.first;
        std::set<llvm::GlobalVariable*> musts = mmp.second;
        ref<Polynomial> newArg;
        if (musts.size() == 1 && mays.size() == 0) {
            // unique!
            newArg = Polynomial::create(getVar(*musts.begin()));
        } else {
            // nondef...
            newArg = Polynomial::create(getNondef(&I));
        }
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        ++m_counter;
        ref<Term> rhs = Term::create(getEval(m_counter), getNewArgs(I, newArg));
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_blockRules.push_back(rule);
    }
}

void Converter::visitStoreInst(llvm::StoreInst &I)
{
    if (m_phase1) {
    } else {
        llvm::Value *val = I.getOperand(0);
        MayMustMap::iterator it = m_mmMap.find(&I);
        if (it == m_mmMap.end()) {
            std::cerr << "Could not find alias information (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(321);
        }
        MayMustPair mmp = it->second;
        std::set<llvm::GlobalVariable*> mays = mmp.first;
        std::set<llvm::GlobalVariable*> musts = mmp.second;
        std::list<ref<Polynomial> > newArgs;
        if (musts.size() == 1 && mays.size() == 0) {
            // unique!
            newArgs = getNewArgs(**musts.begin(), getPolynomial(val));
        } else if (musts.size() != 0) {
            std::cerr << "Strange number of must aliases!" << std::endl;
            exit(1212);
        } else {
            // zap all that may
            newArgs = getZappedArgs(mays);
        }
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        ++m_counter;
        ref<Term> rhs = Term::create(getEval(m_counter), newArgs);
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_blockRules.push_back(rule);
    }
}

void Converter::visitAllocaInst(llvm::AllocaInst &)
{}

void Converter::visitFPToSIInst(llvm::FPToSIInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitFPToUIInst(llvm::FPToUIInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitInstruction(llvm::Instruction &I)
{
    if (I.getType() == m_boolType || !I.getType()->isIntegerTy()) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        ref<Polynomial> nondef = Polynomial::create(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitSExtInst(llvm::SExtInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        ref<Polynomial> copy = getPolynomial(I.getOperand(0));
        ++m_counter;
        if (m_boundedIntegers && m_unsignedEncoding) {
            unsigned int bitwidthNew = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
            unsigned int bitwidthOld = llvm::cast<llvm::IntegerType>(I.getOperand(0)->getType())->getBitWidth();
            ref<Polynomial> sizeNew = Polynomial::power_of_two(bitwidthNew);
            ref<Polynomial> sizeOld = Polynomial::power_of_two(bitwidthOld);
            ref<Polynomial> sizeDiff = sizeNew->sub(sizeOld);
            ref<Polynomial> intmaxOld = Polynomial::simax(bitwidthOld);
            ref<Polynomial> converted = sizeDiff->add(copy);
            ref<Term> rhs1 = Term::create(getEval(m_counter), getNewArgs(I, copy));
            ref<Term> rhs2 = Term::create(getEval(m_counter), getNewArgs(I, converted));
            ref<Constraint> c1 = Atom::create(copy, intmaxOld, Atom::Leq);
            ref<Constraint> c2 = Atom::create(copy, intmaxOld, Atom::Gtr);
            ref<Rule> rule1 = Rule::create(lhs, rhs1, c1);
            ref<Rule> rule2 = Rule::create(lhs, rhs2, c2);
            m_blockRules.push_back(rule1);
            m_blockRules.push_back(rule2);
        } else {
            // mathematical integers or bounded integers with signed encoding
            ref<Term> rhs = Term::create(getEval(m_counter), getNewArgs(I, copy));
            ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
            m_blockRules.push_back(rule);
        }
    }
}

void Converter::visitZExtInst(llvm::ZExtInst &I)
{
    if (isAssumeArg(I)) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        ++m_counter;
        if (I.getOperand(0)->getType() == m_boolType) {
            ref<Polynomial> zero = Polynomial::null;
            ref<Polynomial> one = Polynomial::one;
            ref<Term> rhszero = Term::create(getEval(m_counter), getNewArgs(I, zero));
            ref<Term> rhsone = Term::create(getEval(m_counter), getNewArgs(I, one));
            ref<Constraint> c = getConditionFromValue(I.getOperand(0));
            ref<Rule> rulezero = Rule::create(lhs, rhszero, c->toNNF(true));
            ref<Rule> ruleone = Rule::create(lhs, rhsone, c->toNNF(false));
            m_blockRules.push_back(rulezero);
            m_blockRules.push_back(ruleone);
        } else {
            ref<Polynomial> copy = getPolynomial(I.getOperand(0));
            if (m_boundedIntegers && !m_unsignedEncoding) {
                unsigned int bitwidthOld = llvm::cast<llvm::IntegerType>(I.getOperand(0)->getType())->getBitWidth();
                ref<Polynomial> shifter = Polynomial::power_of_two(bitwidthOld);
                ref<Polynomial> converted = shifter->add(copy);
                ref<Term> rhs1 = Term::create(getEval(m_counter), getNewArgs(I, copy));
                ref<Term> rhs2 = Term::create(getEval(m_counter), getNewArgs(I, converted));
                ref<Polynomial> zero = Polynomial::null;
                ref<Constraint> c1 = Atom::create(copy, zero, Atom::Geq);
                ref<Constraint> c2 = Atom::create(copy, zero, Atom::Lss);
                ref<Rule> rule1 = Rule::create(lhs, rhs1, c1);
                ref<Rule> rule2 = Rule::create(lhs, rhs2, c2);
                m_blockRules.push_back(rule1);
                m_blockRules.push_back(rule2);
            } else {
                // mathematical integers of bounded integers with unsigned encoding
                ref<Term> rhs = Term::create(getEval(m_counter), getNewArgs(I, copy));
                ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
                m_blockRules.push_back(rule);
            }
        }
    }
}

void Converter::visitTruncInst(llvm::TruncInst &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        m_idMap.insert(std::make_pair(&I, m_counter));
        ref<Term> lhs = Term::create(getEval(m_counter), m_lhs);
        ref<Polynomial> val;
        if (m_boundedIntegers) {
            val = Polynomial::create(getNondef(&I));
        } else {
            val = getPolynomial(I.getOperand(0));
        }
        m_counter++;
        ref<Term> rhs = Term::create(getEval(m_counter), getNewArgs(I, val));
        ref<Rule> rule = Rule::create(lhs, rhs, Constraint::_true);
        m_blockRules.push_back(rule);
    }
}

bool Converter::isAssumeArg(llvm::ZExtInst &I)
{
    if (I.getOperand(0)->getType() == m_boolType) {
        if (I.getNumUses() != 1) {
            return false;
        }
#if LLVM_VERSION < VERSION(3, 5)
        llvm::User *user = *I.use_begin();
#else
        llvm::User *user = *I.user_begin();
#endif
        if (!llvm::isa<llvm::CallInst>(user)) {
            return false;
        }
        llvm::CallSite callSite(llvm::cast<llvm::CallInst>(user));
        llvm::Function *calledFunction = callSite.getCalledFunction();
        if (calledFunction == NULL) {
            return false;
        }
        llvm::StringRef functionName = calledFunction->getName();
        if (functionName != "__kittel_assume") {
            return false;
        }
        return true;
    }
    return false;
}

std::set<std::string> Converter::getPhiVariables()
{
    return m_phiVars;
}

std::map<std::string, unsigned int> Converter::getBitwidthMap()
{
    return m_bitwidthMap;
}

std::set<std::string> Converter::getComplexityLHSs()
{
    return m_complexityLHSs;
}
