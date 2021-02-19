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

from __future__ import print_function

import functools

import os
from platform import system as _system

import numpy

from _codac import ffi as _ffi

# TODO __all__ ?


class Product():
    def __init__(self, _x):
        self._x = _x

    def __enter__(self):
        return self

    def __exit__(self, *args): # TODO args?
        close(self)

    def cursor(self):
        cursor = Cursor()
        cursor_set_product(cursor, self)
        return cursor

    # TODO fetch


class Cursor():
    def __init__(self):
        self._x = _ffi.new('coda_cursor *')

    # TODO fetch


class Type():
    def __init__(self, _x):
        self._x = _x


def coda_set_definition_path_conditional(p1, p2, p3):
    def conv(p):
        if p is None:
            return _ffi.NULL
        else:
            return _encode_path(p)
    return _lib.coda_set_definition_path_conditional(conv(p1), conv(p2), conv(p3))


def init():
    _lib.coda_init()


def done():
    _lib.coda_done()


def open(path):
    x = _ffi.new('coda_product **')
    code = _lib.coda_open(_encode_path(path), x)
    if code == 0:
        return Product(x[0])
    else:
        pass # TODO


def close(product):
    _lib.coda_close(product._x)


def cursor_set_product(cursor, product):
    _lib.coda_cursor_set_product(cursor._x, product._x)


def cursor_goto(cursor, name): # TODO name
    _lib.coda_cursor_goto(cursor._x, _encode_string(name))


def cursor_get_array_dim(cursor): # TODO exists in swig version?
    x = _ffi.new('int *')
    y = _ffi.new('long[8]')
    code = _lib.coda_cursor_get_array_dim(cursor._x, x, y)
    return list(y)[:x[0]]


def cursor_read_double_array(cursor):
    shape = cursor_get_array_dim(cursor) # TODO size?
    size = functools.reduce(lambda x, y: x*y, shape, 1)
    d = _ffi.new('double[%d]' % size)
    code = _lib.coda_cursor_read_double_array(cursor._x, d, 0) # TODO order

    buf = _ffi.buffer(d)
    array = numpy.frombuffer(buf).reshape(shape)
    return array


def cursor_get_type(cursor):
    x = _ffi.new('coda_type **')
    code = _lib.coda_cursor_get_type(cursor._x, x)
    return Type(x[0])


def cursor_get_num_elements(cursor):
    x = _ffi.new('long *')
    code = _lib.coda_cursor_get_num_elements(cursor._x, x)
    return x[0]


def type_get_class(type_):
    x = _ffi.new('enum coda_type_class_enum *') # TODO shorter type?
    code = _lib.coda_type_get_class(type_._x, x)
    return x[0]


def type_get_array_base_type(type_):
    x = _ffi.new('coda_type **')
    code = _lib.coda_type_get_array_base_type(type_._x, x)
    return Type(x[0])


def type_get_read_type(type_):
    x = _ffi.new('coda_native_type *')
    code = _lib.coda_type_get_read_type(type_._x, x)
    return x[0]


class Error(Exception):
    """Exception base class for all CODA Python interface errors."""
    pass


class CLibraryError(Error):
    """Exception raised when an error occurs inside the CODA C library.

    Attributes:
        errno       --  error code; if None, the error code will be retrieved from
                        the CODA C library.
        strerror    --  error message; if None, the error message will be retrieved
                        from the CODA C library.

    """
    def __init__(self, errno=None, strerror=None):
        if errno is None:
            errno = _lib.coda_get_errno()[0] # TODO coda.coda_errno? remove [0]

        if strerror is None:
            strerror = _decode_string(_ffi.string(_lib.coda_errno_to_string(errno)))

        super(CLibraryError, self).__init__(errno, strerror)
        self.errno = errno
        self.strerror = strerror

    def __str__(self):
        return self.strerror


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
    return _decode_string(_ffi.string(_lib.coda_get_libcoda_version()))


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
        # This branch will be taken for instances of class str on Python 2 (since this is an alias for class bytes), and
        # on Python 3 for instances of class bytes.
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
        # This branch will be taken for instances of class str on Python 2 (since this is an alias for class bytes), and
        # on Python 3 for instances of class bytes.
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
    if _system() == "Windows":
        return "coda.dll"

    if _system() == "Darwin":
        library_name = "libcoda.dylib"
    else:
        library_name = "libcoda.so"


    # check for library file in the parent directory (for pyinstaller bundles)
    library_path = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", library_name))
    if os.path.exists(library_path):
        return library_path

    # check for library file in the parent directory
    parent_path = os.path.normpath(os.path.join(os.path.dirname(__file__), "../../..", library_name))
    if os.path.exists(parent_path):
        return parent_path

    # TODO revert
    # otherwise assume it can be found by name
    return '/usr/local/lib/libcoda.so'


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
        if _system() == "Windows":
            dirname = None
        else:
            dirname = os.path.dirname(clib)
        relpath = "../share/coda/definitions"
        if coda_set_definition_path_conditional(basename, dirname, relpath) != 0:
            raise CLibraryError() # TODO add check to harppy?

    # TODO UDUNITS2_XML_PATH only for harp?

    # Set default encoding.
    _encoding = "ascii"

    _lib.coda_init()

#
# Initialize the CODA Python interface.
#
_init()
