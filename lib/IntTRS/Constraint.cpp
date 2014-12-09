// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/ConstraintEliminator.h"

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

ref<Constraint> True::create()
{
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

std::string True::toSMTString(bool)
{
    return ""; // No need to output (assert true)
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

ref<Constraint> True::toDNF(EliminateClass*)
{
    return Constraint::_true;
}

void True::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

void True::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

ref<Constraint> True::eliminateNeq()
{
    return Constraint::_true;
}

ref<Constraint> True::evaluateTrivialAtoms()
{
    return Constraint::_true;
}

void True::addVariablesToSet(std::set<std::string> &res)
{}

bool True::equalsInternal(ref<Constraint>)
{
    return true;
}

// False
False::False()
{}

ref<Constraint> False::create()
{
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

std::string False::toSMTString(bool)
{
    std::cerr << "Internal error in creation of SMT string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
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

ref<Constraint> False::toDNF(EliminateClass*)
{
    return Constraint::_false;
}

void False::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

void False::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

ref<Constraint> False::eliminateNeq()
{
    return Constraint::_false;
}

ref<Constraint> False::evaluateTrivialAtoms()
{
    return Constraint::_false;
}

void False::addVariablesToSet(std::set<std::string> &res)
{}

bool False::equalsInternal(ref<Constraint>)
{
    return true;
}

// Nondef
Nondef::Nondef()
{}

ref<Constraint> Nondef::create()
{
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

std::string Nondef::toSMTString(bool)
{
    return ""; // Can always chosen to be true; no need to output (assert true)
}

ref<Constraint> Nondef::instantiate(std::map<std::string, ref<Polynomial> > *)
{
    return new Nondef();
}

ref<Constraint> Nondef::toNNF(bool)
{
    return this;
}

ref<Constraint> Nondef::toDNF(EliminateClass*)
{
    return this;
}

void Nondef::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

void Nondef::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

ref<Constraint> Nondef::eliminateNeq()
{
    return this;
}

ref<Constraint> Nondef::evaluateTrivialAtoms()
{
    return this;
}

void Nondef::addVariablesToSet(std::set<std::string> &res)
{}

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

ref<Constraint> Atom::create(ref<Polynomial> lhs, ref<Polynomial> rhs, Atom::AType type)
{
    return evaluateTrivialAtomsInternal(lhs, rhs, type);
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

std::string Atom::typeToSMTString(AType type)
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

std::string Atom::toSMTString(bool onlyLinearPart)
{
    if (!onlyLinearPart || (m_lhs->isLinear() && m_rhs->isLinear())) {
        std::ostringstream res;
        res << "(assert (" << typeToCIntString(m_type) << ' ' << m_lhs->toSMTString() << ' ' << m_rhs->toSMTString() << "))\n";
        return res.str();
    } else {
        return "";
    }
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

ref<Constraint> Atom::toDNF(EliminateClass*)
{
    return this;
}

void Atom::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

void Atom::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
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
    return evaluateTrivialAtomsInternal(m_lhs, m_rhs, m_type);
}

ref<Constraint> Atom::evaluateTrivialAtomsInternal(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type)
{
    if (lhs->isConst() && rhs->isConst()) {
        mpz_t lconst, rconst;
        mpz_init(lconst);
        mpz_init(rconst);
        lhs->getConst(lconst);
        rhs->getConst(rconst);
        bool eval = true;
        if (type == Equ) {
            eval = (mpz_cmp(lconst, rconst) == 0);
        } else if (type == Neq) {
            eval = (mpz_cmp(lconst, rconst) != 0);
        } else if (type == Geq) {
            eval = (mpz_cmp(lconst, rconst) >= 0);
        } else if (type == Gtr) {
            eval = (mpz_cmp(lconst, rconst) > 0);
        } else if (type == Leq) {
            eval = (mpz_cmp(lconst, rconst) <= 0);
        } else if (type == Lss) {
            eval = (mpz_cmp(lconst, rconst) < 0);
        }
        mpz_clear(lconst);
        mpz_clear(rconst);
        if (eval) {
            return Constraint::_true;
        } else {
            return Constraint::_false;
        }
    } else {
        return new Atom(lhs, rhs, type);
    }
}

void Atom::addVariablesToSet(std::set<std::string> &res)
{
    m_lhs->addVariablesToSet(res);
    m_rhs->addVariablesToSet(res);
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
ref<Constraint> Negation::create(ref<Constraint> c)
{
    if (c->getCType() == CTrue) {
        return _false;
    } else if (c->getCType() == CFalse) {
        return _true;
    } else if (c->getCType() == CNondef) {
        return c;
    } else if (c->getCType() == CNegation) {
        ref<Negation> neg = static_cast<Negation*>(c.get());
        return neg->m_c;
    }

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

std::string Negation::toSMTString(bool)
{
    std::cerr << "Internal error in creation of SMT string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
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

ref<Constraint> Negation::toDNF(EliminateClass*)
{
    std::cerr << "Internal error in Negation::toDNF (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
    exit(77);
}

void Negation::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    res.push_back(this);
}

void Negation::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    m_c->addAtomicsToList(res);
}

ref<Constraint> Negation::eliminateNeq()
{
    return create(m_c->eliminateNeq());
}

ref<Constraint> Negation::evaluateTrivialAtoms()
{
    return create(m_c->evaluateTrivialAtoms());
}

void Negation::addVariablesToSet(std::set<std::string> &res)
{
    m_c->addVariablesToSet(res);
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

ref<Constraint> Operator::create(ref<Constraint> lhs, ref<Constraint> rhs, Operator::OType type)
{
    if (type == And) {
        if (lhs->getCType() == CTrue) {
            return rhs;
        } else if (rhs->getCType() == CTrue) {
            return lhs;
        } else if (lhs->getCType() == CFalse) {
            return _false;
        } else if (rhs->getCType() == CFalse) {
            return _false;
        }
    } else if (type == Or) {
        if (lhs->getCType() == CTrue) {
            return _true;
        } else if (rhs->getCType() == CTrue) {
            return _true;
        } else if (lhs->getCType() == CFalse) {
            return rhs;
        } else if (rhs->getCType() == CFalse) {
            return lhs;
        }
    }

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

std::string Operator::toSMTString(bool onlyLinearPart)
{
    if (m_type == Or) {
        std::cerr << "Internal error in creation of SMT string (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(217);
    }
    std::ostringstream res;
    res << m_lhs->toSMTString(onlyLinearPart) << m_rhs->toSMTString(onlyLinearPart);
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

ref<Constraint> Operator::toDNF(EliminateClass *elim)
{
    if (m_type == Or) {
        return create(m_lhs->toDNF(elim), m_rhs->toDNF(elim), Or);
    } else if (m_type == And) {
        ref<Constraint> lhsdnf = m_lhs->toDNF(elim);
        ref<Constraint> rhsdnf = m_rhs->toDNF(elim);
        std::list<ref<Constraint> > lhsdcs;
        lhsdnf->addDualClausesToList(lhsdcs);
        std::list<ref<Constraint> > rhsdcs;
        rhsdnf->addDualClausesToList(rhsdcs);
        std::list<ref<Constraint> > combined;
        for (std::list<ref<Constraint> >::iterator outeri = lhsdcs.begin(), outere = lhsdcs.end(); outeri != outere; ++outeri) {
            ref<Constraint> outerdc = *outeri;
            for (std::list<ref<Constraint> >::iterator inneri = rhsdcs.begin(), innere = rhsdcs.end(); inneri != innere; ++inneri) {
                ref<Constraint> innerdc = *inneri;
                ref<Constraint> c = create(outerdc, innerdc, And);
                if (c->getCType() == Constraint::CFalse || !elim->shouldEliminate(c)) {
                    combined.push_back(c);
                } else {
                    combined.push_back(_false);
                }
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

void Operator::addDualClausesToList(std::list<ref<Constraint> > &res)
{
    if (m_type == And) {
        res.push_back(this);
    } else if (m_type == Or) {
        m_lhs->addDualClausesToList(res);
        m_rhs->addDualClausesToList(res);
    } else {
        std::cerr << "Internal error in Operator::addDualClausesToList (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xCAFFEE);
    }
}

void Operator::addAtomicsToList(std::list<ref<Constraint> > &res)
{
    m_lhs->addAtomicsToList(res);
    m_rhs->addAtomicsToList(res);
}

ref<Constraint> Operator::eliminateNeq()
{
    return create(m_lhs->eliminateNeq(), m_rhs->eliminateNeq(), m_type);
}

ref<Constraint> Operator::evaluateTrivialAtoms()
{
    return create(m_lhs->evaluateTrivialAtoms(), m_rhs->evaluateTrivialAtoms(), m_type);
}

void Operator::addVariablesToSet(std::set<std::string> &res)
{
    m_lhs->addVariablesToSet(res);
    m_rhs->addVariablesToSet(res);
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
