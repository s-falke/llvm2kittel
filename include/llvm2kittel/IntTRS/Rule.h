// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef RULE_H
#define RULE_H

// C++ includes
#include <set>

class Constraint;
class Term;

class Rule
{

public:
    Rule(Term *lhs, Term *rhs, Constraint *c);
    ~Rule();

    std::string toString();
    std::string toKittelString();

    Term *getLeft();
    Term *getRight();
    Constraint *getConstraint();
    std::set<std::string> *getVariables();

    Rule *dropArgs(std::set<unsigned int> drop);

private:
    Rule(const Rule&);
    Rule &operator=(const Rule&);

    Term *m_lhs;
    Term *m_rhs;
    Constraint *m_c;

};

#endif // RULE_H
