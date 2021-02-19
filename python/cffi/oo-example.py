import os
import sys

import __init__ as coda

# open product
with coda.open('madis-raob.nc') as product:
    # fetch array via product
    array = product.fetch('tpTropQCD')
    print(array)

    # fetch scalar via cursor
    cursor = product.cursor()
    scalar = cursor.fetch('globalInventory')
    print(scalar)
