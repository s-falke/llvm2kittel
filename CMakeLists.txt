project(llvm2kittel)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(WARN_FLAGS
  "-Wall -Wextra -Wold-style-cast -Wshadow -Wconversion -Wsign-conversion"
  " -Weffc++ -Wswitch-default -Wswitch-enum -Wno-long-long -pedantic"
)

find_program(LLVM_CONFIG_EXECUTABLE
  NAMES llvm-config
  DOC "llvm-config executable"
)

if(LLVM_CONFIG_EXECUTABLE STREQUAL "LLVM_CONFIG_EXECUTABLE-NOTFOUND")
  message(FATAL_ERROR "llvm-config could not be found!")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -fexceptions")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX MATCHALL "[0-9]+" LLVM_VERSION_LIST "${LLVM_VERSION}")

list(LENGTH LLVM_VERSION_LIST LLVM_VERSION_LIST_LENGTH)
if(NOT ${LLVM_VERSION_LIST_LENGTH} EQUAL 2)
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

include(FindGMP)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir
  OUTPUT_VARIABLE LLVM_BINDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CLANG "${LLVM_BINDIR}/clang")
configure_file(${CMAKE_SOURCE_DIR}/kittel-llvm.in ${CMAKE_BINARY_DIR}/kittel-llvm @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/kittel-llvm-bounded.in ${CMAKE_BINARY_DIR}/kittel-llvm-bounded @ONLY)

include_directories(include)

add_library(llvm2kittelAnalysis STATIC
  lib/Analysis/ConditionPropagator.cpp
  lib/Analysis/HierarchyBuilder.cpp
  lib/Analysis/InstChecker.cpp
  lib/Analysis/LoopConditionBlocksCollector.cpp
  lib/Analysis/LoopConditionExplicitizer.cpp
  lib/Analysis/MemoryAnalyzer.cpp
)

add_library(llvm2kittelComplexity STATIC
  lib/Complexity/ComplexityTuplePrinter.cpp
  lib/Complexity/UniformComplexityTuplePrinter.cpp
)

add_library(llvm2kittelCore STATIC
  lib/Core/BoundConditioner.cpp
  lib/Core/Converter.cpp
  lib/Core/Kittelizer.cpp
  lib/Core/Slicer.cpp
  lib/Core/ConditionSimplifier.cpp
)

add_library(llvm2kittelRewriteSystem STATIC
  lib/RewriteSystem/Constraint.cpp
  lib/RewriteSystem/Polynomial.cpp
  lib/RewriteSystem/Rule.cpp
  lib/RewriteSystem/Term.cpp
)

add_library(llvm2kittelTransform STATIC
  lib/Transform/BasicBlockSorter.cpp
  lib/Transform/BitcastCallEliminator.cpp
  lib/Transform/ConstantExprEliminator.cpp
  lib/Transform/EagerInliner.cpp
  lib/Transform/ExtremeInliner.cpp
  lib/Transform/Hoister.cpp
  lib/Transform/InstNamer.cpp
  lib/Transform/Mem2Reg.cpp
  lib/Transform/StrengthIncreaser.cpp
)

add_library(llvm2kittelUtil STATIC
  lib/Util/CommandLine.cpp
  lib/Util/gmp_kittel.cpp
)

add_executable(llvm2kittel
  tools/llvm2kittel.cpp
)

set_target_properties(
  llvm2kittel llvm2kittelAnalysis llvm2kittelComplexity llvm2kittelCore
  llvm2kittelRewriteSystem llvm2kittelTransform llvm2kittelUtil
  PROPERTIES COMPILE_FLAGS "${LLVM_CXXFLAGS} ${GMP_CXXFLAGS}" ${WARN_FLAGS}
)

target_link_libraries(llvm2kittelAnalysis
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelComplexity
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelCore
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
  ${GMP_LIBS} ${GMP_LDFLAGS}
)
target_link_libraries(llvm2kittelRewriteSystem
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
  ${GMP_LIBS} ${GMP_LDFLAGS}
)
target_link_libraries(llvm2kittelTransform
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelUtil
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)

target_link_libraries(llvm2kittel
  llvm2kittelAnalysis llvm2kittelComplexity llvm2kittelCore
  llvm2kittelRewriteSystem llvm2kittelTransform llvm2kittelUtil
)
