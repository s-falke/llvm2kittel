// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/ConditionPropagator.h"
#include "llvm2kittel/Util/Version.h"

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
#if LLVM_VERSION < VERSION(3, 4)
  #include <llvm/Transforms/Utils/BasicBlockUtils.h>
#else
  #include <llvm/Analysis/CFG.h>
#endif
#include "WARN_ON.h"

#include <iostream>
#include <cstdlib>

char ConditionPropagator::ID = 0;

ConditionPropagator::ConditionPropagator(bool debug, bool onlyLoopConditions, std::set<llvm::BasicBlock*> lcbs)
  : FunctionPass(ID),
    m_map(),
    m_debug(debug),
    m_onlyLoopConditions(onlyLoopConditions),
    m_lcbs(lcbs)
{}

void ConditionPropagator::getAnalysisUsage(llvm::AnalysisUsage&) const
{}

bool ConditionPropagator::isPred(llvm::BasicBlock *bb1, llvm::BasicBlock *bb2)
{
    for (llvm::pred_iterator pi = llvm::pred_begin(bb2), pe = llvm::pred_end(bb2); pi != pe; ++pi) {
        llvm::BasicBlock *pred = *pi;
        if (pred == bb1) {
            return true;
        }
    }
    return false;
}

std::set<llvm::Value*> ConditionPropagator::intersect(std::set<llvm::Value*> X, std::set<llvm::Value*> Y)
{
    std::set<llvm::Value*> res;
    for (std::set<llvm::Value*>::iterator i = X.begin(), e = X.end(); i != e; ++i) {
        if (Y.find(*i) != Y.end()) {
            res.insert(*i);
        }
    }
    return res;
}

void ConditionPropagator::printSet(std::set<llvm::Value*> X)
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

// Propagate conditions
bool ConditionPropagator::runOnFunction(llvm::Function &F)
{
    m_map.clear();
    llvm::SmallVector<std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*>, 32> backedgesVector;
    llvm::FindFunctionBackedges(F, backedgesVector);
    std::set<std::pair<const llvm::BasicBlock*, const llvm::BasicBlock*> > backedges;
    backedges.insert(backedgesVector.begin(), backedgesVector.end());
    if (m_debug) {
        std::cout << "========================================" << std::endl;
    }
    for (llvm::Function::iterator bbi = F.begin(), bbe = F.end(); bbi != bbe; ++bbi) {
        llvm::BasicBlock *bb = bbi;
        std::set<llvm::BasicBlock*> preds;
        for (llvm::Function::iterator tmpi = F.begin(), tmpe = F.end(); tmpi != tmpe; ++tmpi) {
            if (isPred(tmpi, bb) && backedges.find(std::make_pair(tmpi, bb)) == backedges.end()) {
                if (m_debug) {
                    std::cout << bb->getName().str() << " has non-backedge predecessor " << tmpi->getName().str() << std::endl;
                }
                preds.insert(tmpi);
            }
        }
        std::set<llvm::Value*> trueSet;
        std::set<llvm::Value*> falseSet;
        bool haveStarted = false;
        for (std::set<llvm::BasicBlock*>::iterator i = preds.begin(), e = preds.end(); i != e; ++i) {
            TrueFalseMap::iterator it = m_map.find(*i);
            if (it == m_map.end()) {
                std::cerr << "Did not find condition information for predecessor " << (*i)->getName().str() << "!" << std::endl;
                exit(99999);
            }
            if (!haveStarted) {
                trueSet = it->second.first;
                falseSet = it->second.second;
                haveStarted = true;
            } else {
                // intersect
                trueSet = intersect(trueSet, it->second.first);
                falseSet = intersect(falseSet, it->second.second);
            }
        }
        if (preds.size() == 1) {
            llvm::BasicBlock *pred = *(preds.begin());
            // branch condition!
            if (!m_onlyLoopConditions || m_lcbs.find(pred) != m_lcbs.end()) {
                llvm::TerminatorInst *termi = pred->getTerminator();
                if (llvm::isa<llvm::BranchInst>(termi)) {
                    llvm::BranchInst *br = llvm::cast<llvm::BranchInst>(termi);
                    if (br->isConditional()) {
                        if (br->getSuccessor(0) == bb) {
                            // branch on true
                            trueSet.insert(br->getCondition());
                        } else {
                            // branch on false
                            falseSet.insert(br->getCondition());
                        }
                    }
                }
            }
            // assumes!
            if (!m_onlyLoopConditions) {
                for (llvm::BasicBlock::iterator insti = pred->begin(), inste = pred->end(); insti != inste; ++insti) {
                    if (llvm::isa<llvm::CallInst>(insti)) {
                        llvm::CallInst *ci = llvm::cast<llvm::CallInst>(insti);
                        llvm::Function *calledFunction = ci->getCalledFunction();
                        if (calledFunction != NULL) {
                            std::string functionName = calledFunction->getName().str();
                            if (functionName == "__kittel_assume") {
                                llvm::CallSite callSite(ci);
                                trueSet.insert(callSite.getArgument(0));
                            }
                        }
                    }
                }
            }
        }
        if (m_debug) {
            std::cout << "In " << bb->getName().str() << ":" << std::endl;
            std::cout << "TRUE: "; printSet(trueSet); std::cout << std::endl;
            std::cout << "FALSE: "; printSet(falseSet); std::cout << std::endl;
            if (++bbi != bbe) {
                std::cout << std::endl;
            }
            --bbi;
        }
        m_map.insert(std::make_pair(bb, std::make_pair(trueSet, falseSet)));
    }
    return false;
}

ConditionPropagator *createConditionPropagatorPass(bool debug, bool onlyLoopConditions, std::set<llvm::BasicBlock*> lcbs)
{
    return new ConditionPropagator(debug, onlyLoopConditions, lcbs);
}
