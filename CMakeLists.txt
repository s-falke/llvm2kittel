project(llvm2kittel)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
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
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Using LLVM ${LLVM_VERSION}")

string(REGEX MATCHALL "[0-9]+" LLVM_VERSION_LIST "${LLVM_VERSION}")

list(LENGTH LLVM_VERSION_LIST LLVM_VERSION_LIST_LENGTH)
if(${LLVM_VERSION_LIST_LENGTH} LESS 2)
  message(FATAL_ERROR "Could not parse version number")
endif()

list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VALUE)
set(LLVM_MAJOR "-DLLVM_MAJOR=${LLVM_MAJOR_VALUE}")
list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VALUE)
set(LLVM_MINOR "-DLLVM_MINOR=${LLVM_MINOR_VALUE}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} -fexceptions")
set(LLVM_CXXFLAGS "${LLVM_CXXFLAGS} ${LLVM_MAJOR} ${LLVM_MINOR}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs bitreader ipo target transformutils
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if("${LLVM_MAJOR_VALUE}.${LLVM_MINOR_VALUE}" VERSION_GREATER "3.4")
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(LLVM_LIBS "${LLVM_LIBS} ${LLVM_SYSTEM_LIBS}")
  add_definitions("--std=c++11")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(GMP REQUIRED)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir
  OUTPUT_VARIABLE LLVM_BINDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CLANG "${LLVM_BINDIR}/clang")
configure_file(${CMAKE_SOURCE_DIR}/kittel-llvm.in ${CMAKE_BINARY_DIR}/kittel-llvm @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/kittel-llvm-bounded.in ${CMAKE_BINARY_DIR}/kittel-llvm-bounded @ONLY)

include_directories(include)
include_directories(${GMP_INCLUDE_DIR})

add_library(llvm2kittelAnalysis STATIC
  lib/Analysis/ConditionPropagator.cpp
  lib/Analysis/HierarchyBuilder.cpp
  lib/Analysis/InstChecker.cpp
  lib/Analysis/LoopConditionBlocksCollector.cpp
  lib/Analysis/LoopConditionExplicitizer.cpp
  lib/Analysis/MemoryAnalyzer.cpp
  include/llvm2kittel/Analysis/ConditionPropagator.h
  include/llvm2kittel/Analysis/HierarchyBuilder.h
  include/llvm2kittel/Analysis/InstChecker.h
  include/llvm2kittel/Analysis/LoopConditionBlocksCollector.h
  include/llvm2kittel/Analysis/LoopConditionExplicitizer.h
  include/llvm2kittel/Analysis/MemoryAnalyzer.h
)

set(llvm2kittelExportFiles
  lib/Export/ComplexityTuplePrinter.cpp
  lib/Export/UniformComplexityTuplePrinter.cpp
  lib/Export/T2Export.cpp
  include/llvm2kittel/Export/ComplexityTuplePrinter.h
  include/llvm2kittel/Export/UniformComplexityTuplePrinter.h
  include/llvm2kittel/Export/T2Export.h
)

add_library(llvm2kittelExport STATIC
  ${llvm2kittelExportFiles}
)

add_library(llvm2kittelCore STATIC
  lib/Core/BoundConstrainer.cpp
  lib/Core/Converter.cpp
  lib/Core/ConstraintEliminator.cpp
  lib/Core/Kittelizer.cpp
  lib/Core/Slicer.cpp
  lib/Core/ConstraintSimplifier.cpp
  include/llvm2kittel/BoundConstrainer.h
  include/llvm2kittel/ConstraintEliminator.h
  include/llvm2kittel/ConstraintSimplifier.h
  include/llvm2kittel/Converter.h
  include/llvm2kittel/DivConstraintStore.h
  include/llvm2kittel/DivRemConstraintType.h
  include/llvm2kittel/Kittelizer.h
  include/llvm2kittel/RemConstraintStore.h
  include/llvm2kittel/Slicer.h
)

add_library(llvm2kittelIntTRS STATIC
  lib/IntTRS/Constraint.cpp
  lib/IntTRS/Polynomial.cpp
  lib/IntTRS/Rule.cpp
  lib/IntTRS/Term.cpp
  include/llvm2kittel/IntTRS/Constraint.h
  include/llvm2kittel/IntTRS/Polynomial.h
  include/llvm2kittel/IntTRS/Rule.h
  include/llvm2kittel/IntTRS/Term.h
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
  include/llvm2kittel/Transform/BasicBlockSorter.h
  include/llvm2kittel/Transform/BitcastCallEliminator.h
  include/llvm2kittel/Transform/ConstantExprEliminator.h
  include/llvm2kittel/Transform/EagerInliner.h
  include/llvm2kittel/Transform/ExtremeInliner.h
  include/llvm2kittel/Transform/Hoister.h
  include/llvm2kittel/Transform/InstNamer.h
  include/llvm2kittel/Transform/Mem2Reg.h
  include/llvm2kittel/Transform/NondefFactory.h
  include/llvm2kittel/Transform/StrengthIncreaser.h
)

add_library(llvm2kittelUtil STATIC
  lib/Util/CommandLine.cpp
  lib/Util/gmp_kittel.cpp
  include/llvm2kittel/Util/CommandLine.h
  include/llvm2kittel/Util/gmp_kittel.h
  include/llvm2kittel/Util/quadruple.h
  include/llvm2kittel/Util/Ref.h
  include/llvm2kittel/Util/Version.h
  include/WARN_OFF.h
  include/WARN_ON.h
)

add_custom_target(
  make_git_sha1
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/make_git_sha1.sh ${CMAKE_BINARY_DIR}/GitSHA1.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_source_files_properties(
  ${CMAKE_BINARY_DIR}/GitSHA1.cpp
  PROPERTIES GENERATED TRUE
)
add_executable(llvm2kittel
  tools/llvm2kittel.cpp
  ${CMAKE_BINARY_DIR}/GitSHA1.cpp
  tools/GitSHA1.h
)
add_dependencies(
  llvm2kittel make_git_sha1
)

set_target_properties(
  llvm2kittel llvm2kittelAnalysis llvm2kittelExport llvm2kittelCore
  llvm2kittelIntTRS llvm2kittelTransform llvm2kittelUtil
  PROPERTIES COMPILE_FLAGS "${LLVM_CXXFLAGS}" ${WARN_FLAGS}
)

target_link_libraries(llvm2kittelAnalysis
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelExport
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelCore
  ${LLVM_LIBS} ${LLVM_LDFLAGS} ${GMP_LIBRARIES}
)
target_link_libraries(llvm2kittelIntTRS
  ${LLVM_LIBS} ${LLVM_LDFLAGS} ${GMP_LIBRARIES}
)
target_link_libraries(llvm2kittelTransform
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)
target_link_libraries(llvm2kittelUtil
  ${LLVM_LIBS} ${LLVM_LDFLAGS}
)

target_link_libraries(llvm2kittel
  llvm2kittelAnalysis llvm2kittelExport llvm2kittelCore
  llvm2kittelIntTRS llvm2kittelTransform llvm2kittelUtil
)
