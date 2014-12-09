// This file is part of llvm2KITTeL
//
// Copyright 2014 Jeroen Ketema
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/ConstraintEliminator.h"
#include "llvm2kittel/IntTRS/Constraint.h"

// C/C++ includes
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>

class CVC4Eliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return shouldEliminateInternal(c);
    }

    virtual bool callSolver(char filename_in[], char filename_out[])
    {
        std::ostringstream sstr;
        sstr << "cvc4 --lang=smt2 < " << filename_in << "> " << filename_out;
        return system(sstr.str().c_str()) == 0;
    }
};

class MathSat5Eliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return shouldEliminateInternal(c);
    }

    virtual bool callSolver(char filename_in[], char filename_out[])
    {
        std::ostringstream sstr;
        sstr << "mathsat < " << filename_in << "> " << filename_out;
        return system(sstr.str().c_str()) == 0;
    }
};

class Yices2Eliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return shouldEliminateInternal(c);
    }

    virtual bool callSolver(char filename_in[], char filename_out[])
    {
        std::ostringstream sstr;
        sstr << "yices-smt2 < " << filename_in << "> " << filename_out;
        return system(sstr.str().c_str()) == 0;
    }
};

class Z3Eliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return shouldEliminateInternal(c);
    }

    virtual bool callSolver(char filename_in[], char filename_out[])
    {
        std::ostringstream sstr;
        sstr << "z3 -smt2 -in < " << filename_in << "> " << filename_out;
        return system(sstr.str().c_str()) == 0;
    }
};

class NoEliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return false;
    }

    virtual bool callSolver(char filename_in[], char filename_out[])
    {
        std::cerr << "Internal error in NoEliminate class (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }
};

EliminateClass::EliminateClass()
{}

EliminateClass::~EliminateClass()
{}

bool EliminateClass::shouldEliminateInternal(ref<Constraint> c)
{
    // Build SMT query
    std::ostringstream sstr;
    sstr << "(set-logic QF_LIA)";
    std::set<std::string> vars;
    c->addVariablesToSet(vars);
    for (std::set<std::string>::iterator vi = vars.begin(), ve = vars.end(); vi != ve; ++vi) {
        sstr << "(declare-fun " << *vi << " () Int)\n";
    }
    sstr << c->toSMTString(/* onlyLinearPart = */ true);
    sstr << "(check-sat)\n";


    // Save SMT query
    char filename_in[] = "/tmp/llvm2kittel.in.XXXXXX";
    int fd_in = mkstemp(filename_in);

    if (fd_in == -1) {
        std::cerr << "Could not create temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    ssize_t written = write(fd_in, sstr.str().c_str(), sstr.str().size());
    close(fd_in);
    if (written != (ssize_t)sstr.str().size()) {
        unlink(filename_in);
        std::cerr << "Could not write temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    // Create output file
    char filename_out[] = "/tmp/llvm2kittel.out.XXXXXX";
    int fd_out = mkstemp(filename_out);
    if (fd_out == -1) {
        unlink(filename_in);
        std::cerr << "Could not create temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }
    close(fd_out);

    if (!callSolver(filename_in, filename_out)) {
        unlink(filename_in);
        unlink(filename_out);
        std::cerr << "Call to external solver failed (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    unlink(filename_in);
    std::ifstream result(filename_out);

    if (!result.is_open()) {
        unlink(filename_out);
        std::cerr << "Could not get solver result (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    std::string line;
    if (!std::getline(result, line)) {
        result.close();
        unlink(filename_out);
        std::cerr << "Could not get solver result (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    result.close();
    unlink(filename_out);

    if (line == "unsat") {
        return true;
    } else {
        return false;
    }
}

EliminateClass *eliminateClassFactory(SMTSolver solver)
{
    switch (solver) {
    case CVC4Solver:
        return new CVC4Eliminate();
    case MathSat5Solver:
        return new MathSat5Eliminate();
    case Yices2Solver:
        return new Yices2Eliminate();
    case Z3Solver:
        return new Z3Eliminate();
    case NoSolver:
        return new NoEliminate();
    default:
        std::cerr << "Internal error in solver factory (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }
}
