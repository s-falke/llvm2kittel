// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

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
    Monomial(std::string x);
    ~Monomial();

    unsigned int getPower(std::string x);
    bool empty();

    bool equals(Monomial *mono);

    bool isUnivariateLinear();

    std::string toString();

    Monomial *mult(Monomial *mono);

    std::string getFirst();
    Monomial *lowerFirst();

    std::set<std::string> *getVariables();

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
    Polynomial(std::string x);
    Polynomial(mpz_t c);
    Polynomial(Monomial *mono);
    ~Polynomial();

    void getCoeff(mpz_t res, Monomial *mono);
    void getConst(mpz_t res);

    bool isVar();
    bool isUnivariateLinear();
    bool isConst();
    bool isLinear();

    std::string toString();

    Polynomial *add(Polynomial *poly);
    Polynomial *sub(Polynomial *poly);
    Polynomial *constMult(mpz_t d);
    Polynomial *mult(Polynomial *poly);

    Polynomial *instantiate(std::map<std::string, Polynomial*> *bindings);

    std::set<std::string> *getVariables();

    int normStepsNeeded();

    bool equals(Polynomial *p);
    static mpz_t _null;
    static mpz_t _one;
    static mpz_t _negone;

    static Polynomial *null;
    static Polynomial *one;
    static Polynomial *negone;

    static Polynomial *simax(unsigned int bitwidth);
    static Polynomial *simin_as_ui(unsigned int bitwidth);
    static Polynomial *simin(unsigned int bitwidth);
    static Polynomial *uimax(unsigned int bitwidth);
    static Polynomial *power_of_two(unsigned int power);

private:
    std::list<std::pair<mpz_class, Monomial*> > m_monos;
    mpz_t m_constant;

    static bool __init;
    static bool init();

    static std::map<unsigned int, Polynomial*> m_simax;
    static std::map<unsigned int, Polynomial*> m_simin_as_ui;
    static std::map<unsigned int, Polynomial*> m_simin;
    static std::map<unsigned int, Polynomial*> m_uimax;
    static std::map<unsigned int, Polynomial*> m_power_of_two;

private:
    Polynomial(const Polynomial &);
    Polynomial &operator=(const Polynomial &);

};

#endif // POLYNOMIAL_H
