# ST_CHECK_MACOSX_SYSTEMSTUBS
# ---------------------------
# On Mac OS X 10.4 there are some occasions where you may need to
# explicitly link to the SystemStubs library:
#  - you are building a module using a different version of gcc than
#    with which the application that loads the module was build
#  - you are mixing C/C++/Fortran code with a combination of old and
#    new compilers (e.g. combining g++ 3.3 and gcc 4.0)
# Whenever you encounter an error like:
#   ld: Undefined symbols:
#   _fprintf$LDBLStub
# use this autoconf function to link in the SystemStubs library.
AC_DEFUN([ST_CHECK_MACOSX_SYSTEMSTUBS],
[case $build in
  *-apple-darwin8.*)
    AC_CHECK_LIB([SystemStubs],[__stub_getrealaddr])
    ;;
esac
])# ST_CHECK_MACOSX_SYSTEMSTUBS
