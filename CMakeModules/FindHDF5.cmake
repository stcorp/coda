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

if (NOT HDF5_INCLUDE_DIR)
  if ($ENV{HDF5_INCLUDE} MATCHES ".+")
    file(TO_CMAKE_PATH $ENV{HDF5_INCLUDE} HDF5_INCLUDE)
    message(STATUS "Using HDF5_INCLUDE environment variable: ${HDF5_INCLUDE}")
    set(CMAKE_REQUIRED_INCLUDES ${HDF5_INCLUDE})
  endif ($ENV{HDF5_INCLUDE} MATCHES ".+")
endif (NOT HDF5_INCLUDE_DIR)

if (NOT HDF5_LIBRARIES)
  if ($ENV{HDF5_LIB} MATCHES ".+")
    file(TO_CMAKE_PATH $ENV{HDF5_LIB} HDF5_LIB)
    message(STATUS "Using HDF5_LIB environment variable: ${HDF5_LIB}")
  endif ($ENV{HDF5_LIB} MATCHES ".+")
endif (NOT HDF5_LIBRARIES)

find_package(ZLIB)
find_package(SZIP)

check_include_file(hdf5.h HAVE_HDF5_H)

if (HAVE_HDF5_H)
  set(HDF5_INCLUDE_DIR ${HDF5_INCLUDE} CACHE STRING "Location of HDF5 header file(s)")
endif (HAVE_HDF5_H)

set(HDF5_NAMES hdf5)
find_library(HDF5_LIBRARY 
  NAMES ${HDF5_NAMES}
  PATHS ${HDF5_LIB} ENV HDF5_LIB)
if (HDF5_LIBRARY)
  
  # Leo: check_library_exists will not work right now, because
  # CMake insists on using CMAKE_BUILD_TYPE=Debug and freshly
  # reinitialised CMAKE_FLAGS_DEBUG compiler options for the test
  # program it creates to check for the library. With these
  # default values come the /MD flags, causing the linker to
  # generate LNK4098 errors about multiply defined symbols
  # (because the actual HDF5 library we are linking against is of
  # course *NOT* compiled with /MD). After experimenting a lot I
  # come to the conclusion that this is unfixable:
  # check_library_exists does not allow the possibility to
  # configure the cmake call it makes (the underlying
  # try_compile() function does, but that is not exposed), so we
  # can neither instruct the compiler to stop using /MD, nor
  # instruct the linker to use /NODEFAULTLIB.
  #
  # I will get advice on this issue from the Cmake forums, but
  # for now I am just leaving out the library checks: I will
  # simply assume the library is the correct one.
  #
  # The test would have been:
  #
  #  set(CMAKE_REQUIRED_LIBRARIES
  #    ${ZLIB_LIBRARIES}
  #    ${SZIP_LIBRARIES}
  #    )
  #  check_library_exists(${HDF5_LIBRARY} H5Fopen "" HAVE_HDF5)
  #
  # But instead of the above stanza, we just do:
  #
  set(HAVE_HDF5 1)

  if (HAVE_HDF5)
    set(HDF5_LIBRARIES ${HDF5_LIBRARY} CACHE STRING "HDF5 Libraries")
  endif(HAVE_HDF5)

endif (HDF5_LIBRARY)

if (HAVE_HDF5)
  set(HDF5_LIBRARIES
    ${HDF5_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
endif (HAVE_HDF5)

# handle the QUIETLY and REQUIRED arguments and set HDF5_FOUND to
# TRUE if all listed variables are TRUE
#
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HDF5 DEFAULT_MSG HDF5_LIBRARIES HDF5_INCLUDE_DIR)
