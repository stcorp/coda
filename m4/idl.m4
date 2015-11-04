# ST_CHECK_IDL
# -------------
# Check for the availability of IDL
AC_DEFUN([ST_CHECK_IDL],
[AC_ARG_VAR(IDL, [The IDL root directory. If not specified '/usr/local/rsi/idl' is assumed])
if test "$IDL" = "" ; then
  IDL=/usr/local/rsi/idl
fi
AC_PATH_PROG(idl, idl, no, [$IDL/bin])
old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-I$IDL/external $CPPFLAGS"
AC_CHECK_HEADERS(export.h)
AC_MSG_CHECKING(for IDL installation)
if test $ac_cv_path_idl = no || test $ac_cv_header_export_h = no ; then
   st_cv_have_idl=no
   CPPFLAGS=$old_CPPFLAGS
 else
   st_cv_have_idl=yes
 fi
 AC_MSG_RESULT($st_cv_have_idl)
])# ST_CHECK_IDL

# ST_CHECK_IDL_SYSFUN_DEF2
# ------------------------
# Check whether IDL defines IDL_SYSFUN_DEF2
AC_DEFUN([ST_CHECK_IDL_SYSFUN_DEF2],
[AC_CACHE_CHECK([for IDL_SYSFUN_DEF2],
                [st_cv_have_idl_sysfun_def2],
                [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
                                   [#include <stdio.h>
#include "export.h"],
                                   [static IDL_SYSFUN_DEF2 sysfun;
sysfun.name = "";])],
                [st_cv_have_idl_sysfun_def2=yes],
                [st_cv_have_idl_sysfun_def2=no])])
if test $st_cv_have_idl_sysfun_def2 = yes ; then
  AC_DEFINE(HAVE_IDL_SYSFUN_DEF2, 1, [Define to 1 if your IDL version supports the IDL_SYSFUN_DEF2 type.])
fi
])# ST_CHECK_IDL_SYSFUN_DEF2


# ST_CHECK_IDL_SYSRTN_UNION
# -------------------------
# Check whether IDL defines IDL_SYSRTN_UNION
AC_DEFUN([ST_CHECK_IDL_SYSRTN_UNION],
[AC_CACHE_CHECK([for IDL_SYSRTN_UNION],
                [st_cv_have_idl_sysrtn_union],
                [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <stdio.h>
#include "export.h"],
                                   [static IDL_SYSRTN_UNION sysrtn;
sysrtn.generic = NULL;])],
                [st_cv_have_idl_sysrtn_union=yes],
                [st_cv_have_idl_sysrtn_union=no])])
if test $st_cv_have_idl_sysrtn_union = yes ; then
  AC_DEFINE(HAVE_IDL_SYSRTN_UNION,1,[Define to 1 if your IDL version supports the IDL_SYSRTN_UNION type.])
fi
])# ST_CHECK_IDL_SYSRTN_UNION
