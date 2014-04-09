// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef REM_CONSTRAINT_STORE
#define REM_CONSTRAINT_STORE

class Constraint;

struct RemConstraintStore
{
    Constraint *xEQnull;
    Constraint *zEQnull;
    Constraint *yEQone;
    Constraint *yEQnegone;
    Constraint *yGTRone;
    Constraint *xGTRnull;
    Constraint *zGEQnull;
    Constraint *zLSSy;
    Constraint *xLSSnull;
    Constraint *zLEQnull;
    Constraint *zGTRnegy;
    Constraint *yLSSnegone;
    Constraint *zLSSnegy;
    Constraint *zGTRy;
};

#endif // REM_CONSTRAINT_STORE
