// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/MemoryAnalyzer.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>
#if LLVM_VERSION >= VERSION(3, 8)
  #include <llvm/IR/Module.h>
#endif

// C++ includes
#include <iostream>

#if LLVM_VERSION < VERSION(3, 7)
  typedef llvm::AliasAnalysis::AliasResult AliasResult;
  const AliasResult MayAlias = llvm::AliasAnalysis::MayAlias;
  const AliasResult MustAlias = llvm::AliasAnalysis::MustAlias;
  const AliasResult PartialAlias = llvm::AliasAnalysis::PartialAlias;
  const AliasResult NoAlias = llvm::AliasAnalysis::NoAlias;
#else
  typedef llvm::AliasResult AliasResult;
  const AliasResult MayAlias = llvm::MayAlias;
  const AliasResult MustAlias = llvm::MustAlias;
  const AliasResult PartialAlias = llvm::PartialAlias;
  const AliasResult NoAlias = llvm::NoAlias;
#endif

MemoryAnalyzer::MemoryAnalyzer()
  : llvm::FunctionPass(ID),
    m_globals(),
    m_aa(NULL),
    m_map(),
    m_mayZap()
{}

MemoryAnalyzer::~MemoryAnalyzer()
{}

void MemoryAnalyzer::getAnalysisUsage(llvm::AnalysisUsage &AU) const
{
#if LLVM_VERSION < VERSION(3, 8)
    AU.addRequired<llvm::AliasAnalysis>();
#else
    AU.addRequired<llvm::AAResultsWrapperPass>();
#endif
}

bool MemoryAnalyzer::runOnFunction(llvm::Function &function)
{
    m_globals.clear();
    m_map.clear();
    m_mayZap.clear();
    llvm::Module *module = function.getParent();
    for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
        const llvm::Type *globalType = llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0);
        if (llvm::isa<llvm::IntegerType>(globalType)) {
            m_globals.insert(&*global);
        }
    }
#if LLVM_VERSION < VERSION(3, 8)
    m_aa = &getAnalysis<llvm::AliasAnalysis>();
#else
    m_aa = &getAnalysis<llvm::AAResultsWrapperPass>().getAAResults();
#endif
    visit(function);
    return false;
}

void MemoryAnalyzer::visitLoadInst(llvm::LoadInst &I)
{
    std::set<llvm::GlobalVariable*> maySet;
    std::set<llvm::GlobalVariable*> mustSet;
    llvm::Value *loadAddr = I.getPointerOperand();
#if LLVM_VERSION <= VERSION(3, 7)
    uint64_t loadSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(loadAddr->getType())->getContainedType(0));
#else
    uint64_t loadSize = I.getModule()->getDataLayout().getTypeStoreSize(llvm::cast<llvm::PointerType>(loadAddr->getType())->getContainedType(0));
#endif
    for (std::set<llvm::GlobalVariable*>::iterator globali = m_globals.begin(), globale = m_globals.end(); globali != globale; ++globali) {
        llvm::GlobalVariable *global = *globali;
#if LLVM_VERSION <= VERSION(3, 7)
        uint64_t globalSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
#else
        uint64_t globalSize = I.getModule()->getDataLayout().getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
#endif
        AliasResult ar = m_aa->alias(loadAddr, loadSize, global, globalSize);
        switch (ar) {
        case MayAlias:
            maySet.insert(global);
            break;
        case MustAlias:
            mustSet.insert(global);
            break;
        case NoAlias:
        case PartialAlias:
            break;
        default:
            std::cerr << "Unexpected alias analysis result (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(4711);
        }
    }
    m_map.insert(std::make_pair(&I, std::make_pair(maySet, mustSet)));
}

void MemoryAnalyzer::visitStoreInst(llvm::StoreInst &I)
{
    std::set<llvm::GlobalVariable*> maySet;
    std::set<llvm::GlobalVariable*> mustSet;
    llvm::Value *storeAddr = I.getPointerOperand();
#if LLVM_VERSION <= VERSION(3, 7)
    uint64_t storeSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(storeAddr->getType())->getContainedType(0));
#else
    uint64_t storeSize = I.getModule()->getDataLayout().getTypeStoreSize(llvm::cast<llvm::PointerType>(storeAddr->getType())->getContainedType(0));
#endif
    for (std::set<llvm::GlobalVariable*>::iterator globali = m_globals.begin(), globale = m_globals.end(); globali != globale; ++globali) {
        llvm::GlobalVariable *global = *globali;
#if LLVM_VERSION <= VERSION(3, 7)
        uint64_t globalSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
#else
        uint64_t globalSize = I.getModule()->getDataLayout().getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
#endif
        AliasResult ar = m_aa->alias(storeAddr, storeSize, global, globalSize);
        switch (ar) {
        case MayAlias:
            maySet.insert(global);
            break;
        case MustAlias:
        case PartialAlias:
            mustSet.insert(global);
            break;
        case NoAlias:
            break;
        default:
            std::cerr << "Unexpected alias analysis result (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(4711);
        }
    }
    m_mayZap.insert(maySet.begin(), maySet.end());
    m_mayZap.insert(mustSet.begin(), mustSet.end());
    m_map.insert(std::make_pair(&I, std::make_pair(maySet, mustSet)));
}

char MemoryAnalyzer::ID = 0;

MemoryAnalyzer *createMemoryAnalyzerPass()
{
#if LLVM_VERSION < VERSION(3, 8)
    llvm::initializeAliasAnalysisAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
#else
    llvm::initializeAAResultsWrapperPassPass(*llvm::PassRegistry::getPassRegistry());
#endif
    return new MemoryAnalyzer();
}
