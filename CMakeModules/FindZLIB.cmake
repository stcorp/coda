# Find the Zlib library
#
# This module defines
# ZLIB_INCLUDE_DIR, where to find zlib.h
# ZLIB_LIBRARIES, the libraries to link against to use Zlib.
# ZLIB_FOUND, If false, do not try to use Zlib
#
# The user may specify ZLIB_INCLUDE and ZLIB_LIB environment
# variables to locate include files and library directories
#

find_path(ZLIB_INCLUDE_DIR
  NAMES zlib.h 
  PATHS ENV ZLIB_INCLUDE)

set(ZLIB_NAMES z zlib zdll)
find_library(ZLIB_LIBRARY 
  NAMES ${ZLIB_NAMES}
  PATHS ENV ZLIB_LIB)
if (ZLIB_LIBRARY)
  check_library_exists(${ZLIB_LIBRARY} deflate "" HAVE_ZLIB)
  if (HAVE_ZLIB)
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
  endif(HAVE_ZLIB)
endif (ZLIB_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB DEFAULT_MSG ZLIB_LIBRARIES ZLIB_INCLUDE_DIR)

