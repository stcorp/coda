#!/bin/sh

if test -z "$1" ; then
  echo "Usage: checkf77.sh <fortrancompiler>"
  exit
fi
F77=$1

echo "Using compiler :" $F77
echo "Checking which precompiler settings should be provided:"

echo "      SUBROUTINE FOO_1" > foo.f
echo "      END" >> foo.f
$F77 -c foo.f

# Test whether the fortran compiler generates capital are small-caps identifiers
echo -n "  WRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS : "
use_uppercase_identifiers=no
capitalname=`nm foo.o | grep FOO`
if test ! -z "$capitalname" ; then
  echo "yes" 
  use_uppercase_identifiers=yes
fi
echo $use_uppercase_identifiers

# Test whether the fortran compiler adds an additional '_' for identifiers
# that already contain a '_'.
echo -n "  WRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE : "
use_additional_underscore=no
if test $use_uppercase_identifiers = yes ; then
  extraunderscore=`nm foo.o | grep FOO_1__`;
else
  extraunderscore=`nm foo.o | grep foo_1__`;
fi
if test ! -z "$extraunderscore" ; then
  use_additional_underscore=yes
fi
echo $use_additional_underscore

rm -f foo.o foo.f
