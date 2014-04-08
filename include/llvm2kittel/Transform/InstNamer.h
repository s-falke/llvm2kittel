// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef INST_NAMER_H
#define INST_NAMER_H

#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/Support/InstVisitor.h>
  #include <llvm/InstrTypes.h>
#else
  #include <llvm/InstVisitor.h>
  #include <llvm/IR/InstrTypes.h>
#endif
#include "WARN_ON.h"

#include "WARN_OFF.h"

class InstNamer : public llvm::InstVisitor<InstNamer>
{

#include "WARN_ON.h"

public:
    InstNamer();

    void visitInstruction(llvm::Instruction &I);

private:
    unsigned int m_counter;
    unsigned int m_bbCounter;
    unsigned int m_globalCounter;

    llvm::Function *m_currFun;
    std::string m_currFunName;

    llvm::BasicBlock *m_currBB;

    void nameBasicBlock(llvm::BasicBlock *BB);

    bool m_namedGlobals;

private:
    InstNamer(const InstNamer &);
    InstNamer &operator=(const InstNamer &);

};

#endif // INST_NAMER_H
