import os
import sys

if len(sys.argv) == 1 or sys.argv[1] == 'cffi':
    import __init__ as coda
elif sys.argv[1] == 'swig':
    import coda

with coda.open('madis-raob.nc') as product:
    print('product:', product)

    # fetch
    array = product.fetch('tpTropQCD')
    print(array)

    cursor = product.cursor()
    print('cursor:', cursor)

    code = cursor.goto('tpTropQCD')
    print(code)

    shape = cursor.get_array_dim()
    print('shape:', shape)

    array = cursor.read()
    print(array)
