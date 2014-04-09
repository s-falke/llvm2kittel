// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Term.h"

// C++ includes
#include <sstream>

Term::Term(std::string f, std::list<Polynomial*> args)
  : m_f(f),
    m_args(args),
    m_vars()
{}

Term::~Term()
{
    for (std::list<Polynomial*>::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
        Polynomial *p = *i;
        delete p;
    }
}

std::string Term::toString()
{
    std::ostringstream res;
    res << m_f << '(';
    for (std::list<Polynomial*>::iterator i = m_args.begin(), e = m_args.end(); i != e; ) {
        Polynomial *tmp = *i;
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

std::list<Polynomial*> Term::getArgs()
{
    return m_args;
}

Polynomial *Term::getArg(unsigned int i)
{
    unsigned int c = 0;
    for (std::list<Polynomial*>::iterator it = m_args.begin(), et = m_args.end(); it != et; ++it) {
        if (c == i) {
            return *it;
        }
        ++c;
    }
    return NULL;
}

Term *Term::instantiate(std::map<std::string, Polynomial*> *bindings)
{
    std::list<Polynomial*> newargs;
    for (std::list<Polynomial*>::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
        Polynomial *pol = *i;
        newargs.push_back(pol->instantiate(bindings));
    }
    return new Term(m_f, newargs);
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
        for (std::list<Polynomial*>::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i) {
            Polynomial *tmp = *i;
            m_vars.push_back(tmp->getVariables());
        }
    }
}

Term *Term::dropArgs(std::set<unsigned int> drop)
{
    std::list<Polynomial*> newargs;
    unsigned int argc = 0;
    for (std::list<Polynomial*>::iterator i = m_args.begin(), e = m_args.end(); i != e; ++i, ++argc) {
        if (drop.find(argc) == drop.end()) {
            Polynomial *pol = *i;
            newargs.push_back(pol);
        }
    }
    return new Term(m_f, newargs);
}
