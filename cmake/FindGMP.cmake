find_path(GMP_INCLUDE_DIR gmpxx.h
          DOC "GMP include path"
         )

find_library(GMP_LIBRARY NAMES gmp
             DOC "GMP libary"
            )
find_library(GMPXX_LIBRARY NAMES gmpxx
             DOC "GMPXX library"
            )

if(GMP_INCLUDE_DIR AND GMP_LIBRARY AND GMPXX_LIBRARY)
  get_filename_component(GMP_LIBRARY_DIR ${GMP_LIBRARY} PATH)
  get_filename_component(GMPXX_LIBRARY_DIR ${GMPXX_LIBRARY} PATH)
  set(GMP_CXXFLAGS "-I${GMP_INCLUDE_DIR}")
  set(GMP_LDFLAGS "-L${GMP_LIBRARY_DIR} -L${GMPXX_LIBRARY_DIR}")
  set(GMP_LIBS "-lgmp -lgmpxx")
  set(GMP_FOUND TRUE)
endif()

if(GMP_FOUND)
  message(STATUS "Found GMP")
elseif(NOT GMP_FOUND)
  message(FATAL_ERROR "Could not find GMP")
endif()
