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

    cursor.goto('tpTropQCD')

    shape = cursor.get_array_dim()
    print('shape:', shape)

    array = cursor.read()
    print(array)

    cursor = product.cursor()
    cursor.goto('globalInventory')

    scalar = cursor.read()
    print(scalar)
