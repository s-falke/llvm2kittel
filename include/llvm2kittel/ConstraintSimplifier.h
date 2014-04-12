// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONSTRAINT_SIMPLIFIER_H
#define CONSTRAINT_SIMPLIFIER_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>

class Rule;

std::list<ref<Rule>> simplifyConstraints(std::list<ref<Rule>> rules);

#endif // CONSTRAINT_SIMPLIFIER_H
