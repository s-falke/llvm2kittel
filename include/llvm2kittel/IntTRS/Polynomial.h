// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include "llvm2kittel/Util/Ref.h"

// GMP includes
#include <gmpxx.h>

// C++ includes
#include <list>
#include <map>
#include <set>
#include <string>

// Monomials
class Monomial
{
public:
    unsigned int refCount;

protected:
    Monomial(std::string x);

public:
    static ref<Monomial> create(std::string x);
    ~Monomial();

    unsigned int getPower(std::string x);
    bool empty();

    bool equals(ref<Monomial> mono);

    bool isUnivariateLinear();

    std::string toString();
    std::string toSMTString();

    ref<Monomial> mult(ref<Monomial> mono);

    std::string getFirst();
    ref<Monomial> lowerFirst();

    void addVariablesToSet(std::set<std::string> &res);

private:
    std::list<std::pair<std::string, unsigned int> > m_powers;

private:
    Monomial(const Monomial &);
    Monomial &operator=(const Monomial &);

};

// Polynomials
class Polynomial
{
public:
    unsigned int refCount;

protected:
    Polynomial(std::string x);
    Polynomial(mpz_t c);
    Polynomial(ref<Monomial> mono);

public:
    static ref<Polynomial> create(std::string x);
    static ref<Polynomial> create(mpz_t c);
    static ref<Polynomial> create(ref<Monomial> mono);
    ~Polynomial();

    void getCoeff(mpz_t res, ref<Monomial> mono);
    void getConst(mpz_t res);

    bool isVar();
    bool isUnivariateLinear();
    bool isConst();
    bool isLinear();

    std::string toString();
    std::string toSMTString();

    ref<Polynomial> add(ref<Polynomial> poly);
    ref<Polynomial> sub(ref<Polynomial> poly);
    ref<Polynomial> constMult(mpz_t d);
    ref<Polynomial> mult(ref<Polynomial> poly);

    ref<Polynomial> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    void addVariablesToSet(std::set<std::string> &res);

    long int normStepsNeeded();

    bool equals(ref<Polynomial> p);
    static mpz_t _null;
    static mpz_t _one;
    static mpz_t _negone;

    static ref<Polynomial> null;
    static ref<Polynomial> one;
    static ref<Polynomial> negone;

    static ref<Polynomial> simax(unsigned int bitwidth);
    static ref<Polynomial> simin_as_ui(unsigned int bitwidth);
    static ref<Polynomial> simin(unsigned int bitwidth);
    static ref<Polynomial> uimax(unsigned int bitwidth);
    static ref<Polynomial> power_of_two(unsigned int power);

private:
    std::list<std::pair<mpz_class, ref<Monomial> > > m_monos;
    mpz_t m_constant;

    static bool __init;
    static bool init();

    static std::map<unsigned int, ref<Polynomial> > m_simax;
    static std::map<unsigned int, ref<Polynomial> > m_simin_as_ui;
    static std::map<unsigned int, ref<Polynomial> > m_simin;
    static std::map<unsigned int, ref<Polynomial> > m_uimax;
    static std::map<unsigned int, ref<Polynomial> > m_power_of_two;

private:
    Polynomial(const Polynomial &);
    Polynomial &operator=(const Polynomial &);

    std::string constantToSMTString(mpz_class &constant);

};

#endif // POLYNOMIAL_H
