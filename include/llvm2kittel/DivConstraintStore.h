// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef DIV_CONSTRAINT_STORE
#define DIV_CONSTRAINT_STORE

class Constraint;

struct DivConstraintStore
{
    Constraint *xEQnull;
    Constraint *yEQone;
    Constraint *yEQnegone;
    Constraint *zEQnull;
    Constraint *zEQx;
    Constraint *zEQnegx;
    Constraint *yGTRone;
    Constraint *xGTRnull;
    Constraint *zGEQnull;
    Constraint *zLSSx;
    Constraint *xLSSnull;
    Constraint *zLEQnull;
    Constraint *zGTRx;
    Constraint *yLSSnegone;
    Constraint *zGTRnegx;
    Constraint *zLSSnegx;
};

#endif // DIV_CONSTRAINT_STORE
