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
#include <string.h>

class CVC4Eliminate : public EliminateClass
{
public:
    virtual bool shouldEliminate(ref<Constraint> c)
    {
        return shouldEliminateInternal(c);
    }

    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out)
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

    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out)
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

    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out)
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

    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out)
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

    virtual bool callSolver(const std::string &filename_in, const std::string &filename_out)
    {
        std::cerr << "Internal error in NoEliminate class (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }
};

EliminateClass::EliminateClass()
{}

EliminateClass::~EliminateClass()
{}

std::pair<std::string, int> EliminateClass::makeTempFile(const char templ[])
{
    std::ostringstream sstr;
    char *tmpdir = getenv("TMPDIR");
    if (tmpdir != NULL) {
        sstr << tmpdir;
        if (tmpdir[strlen(tmpdir) - 1] != '/') {
            sstr << '/';
        }
    } else {
        sstr << "/tmp/";
    }
    sstr << templ;
    std::string filename_string = sstr.str();

    char *filename = new char[filename_string.size() + 1];
    strcpy(filename, filename_string.c_str());
    int fd = mkstemp(filename);
    filename_string = filename;
    delete filename;
    return std::make_pair(filename_string, fd);
}

bool EliminateClass::shouldEliminateInternal(ref<Constraint> c)
{
    // Build SMT query
    std::ostringstream sstr;
    sstr << "(set-logic QF_LIA)\n";
    std::set<std::string> vars;
    c->addVariablesToSet(vars);
    for (std::set<std::string>::iterator vi = vars.begin(), ve = vars.end(); vi != ve; ++vi) {
        sstr << "(declare-fun " << *vi << " () Int)\n";
    }
    sstr << c->toSMTString(/* onlyLinearPart = */ true);
    sstr << "(check-sat)\n";


    // Save SMT query
    std::pair<std::string, int> file_in = makeTempFile("llvm2kittel.in.XXXXXX");
    if (file_in.second == -1) {
        std::cerr << "Could not create temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    ssize_t written = write(file_in.second, sstr.str().c_str(), sstr.str().size());
    close(file_in.second);
    if (written != (ssize_t)sstr.str().size()) {
        unlink(file_in.first.c_str());
        std::cerr << "Could not write temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    // Create output file
    std::pair<std::string, int> file_out = makeTempFile("llvm2kittel.out.XXXXXX");
    if (file_out.second == -1) {
        unlink(file_in.first.c_str());
        std::cerr << "Could not create temporary file (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }
    close(file_out.second);

    if (!callSolver(file_in.first, file_out.first)) {
        unlink(file_in.first.c_str());
        unlink(file_out.first.c_str());
        std::cerr << "Call to external solver failed (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    unlink(file_in.first.c_str());
    std::ifstream result(file_out.first.c_str());

    if (!result.is_open()) {
        unlink(file_out.first.c_str());
        std::cerr << "Could not get solver result (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(277);
    }

    std::string line;
    result.close();
    unlink(file_out.first.c_str());

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
