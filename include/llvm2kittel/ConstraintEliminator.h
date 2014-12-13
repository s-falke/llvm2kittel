// This file is part of llvm2KITTeL
//
// Copyright 2014 Jeroen Ketema
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONSTRAINT_ELIMINATOR_H
#define CONSTRAINT_ELIMINATOR_H

#include "llvm2kittel/Util/Ref.h"

#include <string>
#include <utility>

class Constraint;

enum SMTSolver
{
  CVC4Solver,
  MathSat5Solver,
  Yices2Solver,
  Z3Solver,
  NoSolver
};

class EliminateClass
{
protected:
    EliminateClass();

public:
    virtual ~EliminateClass();

    virtual bool shouldEliminate(ref<Constraint> c) = 0;
    friend EliminateClass *eliminateClassFactory(SMTSolver solver);

protected:
    std::pair<std::string, int> makeTempFile(const char templ[]);
    char *makeTempTemplate(const char str[]);
    bool shouldEliminateInternal(ref<Constraint> c);
    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out) = 0;
};

EliminateClass *eliminateClassFactory(SMTSolver solver);

#endif // CONSTRAINT_ELIMINATOR_H
