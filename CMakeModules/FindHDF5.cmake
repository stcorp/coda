# Find the HDF5 library
#
# This module defines
# HDF5_INCLUDE_DIR, where to find hdf5.h, etc.
# HDF5_LIBRARIES, the hdf5 libraries to link against to use HDF5.
# HDF5_FOUND, If false, do not try to use HDF5.
#
# The user may specify HDF5_INCLUDE and HDF5_LIB environment
# variables to locate include files and library
#

if ($ENV{HDF5_INCLUDE} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{HDF5_INCLUDE} HDF5_INCLUDE)
  message(STATUS "HDF5_INCLUDE from environment: ${HDF5_INCLUDE}")
  set(CMAKE_REQUIRED_INCLUDES ${HDF5_INCLUDE})
endif ($ENV{HDF5_INCLUDE} MATCHES ".+")

if ($ENV{HDF5_LIB} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{HDF5_LIB} HDF5_LIB)
  message(STATUS "HDF5_LIB from environment: ${HDF5_LIB}")
endif ($ENV{HDF5_LIB} MATCHES ".+")

find_package(ZLIB)
find_package(SZIP)

check_include_file(hdf5.h HAVE_HDF5_H)

set(HDF5_NAMES hdf5dll hdf5)
find_library(HDF5_LIBRARY 
  NAMES ${HDF5_NAMES}
  PATHS ENV HDF5_LIB)
if (HDF5_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES
    ${ZLIB_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
  check_library_exists(${HDF5_LIBRARY} H5Fopen "" HAVE_HDF5)
  if (HAVE_HDF5)
    set(HDF5_LIBRARIES ${HDF5_LIBRARY})
  endif(HAVE_HDF5)
endif (HDF5_LIBRARY)

if (HAVE_HDF5_H)
  set(HDF5_INCLUDE_DIR ${HDF5_INCLUDE})
endif (HAVE_HDF5_H)

if (HAVE_HDF5)
  set(HDF5_LIBRARIES
    ${HDF5_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
endif (HAVE_HDF5)

# handle the QUIETLY and REQUIRED arguments and set HDF5_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HDF5 DEFAULT_MSG HDF5_LIBRARIES HDF5_INCLUDE_DIR)
