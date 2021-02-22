import os
import sys

import __init__ as coda


with coda.open('madis-raob.nc') as product:
    # fetch array via product
    array = product.fetch('tpTropQCD')
    print(array)

    # fetch scalar via cursor
    cursor = product.cursor()
    scalar = cursor.fetch('globalInventory')
    print(scalar)

    # properties
    print(product.version)


with coda.open('AE_TEST_ALD_U_N_1B_20190105T011602023_008364010_002143_0001.DBL') as product:
    print(product.description)
