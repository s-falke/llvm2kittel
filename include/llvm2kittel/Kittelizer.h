// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef KITTELIZER_H
#define KITTELIZER_H

#include "llvm2kittel/ConstraintEliminator.h"
#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>

class Rule;

std::list<ref<Rule> > kittelize(std::list<ref<Rule> > rules, SMTSolver solver);

#endif // KITTELIZER_H
