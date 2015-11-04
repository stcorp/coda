# ST_CHECK_WRAPFORTRAN_F77_OPTIONS
# --------------------------------
# Check for C code wrapping options for F77 compiler
AC_DEFUN([ST_CHECK_WRAPFORTRAN_F77_OPTIONS],
[AC_REQUIRE([AC_PROG_F77])[]dnl
st_wrapfortran_f77_use_uppercase_identifiers=no
st_wrapfortran_f77_use_additional_underscore=no
if test "$F77" != "" ; then
  AC_LANG_PUSH(Fortran 77)
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([      SUBROUTINE FOO_1
      END])],
                    [st_capitalname=`nm conftest.o | grep FOO`
    AC_MSG_CHECKING(whether $F77 uses uppercase identifiers)
    if test ! -z "$st_capitalname" ; then
      st_wrapfortran_f77_use_uppercase_identifiers=yes
    fi
    AC_MSG_RESULT($st_wrapfortran_f77_use_uppercase_identifiers)
    AC_MSG_CHECKING(whether $F77 uses an extra underscore in identifiers)
    if test $st_wrapfortran_f77_use_uppercase_identifiers = yes ; then
      st_extraunderscore=`nm conftest.o | grep FOO_1__`
    else
      st_extraunderscore=`nm conftest.o | grep foo_1__`
    fi
    if test ! -z "$st_extraunderscore" ; then
      st_wrapfortran_f77_use_additional_underscore=yes
    fi
    AC_MSG_RESULT($st_wrapfortran_f77_use_additional_underscore)])
  AC_LANG_POP(Fortran 77)
fi
WRAPFORTRAN_FLAGS=
if test $st_wrapfortran_f77_use_uppercase_identifiers = yes ; then
  AC_DEFINE(WRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS, 1,
            [Define to 1 if your Fortran compiler generates object files containing uppercase identifiers.])
  WRAPFORTRAN_FLAGS="$WRAPFORTRAN_FLAGS -DWRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS=1"
fi
if test $st_wrapfortran_f77_use_additional_underscore = yes ; then
  AC_DEFINE(WRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE, 1,
            [Define to 1 if your Fortran compiler adds an additional underscore to an identifier in the object file if the identifier contains an underscore.])
  WRAPFORTRAN_FLAGS="$WRAPFORTRAN_FLAGS -DWRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE=1"
fi
AC_SUBST(WRAPFORTRAN_FLAGS)
])# ST_CHECK_WRAPFORTRAN_F77_OPTIONS

# ST_CHECK_WRAPFORTRAN_FC_OPTIONS
# --------------------------------
# Check for C code wrapping options for FC compiler
AC_DEFUN([ST_CHECK_WRAPFORTRAN_FC_OPTIONS],
[AC_REQUIRE([AC_PROG_FC])[]dnl
st_wrapfortran_fc_use_uppercase_identifiers=no
st_wrapfortran_fc_use_additional_underscore=no
if test "$FC" != "" ; then
  AC_LANG_PUSH(Fortran)
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([      SUBROUTINE FOO_1
      END])],
                    [st_capitalname=`nm conftest.o | grep FOO`
    AC_MSG_CHECKING(whether $FC uses uppercase identifiers)
    if test ! -z "$st_capitalname" ; then
      st_wrapfortran_fc_use_uppercase_identifiers=yes
    fi
    AC_MSG_RESULT($st_wrapfortran_fc_use_uppercase_identifiers)
    AC_MSG_CHECKING(whether $FC uses an extra underscore in identifiers)
    if test $st_wrapfortran_fc_use_uppercase_identifiers = yes ; then
      st_extraunderscore=`nm conftest.o | grep FOO_1__`
    else
      st_extraunderscore=`nm conftest.o | grep foo_1__`
    fi
    if test ! -z "$st_extraunderscore" ; then
      st_wrapfortran_fc_use_additional_underscore=yes
    fi
    AC_MSG_RESULT($st_wrapfortran_fc_use_additional_underscore)])
  AC_LANG_POP(Fortran)
fi
WRAPFORTRAN_FLAGS=
if test $st_wrapfortran_fc_use_uppercase_identifiers = yes ; then
  AC_DEFINE(WRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS, 1,
            [Define to 1 if your Fortran compiler generates object files containing uppercase identifiers.])
  WRAPFORTRAN_FLAGS="$WRAPFORTRAN_FLAGS -DWRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS=1"
fi
if test $st_wrapfortran_fc_use_additional_underscore = yes ; then
  AC_DEFINE(WRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE, 1,
            [Define to 1 if your Fortran compiler adds an additional underscore to an identifier in the object file if the identifier contains an underscore.])
  WRAPFORTRAN_FLAGS="$WRAPFORTRAN_FLAGS -DWRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE=1"
fi
AC_SUBST(WRAPFORTRAN_FLAGS)
])# ST_CHECK_WRAPFORTRAN_FC_OPTIONS
