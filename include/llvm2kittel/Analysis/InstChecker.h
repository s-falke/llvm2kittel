// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef INST_CHECKER_H
#define INST_CHECKER_H

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

#include "WARN_OFF.h"

class InstChecker : public llvm::InstVisitor<InstChecker>
{

#include "WARN_ON.h"

public:
    InstChecker(const llvm::Type *boolType, const llvm::Type *floatType, const llvm::Type *doubleType);
    ~InstChecker();

    void visitTerminatorInst(llvm::TerminatorInst &I);

    void visitAdd(llvm::BinaryOperator &I);
    void visitSub(llvm::BinaryOperator &I);
    void visitMul(llvm::BinaryOperator &I);
    void visitSDiv(llvm::BinaryOperator &I);
    void visitSRem(llvm::BinaryOperator &I);
    void visitUDiv(llvm::BinaryOperator &I);
    void visitURem(llvm::BinaryOperator &I);

    void visitAnd(llvm::BinaryOperator &I);
    void visitOr(llvm::BinaryOperator &I);
    void visitXor(llvm::BinaryOperator &I);

    void visitZExtInst(llvm::ZExtInst &I);
    void visitSExtInst(llvm::SExtInst &I);
    void visitTruncInst(llvm::TruncInst &I);

    void visitICmpInst(llvm::ICmpInst &I);

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
    void visitFCmpInst(llvm::FCmpInst &I);

    void visitInstruction(llvm::Instruction &I);

    std::list<llvm::Instruction*> getUnsuitableInsts();

private:
    const llvm::Type *m_boolType;
    const llvm::Type *m_floatType;
    const llvm::Type *m_doubleType;

    std::list<llvm::Instruction*> m_unsuitableInsts;

    void addUnsuitable(llvm::Instruction &I);

private:
    InstChecker(const InstChecker &);
    InstChecker &operator=(const InstChecker &);

};

#endif // INST_CHECKER_H
