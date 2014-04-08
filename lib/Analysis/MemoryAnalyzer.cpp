// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Analysis/MemoryAnalyzer.h"

// llvm includes
#include <llvm/InitializePasses.h>
#include <llvm/PassRegistry.h>

// C++ includes
#include <iostream>

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
    AU.addRequired<llvm::AliasAnalysis>();
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
            m_globals.insert(global);
        }
    }
    m_aa = &getAnalysis<llvm::AliasAnalysis>();
    visit(function);
    return false;
}

void MemoryAnalyzer::visitLoadInst(llvm::LoadInst &I)
{
    std::set<llvm::GlobalVariable*> maySet;
    std::set<llvm::GlobalVariable*> mustSet;
    llvm::Value *loadAddr = I.getPointerOperand();
    uint64_t loadSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(loadAddr->getType())->getContainedType(0));
    for (std::set<llvm::GlobalVariable*>::iterator globali = m_globals.begin(), globale = m_globals.end(); globali != globale; ++globali) {
        llvm::GlobalVariable *global = *globali;
        uint64_t globalSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
        llvm::AliasAnalysis::AliasResult ar = m_aa->alias(loadAddr, loadSize, global, globalSize);
        switch (ar) {
        case llvm::AliasAnalysis::MayAlias:
            maySet.insert(global);
            break;
        case llvm::AliasAnalysis::MustAlias:
            mustSet.insert(global);
            break;
        case llvm::AliasAnalysis::NoAlias:
        case llvm::AliasAnalysis::PartialAlias:
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
    uint64_t storeSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(storeAddr->getType())->getContainedType(0));
    for (std::set<llvm::GlobalVariable*>::iterator globali = m_globals.begin(), globale = m_globals.end(); globali != globale; ++globali) {
        llvm::GlobalVariable *global = *globali;
        uint64_t globalSize = m_aa->getTypeStoreSize(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0));
        llvm::AliasAnalysis::AliasResult ar = m_aa->alias(storeAddr, storeSize, global, globalSize);
        switch (ar) {
        case llvm::AliasAnalysis::MayAlias:
            maySet.insert(global);
            break;
        case llvm::AliasAnalysis::MustAlias:
        case llvm::AliasAnalysis::PartialAlias:
            mustSet.insert(global);
            break;
        case llvm::AliasAnalysis::NoAlias:
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
    llvm::initializeAliasAnalysisAnalysisGroup(*llvm::PassRegistry::getPassRegistry());
    return new MemoryAnalyzer();
}
