// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef UNIFORM_COMPLEXITY_TUPLE_PRINTER_H
#define UNIFORM_COMPLEXITY_TUPLE_PRINTER_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>
#include <set>
#include <string>
#include <iostream>

class Rule;

void printUniformComplexityTuples(std::list<ref<Rule> > &rules, std::set<std::string> &complexityLHSs, std::string &startFun, std::ostream &stream);

#endif // UNIFORM_COMPLEXITY_TUPLE_PRINTER_H
