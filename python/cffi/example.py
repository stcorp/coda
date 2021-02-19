import os
import sys

if len(sys.argv) == 1 or sys.argv[1] == 'cffi':
    import __init__ as coda
elif sys.argv[1] == 'swig':
    import coda

# initialize
coda.init()

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
code = coda.cursor_set_product(cursor, product)
print(code)

code = coda.cursor_goto(cursor, 'globalInventory')
print(code)

gi = coda.cursor_read_int32(cursor)
print('globalInventory:', gi)

# read double array
code = coda.cursor_goto_root(cursor)
print(code)

code = coda.cursor_goto(cursor, 'tpTropQCD')
print(code)

shape = coda.cursor_get_array_dim(cursor)
print('shape:', shape)

array = coda.cursor_read_double_array(cursor)
print(array)

# read double partial array
code = coda.cursor_goto_root(cursor)
print(code)

code = coda.cursor_goto(cursor, 'tpTropQCD')
print(code)

array = coda.cursor_read_double_partial_array(cursor, 10, 22)
print(array.shape)
print(array)

#finalize
code = coda.close(product)
print(code)
coda.done()
