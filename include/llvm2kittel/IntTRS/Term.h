// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef TERM_H
#define TERM_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class Polynomial;

class Term
{
public:
    unsigned int refCount;

protected:
    Term(std::string f, std::list<ref<Polynomial> > args);

public:
    static ref<Term> create(std::string f, std::list<ref<Polynomial> > args);
    ~Term();

    std::string toString();

    std::string getFunctionSymbol();
    std::list<ref<Polynomial> > getArgs();
    ref<Polynomial> getArg(unsigned int argpos);

    ref<Term> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    void addVariablesToSet(std::set<std::string> &res);

    void addVariablesToSet(unsigned int argpos, std::set<std::string> &res);

    ref<Term> dropArgs(std::set<unsigned int> drop);

    bool equals(ref<Term> term);

private:
    Term(const Term&);
    Term &operator=(const Term&);

    std::string m_f;
    std::list<ref<Polynomial> > m_args;
    std::vector<std::set<std::string> > m_vars;

    void setupVars(void);

};

#endif // TERM_H
