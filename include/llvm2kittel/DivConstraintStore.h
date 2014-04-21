// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef DIV_CONSTRAINT_STORE
#define DIV_CONSTRAINT_STORE

#include "llvm2kittel/Util/Ref.h"

class Constraint;

struct DivConstraintStore
{
    ref<Constraint> xEQnull;
    ref<Constraint> yEQone;
    ref<Constraint> yEQnegone;
    ref<Constraint> zEQnull;
    ref<Constraint> zEQx;
    ref<Constraint> zEQnegx;
    ref<Constraint> yGTRone;
    ref<Constraint> xGTRnull;
    ref<Constraint> zGEQnull;
    ref<Constraint> zLSSx;
    ref<Constraint> xLSSnull;
    ref<Constraint> zLEQnull;
    ref<Constraint> zGTRx;
    ref<Constraint> yLSSnegone;
    ref<Constraint> zGTRnegx;
    ref<Constraint> zLSSnegx;
};

#endif // DIV_CONSTRAINT_STORE
