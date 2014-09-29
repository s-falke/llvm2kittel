// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef RULE_H
#define RULE_H

#include "llvm2kittel/Util/Ref.h"
// C++ includes
#include <set>
#include <map>
#include <string>

class Constraint;
class Term;
class Polynomial;

class Rule
{
public:
    unsigned int refCount;

protected:
    Rule(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c);

public:
    static ref<Rule> create(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c);
    ~Rule();

    std::string toString();
    std::string toKittelString();

    ref<Term> getLeft();
    ref<Term> getRight();
    ref<Constraint> getConstraint();
    void addVariablesToSet(std::set<std::string> &res);

    ref<Rule> dropArgs(std::set<unsigned int> drop);
    ref<Rule> instantiate(std::map<std::string, ref<Polynomial> > *subst);

    bool equals(ref<Rule> rule);

private:
    Rule(const Rule&);
    Rule &operator=(const Rule&);

    ref<Term> m_lhs;
    ref<Term> m_rhs;
    ref<Constraint> m_c;

};

#endif // RULE_H
