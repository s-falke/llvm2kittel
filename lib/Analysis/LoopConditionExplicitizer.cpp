// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Util/Version.h"

#include "llvm2kittel/Analysis/LoopConditionExplicitizer.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Instructions.h>
#else
  #include <llvm/IR/Instructions.h>
#endif
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/CallSite.h>
#else
  #include <llvm/IR/CallSite.h>
#endif
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/CFG.h>
#else
  #include <llvm/IR/CFG.h>
#endif
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "WARN_ON.h"

#include <iostream>
#include <cstdlib>

char LoopConditionExplicitizer::ID = 0;

LoopConditionExplicitizer::LoopConditionExplicitizer(bool debug)
  : llvm::LoopPass(ID),
    m_map(),
    m_debug(debug)
{}

void LoopConditionExplicitizer::getAnalysisUsage(llvm::AnalysisUsage&) const
{}

void LoopConditionExplicitizer::printSet(std::set<llvm::Value*> X)
{
    if (X.size() == 0) {
        std::cout << "(NONE)";
    } else {
        for (std::set<llvm::Value*>::iterator i = X.begin(), e = X.end(); i != e; ) {
            llvm::Value *v = *i;
            std::string name = v->getName().str();
            if (name.find_first_not_of("0123456789") == name.npos) {
                std::cout << "%\"" << name << "\"";
            } else {
                std::cout << "%" << name;
            }
            if (++i != e) {
                std::cout << ", ";
            }
        }
    }
}

static llvm::CmpInst::Predicate flip(llvm::CmpInst::Predicate pred)
{
    switch (pred) {
    case llvm::CmpInst::ICMP_SGT:
        return llvm::CmpInst::ICMP_SLT;
    case llvm::CmpInst::ICMP_UGT:
        return llvm::CmpInst::ICMP_ULT;
    case llvm::CmpInst::ICMP_SGE:
        return llvm::CmpInst::ICMP_SLE;
    case llvm::CmpInst::ICMP_UGE:
        return llvm::CmpInst::ICMP_ULE;
    case llvm::CmpInst::ICMP_SLT:
        return llvm::CmpInst::ICMP_SGT;
    case llvm::CmpInst::ICMP_ULT:
        return llvm::CmpInst::ICMP_UGT;
    case llvm::CmpInst::ICMP_SLE:
        return llvm::CmpInst::ICMP_SGE;
    case llvm::CmpInst::ICMP_ULE:
        return llvm::CmpInst::ICMP_UGE;
    case llvm::CmpInst::ICMP_EQ:
    case llvm::CmpInst::ICMP_NE:
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
        return pred;
    }
}

// Make loop conditions explicit
bool LoopConditionExplicitizer::runOnLoop(llvm::Loop *L, llvm::LPPassManager &)
{
    llvm::PHINode *iv = L->getCanonicalInductionVariable();
    llvm::BasicBlock *exiting = L->getExitingBlock();
    if (iv != NULL && exiting != NULL) {
        llvm::Value *exitingTerminator = exiting->getTerminator();
        if (llvm::isa<llvm::BranchInst>(exitingTerminator)) {
            llvm::BranchInst *br = llvm::cast<llvm::BranchInst>(exitingTerminator);
            if (br->isConditional()) {
                llvm::Value *brCond = br->getCondition();
                if (llvm::isa<llvm::ICmpInst>(brCond)) {
                    std::vector<llvm::Loop*> subLoops = L->getSubLoops();
                    llvm::ICmpInst *icmp = llvm::cast<llvm::ICmpInst>(brCond);
                    llvm::Value *zero = llvm::Constant::getNullValue(iv->getType());
                    llvm::CmpInst::Predicate pred = icmp->getPredicate();
                    bool doflip = !L->contains(br->getSuccessor(0));
                    if (icmp->getOperand(0) == iv) {
                        llvm::Value *op = icmp->getOperand(1);
                        doflip = !doflip;
                        if (!llvm::isa<llvm::Instruction>(op) || !L->contains(llvm::cast<llvm::Instruction>(op))) {
                            insertForAll(op, doflip ? flip(pred) : pred, zero, zero, subLoops);
                        }
                    } else if (icmp->getOperand(1) == iv) {
                        llvm::Value *op = icmp->getOperand(0);
                        if (!llvm::isa<llvm::Instruction>(op) || !L->contains(llvm::cast<llvm::Instruction>(op))) {
                            insertForAll(op, doflip ? flip(pred) : pred, zero, zero, subLoops);
                        }
                    }
                    llvm::BasicBlock *exitBB = L->getExitBlock();
                    if (exitBB != NULL && exitBB->getSinglePredecessor() == exiting) {
                        if (icmp->getOperand(0) == iv) {
                            if (pred == llvm::CmpInst::ICMP_SLT || pred == llvm::CmpInst::ICMP_ULT) {
                                insert(iv, llvm::CmpInst::ICMP_EQ, icmp->getOperand(1), zero, exitBB);
                            } else if (pred == llvm::CmpInst::ICMP_SLE || pred == llvm::CmpInst::ICMP_ULE) {
                            }
                        } else if (icmp->getOperand(1) == iv) {
                            if (pred == llvm::CmpInst::ICMP_SGT || pred == llvm::CmpInst::ICMP_UGT) {
                                insert(iv, llvm::CmpInst::ICMP_EQ, icmp->getOperand(0), zero, exitBB);
                            } else if (pred == llvm::CmpInst::ICMP_SGE || pred == llvm::CmpInst::ICMP_UGE) {
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

void LoopConditionExplicitizer::insertForAll(llvm::Value* val1, llvm::CmpInst::Predicate pred, llvm::Value *val2, llvm::Value* off, std::vector<llvm::Loop*> &loops)
{
    for (std::vector<llvm::Loop*>::iterator i = loops.begin(), e = loops.end(); i != e; ++i) {
        insertForAll(val1, pred, val2, off, *i);
    }
}

void LoopConditionExplicitizer::insertForAll(llvm::Value* val1, llvm::CmpInst::Predicate pred, llvm::Value *val2, llvm::Value* off, llvm::Loop* loop)
{
    for (llvm::Loop::block_iterator i = loop->block_begin(), e = loop->block_end(); i != e; ++i) {
        insert(val1, pred, val2, off, *i);
    }
}

/*
static std::string predString(llvm::CmpInst::Predicate pred)
{
    switch (pred) {
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::ICMP_UGT:
        return ">";
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::ICMP_UGE:
        return ">=";
    case llvm::CmpInst::ICMP_SLT:
    case llvm::CmpInst::ICMP_ULT:
        return "<";
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::ICMP_ULE:
        return "<=";
    case llvm::CmpInst::ICMP_EQ:
        return "=";
    case llvm::CmpInst::ICMP_NE:
        return "!=";
    default:
        return "unknown";
    }
}
*/

void LoopConditionExplicitizer::insert(llvm::Value *val1, llvm::CmpInst::Predicate pred, llvm::Value *val2, llvm::Value* off, llvm::BasicBlock *bb)
{
    std::map<llvm::BasicBlock*, std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > >::iterator it = m_map.find(bb);
    if (it == m_map.end()) {
        std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > tmp;
        tmp.insert(make_quadruple(val1, pred, val2, off));
        m_map.insert(std::make_pair(bb, tmp));
    } else {
        std::set<quadruple<llvm::Value*, llvm::CmpInst::Predicate, llvm::Value*, llvm::Value*> > tmp = it->second;
        m_map.erase(it);
        tmp.insert(make_quadruple(val1, pred, val2, off));
        m_map.insert(std::make_pair(bb, tmp));
    }
}

LoopConditionExplicitizer *createLoopConditionExplicitizerPass(bool debug)
{
    return new LoopConditionExplicitizer(debug);
}
