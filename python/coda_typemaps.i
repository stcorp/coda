/*
 * Copyright (C) 2007-2018 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
    pass returned PyObject pointers back straight to Python. this is
    used by several numpy related helper functions. (see for instance:
    NUMPY_OUTPUT_HELPER).
*/
%typemap(out) PyObject *HELPER_FUNCTION_RETURN_VALUE
{
    $result = $1;
}


/*
    handle opaque pointer output arguments, i.e. arguments of the
    general form type**, where type* is a pointer to an opaque type.
    (e.g. coda_product** in coda_open())
*/
%typemap(in, numinputs = 0) opaque_pointer **OUTPUT ($*1_type tmp = NULL)
{
    $1 = &tmp;
}

%typemap(argout) opaque_pointer **OUTPUT
{
    $result = SWIG_Python_AppendOutput($result, SWIG_NewPointerObj(*$1, $*1_descriptor, 0));
}


/*
    typemap to convert a Python sequence of strings to a C array
    of strings and a string count. adapted from the swig
    documentation (section 10.7: "multi-argument typemaps")
*/
%typemap(in) (int COUNT, const char **INPUT_ARRAY)
{
    if (PyString_Check($input))
    {
        $1 = 1;
        $2 = (char **)malloc($1 * sizeof(char *));
        if($2 == NULL)
        {
            return PyErr_NoMemory();
        }

        $2[0] = PyString_AsString($input);
    }
    else if (PySequence_Check($input))
    {
        long i;

        $1 = (int)PySequence_Size($input);

        /*
            malloc(0) may not work as expected on every
            platform.
        */
        if ($1 > 0)
        {
            $2 = (char **)malloc($1 * sizeof(char *));
            if ($2 == NULL)
            {
                return PyErr_NoMemory();
            }
        }

        for (i = 0; i < $1; i++)
        {
            PyObject *o = PySequence_GetItem($input, i);
            if (PyString_Check(o))
            {
                $2[i] = PyString_AsString(o);
            }
            else
            {
                $cleanup
                PyErr_SetString(PyExc_ValueError, "expected a sequence of strings");
                return NULL;
            }
        }
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "expected either a string or a sequence of strings");
        return NULL;
    }
}

%typemap(freearg) (int COUNT, const char **INPUT_ARRAY)
{
    if ($2 != NULL)
    {
        free($2);
    }
}


/*
    typemaps to convert a Python sequence of plain integers to
    a C array of int's and a length. adapted from codal2cmaps.i.
*/
/*
    "arginit" typemap: SWIG does not initialize $2 (arg3) to NULL (because
    it is declared const??), which may cause a call of free() on an
    uninitialized pointer (when SWIG_fail is called before $2 is initialized
    by the "in" typemap, see below). the "arginit" typemap should fix this
    problem, as it ensures $2 is initialized to NULL before any conversions
    take place.
*/
%typemap(arginit) (int COUNT, const long INPUT_ARRAY[]),
                  (int COUNT, const long *INPUT_ARRAY )
{
    $2 = NULL;
}

%typemap(in) (int COUNT, const long INPUT_ARRAY[]),
             (int COUNT, const long *INPUT_ARRAY )
{
    if (PyInt_Check($input))
    {
        $1 = 1;
        $2 = (long *)malloc($1 * sizeof(long));
        if ($2 == NULL)
        {
            return PyErr_NoMemory();
        }

        $2[0] = (int)PyInt_AsLong($input);
    }
    else if (PySequence_Check($input))
    {
        long i;

        $1 = (long)PySequence_Size($input);

        /* malloc(0) may not work as expected on every platform. */
        if ($1 > 0)
        {
            $2 = (long *)malloc($1 * sizeof(long));
            if ($2 == NULL)
            {
                return PyErr_NoMemory();
            }
        }

        for (i = 0; i < $1; i++)
        {
            PyObject *o = PySequence_GetItem($input,i);
            if (PyInt_Check(o))
            {
                $2[i] = PyInt_AsLong(o);
            }
            else
            {
                $cleanup
                PyErr_SetString(PyExc_ValueError, "expected a sequence of integers");
                return NULL;
            }
        }
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "expected either a plain integer or a sequence of plain integers");
        return NULL;
    }
}

%typemap(freearg) (int COUNT, const long INPUT_ARRAY[]),
                  (int COUNT, const long *INPUT_ARRAY )
{
    if ($2 != NULL)
    {
        free($2);
    }
}


/*
    typemaps to convert a C array of int's and a length to a
    Python list of plain integers.
*/
%typemap(in, numinputs = 0) (int *COUNT, long OUTPUT_ARRAY[]) (int tmp_count, long tmp_array[CODA_MAX_NUM_DIMS]),
                            (int *COUNT, long *OUTPUT_ARRAY ) (int tmp_count, long tmp_array[CODA_MAX_NUM_DIMS])
{
    $1 = &tmp_count;
    $2 = tmp_array;
}

%typemap(argout) (int *COUNT, long OUTPUT_ARRAY[]),
                 (int *COUNT, long *OUTPUT_ARRAY )
{
    long i;

    PyObject *tmp = PyList_New(*$1);
    if (tmp == NULL)
    {
        return PyErr_NoMemory();
    }

    for (i = 0; i < (*$1); i++)
    {
        PyObject *o = PyInt_FromLong((long)$2[i]);
        PyList_SET_ITEM(tmp, i, o);
    }
    $result = SWIG_Python_AppendOutput($result, tmp);
}


/*
    'generic' typemap to convert output POSIX scalar arguments
    (e.g. int8_t *) to corresponding Python scalars.
*/
%define POSIX_SCALAR_OUTPUT_HELPER(__posix_type, __Python_from_method)
%typemap(in, numinputs = 0) __posix_type *OUTPUT ($*1_ltype tmp)
{
    $1 = &tmp;
}

%typemap(argout) __posix_type *OUTPUT
{
    $result = SWIG_Python_AppendOutput($result, __Python_from_method(*$1));
}
%enddef


/*
    handle output "const char **" arguments. This snippet was
    taken from the SWIG files used by the Subversion project.
*/
%typemap(in, numinputs = 0) const char **OUTPUT (const char *tmp = NULL)
{
    $1 = (char **)&tmp;
}

%typemap(argout) const char **OUTPUT
{
    PyObject *tmp_result;

    if (*$1 == NULL)
    {
        Py_INCREF(Py_None);
        tmp_result = Py_None;
    }
    else
    {
        tmp_result = PyString_FromString(*$1);
        if (tmp_result == NULL)
        {
            return NULL;
        }
    }
    $result = SWIG_Python_AppendOutput($result, tmp_result);
}


/*
    handle output "const char **" arguments with a length of
    type 'long'. this wrapper is able to return strings with
    \0 characters (e.g. binary data).
*/
%typemap(in, numinputs = 0) (const char **BINARY_OUTPUT, long *LENGTH) (const char *tmp = NULL, long tmp_length)
{
    $1 = (char **)&tmp;
    $2 = &tmp_length;
}

%typemap(argout) (const char **BINARY_OUTPUT, long *LENGTH)
{
    PyObject *tmp_result;

    if (*$1 == NULL)
    {
        Py_INCREF(Py_None);
        tmp_result = Py_None;
    }
    else
    {
        tmp_result = PyBytes_FromStringAndSize(*$1, *$2);
        if (tmp_result == NULL)
        {
            return NULL;
        }
    }
    $result = SWIG_Python_AppendOutput($result, tmp_result);
}

%typemap(in, numinputs = 1) (const char *FORMAT, char *STRING) (int alloc = 0)
{
	size_t length;
	int res = SWIG_AsCharPtrAndSize($input, &$1, &length, &alloc);
    if (!SWIG_IsOK(res))
    {
        %variable_fail(res,"$1_type","$1_name");
    }
 	$2 = malloc(length);
    if( $2 == NULL )
    {
        return PyErr_NoMemory();
    }
    $2[0] = '\0';
}

%typemap(argout) (const char *FORMAT, char *STRING)
{
    $result = SWIG_Python_AppendOutput($result, SWIG_FromCharPtr($2));
}

%typemap(freearg) (const char *FORMAT, char *STRING)
{
	if (alloc$argnum == SWIG_NEWOBJ) free($1);
    free($2);
}


/*
----------------------------------------------------------------------------------------
- HELPER FUNCTIONS THAT ALLOW READ FUNCTIONS TO RETURN NUMPY OBJECTS                -
----------------------------------------------------------------------------------------
*/
/*
    'generic' helper function to read C arrays of a specific type
    (e.g. int8) into a numpy object of equivalent type.
*/
%define NUMPY_OUTPUT_HELPER(__helper_name, __function_name, __native_type, __numpy_type)
%inline
%{
    PyObject * ## __helper_name ## (const coda_cursor *cursor)
    {
        int tmp_result, tmp_num_dims;
        npy_intp tmp_dims_int[CODA_MAX_NUM_DIMS];
        long tmp_dims_long[CODA_MAX_NUM_DIMS];
        PyObject *tmp;
        int i;

        tmp_result = coda_cursor_get_array_dim(cursor, &tmp_num_dims, tmp_dims_long);
        if (tmp_result != 0)
        {
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        for (i = 0; i < tmp_num_dims; i++)
        {
            tmp_dims_int[i] = tmp_dims_long[i];
        }

        /* convert a rank-0 array to a rank-1 array of size 1. */
        if (tmp_num_dims == 0)
        {
            tmp_dims_int[tmp_num_dims++] = 1;
        }

        tmp = PyArray_SimpleNew(tmp_num_dims, tmp_dims_int, __numpy_type);
        if (tmp == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp_result = __function_name(cursor, (__native_type*)PyArray_DATA((PyArrayObject *)tmp), coda_array_ordering_c);
        if (tmp_result < 0)
        {
            Py_DECREF(tmp);
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        return tmp;
    }        
%}
%ignore __function_name ## ;
%enddef

/*
    'generic' helper function to read partial C arrays of a specific type
    (e.g. int8) into a numpy object of equivalent type.
*/
%define PARTIAL_NUMPY_OUTPUT_HELPER(__helper_name, __function_name, __native_type, __numpy_type)
%inline
%{
    PyObject * ## __helper_name ## (const coda_cursor *cursor, long offset, long length)
    {
        int tmp_result;
        npy_intp tmp_dims_int[CODA_MAX_NUM_DIMS];
        PyObject *tmp;

		tmp_dims_int[0] = length;
        tmp = PyArray_SimpleNew(1, tmp_dims_int, __numpy_type);
        if (tmp == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp_result = __function_name(cursor, offset, length, (__native_type*)PyArray_DATA((PyArrayObject *)tmp));
        if (tmp_result < 0)
        {
            Py_DECREF(tmp);
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        return tmp;
    }        
%}
%ignore __function_name ## ;
%enddef

/*
    'generic' helper function to read C arrays of special types
    (e.g. complex) into two numpy objects ('split' representation).
*/
%define SPLIT_NUMPY_OUTPUT_HELPER(__helper_name, __function_name, __native_type, __numpy_type)
%inline
%{
    PyObject * ## __helper_name ## (const coda_cursor *cursor)
    {
        int tmp_result, tmp_num_dims;
        npy_intp tmp_dims_int[CODA_MAX_NUM_DIMS];
        long tmp_dims_long[CODA_MAX_NUM_DIMS];
        PyObject *tmp[2];
        PyObject *result_list;
        int i;

        tmp_result = coda_cursor_get_array_dim(cursor, &tmp_num_dims, tmp_dims_long);
        if (tmp_result != 0)
        {
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        for (i = 0; i < tmp_num_dims; i++)
        {
            tmp_dims_int[i] = tmp_dims_long[i];
        }

        /* convert a rank-0 array to a rank-1 array of size 1 */
        if (tmp_num_dims==0)
        {
            tmp_dims_int[tmp_num_dims++] = 1;
        }

        tmp[0] = PyArray_SimpleNew(tmp_num_dims, tmp_dims_int, __numpy_type);
        if (tmp[0] == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp[1] = PyArray_SimpleNew(tmp_num_dims, tmp_dims_int, __numpy_type);
        if (tmp[1] == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp_result = __function_name(cursor, (__native_type *)PyArray_DATA((PyArrayObject *)tmp[0]),
                                     (__native_type *)PyArray_DATA((PyArrayObject *)tmp[1]),
                                     coda_array_ordering_c);
        if (tmp_result < 0)
        {
            Py_DECREF(tmp[0]);
            Py_DECREF(tmp[1]);
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        result_list = PyList_New(2);
        if (result_list == NULL)        
        {
            Py_DECREF(tmp[0]);
            Py_DECREF(tmp[1]);
            return PyErr_NoMemory();
        }

        PyList_SET_ITEM(result_list, 0, tmp[0]);
        PyList_SET_ITEM(result_list, 1, tmp[1]);

        return result_list;
    }        
%}
%ignore __function_name ## ;
%enddef

/*
    'generic' helper function to read a pair of doubles as a 1x2 numpy
    object (e.g. complex).
*/
%define DOUBLE_PAIR_NUMPY_OUTPUT_HELPER(__helper_name, __function_name)
%inline
%{
    PyObject * ## __helper_name ## (const coda_cursor *cursor)
    {
        npy_intp tmp_dims[1] = {2};
        int tmp_result;
        PyObject *tmp;

        tmp = PyArray_SimpleNew(1, tmp_dims, NPY_FLOAT64);
        if (tmp == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp_result = __function_name(cursor, (double *)PyArray_DATA((PyArrayObject *)tmp));
        if (tmp_result < 0)
        {
            Py_DECREF(tmp);
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        return tmp;
    }        
%}
%ignore __function_name ## ;
%enddef


/*
    'generic' helper function to read an array of pairs of doubles (e.g.
    complex), which will be returned as a numpy with an added
    dimension of size 2.
*/
%define DOUBLE_PAIR_ARRAY_NUMPY_OUTPUT_HELPER(__helper_name, __function_name)
%inline
%{
    PyObject * ## __helper_name ## (const coda_cursor *cursor)
    {
        int tmp_result, tmp_num_dims;
        npy_intp tmp_dims_int[CODA_MAX_NUM_DIMS+1];
        long tmp_dims_long[CODA_MAX_NUM_DIMS+1];
        PyObject *tmp;
        int i;

        tmp_result = coda_cursor_get_array_dim(cursor, &tmp_num_dims, tmp_dims_long);
        if (tmp_result != 0)
        {
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        for (i = 0; i < tmp_num_dims; i++)
        {
            tmp_dims_int[i] = tmp_dims_long[i];
        }

        /* convert a rank-0 array to a rank-1 array of size 1 */
        if (tmp_num_dims == 0)
        {
            tmp_dims_int[tmp_num_dims++] = 1;
        }
        tmp_dims_int[tmp_num_dims++] = 2;

        tmp = PyArray_SimpleNew(tmp_num_dims, tmp_dims_int, NPY_FLOAT64);
        if (tmp == NULL)
        {
            return PyErr_NoMemory();
        }

        tmp_result = __function_name(cursor, (double *)PyArray_DATA((PyArrayObject *)tmp),
                                     coda_array_ordering_c);
        if (tmp_result < 0)
        {
            Py_DECREF(tmp);
            return PyErr_Format(codacError, #__function_name"(): %s", coda_errno_to_string(coda_errno));
        }

        return (PyObject*) tmp;
    }        
%}
%ignore __function_name ## ;
%enddef
