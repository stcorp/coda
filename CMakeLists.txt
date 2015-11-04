# CMake configuration file for Coda.
#
project(CODA)

set(CODA_VERSION 1.0)
set(VERSION 1.0)

# General setup
#
cmake_minimum_required(VERSION 2.6.0)
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/CMakeModules
  ${CMAKE_MODULE_PATH}
  CACHE INTERNAL "Local CMake modules"
  )
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(TestBigEndian)
include(CheckTypeExists)

set(CMAKE_VERBOSE_MAKEFILE OFF)


# Option handling
#
option(SUBPACKAGE_MODE "build CODA as sub-package" OFF)
option(BUILD_IDL "build IDL interface" OFF)
option(BUILD_MATLAB "build MATLAB interface" OFF)
option(BUILD_PYTHON "build Python interface" OFF)
option(DISABLE_HDF4_VDATA_ATTRIBUTES "enable HDF4 Vdata attributes" OFF)
option(WITH_HDF4 "use HDF4" OFF)
option(WITH_HDF5 "use HDF5" OFF)

# The following is done because in CMake options are always
# ON/OFF only, but for config.h substition we need something with
# value 1.
#
if (NOT DISABLE_HDF4_VDATA_ATTRIBUTES)
  set(ENABLE_HDF4_VDATA_ATTRIBUTES 1)
endif (NOT DISABLE_HDF4_VDATA_ATTRIBUTES)

# zlib is needed by libcoda independent of hdf.

find_package(ZLIB)

if (NOT ZLIB_FOUND)
  message(FATAL_ERROR, "zlib libraries and/or header files are not found. Try setting the ZLIB_LIB and ZLIB_INCLUDE environment variables to the location of your zlib library and include files.")
else (NOT ZLIB_FOUND)
  include_directories(${ZLIB_INCLUDE_DIR})
  link_libraries(${ZLIB_LIBRARIES})
  set (HAVE_ZLIB 1)
endif (NOT ZLIB_FOUND)

# hdf

if (WITH_HDF4)

  find_package(HDF4)

  if (NOT HDF4_FOUND)
    message(FATAL_ERROR "HDF4 libraries and/or header files are not found. Try setting the HDF4_LIB and HDF4_INCLUDE environment variables to the location of your HDF4 library and include files.")
  else (NOT HDF4_FOUND)
    include_directories(${HDF4_INCLUDE})
    link_libraries(${HDF4_LIBRARIES})
    set(HAVE_HDF4 1)
  endif (NOT HDF4_FOUND)

endif (WITH_HDF4)


if (WITH_HDF5)

  find_package(HDF5)

  if (NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 libraries and/or header files are not found. Try setting the HDF5_LIB and HDF5_INCLUDE environment variables to the location of your HDF5 library and include files.")
  else (NOT HDF5_FOUND)
    include_directories(${HDF5_INCLUDE})
    link_libraries(${HDF5_LIBRARIES})
    set(HAVE_HDF5 1)
  endif (NOT HDF5_FOUND)

endif (WITH_HDF5)


# *** xml ***
set(XML_NS 1)
set(XML_DTD 1)
set(XML_LARGE_SIZE 1)
set(XML_CONTEXT_BYTES 1024)


# Required libraries
#
find_library(LIBM_LIBRARY m)
if (LIBM_LIBRARY)
  link_libraries (${LIBM_LIBRARY})
  set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${LIBM_LIBRARY})
  set(HAVE_LIBM 1)
endif (LIBM_LIBRARY)

if (APPLE)
  find_library(LIBSYSTEMSTUBS_LIBRARY SystemStubs)
  if (LIBSYSTEMSTUBS_LIBRARY)
    set(HAVE_LIBSYSTEMSTUBS 1)
  endif (LIBSYSTEMSTUBS_LIBRARY)
endif (APPLE)

# Lex / Yacc - adapted from CMake's own CMakeLists.txt
#
FIND_PROGRAM(YACC_EXECUTABLE
  NAMES yacc bison
  PATHS /usr/bin
  DOC "Yacc or Bison executable")
FIND_PROGRAM(FLEX_EXECUTABLE
  NAMES flex
  PATHS /usr/bin
  DOC "Flex executable")
MARK_AS_ADVANCED(YACC_EXECUTABLE FLEX_EXECUTABLE)
IF(YACC_EXECUTABLE)
  SET(BISON_FLAGS)
  IF(YACC_EXECUTABLE MATCHES "bison")
    SET(BISON_FLAGS "--yacc --defines")
  ENDIF(YACC_EXECUTABLE MATCHES "bison")
  ADD_CUSTOM_COMMAND(
    OUTPUT coda-expr-parser.c coda-expr-parser.h
    DEPENDS libcoda/coda-expr-parser.y
    COMMAND
    ${YACC_EXECUTABLE} --defines=coda-expr-parser.h -ocoda-expr-parser.c ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-parser.y
    )
  ADD_CUSTOM_TARGET(RerunYacc DEPENDS coda-expr-parser.c)
ENDIF(YACC_EXECUTABLE)

IF(FLEX_EXECUTABLE)
  ADD_CUSTOM_COMMAND(
    OUTPUT coda-expr-tokenizer.c
    DEPENDS libcoda/coda-expr-tokenizer.l
    COMMAND
    ${FLEX_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-tokenizer.l
# Leo: copy lex.yy.c manually -- --outfile option to flex does not work,
# because it gets explicitly overriden in the .l file.
    COMMAND
    ${CMAKE_COMMAND} -E copy lex.yy.c coda-expr-tokenizer.c
    )
  ADD_CUSTOM_TARGET(RerunLex DEPENDS code-expr-tokenizer.c)
ENDIF(FLEX_EXECUTABLE)


# Required include files
#
set(INCLUDES "")

macro (FIND_INCLUDE FILE VARIABLE)
  CHECK_INCLUDE_FILE(${FILE} ${VARIABLE})
  if (${VARIABLE})
    set(INCLUDES ${INCLUDES} ${FILE})
  endif (${VARIABLE})
endmacro (FIND_INCLUDE)

FIND_INCLUDE(dirent.h		HAVE_DIRENT_H)
FIND_INCLUDE(dlfcn.h		HAVE_DLFCN_H)
FIND_INCLUDE(export.h		HAVE_EXPORT_H)
FIND_INCLUDE(inttypes.h		HAVE_INTTYPES_H)
FIND_INCLUDE(memory.h		HAVE_MEMORY_H)
FIND_INCLUDE(stdint.h		HAVE_STDINT_H)
FIND_INCLUDE(stdlib.h		HAVE_STDLIB_H)
FIND_INCLUDE(strings.h		HAVE_STRINGS_H)
FIND_INCLUDE(string.h		HAVE_STRING_H)
FIND_INCLUDE(sys/mman.h	    HAVE_SYS_MMAN_H)
FIND_INCLUDE(sys/stat.h		HAVE_SYS_STAT_H)
FIND_INCLUDE(sys/types.h	HAVE_SYS_TYPES_H)
FIND_INCLUDE(unistd.h		HAVE_UNISTD_H)
FIND_INCLUDE(zlib.h		    HAVE_ZLIB_H)

set(CMAKE_EXTRA_INCLUDE_FILES ${INCLUDES})

# Required functions
#
CHECK_FUNCTION_EXISTS(bcopy HAVE_BCOPY)
CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(getpagesize HAVE_GETPAGESIZE)
CHECK_FUNCTION_EXISTS(malloc HAVE_MALLOC)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(pread HAVE_PREAD)
CHECK_FUNCTION_EXISTS(realloc HAVE_REALLOC)
CHECK_FUNCTION_EXISTS(stat HAVE_STAT)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strdup HAVE_STRDUP)
CHECK_FUNCTION_EXISTS(strncasecmp HAVE_STRNCASECMP)
CHECK_FUNCTION_EXISTS(vsnprintf HAVE_VSNPRINTF)

if (WIN32)
  # Hack for Windows
  set (HAVE_STRDUP 1)
  set (HAVE_STRCASECMP 1)
  set (HAVE_STRNCASECMP 1)
  set (HAVE_VSNPRINTF 1)
endif (WIN32)

# Required types (and their sizes)
#
CHECK_TYPE_EXISTS(uchar "${INCLUDES}" HAVE_UCHAR)
set(UCHAR "unsigned char")
CHECK_TYPE_EXISTS(size_t "${INCLUDES}" HAVE_SIZE_T)
set(SIZE_T "long")
CHECK_TYPE_EXISTS(ptrdiff_t "${INCLUDES}" HAVE_PTRDIFF_T)
set(PTRDIFF_T "int")

CHECK_TYPE_SIZE(size_t SIZEOF_SIZE_T)
CHECK_TYPE_SIZE(off_t SIZEOF_OFF_T)
CHECK_TYPE_SIZE(double SIZEOF_DOUBLE)
CHECK_TYPE_SIZE(float SIZEOF_FLOAT)
CHECK_TYPE_SIZE(int SIZEOF_INT)
CHECK_TYPE_SIZE(long SIZEOF_LONG)
CHECK_TYPE_SIZE(short SIZEOF_SHORT)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

if (HAVE_STDLIB_H AND HAVE_STDDEF_H)
  set(STDC_HEADERS 1)
endif (HAVE_STDLIB_H AND HAVE_STDDEF_H)
if (NOT HAVE_SYS_TYPES_H)
  set(NO_SYS_TYPES_H 1)
endif (NOT HAVE_SYS_TYPES_H)
if (NOT HAVE_STRERROR)
  set(NO_STRERROR 1)
endif (NOT HAVE_STRERROR)

set(CODA_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda
  ${CMAKE_CURRENT_BINARY_DIR}
  CACHE INTERNAL "Include directories for the CODA library")

include_directories(
  ${CODA_INCLUDE_DIRS}
)


# config files


ADD_DEFINITIONS(-DHAVE_CONFIG_H -DHAVE_CODA_CONFIG)

CONFIGURE_FILE(config.h.cmake.in config.h)
CONFIGURE_FILE(libcoda/coda.h.cmake.in coda.h)
CONFIGURE_FILE(idl/coda-idl.dlm.in idl/coda-idl.dlm)

# The Doxyfile uses some autoconf variables CMake does not have.
# We set them here manually, so that we can use the same .in
# file for both autoconf and cmake
#
set(top_builddir ${CMAKE_BINARY_DIR})
set(top_srcdir ${CMAKE_SOURCE_DIR})
set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/doc)
CONFIGURE_FILE(libcoda/doc/Doxyfile.in libcoda/doc/Doxyfile)

# libcoda sources
#
set(LIBCODA_SOURCES
  libcoda/coda-ascbin-cursor.c
  libcoda/coda-ascbin-definition.c
  libcoda/coda-ascbin-definition.h
  libcoda/coda-ascbin-internal.h
  libcoda/coda-ascbin-type.c
  libcoda/coda-ascbin.c
  libcoda/coda-ascbin.h
  libcoda/coda-ascii-cursor.c
  libcoda/coda-ascii-definition.c
  libcoda/coda-ascii-definition.h
  libcoda/coda-ascii-internal.h
  libcoda/coda-ascii-type.c
  libcoda/coda-ascii.c
  libcoda/coda-ascii.h
  libcoda/coda-bin-cursor.c
  libcoda/coda-bin-definition.c
  libcoda/coda-bin-definition.h
  libcoda/coda-bin-internal.h
  libcoda/coda-bin-type.c
  libcoda/coda-bin.c
  libcoda/coda-bin.h
  libcoda/coda-cursor.c
  libcoda/coda-definition-parse.c
  libcoda/coda-definition.c
  libcoda/coda-definition.h
  libcoda/coda-errno.c
  libcoda/coda-expr-internal.h
  coda-expr-parser.c
  coda-expr-parser.h
  coda-expr-tokenizer.c
  libcoda/coda-expr.c
  libcoda/coda-expr.h
  libcoda/coda-filefilter.c
  libcoda/coda-filefilter.h
  libcoda/coda-internal.h
  libcoda/coda-path.h
  libcoda/coda-product.c
  libcoda/coda-type.c
  libcoda/coda-utils.c
  libcoda/coda-xml-cursor.c
  libcoda/coda-xml-definition.c
  libcoda/coda-xml-definition.h
  libcoda/coda-xml-dynamic.c
  libcoda/coda-xml-dynamic.h
  libcoda/coda-xml-internal.h
  libcoda/coda-xml-parser.c
  libcoda/coda-xml-type.c
  libcoda/coda-xml.c
  libcoda/coda-xml.h
  libcoda/coda.c
  libcoda/hashtable.c
  libcoda/hashtable.h
  libcoda/ziparchive.c
  libcoda/ziparchive.h
)

# HDF4/HDF5 stuff not used yet.
#
set(LIBCODA_HDF4_FILES
  libcoda/coda-hdf4-cursor.c
  libcoda/coda-hdf4-definition.c
  libcoda/coda-hdf4-internal.h
  libcoda/coda-hdf4-type.c
  libcoda/coda-hdf4.h
)

set(LIBCODA_HDF5_FILES
  libcoda/coda-hdf5-cursor.c
  libcoda/coda-hdf5-definition.c
  libcoda/coda-hdf5-internal.h
  libcoda/coda-hdf5-type.c
  libcoda/coda-hdf5.h
)

if (WITH_HDF4)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF4_FILES})
endif (WITH_HDF4)

if (WITH_HDF5)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF5_FILES})
endif (WITH_HDF5)


# Custom CODA expat library
#
set(LIBEXPAT_SOURCES
  libcoda/expat/ascii.h
  libcoda/expat/asciitab.h
  libcoda/expat/coda_expat_mangle.h
  libcoda/expat/expat.h
  libcoda/expat/expat_external.h
  libcoda/expat/iasciitab.h
  libcoda/expat/internal.h
  libcoda/expat/latin1tab.h
  libcoda/expat/nametab.h
  libcoda/expat/utf8tab.h
  libcoda/expat/xmlparse.c
  libcoda/expat/xmlrole.c
  libcoda/expat/xmlrole.h
  libcoda/expat/xmltok.c
  libcoda/expat/xmltok.h
  libcoda/expat/xmltok_impl.h
)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/expat)
add_library(expat STATIC ${LIBEXPAT_SOURCES})

install(FILES coda.h DESTINATION include)

# Build both shared and static libraries.
#


if (MSVC)
  set_SOURCE_FILES_PROPERTIES(${LIBCODA_SOURCES}
    COMPILE_FLAGS "/nologo /Gm /GX /ZI /D_MBCS /D_USRDLL /DLIBCODADLL_EXPORTS /DLIBCODADLL /DHAVE_CONFIG_H /YX /FD /c"
    )
endif (MSVC)


# Set dynamic library version

set(LIBCODA_CURRENT 0)
set(LIBCODA_REVISION 0)
set(LIBCODA_AGE 0)


#add_library(coda SHARED ${LIBCODA_SOURCES})
#target_link_libraries(coda expat ${ZLIB_LIBRARIES})
#set_target_properties(coda PROPERTIES
#  LINK_FLAGS "/VERBOSE:LIB"
#  VERSION ${LIBCODA_CURRENT}.${LIBCODA_REVISION}.${LIBCODA_AGE}
#  SOVERSION ${LIBCODA_CURRENT}
#)
#install(TARGETS coda DESTINATION lib)

add_library(coda-static STATIC ${LIBCODA_SOURCES})
target_link_libraries(coda-static expat ${ZLIB_LIBRARIES})
set_target_properties(coda-static PROPERTIES
  OUTPUT_NAME "coda"
)
install(TARGETS coda-static DESTINATION lib)


# tools codacheck

set(codacheck_SOURCES
  tools/codacheck/codacheck.c
  )
add_executable(codacheck ${codacheck_SOURCES})
target_link_libraries(codacheck coda-static)

# tools codacmp

set(codacmp_SOURCES
  tools/codacmp/codacmp.c
  )
add_executable(codacmp ${codacmp_SOURCES})
target_link_libraries(codacmp coda-static)

# tools/codadump

set(codadump_SOURCES
	tools/codadump/codadump-ascii.c
	tools/codadump/codadump-debug.c
	tools/codadump/codadump-dim.c
	tools/codadump/codadump-filter.c
	tools/codadump/codadump-traverse.c
	tools/codadump/codadump.c
	tools/codadump/codadump-filter.h
	tools/codadump/codadump.h
  )
set(codadump_hdf4_files
  tools/codadump/codadump-hdf4.c
  )
if (WITH_HDF4)
  set (codadump_SOURCES ${codadump_SOURCES} ${codadump_hdf4_files})
endif (WITH_HDF4)
add_executable(codadump ${codadump_SOURCES})
target_link_libraries(codadump coda-static)

# tools codafind

set(codafind_SOURCES
  tools/codafind/codafind.c
  )
add_executable(codafind ${codafind_SOURCES})
target_link_libraries(codafind coda-static)

# tools codadd

set(codadd_SOURCES
  tools/codadd/codadd.c
  )
add_executable(codadd ${codadd_SOURCES})
target_link_libraries(codadd coda-static)

# fortran

if (NOT SUBPACKAGE_MODE)

  set(fortran_DATA
    fortran/Makefile
	fortran/coda_fortran.c
	fortran/example.f
    )

  set(fortran_SCRIPTS
	fortran/checkf77.sh
    )
endif (NOT SUBPACKAGE_MODE)

set(EXTRA_DIST ${EXTRA_DIST}
	fortran/Makefile.in
	fortran/coda_fortran.c
	fortran/checkf77.sh
	fortran/example.f
    )

set(generate_finc_SOURCES
  fortran/generate-finc.c
  )

add_executable(generate-finc ${generate_finc_SOURCES})
target_link_libraries(generate-finc coda-static)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fortran/coda.inc
  MAIN_DEPENDENCY generate-finc
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generate-finc${EXEEXT} > ${CMAKE_CURRENT_BINARY_DIR}/fortran/coda/.inc
  )

# idl

if (BUILD_IDL)
  message("CMake IDL build not supported yet.")
endif (BUILD_IDL)

# matlab

set(CODA_MATLAB_SOURCE_FILES
  matlab/coda-matlab.h
  matlab/coda-matlab-getdata.c
  matlab/coda-matlab-traverse.c
  matlab/coda_matlab.c
  matlab/mxCreateDoubleScalar.c
  matlab/mxCreateNumericMatrix.c
  )

set(EXTRA_DIST ${EXTRA_DIST}
  ${CODA_MATLAB_SOURCE_FILES}
  )

if (BUILD_MATLAB)
  message(STATUS "CMake MATLAB build not supported yet.")
endif (BUILD_MATLAB)

# python

if (BUILD_PYTHON)

  find_package(Python)

  # Not including SWIG rules (yet)

  set(EXTRA_DIST
    ${EXTA_DIST}
    python/codac.c
	python/codac.py
	$(srcdir)/python/codac.i
	$(srcdir)/python/coda_typemaps.i
	$(srcdir)/python/coda_ignore.i
	$(srcdir)/python/coda_rename.i
    )

  message(FATAL_ERROR "CMake Python support not complete yet yet")

endif (BUILD_PYTHON)

# *** misc distribution files ***

set(EXTRA_DIST ${EXTRA_DIST}
  CHANGES
  COPYING
  FAQ
  INSTALL
  LICENSES
  README
  bootstrap
  codadef.sh
  )

# *** idl data ***

set(CODA_IDL_DIMFILES
  idl/coda-idl.dlm
  )

set(EXTRA_DIST ${EXTRA_DIST}
  idl/coda-idl.dlm.in
  )

if (NOT SUBPACKAGE_MODE)
  if (BUILD_IDL)
    set(idldlmexec_DATA ${CODA_IDL_DIMFILES})
  endif(BUILD_IDL)
endif(NOT SUBPACKAGE_MODE)

# *** matlab data ***

set(CODA_MATLAB_MFILES
	matlab/coda_attributes.m
	matlab/coda_class.m
	matlab/coda_clearall.m
	matlab/coda_close.m
	matlab/coda_description.m
	matlab/coda_fetch.m
	matlab/coda_fieldavailable.m
	matlab/coda_fieldcount.m
	matlab/coda_fieldnames.m
	matlab/coda_getopt.m
	matlab/coda_open.m
	matlab/coda_options.m
	matlab/coda_param.m
	matlab/coda_product_class.m
	matlab/coda_product_type.m
	matlab/coda_product_version.m
	matlab/coda_setopt.m
	matlab/coda_size.m
	matlab/coda_time_to_string.m
	matlab/coda_unit.m
	matlab/coda_version.m
    )

set(EXTRA_DIST ${EXTRA_DIST}
  ${CODA_MATLAB_MFILES}
  )

if (NOT SUBPACKAGE_MODE)
  if (BUILD_MATLAB)
    set(matlabmexexec_DATA ${CODA_MATLAB_MFILES})
  endif(BUILD_MATLAB)
endif(NOT SUBPACKAGE_MODE)

# *** documentation ***
#
# TBD

# *** indent ***
#
# Um, let's not, for now. :-)


# Windows stuff

# Notice that doing this on Linux does not copy the 
# DOS line endings (CMake bug 0008506)
CONFIGURE_FILE(win32/config.h.cmake.in win32/config.h)
CONFIGURE_FILE(win32/coda.iss.in win32/coda.iss)
CONFIGURE_FILE(win32/setup.py.in win32/setup.py)


