# CMake configuration file for Coda.
#
project(CODA)

set(VERSION 1.1.1)
set(CODA_VERSION \"${VERSION}\")

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

# Some autoconf variables that CMake does not use itself, but
# that are used in the config files.
#
set(PACKAGE_NAME \"${CMAKE_PROJECT_NAME}\")
string(TOLOWER ${PACKAGE_NAME} PACKAGE)
set(PACKAGE_VERSION ${CODA_VERSION})
string(REPLACE "\"" "" PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_STRING \"${PACKAGE_STRING}\")
set(VERSION \"${VERSION}\")
set(PACKAGE_TARNAME ${PACKAGE})
set(PACKAGE_BUGREPORT \"\")

# Global compilation settings
#
if (CMAKE_COMPILER_IS_GNUCC)
  set(GNU_WARNING_FLAGS "-W -Wall")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_WARNING_FLAGS}")
endif (CMAKE_COMPILER_IS_GNUCC)

# hdf
#
if (WITH_HDF4)

  find_package(HDF4)

  if (NOT HDF4_FOUND)
    message(FATAL_ERROR "HDF4 libraries and/or header files are not found. Try setting the HDF4_LIB and HDF4_INCLUDE environment variables to the location of your HDF4 library and include files.")
  else (NOT HDF4_FOUND)
    include_directories(${HDF4_INCLUDE_DIR})
  endif (NOT HDF4_FOUND)

endif (WITH_HDF4)


if (WITH_HDF5)

  find_package(HDF5)

  if (NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 libraries and/or header files are not found. Try setting the HDF5_LIB and HDF5_INCLUDE environment variables to the location of your HDF5 library and include files.")
  else (NOT HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIR})
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

# Lex / Yacc - adapted from CMake's own CMakeLists.txt
#
find_program(YACC_EXECUTABLE
  NAMES yacc bison
  PATHS /usr/bin
  DOC "Yacc or Bison executable")
find_program(FLEX_EXECUTABLE
  NAMES flex
  PATHS /usr/bin
  DOC "Flex executable")
mark_as_advanced(YACC_EXECUTABLE FLEX_EXECUTABLE)
if(YACC_EXECUTABLE)
  set(BISON_FLAGS)
  if(YACC_EXECUTABLE MATCHES "bison")
    set(BISON_FLAGS "--yacc --defines")
  endif(YACC_EXECUTABLE MATCHES "bison")
  add_custom_command(
    OUTPUT coda-expr-parser.c coda-expr-parser.h
    DEPENDS libcoda/coda-expr-parser.y
    COMMAND
    ${YACC_EXECUTABLE} --defines=coda-expr-parser.h -ocoda-expr-parser.c ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-parser.y
    )
  add_custom_target(RerunYacc DEPENDS coda-expr-parser.c)
endif(YACC_EXECUTABLE)

if(FLEX_EXECUTABLE)
  add_custom_command(
    OUTPUT coda-expr-tokenizer.c
    DEPENDS libcoda/coda-expr-tokenizer.l
    COMMAND
    ${FLEX_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-tokenizer.l
# Leo: copy lex.yy.c manually -- --outfile option to flex does not work,
# because it gets explicitly overriden in the .l file.
    COMMAND
    ${CMAKE_COMMAND} -E copy lex.yy.c coda-expr-tokenizer.c
    )
  add_custom_target(RerunLex DEPENDS code-expr-tokenizer.c)
endif(FLEX_EXECUTABLE)


# Required include files
#
set(INCLUDES "")

macro (find_include FILE VARIABLE)
  CHECK_INCLUDE_FILE(${FILE} ${VARIABLE})
  if (${VARIABLE})
    set(INCLUDES ${INCLUDES} ${FILE})
  endif (${VARIABLE})
endmacro (find_include)

find_include(dirent.h		HAVE_DIRENT_H)
find_include(dlfcn.h		HAVE_DLFCN_H)
find_include(export.h		HAVE_EXPORT_H)
find_include(inttypes.h		HAVE_INTTYPES_H)
find_include(memory.h		HAVE_MEMORY_H)
find_include(stdint.h		HAVE_STDINT_H)
find_include(stdlib.h		HAVE_STDLIB_H)
find_include(strings.h		HAVE_STRINGS_H)
find_include(string.h		HAVE_STRING_H)
find_include(sys/mman.h	    HAVE_SYS_MMAN_H)
find_include(sys/stat.h		HAVE_SYS_STAT_H)
find_include(sys/types.h	HAVE_SYS_TYPES_H)
find_include(unistd.h		HAVE_UNISTD_H)

set(CMAKE_EXTRA_INCLUDE_FILES ${INCLUDES})

# Required functions
#
check_function_exists(bcopy HAVE_BCOPY)
check_function_exists(floor HAVE_FLOOR)
check_function_exists(getpagesize HAVE_GETPAGESIZE)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(pread HAVE_PREAD)
check_function_exists(realloc HAVE_REALLOC)
check_function_exists(stat HAVE_STAT)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(vsnprintf HAVE_VSNPRINTF)

if (WIN32)
  # Hack for Windows
  set (HAVE_STRDUP 1)
  set (HAVE_STRCASECMP 1)
  set (HAVE_STRNCASECMP 1)
  set (HAVE_VSNPRINTF 1)
endif (WIN32)


# Required types (and their sizes)
#
check_type_exists(uchar "${INCLUDES}" HAVE_UCHAR)
set(UCHAR "unsigned char")
check_type_exists(size_t "${INCLUDES}" HAVE_SIZE_T)
set(SIZE_T "long")
check_type_exists(ptrdiff_t "${INCLUDES}" HAVE_PTRDIFF_T)
set(PTRDIFF_T "int")

check_type_size(size_t SIZEOF_SIZE_T)
check_type_size(off_t SIZEOF_OFF_T)
check_type_size(double SIZEOF_DOUBLE)
check_type_size(float SIZEOF_FLOAT)
check_type_size(int SIZEOF_INT)
check_type_size(long SIZEOF_LONG)
check_type_size(short SIZEOF_SHORT)

test_big_endian(WORDS_BIGENDIAN)

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
#
add_definitions(-DHAVE_CONFIG_H)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in 
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda.h.cmake.in 
  ${CMAKE_CURRENT_BINARY_DIR}/coda.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/idl/coda-idl.dlm.in 
  ${CMAKE_CURRENT_BINARY_DIR}/idl/coda-idl.dlm)


# The Doxyfile uses some autoconf variables CMake does not have.
# We set them here manually, so that we can use the same .in
# file for both autoconf and cmake
#
set(top_builddir ${CMAKE_BINARY_DIR})
set(top_srcdir ${CMAKE_SOURCE_DIR})
set(srcdir ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/doc)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/doc/Doxyfile.in 
  ${CMAKE_CURRENT_BINARY_DIR}/libcoda/doc/Doxyfile)


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

set(LIBCODA_HDF4_FILES
  libcoda/coda-hdf4-cursor.c
  libcoda/coda-hdf4-definition.c
  libcoda/coda-hdf4-internal.h
  libcoda/coda-hdf4-type.c
  libcoda/coda-hdf4.h
)

if (WITH_HDF4)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF4_FILES})
endif (WITH_HDF4)


set(LIBCODA_HDF5_FILES
  libcoda/coda-hdf5-cursor.c
  libcoda/coda-hdf5-definition.c
  libcoda/coda-hdf5-internal.h
  libcoda/coda-hdf5-type.c
  libcoda/coda-hdf5.h
  )

if (WITH_HDF5)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF5_FILES})
endif (WITH_HDF5)


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

set(generate_finc_SOURCES
  fortran/generate-finc.c
  )

add_executable(generate-finc ${generate_finc_SOURCES})
target_link_libraries(generate-finc)
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

if (BUILD_MATLAB)
  message(STATUS "CMake MATLAB build not supported yet.")
endif (BUILD_MATLAB)

# python

set(CODA_PYTHON_SOURCE_FILES
  python/codac.c
)

if (BUILD_PYTHON)
  find_package(Python)

  if (NOT PYTHON)
    message(FATAL_ERROR "PYTHON interpreter and/or header files are not found. Try setting the PYTHON and/or PYTHON_INCLUDE environment variables to the location of your PYTHON interpreter and include files.")
  else (NOT PYTHON)
    include_directories(${PYTHON_INCLUDE})
  endif (NOT PYTHON)

  find_package(NumPy)

  if (NOT NUMPY_FOUND)
    message(FATAL_ERROR "Python numpy package is needed for the Python interface. Please install 'numpy' or, if you have already installed this package, set the NUMPY_INCLUDE environment variable and make sure that the file $NUMPY_INCLUDE/numpy/ndarrayobject.h exists.")
  else (NOT NUMPY_FOUND)
    include_directories(${NUMPY_INCLUDE_DIR})
  endif (NOT NUMPY_FOUND)



  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${CODA_PYTHON_SOURCE_FILES})




endif (BUILD_PYTHON)

# *** idl data ***

set(CODA_IDL_DIMFILES
  idl/coda-idl.dlm
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

if (NOT SUBPACKAGE_MODE)
  if (BUILD_MATLAB)
    set(matlabmexexec_DATA ${CODA_MATLAB_MFILES})
  endif(BUILD_MATLAB)
endif(NOT SUBPACKAGE_MODE)

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


# Custom CODA zlib library
#
set(LIBZLIB_SOURCES
  libcoda/zlib/adler32.c
  libcoda/zlib/coda_zlib_mangle.h
  libcoda/zlib/crc32.c
  libcoda/zlib/crc32.h
  libcoda/zlib/inffast.c
  libcoda/zlib/inffast.h
  libcoda/zlib/inffixed.h
  libcoda/zlib/inflate.c
  libcoda/zlib/inflate.h
  libcoda/zlib/inftrees.c
  libcoda/zlib/inftrees.h
  libcoda/zlib/zconf.h
  libcoda/zlib/zlib.h
  libcoda/zlib/zutil.c
  libcoda/zlib/zutil.h
)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/zlib)
add_library(z STATIC ${LIBZLIB_SOURCES})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/coda.h DESTINATION include)

# Do not build the dynamic version for now.
#
# Set dynamic library version
#
set(LIBCODA_CURRENT 0)
set(LIBCODA_REVISION 0)
set(LIBCODA_AGE 1)

add_library(coda SHARED ${LIBCODA_SOURCES})
target_link_libraries(coda expat z ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
set_target_properties(coda PROPERTIES
  VERSION ${LIBCODA_CURRENT}.${LIBCODA_REVISION}.${LIBCODA_AGE}
  SOVERSION ${LIBCODA_CURRENT}
)
install(TARGETS coda DESTINATION lib)

add_library(coda_static STATIC ${LIBCODA_SOURCES})
target_link_libraries(coda_static expat z)
set_target_properties(coda_static PROPERTIES
  OUTPUT_NAME "coda"
)
install(TARGETS coda_static DESTINATION lib)

if (MSVC)
  set_source_files_properties(coda-expr-tokenizer.c PROPERTIES
    COMPILE_FLAGS "/D YY_NO_UNISTD_H"
    )
endif (MSVC)


# tools codacheck

set(codacheck_SOURCES
  tools/codacheck/codacheck.c
  )
add_executable(codacheck ${codacheck_SOURCES})
target_link_libraries(codacheck coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
install(TARGETS codacheck DESTINATION bin)

if (MSVC)
  set_target_properties(codacheck PROPERTIES
    COMPILE_FLAGS "/nologo /GX /D_CONSOLE /D_MBCS /YX /FD"
    )
endif (MSVC)

# tools codacmp

set(codacmp_SOURCES
  tools/codacmp/codacmp.c
  )
add_executable(codacmp ${codacmp_SOURCES})
target_link_libraries(codacmp coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
install(TARGETS codacmp DESTINATION bin)

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
target_link_libraries(codadump coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
install(TARGETS codadump DESTINATION bin)

# tools codafind

set(codafind_SOURCES
  tools/codafind/codafind.c
  )
add_executable(codafind ${codafind_SOURCES})
target_link_libraries(codafind coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
install(TARGETS codafind DESTINATION bin)

# tools codadd

set(codadd_SOURCES
  tools/codadd/codadd.c
  tools/codadd/codadd-doc.c
  tools/codadd/codadd-list.c
  )
add_executable(codadd ${codadd_SOURCES})
target_link_libraries(codadd coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
install(TARGETS codadd DESTINATION bin)


# Windows stuff

# Notice that doing this on Linux does not copy the
# DOS line endings (CMake bug 0008506)
#
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/win32/coda.iss.in 
  ${CMAKE_CURRENT_BINARY_DIR}/win32/coda.iss)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/win32/setup.py.in 
  ${CMAKE_CURRENT_BINARY_DIR}/win32/setup.py)

