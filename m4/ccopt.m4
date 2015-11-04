# ST_PROG_CC_OPT(OPTION, VARIABLE)
# --------------------------------
# Check whether the C compiler supports a certain option
AC_DEFUN([ST_PROG_CC_OPT],
[AC_REQUIRE([AC_PROG_CC])[]dnl
AC_CACHE_CHECK([whether $CC accepts $1], [$2],
[echo 'void f(){}' > conftest.c
if test -z "`$CC $1 conftest.c 2>&1`"; then
  $2="$1"
else
  $2=""
fi
rm -f conftest*])
AC_SUBST([$2])
])# ST_CHECK_CC_OPT
