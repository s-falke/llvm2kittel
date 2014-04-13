// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef REM_CONSTRAINT_STORE
#define REM_CONSTRAINT_STORE

#include "llvm2kittel/Util/Ref.h"

class Constraint;

struct RemConstraintStore
{
    ref<Constraint> xEQnull;
    ref<Constraint> zEQnull;
    ref<Constraint> yEQone;
    ref<Constraint> yEQnegone;
    ref<Constraint> yGTRone;
    ref<Constraint> xGTRnull;
    ref<Constraint> zGEQnull;
    ref<Constraint> zLSSy;
    ref<Constraint> xLSSnull;
    ref<Constraint> zLEQnull;
    ref<Constraint> zGTRnegy;
    ref<Constraint> yLSSnegone;
    ref<Constraint> zLSSnegy;
    ref<Constraint> zGTRy;
};

#endif // REM_CONSTRAINT_STORE
