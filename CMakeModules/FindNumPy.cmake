# Find the Python numpy module development files for CODA
#
# This module defines:
#
# - NUMPY_FOUND - if the numpy module was found
# - NUMPY_INCLUDE_DIR - where to find numpy/ndarrayobject.h etc.
#

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; import os; p = [p for p in sys.path if os.path.exists(os.path.join(p, 'numpy'))]; sys.stdout.write(os.path.join(p[0], 'numpy') if len(p) > 0 else '')" OUTPUT_VARIABLE NUMPY_INSTALL_PREFIX)

find_path(NUMPY_INCLUDE_DIR numpy/ndarrayobject.h
  PATHS ${NUMPY_INCLUDE} "${NUMPY_INSTALL_PREFIX}/core/include" ENV NUMPY_INCLUDE
  NO_DEFAULT_PATH)

set(NUMPY_INCLUDE ${NUMPY_INCLUDE} CACHE STRING "Location of Python numpy include files" FORCE)


# handle the QUIETLY and REQUIRED arguments and set NUMPY_FOUND to
# TRUE if all listed variables are TRUE
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NUMPY DEFAULT_MSG NUMPY_INCLUDE_DIR)
mark_as_advanced(NUMPY_INCLUDE_DIR)
