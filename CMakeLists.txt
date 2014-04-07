cmake_minimum_required(VERSION 2.8)
project(llvm2kittel)

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config DOC "llvm-config executable")
if(LLVM_CONFIG_EXECUTABLE STREQUAL "LLVM_CONFIG_EXECUTABLE-NOTFOUND")
  message(FATAL_ERROR "llvm-config could not be found!")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -fno-exceptions -fno-rtti")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX MATCHALL "[0-9]+" LLVM_VERSION_LIST "${LLVM_VERSION}")

list(LENGTH LLVM_VERSION_LIST LLVM_VERSION_LIST_LENGTH)
if (NOT ${LLVM_VERSION_LIST_LENGTH} EQUAL 2)
  message(FATAL_ERROR "Could not parse version number")
endif()

list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VALUE)
set(LLVM_MAJOR "-DLLVM_MAJOR=${LLVM_MAJOR_VALUE}")
list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VALUE)
set(LLVM_MINOR "-DLLVM_MINOR=${LLVM_MINOR_VALUE}")
set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} ${LLVM_MAJOR} ${LLVM_MINOR}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs bitreader ipo target transformutils
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_LDFLAGS "${LLVM_LDFLAGS} -lgmpxx -lgmp")

add_library(llvm2kittelLibrary STATIC
  BasicBlockSorter.cpp
  BitcastCallEliminator.cpp
  BoundConditioner.cpp
  CommandLine.cpp
  ComplexityTuplePrinter.cpp
  ConditionPropagator.cpp
  ConditionSimplifier.cpp
  ConstantExprEliminator.cpp
  Constraint.cpp
  Converter.cpp
  EagerInliner.cpp
  ExtremeInliner.cpp
  HierarchyBuilder.cpp
  Hoister.cpp
  InstChecker.cpp
  InstNamer.cpp
  Kittelizer.cpp
  LoopConditionBlocksCollector.cpp
  LoopConditionExplicitizer.cpp
  Mem2Reg.cpp
  MemoryAnalyzer.cpp
  Polynomial.cpp
  Rule.cpp
  Slicer.cpp
  StrengthIncreaser.cpp
  Term.cpp
  UniformComplexityTuplePrinter.cpp
  gmp_kittel.cpp
  BasicBlockSorter.h
  BitcastCallEliminator.h
  BoundConditioner.h
  CommandLine.h
  ComplexityTuplePrinter.h
  ConditionPropagator.h
  ConditionSimplifier.h
  ConstantExprEliminator.h
  Constraint.h
  Converter.h
  DivConstraintStore.h
  EagerInliner.h
  ExtremeInliner.h
  HierarchyBuilder.h
  Hoister.h
  InstChecker.h
  InstNamer.h
  Kittelizer.h
  LoopConditionBlocksCollector.h
  LoopConditionExplicitizer.h
  Macros.h
  Mem2Reg.h
  MemoryAnalyzer.h
  NondefFactory.h
  Polynomial.h
  RemConstraintStore.h
  Rule.h
  Slicer.h
  StrengthIncreaser.h
  Term.h
  UniformComplexityTuplePrinter.h
  Version.h
  WARN_OFF.h
  WARN_ON.h
  gmp_kittel.h
  quadruple.h
)

add_executable(llvm2kittel
  llvm2kittel.cpp
)

set_target_properties(llvm2kittel llvm2kittelLibrary
    PROPERTIES COMPILE_FLAGS "${LLVM_CXXFLAGS}")

target_link_libraries(llvm2kittelLibrary ${LLVM_LIBS} ${LLVM_LDFLAGS})
target_link_libraries(llvm2kittel llvm2kittelLibrary)
