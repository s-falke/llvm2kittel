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
  for (ref<Rule> rule : rules) {
    //Get the location IDs:
    auto lhsFunSym = rule->getLeft()->getFunctionSymbol();
    if (! funToLocId.count(lhsFunSym)) {
      funToLocId[lhsFunSym] = nextLocId++;
    }

    auto rhsFunSym = rule->getRight()->getFunctionSymbol();
    if (! funToLocId.count(rhsFunSym)) {
      funToLocId[rhsFunSym] = nextLocId++;
    }

    //Now get the default parameter names:
    auto lhsArgs = rule->getLeft()->getArgs();
    std::list<std::string> lhsNames;
    for (auto lhsArg : lhsArgs) {
      lhsNames.push_back(lhsArg->toString());
    }
    if (funToLhsNames.count(lhsFunSym)) {
      auto otherLhsNames = funToLhsNames[lhsFunSym];
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
  auto lhsFun = rule->getLeft()->getFunctionSymbol();
  auto rhsFun = rule->getRight()->getFunctionSymbol();
  auto preVarLhsNames = funToLhsNames[lhsFun];
  auto postVarLhsNames = funToLhsNames[rhsFun];

  stream << "FROM: " << funToLocId[lhsFun] << ";" << std::endl;

  std::set<std::string> changedPostVars;
  { //I blame not having boost, and don't want to polute the outside scope
    auto postVarNameIt = postVarLhsNames.begin();
    auto rhsArgs = rule->getRight()->getArgs();
    auto rhsArgIt = rhsArgs.begin();
    for(;rhsArgIt != rhsArgs.end() && postVarNameIt != postVarLhsNames.end();rhsArgIt++, postVarNameIt++) {
      if ((*rhsArgIt)->toString().compare(*postVarNameIt) != 0) {
        changedPostVars.insert(*postVarNameIt);
      }
    }
  }

  //Step (2)
  std::map<std::string, ref<Polynomial> > sigma;
  for (auto preVarName : preVarLhsNames) {
    if (changedPostVars.count(preVarName)) {
      auto preVarNameCopy = "kittel_old__" + preVarName;
      stream << "  " << preVarNameCopy << " := " << preVarName << ";" << std::endl;
      sigma[preVarName] = Polynomial::create(preVarNameCopy);
    }
  }
  
  //Step (3)
  std::set<std::string> freeVariables;
  rule->addVariablesToSet(freeVariables);
  for (auto lhsVar : preVarLhsNames) {
    freeVariables.erase(lhsVar);
  }
  for (auto freeVar : freeVariables) {
    stream << "  " << freeVar << " := nondet();" << std::endl;
  }

  //Step (4)
  auto renamedRule = rule->instantiate(&sigma);
  auto constraint = renamedRule->getConstraint();
  if (constraint->getCType() != Constraint::CTrue) {
    stream << "  assume(" << constraint->toString() << ");" << std::endl;
  }

  //Step (5)
  auto renamedRhsArgs = renamedRule->getRight()->getArgs();
  { // Again, I don't have boost
    auto postVarNameIt = postVarLhsNames.begin();
    auto rhsArgIt = renamedRhsArgs.begin();
    for(;rhsArgIt != renamedRhsArgs.end() && postVarNameIt != postVarLhsNames.end(); rhsArgIt++, postVarNameIt++) {
      auto postVarName = *postVarNameIt;
      if (changedPostVars.count(postVarName)) {
        auto rhsArg = *rhsArgIt;
        stream << "  " << postVarName << " := " << rhsArg->toString() << ";" << std::endl;
      }
    }
  }
  
  stream << "TO: " << funToLocId[rhsFun] << ";" << std::endl;
}

void printT2System(std::list<ref<Rule> > &rules, std::string &startFun, std::ostream &stream)
{
  auto t = getFunToLocIDAndLhsNames(rules);
  auto funToLocId = t.first;

  stream << "START: " << funToLocId[startFun] << ";" << std::endl << std::endl;
  for (auto rule : rules) {
    printT2Rule(rule, funToLocId, t.second, stream);
  }
}
