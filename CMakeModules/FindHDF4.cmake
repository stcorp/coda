# Find the HDF4 library
#
# This module defines
# HDF4_INCLUDE_DIR, where to find hdfi.h, etc.
# HDF4_LIBRARIES, the hdf libraries to link against to use HDF4.
# HDF4_FOUND, If false, do not try to use HDF4.
#
# The user may specify HDF4_INCLUDE and HDF4_LIB environment
# variables to locate include files and library
#

if ($ENV{HDF4_INCLUDE} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{HDF4_INCLUDE} HDF4_INCLUDE)
  message(STATUS "HDF4_INCLUDE from environment: ${HDF4_INCLUDE}")
  set(CMAKE_REQUIRED_INCLUDES ${HDF4_INCLUDE})
endif ($ENV{HDF4_INCLUDE} MATCHES ".+")

if ($ENV{HDF4_LIB} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{HDF4_LIB} HDF4_LIB)
  message(STATUS "HDF4_LIB from environment: ${HDF4_LIB}")
endif ($ENV{HDF4_LIB} MATCHES ".+")

find_package(JPEG)
find_package(ZLIB)
find_package(SZIP)

check_include_file(hdf.h HAVE_HDF_H)
check_include_file(netcdf.h HAVE_NETCDF_H)
check_include_file(mfhdf.h HAVE_MFHDF_H)

set(DF_NAMES df hd423m)
find_library(DF_LIBRARY 
  NAMES ${DF_NAMES}
  PATHS ENV HDF4_LIB)
if (DF_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES
    ${ZLIB_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
  check_library_exists(${DF_LIBRARY} Hopen "" HAVE_DF)
  if (HAVE_DF)
    set(DF_LIBRARIES ${DF_LIBRARY})
  endif(HAVE_DF)
endif (DF_LIBRARY)

set(MFHDF_NAMES mfhdf hm423m)
find_library(MFHDF_LIBRARY 
  NAMES ${MFHDF_NAMES}
  PATHS ENV HDF4_LIB)
if (MFHDF_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES
    ${DF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
  check_library_exists(${MFHDF_LIBRARY} SDstart "" HAVE_MFHDF)
  if (HAVE_MFHDF)
    set(MFHDF_LIBRARIES ${MFHDF_LIBRARY})
  endif(HAVE_MFHDF)
endif (MFHDF_LIBRARY)

if (HAVE_HDF_H AND
    HAVE_MFHDF_H)
  set(HDF4_INCLUDE_DIR ${HDF4_INCLUDE})
endif (HAVE_HDF_H AND
    HAVE_MFHDF_H)

if (HAVE_DF AND
    HAVE_MFHDF)
  set(HDF4_LIBRARIES
    ${MFHDF_LIBRARIES}
    ${DF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${SZIP_LIBRARIES}
    )
endif (HAVE_DF AND
  HAVE_MFHDF)


# handle the QUIETLY and REQUIRED arguments and set HDF4_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HDF4 DEFAULT_MSG HDF4_LIBRARIES HDF4_INCLUDE_DIR)
