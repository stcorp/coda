import os
import sys

if len(sys.argv) == 1 or sys.argv[1] == 'cffi':
    import __init__ as coda
elif sys.argv[1] == 'swig':
    import coda

print('MAX_NUM_DIMS:', coda.CODA_MAX_NUM_DIMS)

coda.init()

product = coda.open('madis-raob.nc')
print('product:', product)

# fetch
array = coda.fetch(product, 'tpTropQCD')
print(array)

cursor = coda.Cursor()
print('cursor:', cursor)
code = coda.cursor_set_product(cursor, product)
print(code)

code = coda.cursor_goto(cursor, 'tpTropQCD')
print(code)

shape = coda.cursor_get_array_dim(cursor)
print('shape:', shape)

array = coda.cursor_read_double_array(cursor)
print(array)

code = coda.close(product)
print(code)

coda.done()
