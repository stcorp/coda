cmake_minimum_required(VERSION 2.8.8)
if(POLICY CMP0075)
  cmake_policy(SET CMP0075 NEW)
endif()

project(CODA)

set(VERSION 2.19)
set(CODA_VERSION \"${VERSION}\")
set(CPACK_PACKAGE_VERSION ${VERSION})

# Set dynamic library version
set(LIBCODA_CURRENT 14)
set(LIBCODA_REVISION 4)
set(LIBCODA_AGE 0)
math(EXPR LIBCODA_MAJOR "${LIBCODA_CURRENT} - ${LIBCODA_AGE}")
set(LIBCODA_MINOR ${LIBCODA_AGE})


# General setup
set(CMAKE_MODULE_PATH
  ${CODA_SOURCE_DIR}/CMakeModules
  ${CMAKE_MODULE_PATH}
  CACHE INTERNAL "Local CMake modules")

include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(TestBigEndian)
include(CMakeDependentOption)

if(NOT MSVC AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_C_FLAGS)
  set(CMAKE_C_FLAGS "-O2 -g"
      CACHE STRING "Flags used by the C compiler during all build types." FORCE)
endif(NOT MSVC AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_C_FLAGS)

# Option handling
#
option(CODA_BUILD_SUBPACKAGE_MODE "build CODA as sub-package" OFF)
option(CODA_BUILD_IDL "build IDL interface" OFF)
option(CODA_BUILD_MATLAB "build MATLAB interface" OFF)
option(CODA_BUILD_PYTHON "build Python interface" OFF)
option(CODA_BUILD_JAVA "build Java interface" OFF)
option(CODA_WITH_HDF4 "use HDF4" OFF)
option(CODA_WITH_HDF5 "use HDF5" OFF)
option(CODA_ENABLE_CONDA_INSTALL OFF)
option(CODA_MACOSX_RPATH ON)

set(CODA_EXPAT_NAME_MANGLE 1)
set(CMAKE_MACOSX_RPATH ${CODA_MACOSX_RPATH})

cmake_dependent_option(
  CODA_ENABLE_HDF4_VDATA_ATTRIBUTES "enable HDF4 Vdata attributes" ON
  CODA_WITH_HDF4 ON)
if(CODA_ENABLE_HDF4_VDATA_ATTRIBUTES)
  set(ENABLE_HDF4_VDATA_ATTRIBUTES 1)
endif(CODA_ENABLE_HDF4_VDATA_ATTRIBUTES)

# settings for expat
set(XML_NS 1)
set(XML_DTD 1)
set(XML_LARGE_SIZE 1)
set(XML_CONTEXT_BYTES 1024)

if(MSVC)
  # Suppress warnings from VC on deprecated POSIX names and unsafe functions
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT CODA_BUILD_SUBPACKAGE_MODE)
  if(CODA_BUILD_PYTHON)
    find_package(PythonInterp)
    if(NOT PYTHONINTERP_FOUND)
      message(FATAL_ERROR "PYTHON interpreter not found.")
    endif(NOT PYTHONINTERP_FOUND)
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.get_python_lib(1,0,prefix=''))" OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
    set(CODA_PYTHON_INSTALL_DIR "${PYTHON_INSTALL_DIR}" CACHE STRING "Install location for CODA Python package")
  endif()

  set(BIN_PREFIX bin)
  set(LIB_PREFIX lib)
  set(INCLUDE_PREFIX include)
  set(DOC_PREFIX share/coda/doc)
  set(PYTHON_PREFIX ${CODA_PYTHON_INSTALL_DIR}/coda)
  set(MATLAB_PREFIX lib/coda/matlab)
  set(IDL_PREFIX lib/coda/idl)
  set(JAVA_PREFIX share/coda/java)
  if(WIN32)
    # we use a different installation directory structure on Windows
    if(CODA_ENABLE_CONDA_INSTALL)
      # all non-python components need to be installed in a Library sub-directory for Conda on Windows
      set(BIN_PREFIX Library/bin)
      set(LIB_PREFIX Library/lib)
      set(INCLUDE_PREFIX Library/include)
      set(DOC_PREFIX Library/share/coda/doc)
      set(MATLAB_PREFIX Library/lib/coda/matlab)
      set(IDL_PREFIX Library/lib/coda/idl)
      set(JAVA_PREFIX Library/share/coda/java)
    else(CODA_ENABLE_CONDA_INSTALL)
      set(DOC_PREFIX doc)
      set(PYTHON_PREFIX python/coda)
    endif(CODA_ENABLE_CONDA_INSTALL)
  endif(WIN32)
endif(NOT CODA_BUILD_SUBPACKAGE_MODE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_PREFIX}")

if(NOT MSVC)
  set(MATHLIB m)
endif(NOT MSVC)

set(INCLUDES "")

macro(find_include FILE VARIABLE)
  CHECK_INCLUDE_FILE(${FILE} ${VARIABLE})
  if(${VARIABLE})
    set(INCLUDES ${INCLUDES} ${FILE})
  endif(${VARIABLE})
endmacro(find_include)

find_include(dirent.h HAVE_DIRENT_H)
find_include(dlfcn.h HAVE_DLFCN_H)
find_include(export.h HAVE_EXPORT_H)
find_include(inttypes.h HAVE_INTTYPES_H)
find_include(limits.h HAVE_LIMITS_H)
find_include(memory.h HAVE_MEMORY_H)
find_include(stdarg.h HAVE_STDARG_H)
find_include(stdint.h HAVE_STDINT_H)
find_include(stdlib.h HAVE_STDLIB_H)
find_include(string.h HAVE_STRING_H)
find_include(strings.h HAVE_STRINGS_H)
find_include(sys/mman.h HAVE_SYS_MMAN_H)
find_include(sys/socket.h HAVE_SYS_SOCKET_H)
find_include(sys/stat.h HAVE_SYS_STAT_H)
find_include(sys/types.h HAVE_SYS_TYPES_H)
find_include(unistd.h HAVE_UNISTD_H)

set(CMAKE_EXTRA_INCLUDE_FILES ${INCLUDES})

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
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(vsnprintf HAVE_VSNPRINTF)

check_symbol_exists(uchar "${INCLUDES}" HAVE_UCHAR)
set(UCHAR "unsigned char")
check_symbol_exists(size_t "${INCLUDES}" HAVE_SIZE_T)
set(SIZE_T "long")
check_symbol_exists(ptrdiff_t "${INCLUDES}" HAVE_PTRDIFF_T)
set(PTRDIFF_T "int")

check_type_size(size_t SIZEOF_SIZE_T)
check_type_size(off_t SIZEOF_OFF_T)
check_type_size(double SIZEOF_DOUBLE)
check_type_size(float SIZEOF_FLOAT)
check_type_size(int SIZEOF_INT)
check_type_size(long SIZEOF_LONG)
check_type_size(short SIZEOF_SHORT)

test_big_endian(WORDS_BIGENDIAN)

# Check for thread-local storage (TLS) keyword
if(NOT DEFINED THREAD_LOCAL)
  message(STATUS "Check for thread local keyword")
  foreach(keyword IN ITEMS "thread_local" "_Thread_local" "__thread" "__declspec(thread)")
    set(CMAKE_REQUIRED_QUIET True)
    check_c_source_compiles("int main(void) { static ${keyword} int bar; return 0; }" Thread_Local_Keyword)
    if(${Thread_Local_Keyword})
      unset(Thread_Local_Keyword CACHE)
      set(THREAD_LOCAL ${keyword} CACHE STRING "Thread-local storage (TLS) keyword")
      message(STATUS "Check for thread local keyword - ${keyword}")
      break()
    endif()
    unset(Thread_Local_Keyword CACHE)
  endforeach()
  if(NOT DEFINED THREAD_LOCAL)
    message(STATUS "Check for thread local keyword - not found")
    set(THREAD_LOCAL "" CACHE STRING "Thread-local storage (TLS) keyword")
  endif()
endif()

if(HAVE_STDLIB_H AND HAVE_STDDEF_H)
  set(STDC_HEADERS 1)
endif(HAVE_STDLIB_H AND HAVE_STDDEF_H)

# This is needed for the embedded zlib
if(NOT HAVE_STRERROR)
  set(NO_STRERROR 1)
endif(NOT HAVE_STRERROR)

if(MSVC AND MSVC_VERSION LESS 1600 AND NOT MINGW)
# For Visual Studio > 2010 and MinGW we can use stdint.h
# For earlier versions of Visual Studio we need to provide our own defines
set(int8_t "signed char")
set(int16_t "signed short")
set(int32_t "int")
set(int64_t "__int64")
set(uint8_t "unsigned char")
set(uint16_t "unsigned short")
set(uint32_t "unsigned int")
set(uint64_t "unsigned __int64")
endif(MSVC AND MSVC_VERSION LESS 1600 AND NOT MINGW)

find_package(BISON)
find_package(FLEX)

if(CODA_WITH_HDF4)
  find_package(HDF4)
  if(NOT HDF4_FOUND)
    message(FATAL_ERROR "HDF4 libraries and/or header files are not found. Try setting the HDF4_LIBRARY_DIR and HDF4_INCLUDE_DIR cmake variables to the location of your HDF4 library and include files.")
  else(NOT HDF4_FOUND)
    set(HAVE_HDF4 1)
    include_directories(${HDF4_INCLUDE_DIR})
  endif(NOT HDF4_FOUND)
endif(CODA_WITH_HDF4)

if(CODA_WITH_HDF5)
  find_package(HDF5)
  if(NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 libraries and/or header files are not found. Try setting the HDF5_LIBRARY_DIR and HDF5_INCLUDE_DIR cmake variables to the location of your HDF5 library and include files.")
  else(NOT HDF5_FOUND)
    set(HAVE_HDF5 1)
    include_directories(${HDF5_INCLUDE_DIR})
  endif(NOT HDF5_FOUND)
endif(CODA_WITH_HDF5)

include_directories(BEFORE ${CODA_BINARY_DIR} ${CODA_BINARY_DIR}/libcoda ${CODA_SOURCE_DIR}/libcoda)

add_definitions(-DHAVE_CONFIG_H)

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


if(CMAKE_COMPILER_IS_GNUCC)
  set(GNU_WARNING_FLAGS "-W -Wall -Wextra")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_WARNING_FLAGS}")
  if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    # make sure that the coda static library can be linked into a shared library
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
  endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
endif(CMAKE_COMPILER_IS_GNUCC)


# libcoda sources
#

BISON_TARGET(coda-expr-parser ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-parser.y ${CMAKE_CURRENT_BINARY_DIR}/libcoda/coda-expr-parser.c)
FLEX_TARGET(coda-expr-tokenizer ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda-expr-tokenizer.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
add_flex_bison_dependency(coda-expr-tokenizer coda-expr-parser)

set(LIBCODA_SOURCES
  libcoda/coda-ascbin-cursor.c
  libcoda/coda-ascbin.h
  libcoda/coda-ascii-cursor.c
  libcoda/coda-ascii-internal.h
  libcoda/coda-ascii.c
  libcoda/coda-ascii.h
  libcoda/coda-bin-cursor.c
  libcoda/coda-bin-internal.h
  libcoda/coda-bin.c
  libcoda/coda-bin.h
  libcoda/coda-cdf-cursor.c
  libcoda/coda-cdf-internal.h
  libcoda/coda-cdf-type.c
  libcoda/coda-cdf.c
  libcoda/coda-cdf.h
  libcoda/coda-check.c
  libcoda/coda-cursor-read.c
  libcoda/coda-cursor.c
  libcoda/coda-definition-parse.c
  libcoda/coda-definition.c
  libcoda/coda-definition.h
  libcoda/coda-detection.c
  libcoda/coda-errno.c
  libcoda/coda-expr.c
  libcoda/coda-expr.h
  libcoda/coda-filefilter.c
  libcoda/coda-filefilter.h
  libcoda/coda-grib-cursor.c
  libcoda/coda-grib-internal.h
  libcoda/coda-grib-type.c
  libcoda/coda-grib.c
  libcoda/coda-grib.h
  libcoda/coda-internal.h
  libcoda/coda-mem-cursor.c
  libcoda/coda-mem-internal.h
  libcoda/coda-mem-type.c
  libcoda/coda-mem.c
  libcoda/coda-mem.h
  libcoda/coda-netcdf-cursor.c
  libcoda/coda-netcdf-internal.h
  libcoda/coda-netcdf-type.c
  libcoda/coda-netcdf.c
  libcoda/coda-netcdf.h
  libcoda/coda-path.h
  libcoda/coda-product.c
  libcoda/coda-read-array.h
  libcoda/coda-read-bits.h
  libcoda/coda-read-bytes.h
  libcoda/coda-read-bytes-in-bounds.h
  libcoda/coda-read-partial-array.h
  libcoda/coda-rinex.c
  libcoda/coda-rinex.h
  libcoda/coda-sp3.c
  libcoda/coda-sp3.h
  libcoda/coda-swap2.h
  libcoda/coda-swap4.h
  libcoda/coda-swap8.h
  libcoda/coda-time.c
  libcoda/coda-transpose-array.h
  libcoda/coda-tree.c
  libcoda/coda-tree.h
  libcoda/coda-type.c
  libcoda/coda-type.h
  libcoda/coda-utils.c
  libcoda/coda-xml-internal.h
  libcoda/coda-xml-parser.c
  libcoda/coda-xml.c
  libcoda/coda-xml.h
  libcoda/coda.c
  libcoda/hashtable.c
  libcoda/hashtable.h
  libcoda/ipow.h
  libcoda/ziparchive.c
  libcoda/ziparchive.h
  ${BISON_coda-expr-parser_OUTPUTS}
  ${FLEX_coda-expr-tokenizer_OUTPUTS})

set(LIBCODA_HDF4_FILES
  libcoda/coda-hdf4-cursor.c
  libcoda/coda-hdf4-internal.h
  libcoda/coda-hdf4-type.c
  libcoda/coda-hdf4.c
  libcoda/coda-hdf4.h)
if(CODA_WITH_HDF4)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF4_FILES})
endif(CODA_WITH_HDF4)

set(LIBCODA_HDF5_FILES
  libcoda/coda-hdf5-cursor.c
  libcoda/coda-hdf5-internal.h
  libcoda/coda-hdf5-type.c
  libcoda/coda-hdf5.c
  libcoda/coda-hdf5.h)
if(CODA_WITH_HDF5)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF5_FILES})
endif(CODA_WITH_HDF5)

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
  libcoda/expat/xmltok_impl.h)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/expat)

set(LIBPCRE_SOURCES
  libcoda/pcre/coda_pcre_config.h
  libcoda/pcre/coda_pcre_mangle.h
  libcoda/pcre/pcre.h
  libcoda/pcre/pcre_byte_order.c
  libcoda/pcre/pcre_chartables.c
  libcoda/pcre/pcre_compile.c
  libcoda/pcre/pcre_config.c
  libcoda/pcre/pcre_dfa_exec.c
  libcoda/pcre/pcre_exec.c
  libcoda/pcre/pcre_fullinfo.c
  libcoda/pcre/pcre_get.c
  libcoda/pcre/pcre_globals.c
  libcoda/pcre/pcre_internal.h
  libcoda/pcre/pcre_maketables.c
  libcoda/pcre/pcre_newline.c
  libcoda/pcre/pcre_ord2utf8.c
  libcoda/pcre/pcre_printint.c
  libcoda/pcre/pcre_refcount.c
  libcoda/pcre/pcre_string_utils.c
  libcoda/pcre/pcre_study.c
  libcoda/pcre/pcre_tables.c
  libcoda/pcre/pcre_ucd.c
  libcoda/pcre/pcre_valid_utf8.c
  libcoda/pcre/pcre_version.c
  libcoda/pcre/pcre_xclass.c
  libcoda/pcre/ucp.h)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/pcre)

set(LIBZLIB_SOURCES
  libcoda/zlib/adler32.c
  libcoda/zlib/coda_zlib_mangle.h
  libcoda/zlib/crc32.c
  libcoda/zlib/crc32.h
  libcoda/zlib/gzguts.h
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
  libcoda/zlib/zutil.h)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/zlib)

add_library(coda_static STATIC ${LIBCODA_SOURCES} ${LIBEXPAT_SOURCES} ${LIBPCRE_SOURCES} ${LIBZLIB_SOURCES})
# On Windows, we want libcoda.lib for static, coda.dll & coda.lib for shared.
# On Unix, we want libcoda.a and libcoda.so
if(WIN32)
  set_target_properties(coda_static PROPERTIES OUTPUT_NAME "libcoda")
  if(CODA_WITH_HDF5)
    # Also set H5_BUILT_AS_DYNAMIC_LIB since we link against the dynamic HDF5 library
    set_target_properties(coda_static PROPERTIES COMPILE_FLAGS "-DH5_BUILT_AS_DYNAMIC_LIB -DXML_BUILDING_EXPAT -DYY_NO_UNISTD_H")
  else(CODA_WITH_HDF5)
    set_target_properties(coda_static PROPERTIES COMPILE_FLAGS "-DXML_BUILDING_EXPAT -DYY_NO_UNISTD_H")
  endif(CODA_WITH_HDF5)
else(WIN32)
  set_target_properties(coda_static PROPERTIES OUTPUT_NAME "coda")
endif(WIN32)

if(NOT CODA_BUILD_SUBPACKAGE_MODE)

  add_library(coda SHARED ${LIBCODA_SOURCES} ${LIBEXPAT_SOURCES} ${LIBPCRE_SOURCES} ${LIBZLIB_SOURCES})
  target_link_libraries(coda ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
  set_target_properties(coda PROPERTIES
    VERSION ${LIBCODA_MAJOR}.${LIBCODA_MINOR}.${LIBCODA_REVISION}
    SOVERSION ${LIBCODA_MAJOR})
  if(WIN32)
    if(CODA_WITH_HDF5)
      # Also set H5_BUILT_AS_DYNAMIC_LIB since we link against the dynamic HDF5 library
      set_target_properties(coda PROPERTIES COMPILE_FLAGS "-DH5_BUILT_AS_DYNAMIC_LIB -DXML_BUILDING_EXPAT -DLIBCODADLL -DLIBCODADLL_EXPORTS -DYY_NO_UNISTD_H")
    else(CODA_WITH_HDF5)
      set_target_properties(coda PROPERTIES COMPILE_FLAGS "-DXML_BUILDING_EXPAT -DLIBCODADLL -DLIBCODADLL_EXPORTS -DYY_NO_UNISTD_H")
    endif(CODA_WITH_HDF5)
    install(TARGETS coda RUNTIME DESTINATION ${BIN_PREFIX} ARCHIVE DESTINATION ${LIB_PREFIX})
  else(WIN32)
    install(TARGETS coda DESTINATION ${LIB_PREFIX})
  endif(WIN32)
  install(TARGETS coda_static DESTINATION ${LIB_PREFIX})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/coda.h DESTINATION ${INCLUDE_PREFIX})

  # codacheck
  set(codacheck_SOURCES tools/codacheck/codacheck.c)
  add_executable(codacheck ${codacheck_SOURCES})
  target_link_libraries(codacheck coda ${MATHLIB})
  if(WIN32)
    set_target_properties(codacheck PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
  endif(WIN32)
  install(TARGETS codacheck DESTINATION ${BIN_PREFIX})

  # codacmp
  set(codacmp_SOURCES tools/codacmp/codacmp.c)
  add_executable(codacmp ${codacmp_SOURCES})
  # we need to link against coda_static because codacmp uses internal coda functions
  target_link_libraries(codacmp coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${MATHLIB})
  install(TARGETS codacmp DESTINATION ${BIN_PREFIX})

  # codadump
  set(codadump_SOURCES
      tools/codadump/codadump-ascii.c
      tools/codadump/codadump-debug.c
      tools/codadump/codadump-dim.c
      tools/codadump/codadump-filter.c
      tools/codadump/codadump-json.c
      tools/codadump/codadump-traverse.c
      tools/codadump/codadump-yaml.c
      tools/codadump/codadump.c
      tools/codadump/codadump-filter.h
      tools/codadump/codadump.h)
  set(codadump_hdf4_files tools/codadump/codadump-hdf4.c)
  if(CODA_WITH_HDF4)
    set(codadump_SOURCES ${codadump_SOURCES} ${codadump_hdf4_files})
  endif(CODA_WITH_HDF4)
  add_executable(codadump ${codadump_SOURCES})
  target_link_libraries(codadump coda ${HDF4_LIBRARIES} ${MATHLIB})
  if(WIN32)
    set_target_properties(codadump PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
  endif(WIN32)
  install(TARGETS codadump DESTINATION ${BIN_PREFIX})

  # codaeval
  set(codaeval_SOURCES tools/codaeval/codaeval.c)
  add_executable(codaeval ${codaeval_SOURCES})
  target_link_libraries(codaeval coda ${MATHLIB})
  if(WIN32)
    set_target_properties(codaeval PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
  endif(WIN32)
  install(TARGETS codaeval DESTINATION ${BIN_PREFIX})

  # codafind
  set(codafind_SOURCES tools/codafind/codafind.c)
  add_executable(codafind ${codafind_SOURCES})
  target_link_libraries(codafind coda ${MATHLIB})
  if(WIN32)
    set_target_properties(codafind PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
  endif(WIN32)
  install(TARGETS codafind DESTINATION ${BIN_PREFIX})

endif(NOT CODA_BUILD_SUBPACKAGE_MODE)

# codadd
set(codadd_SOURCES
  tools/codadd/codadd.c
  tools/codadd/codadd-definition.c
  tools/codadd/codadd-detect.c
  tools/codadd/codadd-doc.c
  tools/codadd/codadd-list.c
  tools/codadd/codadd-xmlschema.c)
add_executable(codadd ${codadd_SOURCES})
target_link_libraries(codadd coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${MATHLIB})
if(NOT CODA_BUILD_SUBPACKAGE_MODE)
  install(TARGETS codadd DESTINATION ${BIN_PREFIX})
endif(NOT CODA_BUILD_SUBPACKAGE_MODE)


if(NOT CODA_BUILD_SUBPACKAGE_MODE)

  # fortran
  set(fortran_DATA
    fortran/Makefile
    fortran/coda.inc
    fortran/coda_fortran.c
    fortran/example.f)
  set(fortran_SCRIPTS fortran/checkf77.sh)

  # idl
  if(CODA_BUILD_IDL)
    find_package(IDL)
    if(NOT IDL_FOUND)
      message(FATAL_ERROR "IDL installation not found.")
    endif(NOT IDL_FOUND)
    include_directories(AFTER ${IDL_INCLUDE_DIRS})
    set(CODA_IDL_SOURCES idl/coda_idl.c idl/coda_idl.def)
    add_library(coda_idl MODULE ${CODA_IDL_SOURCES})
    set_target_properties(coda_idl PROPERTIES PREFIX "")
    target_link_libraries(coda_idl coda ${IDL_LIBRARIES})
    if(WIN32)
      set_target_properties(coda_idl PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
    endif(WIN32)
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/idl/coda_idl.dlm.in
      ${CMAKE_CURRENT_BINARY_DIR}/idl/coda_idl.dlm)
    install(TARGETS coda_idl DESTINATION ${IDL_PREFIX})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/idl/coda_idl.dlm DESTINATION ${IDL_PREFIX})
  endif(CODA_BUILD_IDL)

  # matlab
  if(CODA_BUILD_MATLAB)
    set(Matlab_FIND_COMPONENTS MX_LIBRARY)
    find_package(MATLAB)
    if(NOT MATLAB_FOUND)
      message(FATAL_ERROR "MATLAB installation not found.")
    endif(NOT MATLAB_FOUND)
    include_directories(AFTER ${Matlab_INCLUDE_DIRS})
    set(CODA_MATLAB_SOURCES
      matlab/coda-matlab-getdata.c
      matlab/coda-matlab-traverse.c
      matlab/coda_matlab.c
      matlab/coda_matlab.def)
    add_library(coda_matlab MODULE ${CODA_MATLAB_SOURCES})
    set_target_properties(coda_matlab PROPERTIES
      PREFIX ""
      SUFFIX ".${Matlab_MEX_EXTENSION}"
      COMPILE_DEFINITIONS MATLAB_MEX_FILE)
    target_link_libraries(coda_matlab coda ${Matlab_LIBRARIES})
    if(WIN32)
      set_target_properties(coda_matlab PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
    endif(WIN32)
    set(CODA_MATLAB_M_FILES
      matlab/coda_attributes.m
      matlab/coda_class.m
      matlab/coda_clearall.m
      matlab/coda_close.m
      matlab/coda_description.m
      matlab/coda_eval.m
      matlab/coda_fetch.m
      matlab/coda_fieldavailable.m
      matlab/coda_fieldcount.m
      matlab/coda_fieldnames.m
      matlab/coda_getopt.m
      matlab/coda_open.m
      matlab/coda_open_as.m
      matlab/coda_options.m
      matlab/coda_param.m
      matlab/coda_product_class.m
      matlab/coda_product_type.m
      matlab/coda_product_version.m
      matlab/coda_setopt.m
      matlab/coda_size.m
      matlab/coda_time_to_string.m
      matlab/coda_unit.m
      matlab/coda_version.m)
    install(TARGETS coda_matlab DESTINATION ${MATLAB_PREFIX})
    install(FILES ${CODA_MATLAB_M_FILES} DESTINATION ${MATLAB_PREFIX})
  endif(CODA_BUILD_MATLAB)

  # python
  if(CODA_BUILD_PYTHON)
    find_package(PythonLibs)
    if(NOT PYTHONLIBS_FOUND)
      message(FATAL_ERROR "PYTHON libraries or include files not found.")
    endif(NOT PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
    find_package(NumPy)
    if(NOT NUMPY_FOUND)
      message(FATAL_ERROR "Python numpy package is needed for the Python interface. Please install 'numpy' or, if you have already installed this package, set the NUMPY_INCLUDE environment variable and make sure that the file $NUMPY_INCLUDE/numpy/ndarrayobject.h exists.")
    endif(NOT NUMPY_FOUND)
    include_directories(${NUMPY_INCLUDE_DIR})
    add_library(coda_python MODULE python/codac.c)
    set_target_properties(coda_python PROPERTIES
      OUTPUT_NAME "_codac"
      PREFIX "")
    if(WIN32)
      set_target_properties(coda_python PROPERTIES SUFFIX .pyd)
    endif()
    target_link_libraries(coda_python coda ${PYTHON_LIBRARIES})
    if(WIN32)
      set_target_properties(coda_python PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
    endif(WIN32)
    install(TARGETS coda_python DESTINATION ${PYTHON_PREFIX})
    install(FILES
      python/__init__.py
      python/codac.py
      python/codapython.py
      DESTINATION ${PYTHON_PREFIX})
  endif(CODA_BUILD_PYTHON)

  # Java
  if(CODA_BUILD_JAVA)
    find_package(Java 1.8)
    if(NOT Java_FOUND)
      message(FATAL_ERROR "Java not found.")
    endif(NOT Java_FOUND)
    find_package(JNI)
    if(NOT JNI_FOUND)
      message(FATAL_ERROR "Java JNI interfaces not found. Make sure you have the full JDK installed, not just the Runtime Environment (JRE).")
    endif(NOT JNI_FOUND)
    include(UseJava)
    include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
    add_library(coda_jni MODULE java/coda_jni.c)
    target_link_libraries(coda_jni coda)
    if(WIN32)
      set_target_properties(coda_jni PROPERTIES COMPILE_FLAGS "-DLIBCODADLL")
    endif(WIN32)
    if(APPLE)
      set_target_properties(coda_jni PROPERTIES SUFFIX ".jnilib")
    endif(APPLE)
    set(CODA_JAVA_FILES
      java/nl/stcorp/coda/CodaException.java
      java/nl/stcorp/coda/Product.java
      java/nl/stcorp/coda/Type.java
      java/nl/stcorp/coda/Cursor.java
      java/nl/stcorp/coda/Expression.java
      java/nl/stcorp/coda/Coda.java
      java/nl/stcorp/coda/ArrayOrderingEnum.java
      java/nl/stcorp/coda/FilefilterStatusEnum.java
      java/nl/stcorp/coda/FormatEnum.java
      java/nl/stcorp/coda/NativeTypeEnum.java
      java/nl/stcorp/coda/SpecialTypeEnum.java
      java/nl/stcorp/coda/ExpressionTypeEnum.java
      java/nl/stcorp/coda/TypeClassEnum.java
      java/nl/stcorp/coda/codacConstants.java
      java/nl/stcorp/coda/codac.java
      java/nl/stcorp/coda/codacJNI.java
      java/nl/stcorp/coda/SWIGTYPE_p_coda_product_struct.java
      java/nl/stcorp/coda/SWIGTYPE_p_coda_expression_struct.java
      java/nl/stcorp/coda/SWIGTYPE_p_coda_cursor_struct.java
      java/nl/stcorp/coda/SWIGTYPE_p_coda_type_struct.java)
    add_jar(codajar SOURCE ${CODA_JAVA_FILES} OUTPUT_NAME coda)
    set(JAVA ${Java_JAVA_EXECUTABLE})
    set(JAVAC ${Java_JAVAC_EXECUTABLE})
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/java/build.xml.in
      ${CMAKE_CURRENT_BINARY_DIR}/java/build.xml @ONLY)
    install(TARGETS coda_jni DESTINATION ${JAVA_PREFIX})
    install_jar(codajar DESTINATION ${JAVA_PREFIX})
    install(FILES
      java/CodaExample.java
      ${CMAKE_CURRENT_BINARY_DIR}/java/build.xml
      DESTINATION ${JAVA_PREFIX})
  endif(CODA_BUILD_JAVA)

  # documentation
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/html DESTINATION ${DOC_PREFIX})

  # installer
  set(CPACK_PACKAGE_VENDOR "S[&]T")
  if(WIN32)
    install(FILES ${HDF4_DLLS} ${HDF5_DLLS} DESTINATION ${BIN_PREFIX})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Release/coda.exp DESTINATION ${LIB_PREFIX})
    set(CPACK_GENERATOR "WIX")
    configure_file(COPYING copying.txt @ONLY NEWLINE_STYLE WIN32)
    configure_file(FAQ faq.txt @ONLY NEWLINE_STYLE WIN32)
    configure_file(INSTALL install.txt @ONLY NEWLINE_STYLE WIN32)
    configure_file(LICENSES licenses.txt @ONLY NEWLINE_STYLE WIN32)
    configure_file(README readme.txt @ONLY NEWLINE_STYLE WIN32)
    set(CPACK_RESOURCE_FILE_LICENSE copying.txt)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/copying.txt" "${CMAKE_CURRENT_BINARY_DIR}/faq.txt" "${CMAKE_CURRENT_BINARY_DIR}/install.txt" "${CMAKE_CURRENT_BINARY_DIR}/licenses.txt" "${CMAKE_CURRENT_BINARY_DIR}/readme.txt" DESTINATION .)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "CODA")
    set(CPACK_WIX_UPGRADE_GUID "873ec1cd-f35a-407b-af00-9f85e740d1a0")
    set(CPACK_WIX_PRODUCT_GUID "2caa4283-49bc-4709-ae12-c5f41c1346d1")
    set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cpack_wix_patch.xml")
  endif(WIN32)
  include(CPack)

endif(NOT CODA_BUILD_SUBPACKAGE_MODE)

# Some autoconf variables that CMake does not use itself, but that are used in the config file substitutions.
# (leave this section at the end so other rules will use the unquoted VERSION value)
#
set(PACKAGE_NAME \"${CMAKE_PROJECT_NAME}\")
string(TOLOWER ${PACKAGE_NAME} PACKAGE)
set(PACKAGE_VERSION ${CODA_VERSION})
string(REPLACE "\"" "" PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_STRING \"${PACKAGE_STRING}\")
set(VERSION \"${VERSION}\")
set(PACKAGE_TARNAME ${PACKAGE})
set(PACKAGE_BUGREPORT \"\")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda.h.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/coda.h)
