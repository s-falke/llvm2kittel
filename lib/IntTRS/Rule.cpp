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
#include <map>
#include <sstream>

Rule::Rule(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c)
  : refCount(0),
    m_lhs(lhs),
    m_rhs(rhs),
    m_c(c)
{}

ref<Rule> Rule::create(ref<Term> lhs, ref<Term> rhs, ref<Constraint> c)
{
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

void Rule::addVariablesToSet(std::set<std::string> &res)
{
    m_lhs->addVariablesToSet(res);
    m_rhs->addVariablesToSet(res);
    m_c->addVariablesToSet(res);
}

ref<Rule> Rule::dropArgs(std::set<unsigned int> drop)
{
    return create(m_lhs->dropArgs(drop), m_rhs->dropArgs(drop), m_c);
}

bool Rule::equals(ref<Rule> rule)
{
    return (m_lhs->equals(rule->m_lhs) && m_rhs->equals(rule->m_rhs) && m_c->equals(rule->m_c));
}

ref<Rule> Rule::instantiate(std::map<std::string, ref<Polynomial> > *subst)
{
  return create(m_lhs->instantiate(subst), m_rhs->instantiate(subst), m_c->instantiate(subst));
}
