// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Transform/InstNamer.h"

// C++ includes
#include <sstream>

InstNamer::InstNamer()
  : m_counter(0),
    m_bbCounter(0),
    m_globalCounter(0),
    m_currFun(NULL),
    m_currFunName(""),
    m_currBB(NULL),
    m_namedGlobals(false)
{}

static bool filter(char c)
{
    return (c == ' ') || (c == '*') || (c == '+') || (c == '-');
}

static std::string sanitize(std::string s)
{
    std::string res(s);
    res.resize(static_cast<unsigned int>(std::remove_if(res.begin(), res.end(), filter) - res.begin()));
    return res;
}

void InstNamer::visitInstruction(llvm::Instruction &I)
{
    nameBasicBlock(I.getParent());
    if (!I.hasName() && !I.getType()->isVoidTy()) {
        std::ostringstream tmp;
        tmp << m_counter++;
        I.setName(tmp.str());
    }
}

void InstNamer::nameBasicBlock(llvm::BasicBlock *BB)
{
    if (!m_namedGlobals) {
        m_namedGlobals = true;
        llvm::Module *module = BB->getParent()->getParent();
        for (llvm::Module::global_iterator global = module->global_begin(), globale = module->global_end(); global != globale; ++global) {
            if (llvm::isa<llvm::IntegerType>(llvm::cast<llvm::PointerType>(global->getType())->getContainedType(0))) {
                if (!global->hasName()) {
                    std::ostringstream tmp;
                    tmp << "'global" << m_globalCounter++;
                    global->setName(tmp.str());
                } else {
                    std::ostringstream tmp;
                    tmp << "'" << sanitize(global->getName().str());
                    global->setName(tmp.str());
                }
            }
        }
    }
    if (BB == m_currBB) {
        return;
    }
    m_currBB = BB;
    if (BB->getParent() != m_currFun) {
        m_currFun = BB->getParent();
        m_currFunName = sanitize(m_currFun->getName().str());
        unsigned int c = 0;
        for (llvm::Function::arg_iterator i = m_currFun->arg_begin(), e = m_currFun->arg_end(); i != e; ++i) {
            llvm::Argument &arg = *i;
            if (!arg.hasName()) {
                std::ostringstream tmp;
                tmp << "arg" << c++;
                arg.setName(tmp.str());
            }
        }
    }
    if (!BB->hasName()) {
        std::ostringstream tmp;
        tmp << m_currFunName << "_bb" << m_bbCounter++;
        BB->setName(tmp.str());
    } else {
        std::ostringstream tmp;
        tmp << m_currFunName << "_" << sanitize(BB->getName().str());
        BB->setName(tmp.str());
    }
}
