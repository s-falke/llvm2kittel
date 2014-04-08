// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef TERM_H
#define TERM_H

#include "Polynomial.h"

// C++ includes
#include <vector>

class Term
{

public:
    Term(std::string f, std::list<Polynomial*> args);
    ~Term();

    std::string toString();

    std::string getFunctionSymbol();
    std::list<Polynomial*> getArgs();
    Polynomial *getArg(unsigned int argpos);

    Term *instantiate(std::map<std::string, Polynomial*> *bindings);

    std::set<std::string> *getVariables();

    std::set<std::string> *getVariables(unsigned int argpos);

    Term *dropArgs(std::set<unsigned int> drop);

private:
    Term(const Term&);
    Term &operator=(const Term&);

    std::string m_f;
    std::list<Polynomial*> m_args;
    std::vector<std::set<std::string>* > m_vars;

    void setupVars(void);

};

#endif // TERM_H
