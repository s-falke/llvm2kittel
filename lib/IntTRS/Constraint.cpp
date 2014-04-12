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

Constraint *Constraint::_true;
Constraint *Constraint::_false;

bool Constraint::__init = init();

bool Constraint::init()
{
    Constraint::_true = new True();
    Constraint::_false = new False();
    return true;
}

// base class
bool Constraint::equals(Constraint *c)
{
    if (getCType() != c->getCType()) {
        return false;
    } else {
        return equalsInternal(c);
    }
}

// True
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

Constraint *True::instantiate(std::map<std::string, ref<Polynomial>> *)
{
    return Constraint::_true;
}

Constraint *True::toNNF(bool negate)
{
    if (negate) {
        return Constraint::_false;
    } else {
        return Constraint::_true;
    }
}

Constraint *True::toDNF()
{
    return Constraint::_true;
}

std::list<Constraint*> True::getDualClauses()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

std::list<Constraint*> True::getAtomics()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

Constraint *True::eliminateNeq()
{
    return Constraint::_true;
}

Constraint *True::evaluateTrivialAtoms()
{
    return Constraint::_true;
}

std::set<std::string> *True::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool True::equalsInternal(Constraint*)
{
    return true;
}

// False
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

Constraint *False::instantiate(std::map<std::string, ref<Polynomial>> *)
{
    return Constraint::_false;
}

Constraint *False::toNNF(bool negate)
{
    if (negate) {
        return Constraint::_true;
    } else {
        return Constraint::_false;
    }
}

Constraint *False::toDNF()
{
    return Constraint::_false;
}

std::list<Constraint*> False::getDualClauses()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

std::list<Constraint*> False::getAtomics()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

Constraint *False::eliminateNeq()
{
    return Constraint::_false;
}

Constraint *False::evaluateTrivialAtoms()
{
    return Constraint::_false;
}

std::set<std::string> *False::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool False::equalsInternal(Constraint*)
{
    return true;
}

// Nondef
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

Constraint *Nondef::instantiate(std::map<std::string, ref<Polynomial>> *)
{
    return new Nondef();
}

Constraint *Nondef::toNNF(bool)
{
    return new Nondef();
}

Constraint *Nondef::toDNF()
{
    return new Nondef();
}

std::list<Constraint*> Nondef::getDualClauses()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

std::list<Constraint*> Nondef::getAtomics()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

Constraint *Nondef::eliminateNeq()
{
    return new Nondef();
}

Constraint *Nondef::evaluateTrivialAtoms()
{
    return new Nondef();
}

std::set<std::string> *Nondef::getVariables()
{
    std::set<std::string> *res = new std::set<std::string>();
    return res;
}

bool Nondef::equalsInternal(Constraint *c)
{
    return this == c;
}

// Atom
Atom::Atom(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type)
  : m_lhs(lhs),
    m_rhs(rhs),
    m_type(type)
{}

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

Constraint *Atom::instantiate(std::map<std::string, ref<Polynomial>> *bindings)
{
    return new Atom(m_lhs->instantiate(bindings), m_rhs->instantiate(bindings), m_type);
}

Constraint *Atom::toNNF(bool negate)
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
    return new Atom(m_lhs, m_rhs, newType);
}

Constraint *Atom::toDNF()
{
    return new Atom(m_lhs, m_rhs, m_type);
}

std::list<Constraint*> Atom::getDualClauses()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

std::list<Constraint*> Atom::getAtomics()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

Constraint *Atom::eliminateNeq()
{
    if (m_type == Neq) {
        Atom *lss = new Atom(m_lhs, m_rhs, Lss);
        Atom *gtr = new Atom(m_lhs, m_rhs, Gtr);
        return new Operator(lss, gtr, Operator::Or);
    } else {
        return new Atom(m_lhs, m_rhs, m_type);
    }
}

Constraint *Atom::evaluateTrivialAtoms()
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

bool Atom::equalsInternal(Constraint *c)
{
    Atom *atom = static_cast<Atom*>(c);
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
Negation::Negation(Constraint *c)
  : m_c(c)
{}

Negation::~Negation()
{
    delete m_c;
}

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

Constraint *Negation::instantiate(std::map<std::string, ref<Polynomial>> *bindings)
{
    return new Negation(m_c->instantiate(bindings));
}

Constraint *Negation::toNNF(bool negate)
{
    return m_c->toNNF(!negate);
}

Constraint *Negation::toDNF()
{
    std::cerr << "Internal error in Negation::toDNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(77);
}

std::list<Constraint*> Negation::getDualClauses()
{
    std::list<Constraint*> res;
    res.push_back(this);
    return res;
}

std::list<Constraint*> Negation::getAtomics()
{
    return m_c->getAtomics();
}

Constraint *Negation::eliminateNeq()
{
    return new Negation(m_c->eliminateNeq());
}

Constraint *Negation::evaluateTrivialAtoms()
{
    return new Negation(m_c->evaluateTrivialAtoms());
}

std::set<std::string> *Negation::getVariables()
{
    return m_c->getVariables();
}

bool Negation::equalsInternal(Constraint *c)
{
    Negation *neg = static_cast<Negation*>(c);
    return m_c->equals(neg->m_c);
}

// Operator
Operator::Operator(Constraint *lhs, Constraint *rhs, OType type)
  : m_lhs(lhs),
    m_rhs(rhs),
    m_type(type)
{}

Operator::~Operator()
{
    delete m_lhs;
    delete m_rhs;
}

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

Constraint *Operator::instantiate(std::map<std::string, ref<Polynomial>> *bindings)
{
    return new Operator(m_lhs->instantiate(bindings), m_rhs->instantiate(bindings), m_type);
}

Constraint *Operator::toNNF(bool negate)
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
    return new Operator(m_lhs->toNNF(negate), m_rhs->toNNF(negate), newType);
}

Constraint *Operator::toDNF()
{
    if (m_type == Or) {
        return new Operator(m_lhs->toDNF(), m_rhs->toDNF(), Or);
    } else if (m_type == And) {
        Constraint *lhsdnf = m_lhs->toDNF();
        Constraint *rhsdnf = m_rhs->toDNF();
        std::list<Constraint*> lhsdcs = lhsdnf->getDualClauses();
        std::list<Constraint*> rhsdcs = rhsdnf->getDualClauses();
        std::list<Constraint*> combined;
        for (std::list<Constraint*>::iterator outeri = lhsdcs.begin(), outere = lhsdcs.end(); outeri != outere; ++outeri) {
            Constraint *outerdc = *outeri;
            for (std::list<Constraint*>::iterator inneri = rhsdcs.begin(), innere = rhsdcs.end(); inneri != innere; ++inneri) {
                Constraint *innerdc = *inneri;
                combined.push_back(new Operator(outerdc, innerdc, And));
            }
        }
        Constraint *res = *combined.rbegin();
        combined.erase(--combined.end());
        for (std::list<Constraint*>::reverse_iterator ri = combined.rbegin(), re = combined.rend(); ri != re; ++ri) {
            res = new Operator(*ri, res, Or);
        }
        return res;
    } else {
        std::cerr << "Internal error in Operator::toDNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(2107);
    }
}

std::list<Constraint*> Operator::getDualClauses()
{
    if (m_type == And) {
        std::list<Constraint*> res;
        res.push_back(this);
        return res;
    } else if (m_type == Or) {
        std::list<Constraint*> res;
        std::list<Constraint*> tmp1 = m_lhs->getDualClauses();
        std::list<Constraint*> tmp2 = m_rhs->getDualClauses();
        res.insert(res.begin(), tmp2.begin(), tmp2.end());
        res.insert(res.begin(), tmp1.begin(), tmp1.end());
        return res;
    } else {
        std::cerr << "Internal error in Operator::getDualClauses (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xCAFFEE);
    }
}

std::list<Constraint*> Operator::getAtomics()
{
    std::list<Constraint*> res;
    std::list<Constraint*> tmp1 = m_lhs->getAtomics();
    std::list<Constraint*> tmp2 = m_rhs->getAtomics();
    res.insert(res.begin(), tmp2.begin(), tmp2.end());
    res.insert(res.begin(), tmp1.begin(), tmp1.end());
    return res;
}

Constraint *Operator::eliminateNeq()
{
    return new Operator(m_lhs->eliminateNeq(), m_rhs->eliminateNeq(), m_type);
}

Constraint *Operator::evaluateTrivialAtoms()
{
    return new Operator(m_lhs->evaluateTrivialAtoms(), m_rhs->evaluateTrivialAtoms(), m_type);
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

Constraint *Operator::getLeft()
{
    return m_lhs;
}

Constraint *Operator::getRight()
{
    return m_rhs;
}

bool Operator::equalsInternal(Constraint *c)
{
    Operator *op = static_cast<Operator*>(c);
    return m_type == op->m_type && m_lhs->equals(op->m_lhs) && m_rhs->equals(op->m_rhs);
}
