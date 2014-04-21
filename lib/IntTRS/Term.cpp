// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Term.h"
#include "llvm2kittel/IntTRS/Polynomial.h"

// C++ includes
#include <sstream>

Term::Term(std::string f, std::list<ref<Polynomial> > args)
  : refCount(0),
    m_f(f),
    m_args(args),
    m_vars()
{}

ref<Term> Term::create(std::string f, std::list<ref<Polynomial> > args)
{
    return new Term(f, args);
}

Term::~Term()
{}

std::string Term::toString()
{
    std::ostringstream res;
    res << m_f << '(';
    for (std::list<ref<Polynomial> >::iterator i = m_args.begin(), e = m_args.end(); i != e; ) {
        ref<Polynomial> tmp = *i;
        res << tmp->toString();
        if (++i != e) {
            res << ", ";
        }
    }
    res << ')';
    return res.str();
}

std::string Term::getFunctionSymbol()
{
    return m_f;
}

std::list<ref<Polynomial> > Term::getArgs()
{
    return m_args;
}

ref<Polynomial> Term::getArg(unsigned int i)
{
    unsigned int c = 0;
    for (std::list<ref<Polynomial> >::iterator it = m_args.begin(), et = m_args.end(); it != et; ++it) {
        if (c == i) {
            return *it;
        }
        ++c;
    }
    return ref<Polynomial>();
}

ref<Term> Term::instantiate(std::map<std::string, ref<Polynomial> > *bindings)
{
    std::list<ref<Polynomial> > newargs;
    for (std::list<ref<Polynomial> >::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
        ref<Polynomial> pol = *i;
        newargs.push_back(pol->instantiate(bindings));
    }
    return create(m_f, newargs);
}

std::set<std::string> *Term::getVariables()
{
    setupVars();
    std::set<std::string> *res = new std::set<std::string>();
    for (std::vector<std::set<std::string>*>::iterator i = m_vars.begin(), e = m_vars.end(); i != e; ++i) {
        std::set<std::string> *tmp = *i;
        res->insert(tmp->begin(), tmp->end());
    }
    return res;
}

std::set<std::string> *Term::getVariables(unsigned int argpos)
{
    setupVars();
    std::set<std::string> *res = new std::set<std::string>();
    std::set<std::string> *tmp = m_vars[argpos];
    res->insert(tmp->begin(), tmp->end());
    return res;
}

void Term::setupVars(void)
{
    if (m_args.size() != m_vars.size()) {
        m_vars.clear();
        for (std::list<ref<Polynomial> >::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
            ref<Polynomial> tmp = *i;
            m_vars.push_back(tmp->getVariables());
        }
    }
}

ref<Term> Term::dropArgs(std::set<unsigned int> drop)
{
    std::list<ref<Polynomial> > newargs;
    unsigned int argc = 0;
    for (std::list<ref<Polynomial> >::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i, ++argc) {
        if (drop.find(argc) == drop.end()) {
            ref<Polynomial> pol = *i;
            newargs.push_back(pol);
        }
    }
    return create(m_f, newargs);
}
