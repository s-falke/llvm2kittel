// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

// Contains code from LLVM's LICM.cpp

#include "llvm2kittel/Transform/Hoister.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include <llvm/InitializePasses.h>
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/LLVMContext.h>
  #include <llvm/Constants.h>
#else
  #include <llvm/IR/LLVMContext.h>
  #include <llvm/IR/Constants.h>
#endif
#include <llvm/PassRegistry.h>
#include "WARN_ON.h"

// C++ includes
#include <iostream>

char Hoister::ID = 0;

Hoister::Hoister()
  : llvm::LoopPass(ID),
    changed(false),
    CurAST(NULL),
    LoopToAliasMap()
{}

void Hoister::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
    AU.addRequired<llvm::AliasAnalysis>();
#if LLVM_VERSION < VERSION(3, 5)
    AU.addRequired<llvm::DominatorTree>();
#else
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
#endif
    AU.addRequired<llvm::LoopInfo>();
}

bool Hoister::doFinalization()
{
    // Free the values stored in the map
    for (std::map<llvm::Loop*, llvm::AliasSetTracker*>::iterator I = LoopToAliasMap.begin(), E = LoopToAliasMap.end(); I != E; ++I) {
        delete I->second;
    }
    LoopToAliasMap.clear();
    return false;
}

/// Hoist expressions out of the specified loop.
bool Hoister::runOnLoop(llvm::Loop *L, llvm::LPPassManager &)
{
    changed = false;

    llvm::AliasAnalysis *AA = &getAnalysis<llvm::AliasAnalysis>();
#if LLVM_VERSION < VERSION(3, 5)
    llvm::DominatorTree *DT = &getAnalysis<llvm::DominatorTree>();
#else
    llvm::DominatorTree *DT = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
#endif
    llvm::LoopInfo *LI = &getAnalysis<llvm::LoopInfo>();

    CurAST = new llvm::AliasSetTracker(*AA);
    // Collect Alias info from subloops
    for (llvm::Loop::iterator LoopItr = L->begin(), LoopItrE = L->end(); LoopItr != LoopItrE; ++LoopItr) {
        llvm::Loop *InnerL = *LoopItr;
        llvm::AliasSetTracker *InnerAST = LoopToAliasMap[InnerL];
        // What if InnerLoop was modified by other passes ?
        CurAST->add(*InnerAST);
    }

    // Get the preheader block to move instructions into...
    llvm::BasicBlock *preheader = L->getLoopPreheader();

    // Loop over the body of this loop, looking for calls, invokes, and stores.
    // Because subloops have already been incorporated into AST, we skip blocks in
    // subloops.
    for (llvm::Loop::block_iterator I = L->block_begin(), E = L->block_end(); I != E; ++I) {
        llvm::BasicBlock *BB = *I;
        if (LI->getLoopFor(BB) == L)          // Ignore blocks in subloops...
            CurAST->add(*BB);                 // Incorporate the specified basic block
    }

    // We want to visit all of the instructions in this loop... that are not parts
    // of our subloops (they have already had their invariants hoisted out of
    // their loop, into this loop, so there is no need to process the BODIES of
    // the subloops).
    if (preheader != NULL) {
        HoistRegion(L, DT->getNode(L->getHeader()), AA);
    }

    LoopToAliasMap[L] = CurAST;

    return changed;
}

/// HoistRegion - Walk the specified region of the CFG (defined by all blocks
/// dominated by the specified block, and that are in the current loop) in depth
/// first order w.r.t the DominatorTree.  This allows us to visit definitions
/// before uses, allowing us to hoist a loop body in one pass without iteration.
void Hoister::HoistRegion(llvm::Loop *L, llvm::DomTreeNode *N, llvm::AliasAnalysis *AA)
{
    llvm::BasicBlock *BB = N->getBlock();

    // If this subregion is not in the top level loop at all, exit.
    if (!L->contains(BB)) {
        return;
    }

    // Only need to process the contents of this block if it is not part of a
    // subloop (which would already have been processed).
    if (!inSubLoop(L, BB)) {
        for (llvm::BasicBlock::iterator II = BB->begin(), E = BB->end(); II != E; ) {
            llvm::Instruction &I = *II++;

            // Try hoisting the instruction out to the preheader.  We can only do this
            // if all of the operands of the instruction are loop invariant and if it
            // is safe to hoist the instruction.
            if (isLoopInvariantInst(L, I) && canHoistInst(I, AA)) {
                hoist(L->getLoopPreheader(), I);
            }
        }

        const std::vector<llvm::DomTreeNode*> &children = N->getChildren();
        for (unsigned int i = 0, e = static_cast<unsigned int>(children.size()); i != e; ++i) {
            HoistRegion(L, children[i], AA);
        }
    }
}

bool Hoister::doesNotAccessMemory(llvm::Function *F)
{
    if (F == NULL || F->isDeclaration()) {
        return false;
    }
    for (llvm::Function::iterator bb = F->begin(), fend = F->end(); bb != fend; ++bb) {
        for (llvm::BasicBlock::iterator inst = bb->begin(), bbend = bb->end(); inst != bbend; ++inst) {
            if (llvm::isa<llvm::LoadInst>(inst) || llvm::isa<llvm::StoreInst>(inst)) {
                return false;
            }
            if (llvm::isa<llvm::CallInst>(inst)) {
                llvm::CallInst *callInst = llvm::cast<llvm::CallInst>(inst);
                llvm::Function *calledFunction = callInst->getCalledFunction();
                if (calledFunction == NULL) {
                    return false;
                }
                if (!doesNotAccessMemory(calledFunction)) {
                    return false;
                }
            }
        }
    }
    return true;
}

/// canHoistInst - Return true if the hoister can handle this instruction.
bool Hoister::canHoistInst(llvm::Instruction &I, llvm::AliasAnalysis *AA)
{
    // Loads have extra constraints we have to verify before we can hoist them.
    if (llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
        if (LI->isVolatile()) {
            return false;        // Don't hoist volatile loads!
        }

        // Loads from constant memory are always safe to move, even if they end up
        // in the same alias set as something that ends up being modified.
        if (AA->pointsToConstantMemory(LI->getOperand(0))) {
            return true;
        }

        // Don't hoist loads which have may-aliased stores in loop.
        uint64_t Size = 0;
        if (LI->getType()->isSized()) {
            Size = AA->getTypeStoreSize(LI->getType());
        }
#if LLVM_VERSION <= VERSION(3, 5)
        return !CurAST->getAliasSetForPointer(LI->getOperand(0), Size, LI->getMetadata(llvm::LLVMContext::MD_tbaa)).isMod();
#else
        llvm::AAMDNodes AAInfo;
        LI->getAAMetadata(AAInfo);
        return !CurAST->getAliasSetForPointer(LI->getOperand(0), Size, AAInfo).isMod();
#endif
    } else if (llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
        // Handle obvious cases efficiently.
        llvm::AliasAnalysis::ModRefBehavior Behavior = AA->getModRefBehavior(CI);
        if (Behavior == llvm::AliasAnalysis::DoesNotAccessMemory || doesNotAccessMemory(CI->getCalledFunction())) {
            return true;
        } else if (Behavior == llvm::AliasAnalysis::OnlyReadsMemory) {
            // If this call only reads from memory and there are no writes to memory
            // in the loop, we can hoist or sink the call as appropriate.
            bool FoundMod = false;
            for (llvm::AliasSetTracker::iterator IT = CurAST->begin(), ET = CurAST->end(); IT != ET; ++IT) {
                llvm::AliasSet &AS = *IT;
                if (!AS.isForwardingAliasSet() && AS.isMod()) {
                    FoundMod = true;
                    break;
                }
            }
            if (!FoundMod) {
                return true;
            }
        }

        // FIXME: This should use mod/ref information to see if we can hoist the call.
        return false;
    }

    // Otherwise these instructions are hoistable.
    if (llvm::isa<llvm::BinaryOperator>(I)) {
        llvm::BinaryOperator *binop = llvm::cast<llvm::BinaryOperator>(&I);
        llvm::BinaryOperator::BinaryOps opcode = binop->getOpcode();
        if (opcode == llvm::BinaryOperator::Xor) {
            if (llvm::isa<llvm::ConstantInt>(I.getOperand(1)) && llvm::cast<llvm::ConstantInt>(I.getOperand(1))->isAllOnesValue()) {
                // it is xor %i, -1 --> actually, it is -%i - 1
                return false;
            } else {
                return true;
            }
        } else if (I.getType()->isFloatingPointTy()) {
            return true;
        } else {
            bool res = (opcode == llvm::BinaryOperator::SDiv);
            res |= (opcode == llvm::BinaryOperator::UDiv);
            res |= (opcode == llvm::BinaryOperator::SRem);
            res |= (opcode == llvm::BinaryOperator::URem);
            res |= (opcode == llvm::BinaryOperator::AShr);
            res |= (opcode == llvm::BinaryOperator::LShr);
            res |= (opcode == llvm::BinaryOperator::Shl);
            res |= (opcode == llvm::BinaryOperator::And);
            res |= (opcode == llvm::BinaryOperator::Or);
            res |= (opcode == llvm::BinaryOperator::Xor);
            return res;
        }
    } else if (llvm::isa<llvm::ZExtInst>(I) || llvm::isa<llvm::SExtInst>(I) || llvm::isa<llvm::TruncInst>(I)) {
        return true;
    } else if (llvm::isa<llvm::PHINode>(I)) {
        return false;
    } else if (llvm::isa<llvm::PtrToIntInst>(I)) {
        return true;
    } else if (llvm::isa<llvm::FPToSIInst>(I) || llvm::isa<llvm::FPToUIInst>(I)) {
        return true;
    } else if (llvm::isa<llvm::PointerType>(I.getType())) {
        return true;
    } else {
        return false;
    }
}

/// isLoopInvariantInst - Return true if all operands of this instruction are loop invariant.
bool Hoister::isLoopInvariantInst(llvm::Loop *L, llvm::Instruction &I)
{
    // The instruction is loop invariant if all of its operands are loop-invariant
    for (unsigned int i = 0, e = I.getNumOperands(); i != e; ++i) {
        if (!L->isLoopInvariant(I.getOperand(i))) {
            return false;
        }
    }

    // If we got this far, the instruction is loop invariant!
    return true;
}

/// hoist - When an instruction is found to only use loop invariant operands
/// that is safe to hoist, this instruction is called to do the dirty work.
void Hoister::hoist(llvm::BasicBlock *preheader, llvm::Instruction &I)
{
    // Remove the instruction from its current basic block... but don't delete the
    // instruction.
    I.removeFromParent();

    // Insert the new node in Preheader, before the terminator.
    preheader->getInstList().insert(preheader->getTerminator(), &I);

    changed = true;
}

bool Hoister::inSubLoop(llvm::Loop *L, llvm::BasicBlock *BB)
{
    for (llvm::Loop::iterator I = L->begin(), E = L->end(); I != E; ++I) {
        if ((*I)->contains(BB)) {
            return true;  // A subloop actually contains this block!
        }
    }
    return false;
}

llvm::LoopPass *createHoisterPass()
{
    llvm::initializeAliasAnalysisAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
#if LLVM_VERSION < VERSION(3, 5)
    llvm::initializeDominatorTreePass(*llvm::PassRegistry::getPassRegistry());
#else
    llvm::initializeDominatorTreeWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
#endif
    llvm::initializeLoopInfoPass(*llvm::PassRegistry::getPassRegistry());
    return new Hoister();
}
