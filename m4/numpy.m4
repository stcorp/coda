# ST_PATH_NUMPY_INCLUDE
# ----------------------
# Check for the location of the numpy include file directory
AC_DEFUN([ST_PATH_NUMPY_INCLUDE],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_ARG_VAR(NUMPY_INCLUDE, [The Python include directory where the 'numpy' package is installed ($NUMPY_INCLUDE/numpy/ndarrayobject.h should exist)])
AC_MSG_CHECKING([for numpy include directory])
if test "$NUMPY_INCLUDE" = "" ; then 
  NUMPY_INCLUDE=[`($PYTHON -c "import sys; import os; p = [p for p in sys.path if os.path.exists(os.path.join(p, 'numpy', 'core', 'include', 'numpy', 'ndarrayobject.h'))]; sys.stdout.write(os.path.join(p[0], 'numpy', 'core', 'include') if len(p) > 0 else '')") 2>/dev/null`]
fi
AC_MSG_RESULT($NUMPY_INCLUDE)
if test "$NUMPY_INCLUDE" != "" ; then
  CPPFLAGS="-I$NUMPY_INCLUDE $CPPFLAGS"
fi
AC_SUBST([NUMPY_INCLUDE])
])# ST_PATH_NUMPY_INCLUDE
