// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONVERTER_H
#define CONVERTER_H

#include "llvm2kittel/DivConstraintStore.h"
#include "llvm2kittel/DivRemConstraintType.h"
#include "llvm2kittel/RemConstraintStore.h"
#include "llvm2kittel/Analysis/MemoryAnalyzer.h"
#include "llvm2kittel/Analysis/ConditionPropagator.h"
#include "llvm2kittel/Analysis/LoopConditionExplicitizer.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/Util/Ref.h"
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
#include <fstream>

#include "WARN_OFF.h"

class Polynomial;
class Rule;

class Converter : public llvm::InstVisitor<Converter>
{

#include "WARN_ON.h"

public:
    Converter(const llvm::Type *boolType, bool assumeIsControl, bool selectIsControl, bool onlyMultiPredIsControl, bool boundedIntegers, bool unsignedEncoding, bool onlyLoopConditions, DivRemConstraintType divisionConstraintType, bool bitwiseConditions, bool complexityTuples, std::ofstream &t2file);

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

    std::list<ref<Rule> > getRules();

    std::list<ref<Rule> > getCondensedRules();

    std::set<std::string> getPhiVariables();

    std::map<std::string, unsigned int> getBitwidthMap();

    std::set<std::string> getComplexityLHSs();

private:
    llvm::BasicBlock *m_entryBlock;
    void visitBB(llvm::BasicBlock *bb);

    const llvm::Type *m_boolType;

    std::list<ref<Rule> > m_blockRules;
    std::ofstream *m_t2file;
    std::list<ref<Rule> > m_rules;
    std::list<std::string> m_vars;
    std::list<ref<Polynomial> > m_lhs;
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

    std::list<ref<Polynomial> > getArgsWithPhis(llvm::BasicBlock *from, llvm::BasicBlock *to);

    void visitGenericInstruction(llvm::Instruction &I, std::list<ref<Polynomial> > newArgs, ref<Constraint> c=Constraint::_true);
    void visitGenericInstruction(llvm::Instruction &I, ref<Polynomial> value, ref<Constraint> c=Constraint::_true);

    ref<Polynomial> getPolynomial(llvm::Value *V);
    std::list<ref<Polynomial> > getNewArgs(llvm::Value &V, ref<Polynomial> p);
    std::list<ref<Polynomial> > getZappedArgs(std::set<llvm::GlobalVariable*> toZap);
    std::list<ref<Polynomial> > getZappedArgs(std::set<llvm::GlobalVariable*> toZap, llvm::Value &V, ref<Polynomial> p);

    std::list<llvm::BasicBlock*> m_returns;
    std::map<llvm::Instruction*, unsigned int> m_idMap;
    std::map<llvm::BasicBlock*, std::list<std::pair<std::string,llvm::Value*>>> m_phiMap;
    
    unsigned int m_nondef;
    std::string getNondef(llvm::Value *V);

    ref<Constraint> getConditionFromValue(llvm::Value *cond);
    ref<Constraint> getConditionFromInstruction(llvm::Instruction *I);
    ref<Constraint> getUnsignedComparisonForSignedBounded(llvm::CmpInst::Predicate pred, ref<Polynomial> x, ref<Polynomial> y);
    ref<Constraint> getSignedComparisonForUnsignedBounded(llvm::CmpInst::Predicate pred, ref<Polynomial> x, ref<Polynomial> y, unsigned int bitwidth);
    Atom::AType getAtomType(llvm::CmpInst::Predicate pred);

    bool m_assumeIsControl;
    bool m_selectIsControl;
    bool m_onlyMultiPredIsControl;
    std::set<std::string> m_controlPoints;

    ref<Constraint> getSDivConstraint(DivConstraintStore &store);
    ref<Constraint> getSDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getSDivConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getSDivConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res, unsigned int bitwidth);
    ref<Constraint> getExactSDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);

    ref<Constraint> getUDivConstraint(DivConstraintStore &store);
    ref<Constraint> getUDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getUDivConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getUDivConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getExactUDivConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);

    ref<Constraint> getSRemConstraint(RemConstraintStore &store);
    ref<Constraint> getSRemConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getSRemConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getSRemConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res, unsigned int bitwidth);

    ref<Constraint> getURemConstraint(RemConstraintStore &store);
    ref<Constraint> getURemConstraintForUnbounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getURemConstraintForSignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);
    ref<Constraint> getURemConstraintForUnsignedBounded(ref<Polynomial> upper, ref<Polynomial> lower, ref<Polynomial> res);

    ref<Constraint> getAndConstraintForBounded(ref<Polynomial> x, ref<Polynomial> y, ref<Polynomial> res);
    ref<Constraint> getOrConstraintForBounded(ref<Polynomial> x, ref<Polynomial> y, ref<Polynomial> res);

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

    ref<Constraint> buildConjunction(std::set<llvm::Value*> &trues, std::set<llvm::Value*> &falses);
    ref<Constraint> buildBoundConjunction(std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > &bounds);

    bool m_onlyLoopConditions;
    std::set<llvm::BasicBlock*> m_loopConditionBlocks;

    DivRemConstraintType m_divisionConstraintType;

    bool m_bitwiseConditions;

    bool m_complexityTuples;
    std::set<std::string> m_complexityLHSs;

private:
    Converter(const Converter &);
    Converter &operator=(const Converter &);

};

#endif // CONVERTER_H
