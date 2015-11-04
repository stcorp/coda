#!/bin/sh
# create .codadef file from a directory containg xml definition files

if test $# -eq 0 ; then
  echo "Usage: $0 <directory>"
  exit 1
fi

if test ! -d $1 ; then
  echo "Directory $1 does not exist" 
  exit 1
fi

targetdir=`pwd`

class=`grep "<cd:ProductClass" $1/index.xml | sed -e 's:^.*name=\"\([^"]*\).*$:\1:'`

echo Determining last modification date for $class definitions
files=`find $1 $1/types $1/products -maxdepth 1 -name "*.xml"`
date=`grep -m 1 last-modified $files | sed -e 's:^.*last-modified=\"\([^"]*\).*$:\1:' | sort -r | head -1 | sed 's:-::g'`

echo Creating $class-$date.codadef
rm -f $class-$date.codadef
cd $1
echo $date > VERSION
zip -q $targetdir/$class-$date.codadef VERSION index.xml types/*.xml products/*.xml
rm -f VERSION
cd $targetdir
