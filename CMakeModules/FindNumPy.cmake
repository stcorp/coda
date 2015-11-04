# Find the Python numpy module development files for CODA
#
# This module defines:
#
# - NUMPY_FOUND - if the numpy module was found The location of the Python interpreter (identical to
# PYTHONINTERP).
# - NUMPY_INCLUDE_DIR - where to find numpy/ndarrayobject.h etc.
#

find_path(NUMPY_INCLUDE_DIR
  NAMES numpy/ndarrayobject.h
  PATHS ${NUMPY_INCLUDE} ENV NUMPY_INCLUDE)

# handle the QUIETLY and REQUIRED arguments and set NUMPY_FOUND to
# TRUE if all listed variables are TRUE
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NUMPY DEFAULT_MSG NUMPY_INCLUDE_DIR)
