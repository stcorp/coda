"""
Copyright (C) 2007-2017 S[&]T, The Netherlands.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
"""

from .codac import *
import numpy
import copy
import os
import io

#
# EXCEPTION HIERARCHY
#
class CodaError(Exception):
    """Generalization of all CODA Layer 1 high-level API errors"""
    pass


class FetchError(CodaError):
    def __init__(self, str):
        CodaError.__init__(self)
        self.str = str

    def __str__(self):
        return "CODA FetchError: " + self.str


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
# CLASS RECORD; REPRESENTS CODA RECORDS IN PYTHON
#
class Record(object):
    """
    A class that represents the CODA record type in Python.

    When a record is read from a product file, a Record instance is
    created and populated with fields using the _registerField() method.
    Each field will appear as an instance attribute. The field name is used as
    the name of the attribute, and its value is read from the product file.
    """

    # dictionary to convert from numpy types to
    # a string representation of the corresponding CODA type.
    _typeToString = {
        numpy.int8:   "int8",
        numpy.uint8:  "uint8",
        numpy.int16:  "int16",
        numpy.uint16: "uint16",
        numpy.int32:  "int32",
        numpy.uint32: "uint32",
        numpy.int64:  "int64",
        numpy.float32: "float",
        numpy.float64: "double",
        numpy.complex128: "complex",
        numpy.object_: "object" }

    def __init__(self):
        self._registeredFields = []

    def _registerField(self,name,data):
        """
        _registerField() is a private method that is used to populate
        the Record with fields read from the product file.
        """
        self._registeredFields.append(name)
        self.__setattr__(name,data)

    def __len__(self):
        """
        Return the number of fields in this record.
        """
        return len(self._registeredFields)

    def __getitem__(self, key):
        if not isinstance(key, int):
            raise TypeError("index should be an integer")

        if key < 0:
            key += len(self._registeredFields)

        if key < 0 or key >= len(self._registeredFields):
            raise IndexError

        return self.__dict__[self._registeredFields[key]]

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

        for field in self._registeredFields:
            data = self.__dict__[field]

            out.write(u"%32s:" % (field))

            if isinstance(data,Record):
                out.write(u"record (%i fields)\n" % (len(data),))

            elif isinstance(data,numpy.ndarray):
                dim = data.shape
                dimString = ""
                for d in dim[:-1]:
                    dimString += "%ix" % (d,)
                dimString += "%i" % (dim[-1],)
                out.write(u"[%s %s]\n" % (dimString,self._typeToString[data.dtype.type]))

            elif isinstance(data,str):
                out.write(u"\"%s\"\n" % (data,))

            else:
                # if type is none of the above, fall back
                # on the type specific __str__() function.
                out.write(u"%s\n" % (data,))

        return out.getvalue()


#
# PATH TRAVERSAL
#
def _traverse_path(cursor,path,start=0):
    """
    _traverse_path() traverses the specified path until
    an array with variable indices is encountered or the
    end of the path is reached. It checks field availability
    for records and index ranges for arrays. An exception is
    thrown when a check fails.
    """

    for pathIndex in range(start,len(path)):
        if isinstance(path[pathIndex],str):
            cursor_goto(cursor,path[pathIndex])
        else:
            if isinstance(path[pathIndex],int):
                arrayIndex = [path[pathIndex]]
            elif isinstance(path[pathIndex],(list,tuple)):
                arrayIndex = path[pathIndex]
            else:
                raise ValueError("path specification (%s) should be a string or (list of) integers" % (path[pathIndex],))

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
            for i in range(0,len(arrayIndex)):
                if arrayIndex[i] == -1:
                    intermediateArray = True
                elif (arrayIndex[i] < 0) or (arrayIndex[i] >= arrayShape[i]):
                    raise ValueError("array index (%i) exceeds array range [0:%i)" % (arrayIndex[i], arrayShape[i]))

            if intermediateArray:
                return (True,pathIndex)
            else:
                # if all indices are non-variable, just move the cursor
                # to the indicated element.
                if rankZeroArray:
                    cursor_goto_array_element(cursor,[])
                else:
                    cursor_goto_array_element(cursor,arrayIndex)

    # we've arrived at the end of the path.
    return (False,len(path) - 1)


#
# HELPER FUNCTIONS FOR CODA.FETCH()
#
def _fetch_intermediate_array(cursor,path,pathIndex=0):
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

    if isinstance(path[pathIndex],int):
        # if the current path element is of type int, then
        # the intermediate array must be of rank 1. hence
        # the int in question must equal -1.
        assert path[pathIndex] == -1, "A rank-1 intermediate array should always be indexed by -1 (got %i)." % (path[pathIndex],)

        fetchShape.append(arrayShape[0])
        fetchStep.append(1)
        fetchStep.append(arrayShape[0])
        elementCount = arrayShape[0]
    else:
        step = 1
        arrayIndex = path[pathIndex]

        for i in reversed(list(range(0,len(arrayIndex)))):
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
    for i in range(0,elementCount):
        # move the cursor to the next required array element.
        while currentElementIndex < nextElementIndex:
            cursor_goto_next_array_element(cursor)
            currentElementIndex += 1

        depth = cursor_get_depth(cursor)

        # traverse the path.
        (intermediateNode,copiedPathIndex) = _traverse_path(cursor,path,pathIndex+1)

        # create the result array by examining the type of the first element.
        # This is equivalent to i == 0
        if array is None:
            assert i == 0 
            # everything is an object until proven a scalar. :-)
            scalar = False

            # check for scalar types.
            nodeType = cursor_get_type(cursor)
            nodeClass = type_get_class(nodeType)

            if ((nodeClass == coda_array_class)
                or (nodeClass == coda_record_class)):
                    # records and arrays are non-scalar.
                    scalar = False

            elif  ((nodeClass == coda_integer_class)
                or (nodeClass == coda_real_class) 
                or (nodeClass == coda_text_class)
                or (nodeClass == coda_raw_class)):

                nodeReadType = type_get_read_type(nodeType)

                if nodeReadType == coda_native_type_not_available:
                    raise FetchError("cannot read array (not all elements are available)")
                else:
                    (scalar,numpyType) = _numpyNativeTypeDictionary[nodeReadType]

            elif nodeClass == coda_special_class:
                nodeSpecialType = type_get_special_type(nodeType)
                (scalar,numpyType) = _numpySpecialTypeDictionary[nodeSpecialType]

            # for convenience, fetchShape is constructed in reverse order. however,
            # numpy's array creation functions expect a shape argument in regular
            # order.
            tmpShape = copy.copy(fetchShape)
            tmpShape.reverse()

            # instantiate the required array class.
            if scalar:
                array = numpy.empty(dtype=numpyType,shape=tmpShape)
            else:
                #element_dtype = numpy.dtype(first_element)
                array = numpy.empty(dtype=object,shape=tmpShape)

        # when this point is reached, a result array has been allocated
        # and the flatArrayIter is set.
        # The required element will now be read, the iterator incremented and the
        # result stored.
        if intermediateNode:
            # an intermediate array was encountered.
            array.flat[i] = _fetch_intermediate_array(cursor,path,copiedPathIndex)
        else:
            # the end of the path was reached. from this point on,
            # the entire subtree is fetched.
            array.flat[i] = _fetch_subtree(cursor)

        # update fetchIndex and nextElementIndex.
        for j in range(0,len(fetchShape)):
            fetchIndex[j] += 1
            nextElementIndex += fetchStep[j]

            if fetchIndex[j] < fetchShape[j]:
                break

            fetchIndex[j] = 0
            nextElementIndex -= fetchStep[j+1]

        for j in range(cursor_get_depth(cursor) - depth):
            cursor_goto_parent(cursor)

    cursor_goto_parent(cursor)
    return array


def _fetch_object_array(cursor):
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
    array = numpy.empty(dtype=object,shape=arrayShape)
    
    # goto the first element
    cursor_goto_first_array_element(cursor)

    # loop over all elements excluding the last one
    arraySizeMinOne = array.size - 1
    for i in range(arraySizeMinOne):
        array.flat[i] = _fetch_subtree(cursor)
        cursor_goto_next_array_element(cursor)

    # final element then back tp parent scope
    array.flat[arraySizeMinOne] = _fetch_subtree(cursor)
    cursor_goto_parent(cursor)

    return array


def _fetch_subtree(cursor):
    """
    _fetch_subtree() recursively fetches all data starting from a specified
    position. this function is commonly called when path traversal reaches the
    end of the path. from that point on _all_ data has to be fetched, i.e. no
    array slicing or fetching of single specified fields has to be performed.
    note: unavailable fields are skipped (i.e. not added to the Record instance),
    while hidden fields are only skipped if the filtering option is set to True.
    """

    nodeType = cursor_get_type(cursor)
    nodeClass = type_get_class(nodeType)

    if nodeClass == coda_record_class:
        fieldCount = cursor_get_num_elements(cursor)

        # check for empty record.
        if fieldCount == 0:
            return Record()

        # get information about the record fields.
        skipField = [False] * fieldCount
        for i in range(0,fieldCount):
            if cursor_get_record_field_available_status(cursor,i) != 1:
                    # skip field if unavailable.
                    skipField[i] = True
                    continue

            if _filterRecordFields:
                skipField[i] = bool(type_get_record_field_hidden_status(nodeType,i))

        # create a new Record.
        record = Record()

        # read data.
        cursor_goto_first_record_field(cursor)
        for i in range(0,fieldCount):
            if not skipField[i]:
                data = _fetch_subtree(cursor)
                fieldName = type_get_record_field_name(nodeType,i)
                record._registerField(fieldName,data)

            # avoid calling cursor_goto_next_record_field() after reading
            # the final field. otherwise, the cursor would get positioned
            # outside the record.
            if i < fieldCount - 1:
                cursor_goto_next_record_field(cursor)

        cursor_goto_parent(cursor)
        return record

    elif (nodeClass == coda_array_class):
        # check for empty array.
        if cursor_get_num_elements(cursor) == 0:
            return None

        # get base type information.
        arrayBaseType = type_get_array_base_type(nodeType)
        arrayBaseClass = type_get_class(arrayBaseType)

        if ((arrayBaseClass == coda_array_class)
            or (arrayBaseClass == coda_record_class)):
                # neither an array of arrays nor an array of records can be read directly.
                # therefore, the elements of the array are read one at a time and stored
                # in a numpy array.
                return _fetch_object_array(cursor)

        elif ((arrayBaseClass == coda_integer_class)
            or (arrayBaseClass == coda_real_class)
            or (arrayBaseClass == coda_text_class)
            or (arrayBaseClass == coda_raw_class)):

            # scalar base type.
            arrayBaseReadType = type_get_read_type(arrayBaseType)
            return _readNativeTypeArrayFunctionDictionary[arrayBaseReadType](cursor)

        elif arrayBaseClass == coda_special_class:
            # special base type.
            arrayBaseSpecialType = type_get_special_type(arrayBaseType)
            if arrayBaseSpecialType == coda_special_no_data:
                # this is a very weird special case that will probably never occur.
                # for consistency, an array with base type coda_special_no_data will
                # be returned as an array of the specified size filled with None.
                arrayShape = cursor_get_array_dim(cursor)

                # handle a rank-0 array by converting it to
                # a 1-dimensional array of size 1.
                if len(arrayShape) == 0:
                    arrayShape.append(1)

                return numpy.empty(None,shape=arrayShape)
            else:
                return _readSpecialTypeArrayFunctionDictionary[arrayBaseSpecialType](cursor)

        else:
            raise FetchError("array of unknown base type")

    elif ((nodeClass == coda_integer_class)
         or (nodeClass == coda_real_class) 
         or (nodeClass == coda_text_class)
         or (nodeClass == coda_raw_class)):

        # scalar type.
        nodeReadType = type_get_read_type(nodeType)
        return _readNativeTypeScalarFunctionDictionary[nodeReadType](cursor)

    elif nodeClass == coda_special_class:
        # special type.
        nodeSpecialType = cursor_get_special_type(cursor)
        return _readSpecialTypeScalarFunctionDictionary[nodeSpecialType](cursor)

    else:
        raise FetchError("element of unknown type")


#
# CODA LAYER I HIGH LEVEL API
#
def _get_cursor(start):
    """
    _get_cursor() takes a valid CODA product file handle _or_ a valid CODA
    cursor as input and returns a new cursor object.
    """

    if not isinstance(start,Cursor):
        # create a cursor
        cursor = Cursor()
        cursor_set_product(cursor,start)
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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
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


def fetch(start,*path):
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
    retrieved with coda.open() _or_ a valid CODA cursor. If the start
    argument is a cursor, then the specified path is traversed starting from
    the position represented by the cursor.

    More information can be found in the CODA Python documentation.
    """

    cursor = _get_cursor(start)

    # traverse the path
    (intermediateNode,pathIndex) = _traverse_path(cursor,path)

    if (intermediateNode):
        result = _fetch_intermediate_array(cursor,path,pathIndex)
    else:
        result = _fetch_subtree(cursor)

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

    if len(path) == 0 or not isinstance(path[-1],str):
        raise ValueError("path argument should not be empty and should end with name of a record field")

    cursor = _get_cursor(start)

    # traverse up until the last node of the path.
    (intermediateNode,pathIndex) = _traverse_path(cursor,path[:-1])
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    # get the field index.
    nodeType = cursor_get_type(cursor)
    fieldIndex = type_get_record_field_index_from_name(nodeType,path[-1])

    # get field availability.
    result = bool(cursor_get_record_field_available_status(cursor,fieldIndex))

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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    fieldCount = type_get_num_record_fields(nodeType)
    instanceFieldCount = fieldCount
    for i in range(0,fieldCount):
        if cursor_get_record_field_available_status(cursor,i) != 1:
            instanceFieldCount -= 1
            continue

        if _filterRecordFields and bool(type_get_record_field_hidden_status(nodeType,i)):
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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    fieldCount = type_get_num_record_fields(nodeType)
    fieldNames = []
    for i in range(0,fieldCount):
        if cursor_get_record_field_available_status(cursor,i) != 1:
            continue

        if _filterRecordFields and bool(type_get_record_field_hidden_status(nodeType,i)):
            continue

        fieldNames.append(type_get_record_field_name(nodeType,i))

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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
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

    (intermediateNode,pathIndex) = _traverse_path(cursor,path)
    if intermediateNode:
        # we encountered an array with variable (-1) indices.
        # this is only allowed when calling coda.fetch().
        raise ValueError("variable (-1) array indices are only allowed when calling coda.fetch()")

    nodeType = cursor_get_type(cursor)
    del cursor

    return type_get_unit(nodeType)


#
# MODULE (PRIVATE) ATTRIBUTES
#

# dictionary (a.k.a. switch construct ;) for native type scalar read functions.
# scalars with type coda_native_type_bytes require extra code to find out their size, so this
# type is omitted here.
_readNativeTypeScalarFunctionDictionary = {
    coda_native_type_int8:   cursor_read_int8,
    coda_native_type_uint8:  cursor_read_uint8,
    coda_native_type_int16:  cursor_read_int16,
    coda_native_type_uint16: cursor_read_uint16,
    coda_native_type_int32:  cursor_read_int32,
    coda_native_type_uint32: cursor_read_uint32,
    coda_native_type_int64:  cursor_read_int64,
    coda_native_type_uint64: cursor_read_uint64,
    coda_native_type_float:  cursor_read_float,
    coda_native_type_double: cursor_read_double,
    coda_native_type_char:   cursor_read_char,
    coda_native_type_string: cursor_read_string,
    coda_native_type_bytes:  cursor_read_bytes }

# dictionary (a.k.a. switch construct ;) for native type array read functions.
_readNativeTypeArrayFunctionDictionary = {
    coda_native_type_int8:   cursor_read_int8_array,
    coda_native_type_uint8:  cursor_read_uint8_array,
    coda_native_type_int16:  cursor_read_int16_array,
    coda_native_type_uint16: cursor_read_uint16_array,
    coda_native_type_int32:  cursor_read_int32_array,
    coda_native_type_uint32: cursor_read_uint32_array,
    coda_native_type_int64:  cursor_read_int64_array,
    coda_native_type_uint64: cursor_read_uint64_array,
    coda_native_type_float:  cursor_read_float_array,
    coda_native_type_double: cursor_read_double_array,
    coda_native_type_char:   _fetch_object_array,
    coda_native_type_string: _fetch_object_array,
    coda_native_type_bytes:  _fetch_object_array }

# dictionary (a.k.a. switch construct ;) for special type scalar read functions.
_readSpecialTypeScalarFunctionDictionary = {
    coda_special_no_data:      lambda x: None,
    coda_special_vsf_integer:  cursor_read_double,
    coda_special_time:         cursor_read_double,
    coda_special_complex:      cursor_read_complex }

# dictionary (a.k.a. switch construct ;) for special type array read functions.
# scalars with type coda_special_no_data is a special case that requires extra code, and
# is therefore omitted here.
_readSpecialTypeArrayFunctionDictionary = {
    coda_special_vsf_integer:  cursor_read_double_array,
    coda_special_time:         cursor_read_double_array,
    coda_special_complex:      cursor_read_complex_array }

# dictionary used as a 'typemap'. a tuple is returned, of which the first element is a
# boolean that indicates if the type is considered to be scalar. if so, the second
# element gives the numpy type that matches the specified CODA type. otherwise the second
# element is None.
_numpyNativeTypeDictionary = {
    coda_native_type_int8:   (True,numpy.int8),
    coda_native_type_uint8:  (True,numpy.uint8),
    coda_native_type_int16:  (True,numpy.int16),
    coda_native_type_uint16: (True,numpy.uint16),
    coda_native_type_int32:  (True,numpy.int32),
    coda_native_type_uint32: (True,numpy.uint32),
    coda_native_type_int64:  (True,numpy.int64),
    coda_native_type_uint64: (True,numpy.uint64),
    coda_native_type_float:  (True,numpy.float32),
    coda_native_type_double: (True,numpy.float64),
    coda_native_type_char:   (False,None),
    coda_native_type_string: (False,None),
    coda_native_type_bytes:  (False,None) }


# dictionary used as a 'typemap'. a tuple is returned, of which the first element is a
# boolean that indicates if the type is considered to be scalar. if so, the second
# element gives the numpy type that matches the specified CODA type. otherwise the second
# element is None.
_numpySpecialTypeDictionary = {
    coda_special_no_data:     (False,None),
    coda_special_vsf_integer: (True,numpy.float64),
    coda_special_time:        (True,numpy.float64),
    coda_special_complex:     (True,numpy.complex128) }

#
# MODULE OPTIONS
#

# _filterRecordFields: if set to True, hidden record fields are ignored.
_filterRecordFields = True

def set_option_filter_record_fields(enable):
    global _filterRecordFields

    _filterRecordFields = bool(enable)

def get_option_filter_record_fields():
    return _filterRecordFields

#
# MODULE INITIALIZATION
#

def _init():
    if os.getenv('CODA_DEFINITION') is None:
        # Set coda definition path relative to coda python package
        relpath = "../../../../share/coda/definitions"
        coda_set_definition_path_conditional(os.path.basename(__file__), os.path.dirname(__file__), relpath)
    init()

# initialize libcoda by calling coda_init()
_init()
