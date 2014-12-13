// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/Util/gmp_kittel.h"

// C++ includes
#include <sstream>
#include <cstdlib>

Monomial::Monomial(std::string x)
  : refCount(0),
    m_powers()
{
    m_powers.push_back(std::make_pair(x, 1));
}

ref<Monomial> Monomial::create(std::string x)
{
    return new Monomial(x);
}

Monomial::~Monomial()
{}

unsigned int Monomial::getPower(std::string x)
{
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
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

bool Monomial::equals(ref<Monomial> mono)
{
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
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
        std::pair<std::string, unsigned int> &tmp = *m_powers.begin();
        return tmp.second == 1;
    }
}

std::string Monomial::toString()
{
    std::ostringstream sstr;
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ) {
        std::pair<std::string, unsigned int> &tmp = *i;
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

std::string Monomial::toSMTString()
{
    std::ostringstream sstr;
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ) {
        std::pair<std::string, unsigned int> &tmp = *i;
        if (m_powers.size() != 1) {
            sstr << "(* ";
        }
        if (tmp.second == 1) {
            sstr << tmp.first;
        } else {
            for (unsigned int c = 1; c < tmp.second; ++c) {
                sstr << "(* " << tmp.first << " ";
            }
            sstr << tmp.first;
            for (unsigned int c = 1; c < tmp.second; ++c) {
                sstr << ")";
            }
        }
        if (++i != e) {
            sstr << ' ';
        }
    }
    for (unsigned int c = 1; c < m_powers.size(); ++c) {
        sstr << ")";
    }
    return sstr.str();
}

ref<Monomial> Monomial::mult(ref<Monomial> mono)
{
    ref<Monomial> res = create("x");
    res->m_powers.clear();
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
        unsigned int otherPower = mono->getPower(tmp.first);
        unsigned int newPower = tmp.second + otherPower;
        if (newPower != 0) {
            res->m_powers.push_back(std::make_pair(tmp.first, newPower));
        }
    }
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = mono->m_powers.begin(), e = mono->m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
        if (getPower(tmp.first) == 0) {
            res->m_powers.push_back(tmp);
        }
    }
    return res;
}

ref<Monomial> Monomial::lowerFirst()
{
    ref<Monomial> res = create("x");
    res->m_powers.clear();
    bool isFirst = true;
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
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

void Monomial::addVariablesToSet(std::set<std::string> &res)
{
    for (std::list<std::pair<std::string, unsigned int> >::iterator i = m_powers.begin(), e = m_powers.end(); i != e; ++i) {
        std::pair<std::string, unsigned int> &tmp = *i;
        if (tmp.second != 0) {
            res.insert(tmp.first);
        }
    }
}

ref<Polynomial> Polynomial::null;
ref<Polynomial> Polynomial::one;
ref<Polynomial> Polynomial::negone;

mpz_t Polynomial::_null;
mpz_t Polynomial::_one;
mpz_t Polynomial::_negone;

std::map<unsigned int, ref<Polynomial> > Polynomial::m_simax;
std::map<unsigned int, ref<Polynomial> > Polynomial::m_simin_as_ui;
std::map<unsigned int, ref<Polynomial> > Polynomial::m_simin;
std::map<unsigned int, ref<Polynomial> > Polynomial::m_uimax;
std::map<unsigned int, ref<Polynomial> > Polynomial::m_power_of_two;

bool Polynomial::__init = Polynomial::init();
bool Polynomial::init()
{
    mpz_init(Polynomial::_null);
    mpz_init_set_si(Polynomial::_one, 1);
    mpz_init_set_si(Polynomial::_negone, -1);
    Polynomial::null = create(_null);
    Polynomial::one = create(_one);
    Polynomial::negone = create(_negone);
    Polynomial::m_simax.clear();
    Polynomial::m_simin_as_ui.clear();
    Polynomial::m_simin.clear();
    Polynomial::m_uimax.clear();
    Polynomial::m_power_of_two.clear();
    return true;
}

ref<Polynomial> Polynomial::simax(unsigned int bitwidth)
{
    std::map<unsigned int, ref<Polynomial> >::iterator found = m_simax.find(bitwidth);
    if (found != m_simax.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simax(tmp, bitwidth);
    ref<Polynomial> pol = create(tmp);
    mpz_clear(tmp);
    m_simax.insert(std::make_pair(bitwidth, pol));
    return pol;
}

ref<Polynomial> Polynomial::simin_as_ui(unsigned int bitwidth)
{
    std::map<unsigned int, ref<Polynomial> >::iterator found = m_simin_as_ui.find(bitwidth);
    if (found != m_simin_as_ui.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simin_as_ui(tmp, bitwidth);
    ref<Polynomial> pol = create(tmp);
    mpz_clear(tmp);
    m_simin_as_ui.insert(std::make_pair(bitwidth, pol));
    return pol;
}

ref<Polynomial> Polynomial::simin(unsigned int bitwidth)
{
    std::map<unsigned int, ref<Polynomial> >::iterator found = m_simin.find(bitwidth);
    if (found != m_simin.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_simin(tmp, bitwidth);
    ref<Polynomial> pol = create(tmp);
    mpz_clear(tmp);
    m_simin.insert(std::make_pair(bitwidth, pol));
    return pol;
}

ref<Polynomial> Polynomial::uimax(unsigned int bitwidth)
{
    std::map<unsigned int, ref<Polynomial> >::iterator found = m_uimax.find(bitwidth);
    if (found != m_uimax.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_uimax(tmp, bitwidth);
    ref<Polynomial> pol = create(tmp);
    mpz_clear(tmp);
    m_uimax.insert(std::make_pair(bitwidth, pol));
    return pol;
}

ref<Polynomial> Polynomial::power_of_two(unsigned int power)
{
    std::map<unsigned int, ref<Polynomial> >::iterator found = m_power_of_two.find(power);
    if (found != m_power_of_two.end()) {
        return found->second;
    }
    mpz_t tmp;
    mpz_init(tmp);
    mpz_kittel_set_power_of_two(tmp, power);
    ref<Polynomial> pol = create(tmp);
    mpz_clear(tmp);
    m_power_of_two.insert(std::make_pair(power, pol));
    return pol;
}

Polynomial::Polynomial(std::string x)
  : refCount(0),
    m_monos()
{
    mpz_init(m_constant);
    m_monos.push_back(std::make_pair(mpz_class(Polynomial::_one), Monomial::create(x)));
}

ref<Polynomial> Polynomial::create(std::string x)
{
    return new Polynomial(x);
}

Polynomial::Polynomial(mpz_t c)
  : refCount(0),
    m_monos()
{
    mpz_init_set(m_constant, c);
}

ref<Polynomial> Polynomial::create(mpz_t c)
{
    return new Polynomial(c);
}

Polynomial::Polynomial(ref<Monomial> mono)
  : refCount(0),
    m_monos()
{
    mpz_init(m_constant);
    m_monos.push_back(std::make_pair(mpz_class(Polynomial::_one), mono));
}

ref<Polynomial> Polynomial::create(ref<Monomial> mono)
{
    return new Polynomial(mono);
}

Polynomial::~Polynomial()
{
    mpz_clear(m_constant);
}

void Polynomial::getCoeff(mpz_t res, ref<Monomial> mono)
{
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
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
        std::pair<mpz_class, ref<Monomial> > &tmp = *m_monos.begin();
        return (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_one) == 0) && (tmp.second->isUnivariateLinear());
    }
}


bool Polynomial::isUnivariateLinear()
{
    if (m_monos.size() != 1) {
        return false;
    } else {
        std::pair<mpz_class, ref<Monomial> > &tmp = *m_monos.begin();
        return tmp.second->isUnivariateLinear();
    }
}

bool Polynomial::isConst()
{
    return m_monos.empty();
}

bool Polynomial::isLinear()
{
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
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
        for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ) {
            std::pair<mpz_class, ref<Monomial> > &tmp = *i;
            if (mpz_cmp(tmp.first.get_mpz_t(), Polynomial::_one) == 0) {
                // Do nothing
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
                std::pair<mpz_class, ref<Monomial> > peek = *i;
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

std::string Polynomial::constantToSMTString(mpz_class &constant)
{
    std::ostringstream sstr;
    if (mpz_cmp(constant.get_mpz_t(), Polynomial::_null) < 0) {
        mpz_t abs;
        mpz_init(abs);
        mpz_abs(abs, constant.get_mpz_t());
        sstr << "(- " << abs << ")";
        mpz_clear(abs);
    } else {
        sstr << constant;
    }
    return sstr.str();
}

std::string Polynomial::toSMTString()
{
    std::ostringstream sstr;
    if (m_monos.empty()) {
        mpz_class constant = mpz_class(m_constant);
        return constantToSMTString(constant);
    } else {
        for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
            std::pair<mpz_class, ref<Monomial> > &tmp = *i;
            sstr << "(+ (* " << constantToSMTString(tmp.first) << " " << tmp.second->toSMTString() << ") ";
        }
        mpz_class constant = mpz_class(m_constant);
        sstr << constantToSMTString(constant);
        for (unsigned int c = 0; c < m_monos.size(); ++c) {
            sstr << ")";
        }
    }
    return sstr.str();
}

ref<Polynomial> Polynomial::add(ref<Polynomial> poly)
{
    mpz_t newConstant;
    mpz_init(newConstant);
    mpz_add(newConstant, m_constant, poly->m_constant);
    ref<Polynomial> res = create(newConstant);
    mpz_clear(newConstant);
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
        mpz_t otherCoeff;
        mpz_init(otherCoeff);
        poly->getCoeff(otherCoeff, tmp.second);
        mpz_t newCoeff;
        mpz_init(newCoeff);
        mpz_add(newCoeff, tmp.first.get_mpz_t(), otherCoeff);
        mpz_clear(otherCoeff);
        if (mpz_cmp(newCoeff, Polynomial::_null) != 0) {
            res->m_monos.push_back(std::make_pair(mpz_class(newCoeff), tmp.second));
        }
        mpz_clear(newCoeff); // The mpz_class constructor does not take ownership
    }
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = poly->m_monos.begin(), e = poly->m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
        mpz_t coeff;
        mpz_init(coeff);
        getCoeff(coeff, tmp.second);
        if (mpz_cmp(coeff, Polynomial::_null) == 0) {
            res->m_monos.push_back(tmp);
        }
        mpz_clear(coeff);
    }
    return res;
}

ref<Polynomial> Polynomial::sub(ref<Polynomial> poly)
{
    return add(poly->constMult(Polynomial::_negone));
}

ref<Polynomial> Polynomial::constMult(mpz_t d)
{
    if (mpz_cmp(d, Polynomial::_null) == 0) {
        return Polynomial::null;
    } else {
        mpz_t newConstant;
        mpz_init(newConstant);
        mpz_mul(newConstant, m_constant, d);
        ref<Polynomial> res = create(newConstant);
        mpz_clear(newConstant);
        for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
            std::pair<mpz_class, ref<Monomial> > &tmp = *i;
            mpz_t newCoeff;
            mpz_init(newCoeff);
            mpz_mul(newCoeff, tmp.first.get_mpz_t(), d);
            res->m_monos.push_back(std::make_pair(mpz_class(newCoeff), tmp.second));
            mpz_clear(newCoeff); // The mpz_class constructor does not take ownership
        }
        return res;
    }
}

ref<Polynomial> Polynomial::mult(ref<Polynomial> poly)
{
    if (poly->isConst()) {
        ref<Polynomial> res = this->constMult(poly->m_constant);
        return res;
    }
    ref<Polynomial> pp = Polynomial::null;
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator oi = m_monos.begin(), oe = m_monos.end(); oi != oe; ++oi) {
        std::pair<mpz_class, ref<Monomial> > &outer = *oi;
        for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator ii = poly->m_monos.begin(), ie = poly->m_monos.end(); ii != ie; ++ii) {
            std::pair<mpz_class, ref<Monomial> > &inner = *ii;
            ref<Polynomial> tmp = create(outer.second->mult(inner.second));
            mpz_t newCoeff;
            mpz_init(newCoeff);
            mpz_mul(newCoeff, outer.first.get_mpz_t(), inner.first.get_mpz_t());
            if (mpz_cmp(newCoeff, Polynomial::_null) != 0) {
                pp = pp->add(tmp->constMult(newCoeff));
            }
            mpz_clear(newCoeff);
        }
    }
    ref<Polynomial> polyZero = create(_null);
    polyZero->m_monos = poly->m_monos;
    ref<Polynomial> cp = polyZero->constMult(m_constant);
    ref<Polynomial> thisZero = create(_null);
    thisZero->m_monos = m_monos;
    ref<Polynomial> pc = thisZero->constMult(poly->m_constant);
    mpz_t newConstant;
    mpz_init(newConstant);
    mpz_mul(newConstant, m_constant, poly->m_constant);
    ref<Polynomial> cc = create(newConstant);
    mpz_clear(newConstant);
    return pp->add(cp->add(pc->add(cc)));
}

ref<Polynomial> Polynomial::instantiate(std::map<std::string, ref<Polynomial> > *bindings)
{
    ref<Polynomial> res = create(m_constant);
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        ref<Polynomial> accu = Polynomial::one;
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
        ref<Monomial> mono = tmp.second;
        while (!mono->empty()) {
            std::string x = mono->getFirst();
            mono = mono->lowerFirst();
            ref<Polynomial> xnew = create(x);
            std::map<std::string, ref<Polynomial> >::iterator found = bindings->find(x);
            if (found != bindings->end()) {
                xnew = found->second;
            }
            accu = accu->mult(xnew);
        }
        res = res->add(accu->constMult(tmp.first.get_mpz_t()));
    }
    return res;
}

void Polynomial::addVariablesToSet(std::set<std::string> &res)
{
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
        tmp.second->addVariablesToSet(res);
    }
}

long int Polynomial::normStepsNeeded()
{
    if (isConst() || isVar()) {
        return 0;
    }
    long int res = 0;
    for (std::list<std::pair<mpz_class, ref<Monomial> > >::iterator i = m_monos.begin(), e = m_monos.end(); i != e; ++i) {
        std::pair<mpz_class, ref<Monomial> > &tmp = *i;
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

bool Polynomial::equals(ref<Polynomial> p)
{
    ref<Polynomial> tmp = sub(p);
    bool res = false;
    if (tmp->isConst()) {
        res = (mpz_cmp(tmp->m_constant, Polynomial::_null) == 0);
    }
    return res;
}
