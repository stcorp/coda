import os
import sys

import numpy

# hook into build tree

# use ../.libs/libcoda.so
os.environ['CODA_LIBRARY_PATH'] = '../.libs'

# use ./definitions
os.environ['CODA_DEFINITION'] = 'definitions'

# use ../python/coda
sys.path.insert(0, '../python')
import coda



class TestCoda:
    def setUp(self):
        self.product = coda.open_as('products/left.nc', 'CODA_TEST', 'LEFT', 0)

    def tearDown(self):
        coda.close(self.product)

    def test_product(self):
        p = self.product
        assert p.product_class == 'CODA_TEST'
        assert p.product_type == 'LEFT'
        assert p.version == 0
        assert p.field_names == ['t', 't4', 'x']

    def test_double(self):
        value = self.product.fetch('x')
        assert value == 1.1

    def test_float_array(self):
        value = self.product.fetch('t')
        assert isinstance(value, numpy.ndarray)
        assert value.shape == (3,)
        assert value.dtype == 'f4'
        assert list(value) == [0, 0, 0]
