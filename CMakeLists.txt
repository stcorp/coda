# CMake configuration file for CODA.
#
project(CODA)

set(VERSION 1.3)
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
include(CMakeDependentOption)

# Option handling
#
option(CODA_BUILD_SUBPACKAGE_MODE "build CODA as sub-package" OFF)
option(CODA_BUILD_IDL "build IDL interface" OFF)
option(CODA_BUILD_MATLAB "build MATLAB interface" OFF)
option(CODA_WITH_HDF4 "use HDF4" OFF)
option(CODA_WITH_HDF5 "use HDF5" OFF)
cmake_dependent_option(
  CODA_DISABLE_HDF4_VDATA_ATTRIBUTES "enable HDF4 Vdata attributes" OFF
  CODA_WITH_HDF4 ON)
#Python is not yet supported in this release of CODA.
#option(CODA_BUILD_PYTHON "build Python interface" OFF)

# The following is done because in CMake options are always
# ON/OFF only, but for config.h substition we need something with
# value 1.
#
if(NOT CODA_DISABLE_HDF4_VDATA_ATTRIBUTES)
  set(ENABLE_HDF4_VDATA_ATTRIBUTES 1)
endif(NOT CODA_DISABLE_HDF4_VDATA_ATTRIBUTES)

# Some autoconf variables that CMake does not use itself, but
# that are used in the config file substitutions.
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
if(CMAKE_COMPILER_IS_GNUCC)
  set(GNU_WARNING_FLAGS "-W -Wall -Wextra")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNU_WARNING_FLAGS}")
  if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC"  )
  endif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
endif(CMAKE_COMPILER_IS_GNUCC)
# CMake adds -DNDEBUG by default in the Release targets. We want
# asserts enabled even in production releases, so we explicitly
# re-enable them here.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -UNDEBUG")

# hdf
#
if(CODA_WITH_HDF4)

  find_package(HDF4)

  if(NOT HDF4_FOUND)
    message(FATAL_ERROR "HDF4 libraries and/or header files are not found. Try setting the HDF4_LIB and HDF4_INCLUDE environment variables to the location of your HDF4 library and include files.")
  else(NOT HDF4_FOUND)
    include_directories(${HDF4_INCLUDE_DIR})
  endif(NOT HDF4_FOUND)

endif(CODA_WITH_HDF4)


if(CODA_WITH_HDF5)

  find_package(HDF5)

  if(NOT HDF5_FOUND)
    message(FATAL_ERROR "HDF5 libraries and/or header files are not found. Try setting the HDF5_LIB and HDF5_INCLUDE environment variables to the location of your HDF5 library and include files.")
  else(NOT HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIR})
  endif(NOT HDF5_FOUND)

endif(CODA_WITH_HDF5)


# *** xml ***
#
set(XML_NS 1)
set(XML_DTD 1)
set(XML_LARGE_SIZE 1)
set(XML_CONTEXT_BYTES 1024)


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

macro(find_include FILE VARIABLE)
  CHECK_INCLUDE_FILE(${FILE} ${VARIABLE})
  if(${VARIABLE})
    set(INCLUDES ${INCLUDES} ${FILE})
  endif(${VARIABLE})
endmacro(find_include)

find_include(dirent.h		HAVE_DIRENT_H)
find_include(dlfcn.h		HAVE_DLFCN_H)
find_include(export.h		HAVE_EXPORT_H)
find_include(inttypes.h		HAVE_INTTYPES_H)
find_include(memory.h		HAVE_MEMORY_H)
find_include(stdint.h		HAVE_STDINT_H)
find_include(stdlib.h		HAVE_STDLIB_H)
find_include(strings.h		HAVE_STRINGS_H)
find_include(string.h		HAVE_STRING_H)
find_include(sys/mman.h		HAVE_SYS_MMAN_H)
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

if(WIN32)
  # Hack for Windows
  set(HAVE_STRDUP 1)
  set(HAVE_STRCASECMP 1)
  set(HAVE_STRNCASECMP 1)
  set(HAVE_VSNPRINTF 1)
endif(WIN32)


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

if(HAVE_STDLIB_H AND HAVE_STDDEF_H)
  set(STDC_HEADERS 1)
endif(HAVE_STDLIB_H AND HAVE_STDDEF_H)
if(NOT HAVE_SYS_TYPES_H)
  set(NO_SYS_TYPES_H 1)
endif(NOT HAVE_SYS_TYPES_H)
if(NOT HAVE_STRERROR)
  set(NO_STRERROR 1)
endif(NOT HAVE_STRERROR)

set(CODA_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda
  ${CMAKE_CURRENT_BINARY_DIR}
  CACHE INTERNAL "Include directories for the CODA library")

include_directories(
  ${CODA_INCLUDE_DIRS}
)


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
  libcoda/coda-netcdf-cursor.c
  libcoda/coda-netcdf-dynamic.c
  libcoda/coda-netcdf-internal.h
  libcoda/coda-netcdf-type.c
  libcoda/coda-netcdf.c
  libcoda/coda-netcdf.h
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

if(CODA_WITH_HDF4)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF4_FILES})
endif(CODA_WITH_HDF4)


set(LIBCODA_HDF5_FILES
  libcoda/coda-hdf5-cursor.c
  libcoda/coda-hdf5-definition.c
  libcoda/coda-hdf5-internal.h
  libcoda/coda-hdf5-type.c
  libcoda/coda-hdf5.h
  )

if(CODA_WITH_HDF5)
  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${LIBCODA_HDF5_FILES})
endif(CODA_WITH_HDF5)


# fortran

if(NOT CODA_BUILD_SUBPACKAGE_MODE)

  set(fortran_DATA
    fortran/Makefile
	fortran/coda_fortran.c
	fortran/example.f
    )

  set(fortran_SCRIPTS
	fortran/checkf77.sh
    )
endif(NOT CODA_BUILD_SUBPACKAGE_MODE)

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

if(CODA_BUILD_IDL)

  # If not in cache, try from environment
  if(NOT IDL_DIR)
    if($ENV{IDL_DIR} MATCHES ".+")
      file(TO_CMAKE_PATH $ENV{IDL_DIR} IDL_DIR)
    endif($ENV{IDL_DIR} MATCHES ".+")
  endif(NOT IDL_DIR)

  if(IDL_DIR)
    set(IDL_DIR ${IDL_DIR} CACHE STRING "Root directory of an IDL installation" FORCE)
    set(IDL_INSTALL_DIR "idl")

    set(HAVE_IDL_SYSFUN_DEF2 1)
    set(HAVE_IDL_SYSRTN_UNION 1)
    if(WIN32)
      set(IDL_COMPILE_FLAGS "-nologo -I. -I${CODA_BINARY_DIR} -I${CODA_SOURCE_DIR}/libcoda -DWIN32_LEAN_AND_MEAN -DWIN32 -DHAVE_CONFIG_H -DIDL_V5_4")
      set(IDL_LIBDIR "${IDL_DIR}/bin/bin.x86/")
      set(IDL_LIB "${IDL_LIBDIR}/idl.lib")
      set(IDL_LINK_FLAGS "/DEF:${CODA_SOURCE_DIR}/win32/coda-idl.def")
    else(WIN32)
      set(IDL_LINK_FLAGS "-module -shared -avoid-version")
      if(APPLE)
        set(IDL_LINK_FLAGS "${IDL_LINK_FLAGS} -undefined dynamic_lookup")
      endif(APPLE)
    endif(WIN32)

    set(IDL_INCLUDE_DIR "${IDL_DIR}/external")
    include_directories(${IDL_INCLUDE_DIR})

    set(CODA_IDL_SOURCES
      idl/coda-idl.c
      )
    set_source_files_properties(${CODA_IDL_SOURCES}
      PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} ${IDL_COMPILE_FLAGS}"
      )
    # Visual Studio does not like targets with a dash in the
    # name, hence coda_idl rather than coda-idl.
    add_library(coda_idl MODULE
      ${CODA_IDL_SOURCES}
      )
    target_link_libraries(coda_idl coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${IDL_LIB})
    set_target_properties(coda_idl
      PROPERTIES LINK_FLAGS "${IDL_LINK_FLAGS}"
      OUTPUT_NAME "coda-idl"
      PREFIX ""
      )
    
    install(TARGETS coda_idl DESTINATION ${IDL_INSTALL_DIR})

    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/idl/coda-idl.dlm.in
      ${CMAKE_CURRENT_BINARY_DIR}/idl/coda-idl.dlm)
    
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/idl/coda-idl.dlm 
      DESTINATION ${IDL_INSTALL_DIR}
      )

  else(IDL_DIR)
    message(FATAL_ERROR "IDL not found. Try setting the IDL_DIR environment or CMake variable to the root directory of an IDL installation.")
  endif(IDL_DIR)

endif(CODA_BUILD_IDL)


# matlab

if(CODA_BUILD_MATLAB)

  # If not in cache, try from environment
  if(NOT MATLAB_DIR)
    if($ENV{MATLAB_DIR} MATCHES ".+")
      file(TO_CMAKE_PATH $ENV{MATLAB_DIR} MATLAB_DIR)
    endif($ENV{MATLAB_DIR} MATCHES ".+")
  endif(NOT MATLAB_DIR)
  
  if(MATLAB_DIR)

    set(MATLAB_DIR ${MATLAB_DIR} CACHE STRING "Root directory of MATLAB installation" FORCE)
    set(MATLAB_INSTALL_DIR "matlab")

    find_path(MATLAB_INCLUDE_DIR "mex.h" ${MATLAB_DIR}/extern/include)
    mark_as_advanced(MATLAB_INCLUDE_DIR)

    set(CODA_MATLAB_SOURCE_FILES
      ${CODA_SOURCE_DIR}/matlab/coda-matlab-getdata.c
      ${CODA_SOURCE_DIR}/matlab/coda-matlab-traverse.c
      ${CODA_SOURCE_DIR}/matlab/coda_matlab.c
      )

    # Configure the mex compiler
    set(MEX ${MATLAB_DIR}/bin/mex)

    set(MEXFLAGS -I${MATLAB_INCLUDE_DIR} -I. -I${CODA_BINARY_DIR} -I${CODA_SOURCE_DIR}/libcoda -DHAVE_CONFIG_H)
    if(WIN32)
      set(MEXEXT_PROG "mexext.bat")
      set(MEXFLAGS ${MEXFLAGS} -DWIN32_LEAN_AND_MEAN -DWIN32 -DHAVE_CONFIG_H -v)
    else(WIN32)
      set(MEXEXT_PROG "mexext")
      set(MEXFLAGS ${MEXFLAGS} -g -O)
    endif(WIN32)

    file(TO_NATIVE_PATH ${MATLAB_DIR}/bin/${MEXEXT_PROG} MEXEXT_CMD)
    execute_process(
      COMMAND ${MEXEXT_CMD}
      OUTPUT_VARIABLE MEXEXT
      )
    string(STRIP ${MEXEXT} MEXEXT)

    set(MEX_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/coda_matlab.${MEXEXT})


    # Because we are essentially bypassing CMake with the mex command,
    # we need to specify exactly where the build libraries can be found.
    # Since the user may have overridden CMAKE_ARCHIVE_OUTPUT_DIRECTORY,
    # we have to take that possibility into account. Just prefixing is
    # not enough, because the variable can also be empty, and then you'd
    # get names starting with a slash.
    set(INT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

    if(WIN32)
      set(MEX_CMD ${MEX} ${MEXFLAGS} ${CODA_MATLAB_SOURCE_FILES} ${INT_LIB_DIR}/libcoda.lib ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} -output ${MEX_OUTPUT})
    else(WIN32)
      set(MEX_CMD ${MEX} ${MEXFLAGS} ${CODA_MATLAB_SOURCE_FILES} ${INT_LIB_DIR}/libcoda.a ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} -o ${MEX_OUTPUT})
    endif(WIN32)
 
    add_custom_command(
      COMMENT "Compiling the CODA MATLAB interface..."
      DEPENDS ${CODA_MATLAB_SOURCE_FILES}
      OUTPUT ${MEX_OUTPUT}
      COMMAND ${MEX_CMD}
      )
 
    add_custom_target(coda_matlab ALL DEPENDS
      ${MEX_OUTPUT}
      )

    add_dependencies(coda_matlab coda_static)
    install(FILES ${MEX_OUTPUT} DESTINATION ${MATLAB_INSTALL_DIR})

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
    
    install(FILES ${CODA_MATLAB_MFILES} DESTINATION ${MATLAB_INSTALL_DIR})
    
  else(MATLAB_DIR)
    
    message(FATAL_ERROR "MATLAB not found. Try setting the MATLAB_DIR environment or CMake variable to the root directory of a MATLAB installation.")
    
  endif(MATLAB_DIR)
  
endif(CODA_BUILD_MATLAB)


# python -- not yet supported for CMake in this release of CODA.
#
#set(CODA_PYTHON_SOURCE_FILES
#  python/codac.c
#)
#
#if(CODA_BUILD_PYTHON)
#  find_package(Python)
#
#  if(NOT PYTHON)
#    message(FATAL_ERROR "PYTHON interpreter and/or header files are not found. Try setting the PYTHON and/or PYTHON_INCLUDE environment variables to the location of your PYTHON interpreter and include files.")
#  else(NOT PYTHON)
#    include_directories(${PYTHON_INCLUDE})
# endif(NOT PYTHON)
#
#  find_package(NumPy)
#
#  if(NOT NUMPY_FOUND)
#    message(FATAL_ERROR "Python numpy package is needed for the Python interface. Please install 'numpy' or, if you have already installed this package, set the NUMPY_INCLUDE environment variable and make sure that the file $NUMPY_INCLUDE/numpy/ndarrayobject.h exists.")
#  else(NOT NUMPY_FOUND)
#    include_directories(${NUMPY_INCLUDE_DIR})
#  endif(NOT NUMPY_FOUND)
#
#  set(LIBCODA_SOURCES ${LIBCODA_SOURCES} ${CODA_PYTHON_SOURCE_FILES})
#
#endif(CODA_BUILD_PYTHON)


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


install(FILES ${CMAKE_CURRENT_BINARY_DIR}/coda.h DESTINATION include)

# Do not build the dynamic version for now.
#
# Set dynamic library version
#
set(LIBCODA_CURRENT 1)
set(LIBCODA_REVISION 0)
set(LIBCODA_AGE 0)

add_library(coda SHARED ${LIBCODA_SOURCES} ${LIBEXPAT_SOURCES} ${LIBZLIB_SOURCES})
target_link_libraries(coda ${HDF4_LIBRARIES} ${HDF5_LIBRARIES})
set_target_properties(coda PROPERTIES
  VERSION ${LIBCODA_CURRENT}.${LIBCODA_REVISION}.${LIBCODA_AGE}
  SOVERSION ${LIBCODA_CURRENT}
)
install(TARGETS coda DESTINATION lib)

add_library(coda_static STATIC ${LIBCODA_SOURCES} ${LIBEXPAT_SOURCES} ${LIBZLIB_SOURCES})

# On Windows, we want libcoda.lib for static, coda.dll & coda.lib for shared.
# On Unix, we want libcoda.a and libcoda.so
if(WIN32)
  set_target_properties(coda_static PROPERTIES
    OUTPUT_NAME "libcoda"
    )
else(WIN32)
  set_target_properties(coda_static PROPERTIES
    OUTPUT_NAME "coda"
    )
endif(WIN32)
install(TARGETS coda_static DESTINATION lib)

if(MSVC)
  set_source_files_properties(coda-expr-tokenizer.c PROPERTIES
    COMPILE_FLAGS "/D YY_NO_UNISTD_H"
    )
endif(MSVC)


# Required libraries
#
# Only on Unix -- Windows does not need this, and the CMake
# people explicitly recommend just special-casing the platform
# through 'if'.
if(NOT WIN32)
  find_library(LIBM_LIBRARY m)
  mark_as_advanced(LIBM_LIBRARY)
endif(NOT WIN32)

# tools codacheck

set(codacheck_SOURCES
  tools/codacheck/codacheck.c
  )
add_executable(codacheck ${codacheck_SOURCES})
target_link_libraries(codacheck coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${LIBM_LIBRARY})
install(TARGETS codacheck DESTINATION bin)

if(MSVC)
  set_target_properties(codacheck PROPERTIES
    COMPILE_FLAGS "/nologo /GX /D_CONSOLE /D_MBCS /YX /FD"
    )
endif(MSVC)

# tools codacmp

set(codacmp_SOURCES
  tools/codacmp/codacmp.c
  )
add_executable(codacmp ${codacmp_SOURCES})
target_link_libraries(codacmp coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${LIBM_LIBRARY})
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
if(CODA_WITH_HDF4)
  set(codadump_SOURCES ${codadump_SOURCES} ${codadump_hdf4_files})
endif(CODA_WITH_HDF4)
add_executable(codadump ${codadump_SOURCES})
target_link_libraries(codadump coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${LIBM_LIBRARY})
install(TARGETS codadump DESTINATION bin)

# tools codafind

set(codafind_SOURCES
  tools/codafind/codafind.c
  )
add_executable(codafind ${codafind_SOURCES})
target_link_libraries(codafind coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${LIBM_LIBRARY})
install(TARGETS codafind DESTINATION bin)

# tools codadd

set(codadd_SOURCES
  tools/codadd/codadd.c
  tools/codadd/codadd-doc.c
  tools/codadd/codadd-list.c
  )
add_executable(codadd ${codadd_SOURCES})
target_link_libraries(codadd coda_static ${HDF4_LIBRARIES} ${HDF5_LIBRARIES} ${LIBM_LIBRARY})
install(TARGETS codadd DESTINATION bin)


# config files
#
add_definitions(-DHAVE_CONFIG_H)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libcoda/coda.h.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/coda.h)


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

