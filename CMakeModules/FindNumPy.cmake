# Find the Python numpy module development files for CODA
#
# This module defines:
#
# - NUMPY_FOUND - if the numpy module was found
# - NUMPY_INCLUDE_DIR - where to find numpy/ndarrayobject.h etc.
#

find_path(NUMPY_INCLUDE_DIR
  NAMES numpy/ndarrayobject.h
  PATHS ${NUMPY_INCLUDE} ENV NUMPY_INCLUDE)

set(NUMPY_INCLUDE ${NUMPY_INCLUDE} CACHE STRING "Location of Python numpy include files" FORCE)


# handle the QUIETLY and REQUIRED arguments and set NUMPY_FOUND to
# TRUE if all listed variables are TRUE
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NUMPY DEFAULT_MSG NUMPY_INCLUDE_DIR)
mark_as_advanced(NUMPY_INCLUDE_DIR)