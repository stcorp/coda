# Find the Python stuff for CODA
#
# This module builds on top of the CMake PythonInterp and
# PythonLibs packages. In addition to the variables resulting
# from those packages, this module defines:
#
# - PYTHON The location of the Python interpreter (identical to
# PYTHONINTERP).
#
#
# The user may specify PYTHON, PYTHON_INCLUDE and
# NUMARRAY_INCLUDE environment variables to locate the Python
# interpreter, include directory, and Numarray include directory
# respectively.
#

if ($ENV{PYTHON} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{PYTHON} PYTHON)
  message(STATUS "PYTHON from environment: ${PYTHON}")
endif ($ENV{PYTHON} MATCHES ".+")

if (NOT PYTHON)
  find_package(PythonInterp)

  if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Python is required to build the Python interface. Try setting the PYTHON environment variable to the full path to your python executable if you have Python installed.")
  else (NOT PYTHONINTERP_FOUND)
    set(PYTHON ${PYTHONINTERP})
  endif (NOT PYTHONINTERP_FOUND)

endif(NOT PYTHON)

if ($ENV{PYTHON_INCLUDE} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{PYTHON_INCLUDE} PYTHON_INCLUDE)
  message(STATUS "PYTHON_INCLUDE from environment: ${PYTHON_INCLUDE}")
endif ($ENV{PYTHON_INCLUDE} MATCHES ".+")

if (NOT PYTHON_INCLUDE)
  find_package(PythonLibs)
  if (NOT PYTHONLIBS_FOUND)
    message(FATAL_ERROR "Python include files are needed to build the Python interface. Try setting the PYTHON_INCLUDE environment variable to the directory that contains Python.h.")
  else (NOT PYTHONLIBS_FOUND)
    set(PYTHON_INCLUDE ${PYTHON_INCLUDE_PATH})
  endif (NOT PYTHONLIBS_FOUND)

  if ($ENV{NUMARRAY_INCLUDE} MATCHES ".+")
    file(TO_CMAKE_PATH $ENV{NUMARRAY_INCLUDE} NUMARRAY_INCLUDE)
    message(STATUS "NUMARRAY_INCLUDE from environment: ${NUMARRAY_INCLUDE}")
  else ($ENV{NUMARRAY_INCLUDE} MATCHES ".+")
    set(NUMARRAY_INCLUDE ${PYTHON_INCLUDE})
  endif ($ENV{NUMARRAY_INCLUDE} MATCHES ".+")

  set(CMAKE_REQUIRED_INCLUDES ${NUMARRAY_INCLUDE})
  message(STATUS "NUMARRAY_INCLUDE is ${NUMARRAY_INCLUDE}")
  check_include_file(numarray/arrayobject.h HAVE_ARRAYOBJECT_H)
  if (NOT HAVE_ARRAYOBJECT_H)
    message(FATAL_ERROR "The Python numarray package is needed for the Python interface. Please install 'numarray' or, if you have already installed this package, set the NUMARRAY_INCLUDE environment variable and make sure that the file $NUMARRAY_INCLUDE/numarray/arrayobject.h exists.")
  endif (NOT HAVE_ARRAYOBJECT_H)

  set(codapythondir "TODO")
  set(codapyexecdir "TODO")

endif (NOT PYTHON_INCLUDE)
