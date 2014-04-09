// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "llvm2kittel/IntTRS/Polynomial.h"

// C++ includes
#include <list>

class Constraint
{

public:
    enum CType {
        CTrue,
        CFalse,
        CNondef,
        CAtom,
        CNegation,
        COperator
    };

    virtual ~Constraint() {}

    virtual CType getCType() = 0;

    virtual std::string toString() = 0;
    virtual std::string toKittelString() = 0; // only if no True, False, Nondef, Negation, Or
    virtual std::string toCIntString() = 0; // only if no True, False, Nondef, Negation, Or

    virtual Constraint *instantiate(std::map<std::string, Polynomial*> *bindings) = 0;

    virtual Constraint *toNNF(bool negate) = 0;
    virtual Constraint *toDNF() = 0; // formula needs to be in NNF!
    virtual std::list<Constraint*> getDualClauses() = 0; // formula needs to be in DNF!

    virtual std::list<Constraint*> getAtomics() = 0;

    virtual Constraint *eliminateNeq() = 0;
    virtual Constraint *evaluateTrivialAtoms() = 0;

    virtual std::set<std::string> *getVariables() = 0;

    virtual bool equals(Constraint *c);

    static Constraint *_true;
    static Constraint *_false;

protected:
    virtual bool equalsInternal(Constraint *c) = 0;

private:
    static bool __init;
    static bool init();

};

// Truth values
class True : public Constraint
{

public:
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

protected:
    bool equalsInternal(Constraint *c);

};

class False : public Constraint
{

public:
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

protected:
    bool equalsInternal(Constraint *c);

};

class Nondef : public Constraint
{

public:
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

protected:
    bool equalsInternal(Constraint *c);

};

// Atoms
class Atom : public Constraint
{

public:
    enum AType {
        Equ,
        Neq,
        Geq,
        Gtr,
        Leq,
        Lss
    };

    Atom(Polynomial *lhs, Polynomial *rhs, AType type);
    ~Atom();

    AType getAType();

    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

    Polynomial *getLeft();
    Polynomial *getRight();

protected:
    bool equalsInternal(Constraint *c);

private:
    Polynomial *m_lhs;
    Polynomial *m_rhs;
    AType m_type;

    std::string typeToString(AType type);
    std::string typeToKittelString(AType type);
    std::string typeToCIntString(AType type);

private:
    Atom(const Atom &);
    Atom &operator=(const Atom &);

};

// Negation
class Negation : public Constraint
{

public:
    Negation(Constraint *c);
    ~Negation();

    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

protected:
    bool equalsInternal(Constraint *c);

private:
    Constraint *m_c;

private:
    Negation(const Negation &);
    Negation &operator=(const Negation &);

};

// Operators
class Operator : public Constraint
{

public:
    enum OType {
        And,
        Or
    };

    Operator(Constraint *lhs, Constraint *rhs, OType type);
    ~Operator();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();

    OType getOType();

    CType getCType();

    Constraint *instantiate(std::map<std::string, Polynomial*> *bindings);

    Constraint *toNNF(bool negate);
    Constraint *toDNF();
    std::list<Constraint*> getDualClauses();

    std::list<Constraint*> getAtomics();

    Constraint *eliminateNeq();
    Constraint *evaluateTrivialAtoms();

    std::set<std::string> *getVariables();

    Constraint *getLeft();
    Constraint *getRight();

protected:
    bool equalsInternal(Constraint *c);

private:
    Constraint *m_lhs;
    Constraint *m_rhs;
    OType m_type;

    std::string typeToString(OType type);

private:
    Operator(const Operator &);
    Operator &operator=(const Operator &);

};

#endif // CONSTRAINT_H
