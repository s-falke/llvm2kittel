// This file is part of llvm2KITTeL
//
// Copyright 2010-2014 Stephan Falke
//
// Licensed under the University of Illinois/NCSA Open Source License.
// See LICENSE for details.

#include "llvm2kittel/BoundConstrainer.h"
#include "llvm2kittel/ConstraintEliminator.h"
#include "llvm2kittel/ConstraintSimplifier.h"
#include "llvm2kittel/Converter.h"
#include "llvm2kittel/DivRemConstraintType.h"
#include "llvm2kittel/Kittelizer.h"
#include "llvm2kittel/Slicer.h"
#include "llvm2kittel/Analysis/ConditionPropagator.h"
#include "llvm2kittel/Analysis/HierarchyBuilder.h"
#include "llvm2kittel/Analysis/InstChecker.h"
#include "llvm2kittel/Analysis/LoopConditionBlocksCollector.h"
#include "llvm2kittel/Analysis/LoopConditionExplicitizer.h"
#include "llvm2kittel/Analysis/MemoryAnalyzer.h"
#include "llvm2kittel/Export/ComplexityTuplePrinter.h"
#include "llvm2kittel/Export/UniformComplexityTuplePrinter.h"
#include "llvm2kittel/Export/T2Export.h"
#include "llvm2kittel/IntTRS/Polynomial.h"
#include "llvm2kittel/IntTRS/Rule.h"
#include "llvm2kittel/Transform/BasicBlockSorter.h"
#include "llvm2kittel/Transform/BitcastCallEliminator.h"
#include "llvm2kittel/Transform/ConstantExprEliminator.h"
#include "llvm2kittel/Transform/EagerInliner.h"
#include "llvm2kittel/Transform/ExtremeInliner.h"
#include "llvm2kittel/Transform/Hoister.h"
#include "llvm2kittel/Transform/InstNamer.h"
#include "llvm2kittel/Transform/Mem2Reg.h"
#include "llvm2kittel/Transform/NondefFactory.h"
#include "llvm2kittel/Transform/StrengthIncreaser.h"
#include "llvm2kittel/Util/CommandLine.h"
#include "llvm2kittel/Util/Version.h"

// llvm includes
#include "WARN_OFF.h"
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Assembly/PrintModulePass.h>
#endif
#include <llvm/Bitcode/ReaderWriter.h>
#if LLVM_VERSION < VERSION(3, 3)
  #include <llvm/LLVMContext.h>
#else
  #include <llvm/IR/LLVMContext.h>
#endif
#if LLVM_VERSION < VERSION(3, 2)
  #include <llvm/Target/TargetData.h>
#elif LLVM_VERSION == VERSION(3, 2)
  #include <llvm/DataLayout.h>
#else
  #include <llvm/IR/DataLayout.h>
#endif
#if LLVM_VERSION < VERSION(3, 7)
  #include <llvm/PassManager.h>
#else
  #include <llvm/IR/LegacyPassManager.h>
#endif
#include <llvm/Analysis/Passes.h>
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Analysis/Verifier.h>
#else
  #include <llvm/IR/Verifier.h>
#endif
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#if LLVM_VERSION < VERSION(3, 5)
  #include <llvm/Support/system_error.h>
#else
  #include <system_error>
#endif
#if LLVM_VERSION >= VERSION(3, 5)
  #include <llvm/Support/FileSystem.h>
#endif
#include "WARN_ON.h"

// C++ includes
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdlib>

// command line stuff

#include "GitSHA1.h"

void versionPrinter()
{
    std::cout << "llvm2KITTeL" << std::endl;
    std::cout << "Copyright 2010-2014 Stephan Falke" << std::endl;
    std::cout << "Version " << get_git_sha1();
    std::cout << ", using LLVM " << LLVM_MAJOR << "." << LLVM_MINOR << std::endl;
}

static cl::opt<std::string> filename(cl::Positional, cl::Required, cl::desc("<input bitcode>"), cl::init(std::string()));
static cl::opt<std::string> functionname("function", cl::desc("Start function for the termination analysis"), cl::init(std::string()));
static cl::opt<unsigned int> numInlines("inline", cl::desc("Maximum number of function inline steps"), cl::init(0));
static cl::opt<bool> eagerInline("eager-inline", cl::desc("Exhaustively inline (acyclic call hierarchies only)"), cl::init(false));

static cl::opt<bool> debug("debug", cl::desc(""), cl::init(false), cl::ReallyHidden);
static cl::opt<bool> assumeIsControl("assume-is-control", cl::desc("Calls to assume are control points"), cl::init(false));
static cl::opt<bool> selectIsControl("select-is-control", cl::desc("Select instructions are control points"), cl::init(false));
static cl::opt<bool> inlineVoids("inline-voids", cl::desc("Function with return type void are also inlined"), cl::init(false));
static cl::opt<bool> increaseStrength("increase-strength", cl::desc("Replace shifts by multiplication/division"), cl::init(false));
static cl::opt<bool> noSlicing("no-slicing", cl::desc("Do not slice the generated TRS"), cl::init(false));
static cl::opt<bool> conservativeSlicing("conservative-slicing", cl::desc("Be conservative in slicing the generated TRS"), cl::init(false));
static cl::opt<bool> onlyMultiPredIsControl("multi-pred-control", cl::desc("Only basic blocks with multiple predecessors are control points"), cl::init(false));
static cl::opt<bool> boundedIntegers("bounded-integers", cl::desc("Use bounded integers instead of mathematical integers"), cl::init(false));
static cl::opt<bool> unsignedEncoding("unsigned-encoding", cl::desc("Use unsigned box for bounded integers"), cl::init(false));
static cl::opt<bool> propagateConditions("propagate-conditions", cl::desc("Propagate branch conditions to successor basic blocks"), cl::init(false));
static cl::opt<bool> explicitizeLoopConditions("explicitize-loop-conditions", cl::desc("Derive certain loop conditions"), cl::init(false), cl::ReallyHidden);
static cl::opt<bool> simplifyConds("simplify-conditions", cl::desc("Simplify conditions in the generated TRS"), cl::init(false));
static cl::opt<bool> onlyLoopConditions("only-loop-conditions", cl::desc("Only encode loop conditions in the generated TRS"), cl::init(false));
static cl::opt<DivRemConstraintType> divisionConstraintType("division-constraint",
                                                            cl::desc("Determine the constraint type generated for division and remainder operations:"),
                                                            cl::init(Approximated),
                                                            cl::values(
                                                                       clEnumValN(Exact, "exact", "exactly model the semantics (only for mathematical integers)"),
                                                                       clEnumValN(Approximated, "approximated", "approximately model the semantics (default)"),
                                                                       clEnumValN(None, "none", "do not model the semantics"),
                                                                       clEnumValEnd)
                                                           );
static cl::opt<SMTSolver> smtSolver("smt-solver",
                                    cl::desc("Use SMT solver to eliminate rules with unsatisfiable contraints"),
                                    cl::init(NoSolver),
                                    cl::values(
                                               clEnumValN(CVC4Solver, "cvc4", "use CVC4"),
                                               clEnumValN(MathSAT5Solver, "mathsat5", "use MathSAT5"),
                                               clEnumValN(Yices2Solver, "yices2", "use Yices2"),
                                               clEnumValN(Z3Solver, "z3", "use Z3"),
                                               clEnumValN(NoSolver, "none", "do not eliminate unsatisfiable contraints"),
                                               clEnumValEnd)
                                    );
static cl::opt<bool> bitwiseConditions("bitwise-conditions", cl::desc("Add conditions for bitwise & and |"), cl::init(false));

static cl::opt<bool> dumpLL("dump-ll", cl::desc("Dump transformed bitcode into a file"), cl::init(false));

static cl::opt<bool> t2output("t2", cl::desc("Generate T2 format"), cl::init(false), cl::ReallyHidden);
static cl::opt<bool> complexityTuples("complexity-tuples", cl::desc("Generate complexity tuples"), cl::init(false), cl::ReallyHidden);
static cl::opt<bool> uniformComplexityTuples("uniform-complexity-tuples", cl::desc("Generate uniform complexity tuples"), cl::init(false), cl::ReallyHidden);

void transformModule(llvm::Module *module, llvm::Function *function, NondefFactory &ndf)
{
#if LLVM_VERSION < VERSION(3, 2)
    llvm::TargetData *TD = NULL;
#elif LLVM_VERSION < VERSION(3, 5)
    llvm::DataLayout *TD = NULL;
#elif LLVM_VERSION < VERSION(3, 7)
    llvm::DataLayoutPass *TD = NULL;
#endif
#if LLVM_VERSION < VERSION(3, 5)
    const std::string &ModuleDataLayout = module->getDataLayout();
#elif LLVM_VERSION == VERSION(3, 5)
    const std::string &ModuleDataLayout = module->getDataLayout()->getStringRepresentation();
#endif
#if LLVM_VERSION < VERSION(3, 2)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::TargetData(ModuleDataLayout);
    }
#elif LLVM_VERSION < VERSION(3, 5)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::DataLayout(ModuleDataLayout);
    }
#elif LLVM_VERSION == VERSION(3, 5)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::DataLayoutPass(llvm::DataLayout(ModuleDataLayout));
    }
#elif LLVM_VERSION < VERSION(3, 7)
    TD = new llvm::DataLayoutPass();
#endif

    // pass manager
#if LLVM_VERSION < VERSION(3, 7)
    llvm::PassManager llvmPasses;
#else
    llvm::legacy::PassManager llvmPasses;
#endif

#if LLVM_VERSION < VERSION(3, 7)
    if (TD != NULL) {
        llvmPasses.add(TD);
    }
#endif

    // first, do some verification of the input code before we modify it
    llvmPasses.add(llvm::createVerifierPass());

    // eliminate calls to bitcast functions
    llvmPasses.add(createBitcastCallEliminatorPass());

    // function inlining
    if (eagerInline) {
        llvmPasses.add(createEagerInlinerPass());
    } else {
        for (unsigned int i = 0; i < numInlines; ++i) {
            llvmPasses.add(createExtremeInlinerPass(function, inlineVoids));
        }
    }

    // mem2reg
    llvmPasses.add(createMem2RegPass(ndf));

    // Hoist
    llvmPasses.add(createHoisterPass());

    // DCE
    llvmPasses.add(llvm::createDeadCodeEliminationPass());

    // simplify cfg
    llvmPasses.add(llvm::createCFGSimplificationPass());

    // DCE
    llvmPasses.add(llvm::createDeadCodeEliminationPass());

    // lower switch to branches
    llvmPasses.add(llvm::createLowerSwitchPass());

    // eliminate constant expressions
    llvmPasses.add(createConstantExprEliminatorPass());

    // strength increasing
    if (increaseStrength) {
        llvmPasses.add(createStrengthIncreaserPass());
    }

    // sort basic blocks
    llvmPasses.add(createBasicBlockSorterPass());

    // Alias analysis
    llvmPasses.add(llvm::createBasicAliasAnalysisPass());

    // lastly, do some verification of the modified code
    llvmPasses.add(llvm::createVerifierPass());

    // run them!
    llvmPasses.run(*module);
}

std::pair<MayMustMap, std::set<llvm::GlobalVariable*> > getMayMustMap(llvm::Function *function)
{
    llvm::Module *module = function->getParent();

#if LLVM_VERSION < VERSION(3, 2)
    llvm::TargetData *TD = NULL;
#elif LLVM_VERSION < VERSION(3, 5)
    llvm::DataLayout *TD = NULL;
#elif LLVM_VERSION < VERSION(3, 7)
    llvm::DataLayoutPass *TD = NULL;
#endif
#if LLVM_VERSION < VERSION(3, 5)
    const std::string &ModuleDataLayout = module->getDataLayout();
#elif LLVM_VERSION == VERSION(3, 5)
    const std::string &ModuleDataLayout = module->getDataLayout()->getStringRepresentation();
#endif
#if LLVM_VERSION < VERSION(3, 2)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::TargetData(ModuleDataLayout);
    }
#elif LLVM_VERSION < VERSION(3, 5)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::DataLayout(ModuleDataLayout);
    }
#elif LLVM_VERSION == VERSION(3, 5)
    if (!ModuleDataLayout.empty()) {
        TD = new llvm::DataLayoutPass(llvm::DataLayout(ModuleDataLayout));
    }
#elif LLVM_VERSION < VERSION(3, 7)
    TD = new llvm::DataLayoutPass();
#endif

    // pass manager
#if LLVM_VERSION < VERSION(3, 7)
    llvm::FunctionPassManager PM(module);
#else
    llvm::legacy::FunctionPassManager PM(module);
#endif

#if LLVM_VERSION < VERSION(3, 7)
    if (TD != NULL) {
        PM.add(TD);
    }
#endif

    PM.add(llvm::createBasicAliasAnalysisPass());

    MemoryAnalyzer *maPass = createMemoryAnalyzerPass();
    PM.add(maPass);

    PM.run(*function);

    return std::make_pair(maPass->getMayMustMap(), maPass->getMayZap());
}

TrueFalseMap getConditionPropagationMap(llvm::Function *function, std::set<llvm::BasicBlock*> lcbs)
{
    llvm::Module *module = function->getParent();

    // pass manager
#if LLVM_VERSION < VERSION(3, 7)
    llvm::FunctionPassManager PM(module);
#else
    llvm::legacy::FunctionPassManager PM(module);
#endif

    ConditionPropagator *cpPass = createConditionPropagatorPass(debug, onlyLoopConditions, lcbs);
    PM.add(cpPass);

    PM.run(*function);

    return cpPass->getTrueFalseMap();
}

ConditionMap getExplicitizedLoopConditionMap(llvm::Function *function)
{
    llvm::Module *module = function->getParent();

    // pass manager
#if LLVM_VERSION < VERSION(3, 7)
    llvm::FunctionPassManager PM(module);
#else
    llvm::legacy::FunctionPassManager PM(module);
#endif

    LoopConditionExplicitizer *lcePass = createLoopConditionExplicitizerPass(debug);
    PM.add(lcePass);

    PM.run(*function);

    return lcePass->getConditionMap();
}

std::set<llvm::BasicBlock*> getLoopConditionBlocks(llvm::Function *function)
{
    llvm::Module *module = function->getParent();

    // pass manager
#if LLVM_VERSION < VERSION(3, 7)
    llvm::FunctionPassManager PM(module);
#else
    llvm::legacy::FunctionPassManager PM(module);
#endif

    LoopConditionBlocksCollector *lcbPass = createLoopConditionBlocksCollectorPass();
    PM.add(lcbPass);

    PM.run(*function);

    return lcbPass->getLoopConditionBlocks();
}

std::string getPartNumber(unsigned int currNum, unsigned int maxNum)
{
    unsigned int width;
    if (maxNum >= 100) {
        width = 3;
    } else if (maxNum >= 10) {
        width = 2;
    } else {
        width = 1;
    }
    std::ostringstream sstream;
    sstream.fill('0');
    sstream << std::setw(static_cast<int>(width)) << currNum;
    return sstream.str();
}

std::string getSccName(std::list<llvm::Function*> scc)
{
    std::ostringstream sstream;
    for (std::list<llvm::Function*>::iterator i = scc.begin(), e = scc.end(); i != e; ) {
        sstream << (*i)->getName().str();
        if (++i != e) {
            sstream << "_";
        }
    }
    return sstream.str();
}

int main(int argc, char *argv[])
{
    cl::SetVersionPrinter(&versionPrinter);
    cl::ParseCommandLineOptions(argc, argv, "llvm2kittel\n");

    if (boundedIntegers && divisionConstraintType == Exact) {
        std::cerr << "Cannot use \"-division-constraint=exact\" in combination with \"-bounded-integers\"" << std::endl;
        return 333;
    }
    if (!boundedIntegers && unsignedEncoding) {
        std::cerr << "Cannot use \"-unsigned-encoding\" without \"-bounded-integers\"" << std::endl;
        return 333;
    }
    if (!boundedIntegers && bitwiseConditions) {
        std::cerr << "Cannot use \"-bitwise-conditions\" without \"-bounded-integers\"" << std::endl;
        return 333;
    }
    if (numInlines != 0 && eagerInline) {
        std::cerr << "Cannot use \"-inline\" in combination with \"-eager-inline\"" << std::endl;
        return 333;
    }

#if LLVM_VERSION < VERSION(3, 5)
    llvm::OwningPtr<llvm::MemoryBuffer> owningBuffer;
    llvm::MemoryBuffer::getFileOrSTDIN(filename, owningBuffer);
    llvm::MemoryBuffer *buffer = owningBuffer.get();
#else
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> owningBuffer = llvm::MemoryBuffer::getFileOrSTDIN(filename);
    llvm::MemoryBuffer *buffer = NULL;
    if (!owningBuffer.getError()) {
        buffer = owningBuffer->get();
    }
#endif

    if (buffer == NULL) {
        std::cerr << "LLVM bitcode file \"" << filename << "\" does not exist or cannot be read." << std::endl;
        return 1;
    }

    llvm::LLVMContext context;
    std::string errMsg;
#if LLVM_VERSION < VERSION(3, 5)
    llvm::Module *module = llvm::ParseBitcodeFile(buffer, context, &errMsg);
#elif LLVM_VERSION == VERSION(3, 5)
    llvm::Module *module = NULL;
    llvm::ErrorOr<llvm::Module*> moduleOrError = llvm::parseBitcodeFile(buffer, context);
    std::error_code ec = moduleOrError.getError();
    if (ec) {
        errMsg = ec.message();
    } else {
        module = moduleOrError.get();
    }
#elif LLVM_VERSION == VERSION(3, 6)
    llvm::Module *module = NULL;
    llvm::ErrorOr<llvm::Module*> moduleOrError = llvm::parseBitcodeFile(buffer->getMemBufferRef(), context);
    std::error_code ec = moduleOrError.getError();
    if (ec) {
        errMsg = ec.message();
    } else {
        module = moduleOrError.get();
    }
#else
    llvm::Module *module = NULL;
    llvm::ErrorOr<std::unique_ptr<llvm::Module>> moduleOrError = llvm::parseBitcodeFile(buffer->getMemBufferRef(), context);
    std::error_code ec = moduleOrError.getError();
    if (ec) {
        errMsg = ec.message();
    } else {
        module = moduleOrError.get().release();
    }
#endif

    // check if the file is a proper bitcode file and contains a module
    if (module == NULL) {
        std::cerr << "LLVM bitcode file doesn't contain a valid module." << std::endl;
        return 2;
    }

    llvm::Function *function = NULL;
    llvm::Function *firstFunction = NULL;
    unsigned int numFunctions = 0;
    std::list<std::string> functionNames;

    for (llvm::Module::iterator i = module->begin(), e = module->end(); i != e; ++i) {
        if (i->getName() == functionname) {
            function = i;
            break;
        } else if (functionname.empty() && i->getName() == "main") {
            function = i;
            break;
        } else if (!i->isDeclaration()) {
            ++numFunctions;
            functionNames.push_back(i->getName());
            if (firstFunction == NULL) {
                firstFunction = i;
            }
        }
    }

    if (function == NULL) {
        if (numFunctions == 0) {
            std::cerr << "Module does not contain any function." << std::endl;
            return 3;
        }
        if (functionname.empty()) {
            if (numFunctions == 1) {
                function = firstFunction;
            } else {
                std::cerr << "LLVM module contains more than one function:" << std::endl;
                for (std::list<std::string>::iterator i = functionNames.begin(), e = functionNames.end(); i != e; ++i) {
                    std::cerr << "    " << *i << std::endl;
                }
                std::cerr << "Please specify which function should be used." << std::endl;
                return 4;
            }
        } else  {
            std::cerr << "Specified function not found." << std::endl;
            std::cerr << "Candidates are:" << std::endl;
            for (std::list<std::string>::iterator i = functionNames.begin(), e = functionNames.end(); i != e; ++i) {
                std::cerr << "    " << *i << std::endl;
            }
            return 5;
        }
    }

    // check for cyclic call hierarchies
    HierarchyBuilder checkHierarchy;
    checkHierarchy.computeHierarchy(module);
    if (eagerInline && checkHierarchy.isCyclic()) {
        std::cerr << "Cannot use \"-eager-inline\" with a cyclic call hierarchy!" << std::endl;
        return 7;
    }

    // transform!
    NondefFactory ndf(module);
    transformModule(module, function, ndf);

    // name them!
    InstNamer namer;
    namer.visit(module);

    // check them!
    const llvm::Type *boolType = llvm::Type::getInt1Ty(context);
    const llvm::Type *floatType = llvm::Type::getFloatTy(context);
    const llvm::Type *doubleType = llvm::Type::getDoubleTy(context);
    InstChecker checker(boolType, floatType, doubleType);
    checker.visit(module);

    // print it!
    if (debug) {
#if LLVM_VERSION < VERSION(3, 5)
        llvm::PassManager printPass;
        printPass.add(llvm::createPrintModulePass(&llvm::outs()));
        printPass.run(*module);
#else
        llvm::outs() << *module << '\n';
#endif
    }
    if (dumpLL) {
        std::string outFile = filename.substr(0, filename.length() - 3) + ".ll";
#if LLVM_VERSION < VERSION(3, 5)
        std::string errorInfo;
        llvm::raw_fd_ostream stream(outFile.data(), errorInfo);
        if (errorInfo.empty()) {
            llvm::PassManager dumpPass;
            dumpPass.add(llvm::createPrintModulePass(&stream));
            dumpPass.run(*module);
            stream.close();
        }
#elif LLVM_VERSION == VERSION(3, 5)
        std::string errorInfo;
        llvm::raw_fd_ostream stream(outFile.data(), errorInfo, llvm::sys::fs::F_Text);
        if (errorInfo.empty()) {
            stream << *module << '\n';
            stream.close();
        }
#else
        std::error_code errorCode;
        llvm::raw_fd_ostream stream(outFile.data(), errorCode, llvm::sys::fs::F_Text);
        if (!errorCode) {
            stream << *module << '\n';
            stream.close();
        }
#endif
    }

    // check for junk
    std::list<llvm::Instruction*> unsuitable = checker.getUnsuitableInsts();
    if (!unsuitable.empty()) {
        std::cerr << "Unsuitable instructions detected:" << std::endl;
        for (std::list<llvm::Instruction*>::iterator i = unsuitable.begin(), e = unsuitable.end(); i != e; ++i) {
            (*i)->dump();
        }
        return 6;
    }

    // compute recursion hierarchy
    HierarchyBuilder hierarchy;
    hierarchy.computeHierarchy(module);
    std::list<std::list<llvm::Function*> > sccs = hierarchy.getSccs();

    std::map<llvm::Function*, std::list<llvm::Function*> > funToScc;
    for (std::list<std::list<llvm::Function*> >::iterator i = sccs.begin(), e = sccs.end(); i != e; ++i) {
        std::list<llvm::Function*> scc = *i;
        for (std::list<llvm::Function*>::iterator si = scc.begin(), se = scc.end(); si != se; ++si) {
            funToScc.insert(std::make_pair(*si, scc));
        }
    }
    std::list<llvm::Function*> dependsOnList = hierarchy.getTransitivelyCalledFunctions(function);
    std::set<llvm::Function*> dependsOn;
    dependsOn.insert(dependsOnList.begin(), dependsOnList.end());
    dependsOn.insert(function);
    std::list<std::list<llvm::Function*> > dependsOnSccs;
    for (std::list<std::list<llvm::Function*> >::iterator i = sccs.begin(), e = sccs.end(); i != e; ++i) {
        std::list<llvm::Function*> scc = *i;
        for (std::list<llvm::Function*>::iterator fi = scc.begin(), fe = scc.end(); fi != fe; ++fi) {
            llvm::Function *f = *fi;
            if (dependsOn.find(f) != dependsOn.end()) {
                dependsOnSccs.push_back(scc);
                break;
            }
        }
    }

    // compute may/must info, propagated conditions, and compute loop exiting blocks for all functions
    std::map<llvm::Function*, MayMustMap> mmMap;
    std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > funcMayZapDirect;
    std::map<llvm::Function*, TrueFalseMap> tfMap;
    std::map<llvm::Function*, std::set<llvm::BasicBlock*> > lebMap;
    std::map<llvm::Function*, ConditionMap> elcMap;
    for (std::set<llvm::Function*>::iterator df = dependsOn.begin(), dfe = dependsOn.end(); df != dfe; ++df) {
        llvm::Function *func = *df;
        std::pair<MayMustMap, std::set<llvm::GlobalVariable*> > tmp = getMayMustMap(func);
        mmMap.insert(std::make_pair(func, tmp.first));
        funcMayZapDirect.insert(std::make_pair(func, tmp.second));
        std::set<llvm::BasicBlock*> lcbs;
        if (onlyLoopConditions) {
            lcbs = getLoopConditionBlocks(func);
            lebMap.insert(std::make_pair(func, lcbs));
        }
        if (propagateConditions) {
            tfMap.insert(std::make_pair(func, getConditionPropagationMap(func, lcbs)));
        }
        if (explicitizeLoopConditions) {
            elcMap.insert(std::make_pair(func, getExplicitizedLoopConditionMap(func)));
        }
    }

    // transitively close funcMayZapDirect
    std::map<llvm::Function*, std::set<llvm::GlobalVariable*> > funcMayZap;
    for (std::set<llvm::Function*>::iterator df = dependsOn.begin(), dfe = dependsOn.end(); df != dfe; ++df) {
        llvm::Function *func = *df;
        std::set<llvm::GlobalVariable*> funcTransZap;
        std::list<llvm::Function*> funcDependsOnList = hierarchy.getTransitivelyCalledFunctions(func);
        std::set<llvm::Function*> funcDependsOn;
        funcDependsOn.insert(funcDependsOnList.begin(), funcDependsOnList.end());
        funcDependsOn.insert(func);
        for (std::set<llvm::Function*>::iterator depfi = funcDependsOn.begin(), depfe = funcDependsOn.end(); depfi != depfe; ++depfi) {
            llvm::Function *depf = *depfi;
            std::map<llvm::Function*, std::set<llvm::GlobalVariable*> >::iterator depfZap = funcMayZapDirect.find(depf);
            if (depfZap == funcMayZapDirect.end()) {
                std::cerr << "Could not find alias information (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
                exit(9876);
            }
            funcTransZap.insert(depfZap->second.begin(), depfZap->second.end());
        }
        funcMayZap.insert(std::make_pair(func, funcTransZap));
    }

    // convert sccs separately
    unsigned int num = static_cast<unsigned int>(dependsOnSccs.size());
    unsigned int currNum = 0;
    for (std::list<std::list<llvm::Function*> >::iterator scci = dependsOnSccs.begin(), scce = dependsOnSccs.end(); scci != scce; ++scci) {
        std::list<llvm::Function*> scc = *scci;
        std::list<ref<Rule> > allRules;
        std::list<ref<Rule> > allCondensedRules;
        std::list<ref<Rule> > allKittelizedRules;
        std::list<ref<Rule> > allSlicedRules;
        if (debug) {
            std::cout << "========================================" << std::endl;
        }
        if ((!complexityTuples && !uniformComplexityTuples) || debug) {
            std::cout << "///*** " << getPartNumber(++currNum, num) << '_' << getSccName(scc) << " ***///" << std::endl;
        }
        std::set<llvm::Function*> sccSet;
        sccSet.insert(scc.begin(), scc.end());

        std::set<std::string> complexityLHSs;

        for (std::list<llvm::Function*>::iterator fi = scc.begin(), fe = scc.end(); fi != fe; ++fi) {
            llvm::Function *curr = *fi;
            Converter converter(boolType, assumeIsControl, selectIsControl, onlyMultiPredIsControl, boundedIntegers, unsignedEncoding, onlyLoopConditions, divisionConstraintType, bitwiseConditions, complexityTuples || uniformComplexityTuples);
            std::map<llvm::Function*, MayMustMap>::iterator tmp1 = mmMap.find(curr);
            if (tmp1 == mmMap.end()) {
                std::cerr << "Could not find alias information (" << __FILE__ << ":" << __LINE__ << ")!" << std::endl;
                exit(9876);
            }
            MayMustMap curr_mmMap = tmp1->second;
            std::map<llvm::Function*, TrueFalseMap>::iterator tmp2 = tfMap.find(curr);
            TrueFalseMap curr_tfMap;
            if (tmp2 != tfMap.end()) {
                curr_tfMap = tmp2->second;
            }
            std::map<llvm::Function*, std::set<llvm::BasicBlock*> >::iterator tmp3 = lebMap.find(curr);
            std::set<llvm::BasicBlock*> curr_leb;
            if (tmp3 != lebMap.end()) {
                curr_leb = tmp3->second;
            }
            std::map<llvm::Function*, ConditionMap>::iterator tmp4 = elcMap.find(curr);
            ConditionMap curr_elcMap;
            if (tmp4 != elcMap.end()) {
                curr_elcMap = tmp4->second;
            }
            converter.phase1(curr, sccSet, curr_mmMap, funcMayZap, curr_tfMap, curr_leb, curr_elcMap);
            converter.phase2(curr, sccSet, curr_mmMap, funcMayZap, curr_tfMap, curr_leb, curr_elcMap);
            std::list<ref<Rule> > rules = converter.getRules();
            std::list<ref<Rule> > condensedRules = converter.getCondensedRules();
            std::list<ref<Rule> > kittelizedRules = kittelize(condensedRules, smtSolver);
            Slicer slicer(curr, converter.getPhiVariables());
            std::list<ref<Rule> > slicedRules;
            if (noSlicing) {
                slicedRules = kittelizedRules;
            } else {
                slicedRules = slicer.sliceUsage(kittelizedRules);
                slicedRules = slicer.sliceConstraint(slicedRules);
                slicedRules = slicer.sliceDefined(slicedRules);
                slicedRules = slicer.sliceStillUsed(slicedRules, conservativeSlicing);
                slicedRules = slicer.sliceDuplicates(slicedRules);
            }
            if (boundedIntegers) {
                slicedRules = kittelize(addBoundConstraints(slicedRules, converter.getBitwidthMap(), unsignedEncoding), smtSolver);
            }
            if (debug) {
                allRules.insert(allRules.end(), rules.begin(), rules.end());
                allCondensedRules.insert(allCondensedRules.end(), condensedRules.begin(), condensedRules.end());
                allKittelizedRules.insert(allKittelizedRules.end(), kittelizedRules.begin(), kittelizedRules.end());
            }
            if (simplifyConds) {
                slicedRules = simplifyConstraints(slicedRules);
            }
            allSlicedRules.insert(allSlicedRules.end(), slicedRules.begin(), slicedRules.end());

            if (complexityTuples || uniformComplexityTuples) {
                std::set<std::string> tmpLHSs = converter.getComplexityLHSs();
                complexityLHSs.insert(tmpLHSs.begin(), tmpLHSs.end());
            }
        }
        if (debug) {
            std::cout << "========================================" << std::endl;
            for (std::list<ref<Rule> >::iterator i = allRules.begin(), e = allRules.end(); i != e; ++i) {
                ref<Rule> tmp = *i;
                std::cout << tmp->toString() << std::endl;
            }
            std::cout << "========================================" << std::endl;
            for (std::list<ref<Rule> >::iterator i = allCondensedRules.begin(), e = allCondensedRules.end(); i != e; ++i) {
                ref<Rule> tmp = *i;
                std::cout << tmp->toString() << std::endl;
            }
            std::cout << "========================================" << std::endl;
            for (std::list<ref<Rule> >::iterator i = allKittelizedRules.begin(), e = allKittelizedRules.end(); i != e; ++i) {
                ref<Rule> tmp = *i;
                std::cout << tmp->toString() << std::endl;
            }
            std::cout << "========================================" << std::endl;
        }
        if (complexityTuples) {
            printComplexityTuples(allSlicedRules, complexityLHSs, std::cout);
        } else if (uniformComplexityTuples) {
            std::ostringstream startfun;
            startfun << "eval_" << getSccName(scc) << "_start";
            std::string name = startfun.str();
            printUniformComplexityTuples(allSlicedRules, complexityLHSs, name, std::cout);
        } else if (t2output) {
            std::string startFun = "eval_" + getSccName(scc) + "_start";
            printT2System(allSlicedRules, startFun, std::cout);
        } else {
            for (std::list<ref<Rule> >::iterator i = allSlicedRules.begin(), e = allSlicedRules.end(); i != e; ++i) {
                ref<Rule> tmp = *i;
                std::cout << tmp->toKittelString() << std::endl;
            }
        }
    }

    return 0;
}
