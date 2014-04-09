// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONVERTER_H
#define CONVERTER_H

#include "llvm2kittel/DivConstraintStore.h"
#include "llvm2kittel/RemConstraintStore.h"
#include "llvm2kittel/Analysis/MemoryAnalyzer.h"
#include "llvm2kittel/Analysis/ConditionPropagator.h"
#include "llvm2kittel/Analysis/LoopConditionExplicitizer.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Support/InstVisitor.h>
#elif LLVM_VERSION < VERSION(3, 5)
  #include <llvm/InstVisitor.h>
#else
  #include <llvm/IR/InstVisitor.h>
#endif
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/InstrTypes.h>
#else
  #include <llvm/IR/InstrTypes.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <list>
#include <map>
#include <set>

#include "WARN_OFF.h"

class Polynomial;
class Rule;

class Converter : public llvm::InstVisitor<Converter>
{

#include "WARN_ON.h"

public:
    Converter(const llvm::Type *boolType, bool assumeIsControl, bool selectIsControl, bool onlyMultiPredIsControl, bool boundedIntegers, bool unsignedEncoding, bool onlyLoopConditions, bool exactDivision, bool bitwiseConditions, bool complexityTuples);

    void phase1(llvm::Function *function, std::set<llvm::Function*> &scc, MayMustMap &mmMap, std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > &funcMayZap, TrueFalseMap &tfMap, std::set<llvm::BasicBlock*> &lcbs, ConditionMap &elcMap);
    void phase2(llvm::Function *function, std::set<llvm::Function*> &scc, MayMustMap &mmMap, std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > &funcMayZap, TrueFalseMap &tfMap, std::set<llvm::BasicBlock*> &lcbs, ConditionMap &elcMap);

    void visitTerminatorInst(llvm::TerminatorInst &I);

    void visitAdd(llvm::BinaryOperator &I);
    void visitSub(llvm::BinaryOperator &I);
    void visitMul(llvm::BinaryOperator &I);
    void visitSDiv(llvm::BinaryOperator &I);
    void visitUDiv(llvm::BinaryOperator &I);
    void visitSRem(llvm::BinaryOperator &I);
    void visitURem(llvm::BinaryOperator &I);

    void visitAnd(llvm::BinaryOperator &I);
    void visitOr(llvm::BinaryOperator &I);
    void visitXor(llvm::BinaryOperator &I);

    void visitCallInst(llvm::CallInst &I);

    void visitSelectInst(llvm::SelectInst &I);

    void visitPHINode(llvm::PHINode &I);

    void visitGetElementPtrInst(llvm::GetElementPtrInst &I);
    void visitIntToPtrInst(llvm::IntToPtrInst &I);
    void visitPtrToIntInst(llvm::PtrToIntInst &I);
    void visitBitCastInst(llvm::BitCastInst &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitAllocaInst(llvm::AllocaInst &I);

    void visitFPToSIInst(llvm::FPToSIInst &I);
    void visitFPToUIInst(llvm::FPToUIInst &I);
    void visitSExtInst(llvm::SExtInst &I);
    void visitZExtInst(llvm::ZExtInst &I);
    void visitTruncInst(llvm::TruncInst &I);

    void visitInstruction(llvm::Instruction &I);

    std::list<Rule*> getRules();

    std::list<Rule*> getCondensedRules();

    std::set<std::string> getPhiVariables();

    std::map<std::string, unsigned int> getBitwidthMap();

    std::set<std::string> getComplexityLHSs();

private:
    llvm::BasicBlock *m_entryBlock;
    void visitBB(llvm::BasicBlock *bb);

    const llvm::Type *m_boolType;

    std::list<Rule*> m_blockRules;
    std::list<Rule*> m_rules;
    std::list<std::string> m_vars;
    std::list<Polynomial*> m_lhs;
    unsigned int m_counter;
    bool m_phase1;

    std::list<llvm::GlobalVariable*> m_globals;
    MayMustMap m_mmMap;
    std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > m_funcMayZap;

    TrueFalseMap m_tfMap;
    ConditionMap m_elcMap;

    std::string getVar(llvm::Value *V);
    std::string getEval(unsigned int i);
    std::string getEval(llvm::BasicBlock *bb, std::string inout);
    std::string getEval(llvm::Function *f, std::string startstop);

    std::list<Polynomial*> getArgsWithPhis(llvm::BasicBlock *from, llvm::BasicBlock *to);

    void visitGenericInstruction(llvm::Instruction &I, std::list<Polynomial*> newArgs, Constraint *c=Constraint::_true);
    void visitGenericInstruction(llvm::Instruction &I, Polynomial *value, Constraint *c=Constraint::_true);

    Polynomial *getPolynomial(llvm::Value *V);
    std::list<Polynomial*> getNewArgs(llvm::Value &V, Polynomial *p);
    std::list<Polynomial*> getZappedArgs(std::set<llvm::GlobalVariable*> toZap);
    std::list<Polynomial*> getZappedArgs(std::set<llvm::GlobalVariable*> toZap, llvm::Value &V, Polynomial *p);

    std::list<llvm::BasicBlock*> m_returns;
    std::map<llvm::Instruction*, unsigned int> m_idMap;

    unsigned int m_nondef;
    std::string getNondef(llvm::Value *V);

    Constraint *getConditionFromValue(llvm::Value *cond);
    Constraint *getConditionFromInstruction(llvm::Instruction *I);
    Constraint *getUnsignedComparisonForSignedBounded(llvm::CmpInst::Predicate pred, Polynomial *x, Polynomial *y);
    Constraint *getSignedComparisonForUnsignedBounded(llvm::CmpInst::Predicate pred, Polynomial *x, Polynomial *y, unsigned int bitwidth);
    Atom::AType getAtomType(llvm::CmpInst::Predicate pred);

    bool m_assumeIsControl;
    bool m_selectIsControl;
    bool m_onlyMultiPredIsControl;
    std::set<std::string> m_controlPoints;

    Constraint *getSDivConstraint(DivConstraintStore &store);
    Constraint *getSDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getSDivConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getSDivConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res, unsigned int bitwidth);
    Constraint *getExactSDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);

    Constraint *getUDivConstraint(DivConstraintStore &store);
    Constraint *getUDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getUDivConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getUDivConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getExactUDivConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);

    Constraint *getSRemConstraint(RemConstraintStore &store);
    Constraint *getSRemConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getSRemConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getSRemConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res, unsigned int bitwidth);

    Constraint *getURemConstraint(RemConstraintStore &store);
    Constraint *getURemConstraintForUnbounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getURemConstraintForSignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);
    Constraint *getURemConstraintForUnsignedBounded(Polynomial *upper, Polynomial *lower, Polynomial *res);

    Constraint *getAndConstraintForBounded(Polynomial *x, Polynomial *y, Polynomial *res);
    Constraint *getOrConstraintForBounded(Polynomial *x, Polynomial *y, Polynomial *res);

    bool m_trivial;

    llvm::Function *m_function;

    std::set<llvm::Function*> m_scc;

    bool isAssumeArg(llvm::ZExtInst &I);

    bool isTrivial(void);

    std::list<llvm::Function*> getMatchingFunctions(llvm::CallInst &I);

    std::set<std::string> m_phiVars;

    bool m_boundedIntegers;
    bool m_unsignedEncoding;

    std::map<std::string, unsigned int> m_bitwidthMap;

    Constraint *buildConjunction(std::set<llvm::Value*> &trues, std::set<llvm::Value*> &falses);
    Constraint *buildBoundConjunction(std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > &bounds);

    bool m_onlyLoopConditions;
    std::set<llvm::BasicBlock*> m_loopConditionBlocks;

    bool m_exactDivision;

    bool m_bitwiseConditions;

    bool m_complexityTuples;
    std::set<std::string> m_complexityLHSs;

private:
    Converter(const Converter &);
    Converter &operator=(const Converter &);

};

#endif // CONVERTER_H
