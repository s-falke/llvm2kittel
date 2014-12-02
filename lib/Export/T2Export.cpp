// This file is part of llvm2KITTeL
//
// Copyright 2014 Marc Brockschmidt <marc@marcbrockschmidt.de>
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/Export/T2Export.h"
#include "llvm2kittel/IntTRS/Constraint.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/IntTRS/Term.h"

#include <sstream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <list>
#include <map>
#include <set>

static std::pair<std::map<std::string, int>, std::map<std::string, std::list<std::string> > > getFunToLocIDAndLhsNames(std::list<ref<Rule> > rules)
{
  std::map<std::string, int> funToLocId;
  std::map<std::string, std::list<std::string> > funToLhsNames;
  int nextLocId = 1;
  for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
    //Get the location IDs:
    std::string lhsFunSym = (*i)->getLeft()->getFunctionSymbol();
    if (!funToLocId.count(lhsFunSym)) {
      funToLocId[lhsFunSym] = nextLocId++;
    }

    std::string rhsFunSym = (*i)->getRight()->getFunctionSymbol();
    if (!funToLocId.count(rhsFunSym)) {
      funToLocId[rhsFunSym] = nextLocId++;
    }

    //Now get the default parameter names:
    std::list<ref<Polynomial> > lhsArgs = (*i)->getLeft()->getArgs();
    std::list<std::string> lhsNames;
    for (std::list<ref<Polynomial> >::iterator argi = lhsArgs.begin(), arge = lhsArgs.end(); argi != arge; ++argi) {
      lhsNames.push_back((*argi)->toString());
    }
    if (funToLhsNames.count(lhsFunSym)) {
      std::list<std::string> otherLhsNames = funToLhsNames[lhsFunSym];
      if (lhsNames.size() != otherLhsNames.size()) {
        std::cerr << "Internal error in T2Printer (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xD000);
      } else if (!std::equal(lhsNames.begin(), lhsNames.end(), otherLhsNames.begin())) {
        std::cerr << "Internal error in T2Printer (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
        exit(0xD000);
      }
    } else {
      funToLhsNames[lhsFunSym] = lhsNames;
    }
  }
  return std::make_pair(funToLocId, funToLhsNames);
}

static void printT2Rule(ref<Rule> rule, std::map<std::string, int> funToLocId, std::map<std::string, std::list<std::string> > funToLhsNames, std::ostream &stream)
{
   /*
   * Input: f(x1, ..., xn) -> g(t1, ..., tm) [ COND ]
   * (1) Get canonical variable names y1 ... ym for g parameters (they are well-defined, as this is a llvm2kittel invariant)
   * (2) Add assignments
   *       old_xi := xi;
   *     for those xi that also appear as yj and where tj != xj. Build subst sigma = { xi/old_xi } during this
   * (3) For free variables z1 ... zk in t1 ... tm, COND, set values to nondet()
   *       z1 := nondet();
   *       ...
   *       zk := nondet();
   * (4) Assume guard:
   *       assume(sigma(COND));
   * (5) Assign post-values:
   *       y1 := sigma(t1)
   *       ...
   *       ym := sigma(tm)
   */
  std::string lhsFun = rule->getLeft()->getFunctionSymbol();
  std::string rhsFun = rule->getRight()->getFunctionSymbol();
  std::list<std::string> preVarLhsNames = funToLhsNames[lhsFun];
  std::list<std::string> postVarLhsNames = funToLhsNames[rhsFun];

  stream << "FROM: " << funToLocId[lhsFun] << ";" << std::endl;

  std::set<std::string> changedPostVars;
  { //I blame not having boost, and don't want to polute the outside scope
    std::list<std::string>::iterator postVarNameIt = postVarLhsNames.begin();
    std::list<ref<Polynomial> > rhsArgs = rule->getRight()->getArgs();
    std::list<ref<Polynomial> >::iterator rhsArgIt = rhsArgs.begin();
    while (rhsArgIt != rhsArgs.end() && postVarNameIt != postVarLhsNames.end()) {
      if ((*rhsArgIt)->toString().compare(*postVarNameIt) != 0) {
        changedPostVars.insert(*postVarNameIt);
      }
      ++rhsArgIt;
      ++postVarNameIt;
    }
  }

  //Step (2)
  std::map<std::string, ref<Polynomial> > sigma;
  for (std::list<std::string>::iterator i = preVarLhsNames.begin(), e = preVarLhsNames.end(); i != e; ++i) {
    if (changedPostVars.count(*i)) {
      std::string preVarNameCopy = "kittel_old__" + *i;
      stream << "  " << preVarNameCopy << " := " << *i << ";" << std::endl;
      sigma[*i] = Polynomial::create(preVarNameCopy);
    }
  }

  //Step (3)
  std::set<std::string> freeVariables;
  rule->addVariablesToSet(freeVariables);
  for (std::list<std::string>::iterator i = preVarLhsNames.begin(), e = preVarLhsNames.end(); i != e; ++i) {
    freeVariables.erase(*i);
  }
  for (std::set<std::string>::iterator i = freeVariables.begin(), e = freeVariables.end(); i != e; ++i) {
    stream << "  " << *i << " := nondet();" << std::endl;
  }

  //Step (4)
  ref<Rule> renamedRule = rule->instantiate(&sigma);
  ref<Constraint> constraint = renamedRule->getConstraint();
  if (constraint->getCType() != Constraint::CTrue) {
    stream << "  assume(" << constraint->toString() << ");" << std::endl;
  }

  //Step (5)
  std::list<ref<Polynomial> > renamedRhsArgs = renamedRule->getRight()->getArgs();
  { // Again, I don't have boost
    std::list<std::string>::iterator postVarNameIt = postVarLhsNames.begin();
    std::list<ref<Polynomial> >::iterator rhsArgIt = renamedRhsArgs.begin();
    while (rhsArgIt != renamedRhsArgs.end() && postVarNameIt != postVarLhsNames.end()) {
      std::string postVarName = *postVarNameIt;
      if (changedPostVars.count(postVarName)) {
        ref<Polynomial> rhsArg = *rhsArgIt;
        stream << "  " << postVarName << " := " << rhsArg->toString() << ";" << std::endl;
      }
      ++rhsArgIt;
      ++postVarNameIt;
    }
  }

  stream << "TO: " << funToLocId[rhsFun] << ";" << std::endl;
}

void printT2System(std::list<ref<Rule> > &rules, std::string &startFun, std::ostream &stream)
{
  std::pair<std::map<std::string, int>, std::map<std::string, std::list<std::string > > > t = getFunToLocIDAndLhsNames(rules);
  std::map<std::string, int> funToLocId = t.first;

  stream << "START: " << funToLocId[startFun] << ";" << std::endl << std::endl;
  for (std::list<ref<Rule> >::iterator i = rules.begin(), e = rules.end(); i != e; ++i) {
    printT2Rule(*i, funToLocId, t.second, stream);
  }
}
