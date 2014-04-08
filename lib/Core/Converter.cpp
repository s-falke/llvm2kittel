// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Util/Version.h"

#include "llvm2kittel/Converter.h"
#include "llvm2kittel/RewriteSystem/Polynomial.h"
#include "llvm2kittel/RewriteSystem/Term.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Constants.h>
#else
  #include <llvm/IR/Constants.h>
#endif
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/CallSite.h>
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

Converter::Converter(const llvm::Type *boolType, bool assumeIsControl, bool selectIsControl, bool onlyMultiPredIsControl, bool boundedIntegers, bool unsignedEncoding, bool onlyLoopConditions, bool exactDivision, bool bitwiseConditions, bool complexityTuples)
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
    m_exactDivision(exactDivision),
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
        m_lhs.push_back(new Polynomial(*i));
    }
}

void Converter::phase2(llvm::Function *function, std::set<llvm::Function*> &scc, MayMustMap &mmMap, std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > &funcMayZap, TrueFalseMap &tfMap, std::set<llvm::BasicBlock*> &lcbs, ConditionMap &elcMap)
{
    if (m_trivial) {
        m_rules.push_back(new Rule(new Term(getEval(m_function, "start"), m_lhs), new Term(getEval(m_function, "stop"), m_lhs), Constraint::_true));
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
        Term *lhs = new Term(getEval(*i, "out"), m_lhs);
        Term *rhs = new Term(getEval(m_function, "stop"), m_lhs);
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    }
}

std::list<Rule*> Converter::getRules()
{
    return m_rules;
}

std::list<Rule*> Converter::getCondensedRules()
{
    std::list<Rule*> good;
    std::list<Rule*> junk;
    std::list<Rule*> res;
    for (std::list<Rule*>::iterator i = m_rules.begin(), e = m_rules.end(); i != e; ++i) {
        Rule *rule = *i;
        std::string f = rule->getLeft()->getFunctionSymbol();
        if (m_controlPoints.find(f) != m_controlPoints.end()) {
            good.push_back(rule);
        } else {
            junk.push_back(rule);
        }
    }
    for (std::list<Rule*>::iterator i = good.begin(), e = good.end(); i != e; ++i) {
        Rule *rule = *i;
        std::vector<Rule*> todo;
        todo.push_back(rule);
        while (!todo.empty()) {
            Rule *r = *todo.begin();
            todo.erase(todo.begin());
            Term *rhs = r->getRight();
            std::string f = rhs->getFunctionSymbol();
            if (m_controlPoints.find(f) != m_controlPoints.end()) {
                res.push_back(r);
            } else {
                std::list<Rule*> newtodo;
                for (std::list<Rule*>::iterator ii = junk.begin(), ee = junk.end(); ii != ee; ++ii) {
                    Rule *junkrule = *ii;
                    if (junkrule->getLeft()->getFunctionSymbol() == f) {
                        std::map<std::string, Polynomial*> subby;
                        std::list<Polynomial*> rhsargs = rhs->getArgs();
                        std::list<Polynomial*>::iterator ai = rhsargs.begin();
                        for (std::list<std::string>::iterator vi = m_vars.begin(), ve = m_vars.end(); vi != ve; ++vi, ++ai) {
                            subby.insert(std::make_pair(*vi, *ai));
                        }
                        Rule *newRule = new Rule(r->getLeft(), junkrule->getRight()->instantiate(&subby), new Operator(r->getConstraint(), junkrule->getConstraint()->instantiate(&subby), Operator::And));
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

Polynomial *Converter::getPolynomial(llvm::Value *V)
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
        Polynomial *res = new Polynomial(value);
        mpz_clear(value);
        return res;
    } else if (llvm::isa<llvm::Instruction>(V) || llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::GlobalVariable>(V)) {
        return new Polynomial(getVar(V));
    } else {
        return new Polynomial(getNondef(V));
    }
}

std::list<Polynomial*> Converter::getNewArgs(llvm::Value &V, Polynomial *p)
{
    std::list<Polynomial*> res;
    std::string Vname = getVar(&V);
    std::list<Polynomial*>::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        if (Vname == *i) {
            res.push_back(p);
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

std::list<Polynomial*> Converter::getZappedArgs(std::set<llvm::GlobalVariable*> toZap)
{
    std::list<Polynomial*> res;
    std::list<Polynomial*>::iterator pp = m_lhs.begin();
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
            res.push_back(new Polynomial(nondef));
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

std::list<Polynomial*> Converter::getZappedArgs(std::set<llvm::GlobalVariable*> toZap, llvm::Value &V, Polynomial *p)
{
    std::list<Polynomial*> res;
    std::string Vname = getVar(&V);
    std::list<Polynomial*>::iterator pp = m_lhs.begin();
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
            res.push_back(new Polynomial(nondef));
        } else if (Vname == *i) {
            res.push_back(p);
        } else {
            res.push_back(*pp);
        }
    }
    return res;
}

Constraint *Converter::getConditionFromValue(llvm::Value *cond)
{
    if (llvm::isa<llvm::ConstantInt>(cond)) {
        return new Atom(getPolynomial(cond), Polynomial::null, Atom::Neq);
    }
    if (!llvm::isa<llvm::Instruction>(cond)) {
        cond->dump();
        std::cerr << "Cannot handle non-instructions!" << std::endl;
        exit(5);
    }
    llvm::Instruction *I = llvm::cast<llvm::Instruction>(cond);
    return getConditionFromInstruction(I);
}

Constraint *Converter::getConditionFromInstruction(llvm::Instruction *I)
{
    if (llvm::isa<llvm::ZExtInst>(I)) {
        return getConditionFromValue(I->getOperand(0));
    }
    if (I->getType() != m_boolType) {
        return new Atom(getPolynomial(I), Polynomial::null, Atom::Neq);
    }
    if (llvm::isa<llvm::CmpInst>(I)) {
        llvm::CmpInst *cmp = llvm::cast<llvm::CmpInst>(I);
        if (cmp->getOperand(0)->getType()->isPointerTy()) {
            return new Nondef();
        } else if (cmp->getOperand(0)->getType()->isFloatingPointTy()) {
            return new Nondef();
        } else {
            llvm::CmpInst::Predicate pred = cmp->getPredicate();
            if (m_boundedIntegers && !m_unsignedEncoding && (pred == llvm::CmpInst::ICMP_UGT || pred == llvm::CmpInst::ICMP_UGE || pred == llvm::CmpInst::ICMP_ULT || pred == llvm::CmpInst::ICMP_ULE)) {
                return getUnsignedComparisonForSignedBounded(pred, getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)));
            } else if (m_boundedIntegers && m_unsignedEncoding && (pred == llvm::CmpInst::ICMP_SGT || pred == llvm::CmpInst::ICMP_SGE || pred == llvm::CmpInst::ICMP_SLT || pred == llvm::CmpInst::ICMP_SLE)) {
                unsigned int bitwidth = llvm::cast<llvm::IntegerType>(cmp->getOperand(0)->getType())->getBitWidth();
                return getSignedComparisonForUnsignedBounded(pred, getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)), bitwidth);
            } else {
                return new Atom(getPolynomial(cmp->getOperand(0)), getPolynomial(cmp->getOperand(1)), getAtomType(pred));
            }
        }
    }
    std::string opcodeName = I->getOpcodeName();
    if (opcodeName == "and") {
        return new Operator(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(1)), Operator::And);
    } else if (opcodeName == "or") {
        return new Operator(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(1)), Operator::Or);
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
            return new Negation(getConditionFromValue(I->getOperand(realIdx)));
        } else {
            return getConditionFromValue(I->getOperand(realIdx));
        }
    } else if (opcodeName == "select") {
        llvm::ConstantInt *ci;
        unsigned int realIdx;
        if (llvm::isa<llvm::ConstantInt>(I->getOperand(1))) {
            ci = llvm::cast<llvm::ConstantInt>(I->getOperand(1));
            realIdx = 2;
        } else {
            ci = llvm::cast<llvm::ConstantInt>(I->getOperand(2));
            realIdx = 1;
        }
        Operator::OType otype;
        if (ci->isOne()) {
            // true --> or
            otype = Operator::Or;
        } else {
            otype = Operator::And;
        }
        return new Operator(getConditionFromValue(I->getOperand(0)), getConditionFromValue(I->getOperand(realIdx)), otype);
    } else {
        return new Nondef();
    }
}

Constraint *Converter::getUnsignedComparisonForSignedBounded(llvm::CmpInst::Predicate pred, Polynomial *x, Polynomial *y)
{
    switch (pred) {
    case llvm::CmpInst::ICMP_UGT:
    case llvm::CmpInst::ICMP_UGE: {
        Atom *xge = new Atom(x, Polynomial::null, Atom::Geq);
        Atom *yge = new Atom(y, Polynomial::null, Atom::Geq);
        Atom *xlt = new Atom(x, Polynomial::null, Atom::Lss);
        Atom *ylt = new Atom(y, Polynomial::null, Atom::Lss);
        Constraint *gege = new Operator(xge, yge, Operator::And);
        Constraint *ltlt = new Operator(xlt, ylt, Operator::And);
        Constraint *ltge = new Operator(xlt, yge, Operator::And);
        Constraint *disj1 = new Operator(gege, new Atom(x, y, getAtomType(pred)), Operator::And);
        Constraint *disj2 = new Operator(ltlt, new Atom(x, y, getAtomType(pred)), Operator::And);
        Constraint *disj3 = ltge;
        Constraint *tmp = new Operator(disj1, disj2, Operator::Or);
        return new Operator(tmp, disj3, Operator::Or);
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

Constraint *Converter::getSignedComparisonForUnsignedBounded(llvm::CmpInst::Predicate pred, Polynomial *x, Polynomial *y, unsigned int bitwidth)
{
    Polynomial *maxpos = Polynomial::simax(bitwidth);
    switch (pred) {
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::ICMP_SGE: {
        Atom *xle = new Atom(x, maxpos, Atom::Leq);
        Atom *yle = new Atom(y, maxpos, Atom::Leq);
        Atom *xgt = new Atom(x, maxpos, Atom::Gtr);
        Atom *ygt = new Atom(y, maxpos, Atom::Gtr);
        Constraint *lele = new Operator(xle, yle, Operator::And);
        Constraint *gtgt = new Operator(xgt, ygt, Operator::And);
        Constraint *legt = new Operator(xle, ygt, Operator::And);
        Constraint *disj1 = new Operator(lele, new Atom(x, y, getAtomType(pred)), Operator::And);
        Constraint *disj2 = new Operator(gtgt, new Atom(x, y, getAtomType(pred)), Operator::And);
        Constraint *disj3 = legt;
        Constraint *tmp = new Operator(disj1, disj2, Operator::Or);
        return new Operator(tmp, disj3, Operator::Or);
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

Constraint *Converter::buildConjunction(std::set<llvm::Value*> &trues, std::set<llvm::Value*> &falses)
{
    Constraint *res = Constraint::_true;
    for (std::set<llvm::Value*>::iterator i = trues.begin(), e = trues.end(); i != e; ++i) {
        Constraint *c = getConditionFromValue(*i)->toNNF(false);
        res = new Operator(res, c, Operator::And);
    }
    for (std::set<llvm::Value*>::iterator i = falses.begin(), e = falses.end(); i != e; ++i) {
        Constraint *c = getConditionFromValue(*i)->toNNF(true);
        res = new Operator(res, c, Operator::And);
    }
    return res;
}

Constraint *Converter::buildBoundConjunction(std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > &bounds)
{
    Constraint *res = Constraint::_true;
    for (std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> >::iterator i = bounds.begin(), e = bounds.end(); i != e; ++i) {
        Polynomial *p = getPolynomial(i->first);
        Polynomial *q1 = getPolynomial(i->third);
        Polynomial *q2 = getPolynomial(i->fourth);
        Constraint *c = new Atom(p, q1->add(q2), getAtomType(i->second));
        res = new Operator(res, c, Operator::And);
    }
    return res;
}

void Converter::visitBB(llvm::BasicBlock *bb)
{
    // start
    if (bb == m_entryBlock) {
        Term *lhs = new Term(getEval(m_function, "start"), m_lhs);
        Term *rhs = new Term(getEval(bb, "in"), m_lhs);
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
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
    Constraint *cond = NULL;
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
        Constraint *c_new = buildBoundConjunction(bounds);
        cond = new Operator(cond, c_new, Operator::And);
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
        Term *lhs = new Term(getEval(bb, "in"), m_lhs);
        Term *rhs = new Term(getEval(firstID), m_lhs);
        Rule *rule = new Rule(lhs, rhs, cond);
        m_rules.push_back(rule);
    } else {
        Term *lhs = new Term(getEval(bb, "in"), m_lhs);
        Term *rhs = new Term(getEval(bb, "out"), m_lhs);
        Rule *rule = new Rule(lhs, rhs, cond);
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
        Term *lhs = new Term(getEval(lastID+1), m_lhs);
        m_counter++;
        Term *rhs = new Term(getEval(bb, "out"), m_lhs);
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    }

    // jump from bb_out to succs
    llvm::TerminatorInst *terminator = bb->getTerminator();
    if (llvm::isa<llvm::ReturnInst>(terminator)) {
    } else if (llvm::isa<llvm::UnreachableInst>(terminator)) {
        Term *lhs = new Term(getEval(bb, "out"), m_lhs);
        Term *rhs = new Term(getEval(m_function, "stop"), m_lhs);
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
        m_rules.push_back(rule);
    } else {
        llvm::BranchInst *branch = llvm::cast<llvm::BranchInst>(terminator);
        bool useCondition = (!m_onlyLoopConditions || m_loopConditionBlocks.find(bb) != m_loopConditionBlocks.end());
        if (branch->isUnconditional()) {
            Term *lhs = new Term(getEval(bb, "out"), m_lhs);
            Term *rhs = new Term(getEval(branch->getSuccessor(0), "in"), getArgsWithPhis(bb, branch->getSuccessor(0)));
            Rule *rule = new Rule(lhs, rhs, Constraint::_true);
            m_rules.push_back(rule);
        } else {
            Term *lhs = new Term(getEval(bb, "out"), m_lhs);
            Term *rhs1 = new Term(getEval(branch->getSuccessor(0), "in"), getArgsWithPhis(bb, branch->getSuccessor(0)));
            Term *rhs2 = new Term(getEval(branch->getSuccessor(1), "in"), getArgsWithPhis(bb, branch->getSuccessor(1)));
            Constraint *c = getConditionFromValue(branch->getCondition());
            Rule *rule1 = new Rule(lhs, rhs1, useCondition ? c->toNNF(false) : Constraint::_true);
            m_rules.push_back(rule1);
            Rule *rule2 = new Rule(lhs, rhs2, useCondition ? c->toNNF(true) : Constraint::_true);
            m_rules.push_back(rule2);
        }
    }
}

std::list<Polynomial*> Converter::getArgsWithPhis(llvm::BasicBlock *from, llvm::BasicBlock *to)
{
    std::list<Polynomial*> res;
    std::map<std::string, Polynomial*> phiValues;
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
    std::list<Polynomial*>::iterator pp = m_lhs.begin();
    for (std::list<std::string>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i, ++pp) {
        std::map<std::string, Polynomial*>::iterator found = phiValues.find(*i);
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

void Converter::visitGenericInstruction(llvm::Instruction &I, std::list<Polynomial*> newArgs, Constraint *c)
{
    m_idMap.insert(std::make_pair(&I, m_counter));
    Term *lhs = new Term(getEval(m_counter), m_lhs);
    ++m_counter;
    Term *rhs = new Term(getEval(m_counter), newArgs);
    Rule *rule = new Rule(lhs, rhs, c);
    m_blockRules.push_back(rule);
}

void Converter::visitGenericInstruction(llvm::Instruction &I, Polynomial *value, Constraint *c)
{
    visitGenericInstruction(I, getNewArgs(I, value), c);
}

void Converter::visitAdd(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *p1 = getPolynomial(I.getOperand(0));
        Polynomial *p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->add(p2));
    }
}

void Converter::visitSub(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *p1 = getPolynomial(I.getOperand(0));
        Polynomial *p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->sub(p2));
    }
}

void Converter::visitMul(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *p1 = getPolynomial(I.getOperand(0));
        Polynomial *p2 = getPolynomial(I.getOperand(1));
        visitGenericInstruction(I, p1->mult(p2));
    }
}

Constraint *Converter::getSDivConstraint(DivConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    Constraint *case1 = new Operator(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = x
    Constraint *case2 = new Operator(store.yEQone, store.zEQx, Operator::And);
    // 3. y = -1 /\ z = -x
    Constraint *case3 = new Operator(store.yEQnegone, store.zEQnegx, Operator::And);
    // 4. y > 1 /\ x > 0 /\ z >= 0 /\ z < x
    Constraint *case41 = new Operator(store.yGTRone, store.xGTRnull, Operator::And);
    Constraint *case42 = new Operator(case41, store.zGEQnull, Operator::And);
    Constraint *case4 = new Operator(case42, store.zLSSx, Operator::And);
    // 5. y > 1 /\ x < 0 /\ z <= 0 /\ z > x
    Constraint *case51 = new Operator(store.yGTRone, store.xLSSnull, Operator::And);
    Constraint *case52 = new Operator(case51, store.zLEQnull, Operator::And);
    Constraint *case5 = new Operator(case52, store.zGTRx, Operator::And);
    // 6. y < -1 /\ x > 0 /\ z <= 0 /\ z > -x
    Constraint *case61 = new Operator(store.yLSSnegone, store.xGTRnull, Operator::And);
    Constraint *case62 = new Operator(case61, store.zLEQnull, Operator::And);
    Constraint *case6 = new Operator(case62, store.zGTRnegx, Operator::And);
    // 7. y < -1 /\ x < 0 /\ z >= 0 /\ z < -x
    Constraint *case71 = new Operator(store.yLSSnegone, store.xLSSnull, Operator::And);
    Constraint *case72 = new Operator(case71, store.zGEQnull, Operator::And);
    Constraint *case7 = new Operator(case72, store.zLSSnegx, Operator::And);
    // disjunct...
    Constraint *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    disj = new Operator(disj, case4, Operator::Or);
    disj = new Operator(disj, case5, Operator::Or);
    disj = new Operator(disj, case6, Operator::Or);
    disj = new Operator(disj, case7, Operator::Or);
    return disj;
}

Constraint *Converter::getSDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;
    Polynomial *negone = Polynomial::negone;
    Polynomial *negupper = upper->constMult(Polynomial::_negone);

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;
    Polynomial *negx = negupper;

    DivConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.yEQnegone = new Atom(y, negone, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.zEQx = new Atom(z, x, Atom::Equ);
    store.zEQnegx = new Atom(z, negx, Atom::Equ);
    store.yGTRone = new Atom(y, one, Atom::Gtr);
    store.xGTRnull = new Atom(x, null, Atom::Gtr);
    store.zGEQnull = new Atom(z, null, Atom::Geq);
    store.zLSSx = new Atom(z, x, Atom::Lss);
    store.xLSSnull = new Atom(x, null, Atom::Lss);
    store.zLEQnull = new Atom(z, null, Atom::Leq);
    store.zGTRx = new Atom(z, x, Atom::Gtr);
    store.yLSSnegone = new Atom(y, negone, Atom::Lss);
    store.zGTRnegx = new Atom(z, negx, Atom::Gtr);
    store.zLSSnegx = new Atom(z, negx, Atom::Lss);

    return getSDivConstraint(store);
}

Constraint *Converter::getSDivConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    return getSDivConstraintForUnbounded(upper, lower, res);
}

Constraint *Converter::getSDivConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res, unsigned int bitwidth)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;
    Polynomial *negone = Polynomial::uimax(bitwidth);
    Polynomial *maxpos = Polynomial::simax(bitwidth);
    Polynomial *minneg = Polynomial::simin_as_ui(bitwidth);
    Polynomial *negupper = upper->constMult(Polynomial::_negone);

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;
    Polynomial *negx = negupper;

    DivConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.yEQnegone = new Atom(y, negone, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.zEQx = new Atom(z, x, Atom::Equ);
    store.zEQnegx = new Atom(z, negx, Atom::Equ);
    Atom *yGTRone1 = new Atom(y, one, Atom::Gtr);
    Atom *yGTRone2 = new Atom(y, maxpos, Atom::Leq);
    store.yGTRone = new Operator(yGTRone1, yGTRone2, Operator::And);
    Atom *xGTRnull1 = new Atom(x, null, Atom::Gtr);
    Atom *xGTRnull2 = new Atom(x, maxpos, Atom::Leq);
    store.xGTRnull = new Operator(xGTRnull1, xGTRnull2, Operator::And);
    store.zGEQnull = new Atom(z, maxpos, Atom::Leq);
    store.zLSSx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, x, bitwidth);
    store.xLSSnull = new Atom(x, minneg, Atom::Geq);
    Atom *zLEQnull1 = new Atom(z, minneg, Atom::Geq);
    Atom *zLEQnull2 = new Atom(z, null, Atom::Equ);
    store.zLEQnull = new Operator(zLEQnull1, zLEQnull2, Operator::Or);
    store.zGTRx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, x, bitwidth);
    Atom *yLSSnegone1 = new Atom(y, minneg, Atom::Geq);
    Atom *yLSSnegone2 = new Atom(y, negone, Atom::Lss);
    store.yLSSnegone = new Operator(yLSSnegone1, yLSSnegone2, Operator::And);
    store.zGTRnegx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, negx, bitwidth);
    store.zLSSnegx = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, negx, bitwidth);

    return getSDivConstraint(store);
}

Constraint *Converter::getExactSDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    // x/y = z
    Polynomial *px = upper;
    Polynomial *py = lower;
    Polynomial *pz = res;
    Polynomial *pnegx = px->constMult(Polynomial::_negone);
    Polynomial *pnegy = py->constMult(Polynomial::_negone);
    Polynomial *pnull = Polynomial::null;
    // 1. x = 0 /\ z = 0
    Atom *pxnull = new Atom(px, pnull, Atom::Equ);
    Atom *pznull = new Atom(pz, pnull, Atom::Equ);
    Constraint *case1 = new Operator(pxnull, pznull, Operator::And);
    // 2. y > 0 /\ x > 0 /\ z >= 0 /\ x - y*z >= 0 /\ x - y*z < y
    Atom *pygtrnull = new Atom(py, pnull, Atom::Gtr);
    Atom *pxgtrnull = new Atom(px, pnull, Atom::Gtr);
    Atom *pzgeqnull = new Atom(pz, pnull, Atom::Geq);
    Polynomial *term2 = px->sub(py->mult(pz));
    Atom *term2geqnull = new Atom(term2, pnull, Atom::Geq);
    Atom *term2lssy = new Atom(term2, py, Atom::Lss);
    Constraint *case21 = new Operator(pygtrnull, pxgtrnull, Operator::And);
    Constraint *case22 = new Operator(case21, pzgeqnull, Operator::And);
    Constraint *case23 = new Operator(case22, term2geqnull, Operator::And);
    Constraint *case2 = new Operator(case23, term2lssy, Operator::And);
    // 3. y < 0 /\ x > 0 /\ z <= 0 /\ x - y*z >= 0 /\ x - y*z < -y
    Atom *pylssnull = new Atom(py, pnull, Atom::Lss);
    Atom *pzleqnull = new Atom(pz, pnull, Atom::Leq);
    Atom *term2lssnegy = new Atom(term2, pnegy, Atom::Lss);
    Constraint *case31 = new Operator(pylssnull, pxgtrnull, Operator::And);
    Constraint *case32 = new Operator(case31, pzleqnull, Operator::And);
    Constraint *case33 = new Operator(case32, term2geqnull, Operator::And);
    Constraint *case3 = new Operator(case33, term2lssnegy, Operator::And);
    // 4. y > 0 /\ x < 0 /\ z <= 0 /\ -x + y*z >= 0 /\ -x + y*z < y
    Atom *pxlssnull = new Atom(px, pnull, Atom::Lss);
    Polynomial *term4 = pnegx->add(py->mult(pz));
    Atom *term4geqnull = new Atom(term4, pnull, Atom::Geq);
    Atom *term4lssy = new Atom(term4, py, Atom::Lss);
    Constraint *case41 = new Operator(pygtrnull, pxlssnull, Operator::And);
    Constraint *case42 = new Operator(case41, pzleqnull, Operator::And);
    Constraint *case43 = new Operator(case42, term4geqnull, Operator::And);
    Constraint *case4 = new Operator(case43, term4lssy, Operator::And);
    // 5. y < 0 /\ x < 0 /\ z >= 0 /\ -x + y*z >= 0 /\ -x + y*z < -y
    Atom *term4lssnegy = new Atom(term4, pnegy, Atom::Lss);
    Constraint *case51 = new Operator(pylssnull, pxlssnull, Operator::And);
    Constraint *case52 = new Operator(case51, pzgeqnull, Operator::And);
    Constraint *case53 = new Operator(case52, term4geqnull, Operator::And);
    Constraint *case5 = new Operator(case53, term4lssnegy, Operator::And);
    // disjunct...
    Operator *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    disj = new Operator(disj, case4, Operator::Or);
    disj = new Operator(disj, case5, Operator::Or);
    return disj;
}

void Converter::visitSDiv(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *nondef = new Polynomial(getNondef(&I));
        Constraint *divC = NULL;
        Polynomial *upper = getPolynomial(I.getOperand(0));
        Polynomial *lower = getPolynomial(I.getOperand(1));
        if (m_boundedIntegers) {
            unsigned int bitwidth = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
            divC = m_unsignedEncoding ? getSDivConstraintForUnsignedBounded(upper, lower, nondef, bitwidth) : getSDivConstraintForSignedBounded(upper, lower, nondef);
        } else {
            divC = m_exactDivision ? getExactSDivConstraintForUnbounded(upper, lower, nondef) : getSDivConstraintForUnbounded(upper, lower, nondef);
        }
        visitGenericInstruction(I, nondef, divC);
    }
}

Constraint *Converter::getUDivConstraint(DivConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    Constraint *case1 = new Operator(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = x
    Constraint *case2 = new Operator(store.yEQone, store.zEQx, Operator::And);
    // 3. y > 1 /\ x > 0 /\ z < x
    Constraint *case31 = new Operator(store.yGTRone, store.xGTRnull, Operator::And);
    Constraint *case3 = new Operator(case31, store.zLSSx, Operator::And);
    // disjunct...
    Constraint *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    return disj;
}

Constraint *Converter::getUDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    return getSDivConstraintForUnbounded(upper, lower, res);
}

Constraint *Converter::getUDivConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;

    DivConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.zEQx = new Atom(z, x, Atom::Equ);
    Atom *yGTRone1 = new Atom(y, one, Atom::Gtr);
    Atom *yGTRone2 = new Atom(y, null, Atom::Lss);
    store.yGTRone = new Operator(yGTRone1, yGTRone2, Operator::Or);
    Atom *xGTRnull1 = new Atom(x, null, Atom::Gtr);
    Atom *xGTRnull2 = new Atom(x, null, Atom::Lss);
    store.xGTRnull = new Operator(xGTRnull1, xGTRnull2, Operator::Or);
    store.zLSSx = getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_ULT, z, x);

    return getUDivConstraint(store);
}

Constraint *Converter::getUDivConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;

    DivConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.zEQx = new Atom(z, x, Atom::Equ);
    store.yGTRone = new Atom(y, one, Atom::Gtr);
    store.xGTRnull = new Atom(x, null, Atom::Gtr);
    store.zLSSx = new Atom(z, x, Atom::Lss);

    return getUDivConstraint(store);
}

Constraint *Converter::getExactUDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    return getExactSDivConstraintForUnbounded(upper, lower, res);
}

void Converter::visitUDiv(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        return;
    }
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *nondef = new Polynomial(getNondef(&I));
        Constraint *divC = NULL;
        Polynomial *upper = getPolynomial(I.getOperand(0));
        Polynomial *lower = getPolynomial(I.getOperand(1));
        if (m_boundedIntegers) {
            divC = m_unsignedEncoding ? getUDivConstraintForUnsignedBounded(upper, lower, nondef) : getUDivConstraintForSignedBounded(upper, lower, nondef);
        } else {
            divC = m_exactDivision ? getExactUDivConstraintForUnbounded(upper, lower, nondef) : getUDivConstraintForUnbounded(upper, lower, nondef);
        }
        visitGenericInstruction(I, nondef, divC);
    }
}

Constraint *Converter::getSRemConstraint(RemConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    Constraint *case1 = new Operator(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = 0
    Constraint *case2 = new Operator(store.yEQone, store.zEQnull, Operator::And);
    // 3. y = -1 /\ z = 0
    Constraint *case3 = new Operator(store.yEQnegone, store.zEQnull, Operator::And);
    // 4. y > 1 /\ x > 0 /\ z >= 0 /\ z < y
    Constraint *case41 = new Operator(store.yGTRone, store.xGTRnull, Operator::And);
    Constraint *case42 = new Operator(case41, store.zGEQnull, Operator::And);
    Constraint *case4 = new Operator(case42, store.zLSSy, Operator::And);
    // 5. y > 1 /\ x < 0 /\ z <= 0 /\ z > -y
    Constraint *case51 = new Operator(store.yGTRone, store.xLSSnull, Operator::And);
    Constraint *case52 = new Operator(case51, store.zLEQnull, Operator::And);
    Constraint *case5 = new Operator(case52, store.zGTRnegy, Operator::And);
    // 6. y < -1 /\ x > 0 /\ z >= 0 /\ z < -y
    Constraint *case61 = new Operator(store.yLSSnegone, store.xGTRnull, Operator::And);
    Constraint *case62 = new Operator(case61, store.zGEQnull, Operator::And);
    Constraint *case6 = new Operator(case62, store.zLSSnegy, Operator::And);
    // 7. y < -1 /\ x < 0 /\ z <= 0 /\ z > y
    Constraint *case71 = new Operator(store.yLSSnegone, store.xLSSnull, Operator::And);
    Constraint *case72 = new Operator(case71, store.zLEQnull, Operator::And);
    Constraint *case7 = new Operator(case72, store.zGTRy, Operator::And);
    // disjunct...
    Operator *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    disj = new Operator(disj, case4, Operator::Or);
    disj = new Operator(disj, case5, Operator::Or);
    disj = new Operator(disj, case6, Operator::Or);
    disj = new Operator(disj, case7, Operator::Or);
    return disj;
}

Constraint *Converter::getSRemConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;
    Polynomial *negone = Polynomial::negone;
    Polynomial *neglower = lower->constMult(Polynomial::_negone);

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;
    Polynomial *negy = neglower;

    RemConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.yEQnegone = new Atom(y, negone, Atom::Equ);
    store.yGTRone = new Atom(y, one, Atom::Gtr);
    store.xGTRnull = new Atom(x, null, Atom::Gtr);
    store.zGEQnull = new Atom(z, null, Atom::Geq);
    store.zLSSy = new Atom(z, y, Atom::Lss);
    store.xLSSnull = new Atom(x, null, Atom::Lss);
    store.zLEQnull = new Atom(z, null, Atom::Leq);
    store.zGTRnegy = new Atom(z, negy, Atom::Gtr);
    store.yLSSnegone = new Atom(y, negone, Atom::Lss);
    store.zLSSnegy = new Atom(z, negy, Atom::Lss);
    store.zGTRy = new Atom(z, y, Atom::Gtr);

    return getSRemConstraint(store);
}

Constraint *Converter::getSRemConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    return getSRemConstraintForUnbounded(upper, lower, res);
}

Constraint *Converter::getSRemConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res, unsigned int bitwidth)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;
    Polynomial *negone = Polynomial::uimax(bitwidth);
    Polynomial *maxpos = Polynomial::simax(bitwidth);
    Polynomial *minneg = Polynomial::simin_as_ui(bitwidth);
    Polynomial *neglower = lower->constMult(Polynomial::_negone);

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;
    Polynomial *negy = neglower;

    RemConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.yEQnegone = new Atom(y, negone, Atom::Equ);
    Atom *yGTRone1 = new Atom(y, one, Atom::Gtr);
    Atom *yGTRone2 = new Atom(y, maxpos, Atom::Leq);
    store.yGTRone = new Operator(yGTRone1, yGTRone2, Operator::And);
    Atom *xGTRnull1 = new Atom(x, null, Atom::Gtr);
    Atom *xGTRnull2 = new Atom(x, maxpos, Atom::Leq);
    store.xGTRnull = new Operator(xGTRnull1, xGTRnull2, Operator::And);
    store.zGEQnull = new Atom(z, maxpos, Atom::Leq);
    store.zLSSy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, y, bitwidth);
    store.xLSSnull = new Atom(x, minneg, Atom::Geq);
    Atom *zLEQnull1 = new Atom(z, minneg, Atom::Geq);
    Atom *zLEQnull2 = new Atom(z, null, Atom::Equ);
    store.zLEQnull = new Operator(zLEQnull1, zLEQnull2, Operator::Or);
    store.zGTRnegy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, negy, bitwidth);
    Atom *yLSSnegone1 = new Atom(y, minneg, Atom::Geq);
    Atom *yLSSnegone2 = new Atom(y, negone, Atom::Lss);
    store.yLSSnegone = new Operator(yLSSnegone1, yLSSnegone2, Operator::And);
    store.zLSSnegy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SLT, z, negy, bitwidth);
    store.zGTRy = getSignedComparisonForUnsignedBounded(llvm::CmpInst::ICMP_SGT, z, y, bitwidth);

    Constraint *ret = getSRemConstraint(store);
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
        Polynomial *nondef = new Polynomial(getNondef(&I));
        Constraint *remC = NULL;
        Polynomial *upper = getPolynomial(I.getOperand(0));
        Polynomial *lower = getPolynomial(I.getOperand(1));
        if (m_boundedIntegers) {
            unsigned int bitwidth = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
            remC = m_unsignedEncoding ? getSRemConstraintForUnsignedBounded(upper, lower, nondef, bitwidth) : getSRemConstraintForSignedBounded(upper, lower, nondef);
        } else {
            remC = getSRemConstraintForUnbounded(upper, lower, nondef);
        }
        visitGenericInstruction(I, nondef, remC);
    }
}

Constraint *Converter::getURemConstraint(RemConstraintStore &store)
{
    // 1. x = 0 /\ z = 0
    Constraint *case1 = new Operator(store.xEQnull, store.zEQnull, Operator::And);
    // 2. y = 1 /\ z = 0
    Constraint *case2 = new Operator(store.yEQone, store.zEQnull, Operator::And);
    // 3. y > 1 /\ x > 0 /\ z < y
    Constraint *case31 = new Operator(store.yGTRone, store.xGTRnull, Operator::And);
    Constraint *case3 = new Operator(case31, store.zLSSy, Operator::And);
    // disjunct...
    Operator *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    return disj;
}

Constraint *Converter::getURemConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    return getSRemConstraintForUnbounded(upper, lower, res);
}

Constraint *Converter::getURemConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;

    RemConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    Atom *yGTRone1 = new Atom(y, one, Atom::Gtr);
    Atom *yGTRone2 = new Atom(y, null, Atom::Lss);
    store.yGTRone = new Operator(yGTRone1, yGTRone2, Operator::Or);
    Atom *xGTRnull1 = new Atom(x, null, Atom::Gtr);
    Atom *xGTRnull2 = new Atom(x, null, Atom::Lss);
    store.xGTRnull = new  Operator(xGTRnull1, xGTRnull2, Operator::Or);
    store.zLSSy = getUnsignedComparisonForSignedBounded(llvm::CmpInst::ICMP_ULT, z, y);

    return getURemConstraint(store);
}

Constraint *Converter::getURemConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res)
{
    Polynomial *null = Polynomial::null;
    Polynomial *one = Polynomial::one;

    Polynomial *x = upper;
    Polynomial *y = lower;
    Polynomial *z = res;

    RemConstraintStore store;

    store.xEQnull = new Atom(x, null, Atom::Equ);
    store.zEQnull = new Atom(z, null, Atom::Equ);
    store.yEQone = new Atom(y, one, Atom::Equ);
    store.yGTRone = new Atom(y, one, Atom::Gtr);
    store.xGTRnull = new Atom(x, null, Atom::Gtr);
    store.zLSSy = new Atom(z, y, Atom::Lss);

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
        Polynomial *nondef = new Polynomial(getNondef(&I));
        Constraint *remC = NULL;
        Polynomial *upper = getPolynomial(I.getOperand(0));
        Polynomial *lower = getPolynomial(I.getOperand(1));
        if (m_boundedIntegers) {
            remC = m_unsignedEncoding ? getURemConstraintForUnsignedBounded(upper, lower, nondef) : getURemConstraintForSignedBounded(upper, lower, nondef);
        } else {
            remC = getURemConstraintForUnbounded(upper, lower, nondef);
        }
        visitGenericInstruction(I, nondef, remC);
    }
}

Constraint *Converter::getAndConstraintForBounded(Polynomial *x, Polynomial *y, Polynomial *res)
{
    Atom *resLEQx = new Atom(res, x, Atom::Leq);
    Atom *resLEQy = new Atom(res, y, Atom::Leq);
    if (m_unsignedEncoding) {
        return new Operator(resLEQx, resLEQy, Operator::And);
    }
    Polynomial *null = Polynomial::null;
    Atom *xGEQnull = new Atom(x, null, Atom::Geq);
    Atom *yGEQnull = new Atom(y, null, Atom::Geq);
    Atom *resGEQnull = new Atom(res, null, Atom::Geq);
    Atom *xLSSnull = new Atom(x, null, Atom::Lss);
    Atom *yLSSnull = new Atom(y, null, Atom::Lss);
    Atom *resLSSnull = new Atom(res, null, Atom::Lss);
    // case 1: x >= 0 /\ y >= 0 /\ res >= 0 /\ res <= x /\ res <= y
    Operator *case1 = new Operator(xGEQnull, yGEQnull, Operator::And);
    case1 = new Operator(case1, resGEQnull, Operator::And);
    case1 = new Operator(case1, resLEQx, Operator::And);
    case1 = new Operator(case1, resLEQy, Operator::And);
    // case 2: x >= 0 /\ y < 0 /\ res >= 0 /\ res <= x
    Operator *case2 = new Operator(xGEQnull, yLSSnull, Operator::And);
    case2 = new Operator(case2, resGEQnull, Operator::And);
    case2 = new Operator(case2, resLEQx, Operator::And);
    // case 3: x < 0 /\ y >= 0 /\ res >= 0 /\ res <= y
    Operator *case3 = new Operator(xLSSnull, yGEQnull, Operator::And);
    case3 = new Operator(case3, resGEQnull, Operator::And);
    case3 = new Operator(case3, resLEQy, Operator::And);
    // case 4: x < 0 /\ y < 0 /\ res < 0 /\ res <= x /\ res <= y
    Operator *case4 = new Operator(xLSSnull, yLSSnull, Operator::And);
    case4 = new Operator(case4, resLSSnull, Operator::And);
    case4 = new Operator(case4, resLEQx, Operator::And);
    case4 = new Operator(case4, resLEQy, Operator::And);
    // disjunct...
    Operator *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    disj = new Operator(disj, case4, Operator::Or);

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
            Polynomial *x = getPolynomial(I.getOperand(0));
            Polynomial *y = getPolynomial(I.getOperand(1));
            Polynomial *nondef = new Polynomial(getNondef(&I));
            Constraint *c = getAndConstraintForBounded(x, y, nondef);
            visitGenericInstruction(I, nondef, c);
        } else {
            Polynomial *nondef = new Polynomial(getNondef(&I));
            visitGenericInstruction(I, nondef);
        }
    }
}

Constraint *Converter::getOrConstraintForBounded(Polynomial *x, Polynomial *y, Polynomial *res)
{
    Atom *resGEQx = new Atom(res, x, Atom::Leq);
    Atom *resGEQy = new Atom(res, y, Atom::Leq);
    if (m_unsignedEncoding) {
        return new Operator(resGEQx, resGEQy, Operator::And);
    }
    Polynomial *null = Polynomial::null;
    Atom *xGEQnull = new Atom(x, null, Atom::Geq);
    Atom *yGEQnull = new Atom(y, null, Atom::Geq);
    Atom *resGEQnull = new Atom(res, null, Atom::Geq);
    Atom *xLSSnull = new Atom(x, null, Atom::Lss);
    Atom *yLSSnull = new Atom(y, null, Atom::Lss);
    Atom *resLSSnull = new Atom(res, null, Atom::Lss);
    // case 1: x >= 0 /\ y >= 0 /\ res >= 0 /\ res >= x /\ res >= y
    Operator *case1 = new Operator(xGEQnull, yGEQnull, Operator::And);
    case1 = new Operator(case1, resGEQnull, Operator::And);
    case1 = new Operator(case1, resGEQx, Operator::And);
    case1 = new Operator(case1, resGEQy, Operator::And);
    // case 2: x >= 0 /\ y < 0 /\ res < 0 /\ res >= y
    Operator *case2 = new Operator(xGEQnull, yLSSnull, Operator::And);
    case2 = new Operator(case2, resLSSnull, Operator::And);
    case2 = new Operator(case2, resGEQy, Operator::And);
    // case 3: x < 0 /\ y >= 0 /\ res < 0 /\ res >= x
    Operator *case3 = new Operator(xLSSnull, yGEQnull, Operator::And);
    case3 = new Operator(case3, resLSSnull, Operator::And);
    case3 = new Operator(case3, resGEQx, Operator::And);
    // case 4: x < 0 /\ y < 0 /\ res < 0 /\ res >= x /\ res >= y
    Operator *case4 = new Operator(xLSSnull, yLSSnull, Operator::And);
    case4 = new Operator(case4, resLSSnull, Operator::And);
    case4 = new Operator(case4, resGEQx, Operator::And);
    case4 = new Operator(case4, resGEQy, Operator::And);
    // disjunct...
    Operator *disj = new Operator(case1, case2, Operator::Or);
    disj = new Operator(disj, case3, Operator::Or);
    disj = new Operator(disj, case4, Operator::Or);

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
            Polynomial *x = getPolynomial(I.getOperand(0));
            Polynomial *y = getPolynomial(I.getOperand(1));
            Polynomial *nondef = new Polynomial(getNondef(&I));
            Constraint *c = getOrConstraintForBounded(x, y, nondef);
            visitGenericInstruction(I, nondef, c);
        } else {
            Polynomial *nondef = new Polynomial(getNondef(&I));
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
            Polynomial *p1 = getPolynomial(I.getOperand(0));
            Polynomial *p2 = Polynomial::one;
            visitGenericInstruction(I, p1->constMult(Polynomial::_negone)->sub(p2));
        } else {
            Polynomial *nondef = new Polynomial(getNondef(&I));
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
                Constraint *c = m_onlyLoopConditions ? Constraint::_true : getConditionFromValue(callSite.getArgument(0));
                visitGenericInstruction(I, m_lhs, c->toNNF(false));
                return;
            } else if (functionName.startswith("__kittel_nondef")) {
                Polynomial *nondef = new Polynomial(getNondef(&I));
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
        if (I.getType()->isIntegerTy() || I.getType()->isVoidTy() || I.getType()->isFloatingPointTy() || I.getType()->isPointerTy()) {
            std::list<llvm::Function*> callees;
            if (calledFunction != NULL) {
                callees.push_back(calledFunction);
            } else {
                callees = getMatchingFunctions(I);
            }
            std::set<llvm::GlobalVariable*> toZap;
            m_idMap.insert(std::make_pair(&I, m_counter));
            Term *lhs = new Term(getEval(m_counter), m_lhs);
            for (std::list<llvm::Function*>::iterator cf = callees.begin(), cfe = callees.end(); cf != cfe; ++cf) {
                llvm::Function *callee = *cf;
                if (m_scc.find(callee) != m_scc.end() || m_complexityTuples) {
                    std::list<Polynomial*> callArgs;
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
                    Term *rhs2 = new Term(getEval(callee, "start"), callArgs);
                    Rule *rule2 = new Rule(lhs, rhs2, Constraint::_true);
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
            std::list<Polynomial*> newArgs;
            if (I.getType()->isIntegerTy() && I.getType() != m_boolType) {
                Polynomial *nondef = new Polynomial(getNondef(&I));
                newArgs = getZappedArgs(toZap, I, nondef);
            } else {
                newArgs = getZappedArgs(toZap);
            }
            Term *rhs1 = new Term(getEval(m_counter), newArgs);
            Rule *rule1 = new Rule(lhs, rhs1, Constraint::_true);
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
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        m_counter++;
        Term *rhs1 = new Term(getEval(m_counter), getNewArgs(I, getPolynomial(I.getTrueValue())));
        Term *rhs2 = new Term(getEval(m_counter), getNewArgs(I, getPolynomial(I.getFalseValue())));
        Constraint *c = m_onlyLoopConditions ? Constraint::_true : getConditionFromValue(I.getCondition());
        Rule *rule1 = new Rule(lhs, rhs1, c->toNNF(false));
        m_blockRules.push_back(rule1);
        Rule *rule2 = new Rule(lhs, rhs2, c->toNNF(true));
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
        Polynomial *value = getPolynomial(&I);
        visitGenericInstruction(I, value);
    }
}

void Converter::visitPtrToIntInst(llvm::PtrToIntInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *nondef = new Polynomial(getNondef(&I));
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
        Polynomial* newArg = NULL;
        if (musts.size() == 1 && mays.size() == 0) {
            // unique!
            newArg = new Polynomial(getVar(*musts.begin()));
        } else {
            // nondef...
            newArg = new Polynomial(getNondef(&I));
        }
        m_idMap.insert(std::make_pair(&I, m_counter));
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        ++m_counter;
        Term *rhs = new Term(getEval(m_counter), getNewArgs(I, newArg));
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
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
        std::list<Polynomial*> newArgs;
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
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        ++m_counter;
        Term *rhs = new Term(getEval(m_counter), newArgs);
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
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
        Polynomial *nondef = new Polynomial(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitFPToUIInst(llvm::FPToUIInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        Polynomial *nondef = new Polynomial(getNondef(&I));
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
        Polynomial *nondef = new Polynomial(getNondef(&I));
        visitGenericInstruction(I, nondef);
    }
}

void Converter::visitSExtInst(llvm::SExtInst &I)
{
    if (m_phase1) {
        m_vars.push_back(getVar(&I));
    } else {
        m_idMap.insert(std::make_pair(&I, m_counter));
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        Polynomial *copy = getPolynomial(I.getOperand(0));
        ++m_counter;
        if (m_boundedIntegers && m_unsignedEncoding) {
            unsigned int bitwidthNew = llvm::cast<llvm::IntegerType>(I.getType())->getBitWidth();
            unsigned int bitwidthOld = llvm::cast<llvm::IntegerType>(I.getOperand(0)->getType())->getBitWidth();
            Polynomial *sizeNew = Polynomial::power_of_two(bitwidthNew);
            Polynomial *sizeOld = Polynomial::power_of_two(bitwidthOld);
            Polynomial *sizeDiff = sizeNew->sub(sizeOld);
            Polynomial *intmaxOld = Polynomial::simax(bitwidthOld);
            Polynomial *converted = sizeDiff->add(copy);
            Term *rhs1 = new Term(getEval(m_counter), getNewArgs(I, copy));
            Term *rhs2 = new Term(getEval(m_counter), getNewArgs(I, converted));
            Constraint *c1 = new Atom(copy, intmaxOld, Atom::Leq);
            Constraint *c2 = new Atom(copy, intmaxOld, Atom::Gtr);
            Rule *rule1 = new Rule(lhs, rhs1, c1);
            Rule *rule2 = new Rule(lhs, rhs2, c2);
            m_blockRules.push_back(rule1);
            m_blockRules.push_back(rule2);
        } else {
            // mathematical integers or bounded integers with signed encoding
            Term *rhs = new Term(getEval(m_counter), getNewArgs(I, copy));
            Rule *rule = new Rule(lhs, rhs, Constraint::_true);
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
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        ++m_counter;
        if (I.getOperand(0)->getType() == m_boolType) {
            Polynomial *zero = Polynomial::null;
            Polynomial *one = Polynomial::one;
            Term *rhszero = new Term(getEval(m_counter), getNewArgs(I, zero));
            Term *rhsone = new Term(getEval(m_counter), getNewArgs(I, one));
            Constraint *c = getConditionFromValue(I.getOperand(0));
            Rule *rulezero = new Rule(lhs, rhszero, c->toNNF(true));
            Rule *ruleone = new Rule(lhs, rhsone, c->toNNF(false));
            m_blockRules.push_back(rulezero);
            m_blockRules.push_back(ruleone);
        } else {
            Polynomial *copy = getPolynomial(I.getOperand(0));
            if (m_boundedIntegers && !m_unsignedEncoding) {
                unsigned int bitwidthOld = llvm::cast<llvm::IntegerType>(I.getOperand(0)->getType())->getBitWidth();
                Polynomial *shifter = Polynomial::power_of_two(bitwidthOld);
                Polynomial *converted = shifter->add(copy);
                Term *rhs1 = new Term(getEval(m_counter), getNewArgs(I, copy));
                Term *rhs2 = new Term(getEval(m_counter), getNewArgs(I, converted));
                Polynomial *zero = Polynomial::null;
                Constraint *c1 = new Atom(copy, zero, Atom::Geq);
                Constraint *c2 = new Atom(copy, zero, Atom::Lss);
                Rule *rule1 = new Rule(lhs, rhs1, c1);
                Rule *rule2 = new Rule(lhs, rhs2, c2);
                m_blockRules.push_back(rule1);
                m_blockRules.push_back(rule2);
            } else {
                // mathematical integers of bounded integers with unsigned encoding
                Term *rhs = new Term(getEval(m_counter), getNewArgs(I, copy));
                Rule *rule = new Rule(lhs, rhs, Constraint::_true);
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
        Term *lhs = new Term(getEval(m_counter), m_lhs);
        Polynomial *val = NULL;
        if (m_boundedIntegers) {
            val = new Polynomial(getNondef(&I));
        } else {
            val = getPolynomial(I.getOperand(0));
        }
        m_counter++;
        Term *rhs = new Term(getEval(m_counter), getNewArgs(I, val));
        Rule *rule = new Rule(lhs, rhs, Constraint::_true);
        m_blockRules.push_back(rule);
    }
}

bool Converter::isAssumeArg(llvm::ZExtInst &I)
{
    if (I.getOperand(0)->getType() == m_boolType) {
        if (I.getNumUses() != 1) {
            return false;
        }
        llvm::User *user = *I.use_begin();
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
