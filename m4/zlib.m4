# ST_CHECK_ZLIB
# -------------
# Check for the availability of the Zlib library and include files
AC_DEFUN([ST_CHECK_ZLIB],
[AC_ARG_VAR([ZLIB_LIB],[The zlib library directory. If not specified no extra LDFLAGS are set])
AC_ARG_VAR([ZLIB_INCLUDE],[The zlib include directory. If not specified no extra CPPFLAGS are set])
old_CPPFLAGS=$CPPFLAGS
old_LDFLAGS=$LDFLAGS
if test "$ZLIB_LIB" != "" ; then
  LDFLAGS="-L$ZLIB_LIB $LDFLAGS"
fi
if test "$ZLIB_INCLUDE" != "" ; then
  CPPFLAGS="-I$ZLIB_INCLUDE $CPPFLAGS"
fi
AC_CHECK_HEADERS(zlib.h)
AC_CHECK_LIB(z, compress, ac_cv_lib_z=yes, ac_cv_lib_z=no)
if test $ac_cv_header_zlib_h = no || test $ac_cv_lib_z = no ; then
  st_cv_have_zlib=no
  CPPFLAGS=$old_CPPFLAGS
  LDFLAGS=$old_LDFLAGS
else
  st_cv_have_zlib=yes
  ZLIB="-lz"
fi
AC_MSG_CHECKING(for zlib installation)
AC_MSG_RESULT($st_cv_have_zlib)
if test $st_cv_have_zlib = yes ; then
  AC_DEFINE(HAVE_ZLIB, 1, [Define to 1 if zlib is available.])
fi
])# ST_CHECK_ZLIB
