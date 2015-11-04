# ST_PATH_PYTHON_INCLUDE
# ----------------------
# Check for the location of the Python.h include file
AC_DEFUN([ST_PATH_PYTHON_INCLUDE],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_ARG_VAR(PYTHON_INCLUDE,[The Python include directory])
AC_MSG_CHECKING([for python include directory])
if test "$PYTHON_INCLUDE" = "" ; then 
  PYTHON_INCLUDE=`($PYTHON -c "import sys; from distutils import sysconfig; sys.stdout.write(sysconfig.get_python_inc())") 2>/dev/null`
  if test ! -r "$PYTHON_INCLUDE/Python.h" ; then
    PYTHON_INCLUDE=
  fi
fi
AC_MSG_RESULT($PYTHON_INCLUDE)
if test "$PYTHON_INCLUDE" != "" ; then
  CPPFLAGS="-I$PYTHON_INCLUDE $CPPFLAGS"
fi
AC_SUBST([PYTHON_INCLUDE])
])# ST_PATH_PYTHON_INCLUDE
