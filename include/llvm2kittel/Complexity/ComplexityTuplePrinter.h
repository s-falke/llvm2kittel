// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef COMPLEXITY_TUPLE_PRINTER_H
#define COMPLEXITY_TUPLE_PRINTER_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>
#include <set>
#include <string>
#include <iostream>

class Rule;

void printComplexityTuples(std::list<ref<Rule> > &rules, std::set<std::string> &complexityLHSs, std::ostream &stream);

#endif // COMPLEXITY_TUPLE_PRINTER_H
