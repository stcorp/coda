# ST_CHECK_MATLAB
# ---------------
# Check for the availability of MATLAB
AC_DEFUN([ST_CHECK_MATLAB],
[AC_ARG_VAR(MATLAB,[The MATLAB root directory. If not specified '/usr/local/matlab' is assumed])
if test "$MATLAB" = "" ; then
  MATLAB=/usr/local/matlab
fi
AC_PATH_PROG(matlab, matlab, no, [$MATLAB/bin])
AC_PATH_PROG(MEX, mex, no, [$MATLAB/bin])
AC_ARG_VAR(MEXFLAGS, [MATLAB MEX flags])
if test "x$MEXFLAGS" = x ; then
  MEXFLAGS="-g -O"
fi
AC_SUBST(MEXFLAGS)
old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-I$MATLAB/extern/include $CPPFLAGS"
AC_CHECK_HEADERS(mex.h)
ST_CHECK_MEX_EXTENSION
AC_MSG_CHECKING(for MATLAB installation)
if test $ac_cv_path_matlab = no || test "$ac_cv_path_MEX" = no ||
   test $ac_cv_header_mex_h = no || test "$MEXEXT" = "" ; then
  st_cv_have_matlab=no
  CPPFLAGS=$old_CPPFLAGS
else
  st_cv_have_matlab=yes
fi
AC_MSG_RESULT($st_cv_have_matlab)
])# ST_CHECK_MATLAB


# ST_CHECK_MEX_EXTENSION
# ----------------------
# Determine the extension that is used for MATLAB MEX modules
AC_DEFUN([ST_CHECK_MEX_EXTENSION],
[AC_MSG_CHECKING([for mex extension])
[cat > conftest.c <<_ACEOF
#include "mex.h"
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {}
_ACEOF]
if { (eval echo "$as_me:$LINENO: \"$MEX $MEXFLAGS conftest.c\"") >&5
     (eval $MEX $MEXFLAGS conftest.c >&5) 2>conftest.er1
     ac_status=$?
     grep -v '^ *+' conftest.er1 >conftest.err
     rm -f conftest.er1
     cat conftest.err >&5
     echo "$as_me:$LINENO: \$? = $ac_status" >&5
     (exit $ac_status); } >/dev/null; then
  mexexttmp=`ls conftest.mex* | sed 's/conftest\.//'`
  if test "$mexexttmp" != "" ; then
    MEXEXT=$mexexttmp
  else
    if test -f conftest.dll ; then
      MEXEXT=dll
    fi
  fi
  if test "$MEXEXT" != "" ; then
    AC_MSG_RESULT([$MEXEXT])
  else
    AC_MSG_RESULT([not found])
  fi 
else
  AC_MSG_RESULT([failed])
fi
rm -f conftest*
AC_SUBST(MEXEXT)
])# ST_CHECK_MEX_EXTENSION


# ST_CHECK_MATLAB_FUNC
# --------------------
# Determine the availability of a MATLAB mx/mex function
AC_DEFUN([ST_CHECK_MATLAB_FUNC],
[AC_CACHE_CHECK([for $1], st_cv_func_$1,
[[cat > conftest.c <<_ACEOF
#include "mex.h"
typedef void* (*exec_t)(void*);
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ static exec_t func = NULL; func = (exec_t)&($1); }
_ACEOF]
if { (eval echo "$as_me:$LINENO: \"$MEX $MEXFLAGS conftest.c\"") >&5
     (eval $MEX $MEXFLAGS conftest.c >&5) 2>conftest.er1
     ac_status=$?
     grep -v '^ *+' conftest.er1 >conftest.err
     rm -f conftest.er1
     cat conftest.err >&5
     echo "$as_me:$LINENO: \$? = $ac_status" >&5
     (exit $ac_status); } >/dev/null; then
  st_cv_func_$1=yes
else
  st_cv_func_$1=no
fi
rm -f conftest*])
])# ST_CHECK_MATLAB_FUNC


# ST_CHECK_MATLAB_R13_FUNCS
# -------------------------
# Determine the availability of some mx functions that have been introduced in MATLAB R13
AC_DEFUN([ST_CHECK_MATLAB_R13_FUNCS],
[ST_CHECK_MATLAB_FUNC([mxCreateDoubleScalar])
if test $st_cv_func_mxCreateDoubleScalar = yes ; then
  AC_DEFINE(HAVE_MXCREATEDOUBLESCALAR, 1, [Define to 1 if matlab has the mxCreateDoubleScalar function.])
else
  AC_DEFINE(mxCreateDoubleScalar,replacement_mxCreateDoubleScalar, [Define to replacement_mxCreateDoubleScalar if HAVE_MXCREATEDOUBLESCALAR is not set.])
  MATLAB_LIBOBJS="$MATLAB_LIBOBJS mxCreateDoubleScalar.\$(OBJEXT)"
fi
ST_CHECK_MATLAB_FUNC([mxCreateNumericMatrix])
if test $st_cv_func_mxCreateNumericMatrix = yes ; then
  AC_DEFINE(HAVE_MXCREATENUMERICMATRIX, 1, [Define to 1 if matlab has the mxCreateNumericMatrix function.])
else
  AC_DEFINE(mxCreateNumericMatrix,replacement_mxCreateNumericMatrix, [Define to replacement_mxCreateNumericMatrix if HAVE_MXCREATENUMERICMATRIX is not set.])
  MATLAB_LIBOBJS="$MATLAB_LIBOBJS mxCreateNumericMatrix.\$(OBJEXT)"
fi
AC_SUBST(MATLAB_LIBOBJS)
])# ST_CHECK_MATLAB_R13_FUNCS
