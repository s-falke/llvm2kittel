// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef BOUND_CONDITIONER_H
#define BOUND_CONDITIONER_H

// C++ includes
#include <list>
#include <map>
#include <string>

class Rule;

std::list<Rule*> addBoundConditions(std::list<Rule*> rules, std::map<std::string, unsigned int> bitwidthMap, bool unsignedEncoding);

#endif // BOUND_CONDITIONER_H
