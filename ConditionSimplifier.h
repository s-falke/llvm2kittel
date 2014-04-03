// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONDITION_SIMPLIFIER_H
#define CONDITION_SIMPLIFIER_H

#include "Rule.h"

// C++ includes
#include <list>

std::list<Rule*> simplifyConditions(std::list<Rule*> rules);

#endif // CONDITION_SIMPLIFIER_H
