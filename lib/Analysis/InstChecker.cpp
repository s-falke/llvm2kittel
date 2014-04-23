// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/InstChecker.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/CallSite.h>
#else
  #include <llvm/IR/CallSite.h>
#endif
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Constants.h>
#else
  #include <llvm/IR/Constants.h>
#endif
#include "WARN_ON.h"

InstChecker::InstChecker(const llvm::Type *boolType, const llvm::Type *floatType, const llvm::Type *doubleType)
  : m_boolType(boolType),
    m_floatType(floatType),
    m_doubleType(doubleType),
    m_unsuitableInsts()
{}

InstChecker::~InstChecker()
{}

void InstChecker::addUnsuitable(llvm::Instruction &I)
{
    m_unsuitableInsts.push_back(&I);
}

void InstChecker::visitTerminatorInst(llvm::TerminatorInst &I)
{
    if (!llvm::isa<llvm::BranchInst>(&I) && !llvm::isa<llvm::ReturnInst>(&I) && !llvm::isa<llvm::UnreachableInst>(&I)) {
        addUnsuitable(I);
    }
}

void InstChecker::visitAdd(llvm::BinaryOperator &I)
{
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitSub(llvm::BinaryOperator &I)
{
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitMul(llvm::BinaryOperator &I)
{
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitSDiv(llvm::BinaryOperator &I)
{
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitSRem(llvm::BinaryOperator &I)
{
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitAnd(llvm::BinaryOperator &)
{}

void InstChecker::visitOr(llvm::BinaryOperator &)
{}

void InstChecker::visitXor(llvm::BinaryOperator &I)
{
    if (I.getType() == m_boolType) {
        if (!llvm::isa<llvm::ConstantInt>(I.getOperand(0)) && !llvm::isa<llvm::ConstantInt>(I.getOperand(1))) {
            // logical xor with constant is identity or negation
            addUnsuitable(I);
        }
    }
}

void InstChecker::visitZExtInst(llvm::ZExtInst &)
{}

void InstChecker::visitSExtInst(llvm::SExtInst &I)
{
    if (I.getOperand(0)->getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitTruncInst(llvm::TruncInst &)
{}

void InstChecker::visitICmpInst(llvm::ICmpInst &I)
{
    if (I.getOperand(0)->getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitCallInst(llvm::CallInst &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
        return;
    }
    llvm::CallSite callSite(&I);
    llvm::Function *calledFunction = callSite.getCalledFunction();
    if (calledFunction == NULL) {
        llvm::Value *calledValue = callSite.getCalledValue();
        if (calledValue == NULL) {
            addUnsuitable(I);
        } else if (llvm::cast<llvm::FunctionType>(llvm::cast<llvm::PointerType>(calledValue->getType())->getContainedType(0))->isVarArg()) {
            addUnsuitable(I);
        }
        return;
    }
    if (!calledFunction->isDeclaration() && calledFunction->getFunctionType()->isVarArg()) {
        addUnsuitable(I);
        return;
    }
    std::string functionName = calledFunction->getName().str();
    if (functionName == "__kittel_assume") {
        if (llvm::isa<llvm::ZExtInst>(callSite.getArgument(0))) {
            llvm::ZExtInst *zext = llvm::cast<llvm::ZExtInst>(callSite.getArgument(0));
            if (zext->getOperand(0)->getType() != m_boolType) {
                addUnsuitable(I);
                return;
            }
        }
    } else if (functionName == "__kittel_nondef") {
        if (callSite.getType() == m_boolType) {
            addUnsuitable(I);
            return;
        }
    } else {
        if (!I.getType()->isVoidTy() && !I.getType()->isFloatTy() && !I.getType()->isDoubleTy() && !I.getType()->isIntegerTy() && !I.getType()->isPointerTy() && !I.getType()->isVectorTy() && !I.getType()->isStructTy()) {
            addUnsuitable(I);
        }
    }
}

void InstChecker::visitSelectInst(llvm::SelectInst &I)
{}

void InstChecker::visitPHINode(llvm::PHINode &I)
{
    if (I.getType()->isFloatTy() || I.getType()->isDoubleTy() || llvm::isa<llvm::PointerType>(I.getType())) {
        return;
    }
    if (!llvm::isa<llvm::IntegerType>(I.getType())) {
        addUnsuitable(I);
    }
}

void InstChecker::visitGetElementPtrInst(llvm::GetElementPtrInst &)
{}

void InstChecker::visitIntToPtrInst(llvm::IntToPtrInst &)
{}

void InstChecker::visitPtrToIntInst(llvm::PtrToIntInst &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitBitCastInst(llvm::BitCastInst &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitLoadInst(llvm::LoadInst &)
{}

void InstChecker::visitStoreInst(llvm::StoreInst &)
{}

void InstChecker::visitAllocaInst(llvm::AllocaInst &)
{}

void InstChecker::visitFPToSIInst(llvm::FPToSIInst &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitFPToUIInst(llvm::FPToUIInst &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
    }
}

void InstChecker::visitFCmpInst(llvm::FCmpInst &)
{}

void InstChecker::visitInstruction(llvm::Instruction &I)
{
    if (I.getType() == m_boolType) {
        addUnsuitable(I);
    }
}

std::list<llvm::Instruction*> InstChecker::getUnsuitableInsts()
{
    return m_unsuitableInsts;
}
