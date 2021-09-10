# Copyright (C) 2015-2021 S[&]T, The Netherlands.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""CODA Python interface

This package implements the CODA Python interface. The interface consists
of a low-level part, corresponding directly to the C interface, and a high-
level part, which adds an object-oriented wrapper layer and additional
convenience methods.

The Python interface depends on the '_cffi_backend' module, which is part
of the C foreign function interface (cffi) package. This package must be
installed in order to be able to use the Python interface.

Using the high-level interface, a CODA Product is represented by an
instance of class Product. One or more instances of class Cursor can be
used to navigate a product, and extract CODA types and product data. CODA
types are represented as instances of class Type. There are several
subclasses of Type, corresponding to the different CODA type classes.

Example of basic usage:

    import coda

    with coda.Product('somefile.nc') as product:
        # use cursor
        cursor = product.cursor()
        cursor.goto('a/b')
        data = cursor.fetch()

        # use convenience method
        data = product.fetch('a/b')

Also using the high-level interface, a CODA expression is represented by
an instance of class Expression.

For the convenience methods, it is possible to specify a path indicating
from where to retrieve data. A path consists of a sequence of strings and
integers, which are resolved from the respective location. The path can be
given as a CODA node expression or as one or more positional arguments.

For both the Cursor and Expression classes, there are much fewer methods
than there are functions in the low-level interface, because in Python
functions can return different types of values. For example, rather than
having to call cursor_read_uint8(cursor), we can just call cursor.fetch().

Further information (also about the low-level interface) is available in
the CODA documentation.

"""

from __future__ import print_function

import copy
import functools
import io
import os
import platform
import sys
import threading

import numpy

from ._codac import ffi as _ffi
ffinew = _ffi.new

PY3 = sys.version_info[0] == 3

if PY3:
    long = int

    def _is_str(s):
        return isinstance(s, str)
else:
    def _is_str(s):
        return isinstance(s, (str, unicode))


# use thread-local storage to avoid calling _ffi.new all the time

class ThreadLocalState(threading.local):
    def __init__(self):
        self.double = _ffi.new('double *')


TLS = ThreadLocalState()


#
# high-level interface
#

class Error(Exception):
    """Exception base class for all CODA Python interface errors."""


class FetchError(Error):
    """Exception raised when an errors occurs when fetching data.

    Attributes:
        str       --  error message

    """

    def __init__(self, str):
        super(Error, self).__init__(self)
        self.str = str

    def __str__(self):
        return "CODA FetchError: " + self.str


class CodacError(Error):
    """Exception raised when an error occurs inside the CODA C library.

    Attributes:
        errno       --  error code; if None, the error code will be retrieved from
                        the CODA C library.
        strerror    --  error message; if None, the error message will be retrieved
                        from the CODA C library.

    """

    def __init__(self, function=None):
        super(CodacError, self).__init__(self)

        errno = _lib.coda_get_errno()[0]
        strerror = _decode_string(_ffi.string(_lib.coda_errno_to_string(errno)))
        if function:
            strerror = function + '(): ' + strerror

        self.errno = errno
        self.strerror = strerror

    def __str__(self):
        return self.strerror


def _check(return_code, function=None):
    if return_code != 0:
        raise CodacError(function=function)


class Node(object):
    """Base class of 'Product' and 'Cursor' classes.

    This class contains shared functionality between Product and Cursor.
    For this functionality, a Product can be used as if it were a Cursor
    (pointing at the product root).

    """

    __slots__ = []

    def fetch(self, *path):
        """Return all product data (recursively) for the current data item
        (or as specified by a path).

        This can result in a combination of nested 'Record' instances,
        numpy arrays, scalars, strings and so on.

        Some examples:

            data = product.fetch('fieldname')
            data = cursor.fetch('a/b')

        Arguments:
        path -- path description (optional)
        """
        return fetch(self, *path)

    def get_attributes(self, *path):
        """Return a 'Record' instance containing the attributes for the
        current data item (or as specified by a path).

        Arguments:
        path -- path description (optional)
        """
        return get_attributes(self, *path)

    @property
    def attributes(self):
        """Return a 'Record' instance containing the attributes for the
        current data item.
        """
        return get_description(self, *path)

    def get_description(self, *path):
        """Return the description in the product format definition for the
        current data item (or as specified by a path).

        Arguments:
        path -- path description (optional)
        """
        return get_description(self, *path)

    @property
    def description(self):
        """Return the description (as a string) in the product format
        definition for the current data item.
        """
        return get_description(self)

    @property
    def get_unit(self, *path):
        """Return unit information (as a string) in the product format
        definition for the current data item (or as specified by a path).

        Arguments:
        path -- path description (optional)
        """
        return get_unit(self, *path)

    @property
    def unit(self):
        """Return unit information (as a string) in the product format
        definition for the current data item.
        """
        return get_unit(self)

    def cursor(self, *path):
        """Return a new 'Cursor' instance, pointing to the same data
        item (or as specified by a path).

        Arguments:
        path -- path description (optional)
        """
        return Cursor(self, *path)

    def read_partial_array(self, offset, count):
        """Return partial (flat) array data, using specified offset and count.

        C array ordering conventions are used.

        Arguments:
        offset -- (flat) array index
        count -- number of elements to read
        """
        cursor = self.cursor()

        nodeType = cursor_get_type(cursor)
        baseType = type_get_array_base_type(nodeType)
        readType = type_get_read_type(baseType)

        return _readNativeTypePartialArrayFunctionDictionary[readType](cursor, offset, count)

    def field_available(self, *path):
        """Return a boolean indicating whether a record field is available.

        The last item of the path description must point to a record field.

        Arguments:
        path -- path description (optional)
        """
        return get_field_available(self, *path)

    def field_count(self, *path):
        """Return the number of fields in a record.

        The last item of the path must point to a record.

        Arguments:
        path -- path description (optional)
        """
        return get_field_count(self, *path)

    def field_names(self, *path):
        """Return the names of the fields in a record.

        The last item of the path must point to a record.

        Arguments:
        path -- path description (optional)
        """
        return get_field_names(self, *path)


class Product(Node):
    """CODA Product class.

    An instance of this class represents a CODA product.

    It is a wrapper class around the low-level coda_product struct.

    It implements the context-manager protocol for conveniently
    closing (these low-level) products.

    """

    __slots__ = ['_x']

    def __init__(self, path=None, _x=None):
        """Initialize a 'Product' instance for specified product file.

        The instance should be cleaned up after use via 'with' keyword or
        by calling the 'close' method (or global function).

        Arguments:
        path -- path to product file
        """
        if path is not None:
            self._x = open(path)._x
        else:
            self._x = _x

    def close(self):
        """Close the low-level CODA product.

        Note that it is also possible to use the 'with' keyword for this.
        """
        close(self)

    @property
    def version(self):
        """Return the product type version.
        """
        return get_product_version(self)

    @property
    def product_class(self):
        """Return the name of the product class.
        """
        return get_product_class(self)

    @property
    def product_type(self):
        """Return the name of the product type.
        """
        return get_product_type(self)

    @property
    def format(self):
        """Return the name of the product format."""
        return type_get_format_name(get_product_format(self))

    @property
    def definition_file(self):
        """Return the path to the coda definition file that describes the product format.
        """
        return get_product_definition_file(self)

    @property
    def file_size(self):
        """Return the product file size.
        """
        return get_product_file_size(self)

    @property
    def filename(self):
        """Return the product filename.
        """
        return get_product_filename(self)

    @property
    def root_type(self):
        """Return the CODA type of the root of the product.
        """
        return get_product_root_type(self)

    def variable_value(self, variable, index=0):
        """Return the value for a product variable.

        Product variables are used to store frequently needed
        information of a product (information that is needed to
        calculate byte offsets or array sizes within a product).

        Consult the CODA Product Definition Documentation for
        an overview of product variables for a certain product type.

        Product variables can be one-dimensional arrays, in which an
        index must be passed.

        Arguments:
        variable -- name of product variable
        index -- array index of the product variable (optional)
        """
        return get_product_variable_value(self, variable, index)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        close(self)


class Cursor(Node):
    """CODA Cursor class.

    An instance of this class represents a CODA cursor.

    It is a wrapper class around the low-level coda_cursor struct.

    Cursors are used to navigate a product hierarchy, and
    extract CODA types and product data.

    Internally, a 'Cursor' instance consists of a stack of pointers,
    making it possible to easily move up and down a product hierarchy.

    """

    __slots__ = ['_x']

    def __init__(self, obj=None, *path):
        """Initialize a 'Cursor' instance.

        If a 'Cursor' instance is passed, the cursor will point to the
        same location.

        If a 'Product' instance is passed, the cursor will point to the
        product root.

        If a path is given, the cursor location will then be changed
        to point as specified.

        If no arguments are given, the 'set_product' method should be
        used to point to a 'Product' instance.

        Arguments:
        obj -- existing 'Cursor' or 'Product' instance (optional)
        path -- path description (optional)
        """
        self._x = _ffi.new('coda_cursor *')

        if obj is not None:
            if isinstance(obj, Product):
                self.set_product(obj)
            elif isinstance(obj, Cursor):
                obj._copy_state_to(self)
            else:
                raise TypeError('argument to Cursor.__init__ must be None, Product or Cursor')

        if path:
            self.goto(*path)

    def _copy_state_to(self, other):
        _ffi.buffer(other._x)[:] = _ffi.buffer(self._x)[:]

    def __deepcopy__(self, memo):
        cursor = Cursor()
        self._copy_state_to(cursor)
        return cursor

    def goto(self, *path):
        """Move the cursor as specified by 'path'.

        Arguments:
        path -- path description (optional)
        """
        _traverse_path(self, path)
        return self

    def goto_parent(self):
        """Move the cursor one level up in the hierarchy.
        """
        cursor_goto_parent(self)
        return self

    def goto_root(self):
        """Move the cursor to the product root.
        """
        cursor_goto_root(self)
        return self

    def goto_first_record_field(self):
        """Move the cursor to the first record field.
        """
        cursor_goto_first_record_field(self)
        return self

    def goto_next_record_field(self):
        """Move the cursor to the next record field.
        """
        cursor_goto_next_record_field(self)
        return self

    def goto_record_field_by_index(self, index):
        """Move the cursor to the record field with the given index.

        Arguments:
        index -- field index
        """
        cursor_goto_record_field_by_index(self, index)
        return self

    def goto_record_field_by_name(self, name):
        """Move the cursor to the record field with the given name.

        Arguments:
        index -- field name
        """
        cursor_goto_record_field_by_name(self, name)
        return self

    def goto_first_array_element(self):
        """Move the cursor to the first array element.
        """
        cursor_goto_first_array_element(self)
        return self

    def goto_next_array_element(self):
        """Move the cursor to the next array element.
        """
        cursor_goto_next_array_element(self)
        return self

    def goto_array_element(self, idcs):
        """Move the cursor to the array element with the given indices.

        Arguments:
        idcs -- sequence of indices (one per dimension)
        """
        cursor_goto_array_element(self, idcs)
        return self

    def goto_array_element_by_index(self, index):
        """Move the cursor to the array element with the given index.

        A multi-dimensional array is treated as a one-dimensional array
        (with the same number of elements).

        The ordering in such a one dimensional array is by definition
        chosen to be equal to the way the array elements are stored as a
        sequence in the product file.

        The mapping of a one dimensional index for each multidimensional
        data array to an array of subscripts (and vice versa) is defined
        in such a way that the last element of a subscript array is the
        one that is the fastest running index (i.e. C array ordering).

        All multidimensional arrays have their dimensions defined using C
        array ordering in CODA.

        Arguments:
        index -- array index
        """
        cursor_goto_array_element_by_index(self, index)
        return self

    def goto_available_union_field(self):
        """Move the cursor to the available union field.
        """
        cursor_goto_available_union_field(self)
        return self

    def goto_attributes(self):
        """Move the cursor to a (virtual) record containing the attributes
        of the current data element.
        """
        cursor_goto_attributes(self)
        return self

    def set_product(self, product):
        """Initialize the cursor to point to the given product root.

        Arguments:
        product -- 'Product' instance
        """
        cursor_set_product(self, product)

    @property
    def product(self):
        """Return the corresponding 'Product' instance.
        """
        return cursor_get_product_file(self)

    def num_elements(self):
        """Return the number of array or record elements (or 1 for other
        types."""
        return cursor_get_num_elements(self)

    def string_length(self):
        """Return the length in bytes of a string."""
        return cursor_get_string_length(self)

    def use_base_type_of_special_type(self):
        """Reinterpret special data using the special type base type.

        All special data types have a base type that can be used to read
        the data in its raw form (e.g. for ASCII time data the type will
        change to a string type and for binary compound time data the type
        will change to a record with fields containing binary numbers).
        """
        cursor_use_base_type_of_special_type(self)

    @property
    def coda_type(self):
        """Return a 'Type' instance corresponding to the CODA type for
        the current location.
        """
        return cursor_get_type(self)

    @property
    def type_class(self):
        """Return the name of the CODA type class for the current
        location.
        """
        return type_get_class_name(cursor_get_type_class(self))

    @property
    def special_type(self):
        """Return the name of the special type for the current
        location.
        """
        return type_get_special_type_name(cursor_get_special_type(self))

    @property
    def format(self):
        """Return the name of the storage format for the current
        location.
        """
        return type_get_format_name(cursor_get_format(self))

    @property
    def has_attributes(self):
        """Return a boolean indicating if there are attributes for
        the current location.
        """
        return bool(cursor_has_attributes(self))

    @property
    def has_ascii_content(self):
        """Return a boolean indicating if the data for the current
        location is stored in ASCII format.
        """
        return bool(cursor_has_ascii_content(self))

    @property
    def available_union_field_index(self):
        """Return the index of the available union field.
        """
        return cursor_get_available_union_field_index(self)

    def record_field_is_available(self, index):
        """Return a boolean indicating if a record field is available.

        Arguments:
        index -- record field index
        """
        return bool(cursor_get_record_field_available_status(self, index))

    def record_field_index_from_name(self, name):
        """Return record field index for the field with the given name.

        Arguments:
        name -- record field name
        """
        return cursor_get_record_field_index_from_name(self, name)

    @property
    def array_dim(self):
        """Return a list containing the dimensions of an array.
        """
        return cursor_get_array_dim(self)

    @property
    def depth(self):
        """Return the hierarchical depth of the current location.
        """
        return cursor_get_depth(self)

    @property
    def index(self):
        """Return the array or record field index for the current location.

        For arrays, a 'flat' index is returned (similator to the argument
        of the 'goto_array_element_by_index' method).
        """
        return cursor_get_index(self)

    def bit_size(self):
        """Return the bit size for the current location.
        """
        return cursor_get_bit_size(self)

    def byte_size(self):
        """Return the byte size for the current location.

        It is calculated by rounding *up* the bit size to the nearest byte.
        """
        return cursor_get_byte_size(self)

    @property
    def file_bit_offset(self):
        """Return the file offset in bits for the current location.
        """
        return cursor_get_file_bit_offset(self)

    @property
    def file_byte_offset(self):
        """Return the file offset in bytes for the current location.

        It is calculated by rounding *down* the bit offset to the nearest
        byte.
        """
        return cursor_get_file_byte_offset(self)


class Record(object):
    """CODA Record class.

    An instance of this class represents a CODA record.

    Each record field will appear as an instance attribute. The field
    name is used as the name of the attribute, and its value is read from
    the product file.
    """

    __slots__ = []


class Type(object):
    """CODA Type base class.

    An instance of this class represents a CODA type.

    It is a wrapper class around the low-level coda_type struct.

    Specialized functionality corresponding to the different CODA
    types is provided by the following subclasses:

    - 'IntegerType'
    - 'RealType'
    - 'RecordType'
    - 'ArrayType'
    - 'SpecialType'
    - 'TextType'
    - 'RawType'

    """

    __slots__ = ['_x']

    def __init__(self, _x):
        self._x = _x

    @property
    def type_class(self):
        """Return the name of the type class.
        """
        return type_get_class_name(type_get_class(self))

    @property
    def format(self):
        """Return the name of the type storage format.
        """
        return type_get_format_name(type_get_format(self))

    @property
    def special_type(self):
        """Return the name of the special type."""
        return type_get_special_type_name(type_get_special_type(self))

    @property
    def description(self):
        """Return the type description.
        """
        return type_get_description(self)

    @property
    def has_attributes(self):
        """Return a boolean indicating whether the type has any
        attributes.
        """
        return bool(type_has_attributes(self))

    @property
    def attributes(self):
        """Return the type for the associated attribute record.
        """
        return type_get_attributes(self)

    @property
    def read_type(self):
        """Return the best native type for reading the data.
        """
        return type_get_read_type(self)

    @property
    def unit(self):
        """Return the type unit.
        """
        return type_get_unit(self)

    @property
    def bit_size(self):
        """Return the bit size for the type.
        """
        return type_get_bit_size(self)

    @property
    def fixed_value(self):
        """Return the associated fixed value string for the type.
        """
        return type_get_fixed_value(self)


class IntegerType(Type):
    """CODA Integer Type class."""

    __slots__ = []


class RealType(Type):
    """CODA Real Type class."""

    __slots__ = []


class RecordTypeField(object):
    """CODA Record Type Field class.
    """

    __slots__ = ['recordtype', 'index']

    def __init__(self, recordtype, index):
        self.recordtype = recordtype
        self.index = index

    @property
    def is_hidden(self):
        """Return a boolean indicating whether the field is hidden.
        """
        return bool(type_get_record_field_hidden_status(self.recordtype, self.index))

    @property
    def is_optional(self):
        """Return a boolean indicating whether the field is optional
        (not always available).
        """
        return type_get_record_field_available_status(self.recordtype, self.index) == -1

    @property
    def coda_type(self):
        """Return 'Type' instance corresponding to the field.
        """
        return type_get_record_field_type(self.recordtype, self.index)

    @property
    def name(self):
        """Return the name (identifier) of the field
        """
        return type_get_record_field_name(self.recordtype, self.index)

    @property
    def real_name(self):
        """Return the real (original) name of the field.

        This may be different from the regular name (identifier) because
        of restrictions on identifier names.
        """
        return type_get_record_field_real_name(self.recordtype, self.index)


class RecordType(Type):
    """CODA Record Type class.

    Unions are implemented in CODA as records where only one field is
    'available' at a time.

    """

    __slots__ = []

    def num_fields(self):
        """Return the total number of fields.
        """
        return type_get_num_record_fields(self)

    def field(self, index):
        """Return an instance of 'RecordTypeField' corresponding to the
        specified field index or name.

        Arguments:
        index -- field index or name
        """
        if _is_str(index):
            index = type_get_record_field_index_from_name(self, index)
        return RecordTypeField(self, index)

    def fields(self):
        """Return a list of 'RecordTypeField' instances corresponding
        to the fields.
        """
        result = []
        for i in range(self.num_fields()):
            result.append(RecordTypeField(self, i))
        return result

    @property
    def is_union(self):
        """Return a boolean indicating whether the record is a union.
        """
        return bool(type_get_record_union_status(self))


class ArrayType(Type):
    """CODA Array Type class."""

    __slots__ = []

    @property
    def base_type(self):
        """Return a 'Type' instance corresponding to the array elements.
        """
        return type_get_array_base_type(self)

    @property
    def dim(self):
        """Return a list with array dimension sizes.

        The size of a variable dimension is represented as -1.
        """
        return type_get_array_dim(self)


class SpecialType(Type):
    """CODA Special Type class."""

    __slots__ = []

    @property
    def base_type(self):
        """Return a 'Type' instance corresponding to the special type
        base type.
        """
        return type_get_special_base_type(self)


class TextType(Type):
    """CODA Text Type class."""

    __slots__ = []

    @property
    def string_length(self):
        """Return the string length in bytes.
        """
        return type_get_string_length(self)


class RawType(Type):
    """CODA Raw Type class."""

    __slots__ = []


class Expression(object):
    """CODA Expression class.

    An instance of this class represents a CODA expression.

    It is a wrapper class around the low-level coda_expression struct.

    Consult the CODA documentation for information about the the CODA
    expression language.

    """

    __slots__ = ['_x']

    def __init__(self, s=None, _x=None):
        """Initialize an 'Expression' instance.

        The instance should be cleaned up after use via 'with' keyword or
        by calling the 'delete' method.

        Arguments:
        s -- string containing CODA expression
        """
        if s is not None:
            self._x = expression_from_string(s)._x
        else:
            self._x = _x

    def is_constant(self):
        """Return a boolean indicating whether the expression is constant.

        An expression is constant if it does not depend on the contents of
        a product and hence can be evaluated without requiring a cursor.
        """
        return bool(expression_is_constant(self))

    def is_equal(self, expr):
        """Return a boolean indicating whether the expression is equal
        to another 'Expression' instance.

        For two expressions to be considered as equal, all operands to an
        operation need to be equal and operands need to be provided in the
        same order.

        For example, the expression '1!=2' is not considered equal to the
        expression '2!=1'.

        Arguments:
        expr -- 'Expression' instance
        """
        return bool(expression_is_equal(self, expr))

    def eval(self, cursor=None):
        """Evaluate the expression and return the resulting value.

        For a constant expression, the 'cursor' argument is optional.

        For a node expression, the cursor is moved to the resulting
        location and no value is returned.

        Arguments:
        cursor -- 'Cursor' instance (optional)
        """
        expression_type = self.expression_type
        if expression_type == 'boolean':
            return bool(expression_eval_bool(self, cursor))
        elif expression_type == 'integer':
            return expression_eval_integer(self, cursor)
        elif expression_type == 'float':
            return expression_eval_float(self, cursor)
        elif expression_type == 'string':
            return expression_eval_string(self, cursor)
        elif expression_type == 'node':
            return expression_eval_node(self, cursor)

    @property
    def expression_type(self):
        """Return the name of the expression type.
        """
        return expression_get_type_name(expression_get_type(self))

    def delete(self):
        """Delete the low-level CODA expression object.

        Note that it is also possible to use the 'with' keyword for this.
        """
        return expression_delete(self)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.delete()


def recognize_file(path):
    """Return a list containing the file size, format, product class,
    product type, and format version of a product file.

    Arguments:
    path -- path to product file
    """

    x = _ffi.new('int64_t *')
    y = _ffi.new('enum coda_format_enum *')
    z = _ffi.new('char **')
    a = _ffi.new('char **')
    b = _ffi.new('int *')

    _check(_lib.coda_recognize_file(_encode_path(path), x, y, z, a, b), 'coda_recognize_file')

    return [long(x[0]), y[0], _string(z[0]), _string(a[0]), b[0]]


def open(path):
    """Return a 'Product' instance for the specified product file.

    Note that products can also be opened as follows:

        product = Product(path)

    Arguments:
    path -- path to CODA compatible product file.
    """
    x = _ffi.new('coda_product **')
    _check(_lib.coda_open(_encode_path(path), x), 'coda_open')
    return Product(_x=x[0])


def open_as(path, product_class, product_type, version):
    """Return a 'Product' instance for the specified product file,
    using the specified format definition.

    Arguments:
    path -- path to CODA compatible product file.
    product_class -- product class name
    product_type -- product type name
    version -- format version number (-1 for latest)
    """
    x = _ffi.new('coda_product **')
    class_ = _encode_string(product_class)
    type_ = _encode_string(product_type)
    _check(_lib.coda_open_as(_encode_path(path), class_, type_, version, x), 'coda_open_as')
    return Product(_x=x[0])


def close(product):
    """Close the given 'Product' instance.

    This will release any memory used by the low-level CODA product.

    Note that products can also be closed as follows:

        product.close()

    The 'with' keyword can also be used for this purpose.

    Arguments:
    product -- instance of 'Product'.

    """
    _check(_lib.coda_close(product._x), 'coda_close')


#
# low-level interface
#

def match_filefilter(filter_, paths, callback):
    if _is_str(paths):
        paths = [paths]

    def passer(filepath, status, error, userdata):
        callback(_string(filepath), status, _string(error))
        return 0

    fptr = _ffi.callback(
                ' int (char *, enum coda_filefilter_status_enum, char *, void *)',
                passer)
    npaths = len(paths)
    paths2 = _ffi.new('char *[%d]' % npaths)
    for i, path in enumerate(paths):
        paths2[i] = _ffi.new('char[]', _encode_string(paths[i]))
    voidp = _ffi.new('char *')

    _check(_lib.coda_match_filefilter(_encode_string(filter_), npaths, paths2, fptr, voidp))


def _string(s):
    if s != _ffi.NULL:
        return _decode_string(_ffi.string(s))


def get_product_class(product):
    c = _ffi.new('char **')
    _check(_lib.coda_get_product_class(product._x, c), 'coda_get_product_class')
    return _string(c[0])


def get_product_version(product):
    c = _ffi.new('int *')
    _check(_lib.coda_get_product_version(product._x, c), 'coda_get_product_version')
    return c[0]


def get_product_type(product):
    c = _ffi.new('char **')
    _check(_lib.coda_get_product_type(product._x, c), 'coda_get_product_type')
    return _string(c[0])


def get_product_filename(product):
    c = _ffi.new('char **')
    _check(_lib.coda_get_product_filename(product._x, c), 'coda_get_product_filename')
    return _string(c[0])


def get_product_definition_file(product):
    c = _ffi.new('char **')
    _check(_lib.coda_get_product_definition_file(product._x, c), 'coda_get_product_definition_file')
    return _string(c[0])


def get_product_file_size(product):
    c = _ffi.new('int64_t *')
    _check(_lib.coda_get_product_file_size(product._x, c), 'coda_get_product_file_size')
    return long(c[0])


def get_product_format(product):
    c = _ffi.new('enum coda_format_enum *')
    _check(_lib.coda_get_product_format(product._x, c), 'coda_get_product_format')
    return c[0]


def _type(coda_type):
    x = _ffi.new('enum coda_type_class_enum *')
    _check(_lib.coda_type_get_class(coda_type, x), 'coda_type_get_class')
    return _codaClassToTypeClass[x[0]](coda_type)


def get_product_root_type(product):
    c = _ffi.new('coda_type **')
    _check(_lib.coda_get_product_root_type(product._x, c), 'coda_get_product_root_type')
    return _type(c[0])


def get_product_variable_value(product, variable, index):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_get_product_variable_value(product._x, _encode_string(variable), index, x),
           'coda_get_product_variable_value')
    return long(x[0])


def cursor_set_product(cursor, product):
    _check(_lib.coda_cursor_set_product(cursor._x, product._x), 'coda_cursor_set_product')


def cursor_goto(cursor, path):
    if _lib.coda_cursor_goto(cursor._x, _encode_string(path)) != 0:
        raise CodacError('coda_cursor_goto')


def cursor_goto_parent(cursor):
    if _lib.coda_cursor_goto_parent(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_parent')


def cursor_goto_root(cursor):
    if _lib.coda_cursor_goto_root(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_root')


def cursor_goto_attributes(cursor):
    if _lib.coda_cursor_goto_attributes(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_attributes')


def cursor_goto_first_array_element(cursor):
    if _lib.coda_cursor_goto_first_array_element(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_first_array_element')


def cursor_goto_next_array_element(cursor):
    if _lib.coda_cursor_goto_next_array_element(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_next_array_element')


def cursor_goto_array_element_by_index(cursor, index):
    if _lib.coda_cursor_goto_array_element_by_index(cursor._x, index) != 0:
        raise CodacError('coda_cursor_goto_array_element_by_index')


def cursor_goto_array_element(cursor, idcs):
    n = len(idcs)
    s = _ffi.new('long [%d]' % n)
    for i, val in enumerate(idcs):
        s[i] = val
    if _lib.coda_cursor_goto_array_element(cursor._x, n, s) != 0:
        raise CodacError('coda_cursor_goto_array_element')


def cursor_goto_first_record_field(cursor):
    if _lib.coda_cursor_goto_first_record_field(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_first_record_field')


def cursor_goto_next_record_field(cursor):
    if _lib.coda_cursor_goto_next_record_field(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_next_record_field')


def cursor_goto_available_union_field(cursor):
    if _lib.coda_cursor_goto_available_union_field(cursor._x) != 0:
        raise CodacError('coda_cursor_goto_available_union_field')


def cursor_goto_record_field_by_index(cursor, index):
    if _lib.coda_cursor_goto_record_field_by_index(cursor._x, index) != 0:
        raise CodacError('coda_cursor_goto_record_field_by_index')


def cursor_goto_record_field_by_name(cursor, name):
    if _lib.coda_cursor_goto_record_field_by_name(cursor._x, _encode_string(name)) != 0:
        raise CodacError('coda_cursor_goto_record_field_by_name')


def cursor_use_base_type_of_special_type(cursor):
    _check(_lib.coda_cursor_use_base_type_of_special_type(cursor._x), 'coda_cursor_use_base_type_of_special_type')


def cursor_get_depth(cursor):
    x = _ffi.new('int *')
    _check(_lib.coda_cursor_get_depth(cursor._x, x), 'coda_cursor_get_depth')
    return x[0]


def cursor_get_index(cursor):
    x = _ffi.new('long *')
    _check(_lib.coda_cursor_get_index(cursor._x, x), 'coda_cursor_get_index')
    return x[0]


def cursor_get_array_dim(cursor):
    x = _ffi.new('int *')
    y = _ffi.new('long[%d]' % _lib.CODA_MAX_NUM_DIMS)
    _check(_lib.coda_cursor_get_array_dim(cursor._x, x, y), 'coda_cursor_get_array_dim')
    return list(y)[:x[0]]


def cursor_get_record_field_available_status(cursor, index):
    x = _ffi.new('int *')
    _check(_lib.coda_cursor_get_record_field_available_status(cursor._x, index, x),
           'coda_cursor_get_record_field_available_status')
    return x[0]


def cursor_get_record_field_index_from_name(cursor, name):
    x = _ffi.new('long *')
    _check(_lib.coda_cursor_get_record_field_index_from_name(cursor._x, _encode_string(name), x),
           'coda_cursor_get_record_field_index_from_name')
    return x[0]


def cursor_get_available_union_field_index(cursor):
    x = _ffi.new('long *')
    _check(_lib.coda_cursor_get_available_union_field_index(cursor._x, x),
           'coda_cursor_get_available_union_field_index')
    return x[0]


def cursor_has_attributes(cursor):
    x = _ffi.new('int *')
    _check(_lib.coda_cursor_has_attributes(cursor._x, x), 'coda_cursor_has_attributes')
    return x[0]


def cursor_get_product_file(cursor):
    x = _ffi.new('coda_product **')
    _check(_lib.coda_cursor_get_product_file(cursor._x, x), 'coda_get_product_file')
    return Product(_x=x[0])


def _read_scalar(cursor, type_):
    x = _ffi.new('%s *' % type_)
    desc = type_
    if desc.endswith('_t'):
        desc = desc[:-2]
    func = getattr(_lib, 'coda_cursor_read_%s' % desc)
    _check(func(cursor._x, x), 'coda_cursor_read_%s' % desc)
    return x[0]


def _read_array(cursor, type_, order):
    shape = cursor_get_array_dim(cursor)
    size = functools.reduce(lambda x, y: x*y, shape, 1)
    d = _ffi.new('%s[%d]' % (type_, size))
    desc = type_
    if desc.endswith('_t'):
        desc = desc[:-2]
    func = getattr(_lib, 'coda_cursor_read_%s_array' % desc)
    _check(func(cursor._x, d, order), 'coda_cursor_read_%s_array' % desc)
    buf = _ffi.buffer(d)
    if desc == 'char':
        array = numpy.array(buf, dtype='int8')
        array = array.reshape(shape)
    else:
        if desc == 'float':
            desc = 'float32'
        array = numpy.ndarray(shape=shape, buffer=buf, dtype=desc)
    return array


def _read_partial(cursor, type_, offset, count):
    d = _ffi.new('%s[%d]' % (type_, count))
    desc = type_
    if desc.endswith('_t'):
        desc = desc[:-2]
    func = getattr(_lib, 'coda_cursor_read_%s_partial_array' % desc)
    _check(func(cursor._x, offset, count, d), 'coda_cursor_read_%s_partial_array' % desc)
    buf = _ffi.buffer(d)
    if desc == 'char':
        array = numpy.array(buf, dtype='int8')
    else:
        array = numpy.frombuffer(buf)
    return array


def cursor_read_char(cursor):
    return _decode_string(_read_scalar(cursor, 'char'))


def cursor_read_char_array(cursor, order=0):
    return _read_array(cursor, 'char', order)


def cursor_read_char_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'char', offset, count)


def cursor_read_int8(cursor):
    return _read_scalar(cursor, 'int8_t')


def cursor_read_int8_array(cursor, order=0):
    return _read_array(cursor, 'int8_t', order)


def cursor_read_int8_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'int8_t', offset, count)


def cursor_read_int16(cursor):
    return _read_scalar(cursor, 'int16_t')


def cursor_read_int16_array(cursor, order=0):
    return _read_array(cursor, 'int16_t', order)


def cursor_read_int16_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'int16_t', offset, count)


def cursor_read_int32(cursor):
    return _read_scalar(cursor, 'int32_t')


def cursor_read_int32_array(cursor, order=0):
    return _read_array(cursor, 'int32_t', order)


def cursor_read_int32_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'int32_t', offset, count)


def cursor_read_int64(cursor):
    return long(_read_scalar(cursor, 'int64_t'))


def cursor_read_int64_array(cursor, order=0):
    return _read_array(cursor, 'int64_t', order)


def cursor_read_int64_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'int64_t', offset, count)


def cursor_read_uint8(cursor):
    return _read_scalar(cursor, 'uint8_t')


def cursor_read_uint8_array(cursor, order=0):
    return _read_array(cursor, 'uint8_t', order)


def cursor_read_uint8_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'uint8_t', offset, count)


def cursor_read_uint16(cursor):
    return _read_scalar(cursor, 'uint16_t')


def cursor_read_uint16_array(cursor, order=0):
    return _read_array(cursor, 'uint16_t', order)


def cursor_read_uint16_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'uint16_t', offset, count)


def cursor_read_uint32(cursor):
    return _read_scalar(cursor, 'uint32_t')


def cursor_read_uint32_array(cursor, order=0):
    return _read_array(cursor, 'uint32_t', order)


def cursor_read_uint32_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'uint32_t', offset, count)


def cursor_read_uint64(cursor):
    return long(_read_scalar(cursor, 'uint64_t'))


def cursor_read_uint64_array(cursor, order=0):
    return _read_array(cursor, 'uint64_t', order)


def cursor_read_uint64_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'uint64_t', offset, count)


def cursor_read_float(cursor):
    return _read_scalar(cursor, 'float')


def cursor_read_float_array(cursor, order=0):
    return _read_array(cursor, 'float', order)


def cursor_read_float_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'float', offset, count)


def cursor_read_double(cursor):
    x = TLS.double
    if _lib.coda_cursor_read_double(cursor._x, x) != 0:
        raise CodacError('coda_cursor_read_double')
    return x[0]


def cursor_read_double_array(cursor, order=0):
    return _read_array(cursor, 'double', order)


def cursor_read_double_partial_array(cursor, offset, count):
    return _read_partial(cursor, 'double', offset, count)


def cursor_read_complex(cursor):
    return complex(*cursor_read_complex_double_split(cursor))


def cursor_read_complex_double_pair(cursor):
    d = _ffi.new('double [2]')
    _check(_lib.coda_cursor_read_complex_double_pair(cursor._x, d), 'coda_cursor_read_complex_double_pair')
    return numpy.asarray(list(d), dtype=float)


def cursor_read_complex_double_split(cursor):
    d = _ffi.new('double *')
    e = _ffi.new('double *')
    _check(_lib.coda_cursor_read_complex_double_split(cursor._x, d, e), 'coda_cursor_read_complex_double_split')
    return [d[0], e[0]]


def cursor_read_complex_array(cursor, order=0):
    array = cursor_read_complex_double_pairs_array(cursor, order)
    return numpy.squeeze(array.view(dtype=complex))


def cursor_read_complex_double_pairs_array(cursor, order=0):
    shape = cursor_get_array_dim(cursor)
    size = functools.reduce(lambda x, y: x*y, shape, 1)
    d = _ffi.new('double[%d]' % (size*2))
    _check(_lib.coda_cursor_read_complex_double_pairs_array(cursor._x, d, order),
           'coda_cursor_read_complex_double_pairs_array')
    buf = _ffi.buffer(d)
    array = numpy.frombuffer(buf).reshape(tuple(shape)+(2,))
    return array


def cursor_read_complex_double_split_array(cursor, order=0):
    shape = cursor_get_array_dim(cursor)
    size = functools.reduce(lambda x, y: x*y, shape, 1)
    d = _ffi.new('double[%d]' % size)
    e = _ffi.new('double[%d]' % size)
    _check(_lib.coda_cursor_read_complex_double_split_array(cursor._x, d, e, order),
           'coda_cursor_read_complex_double_split_array')
    array1 = numpy.frombuffer(_ffi.buffer(d)).reshape(shape)
    array2 = numpy.frombuffer(_ffi.buffer(e)).reshape(shape)
    return [array1, array2]


def cursor_get_bit_size(cursor):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_cursor_get_bit_size(cursor._x, x), 'coda_cursor_get_bit_size')
    return long(x[0])


def cursor_get_byte_size(cursor):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_cursor_get_byte_size(cursor._x, x), 'coda_cursor_get_byte_size')
    return long(x[0])


def cursor_get_file_bit_offset(cursor):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_cursor_get_file_bit_offset(cursor._x, x), 'coda_cursor_get_file_bit_offset')
    return long(x[0])


def cursor_get_file_byte_offset(cursor):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_cursor_get_file_byte_offset(cursor._x, x), 'coda_cursor_get_file_byte_offset')
    return long(x[0])


def cursor_get_format(cursor):
    x = _ffi.new('enum coda_format_enum *')
    _check(_lib.coda_cursor_get_format(cursor._x, x), 'coda_cursor_get_format')
    return x[0]


def cursor_has_ascii_content(cursor):
    x = _ffi.new('int *')
    _check(_lib.coda_cursor_has_ascii_content(cursor._x, x), 'coda_cursor_has_ascii_content')
    return x[0]


def cursor_read_bytes(cursor, offset, count):
    d = _ffi.new('uint8_t[%d]' % count)
    _check(_lib.coda_cursor_read_bytes(cursor._x, d, offset, count), 'coda_cursor_read_bytes')
    buf = _ffi.buffer(d)
    array = numpy.frombuffer(buf, dtype='uint8')
    return array


def cursor_read_bits(cursor, offset, count):
    nbytes = count // 8
    if count % 8 > 0:
        nbytes += 1
    d = _ffi.new('uint8_t[%d]' % nbytes)
    _check(_lib.coda_cursor_read_bits(cursor._x, d, offset, count), 'coda_cursor_read_bits')
    buf = _ffi.buffer(d)
    array = numpy.frombuffer(buf, dtype='uint8')
    return array


def cursor_get_string_length(cursor):
    length = _ffi.new('long *')
    _check(_lib.coda_cursor_get_string_length(cursor._x, length), 'coda_cursor_get_string_length')
    return l[0]


def cursor_read_string(cursor):
    length = cursor_get_string_length(cursor)
    y = _ffi.new('char [%d]' % (l+1))
    _check(_lib.coda_cursor_read_string(cursor._x, y, length + 1), 'coda_cursor_read_string')
    return _decode_string(_ffi.unpack(y, length))


def cursor_get_type(cursor):
    x = _ffi.new('coda_type **')
    _check(_lib.coda_cursor_get_type(cursor._x, x), 'coda_cursor_get_type')
    return _type(x[0])


def cursor_get_type_class(cursor):
    x = _ffi.new('enum coda_type_class_enum *')
    _check(_lib.coda_cursor_get_type_class(cursor._x, x), 'coda_cursor_get_type_class')
    return x[0]


def cursor_get_special_type(cursor):
    x = _ffi.new('enum coda_special_type_enum *')
    _check(_lib.coda_cursor_get_special_type(cursor._x, x), 'coda_cursor_get_special_type')
    return x[0]


def cursor_get_num_elements(cursor):
    x = _ffi.new('long *')
    _check(_lib.coda_cursor_get_num_elements(cursor._x, x), 'coda_cursor_get_num_elements')
    return x[0]


def type_get_class(type_):
    x = _ffi.new('enum coda_type_class_enum *')
    _check(_lib.coda_type_get_class(type_._x, x), 'coda_type_get_class')
    return x[0]


def type_get_format(type_):
    x = _ffi.new('enum coda_format_enum *')
    _check(_lib.coda_type_get_format(type_._x, x), 'coda_type_get_format')
    return x[0]


def type_get_special_type(type_):
    x = _ffi.new('enum coda_special_type_enum *')
    _check(_lib.coda_type_get_special_type(type_._x, x), 'coda_type_get_special_type')
    return x[0]


def type_get_special_type_name(n):
    return _string(_lib.coda_type_get_special_type_name(n))


def type_get_special_base_type(type_):
    x = _ffi.new('coda_type **')
    _check(_lib.coda_type_get_special_base_type(type_._x, x), 'coda_type_get_special_base_type')
    return _type(x[0])


def type_get_array_base_type(type_):
    x = _ffi.new('coda_type **')
    _check(_lib.coda_type_get_array_base_type(type_._x, x), 'coda_type_get_array_base_type')
    return _type(x[0])


def type_get_attributes(type_):
    x = _ffi.new('coda_type **')
    _check(_lib.coda_type_get_attributes(type_._x, x), 'coda_type_get_attributes')
    return _type(x[0])


def type_get_array_num_dims(type_):
    x = _ffi.new('int *')
    _check(_lib.coda_type_get_array_num_dims(type_._x, x), 'coda_type_get_array_num_dims')
    return x[0]


def type_get_array_dim(type_):
    x = _ffi.new('int *')
    y = _ffi.new('long[%d]' % _lib.CODA_MAX_NUM_DIMS)
    _check(_lib.coda_type_get_array_dim(type_._x, x, y), 'coda_type_get_array_dim')
    return list(y)[:x[0]]


def type_get_read_type(type_):
    x = _ffi.new('coda_native_type *')
    _check(_lib.coda_type_get_read_type(type_._x, x), 'coda_type_get_read_type')
    return x[0]


def type_get_description(type_):
    c = _ffi.new('char **')
    _check(_lib.coda_type_get_description(type_._x, c), 'coda_type_get_description')
    return _string(c[0])


def type_get_num_record_fields(type_):
    x = _ffi.new('long *')
    _check(_lib.coda_type_get_num_record_fields(type_._x, x), 'coda_type_get_num_record_fields')
    return x[0]


def type_get_record_field_available_status(type_, index):
    x = _ffi.new('int *')
    _check(_lib.coda_type_get_record_field_available_status(type_._x, index, x),
           'coda_type_get_record_field_available_status')
    return x[0]


def type_get_record_union_status(type_):
    x = _ffi.new('int *')
    _check(_lib.coda_type_get_record_union_status(type_._x, x), 'coda_type_get_record_union_status')
    return x[0]


def type_get_record_field_hidden_status(type_, index):
    x = _ffi.new('int *')
    _check(_lib.coda_type_get_record_field_hidden_status(type_._x, index, x),
           'coda_type_get_record_field_hidden_status')
    return x[0]


def type_get_record_field_name(type_, index):
    x = _ffi.new('char **')
    _check(_lib.coda_type_get_record_field_name(type_._x, index, x), 'coda_type_get_record_field_name')
    return _string(x[0])


def type_get_record_field_real_name(type_, index):
    x = _ffi.new('char **')
    _check(_lib.coda_type_get_record_field_real_name(type_._x, index, x), 'coda_type_get_record_field_real_name')
    return _string(x[0])


def type_get_record_field_type(type_, index):
    x = _ffi.new('coda_type **')
    _check(_lib.coda_type_get_record_field_type(type_._x, index, x), 'coda_type_get_record_field_type')
    return _type(x[0])


def type_get_record_field_index_from_name(type_, name):
    x = _ffi.new('long *')
    _check(_lib.coda_type_get_record_field_index_from_name(type_._x, _encode_string(name), x),
           'coda_type_get_record_field_index_from_name')
    return x[0]


def type_get_record_field_index_from_real_name(type_, name):
    x = _ffi.new('long *')
    _check(_lib.coda_type_get_record_field_index_from_real_name(type_._x, _encode_string(name), x),
           'coda_type_get_record_field_index_from_real_name')
    return x[0]


def type_get_bit_size(type_):
    x = _ffi.new('int64_t *')
    _check(_lib.coda_type_get_bit_size(type_._x, x), 'coda_type_get_bit_size')
    return long(x[0])


def type_get_string_length(type_):
    x = _ffi.new('long *')
    _check(_lib.coda_type_get_string_length(type_._x, x), 'coda_type_get_string_length')
    return x[0]


def type_get_class_name(cl):
    return _string(_lib.coda_type_get_class_name(cl))


def type_get_format_name(f):
    return _string(_lib.coda_type_get_format_name(f))


def type_get_native_type_name(f):
    return _string(_lib.coda_type_get_native_type_name(f))


def type_get_name(type_):
    x = _ffi.new('char **')
    _check(_lib.coda_type_get_name(type_._x, x), 'coda_type_get_name')
    return _string(x[0])


def type_get_unit(type_):
    x = _ffi.new('char **')
    _check(_lib.coda_type_get_unit(type_._x, x), 'coda_type_get_unit')
    return _string(x[0])


def type_get_fixed_value(type_):
    x = _ffi.new('char **')
    y = _ffi.new('long *')
    _check(_lib.coda_type_get_fixed_value(type_._x, x, y), 'coda_type_get_fixed_value')
    return _string(x[0])


def type_has_attributes(type_):
    x = _ffi.new('int *')
    _check(_lib.coda_type_has_attributes(type_._x, x), 'coda_type_has_attributes')
    return x[0]


def expression_from_string(s):
    x = _ffi.new('coda_expression **')
    if not isinstance(s, bytes):
        s = _encode_string_with_encoding(s, 'ascii')
    _check(_lib.coda_expression_from_string(s, x), 'coda_expression_from_string')
    return Expression(_x=x[0])


def expression_eval_bool(expr, cursor=None):
    x = _ffi.new('int *')
    if cursor is None:
        cur = _ffi.NULL
    else:
        cur = cursor._x
    _check(_lib.coda_expression_eval_bool(expr._x, cur, x), 'coda_expression_eval_bool')
    return x[0]


def expression_eval_integer(expr, cursor=None):
    x = _ffi.new('int64_t *')
    if cursor is None:
        cur = _ffi.NULL
    else:
        cur = cursor._x
    _check(_lib.coda_expression_eval_integer(expr._x, cur, x), 'coda_expression_eval_integer')
    return long(x[0])


def expression_eval_float(expr, cursor=None):
    x = _ffi.new('double *')
    if cursor is None:
        cur = _ffi.NULL
    else:
        cur = cursor._x
    _check(_lib.coda_expression_eval_float(expr._x, cur, x), 'coda_expression_eval_float')
    return x[0]


def expression_eval_string(expr, cursor=None):
    x = _ffi.new('char **')
    y = _ffi.new('long *')
    if cursor is None:
        cur = _ffi.NULL
    else:
        cur = cursor._x
    _check(_lib.coda_expression_eval_string(expr._x, cur, x, y), 'coda_expression_eval_string')
    return _ffi.string(x[0])


def expression_eval_node(expr, cursor):
    _check(_lib.coda_expression_eval_node(expr._x, cursor._x), 'coda_expression_eval_node')


def expression_get_type(expr):
    x = _ffi.new('enum coda_expression_type_enum *')
    _check(_lib.coda_expression_get_type(expr._x, x), 'coda_expression_get_type')
    return x[0]


def expression_get_type_name(type_):
    return _string(_lib.coda_expression_get_type_name(type_))


def expression_is_constant(expr):
    return _lib.coda_expression_is_constant(expr._x)


def expression_is_equal(expr1, expr2):
    return _lib.coda_expression_is_equal(expr1._x, expr2._x)


def expression_delete(expr):
    _lib.coda_expression_delete(expr._x)


def _to_parts(dt, from_, fmt=None):
    y = _ffi.new('int *')
    mo = _ffi.new('int *')
    d = _ffi.new('int *')
    h = _ffi.new('int *')
    mi = _ffi.new('int *')
    s = _ffi.new('int *')
    mus = _ffi.new('int *')

    if from_ == 'double':
        _check(_lib.coda_time_double_to_parts(dt, y, mo, d, h, mi, s, mus), 'coda_time_double_to_parts')
    elif from_ == 'double_utc':
        _check(_lib.coda_time_double_to_parts_utc(dt, y, mo, d, h, mi, s, mus), 'coda_time_double_to_parts_utc')
    elif from_ == 'string':
        _check(_lib.coda_time_string_to_parts(fmt, dt, y, mo, d, h, mi, s, mus), 'coda_time_string_to_parts')

    return [y[0], mo[0], d[0], h[0], mi[0], s[0], mus[0]]


def time_double_to_parts(d):
    return _to_parts(d, 'double')


def time_double_to_parts_utc(d):
    return _to_parts(d, 'double_utc')


def time_double_to_string(d, fmt):
    s = _ffi.new('char [%d]' % (len(fmt)+1))
    _check(_lib.coda_time_double_to_string(d, _encode_string(fmt), s), 'coda_time_double_to_string')
    return _string(s)


def time_double_to_string_utc(d, fmt):
    s = _ffi.new('char [%d]' % (len(fmt)+1))
    fmt = _encode_string(fmt)
    _check(_lib.coda_time_double_to_string_utc(d, fmt, s), 'coda_time_double_to_string_utc')
    return _string(s)


def time_parts_to_double(y, mo, d, h, mi, s, mus):
    dt = _ffi.new('double *')
    _check(_lib.coda_time_parts_to_double(y, mo, d, h, mi, s, mus, dt), 'coda_time_parts_to_double')
    return dt[0]


def time_parts_to_double_utc(y, mo, d, h, mi, s, mus):
    dt = _ffi.new('double *')
    _check(_lib.coda_time_parts_to_double_utc(y, mo, d, h, mi, s, mus, dt), 'coda_time_parts_to_double_utc')
    return dt[0]


def time_parts_to_string(y, mo, d, h, mi, s, mus, fmt):
    dt = _ffi.new('char [%d]' % (len(fmt)+1))
    fmt = _encode_string(fmt)
    _check(_lib.coda_time_parts_to_string(y, mo, d, h, mi, s, mus, fmt, dt), 'coda_time_parts_to_string')
    return _string(dt)


def time_string_to_double(fmt, s):
    d = _ffi.new('double *')
    fmt = _encode_string(fmt)
    s = _encode_string(s)
    _check(_lib.coda_time_string_to_double(fmt, s, d), 'coda_time_string_to_double')
    return d[0]


def time_string_to_double_utc(fmt, s):
    d = _ffi.new('double *')
    fmt = _encode_string(fmt)
    s = _encode_string(s)
    _check(_lib.coda_time_string_to_double_utc(fmt, s, d), 'coda_time_string_to_double_utc')
    return d[0]


def time_string_to_parts(fmt, s):
    return _to_parts(_encode_string(s), 'string', _encode_string(fmt))


def set_definition_path_conditional(p1, p2, p3):
    def conv(p):
        if p is None:
            return _ffi.NULL
        else:
            return _encode_path(p)
    _check(_lib.coda_set_definition_path_conditional(conv(p1), conv(p2), conv(p3)),
           'coda_set_definition_path_conditional')


coda_set_definition_path_conditional = set_definition_path_conditional  # compat


def set_option_bypass_special_types(enable):
    _check(_lib.coda_set_option_bypass_special_types(enable), 'coda_set_option_bypass_special_types')


def get_option_bypass_special_types():
    return _lib.coda_get_option_bypass_special_types()


def set_option_perform_boundary_checks(enable):
    _check(_lib.coda_set_option_perform_boundary_checks(enable), 'coda_set_option_perform_boundary_checks')


def get_option_perform_boundary_checks():
    return _lib.coda_get_option_perform_boundary_checks()


def set_option_perform_conversions(enable):
    _check(_lib.coda_set_option_perform_conversions(enable), 'coda_set_option_perform_conversions')


def get_option_perform_conversions():
    return _lib.coda_get_option_perform_conversions()


def set_option_use_fast_size_expressions(enable):
    _check(_lib.coda_set_option_use_fast_size_expressions(enable), 'coda_set_option_use_fast_size_expressions')


def get_option_use_fast_size_expressions():
    return _lib.coda_get_option_use_fast_size_expressions()


def set_option_use_mmap(enable):
    _check(_lib.coda_set_option_use_mmap(enable), 'coda_set_option_use_mmap')


def get_option_use_mmap():
    return _lib.coda_get_option_use_mmap()


def init():
    _check(_lib.coda_init(), 'coda_init')


def done():
    _lib.coda_done()


def c_index_to_fortran_index(shape, index):
    num_dims = len(shape)
    d = _ffi.new('long [%d]' % num_dims)
    for i in range(len(shape)):
        d[i] = shape[i]
    return _lib.coda_c_index_to_fortran_index(num_dims, d, index)


def NaN():
    return _lib.coda_NaN()


def isNaN(x):
    return _lib.coda_isNaN(x)


def isInf(x):
    return _lib.coda_isInf(x)


def MinInf():
    return _lib.coda_MinInf()


def isMinInf(x):
    return _lib.coda_isMinInf(x)


def PlusInf():
    return _lib.coda_PlusInf()


def isPlusInf(x):
    return _lib.coda_isPlusInf(x)


def get_encoding():
    """Return the encoding used to convert between unicode strings and C strings
    (only relevant when using Python 3).

    """
    return _encoding


def set_encoding(encoding):
    """Set the encoding used to convert between unicode strings and C strings
    (only relevant when using Python 3).

    """
    global _encoding

    _encoding = encoding


def version():
    """Return the version of the CODA C library."""
    return _string(_lib.coda_get_libcoda_version())


def _get_filesystem_encoding():
    """Return the encoding used by the filesystem."""
    from sys import getdefaultencoding as _getdefaultencoding, getfilesystemencoding as _getfilesystemencoding

    encoding = _getfilesystemencoding()
    if encoding is None:
        encoding = _getdefaultencoding()

    return encoding


def _encode_string_with_encoding(string, encoding="utf-8"):
    """Encode a unicode string using the specified encoding.

    By default, use the "surrogateescape" error handler to deal with encoding
    errors. This error handler ensures that invalid bytes encountered during
    decoding are converted to the same bytes during encoding, by decoding them
    to a special range of unicode code points.

    The "surrogateescape" error handler is available since Python 3.1. For earlier
    versions of Python 3, the "strict" error handler is used instead.

    """
    try:
        try:
            return string.encode(encoding, "surrogateescape")
        except LookupError:
            # Either the encoding or the error handler is not supported; fall-through to the next try-block.
            pass

        try:
            return string.encode(encoding)
        except LookupError:
            # Here it is certain that the encoding is not supported.
            raise Error("unknown encoding '%s'" % encoding)
    except UnicodeEncodeError:
        raise Error("cannot encode '%s' using encoding '%s'" % (string, encoding))


def _decode_string_with_encoding(string, encoding="utf-8"):
    """Decode a byte string using the specified encoding.

    By default, use the "surrogateescape" error handler to deal with encoding
    errors. This error handler ensures that invalid bytes encountered during
    decoding are converted to the same bytes during encoding, by decoding them
    to a special range of unicode code points.

    The "surrogateescape" error handler is available since Python 3.1. For earlier
    versions of Python 3, the "strict" error handler is used instead. This may cause
    decoding errors if the input byte string contains bytes that cannot be decoded
    using the specified encoding. Since most HARP products use ASCII strings
    exclusively, it is unlikely this will occur often in practice.

    """
    try:
        try:
            return string.decode(encoding, "surrogateescape")
        except LookupError:
            # Either the encoding or the error handler is not supported; fall-through to the next try-block.
            pass

        try:
            return string.decode(encoding)
        except LookupError:
            # Here it is certain that the encoding is not supported.
            raise Error("unknown encoding '%s'" % encoding)
    except UnicodeEncodeError:
        raise Error("cannot decode '%s' using encoding '%s'" % (string, encoding))


def _encode_path(path):
    """Encode the input unicode path using the filesystem encoding.

    On Python 2, this method returns the specified path unmodified.

    """
    if isinstance(path, bytes):
        # This branch will be taken for instances of class str on Python 2 (since this is an alias for class bytes),
        # and on Python 3 for instances of class bytes.
        return path
    elif isinstance(path, str):
        # This branch will only be taken for instances of class str on Python 3. On Python 2 such instances will take
        # the branch above.
        return _encode_string_with_encoding(path, _get_filesystem_encoding())
    else:
        raise TypeError("path must be bytes or str, not %r" % path.__class__.__name__)


def _encode_string(string):
    """Encode the input unicode string using the package default encoding.

    On Python 2, this method returns the specified string unmodified.

    """
    if isinstance(string, bytes):
        # This branch will be taken for instances of class str on Python 2 (since this is an alias for class bytes),
        # and on Python 3 for instances of class bytes.
        return string
    elif isinstance(string, str):
        # This branch will only be taken for instances of class str on Python 3. On Python 2 such instances will take
        # the branch above.
        return _encode_string_with_encoding(string, get_encoding())
    else:
        raise TypeError("string must be bytes or str, not %r" % string.__class__.__name__)


def _decode_string(string):
    """Decode the input byte string using the package default encoding.

    On Python 2, this method returns the specified byte string unmodified.

    """
    if isinstance(string, str):
        # This branch will be taken for instances of class str on Python 2 and Python 3.
        return string
    elif isinstance(string, bytes):
        # This branch will only be taken for instances of class bytes on Python 3. On Python 2 such instances will take
        # the branch above.
        return _decode_string_with_encoding(string, get_encoding())
    else:
        raise TypeError("string must be bytes or str, not %r" % string.__class__.__name__)


def _get_c_library_filename():
    """Return the filename of the CODA shared library depending on the current
    platform.

    """
    if platform.system() == "Windows":
        return "coda.dll"

    if platform.system() == "Darwin":
        library_name = "libcoda.dylib"
    else:
        library_name = "libcoda.so"

    # expand symlinks (for conda-forge, pypy build)
    dirname = os.path.dirname(os.path.realpath(__file__))

    # look in different directories based on platform
    for rel_path in (
        "..",  # pyinstaller bundles
        "../../..",  # regular lib dir
        "../../../../lib",  # on RHEL the python path uses lib64, but the library might have gotten installed in lib
    ):
        library_path = os.path.normpath(os.path.join(dirname, rel_path, library_name))
        if os.path.exists(library_path):
            return library_path


#
# UTILITY FUNCTIONS
#
def _isIterable(maybeIterable):
    """Is the argument an iterable object? Taken from the Python Cookbook, recipe 1.12"""
    try:
        iter(maybeIterable)
    except:
        return False
    else:
        return True


#
# PATH TRAVERSAL
#
def _traverse_path(cursor, path, start=0):
    """
    _traverse_path() traverses the specified path until
    an array with variable indices is encountered or the
    end of the path is reached. It checks field availability
    for records and index ranges for arrays. An exception is
    thrown when a check fails.
    """

    for pathIndex in range(start, len(path)):
        if isinstance(path[pathIndex], str):
            cursor_goto(cursor, path[pathIndex])
        else:
            if isinstance(path[pathIndex], int):
                arrayIndex = [path[pathIndex]]
            elif isinstance(path[pathIndex], (list, tuple)):
                arrayIndex = path[pathIndex]
            else:
                raise ValueError("path specification (%s) should be a string or (list of) integers" %
                                 (path[pathIndex],))

            # get the shape of the array from the cursor. the size of all
            # dynamic dimensions are computed by the coda library.
            arrayShape = cursor_get_array_dim(cursor)

            # handle a rank-0 array by (virtually) converting it to
            # a 1-dimensional array of size 1.
            rankZeroArray = False
            if len(arrayShape) == 0:
                rankZeroArray = True
                arrayShape.append(1)

            # check if the number of indices specified match the
            # dimensionality of the array.
            if len(arrayIndex) != len(arrayShape):
                raise ValueError("number of specified indices does not match the dimensionality of the array")

            # check for variable indices and perform range checks on all
            # non-variable indices.
            intermediateArray = False
            for i in range(0, len(arrayIndex)):
                if arrayIndex[i] == -1:
                    intermediateArray = True
                elif (arrayIndex[i] < 0) or (arrayIndex[i] >= arrayShape[i]):
                    raise ValueError("array index (%i) exceeds array range [0:%i)" % (arrayIndex[i], arrayShape[i]))

            if intermediateArray:
                return (True, pathIndex)
            else:
                # if all indices are non-variable, just move the cursor
                # to the indicated element.
                if rankZeroArray:
                    cursor_goto_array_element(cursor, [])
                else:
                    cursor_goto_array_element(cursor, arrayIndex)

    # we've arrived at the end of the path.
    return (False, len(path) - 1)


#
# HELPER FUNCTIONS FOR CODA.FETCH()
#
def _fetch_intermediate_array(cursor, path, pathIndex=0):
    """
    _fetch_intermediate_array calls _traverse_path() to traverse the path
    until the end is reached or an intermediate array is encountered.
    if the end of the path is reached, then we need to fetch everything
    from that point on (i.e. the whole subtree). in this case _fetch_subtree()
    is called. otherwise _fetch_intermediate_array() is called which
    recursively fetches each element of the array. note that this will result
    in calls to _fetch_data().
    """

    arrayShape = cursor_get_array_dim(cursor)

    # handle a rank-0 array by converting it to
    # a 1-dimensional array of size 1.
    if len(arrayShape) == 0:
        arrayShape.append(1)

    fetchShape = []
    fetchStep = []
    nextElementIndex = 0
    elementCount = 1

    if isinstance(path[pathIndex], int):
        # if the current path element is of type int, then
        # the intermediate array must be of rank 1. hence
        # the int in question must equal -1.
        assert path[pathIndex] == -1, \
            "A rank-1 intermediate array should always be indexed by -1 (got %i)." % (path[pathIndex],)

        fetchShape.append(arrayShape[0])
        fetchStep.append(1)
        fetchStep.append(arrayShape[0])
        elementCount = arrayShape[0]
    else:
        step = 1
        arrayIndex = path[pathIndex]

        for i in reversed(list(range(0, len(arrayIndex)))):
            if arrayIndex[i] == -1:
                fetchShape.append(arrayShape[i])
                fetchStep.append(step)
                elementCount *= arrayShape[i]
            else:
                nextElementIndex += step * arrayIndex[i]

            step *= arrayShape[i]

        fetchStep.append(step)

    # check for empty array (i.e. at least one dimension equals zero).
    if elementCount == 0:
        return None

    # create an index.
    fetchIndex = [0] * len(fetchShape)

    # flag array as uninitialized. the result array is created after traversing
    # the path to the first array element. note that we are fetching an intermediate array,
    # which implies that the end of the path has not been reached yet. therefore, the
    # 'basetype' of the intermediate array can only be determined after traversing the path
    # to a (i.e. the first) array element.
    array = None

    # currentElementIndex represents an index into the flattened array from which elements
    # will be _read_. however, iteration is performed over the flattened array into which
    # elements will be _stored_. at the beginning of each iteration, (nextElementIndex -
    # currentElementIndex) elements are skipped using cursor_goto_next_array_element().
    currentElementIndex = 0

    cursor_goto_first_array_element(cursor)
    for i in range(0, elementCount):
        # move the cursor to the next required array element.
        while currentElementIndex < nextElementIndex:
            cursor_goto_next_array_element(cursor)
            currentElementIndex += 1

        depth = cursor_get_depth(cursor)

        # traverse the path.
        (intermediateNode, copiedPathIndex) = _traverse_path(cursor, path, pathIndex + 1)

        # create the result array by examining the type of the first element.
        # This is equivalent to i == 0
        if array is None:
            assert i == 0
            # everything is an object until proven a scalar. :-)
            scalar = False

            # check for scalar types.
            nodeType = cursor_get_type(cursor)
            nodeClass = type_get_class(nodeType)

            if ((nodeClass == coda_array_class) or (nodeClass == coda_record_class)):
                # records and arrays are non-scalar.
                scalar = False

            elif ((nodeClass == coda_integer_class) or (nodeClass == coda_real_class) or
                  (nodeClass == coda_text_class) or (nodeClass == coda_raw_class)):
                nodeReadType = type_get_read_type(nodeType)

                if nodeReadType == coda_native_type_not_available:
                    raise FetchError("cannot read array (not all elements are available)")
                else:
                    (scalar, numpyType) = _numpyNativeTypeDictionary[nodeReadType]

            elif nodeClass == coda_special_class:
                nodeSpecialType = type_get_special_type(nodeType)
                (scalar, numpyType) = _numpySpecialTypeDictionary[nodeSpecialType]

            # for convenience, fetchShape is constructed in reverse order. however,
            # numpy's array creation functions expect a shape argument in regular
            # order.
            tmpShape = copy.copy(fetchShape)
            tmpShape.reverse()

            # instantiate the required array class.
            if scalar:
                array = numpy.empty(dtype=numpyType, shape=tmpShape)
            else:
                array = numpy.empty(dtype=object, shape=tmpShape)

        # when this point is reached, a result array has been allocated
        # and the flatArrayIter is set.
        # The required element will now be read, the iterator incremented and the
        # result stored.
        if intermediateNode:
            # an intermediate array was encountered.
            array.flat[i] = _fetch_intermediate_array(cursor, path, copiedPathIndex)
        else:
            # the end of the path was reached. from this point on,
            # the entire subtree is fetched.
            array.flat[i] = _fetch_subtree(cursor)

        # update fetchIndex and nextElementIndex.
        for j in range(0, len(fetchShape)):
            fetchIndex[j] += 1
            nextElementIndex += fetchStep[j]

            if fetchIndex[j] < fetchShape[j]:
                break

            fetchIndex[j] = 0
            nextElementIndex -= fetchStep[j + 1]

        for j in range(cursor_get_depth(cursor) - depth):
            cursor_goto_parent(cursor)

    cursor_goto_parent(cursor)
    return array


def _fetch_object_array(cursor, type_tree=None):
    """
    _fetch_object_array() fetches arrays with a basetype that is not considered
    scalar.
    """

    arrayShape = cursor_get_array_dim(cursor)

    # handle a rank-0 array by converting it to
    # a 1-dimensional array of size 1.
    if len(arrayShape) == 0:
        arrayShape.append(1)

    # now create the (empty) array of the correct type and shape
    array = numpy.empty(dtype=object, shape=arrayShape)

    # goto the first element
    cursor_goto_first_array_element(cursor)

    # loop over all elements excluding the last one
    flat = array.flat
    arraySizeMinOne = array.size - 1
    for i in range(arraySizeMinOne):
        flat[i] = _fetch_subtree(cursor, type_tree)
        cursor_goto_next_array_element(cursor)

    # final element then back tp parent scope
    flat[arraySizeMinOne] = _fetch_subtree(cursor, type_tree)
    cursor_goto_parent(cursor)

    return array


CLASS_RECORD = 0
CLASS_ARRAY = 1
CLASS_SCALAR = 2
CLASS_SPECIAL = 3

ARRAY_OBJECT = 0
ARRAY_SCALAR = 1
ARRAY_SPECIAL = 2


def _fetch_subtree(cursor, type_tree=None):
    """
    _fetch_subtree() recursively fetches all data starting from a specified
    position. this function is commonly called when path traversal reaches the
    end of the path. from that point on _all_ data has to be fetched, i.e. no
    array slicing or fetching of single specified fields has to be performed.
    note: unavailable fields are skipped (i.e. not added to the Record instance),
    while hidden fields are only skipped if the filtering option is set to True.
    """

    if type_tree is None:
        type_tree = _determine_type_tree(cursor)

    class_ = type_tree[0]

    if class_ == CLASS_SCALAR:
        return type_tree[1](cursor)

    elif class_ == CLASS_RECORD:
        fields = type_tree[1]
        fieldCount = len(fields)
        registered = type_tree[2]
        record_class = type_tree[3]

        # check for empty record.
        if fieldCount == 0:
            return record_class()

        # read data.
        values = []
        cursor_goto_first_record_field(cursor)
        for i, field in enumerate(fields):
            if field is not None:
                name, type_ = field
                if type_[0] == CLASS_SCALAR:  # inline scalar case for performance
                    data = type_[1](cursor)
                else:
                    data = _fetch_subtree(cursor, type_)
                values.append(data)

            # avoid calling cursor_goto_next_record_field() after reading
            # the final field. otherwise, the cursor would get positioned
            # outside the record.
            if i < fieldCount - 1:
                cursor_goto_next_record_field(cursor)

        cursor_goto_parent(cursor)

        record = record_class(registered, values)
        return record

    elif class_ == CLASS_ARRAY:
        # check for empty array.
        if len(type_tree) == 1:
            return None

        _, baseclass, extratype, subtype = type_tree

        if baseclass == ARRAY_OBJECT:
            # neither an array of arrays nor an array of records can be read directly.
            # therefore, the elements of the array are read one at a time and stored
            # in a numpy array.
            return _fetch_object_array(cursor, subtype)

        elif baseclass == ARRAY_SCALAR:
            return _readNativeTypeArrayFunctionDictionary[extratype](cursor)

        elif baseclass == ARRAY_SPECIAL:
            if extratype == coda_special_no_data:
                # this is a very weird special case that will probably never occur.
                # for consistency, an array with base type coda_special_no_data will
                # be returned as an array of the specified size filled with None.
                arrayShape = cursor_get_array_dim(cursor)

                # handle a rank-0 array by converting it to
                # a 1-dimensional array of size 1.
                if len(arrayShape) == 0:
                    arrayShape.append(1)

                return numpy.empty(None, shape=arrayShape)
            else:
                return _readSpecialTypeArrayFunctionDictionary[extratype](cursor)

    elif class_ == CLASS_SPECIAL:
        return type_tree[1](cursor)


def _determine_type_tree(cursor):
    # type trees consist of nested lists for performance (positional so ugly but fast)

    nodeType = cursor_get_type(cursor)
    nodeClass = type_get_class(nodeType)

    if ((nodeClass == coda_integer_class) or (nodeClass == coda_real_class) or
            (nodeClass == coda_text_class) or (nodeClass == coda_raw_class)):
        nodeReadType = type_get_read_type(nodeType)
        reader = _readNativeTypeScalarFunctionDictionary[nodeReadType]
        tree = [CLASS_SCALAR, reader]

    elif nodeClass == coda_record_class:
        fields = []
        registered = []

        fieldCount = cursor_get_num_elements(cursor)
        if fieldCount != 0:
            # determine field visibility
            skipField = [False] * fieldCount
            for i in range(0, fieldCount):
                if cursor_get_record_field_available_status(cursor, i) != 1:
                    # skip field if unavailable.
                    skipField[i] = True
                    continue

                if _filterRecordFields:
                    skipField[i] = bool(type_get_record_field_hidden_status(nodeType, i))

            # field names (None means invisible)
            cursor_goto_first_record_field(cursor)
            for i in range(0, fieldCount):
                if not skipField[i]:
                    fieldName = type_get_record_field_name(nodeType, i)
                    subtype = _determine_type_tree(cursor)
                    fields.append([fieldName, subtype])
                    registered.append(fieldName)
                else:
                    fields.append(None)

                # avoid calling cursor_goto_next_record_field() after reading
                # the final field. otherwise, the cursor would get positioned
                # outside the record.
                if i < fieldCount - 1:
                    cursor_goto_next_record_field(cursor)

            cursor_goto_parent(cursor)

        class RecordType(Record):
            __slots__ = ['_fields', '_values']

            # field name to index mapping

            _field_to_index = {}
            for i, field in enumerate(registered):
                _field_to_index[field] = i

            # dictionary to convert from numpy types to
            # a string representation of the corresponding CODA type.

            _typeToString = {
                numpy.int8: "int8",
                numpy.uint8: "uint8",
                numpy.int16: "int16",
                numpy.uint16: "uint16",
                numpy.int32: "int32",
                numpy.uint32: "uint32",
                numpy.int64: "int64",
                numpy.float32: "float",
                numpy.float64: "double",
                numpy.complex128: "complex",
                numpy.object_: "object"}

            def __init__(self, fields=[], values=[]):
                super(RecordType, self).__setattr__('_fields', fields)
                super(RecordType, self).__setattr__('_values', values)

            def __len__(self):
                """
                Return the number of fields in this record.
                """
                return len(self._fields)

            def __getitem__(self, key):
                if not isinstance(key, int):
                    raise TypeError("index should be an integer")

                if key < 0:
                    key += len(self._fields)

                if key < 0 or key >= len(self._fields):
                    raise IndexError

                return self._values[key]

            @property
            def __dict__(self):
                return dict(zip(self._fields, self._values))

            def __getattr__(self, field):
                try:
                    return self._values[self._field_to_index[field]]
                except KeyError:
                    raise AttributeError("%r object has no attribute %r" %
                                         (self.__class__.__name__, field))

            def __setattr__(self, field, value):
                self._values[self._field_to_index[field]] = value

            def __repr__(self):
                """
                Return the canonical string representation of the instance.

                This is always the identifying string '<coda record>'.
                """
                return "<coda record>"

            def __str__(self):
                """
                Print type/structure information for this record.

                The output format is identical to how MATLAB shows structure information, except
                that for now a fixed padding value of 32 is used, and that the precision parameters
                for some of the floats will differ.
                """
                out = io.StringIO()

                for field, data in zip(self._fields, self._values):
                    out.write(u"%32s:" % (field))

                    if isinstance(data, Record):
                        out.write(u"record (%i fields)\n" % (len(data),))

                    elif isinstance(data, numpy.ndarray):
                        dim = data.shape
                        dimString = ""
                        for d in dim[:-1]:
                            dimString += "%ix" % (d,)
                        dimString += "%i" % (dim[-1],)
                        out.write(u"[%s %s]\n" % (dimString, self._typeToString[data.dtype.type]))

                    elif isinstance(data, str):
                        out.write(u"\"%s\"\n" % (data,))

                    else:
                        # if type is none of the above, fall back
                        # on the type specific __str__() function.
                        out.write(u"%s\n" % (data,))

                return out.getvalue()

        tree = [CLASS_RECORD, fields, registered, RecordType]

    elif (nodeClass == coda_array_class):
        # check for empty array.
        if cursor_get_num_elements(cursor) == 0:
            return [CLASS_ARRAY]

        # get base type information.
        arrayBaseType = type_get_array_base_type(nodeType)
        arrayBaseClass = type_get_class(arrayBaseType)

        if ((arrayBaseClass == coda_array_class) or (arrayBaseClass == coda_record_class)):
            baseclass = ARRAY_OBJECT
            extratype = None

        elif ((arrayBaseClass == coda_integer_class) or (arrayBaseClass == coda_real_class) or
              (arrayBaseClass == coda_text_class) or (arrayBaseClass == coda_raw_class)):
            baseclass = ARRAY_SCALAR
            extratype = type_get_read_type(arrayBaseType)

        elif arrayBaseClass == coda_special_class:
            baseclass = ARRAY_SPECIAL
            extratype = type_get_special_type(arrayBaseType)

        else:
            raise FetchError("array of unknown base type")

        cursor_goto_first_array_element(cursor)
        subtype = _determine_type_tree(cursor)
        cursor_goto_parent(cursor)

        tree = [CLASS_ARRAY, baseclass, extratype, subtype]

    elif nodeClass == coda_special_class:
        nodeSpecialType = cursor_get_special_type(cursor)
        reader = _readSpecialTypeScalarFunctionDictionary[nodeSpecialType]
        tree = [CLASS_SPECIAL, reader]

    else:
        raise FetchError("element of unknown type")

    return tree


#
# CODA LAYER I HIGH LEVEL API
#
def _get_cursor(start):
    """
    _get_cursor() takes a valid CODA product file handle _or_ a valid CODA
    cursor as input and returns a new cursor object.
    """

    if not isinstance(start, Cursor):
        # create a cursor
        cursor = Cursor()
        cursor_set_product(cursor, start)
        return cursor
    else:
        # copy the cursor passed in by the user
        return copy.deepcopy(start)


def get_attributes(start, *path):
    """
    Retrieve the attributes of the specified data item.

    This function returns a Record containing the attributes of the
    specified data item.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    cursor_goto_attributes(cursor)

    result = _fetch_subtree(cursor)

    del cursor
    return result


def get_description(start, *path):
    """
    Retrieve the description of a field.

    This function returns a string containing the description in the
    product format definition of the specified data item.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    nodeDescription = type_get_description(nodeType)

    del cursor

    if nodeDescription is None:
        return ""
    else:
        return nodeDescription


def fetch(start, *path):
    """
    Retrieve data from a product file.

    Reads the specified data item from the product file. Instead
    of just reading individual values, like strings, integers, doubles,
    etc. it is also possible to read complete arrays or records of data.
    For instance if 'pf' is a product file handle obtained by calling
    coda.open(), then you can read the complete MPH of this product
    with:

    mph = coda.fetch(pf,'mph')

    which gives you a Record containing all the mph fields.

    It is also possible to read an entire product at once by leaving the
    data specification argument list empty (product = coda.fetch(pf)).

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open(), a valid CODA cursor _or_ a product file
    path. If the start argument is a cursor, then the specified path is
    traversed starting from the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    product = None
    if _is_str(start):
        product = start = Product(start)
    cursor = _get_cursor(start)

    # traverse the path
    (intermediateNode, pathIndex) = _traverse_path(cursor, path)

    try:
        if (intermediateNode):
            result = _fetch_intermediate_array(cursor, path, pathIndex)
        else:
            result = _fetch_subtree(cursor)
    finally:
        if product is not None:
            product.close()

    # clean up cursor
    del cursor
    return result


def get_field_available(start, *path):
    """
    Find out whether a dynamically available record field is available or not.

    This function returns True if the record field is available and False
    if it is not. The last item of the path argument should point to a
    record field. An empty path is considered an error, even if the start
    argument is a CODA cursor.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    if len(path) == 0 or not isinstance(path[-1], str):
        raise ValueError("path argument should not be empty and should end with name of a record field")

    cursor = _get_cursor(start)

    # traverse up until the last node of the path.
    (intermediateNode, pathIndex) = _traverse_path(cursor, path[:-1])
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    # get the field index.
    nodeType = cursor_get_type(cursor)
    fieldIndex = type_get_record_field_index_from_name(nodeType, path[-1])

    # get field availability.
    result = bool(cursor_get_record_field_available_status(cursor, fieldIndex))

    del cursor
    return result


def get_field_count(start, *path):
    """
    Retrieve the number of fields in a record.

    This function returns the number of fields in the Record instance
    that will be returned if coda.fetch() is called with the same
    arguments. The last node on the path should reference a record.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    fieldCount = type_get_num_record_fields(nodeType)
    instanceFieldCount = fieldCount
    for i in range(0, fieldCount):
        if cursor_get_record_field_available_status(cursor, i) != 1:
            instanceFieldCount -= 1
            continue

        if _filterRecordFields and bool(type_get_record_field_hidden_status(nodeType, i)):
            instanceFieldCount -= 1

    del cursor
    return instanceFieldCount


def get_field_names(start, *path):
    """
    Retrieve the names of the fields in a record.

    This function returns the names of the fields of the Record instance
    that will be returned if coda.fetch() is called with the same
    arguments. The last node on the path should reference a record.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    fieldCount = type_get_num_record_fields(nodeType)
    fieldNames = []
    for i in range(0, fieldCount):
        if cursor_get_record_field_available_status(cursor, i) != 1:
            continue

        if _filterRecordFields and bool(type_get_record_field_hidden_status(nodeType, i)):
            continue

        fieldNames.append(type_get_record_field_name(nodeType, i))

    del cursor
    return fieldNames


def get_size(start, *path):
    """
    Retrieve the dimensions of the specified array.

    This function returns the dimensions of the array that will be
    returned if coda.fetch() is called with the same arguments. Thus,
    you can check what the dimensions of an array are without having
    to retrieve the entire array with coda.fetch(). The last node on
    the path should reference an array.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    dims = cursor_get_array_dim(cursor)
    del cursor

    # accurately reflect how rank-0 arrays are handled.
    if dims == []:
        return [1]
    else:
        return dims


def time_to_string(times):
    """
    Convert a number of seconds since 2000-01-01 (TAI) to a human readable
    form.

    This function turns a double value specifying a number of seconds
    since 2000-01-01 into a string containing the date and time in a human
    readable form. For example:

    time_to_string(68260079.0)

    would return the string '2002-03-01 01:07:59.000000'.

    It is possible to input a list or tuple of doubles, in which case a
    list of strings will be returned.
    """

    if _isIterable(times):
        return [time_double_to_string(t, "yyyy-MM-dd HH:mm:ss.SSSSSS") for t in times]
    else:
        return time_double_to_string(times, "yyyy-MM-dd HH:mm:ss.SSSSSS")


def time_to_utcstring(times):
    """
    Convert a TAI number of seconds since 2000-01-01 (TAI) to a human readable
    form in UTC format.

    This function turns a double value specifying a number of TAI seconds
    since 2000-01-01 into a string containing the UTC date and time in a human
    readable form (using proper leap second correction in the conversion).
    For example:

    time_to_utcstring(68260111.0)

    would return the string '2002-03-01 01:07:59.000000'.

    It is possible to input a list or tuple of doubles, in which case a
    list of strings will be returned.
    """

    if _isIterable(times):
        return [time_double_to_string_utc(t, "yyyy-MM-dd HH:mm:ss.SSSSSS") for t in times]
    else:
        return time_double_to_string_utc(times, "yyyy-MM-dd HH:mm:ss.SSSSSS")


def get_unit(start, *path):
    """
    Retrieve unit information.

    This function returns a string containing the unit information
    which is stored in the product format definition for the specified data
    item.

    The start argument must be a valid CODA file handle that was
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    (intermediateNode, pathIndex) = _traverse_path(cursor, path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    del cursor

    return type_get_unit(nodeType)


# _filterRecordFields: if set to True, hidden record fields are ignored.
_filterRecordFields = True


def set_option_filter_record_fields(enable):
    global _filterRecordFields

    _filterRecordFields = bool(enable)


def get_option_filter_record_fields():
    return _filterRecordFields


def _init():
    """Initialize the CODA Python interface."""
    global _lib, _encoding
    # Initialize the CODA C library
    clib = _get_c_library_filename()
    _lib = _ffi.dlopen(clib)

    # Import constants
    for attrname in dir(_lib):
        attr = getattr(_lib, attrname)
        if isinstance(attr, int):
            globals()[attrname] = attr

    if os.getenv('CODA_DEFINITION') is None:
        # Set coda definition path relative to C library
        basename = os.path.basename(clib)
        if platform.system() == "Windows":
            dirname = None
        else:
            dirname = os.path.dirname(clib)
        relpath = "../share/coda/definitions"
        coda_set_definition_path_conditional(basename, dirname, relpath)

    # Set default encoding.
    _encoding = "ascii"

    init()


#
# Initialize the CODA Python interface.
#
_init()

#
# MODULE (PRIVATE) ATTRIBUTES
#

# dictionary (a.k.a. switch construct ;) for native type scalar read functions.
# scalars with type coda_native_type_bytes require extra code to find out their size, so this
# type is omitted here.
_readNativeTypeScalarFunctionDictionary = {
    coda_native_type_int8: cursor_read_int8,
    coda_native_type_uint8: cursor_read_uint8,
    coda_native_type_int16: cursor_read_int16,
    coda_native_type_uint16: cursor_read_uint16,
    coda_native_type_int32: cursor_read_int32,
    coda_native_type_uint32: cursor_read_uint32,
    coda_native_type_int64: cursor_read_int64,
    coda_native_type_uint64: cursor_read_uint64,
    coda_native_type_float: cursor_read_float,
    coda_native_type_double: cursor_read_double,
    coda_native_type_char: cursor_read_char,
    coda_native_type_string: cursor_read_string,
    coda_native_type_bytes: cursor_read_bytes
}

# dictionary (a.k.a. switch construct ;) for native type array read functions.
_readNativeTypeArrayFunctionDictionary = {
    coda_native_type_int8: cursor_read_int8_array,
    coda_native_type_uint8: cursor_read_uint8_array,
    coda_native_type_int16: cursor_read_int16_array,
    coda_native_type_uint16: cursor_read_uint16_array,
    coda_native_type_int32: cursor_read_int32_array,
    coda_native_type_uint32: cursor_read_uint32_array,
    coda_native_type_int64: cursor_read_int64_array,
    coda_native_type_uint64: cursor_read_uint64_array,
    coda_native_type_float: cursor_read_float_array,
    coda_native_type_double: cursor_read_double_array,
    coda_native_type_char: _fetch_object_array,
    coda_native_type_string: _fetch_object_array,
    coda_native_type_bytes: _fetch_object_array
}

# dictionary (a.k.a. switch construct ;) for native type partial array read functions.
_readNativeTypePartialArrayFunctionDictionary = {
    coda_native_type_int8: cursor_read_int8_partial_array,
    coda_native_type_uint8: cursor_read_uint8_partial_array,
    coda_native_type_int16: cursor_read_int16_partial_array,
    coda_native_type_uint16: cursor_read_uint16_partial_array,
    coda_native_type_int32: cursor_read_int32_partial_array,
    coda_native_type_uint32: cursor_read_uint32_partial_array,
    coda_native_type_int64: cursor_read_int64_partial_array,
    coda_native_type_uint64: cursor_read_uint64_partial_array,
    coda_native_type_float: cursor_read_float_partial_array,
    coda_native_type_double: cursor_read_double_partial_array,
}

# dictionary (a.k.a. switch construct ;) for special type scalar read functions.
_readSpecialTypeScalarFunctionDictionary = {
    coda_special_no_data: lambda x: None,
    coda_special_vsf_integer: cursor_read_double,
    coda_special_time: cursor_read_double,
    coda_special_complex: cursor_read_complex
}

# dictionary (a.k.a. switch construct ;) for special type array read functions.
# scalars with type coda_special_no_data is a special case that requires extra code, and
# is therefore omitted here.
_readSpecialTypeArrayFunctionDictionary = {
    coda_special_vsf_integer: cursor_read_double_array,
    coda_special_time: cursor_read_double_array,
    coda_special_complex: cursor_read_complex_array
}

# dictionary used as a 'typemap'. a tuple is returned, of which the first element is a
# boolean that indicates if the type is considered to be scalar. if so, the second
# element gives the numpy type that matches the specified CODA type. otherwise the second
# element is None.
_numpyNativeTypeDictionary = {
    coda_native_type_int8: (True, numpy.int8),
    coda_native_type_uint8: (True, numpy.uint8),
    coda_native_type_int16: (True, numpy.int16),
    coda_native_type_uint16: (True, numpy.uint16),
    coda_native_type_int32: (True, numpy.int32),
    coda_native_type_uint32: (True, numpy.uint32),
    coda_native_type_int64: (True, numpy.int64),
    coda_native_type_uint64: (True, numpy.uint64),
    coda_native_type_float: (True, numpy.float32),
    coda_native_type_double: (True, numpy.float64),
    coda_native_type_char: (False, None),
    coda_native_type_string: (False, None),
    coda_native_type_bytes: (False, None)
}


# dictionary used as a 'typemap'. a tuple is returned, of which the first element is a
# boolean that indicates if the type is considered to be scalar. if so, the second
# element gives the numpy type that matches the specified CODA type. otherwise the second
# element is None.
_numpySpecialTypeDictionary = {
    coda_special_no_data: (False, None),
    coda_special_vsf_integer: (True, numpy.float64),
    coda_special_time: (True, numpy.float64),
    coda_special_complex: (True, numpy.complex128)
}

# dictionary mapping coda class to Type subclass
_codaClassToTypeClass = {
    coda_integer_class: IntegerType,
    coda_real_class: RealType,
    coda_text_class: TextType,
    coda_raw_class: RawType,
    coda_array_class: ArrayType,
    coda_record_class: RecordType,
    coda_special_class: SpecialType,
}
