// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "llvm2kittel/Util/Ref.h"

// C++ includes
#include <list>
#include <map>
#include <set>
#include <string>

class Polynomial;
class EliminateClass;

class Constraint
{
public:
    unsigned int refCount;

protected:
    Constraint();

public:
    enum CType {
        CTrue,
        CFalse,
        CNondef,
        CAtom,
        CNegation,
        COperator
    };

    virtual ~Constraint()
    {}

    virtual CType getCType() = 0;

    virtual std::string toString() = 0;
    virtual std::string toKittelString() = 0; // only if no True, False, Nondef, Negation, Or
    virtual std::string toCIntString() = 0; // only if no True, False, Nondef, Negation, Or
    virtual std::string toSMTString(bool onlyLinearPart) = 0; // only if no False, Negation, Or

    virtual ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings) = 0;

    virtual ref<Constraint> toNNF(bool negate) = 0;
    virtual ref<Constraint> toDNF(EliminateClass *elim) = 0; // formula needs to be in NNF!
    virtual void addDualClausesToList(std::list<ref<Constraint> > &res) = 0; // formula needs to be in DNF!

    virtual void addAtomicsToList(std::list<ref<Constraint> > &res) = 0;

    virtual ref<Constraint> eliminateNeq() = 0;
    virtual ref<Constraint> evaluateTrivialAtoms() = 0;
    virtual ref<Constraint> setAtomToTrue(ref<Constraint> c) = 0;

    virtual void addVariablesToSet(std::set<std::string> &res) = 0;

    virtual bool equals(ref<Constraint> c);

    static ref<Constraint> _true;
    static ref<Constraint> _false;

protected:
    virtual bool equalsInternal(ref<Constraint> c) = 0;

private:
    static bool __init;
    static bool init();

};

// Truth values
class True : public Constraint
{
protected:
    True();

public:
    static ref<Constraint> create();
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

protected:
    bool equalsInternal(ref<Constraint> c);

};

class False : public Constraint
{
protected:
    False();

public:
    static ref<Constraint> create();
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

protected:
    bool equalsInternal(ref<Constraint> c);

};

class Nondef : public Constraint
{
protected:
    Nondef();

public:
    static ref<Constraint> create();
    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

protected:
    bool equalsInternal(ref<Constraint> c);

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

protected:
    Atom(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type);

public:
    static ref<Constraint> create(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type);
    ~Atom();

    AType getAType();

    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

    ref<Polynomial> getLeft();
    ref<Polynomial> getRight();

protected:
    bool equalsInternal(ref<Constraint> c);
    static ref<Constraint> evaluateTrivialAtomsInternal(ref<Polynomial> lhs, ref<Polynomial> rhs, AType type);

private:
    ref<Polynomial> m_lhs;
    ref<Polynomial> m_rhs;
    AType m_type;

    std::string typeToString(AType type);
    std::string typeToKittelString(AType type);
    std::string typeToCIntString(AType type);
    std::string typeToSMTString(AType type);

private:
    Atom(const Atom &);
    Atom &operator=(const Atom &);

};

// Negation
class Negation : public Constraint
{
protected:
    Negation(ref<Constraint> c);

public:
    static ref<Constraint> create(ref<Constraint> c);
    ~Negation();

    CType getCType();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

protected:
    bool equalsInternal(ref<Constraint> c);

private:
    ref<Constraint> m_c;

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

protected:
    Operator(ref<Constraint> lhs, ref<Constraint> rhs, OType type);

public:
    static ref<Constraint> create(ref<Constraint> lhs, ref<Constraint> rhs, OType type);
    ~Operator();

    std::string toString();
    std::string toKittelString();
    std::string toCIntString();
    std::string toSMTString(bool onlyLinearPart);

    OType getOType();

    CType getCType();

    ref<Constraint> instantiate(std::map<std::string, ref<Polynomial> > *bindings);

    ref<Constraint> toNNF(bool negate);
    ref<Constraint> toDNF(EliminateClass *elim);
    void addDualClausesToList(std::list<ref<Constraint> > &res);

    void addAtomicsToList(std::list<ref<Constraint> > &res);

    ref<Constraint> eliminateNeq();
    ref<Constraint> evaluateTrivialAtoms();
    ref<Constraint> setAtomToTrue(ref<Constraint> c);

    void addVariablesToSet(std::set<std::string> &res);

    ref<Constraint> getLeft();
    ref<Constraint> getRight();

protected:
    bool equalsInternal(ref<Constraint> c);

private:
    ref<Constraint> m_lhs;
    ref<Constraint> m_rhs;
    OType m_type;

    std::string typeToString(OType type);

private:
    Operator(const Operator &);
    Operator &operator=(const Operator &);

};

#endif // CONSTRAINT_H
