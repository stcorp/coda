# Find Python for CODA
#
# This module builds on top of the CMake PythonInterp and
# PythonLibs packages. In addition to the variables resulting
# from those packages, this module defines:
#
# - PYTHON The location of the Python interpreter (identical to
# PYTHONINTERP).
#
#
# The user may specify PYTHON and PYTHON_INCLUDE environment
# variables to locate the Python interpreter and include
# directory respectively.
#

if ($ENV{PYTHON} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{PYTHON} PYTHON)
  message(STATUS "Using PYTHON environment variable: ${PYTHON}")
endif ($ENV{PYTHON} MATCHES ".+")

if (NOT PYTHON)
  find_package(PythonInterp)

  if (NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Python is required to build the Python interface. Try setting the PYTHON environment variable to the full path to your python executable if you have Python installed.")
  else (NOT PYTHONINTERP_FOUND)
    set(PYTHON ${PYTHON_EXECUTABLE})
  endif (NOT PYTHONINTERP_FOUND)

endif(NOT PYTHON)

if ($ENV{PYTHON_INCLUDE} MATCHES ".+")
  file(TO_CMAKE_PATH $ENV{PYTHON_INCLUDE} PYTHON_INCLUDE)
  message(STATUS "Using PYTHON_INCLUDE environment variable: ${PYTHON_INCLUDE}")
endif ($ENV{PYTHON_INCLUDE} MATCHES ".+")

if (NOT PYTHON_INCLUDE)
  find_package(PythonLibs)
  if (NOT PYTHONLIBS_FOUND)
    message(FATAL_ERROR "Python include files are needed to build the Python interface. Try setting the PYTHON_INCLUDE environment variable to the directory that contains Python.h.")
  else (NOT PYTHONLIBS_FOUND)
    set(PYTHON_INCLUDE ${PYTHON_INCLUDE_PATH})
  endif (NOT PYTHONLIBS_FOUND)

endif (NOT PYTHON_INCLUDE)
