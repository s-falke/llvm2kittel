# Find GMP (GNU multi precision math library)
#
#  GMP_FOUND       - True if GMP was found
#  GMP_INCLUDE_DIR - Path to GMP's include directory.
#  GMP_LIBRARY     - Path to libgmp.
#  GMPXX_LIBRARY   - Path to libgmpxx.
#  GMP_LIBRARIES   - Path to GMP's libraries.

include(FindPackageHandleStandardArgs)

if(GMP_INCLUDE_DIR AND GMP_LIBRARY AND GMPXX_LIBRARY)

  # Already found, don't search again
  set(GMP_FOUND TRUE)

else(GMP_INCLUDE_DIR AND GMP_LIBRARY AND GMPXX_LIBRARY)

  find_path(GMP_INCLUDE_DIR gmpxx.h DOC "Path to GMP's include directory")

  find_library(GMP_LIBRARY NAMES gmp DOC "Path to GMP's library")

  find_library(GMPXX_LIBRARY NAMES gmpxx DOC "Path to GMP's C++ library")

  FIND_PACKAGE_HANDLE_STANDARD_ARGS(GMP DEFAULT_MSG
    GMP_INCLUDE_DIR
    GMP_LIBRARY
    GMPXX_LIBRARY
  )

  set(GMP_LIBRARIES ${GMP_LIBRARY} ${GMPXX_LIBRARY})

  set(GMP_LIBRARIES ${GMP_LIBRARIES} CACHE STRING "Paths to GMP's libraries")

  mark_as_advanced(
    GMP_INCLUDE_DIR
    GMP_LIBRARY
    GMPXX_LIBRARY
    GMP_LIBRARIES
  )

endif(GMP_INCLUDE_DIR AND GMP_LIBRARY AND GMPXX_LIBRARY)
