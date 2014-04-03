// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "Rule.h"

// C++ includes
#include <sstream>

Rule::Rule(Term *lhs, Term *rhs, Constraint *c)
  : m_lhs(lhs),
    m_rhs(rhs),
    m_c(c)
{}

Rule::~Rule()
{
    delete m_lhs;
    delete m_rhs;
    delete m_c;
}

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

Term *Rule::getLeft()
{
    return m_lhs;
}

Term *Rule::getRight()
{
    return m_rhs;
}

Constraint *Rule::getConstraint()
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

Rule *Rule::dropArgs(std::set<unsigned int> drop)
{
    return new Rule(m_lhs->dropArgs(drop), m_rhs->dropArgs(drop), m_c);
}
