import os
import sys

if len(sys.argv) == 1 or sys.argv[1] == 'cffi':
    import __init__ as coda
elif sys.argv[1] == 'swig':
    import coda

with coda.open('madis-raob.nc') as product:
    # fetch array
    array = product.fetch('tpTropQCD')
    print(array)

    # fetch scalar
    scalar = product.fetch('globalInventory')
    print(scalar)
