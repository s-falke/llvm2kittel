CXX=g++
#CXX=clang++
#CXX=c++-analyzer

LLVM_MAJOR=3
LLVM_MINOR=4

LLVM_LIB_DIR=/path/to/llvm/lib
LLVM_INCLUDE_DIR=/path/to/llvm/include

OBJS=llvm2kittel.o InstChecker.o InstNamer.o Polynomial.o Constraint.o Term.o Rule.o Converter.o ConstantExprEliminator.o Kittelizer.o Slicer.o BasicBlockSorter.o ExtremeInliner.o HierarchyBuilder.o Hoister.o MemoryAnalyzer.o StrengthIncreaser.o BoundConditioner.o ConditionPropagator.o ConditionSimplifier.o LoopConditionBlocksCollector.o CommandLine.o gmp_kittel.o Mem2Reg.o LoopConditionExplicitizer.o EagerInliner.o ComplexityTuplePrinter.o UniformComplexityTuplePrinter.o BitcastCallEliminator.o
DEPS=$(OBJS:.o=.cpp.depends)

LLVM_LINKFLAGS=-lLLVMBitReader -lLLVMipo -lLLVMipa -lLLVMScalarOpts -lLLVMTransformUtils -lLLVMAnalysis -lLLVMTarget -lLLVMCore -lLLVMSupport -L$(LLVM_LIB_DIR)
WARNFLAGS=-Werror -Wall -Wextra -Wold-style-cast -Wshadow -Wconversion -Wsign-conversion -Weffc++ -Wswitch-default -Wswitch-enum -Wno-long-long -pedantic
COMPILEFLAGS=-c -g -O3 -I$(LLVM_INCLUDE_DIR) -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -DLLVM_MAJOR=$(LLVM_MAJOR) -DLLVM_MINOR=$(LLVM_MINOR)

.cpp.o:
	$(CXX) -MMD -MF $(patsubst %.cpp,%.cpp.depends,$<) $(WARNFLAGS) $(COMPILEFLAGS) $<

default: llvm2kittel

-include $(DEPS)

llvm2kittel: $(OBJS)
	$(CXX) $(OBJS) $(LLVM_LINKFLAGS) -lncurses -lpthread -ldl -lgmpxx -lgmp -o llvm2kittel

llvm2kittel.static: $(OBJS)
	$(CXX) $(OBJS) $(LLVM_LINKFLAGS) -lncurses -lpthread -ldl -lgmpxx -lgmp -static -o llvm2kittel.static

clean:
	rm -f $(OBJS) $(DEPS)

distclean: clean
	rm -f llvm2kittel llvm2kittel.static
