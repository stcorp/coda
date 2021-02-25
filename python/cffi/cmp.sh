#!/bin/bash

python3 example.py cffi > cffi.out
python3 example.py swig > swig.out

diff -urb swig.out cffi.out

python2 example.py cffi > cffi.out
python2 example.py swig > swig.out

diff -urb swig.out cffi.out
