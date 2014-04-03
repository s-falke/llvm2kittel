// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef NONDEF_FACTORY_H
#define NONDEF_FACTORY_H

#include "Version.h"
#include "Macros.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Module.h>
  #include <llvm/DerivedTypes.h>
#else
  #include <llvm/IR/Module.h>
  #include <llvm/IR/DerivedTypes.h>
#endif
#include "WARN_ON.h"

// STL includes
#include <map>

class NondefFactory
{
public:
    NondefFactory(llvm::Module *M)
      : m_module(M), m_nondefCounter(0), m_nondefMap()
    {}

    llvm::Function *getNondefFunction(LLVM2_CONST llvm::Type* type)
    {
        std::map<LLVM2_CONST llvm::Type*, llvm::Function*>::iterator it = m_nondefMap.find(type);
        if (it != m_nondefMap.end()) {
            return it->second;
        }
        LLVM2_CONST llvm::FunctionType *t = llvm::FunctionType::get(type, false);
        llvm::Function *res = llvm::Function::Create(t, llvm::GlobalValue::ExternalLinkage, "__kittel_nondef." + llvm::Twine(m_nondefCounter++), m_module);
        m_nondefMap.insert(std::make_pair(type, res));
        return res;
    }

private:
    NondefFactory(const NondefFactory &);
    NondefFactory &operator=(const NondefFactory &);

    llvm::Module *m_module;
    unsigned int m_nondefCounter;
    std::map<LLVM2_CONST llvm::Type*, llvm::Function*> m_nondefMap;

};

#endif // NONDEF_FACTORY_H
