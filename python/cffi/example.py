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

print('class', coda.get_product_class(product))
print('type', coda.get_product_type(product))
print('version', coda.get_product_version(product))

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
try:
    coda.cursor_read_uint16(cursor)
except coda.CodacError as e:
    print(str(e))

coda.cursor_goto_root(cursor)
expr = coda.expression_from_string('2 * int(./globalInventory)')
print('expr:', coda.expression_eval_integer(expr, cursor))
coda.expression_delete(expr)

# read double array
coda.cursor_goto_root(cursor)
coda.cursor_goto(cursor, 'tpTropQCD')
coda.cursor_goto_parent(cursor)
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

# node expr
coda.cursor_goto_root(cursor)
print('root depth:', coda.cursor_get_depth(cursor))
expr = coda.expression_from_string('/globalInventory')
coda.expression_eval_node(expr, cursor)
print('expr depth:', coda.cursor_get_depth(cursor))
coda.expression_delete(expr)

# product class etc
product = coda.open('AE_TEST_ALD_U_N_1B_20190105T011602023_008364010_002143_0001.DBL')
print('class', coda.get_product_class(product))
print('type', coda.get_product_type(product))
print('version', coda.get_product_version(product))
print('description', coda.get_description(product))

# product/cursor methods
cursor = coda.Cursor()
coda.cursor_set_product(cursor, product)

print('description', coda.get_description(product))
print('description', coda.get_description(cursor))

coda.cursor_goto(cursor, 'geolocation')
coda.cursor_goto_array_element_by_index(cursor, 0)
coda.cursor_goto(cursor, 'start_of_observation_time')
coda.cursor_use_base_type_of_special_type(cursor)
data = coda.cursor_read_bytes(cursor, 0, 4)
print(type(data), data.shape, data.dtype, data)

# expressions
expr = coda.expression_from_string('1+2')
print(coda.expression_is_constant(expr))
print(coda.expression_is_equal(expr, expr))
result = coda.expression_eval_integer(expr)
print(result)
type_ = coda.expression_get_type(expr)
name = coda.expression_get_type_name(type_)
print('type', type_, name)
coda.expression_delete(expr)

expr = coda.expression_from_string('4.5')
print(coda.expression_eval_float(expr, cursor))
coda.expression_delete(expr)

expr = coda.expression_from_string('true')
print(coda.expression_eval_bool(expr))
coda.expression_delete(expr)

expr = coda.expression_from_string('"bananen" + "vla"')
print(coda.expression_eval_string(expr))
coda.expression_delete(expr)

# time
parts = coda.time_double_to_parts(12345.67890)
print(parts)
parts_utc = coda.time_double_to_parts_utc(12345.67890)
print(parts_utc)
s = coda.time_double_to_string(12345.67890, 'yyyy-mm-dd')
print(s)
s_utc = coda.time_double_to_string_utc(12345.67890, 'yyyy-mm-dd')
print(s_utc)

d = coda.time_parts_to_double(*parts)
print(d)
d = coda.time_parts_to_double_utc(*parts_utc)
print(d)
s = coda.time_parts_to_string(*parts, 'yyyy-mm-dd')
print(s)

d = coda.time_string_to_double('yyyy-mm-dd', s)
print(d)
d = coda.time_string_to_double_utc('yyyy-mm-dd', s)
print(d)
parts = coda.time_string_to_parts( 'yyyy-mm-dd', s)
print(parts)

#options
coda.set_option_bypass_special_types(1)
print(coda.get_option_bypass_special_types())
coda.set_option_perform_boundary_checks(0)
print(coda.get_option_perform_boundary_checks())
coda.set_option_perform_conversions(1)
print(coda.get_option_perform_conversions())
coda.set_option_use_fast_size_expressions(0)
print(coda.get_option_use_fast_size_expressions())
coda.set_option_use_mmap(1)
print(coda.get_option_use_mmap())

#finalize
coda.close(product)
coda.done()
