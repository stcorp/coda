/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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

#include "coda-matlab.h"

static mxArray *coda_matlab_get_multi_index_data(coda_cursor *base_cursor, int nrhs, const mxArray *prhs[],
                                                 int num_dims, int *index);
static int coda_matlab_read_data_direct(coda_cursor *cursor, mxArray *mx_data, int index);
static mxArray *coda_matlab_read_array(coda_cursor *cursor, int num_dims, const long *dim, int num_elements);
static void coda_matlab_get_cursor_info(coda_cursor *cursor, mxClassID *class, mxComplexity *complex_flag,
                                        int *is_scalar);


mxArray *coda_matlab_get_data(coda_product *pf, int nrhs, const mxArray *prhs[])
{
    mxArray *mx_data = NULL;
    coda_cursor cursor;
    coda_MatlabCursorInfo info;

    mxAssert(pf != NULL, "Productfile pointer is zero");

    coda_matlab_traverse_product(pf, nrhs, prhs, &cursor, &info);

    if (info.intermediate_cursor_flag == 0)
    {
        mx_data = coda_matlab_read_data(&cursor);
    }
    else
    {
        mx_data = coda_matlab_get_multi_index_data(&cursor, nrhs - (info.argument_index + 1),
                                                   prhs + (info.argument_index + 1), info.num_variable_indices,
                                                   info.variable_index);
    }

    if (mx_data == NULL)
    {
        /* return empty array instead of NULL pointer */
        mx_data = mxCreateNumericArray(0, NULL, mxDOUBLE_CLASS, mxREAL);
    }

    return mx_data;
}

static mxArray *coda_matlab_get_multi_index_data(coda_cursor *base_cursor, int nrhs, const mxArray *prhs[],
                                                 int num_dims, int *index)
{
    mxArray *mx_array = NULL;
    long array_dim[CODA_MAX_NUM_DIMS];
    int array_num_dims;
    long local_index[CODA_MAX_NUM_DIMS];
    long result_dim[CODA_MAX_NUM_DIMS];
    mwSize matlab_dim[CODA_MAX_NUM_DIMS];
    int result_index;
    int result_is_scalar = 0;
    long num_elements;
    long i;
    long j;

    /* Get the dimensions of the array */
    coda_cursor_get_array_dim(base_cursor, &array_num_dims, array_dim);
    if (array_num_dims == 0)
    {
        /* convert to one-dimensional array of size 1 */
        array_num_dims = 1;
        array_dim[0] = 1;
    }

    /* Check */
    if (array_num_dims != num_dims)
    {
        mexPrintf("ERROR: array dimensions mismatch\n");
        mexErrMsgTxt("Error in parameter");
        return NULL;
    }

    /* Check validity of all array dimension extents, reset local index, and set the dimensions of the result array. */
    num_elements = 1;
    for (i = 0; i < num_dims; i++)
    {
        long dim = coda_env.option_swap_dimensions ? array_dim[i] : array_dim[num_dims - i - 1];

        if (dim == 0)
        {
            /* empty array */
            return NULL;
        }
        num_elements *= dim;
        if (index[i] == -1)
        {
            result_dim[i] = dim;
        }
        else
        {
            if (index[i] < 0 || index[i] >= dim)
            {
                mexPrintf("ERROR: array index out of bounds\n");
                mexErrMsgTxt("Error in parameter");
                return NULL;
            }
            result_dim[i] = 1;
        }
        matlab_dim[i] = result_dim[i];
        local_index[i] = 0;
    }

    /* Traverse all variable indices */
    if (coda_cursor_goto_first_array_element(base_cursor) != 0)
    {
        coda_matlab_coda_error();
    }
    result_index = 0;
    for (i = 0; i < num_elements; i++)
    {
        int read_array_element;

        read_array_element = 1;
        for (j = 0; j < num_dims; j++)
        {
            long ind = coda_env.option_swap_dimensions ? local_index[j] : local_index[num_dims - j - 1];

            if (index[j] != -1 && ind != index[j])
            {
                read_array_element = 0;
                break;
            }
        }

        if (read_array_element)
        {
            coda_MatlabCursorInfo info;
            coda_cursor cursor;

            /* Copy base cursor */
            cursor = *base_cursor;

            /* Traverse data from the new cursor */
            coda_matlab_traverse_data(nrhs, prhs, &cursor, &info);

            /* Create appropriate result array in the first iteration */
            if (mx_array == NULL)
            {
                mxComplexity complex_flag = 0;
                mxClassID class = 0;

                if (info.intermediate_cursor_flag == 0)
                {
                    coda_matlab_get_cursor_info(&cursor, &class, &complex_flag, &result_is_scalar);
                }
                else
                {
                    result_is_scalar = 0;
                }

                if (result_is_scalar)
                {
                    mx_array = mxCreateNumericArray(num_dims, matlab_dim, class, complex_flag);
                }
                else
                {
                    mx_array = mxCreateCellArray(num_dims, matlab_dim);
                }
            }

            /* Read data */
            if (result_is_scalar)
            {
                long index = result_index;

                if (coda_env.option_swap_dimensions)
                {
                    index = coda_c_index_to_fortran_index(num_dims, result_dim, index);
                }
                if (coda_matlab_read_data_direct(&cursor, mx_array, index) != 0)
                {
                    break;
                }
            }
            else
            {
                mxArray *mx_data = NULL;

                if (info.intermediate_cursor_flag == 0)
                {
                    mx_data = coda_matlab_read_data(&cursor);
                }
                else
                {
                    mx_data = coda_matlab_get_multi_index_data(&cursor, nrhs - (info.argument_index + 1),
                                                               prhs + (info.argument_index + 1),
                                                               info.num_variable_indices, info.variable_index);
                }
                if (coda_env.option_swap_dimensions)
                {
                    mxSetCell(mx_array, coda_c_index_to_fortran_index(num_dims, result_dim, result_index), mx_data);
                }
                else
                {
                    mxSetCell(mx_array, result_index, mx_data);
                }
            }
            result_index++;
        }

        /* Update indices and goto next array element */
        for (j = num_dims - 1; j >= 0; j--)
        {
            local_index[j]++;
            if (local_index[j] < array_dim[j])
            {
                break;
            }
            local_index[j] = 0;
        }
        if (i < num_elements - 1)
        {
            if (coda_cursor_goto_next_array_element(base_cursor) != 0)
            {
                coda_matlab_coda_error();
            }
        }
    }

    coda_cursor_goto_parent(base_cursor);

    return mx_array;
}

mxArray *coda_matlab_read_data(coda_cursor *cursor)
{
    mxArray *mx_data = NULL;
    coda_type_class type_class;

    mxAssert(cursor != NULL, "Coda Cursor pointer is zero");

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    switch (type_class)
    {
        case coda_array_class:
            {
                int num_dims;
                long dim[CODA_MAX_NUM_DIMS];
                int num_elements;
                int i;

                if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    coda_matlab_coda_error();
                }
                mxAssert(num_dims <= CODA_MAX_NUM_DIMS, "Number of dimensions is too high");
                num_elements = 1;
                for (i = 0; i < num_dims; i++)
                {
                    num_elements *= dim[i];
                }
                if (num_elements > 0)
                {
                    mx_data = coda_matlab_read_array(cursor, num_dims, dim, num_elements);
                }
            }
            break;
        case coda_record_class:
            {
                long field_index;
                long num_fields;
                int mx_field_index;
                int mx_num_fields;
                char **field_name;
                int *skip;
                coda_type *record_type;

                if (coda_cursor_get_num_elements(cursor, &num_fields) != 0)
                {
                    coda_matlab_coda_error();
                }
                field_name = mxCalloc(num_fields, sizeof(*field_name));
                skip = mxCalloc(num_fields, sizeof(*skip));

                mx_num_fields = 0;

                if (coda_cursor_get_type(cursor, &record_type) != 0)
                {
                    coda_matlab_coda_error();
                }

                for (field_index = 0; field_index < num_fields; field_index++)
                {
                    int available;

                    skip[field_index] = 0;
                    if (coda_cursor_get_record_field_available_status(cursor, field_index, &available) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    if (!available)
                    {
                        skip[field_index] = 1;
                    }
                    else if (coda_env.option_filter_record_fields)
                    {
                        int hidden;

                        if (coda_type_get_record_field_hidden_status(record_type, field_index, &hidden) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        if (hidden)
                        {
                            skip[field_index] = 1;
                        }
                    }
                    if (!skip[field_index])
                    {
                        if (coda_type_get_record_field_name(record_type, field_index,
                                                            (const char **)&field_name[mx_num_fields]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        mx_num_fields++;
                    }
                }

                mx_data = mxCreateStructMatrix(1, 1, mx_num_fields, (const char **)field_name);
                mx_field_index = 0;
                if (num_fields > 0)
                {
                    if (coda_cursor_goto_first_record_field(cursor) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    for (field_index = 0; field_index < num_fields; field_index++)
                    {
                        if (!skip[field_index])
                        {
                            mxArray *mx_array;

                            mx_array = coda_matlab_read_data(cursor);
                            mxSetField(mx_data, 0, field_name[mx_field_index], mx_array);
                            mx_field_index++;
                        }
                        if (field_index < num_fields - 1)
                        {
                            if (coda_cursor_goto_next_record_field(cursor) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                    }
                    coda_cursor_goto_parent(cursor);
                }

                mxFree(field_name);
                mxFree(skip);
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxINT8_CLASS, mxREAL);
                            if (coda_cursor_read_int8(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxUINT8_CLASS, mxREAL);
                            if (coda_cursor_read_uint8(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxINT16_CLASS, mxREAL);
                            if (coda_cursor_read_int16(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxUINT16_CLASS, mxREAL);
                            if (coda_cursor_read_uint16(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
                            if (coda_cursor_read_int32(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
                            if (coda_cursor_read_uint32(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
                            if (coda_cursor_read_int64(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
                            if (coda_cursor_read_uint64(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
                            if (coda_cursor_read_float(cursor, mxGetData(mx_data)) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_double:
                        mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                        if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_native_type_char:
                    case coda_native_type_string:
                        {
                            long length;
                            char *str;

                            if (coda_cursor_get_string_length(cursor, &length) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            str = mxCalloc(length + 1, 1);
                            if (coda_cursor_read_string(cursor, str, length + 1) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            while (length > 0 && str[length - 1] == ' ')
                            {
                                length--;
                            }
                            str[length] = 0;
                            mx_data = mxCreateString(str);
                            mxFree(str);
                        }
                        break;
                    case coda_native_type_bytes:
                        {
                            int64_t byte_size;

                            if (coda_cursor_get_byte_size(cursor, &byte_size) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            if (byte_size == 0)
                            {
                                /* return empty mx_array */
                            }
                            else
                            {
                                mwSize dim[CODA_MAX_NUM_DIMS];

                                dim[0] = (int)byte_size;
                                mx_data = mxCreateNumericArray(1, dim, mxUINT8_CLASS, mxREAL);
                                if (coda_cursor_read_bytes(cursor, mxGetData(mx_data), 0, dim[0]) != 0)
                                {
                                    coda_matlab_coda_error();
                                }
                            }
                        }
                        break;
                    case coda_native_type_not_available:
                        mxAssert(0, "Cannot read data of this type");
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
                        if (coda_cursor_read_double(cursor, mxGetData(mx_data)) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_complex:
                        mx_data = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxCOMPLEX);
                        if (coda_cursor_read_complex_double_split(cursor, mxGetData(mx_data), mxGetImagData(mx_data))
                            != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_no_data:
                        /* return empty mx_array */
                        break;
                }
            }
            break;
    }
    return mx_data;
}

static int coda_matlab_read_data_direct(coda_cursor *cursor, mxArray *mx_data, int index)
{
    coda_type_class type_class;

    mxAssert(cursor != NULL, "Coda Cursor pointer is zero");

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    switch (type_class)
    {
        case coda_array_class:
        case coda_record_class:
            mxAssert(0, "Invalid internal parameters");
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_int8(cursor, &((int8_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_uint8(cursor, &((uint8_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_int16(cursor, &((int16_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_uint16(cursor, &((uint16_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_int32(cursor, &((int32_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_uint32(cursor, &((uint32_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_int64(cursor, &((int64_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_uint64(cursor, &((uint64_t *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            if (coda_cursor_read_float(cursor, &((float *)mxGetData(mx_data))[index]) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_double:
                        if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_native_type_char:
                    case coda_native_type_string:
                    case coda_native_type_bytes:
                    case coda_native_type_not_available:
                        mxAssert(0, "Invalid internal parameters");
                        break;
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        if (coda_cursor_read_double(cursor, &((double *)mxGetData(mx_data))[index]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_complex:
                        if (coda_cursor_read_complex_double_split(cursor, &((double *)mxGetData(mx_data))[index],
                                                                  &((double *)mxGetImagData(mx_data))[index]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_no_data:
                        mxAssert(0, "Invalid internal parameters");
                        break;
                }
            }
            break;
    }

    return 0;
}

/*
 * PRE: num_dims >= 0 and num_elements > 0 (i.e. no empty array traversal)
 */
static mxArray *coda_matlab_read_array(coda_cursor *cursor, int num_dims, const long *dim, int num_elements)
{
    mwSize matlab_dim[CODA_MAX_NUM_DIMS];
    long temp_dim[1];
    mxArray *mx_data = NULL;
    coda_type *array_type;
    coda_type *type;
    coda_type_class type_class;
    int i;

    mxAssert(cursor != NULL, "Coda Cursor pointer is zero");

    if (num_dims == 0)
    {
        /* convert zero dimensional array to one-dimensional array of size 1 */
        temp_dim[0] = 1;
        dim = temp_dim;
        num_dims++;
    }

    for (i = 0; i < num_dims; i++)
    {
        matlab_dim[i] = coda_env.option_swap_dimensions ? dim[i] : dim[num_dims - i - 1];
    }

    if (coda_cursor_get_type(cursor, &array_type) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_type_get_class(array_type, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    mxAssert(type_class == coda_array_class, "Coda Cursor does not point to an array.");
    if (coda_type_get_array_base_type(array_type, &type) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_type_get_class(type, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
    {
        if (coda_type_get_special_base_type(type, &type) != 0)
        {
            coda_matlab_coda_error();
        }
        if (coda_type_get_class(type, &type_class) != 0)
        {
            coda_matlab_coda_error();
        }
    }

    switch (type_class)
    {
        case coda_array_class:
            {
                int new_num_dims;
                long new_dim[CODA_MAX_NUM_DIMS];
                long new_num_elements;
                long index = 0;

                mx_data = mxCreateCellArray(num_dims, matlab_dim);
                if (coda_cursor_goto_first_array_element(cursor) != 0)
                {
                    coda_matlab_coda_error();
                }
                while (index < num_elements)
                {
                    int i;

                    if (coda_cursor_get_array_dim(cursor, &new_num_dims, new_dim) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    mxAssert(new_num_dims <= CODA_MAX_NUM_DIMS, "Number of dimensions is too high");
                    new_num_elements = 1;
                    for (i = 0; i < new_num_dims; i++)
                    {
                        new_num_elements *= new_dim[i];
                    }
                    if (new_num_elements > 0)
                    {
                        mxArray *mx_array;

                        mx_array = coda_matlab_read_array(cursor, new_num_dims, new_dim, new_num_elements);
                        if (coda_env.option_swap_dimensions)
                        {
                            mxSetCell(mx_data, coda_c_index_to_fortran_index(num_dims, dim, index), mx_array);
                        }
                        else
                        {
                            mxSetCell(mx_data, index, mx_array);
                        }
                    }
                    index++;
                    if (index < num_elements)
                    {
                        if (coda_cursor_goto_next_array_element(cursor) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                    }
                }
                coda_cursor_goto_parent(cursor);
            }
            break;
        case coda_record_class:
            {
                int field_index;
                long num_fields;
                int mx_field_index;
                int mx_num_fields;
                char **field_name;
                int *struct_index;
                int *skip;
                int index = 0;
                long new_dim[CODA_MAX_NUM_DIMS];

                if (num_dims == 1)
                {
                    /* This solves a MATLAB bug that was encountered in MATLAB V5.3 for Windows */
                    num_dims = 2;
                    new_dim[0] = dim[0];
                    new_dim[1] = 1;
                    dim = new_dim;
                    matlab_dim[0] = dim[0];
                    matlab_dim[1] = dim[1];
                }

                if (coda_type_get_num_record_fields(type, &num_fields) != 0)
                {
                    coda_matlab_coda_error();
                }
                field_name = mxCalloc(num_fields, sizeof(*field_name));
                struct_index = mxCalloc(num_fields, sizeof(*struct_index));
                skip = mxCalloc(num_fields, sizeof(*skip));

                mx_num_fields = 0;

                for (field_index = 0; field_index < num_fields; field_index++)
                {
                    skip[field_index] = 0;
                    if (coda_env.option_filter_record_fields)
                    {
                        int hidden;

                        if (coda_type_get_record_field_hidden_status(type, field_index, &hidden) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        if (hidden)
                        {
                            skip[field_index] = 1;
                        }
                    }
                    if (!skip[field_index])
                    {
                        if (coda_type_get_record_field_name(type, field_index,
                                                            (const char **)&field_name[mx_num_fields]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        mx_num_fields++;
                    }
                }

                mx_data = mxCreateStructArray(num_dims, matlab_dim, mx_num_fields, (const char **)field_name);

                /* cache the field_indices */
                for (field_index = 0; field_index < mx_num_fields; field_index++)
                {
                    struct_index[field_index] = mxGetFieldNumber(mx_data, field_name[field_index]);
                }

                if (coda_cursor_goto_first_array_element(cursor) != 0)
                {
                    coda_matlab_coda_error();
                }
                while (index < num_elements)
                {
                    coda_cursor record_cursor;

                    mx_field_index = 0;

                    record_cursor = *cursor;
                    if (coda_cursor_goto_first_record_field(cursor) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    for (field_index = 0; field_index < num_fields; field_index++)
                    {
                        if (!skip[field_index])
                        {
                            mxArray *mx_array = NULL;
                            int available;

                            if (coda_cursor_get_record_field_available_status(&record_cursor, field_index,
                                                                              &available) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            if (available)
                            {
                                mx_array = coda_matlab_read_data(cursor);
                                /* note: if the field is not available, we put a NULL mx object in the field array */
                            }
                            if (coda_env.option_swap_dimensions)
                            {
                                mxSetFieldByNumber(mx_data, coda_c_index_to_fortran_index(num_dims, dim, index),
                                                   struct_index[mx_field_index], mx_array);
                            }
                            else
                            {
                                mxSetFieldByNumber(mx_data, index, struct_index[mx_field_index], mx_array);
                            }
                            mx_field_index++;
                        }
                        if (field_index < num_fields - 1)
                        {
                            if (coda_cursor_goto_next_record_field(cursor) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                    }
                    coda_cursor_goto_parent(cursor);
                    index++;
                    if (index < num_elements)
                    {
                        if (coda_cursor_goto_next_array_element(cursor) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                    }
                }
                coda_cursor_goto_parent(cursor);

                mxFree(field_name);
                mxFree(struct_index);
                mxFree(skip);
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;
                coda_array_ordering array_ordering =
                    coda_env.option_swap_dimensions ? coda_array_ordering_fortran : coda_array_ordering_c;

                if (coda_type_get_read_type(type, &read_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxINT8_CLASS, mxREAL);
                            if (coda_cursor_read_int8_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxUINT8_CLASS, mxREAL);
                            if (coda_cursor_read_uint8_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxINT16_CLASS, mxREAL);
                            if (coda_cursor_read_int16_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxUINT16_CLASS, mxREAL);
                            if (coda_cursor_read_uint16_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxINT32_CLASS, mxREAL);
                            if (coda_cursor_read_int32_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxUINT32_CLASS, mxREAL);
                            if (coda_cursor_read_uint32_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxINT64_CLASS, mxREAL);
                            if (coda_cursor_read_int64_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxUINT64_CLASS, mxREAL);
                            if (coda_cursor_read_uint64_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                            if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        else
                        {
                            mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxSINGLE_CLASS, mxREAL);
                            if (coda_cursor_read_float_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                        }
                        break;
                    case coda_native_type_double:
                        mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                        if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_native_type_char:
                        {
                            mxChar *mx_ch;
                            char *ch;
                            int i;

                            mx_data = mxCreateCharArray(num_dims, matlab_dim);
                            mx_ch = mxGetData(mx_data);
                            ch = mxCalloc(num_elements, sizeof(char));
                            if (coda_cursor_read_char_array(cursor, ch, array_ordering) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            for (i = 0; i < num_elements; i++)
                            {
                                mx_ch[i] = (mxChar) ch[i];
                            }
                        }
                        break;
                    case coda_native_type_string:
                        {
                            int index = 0;

                            mx_data = mxCreateCellArray(num_dims, matlab_dim);
                            if (coda_cursor_goto_first_array_element(cursor) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            while (index < num_elements)
                            {
                                mxArray *mx_array;
                                long length;
                                char *str;

                                if (coda_cursor_get_string_length(cursor, &length) != 0)
                                {
                                    coda_matlab_coda_error();
                                }
                                str = mxCalloc(length + 1, 1);
                                if (coda_cursor_read_string(cursor, str, length + 1) != 0)
                                {
                                    coda_matlab_coda_error();
                                }
                                while (length > 0 && str[length - 1] == ' ')
                                {
                                    length--;
                                }
                                str[length] = 0;
                                mx_array = mxCreateString(str);
                                if (coda_env.option_swap_dimensions)
                                {
                                    mxSetCell(mx_data, coda_c_index_to_fortran_index(num_dims, dim, index), mx_array);
                                }
                                else
                                {
                                    mxSetCell(mx_data, index, mx_array);
                                }
                                mxFree(str);

                                index++;
                                if (index < num_elements)
                                {
                                    if (coda_cursor_goto_next_array_element(cursor) != 0)
                                    {
                                        coda_matlab_coda_error();
                                    }
                                }
                            }
                            coda_cursor_goto_parent(cursor);
                        }
                        break;
                    case coda_native_type_bytes:
                        {
                            int index = 0;

                            mx_data = mxCreateCellArray(num_dims, matlab_dim);
                            if (coda_cursor_goto_first_array_element(cursor) != 0)
                            {
                                coda_matlab_coda_error();
                            }
                            while (index < num_elements)
                            {
                                mxArray *mx_bytes;

                                mx_bytes = coda_matlab_read_data(cursor);
                                if (coda_env.option_swap_dimensions)
                                {
                                    mxSetCell(mx_data, coda_c_index_to_fortran_index(num_dims, dim, index), mx_bytes);
                                }
                                else
                                {
                                    mxSetCell(mx_data, index, mx_bytes);
                                }

                                index++;
                                if (index < num_elements)
                                {
                                    if (coda_cursor_goto_next_array_element(cursor) != 0)
                                    {
                                        coda_matlab_coda_error();
                                    }
                                }
                            }
                            coda_cursor_goto_parent(cursor);
                        }
                        break;
                    case coda_native_type_not_available:
                        break;
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;
                coda_array_ordering array_ordering =
                    coda_env.option_swap_dimensions ? coda_array_ordering_fortran : coda_array_ordering_c;

                if (coda_type_get_special_type(type, &special_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxREAL);
                        if (coda_cursor_read_double_array(cursor, mxGetData(mx_data), array_ordering) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_complex:
                        mx_data = mxCreateNumericArray(num_dims, matlab_dim, mxDOUBLE_CLASS, mxCOMPLEX);
                        if (coda_cursor_read_complex_double_split_array(cursor, mxGetData(mx_data),
                                                                        mxGetImagData(mx_data), array_ordering) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        break;
                    case coda_special_no_data:
                        /* return mx_array with empty elements */
                        mx_data = mxCreateCellArray(num_dims, matlab_dim);
                        break;
                }
            }
            break;
    }

    return mx_data;
}

static void coda_matlab_get_cursor_info(coda_cursor *cursor, mxClassID *class, mxComplexity *complex_flag,
                                        int *is_scalar)
{
    coda_type_class type_class;

    mxAssert(cursor != NULL, "Coda Cursor pointer is zero");

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    switch (type_class)
    {
        case coda_array_class:
        case coda_record_class:
            *is_scalar = 0;
            break;

        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxINT8_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_uint8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxUINT8_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_int16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxINT16_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_uint16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxUINT16_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_int32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxINT32_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_uint32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxUINT32_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_int64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxINT64_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_uint64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxUINT64_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_float:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            *class = mxDOUBLE_CLASS;
                        }
                        else
                        {
                            *class = mxSINGLE_CLASS;
                        }
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_double:
                        *class = mxDOUBLE_CLASS;
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_native_type_char:
                    case coda_native_type_string:
                    case coda_native_type_bytes:
                        *is_scalar = 0;
                        break;
                    case coda_native_type_not_available:
                        mxAssert(0, "Cannot read data of this type");
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        *class = mxDOUBLE_CLASS;
                        *complex_flag = mxREAL;
                        *is_scalar = 1;
                        break;
                    case coda_special_complex:
                        *class = mxDOUBLE_CLASS;
                        *complex_flag = mxCOMPLEX;
                        *is_scalar = 1;
                        break;
                    case coda_special_no_data:
                        *is_scalar = 0;
                        break;
                }
            }
            break;
    }
}
