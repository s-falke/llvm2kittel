// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <list>
#include <sstream>

Rule::Rule(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c)
  : refCount(0),
    m_lhs(lhs),
    m_rhs(rhs),
    m_c(c)
{}

ref<Rule> Rule::create(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c) {
  return new Rule(lhs, rhs,c);
}

Rule::~Rule()
{}

std::string Rule::toString()
{
    std::ostringstream res;
    res << m_lhs->toString() << " -> " << m_rhs->toString() << " [ " << m_c->toString() << " ]";
    return res.str();
}

std::string Rule::toKittelString()
{
    std::ostringstream res;
    res << m_lhs->toString() << " -> " << m_rhs->toString();
    if (m_c->getCType() != Constraint::CTrue) {
        res << " [ " << m_c->toKittelString() << " ]";
    }
    return res.str();
}

ref<Term> Rule::getLeft()
{
    return m_lhs;
}

ref<Term> Rule::getRight()
{
    return m_rhs;
}

ref<Constraint> Rule::getConstraint()
{
    return m_c;
}

std::set<std::string> *Rule::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    std::set<std::string> *tmp = m_lhs->getVariables();
    res->insert(tmp->begin(), tmp->end());
    tmp = m_rhs->getVariables();
    res->insert(tmp->begin(), tmp->end());
    tmp = m_c->getVariables();
    res->insert(tmp->begin(), tmp->end());
    return res;
}

ref<Rule> Rule::dropArgs(std::set<unsigned int> drop)
{
    return create(m_lhs->dropArgs(drop), m_rhs->dropArgs(drop), m_c);
}
