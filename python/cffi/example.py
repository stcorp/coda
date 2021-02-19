import os
import sys

if len(sys.argv) == 1 or sys.argv[1] == 'cffi':
    import __init__ as coda
elif sys.argv[1] == 'swig':
    import coda

# initialize
coda.init()

# index conversion
print('index:', coda.c_index_to_fortran_index([4,10], 9))

# NaN, Inf
print(coda.NaN(), coda.MinInf(), coda.PlusInf())
print(coda.isNaN(0), coda.isNaN(coda.NaN()))
print(coda.isInf(0), coda.isInf(coda.PlusInf()), coda.isInf(coda.MinInf()))
print(coda.isMinInf(0), coda.isMinInf(coda.MinInf()), coda.isMinInf(coda.PlusInf()))
print(coda.isPlusInf(0), coda.isPlusInf(coda.PlusInf()), coda.isPlusInf(coda.MinInf()))

# open product
product = coda.open('madis-raob.nc')

# fetch array
array = coda.fetch(product, 'tpTropQCD')
print(array)

# fetch scalar
scalar = coda.fetch(product, 'globalInventory')
print(scalar)

# read scalar uint16
cursor = coda.Cursor()
coda.cursor_set_product(cursor, product)

coda.cursor_goto(cursor, 'globalInventory')

gi = coda.cursor_read_int32(cursor)
print('globalInventory:', gi)

# read double array
coda.cursor_goto_root(cursor)

coda.cursor_goto(cursor, 'tpTropQCD')

shape = coda.cursor_get_array_dim(cursor)
print('shape:', shape)

array = coda.cursor_read_double_array(cursor)
print(array)

# read double partial array
coda.cursor_goto_root(cursor)

coda.cursor_goto(cursor, 'tpTropQCD')

array = coda.cursor_read_double_partial_array(cursor, 10, 22)
print(array.shape)
print(array)

# exceptions
coda.cursor_goto_root(cursor)

try:
    coda.cursor_goto(cursor, 'zzz')
except coda.CodacError as e:
    print(str(e))

try:
    coda.cursor_read_int32(cursor)
except coda.CodacError as e:
    print(str(e))

try:
    coda.open('pipo')
except coda.CodacError as e:
    print(str(e))

# version
print(coda.version())

#finalize
coda.close(product)
coda.done()
