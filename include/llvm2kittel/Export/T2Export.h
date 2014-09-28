// This file is part of llvm2KITTeL
//
// Copyright 2014 Marc Brockschmidt <marc@marcbrockschmidt.de>
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef T2EXPORT_H
#define T2EXPORT_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>
#include <set>
#include <string>
#include <iostream>

class Rule;

void printT2System(std::list<ref<Rule> > &rules, std::string &startFun, std::ostream &stream);

#endif // T2EXPORT_H
