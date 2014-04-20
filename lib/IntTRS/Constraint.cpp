// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"

// C++ includes
#include <sstream>
#include <iostream>
#include <cstdlib>

ref<Constraint> Constraint::_true;
ref<Constraint> Constraint::_false;

Constraint::Constraint()
  : refCount(0)
{}

bool Constraint::__init = init();

bool Constraint::init()
{
    Constraint::_true = True::create();
    Constraint::_false = False::create();
    return true;
}

// base class
bool Constraint::equals(ref<Constraint> c)
{
    if (getCType() != c->getCType()) {
        return false;
    } else {
        return equalsInternal(c);
    }
}

// True
True::True()
{}

ref<Constraint> True::create() {
    return new True();
}

Constraint::CType True::getCType()
{
    return CTrue;
}

std::string True::toString()
{
    return "TRUE";
}

std::string True::toKittelString()
{
    std::cerr << "Internal error in creation of KITTeL string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

std::string True::toCIntString()
{
    std::cerr << "Internal error in creation of CInt string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

ref<Constraint> True::instantiate(std::map<std::string, ref<Polynomial> > *)
{
    return Constraint::_true;
}

ref<Constraint> True::toNNF(bool negate)
{
    if (negate) {
        return Constraint::_false;
    } else {
        return Constraint::_true;
    }
}

ref<Constraint> True::toDNF()
{
    return Constraint::_true;
}

std::list<ref<Constraint> > True::getDualClauses()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

std::list<ref<Constraint> > True::getAtomics()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

ref<Constraint> True::eliminateNeq()
{
    return Constraint::_true;
}

ref<Constraint> True::evaluateTrivialAtoms()
{
    return Constraint::_true;
}

std::set<std::string> *True::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool True::equalsInternal(ref<Constraint>)
{
    return true;
}

// False
False::False()
{}

ref<Constraint> False::create() {
    return new False();
}

Constraint::CType False::getCType()
{
    return CFalse;
}

std::string False::toString()
{
    return "FALSE";
}

std::string False::toKittelString()
{
    std::cerr << "Internal error in creation of KITTeL string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

std::string False::toCIntString()
{
    std::cerr << "Internal error in creation of CInt string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

ref<Constraint> False::instantiate(std::map<std::string, ref<Polynomial> > *)
{
    return Constraint::_false;
}

ref<Constraint> False::toNNF(bool negate)
{
    if (negate) {
        return Constraint::_true;
    } else {
        return Constraint::_false;
    }
}

ref<Constraint> False::toDNF()
{
    return Constraint::_false;
}

std::list<ref<Constraint> > False::getDualClauses()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

std::list<ref<Constraint> > False::getAtomics()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

ref<Constraint> False::eliminateNeq()
{
    return Constraint::_false;
}

ref<Constraint> False::evaluateTrivialAtoms()
{
    return Constraint::_false;
}

std::set<std::string> *False::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool False::equalsInternal(ref<Constraint>)
{
    return true;
}

// Nondef
Nondef::Nondef()
{}

ref<Constraint> Nondef::create() {
    return new Nondef();
}

Constraint::CType Nondef::getCType()
{
    return CNondef;
}

std::string Nondef::toString()
{
    return "?";
}

std::string Nondef::toKittelString()
{
    std::cerr << "Internal error in creation of KITTeL string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

std::string Nondef::toCIntString()
{
    std::cerr << "Internal error in creation of CInt string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

ref<Constraint> Nondef::instantiate(std::map<std::string, ref<Polynomial> > *)
{
    return new Nondef();
}

ref<Constraint> Nondef::toNNF(bool)
{
    return this;
}

ref<Constraint> Nondef::toDNF()
{
    return this;
}

std::list<ref<Constraint> > Nondef::getDualClauses()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

std::list<ref<Constraint> > Nondef::getAtomics()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

ref<Constraint> Nondef::eliminateNeq()
{
    return this;
}

ref<Constraint> Nondef::evaluateTrivialAtoms()
{
    return this;
}

std::set<std::string> *Nondef::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool Nondef::equalsInternal(ref<Constraint> c)
{
    return this == c.get();
}

// Atom
Atom::Atom(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type)
  : m_lhs(lhs),
    m_rhs(rhs),
    m_type(type)
{}

ref<Constraint> Atom::create(ref<Polynomial> lhs, ref<Polynomial> rhs,
                             Atom::AType type) {
    return new Atom(lhs, rhs, type);
}

Atom::~Atom()
{}

Atom::AType Atom::getAType()
{
    return m_type;
}

Constraint::CType Atom::getCType()
{
    return CAtom;
}

std::string Atom::typeToString(AType type)
{
    if (type == Equ) {
        return "==";
    } else if (type == Neq) {
        return "!=";
    } else if (type == Geq) {
        return ">=";
    } else if (type == Gtr) {
        return ">";
    } else if (type == Leq) {
        return "<=";
    } else if (type == Lss) {
        return "<";
    } else {
        return "D'Oh!";
    }
}

std::string Atom::typeToKittelString(AType type)
{
    if (type == Equ) {
        return "=";
    } else if (type == Geq) {
        return ">=";
    } else if (type == Gtr) {
        return ">";
    } else if (type == Leq) {
        return "<=";
    } else if (type == Lss) {
        return "<";
    } else {
        return "D'Oh!";
    }
}

std::string Atom::typeToCIntString(AType type)
{
    if (type == Equ) {
        return "=";
    } else if (type == Geq) {
        return ">=";
    } else if (type == Gtr) {
        return ">";
    } else if (type == Leq) {
        return "<=";
    } else if (type == Lss) {
        return "<";
    } else {
        return "D'Oh!";
    }
}

std::string Atom::toString()
{
    std::ostringstream res;
    res << m_lhs->toString() << ' ' << typeToString(m_type) << ' ' << m_rhs->toString();
    return res.str();
}

std::string Atom::toKittelString()
{
    std::ostringstream res;
    res << m_lhs->toString() << ' ' << typeToKittelString(m_type) << ' ' << m_rhs->toString();
    return res.str();
}

std::string Atom::toCIntString()
{
    std::ostringstream res;
    res << m_lhs->toString() << ' ' << typeToCIntString(m_type) << ' ' << m_rhs->toString();
    return res.str();
}

ref<Constraint> Atom::instantiate(std::map<std::string, ref<Polynomial> > *bindings)
{
    return create(m_lhs->instantiate(bindings), m_rhs->instantiate(bindings), m_type);
}

ref<Constraint> Atom::toNNF(bool negate)
{
    AType newType = m_type;
    if (negate) {
        if (m_type == Equ) {
            newType = Neq;
        } else if (m_type == Neq) {
            newType = Equ;
        } else if (m_type == Geq) {
            newType = Lss;
        } else if (m_type == Gtr) {
            newType = Leq;
        } else if (m_type == Leq) {
            newType = Gtr;
        } else if (m_type == Lss) {
            newType = Geq;
        }
    }
    return create(m_lhs, m_rhs, newType);
}

ref<Constraint> Atom::toDNF()
{
    return this;
}

std::list<ref<Constraint> > Atom::getDualClauses()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

std::list<ref<Constraint> > Atom::getAtomics()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

ref<Constraint> Atom::eliminateNeq()
{
    if (m_type == Neq) {
        ref<Constraint> lss = create(m_lhs, m_rhs, Lss);
        ref<Constraint> gtr = create(m_lhs, m_rhs, Gtr);
        return Operator::create(lss, gtr, Operator::Or);
    } else {
        return this;
    }
}

ref<Constraint> Atom::evaluateTrivialAtoms()
{
    if (m_lhs->isConst() && m_rhs->isConst()) {
        mpz_t lconst, rconst;
        mpz_init(lconst);
        mpz_init(rconst);
        m_lhs->getConst(lconst);
        m_rhs->getConst(rconst);
        bool eval = true;
        if (m_type == Equ) {
            eval = (mpz_cmp(lconst, rconst) == 0);
        } else if (m_type == Neq) {
            eval = (mpz_cmp(lconst, rconst) != 0);
        } else if (m_type == Geq) {
            eval = (mpz_cmp(lconst, rconst) >= 0);
        } else if (m_type == Gtr) {
            eval = (mpz_cmp(lconst, rconst) > 0);
        } else if (m_type == Leq) {
            eval = (mpz_cmp(lconst, rconst) <= 0);
        } else if (m_type == Lss) {
            eval = (mpz_cmp(lconst, rconst) < 0);
        }
        if (eval) {
            return Constraint::_true;
        } else {
            return Constraint::_false;
        }
    } else {
        return new Atom(m_lhs, m_rhs, m_type);
    }
}

std::set<std::string> *Atom::getVariables()
{
    std::set<std::string> *tmp1 = m_lhs->getVariables();
    std::set<std::string> *tmp2 = m_rhs->getVariables();
    std::set<std::string> *res = new std::set<std::string>();
    res->insert(tmp1->begin(), tmp1->end());
    res->insert(tmp2->begin(), tmp2->end());
    return res;
}

bool Atom::equalsInternal(ref<Constraint> c)
{
    ref<Atom> atom = static_cast<Atom*>(c.get());
    return m_type == atom->m_type && m_lhs->equals(atom->m_lhs) && m_rhs->equals(atom->m_rhs);
}

ref<Polynomial> Atom::getLeft()
{
    return m_lhs;
}

ref<Polynomial> Atom::getRight()
{
    return m_rhs;
}

// Negation
ref<Constraint> Negation::create(ref<Constraint> c) {
    return new Negation(c);
}

Negation::Negation(ref<Constraint> c)
  : m_c(c)
{}

Negation::~Negation()
{}

Constraint::CType Negation::getCType()
{
    return CNegation;
}

std::string Negation::toString()
{
    std::ostringstream res;
    res << "not(" << m_c->toString() << ')';
    return res.str();
}

std::string Negation::toKittelString()
{
    std::cerr << "Internal error in creation of KITTeL string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

std::string Negation::toCIntString()
{
    std::cerr << "Internal error in creation of CInt string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(217);
}

ref<Constraint> Negation::instantiate(std::map<std::string, ref<Polynomial> > *bindings)
{
    return create(m_c->instantiate(bindings));
}

ref<Constraint> Negation::toNNF(bool negate)
{
    return m_c->toNNF(!negate);
}

ref<Constraint> Negation::toDNF()
{
    std::cerr << "Internal error in Negation::toDNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(77);
}

std::list<ref<Constraint> > Negation::getDualClauses()
{
    std::list<ref<Constraint> > res;
    res.push_back(this);
    return res;
}

std::list<ref<Constraint> > Negation::getAtomics()
{
    return m_c->getAtomics();
}

ref<Constraint> Negation::eliminateNeq()
{
    return create(m_c->eliminateNeq());
}

ref<Constraint> Negation::evaluateTrivialAtoms()
{
    return create(m_c->evaluateTrivialAtoms());
}

std::set<std::string> *Negation::getVariables()
{
    return m_c->getVariables();
}

bool Negation::equalsInternal(ref<Constraint> c)
{
    ref<Negation> neg = static_cast<Negation*>(c.get());
    return m_c->equals(neg->m_c);
}

// Operator
Operator::Operator(ref<Constraint> lhs, ref<Constraint> rhs, OType type)
  : m_lhs(lhs),
    m_rhs(rhs),
    m_type(type)
{}

ref<Constraint> Operator::create(ref<Constraint> lhs, ref<Constraint> rhs,
                                 Operator::OType type) {
    return new Operator(lhs, rhs, type);
}

Operator::~Operator()
{}

std::string Operator::typeToString(OType type)
{
    if (type == And) {
        return "/\\";
    } else if (type == Or) {
        return "\\/";
    } else {
        return "D'Oh!";
    }
}

Constraint::CType Operator::getCType()
{
    return COperator;
}

Operator::OType Operator::getOType()
{
    return m_type;
}

std::string Operator::toString()
{
    std::ostringstream res;
    res << '(' << m_lhs->toString() << ") " << typeToString(m_type) << " (" << m_rhs->toString() << ')';
    return res.str();
}

std::string Operator::toKittelString()
{
    if (m_type == Or) {
        std::cerr << "Internal error in creation of KITTeL string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(217);
    }
    std::ostringstream res;
    res << m_lhs->toKittelString() << " /\\ " << m_rhs->toKittelString();
    return res.str();
}

std::string Operator::toCIntString()
{
    if (m_type == Or) {
        std::cerr << "Internal error in creation of CInt string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(217);
    }
    std::ostringstream res;
    res << m_lhs->toCIntString() << " && " << m_rhs->toCIntString();
    return res.str();
}

ref<Constraint> Operator::instantiate(std::map<std::string, ref<Polynomial> > *bindings)
{
    return create(m_lhs->instantiate(bindings), m_rhs->instantiate(bindings), m_type);
}

ref<Constraint> Operator::toNNF(bool negate)
{
    OType newType = m_type;
    if (negate) {
        if (m_type == And) {
            newType = Or;
        } else if (m_type == Or) {
            newType = And;
        } else {
            std::cerr << "Internal error in Operator::toNNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
            exit(42);
        }
    }
    return create(m_lhs->toNNF(negate), m_rhs->toNNF(negate), newType);
}

ref<Constraint> Operator::toDNF()
{
    if (m_type == Or) {
        return create(m_lhs->toDNF(), m_rhs->toDNF(), Or);
    } else if (m_type == And) {
        ref<Constraint> lhsdnf = m_lhs->toDNF();
        ref<Constraint> rhsdnf = m_rhs->toDNF();
        std::list<ref<Constraint> > lhsdcs = lhsdnf->getDualClauses();
        std::list<ref<Constraint> > rhsdcs = rhsdnf->getDualClauses();
        std::list<ref<Constraint> > combined;
        for (std::list<ref<Constraint> >::iterator outeri = lhsdcs.begin(), outere = lhsdcs.end(); outeri != outere; ++outeri) {
            ref<Constraint> outerdc = *outeri;
            for (std::list<ref<Constraint> >::iterator inneri = rhsdcs.begin(), innere = rhsdcs.end(); inneri != innere; ++inneri) {
                ref<Constraint> innerdc = *inneri;
                combined.push_back(create(outerdc, innerdc, And));
            }
        }
        ref<Constraint> res = *combined.rbegin();
        combined.erase(--combined.end());
        for (std::list<ref<Constraint> >::reverse_iterator ri = combined.rbegin(), re = combined.rend(); ri != re; ++ri) {
            res = create(*ri, res, Or);
        }
        return res;
    } else {
        std::cerr << "Internal error in Operator::toDNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(2107);
    }
}

std::list<ref<Constraint> > Operator::getDualClauses()
{
    if (m_type == And) {
        std::list<ref<Constraint> > res;
        res.push_back(this);
        return res;
    } else if (m_type == Or) {
        std::list<ref<Constraint> > res;
        std::list<ref<Constraint> > tmp1 = m_lhs->getDualClauses();
        std::list<ref<Constraint> > tmp2 = m_rhs->getDualClauses();
        res.insert(res.begin(), tmp2.begin(), tmp2.end());
        res.insert(res.begin(), tmp1.begin(), tmp1.end());
        return res;
    } else {
        std::cerr << "Internal error in Operator::getDualClauses (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xCAFFEE);
    }
}

std::list<ref<Constraint> > Operator::getAtomics()
{
    std::list<ref<Constraint> > res;
    std::list<ref<Constraint> > tmp1 = m_lhs->getAtomics();
    std::list<ref<Constraint> > tmp2 = m_rhs->getAtomics();
    res.insert(res.begin(), tmp2.begin(), tmp2.end());
    res.insert(res.begin(), tmp1.begin(), tmp1.end());
    return res;
}

ref<Constraint> Operator::eliminateNeq()
{
    return create(m_lhs->eliminateNeq(), m_rhs->eliminateNeq(), m_type);
}

ref<Constraint> Operator::evaluateTrivialAtoms()
{
    return create(m_lhs->evaluateTrivialAtoms(), m_rhs->evaluateTrivialAtoms(), m_type);
}

std::set<std::string> *Operator::getVariables()
{
    std::set<std::string> *tmp1 = m_lhs->getVariables();
    std::set<std::string> *tmp2 = m_rhs->getVariables();
    std::set<std::string> *res = new std::set<std::string>();
    res->insert(tmp1->begin(), tmp1->end());
    res->insert(tmp2->begin(), tmp2->end());
    return res;
}

ref<Constraint> Operator::getLeft()
{
    return m_lhs;
}

ref<Constraint> Operator::getRight()
{
    return m_rhs;
}

bool Operator::equalsInternal(ref<Constraint> c)
{
    ref<Operator> op = static_cast<Operator*>(c.get());
    return m_type == op->m_type && m_lhs->equals(op->m_lhs) && m_rhs->equals(op->m_rhs);
}
