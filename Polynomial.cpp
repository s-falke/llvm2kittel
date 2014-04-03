// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "Polynomial.h"
#include "gmp_kittel.h"

// C++ includes
#include <sstream>
#include <cstdlib>

Monomial::Monomial(std::string x)
  : m_powers()
{
    m_powers.push_back(std::make_pair(x, 1));
}

Monomial::~Monomial()
{}

unsigned int Monomial::getPower(std::string x)
{
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (tmp.first == x) {
            return tmp.second;
        }
    }
    return 0;
}

bool Monomial::empty()
{
    return m_powers.empty();
}

bool Monomial::equals(Monomial *mono)
{
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (mono->getPower(tmp.first) != tmp.second) {
            return false;
        }
    }
    return true;
}

bool Monomial::isUnivariateLinear()
{
    if (m_powers.size() != 1) {
        return false;
    } else {
        std::pair<std::string, unsigned int> tmp = *m_powers.begin();
        return tmp.second == 1;
    }
}

std::string Monomial::toString()
{
    std::ostringstream sstr;
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (tmp.second == 1) {
            sstr << tmp.first;
        } else {
            sstr << tmp.first << '^' << tmp.second;
        }
        if (++i != e) {
            sstr << '*';
        }
    }
    return sstr.str();
}

Monomial *Monomial::mult(Monomial *mono)
{
    Monomial *res = new Monomial("x");
    res->m_powers.clear();
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        unsigned int otherPower = mono->getPower(tmp.first);
        unsigned int newPower = tmp.second + otherPower;
        if (newPower != 0) {
            res->m_powers.push_back(std::make_pair(tmp.first, newPower));
        }
    }
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = mono->m_powers.begin(), e = mono->m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (getPower(tmp.first) == 0) {
            res->m_powers.push_back(tmp);
        }
    }
    return res;
}

Monomial *Monomial::lowerFirst()
{
    Monomial *res = new Monomial("x");
    res->m_powers.clear();
    bool isFirst = true;
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (isFirst) {
            if (tmp.second != 1) {
                res->m_powers.push_back(std::make_pair(tmp.first, tmp.second - 1));
            }
            isFirst = false;
        } else {
            res->m_powers.push_back(tmp);
        }
    }
    return res;
}

std::string Monomial::getFirst()
{
    return m_powers.begin()->first;
}

std::set<std::string> *Monomial::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> tmp = *i;
        if (tmp.second != 0) {
            res->insert(tmp.first);
        }
    }
    return res;
}

Polynomial *Polynomial::null;
Polynomial *Polynomial::one;
Polynomial *Polynomial::negone;

mpz_t Polynomial::_null;
mpz_t Polynomial::_one;
mpz_t Polynomial::_negone;

std::map<unsigned int, Polynomial*> Polynomial::m_simax;
std::map<unsigned int, Polynomial*> Polynomial::m_simin_as_ui;
std::map<unsigned int, Polynomial*> Polynomial::m_simin;
std::map<unsigned int, Polynomial*> Polynomial::m_uimax;
std::map<unsigned int, Polynomial*> Polynomial::m_power_of_two;

bool Polynomial::__init = Polynomial::init();
bool Polynomial::init()
{
    mpz_init(Polynomial::_null);
    mpz_init_set_si(Polynomial::_one, 1);
    mpz_init_set_si(Polynomial::_negone, -1);
    Polynomial::null = new Polynomial(_null);
    Polynomial::one = new Polynomial(_one);
    Polynomial::negone = new Polynomial(_negone);
    Polynomial::m_simax.clear();
    Polynomial::m_simin_as_ui.clear();
    Polynomial::m_simin.clear();
    Polynomial::m_uimax.clear();
    Polynomial::m_power_of_two.clear();
    return true;
}

Polynomial *Polynomial::simax(unsigned int bitwidth)
{
    std::map<unsigned int, Polynomial*>::iterator found = Polynomial::m_simax.find(bitwidth);
    if (found != Polynomial::m_simax.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simax(tmp, bitwidth);
    Polynomial *pol = new Polynomial(tmp);
    mpz_clear(tmp);
    m_simax.insert(std::make_pair(bitwidth, pol));
    return pol;
}

Polynomial *Polynomial::simin_as_ui(unsigned int bitwidth)
{
    std::map<unsigned int, Polynomial*>::iterator found = Polynomial::m_simin_as_ui.find(bitwidth);
    if (found != Polynomial::m_simin_as_ui.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simin_as_ui(tmp, bitwidth);
    Polynomial *pol = new Polynomial(tmp);
    mpz_clear(tmp);
    m_simin_as_ui.insert(std::make_pair(bitwidth, pol));
    return pol;
}

Polynomial *Polynomial::simin(unsigned int bitwidth)
{
    std::map<unsigned int, Polynomial*>::iterator found = Polynomial::m_simin.find(bitwidth);
    if (found != Polynomial::m_simin.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simin(tmp, bitwidth);
    Polynomial *pol = new Polynomial(tmp);
    mpz_clear(tmp);
    m_simin.insert(std::make_pair(bitwidth, pol));
    return pol;
}

Polynomial *Polynomial::uimax(unsigned int bitwidth)
{
    std::map<unsigned int, Polynomial*>::iterator found = Polynomial::m_uimax.find(bitwidth);
    if (found != Polynomial::m_uimax.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_uimax(tmp, bitwidth);
    Polynomial *pol = new Polynomial(tmp);
    mpz_clear(tmp);
    m_uimax.insert(std::make_pair(bitwidth, pol));
    return pol;
}

Polynomial *Polynomial::power_of_two(unsigned int power)
{
    std::map<unsigned int, Polynomial*>::iterator found = Polynomial::m_power_of_two.find(power);
    if (found != Polynomial::m_power_of_two.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_power_of_two(tmp, power);
    Polynomial *pol = new Polynomial(tmp);
    mpz_clear(tmp);
    m_power_of_two.insert(std::make_pair(power, pol));
    return pol;
}

Polynomial::Polynomial(std::string x)
  : m_monos()
{
    mpz_init(m_constant);
    m_monos.push_back(std::make_pair(Polynomial::_one, new Monomial(x)));
}

Polynomial::Polynomial(mpz_t c)
  : m_monos()
{
    mpz_init(m_constant);
    mpz_set(m_constant, c);
}

Polynomial::Polynomial(Monomial *mono)
  : m_monos()
{
    mpz_init(m_constant);
    m_monos.push_back(std::make_pair(Polynomial::_one, mono));
}

Polynomial::~Polynomial()
{
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        delete i->second;
    }
}

void Polynomial::getCoeff(mpz_t res, Monomial *mono)
{
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        if (mono->equals(tmp.second)) {
            mpz_set(res, tmp.first.get_mpz_t());
        }
    }
    return;
}

void Polynomial::getConst(mpz_t res)
{
    mpz_set(res, m_constant);
}

bool Polynomial::isVar()
{
    if (mpz_cmp(m_constant, Polynomial::_null) != 0) {
        return false;
    } else if (m_monos.size() != 1) {
        return false;
    } else {
        std::pair<mpz_class, Monomial*> tmp = *m_monos.begin();
        return (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_one) == 0) && (tmp.second->isUnivariateLinear());
    }
}


bool Polynomial::isUnivariateLinear()
{
    if (m_monos.size() != 1) {
        return false;
    } else {
        std::pair<mpz_class, Monomial*> tmp = *m_monos.begin();
        return tmp.second->isUnivariateLinear();
    }
}

bool Polynomial::isConst()
{
    return m_monos.empty();
}

bool Polynomial::isLinear()
{
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        if (!tmp.second->isUnivariateLinear()) {
            return false;
        }
    }
    return true;
}

std::string Polynomial::toString()
{
    std::ostringstream sstr;
    if (m_monos.empty()) {
        sstr << m_constant;
    } else {
        bool isFirst = true;
        for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ) {
            std::pair<mpz_class, Monomial*> tmp = *i;
            if (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_one) == 0) {
            } else if (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_negone) == 0) {
                if (isFirst) {
                    sstr << '-';
                }
            } else {
                if (isFirst) {
                    if (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_null) < 0) {
                        sstr << '-';
                    }
                }
                mpz_t abs;
                mpz_init(abs);
                mpz_abs(abs, tmp.first.get_mpz_t());
                sstr << abs << '*';
                mpz_clear(abs);
            }
            sstr << tmp.second->toString();
            isFirst = false;
            if (++i != e) {
                std::pair<mpz_class, Monomial*> peek = *i;
                if (mpz_cmp(peek.first.get_mpz_t(), Polynomial::_null) < 0) {
                    sstr << " - ";
                } else {
                    sstr << " + ";
                }
            }
        }
        if (mpz_cmp(m_constant, Polynomial::_null) < 0) {
            mpz_t abs;
            mpz_init(abs);
            mpz_abs(abs, m_constant);
            sstr << " - " << abs;
            mpz_clear(abs);
        } else if (mpz_cmp(m_constant, Polynomial::_null) > 0) {
            sstr << " + " << m_constant;
        }
    }
    return sstr.str();
}

Polynomial *Polynomial::add(Polynomial *poly)
{
    mpz_t newConstant;
    mpz_t polyConstant;
    mpz_init_set(newConstant, m_constant);
    mpz_init(polyConstant);
    poly->getConst(polyConstant);
    mpz_add(newConstant, newConstant, polyConstant);
    Polynomial *res = new Polynomial(newConstant);
    mpz_clear(polyConstant);
    mpz_clear(newConstant);
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        mpz_t otherCoeff;
        mpz_init(otherCoeff);
        poly->getCoeff(otherCoeff, tmp.second);
        mpz_t newCoeff;
        mpz_init(newCoeff);
        mpz_add(newCoeff, tmp.first.get_mpz_t(), otherCoeff);
        if (mpz_cmp(newCoeff, Polynomial::_null) != 0) {
            res->m_monos.push_back(std::make_pair(newCoeff, tmp.second));
        }
    }
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = poly->m_monos.begin(), e = poly->m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        mpz_t coeff;
        mpz_init(coeff);
        getCoeff(coeff, tmp.second);
        if (mpz_cmp(coeff, Polynomial::_null) == 0) {
            res->m_monos.push_back(tmp);
        }
    }
    return res;
}

Polynomial *Polynomial::sub(Polynomial *poly)
{
    return add(poly->constMult(Polynomial::_negone));
}

Polynomial *Polynomial::constMult(mpz_t d)
{
    if (mpz_cmp(d, Polynomial::_null) == 0) {
        return Polynomial::null;
    } else {
        mpz_t newConstant;
        mpz_init_set(newConstant, m_constant);
        mpz_mul(newConstant, newConstant, d);
        Polynomial *res = new Polynomial(newConstant);
        mpz_clear(newConstant);
        for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
            std::pair<mpz_class, Monomial*> tmp = *i;
            mpz_t newCoeff;
            mpz_init_set(newCoeff, tmp.first.get_mpz_t());
            mpz_mul(newCoeff, newCoeff, d);
            res->m_monos.push_back(std::make_pair(newCoeff, tmp.second));
        }
        return res;
    }
}

Polynomial *Polynomial::mult(Polynomial *poly)
{
    if (poly->isConst()) {
        mpz_t constant;
        mpz_init(constant);
        poly->getConst(constant);
        Polynomial *res = this->constMult(constant);
        mpz_clear(constant);
        return res;
    }
    Polynomial *pp = Polynomial::null;
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator oi = m_monos.begin(), oe = m_monos.end(); oi != oe; ++oi) {
        std::pair<mpz_class, Monomial*> outer = *oi;
        for (std::list<std::pair<mpz_class, Monomial*> >::iterator ii = poly->m_monos.begin(), ie = poly->m_monos.end(); ii != ie; ++ii) {
            std::pair<mpz_class, Monomial*> inner = *ii;
            Polynomial *tmp = new Polynomial(outer.second->mult(inner.second));
            mpz_t newCoeff;
            mpz_init_set(newCoeff, outer.first.get_mpz_t());
            mpz_mul(newCoeff, newCoeff, inner.first.get_mpz_t());
            if (mpz_cmp(newCoeff, Polynomial::_null) != 0) {
                pp = pp->add(tmp->constMult(newCoeff));
            }
            mpz_clear(newCoeff);
        }
    }
    Polynomial *polyZero = new Polynomial(_null);
    polyZero->m_monos = poly->m_monos;
    Polynomial *cp = polyZero->constMult(m_constant);
    Polynomial *thisZero = new Polynomial(_null);
    thisZero->m_monos = m_monos;
    mpz_t polyConstant;
    mpz_init(polyConstant);
    poly->getConst(polyConstant);
    Polynomial *pc = thisZero->constMult(polyConstant);
    mpz_t newConstant;
    mpz_init_set(newConstant, m_constant);
    mpz_mul(newConstant, newConstant, polyConstant);
    Polynomial *cc = new Polynomial(newConstant);
    mpz_clear(polyConstant);
    mpz_clear(newConstant);
    return pp->add(cp->add(pc->add(cc)));
}

Polynomial *Polynomial::instantiate(std::map<std::string, Polynomial*> *bindings)
{
    mpz_t oldConstant;
    mpz_init_set(oldConstant, m_constant);
    Polynomial *res = new Polynomial(oldConstant);
    mpz_clear(oldConstant);
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        Polynomial *accu = Polynomial::one;
        std::pair<mpz_class, Monomial*> tmp = *i;
        Monomial *mono = tmp.second;
        while (!mono->empty()) {
            std::string x = mono->getFirst();
            mono = mono->lowerFirst();
            Polynomial *xnew = new Polynomial(x);
            std::map<std::string, Polynomial*>::iterator found = bindings->find(x);
            if (found != bindings->end()) {
                xnew = found->second;
            }
            accu = accu->mult(xnew);
        }
        res = res->add(accu->constMult(tmp.first.get_mpz_t()));
    }
    return res;
}

std::set<std::string> *Polynomial::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        std::set<std::string> *mvars = tmp.second->getVariables();
        res->insert(mvars->begin(), mvars->end());
    }
    return res;
}

int Polynomial::normStepsNeeded()
{
    if (isConst() || isVar()) {
        return 0;
    }
    int res = 0;
    for (std::list<std::pair<mpz_class, Monomial*> >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, Monomial*> tmp = *i;
        if (!tmp.second->isUnivariateLinear()) {
            // multiplication...
            return -1;
        }
        if (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_null) >= 0) {
            if (mpz_fits_sint_p(tmp.first.get_mpz_t())) {
                res += mpz_get_si(tmp.first.get_mpz_t());
            } else {
                // too big...
                return -1;
            }
        } else {
            if (mpz_fits_sint_p(tmp.first.get_mpz_t())) {
                res -= mpz_get_si(tmp.first.get_mpz_t());
                // negative numbers need one step more due to non-symmetric range
                ++res;
            } else {
                // too big...
                return -1;
            }
        }
    }
    if (mpz_cmp(m_constant, Polynomial::_null) != 0) {
        ++res;
    }
    return res - 1;
}

bool Polynomial::equals(Polynomial *p)
{
    Polynomial *tmp = sub(p);
    bool res = false;
    if (tmp->isConst()) {
        mpz_t constant;
        mpz_init(constant);
        tmp->getConst(constant);
        res = (mpz_cmp(constant, Polynomial::_null) == 0);
        mpz_clear(constant);
    }
    delete tmp;
    return res;
}
