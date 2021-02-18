import os

# CFFI
import _codapy as coda

# SWIG
#import coda

coda.init()

product = coda.open('madis-raob.nc')
print('product:', product)

# goto dataset 'tpTropQCD'
cursor = coda.Cursor()
code = coda.cursor_set_product(cursor, product)
print(code)

code = coda.cursor_goto(cursor, 'tpTropQCD')
print(code)

array = coda.cursor_read_double_array(cursor)
print(array)

code = coda.close(product)
print(code)

coda.done()
