/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include "coda-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "coda-hdf5-internal.h"

static void get_hdf5_type_and_size(coda_native_type read_type, hid_t *type_id, int *size)
{
    switch (read_type)
    {
        case coda_native_type_int8:
            *type_id = H5T_NATIVE_INT8;
            *size = 1;
            break;
        case coda_native_type_uint8:
            *type_id = H5T_NATIVE_UINT8;
            *size = 1;
            break;
        case coda_native_type_int16:
            *type_id = H5T_NATIVE_INT16;
            *size = 2;
            break;
        case coda_native_type_uint16:
            *type_id = H5T_NATIVE_UINT16;
            *size = 2;
            break;
        case coda_native_type_int32:
            *type_id = H5T_NATIVE_INT32;
            *size = 4;
            break;
        case coda_native_type_uint32:
            *type_id = H5T_NATIVE_UINT32;
            *size = 4;
            break;
        case coda_native_type_int64:
            *type_id = H5T_NATIVE_INT64;
            *size = 8;
            break;
        case coda_native_type_uint64:
            *type_id = H5T_NATIVE_UINT64;
            *size = 8;
            break;
        case coda_native_type_float:
            *type_id = H5T_NATIVE_FLOAT;
            *size = 4;
            break;
        case coda_native_type_double:
            *type_id = H5T_NATIVE_DOUBLE;
            *size = 8;
            break;
        default:
            assert(0);
            exit(1);
    }
}

int coda_hdf5_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = (coda_dynamic_type *)product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_hdf5_type *record_type;
    coda_dynamic_type *field_type;

    record_type = (coda_hdf5_type *)cursor->stack[cursor->n - 1].type;
    switch (record_type->tag)
    {
        case tag_hdf5_compound_datatype:
            if (index < 0 || index >= ((coda_hdf5_compound_data_type *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf5_compound_data_type *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = (coda_dynamic_type *)((coda_hdf5_compound_data_type *)record_type)->member[index];
            break;
        case tag_hdf5_group:
            if (index < 0 || index >= ((coda_hdf5_group *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf5_group *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = (coda_dynamic_type *)((coda_hdf5_group *)record_type)->object[index];
            break;
        default:
            assert(0);
            exit(1);
    }


    cursor->n++;
    cursor->stack[cursor->n - 1].type = field_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */
    return 0;
}

int coda_hdf5_cursor_goto_next_record_field(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_hdf5_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_hdf5_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_hdf5_type *array_type = (coda_hdf5_type *)cursor->stack[cursor->n - 1].type;
    coda_type_array *definition = (coda_type_array *)array_type->definition;
    coda_dynamic_type *base_type;
    long offset_elements;
    long i;

    switch (array_type->tag)
    {
        case tag_hdf5_dataset:
            base_type = (coda_dynamic_type *)((coda_hdf5_dataset *)array_type)->base_type;
            break;
        default:
            assert(0);
            exit(1);
    }

    /* check the number of dimensions */
    if (num_subs != definition->num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d)", num_subs,
                       definition->num_dims);
        return -1;
    }

    /* check the dimensions... */
    offset_elements = 0;
    for (i = 0; i < definition->num_dims; i++)
    {
        if (subs[i] < 0 || subs[i] >= definition->dim[i])
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", subs[i],
                           definition->dim[i]);
            return -1;
        }
        if (i > 0)
        {
            offset_elements *= definition->dim[i];
        }
        offset_elements += subs[i];
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_hdf5_type *array_type = (coda_hdf5_type *)cursor->stack[cursor->n - 1].type;
    coda_dynamic_type *base_type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        if (coda_hdf5_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    switch (array_type->tag)
    {
        case tag_hdf5_dataset:
            base_type = (coda_dynamic_type *)((coda_hdf5_dataset *)array_type)->base_type;
            break;
        default:
            assert(0);
            exit(1);
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_next_array_element(coda_cursor *cursor)
{
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;
        long index;

        index = cursor->stack[cursor->n - 1].index + 1;

        cursor->n--;
        if (coda_hdf5_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            cursor->n++;
            return -1;
        }
        cursor->n++;

        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    cursor->stack[cursor->n - 1].index++;

    return 0;
}

int coda_hdf5_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_hdf5_type *type;

    type = (coda_hdf5_type *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_hdf5_basic_datatype:
        case tag_hdf5_compound_datatype:
        case tag_hdf5_group:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf5_group *)type)->attributes;
            break;
        case tag_hdf5_dataset:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf5_dataset *)type)->attributes;
            break;
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_hdf5_type *type;

    type = (coda_hdf5_type *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_hdf5_basic_datatype:
            *num_elements = 1;
            break;
        case tag_hdf5_compound_datatype:
            *num_elements = (long)((coda_hdf5_compound_data_type *)type)->definition->num_fields;
            break;
        case tag_hdf5_group:
            *num_elements = (long)((coda_hdf5_group *)type)->definition->num_fields;
            break;
        case tag_hdf5_dataset:
            *num_elements = (long)((coda_hdf5_dataset *)type)->definition->num_elements;
            break;
    }

    return 0;
}

int coda_hdf5_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    coda_hdf5_basic_data_type *base_type;

    base_type = (coda_hdf5_basic_data_type *)cursor->stack[cursor->n - 1].type;
    if (base_type->is_variable_string)
    {
        coda_hdf5_dataset *dataset;
        long array_index;
        hsize_t size;

        /* in CODA, variable strings should only exist when the parent is a dataset */
        dataset = (coda_hdf5_dataset *)cursor->stack[cursor->n - 2].type;
        assert(dataset->tag == tag_hdf5_dataset);
        array_index = cursor->stack[cursor->n - 1].index;
        if (dataset->definition->num_dims > 0)
        {
            hsize_t coord[CODA_MAX_NUM_DIMS];
            int i;

            for (i = dataset->definition->num_dims - 1; i >= 0; i--)
            {
                coord[i] = array_index % dataset->definition->dim[i];
                array_index = array_index / dataset->definition->dim[i];
            }
            if (H5Sselect_elements(dataset->dataspace_id, H5S_SELECT_SET, 1, (const hsize_t *)coord) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                return -1;
            }
        }

        if (H5Dvlen_get_buf_size(dataset->dataset_id, base_type->datatype_id, dataset->dataspace_id, &size) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }

        if (H5Sselect_all(dataset->dataspace_id) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }

        /* if the data type uses 'H5T_STR_NULLTERM', we need to subtract 1 to get the actual string length */
        if (size > 0 && H5Tget_strpad(base_type->datatype_id) == H5T_STR_NULLTERM)
        {
            size--;
        }

        *length = (long)size;
    }
    else
    {
        *length = H5Tget_size(base_type->datatype_id);
    }

    return 0;
}

int coda_hdf5_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    return coda_type_get_array_dim(cursor->stack[cursor->n - 1].type->definition, num_dims, dim);
}

static int read_array(const coda_cursor *cursor, void *dst)
{
    coda_hdf5_basic_data_type *base_type;
    hid_t mem_type_id;
    long num_elements;
    int element_to_size;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];

    if (coda_hdf5_cursor_get_num_elements(cursor, &num_elements) != 0)
    {
        return -1;
    }
    if (num_elements <= 0)
    {
        /* no data to be read */
        return 0;
    }
    if (coda_hdf5_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    base_type = (coda_hdf5_basic_data_type *)((coda_hdf5_dataset *)cursor->stack[cursor->n - 1].type)->base_type;
    assert(base_type->tag == tag_hdf5_basic_datatype);

    if (H5Tget_class(base_type->datatype_id) == H5T_ENUM)
    {
        /* we read the data as an enumeration and perform the conversion to a native type after reading */
        element_to_size = H5Tget_size(base_type->datatype_id);
        /* we make a copy to ease the cleanup process */
        mem_type_id = H5Tcopy(base_type->datatype_id);
    }
    else
    {
        get_hdf5_type_and_size(base_type->definition->read_type, &mem_type_id, &element_to_size);
        /* we make a copy to ease the cleanup process */
        mem_type_id = H5Tcopy(mem_type_id);
    }

    if (H5Dread(((coda_hdf5_dataset *)cursor->stack[cursor->n - 1].type)->dataset_id, mem_type_id, H5S_ALL, H5S_ALL,
                H5P_DEFAULT, dst) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        H5Tclose(mem_type_id);
        return -1;
    }
    H5Tclose(mem_type_id);

    if (H5Tget_class(base_type->datatype_id) == H5T_ENUM)
    {
        hid_t super;
        int native_element_size;

        /* convert the enumeration data to our native type */
        super = H5Tget_super(base_type->datatype_id);
        if (super < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }
        get_hdf5_type_and_size(base_type->definition->read_type, &mem_type_id, &native_element_size);
        assert(native_element_size == element_to_size);
        if (H5Tconvert(super, mem_type_id, num_elements, dst, NULL, H5P_DEFAULT) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Tclose(super);
            return -1;
        }
        H5Tclose(super);
    }

    return 0;
}

static int read_partial_array(const coda_cursor *cursor, long offset, long length, void *dst)
{
    coda_hdf5_basic_data_type *base_type;
    coda_hdf5_dataset *dataset;
    hsize_t start[CODA_MAX_NUM_DIMS];
    hsize_t count[CODA_MAX_NUM_DIMS];
    hsize_t hlength;
    hid_t mem_type_id;
    hid_t mem_space_id;
    int element_to_size;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long block_size = 1;
    int i;

    dataset = (coda_hdf5_dataset *)cursor->stack[cursor->n - 1].type;
    base_type = (coda_hdf5_basic_data_type *)dataset->base_type;
    assert(base_type->tag == tag_hdf5_basic_datatype);

    if (coda_hdf5_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    /* determine hyperslab start/edge */
    if (num_dims == 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "partial array reading not allowed for zero dimensional HDF5 Dataset");
        return -1;
    }
    for (i = num_dims - 1; i >= 0; i--)
    {
        if (length <= block_size * dim[i])
        {
            if (length % block_size != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF5 Datasets requires length "
                               "(%ld) to be a multiple of the subdimension size (%ld)", length, block_size);
                return -1;
            }
            start[i] = (offset / block_size) % dim[i];
            count[i] = length / block_size;
            break;
        }
        start[i] = 0;
        count[i] = dim[i];
        block_size *= dim[i];
    }
    if (offset % block_size != 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF5 Datasets requires offset (%ld) to "
                       "be a multiple of the subdimension size (%ld)", offset, block_size);
        return -1;
    }
    if (start[i] + count[i] > (hsize_t)dim[i])
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF5 Datasets requires offset (%ld) "
                       "and length (%ld) to represent a hyperslab (range [%ld,%ld] exceeds length of dimension #%d "
                       "(%ld)))", offset, length, (long)start[i], (long)(start[i] + count[i] - 1), i, dim[i]);
        return -1;
    }
    while (i > 0)
    {
        block_size *= dim[i];
        i--;
        start[i] = (offset / block_size) % dim[i];
        count[i] = 1;
    }

    if (H5Tget_class(base_type->datatype_id) == H5T_ENUM)
    {
        /* we read the data as an enumeration and perform the conversion to a native type after reading */
        element_to_size = H5Tget_size(base_type->datatype_id);
        /* we make a copy to ease the cleanup process */
        mem_type_id = H5Tcopy(base_type->datatype_id);
    }
    else
    {
        get_hdf5_type_and_size(base_type->definition->read_type, &mem_type_id, &element_to_size);
        /* we make a copy to ease the cleanup process */
        mem_type_id = H5Tcopy(mem_type_id);
    }

    if (H5Sselect_hyperslab(dataset->dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }

    hlength = (hsize_t)length;
    mem_space_id = H5Screate_simple(1, &hlength, NULL);
    if (mem_space_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        H5Tclose(mem_type_id);
        return -1;
    }

    if (H5Dread(dataset->dataset_id, mem_type_id, mem_space_id, dataset->dataspace_id, H5P_DEFAULT, dst) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        H5Sclose(mem_space_id);
        H5Tclose(mem_type_id);
        return -1;
    }

    H5Sclose(mem_space_id);
    H5Tclose(mem_type_id);
    if (H5Sselect_all(dataset->dataspace_id) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }

    if (H5Tget_class(base_type->datatype_id) == H5T_ENUM)
    {
        hid_t super;
        int native_element_size;

        /* convert the enumeration data to our native type */
        super = H5Tget_super(base_type->datatype_id);
        if (super < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }
        get_hdf5_type_and_size(base_type->definition->read_type, &mem_type_id, &native_element_size);
        assert(native_element_size == element_to_size);
        if (H5Tconvert(super, mem_type_id, length, dst, NULL, H5P_DEFAULT) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Tclose(super);
            return -1;
        }
        H5Tclose(super);
    }

    return 0;
}

static int read_basic_type(const coda_cursor *cursor, void *dst, long dst_size)
{
    coda_hdf5_basic_data_type *base_type;
    coda_hdf5_compound_data_type *compound_type = NULL;
    coda_hdf5_dataset *dataset;
    hid_t mem_space_id;
    hid_t datatype_to;
    char *buffer = NULL;
    char *buffer_offset;        /* position in buffer where our value is stored */
    int is_compound_member;
    int array_depth;
    long array_index;
    long compound_index = -1;
    long size = -1;

    assert(cursor->n > 1);
    base_type = (coda_hdf5_basic_data_type *)cursor->stack[cursor->n - 1].type;

    /* if the parent is a compound data type then this is a compound member */
    is_compound_member = (((coda_hdf5_type *)cursor->stack[cursor->n - 2].type)->tag == tag_hdf5_compound_datatype);
    if (is_compound_member)
    {
        assert(cursor->n > 2);
        compound_index = cursor->stack[cursor->n - 1].index;
        compound_type = (coda_hdf5_compound_data_type *)cursor->stack[cursor->n - 2].type;
        /* the parent of the compound data type should be the dataset or attribute */
        array_depth = cursor->n - 3;
        datatype_to = compound_type->member_type[compound_index];
        /* note that datatype_to already has the filter to read the right element from the compound */
    }
    else
    {
        array_depth = cursor->n - 2;
        datatype_to = base_type->datatype_id;
    }
    assert(((coda_hdf5_type *)cursor->stack[array_depth].type)->tag == tag_hdf5_dataset);

    array_index = cursor->stack[array_depth + 1].index;

    if (!base_type->is_variable_string)
    {
        size = H5Tget_size(datatype_to);
    }

    dataset = (coda_hdf5_dataset *)cursor->stack[array_depth].type;

    if (dataset->definition->num_dims > 0)
    {
        hsize_t coord[CODA_MAX_NUM_DIMS];
        int i;

        for (i = dataset->definition->num_dims - 1; i >= 0; i--)
        {
            coord[i] = array_index % dataset->definition->dim[i];
            array_index = array_index / dataset->definition->dim[i];
        }
        if (H5Sselect_elements(dataset->dataspace_id, H5S_SELECT_SET, 1, (const hsize_t *)coord) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }
    }

    if (base_type->is_variable_string)
    {
        hsize_t buffer_size;

        if (H5Dvlen_get_buf_size(dataset->dataset_id, base_type->datatype_id, dataset->dataspace_id, &buffer_size) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }

        /* if the data type uses 'H5T_STR_NULLTERM', we need to subtract 1 to get the actual string length */
        if (buffer_size > 0 && H5Tget_strpad(base_type->datatype_id) == H5T_STR_NULLTERM)
        {
            buffer_size--;
        }

        size = (long)buffer_size;
    }

    buffer = malloc(size);
    if (buffer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)size, __FILE__, __LINE__);
        return -1;
    }

    mem_space_id = H5Screate_simple(0, NULL, NULL);
    if (mem_space_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        free(buffer);
        return -1;
    }
    if (base_type->is_variable_string)
    {
        char *vlen_ptr;

        if (H5Dread(dataset->dataset_id, datatype_to, mem_space_id, dataset->dataspace_id, H5P_DEFAULT, &vlen_ptr) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Sclose(mem_space_id);
            free(buffer);
            return -1;
        }
        memcpy(buffer, vlen_ptr, size);
        if (H5Dvlen_reclaim(datatype_to, mem_space_id, H5P_DEFAULT, &vlen_ptr) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Sclose(mem_space_id);
            free(buffer);
            return -1;
        }
    }
    else
    {
        if (H5Dread(dataset->dataset_id, datatype_to, mem_space_id, dataset->dataspace_id, H5P_DEFAULT, buffer) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Sclose(mem_space_id);
            free(buffer);
            return -1;
        }
    }
    H5Sclose(mem_space_id);
    if (H5Sselect_all(dataset->dataspace_id) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        free(buffer);
        return -1;
    }

    buffer_offset = buffer;

    /* convert buffer and store result in dst */

    if (base_type->definition->read_type == coda_native_type_string)
    {
        long num_chars;

        /* limit the number of returned characters */
        if (size > dst_size - 1)
        {
            num_chars = dst_size - 1;
        }
        else
        {
            num_chars = size;
        }
        memcpy(dst, buffer_offset, num_chars);
        ((char *)dst)[num_chars] = '\0';
    }
    else
    {
        hid_t from_type;
        int new_size;

        if (H5Tget_class(base_type->datatype_id) == H5T_ENUM)
        {
            /* convert the enumeration data to an integer value */
            from_type = H5Tget_super(base_type->datatype_id);
            if (from_type < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                free(buffer);
                return -1;
            }
        }
        else
        {
            from_type = H5Tcopy(base_type->datatype_id);
        }
        get_hdf5_type_and_size(base_type->definition->read_type, &datatype_to, &new_size);
        if (new_size > size)
        {
            /* use 'dst' for the conversion buffer */
            memcpy(dst, buffer_offset, size);
            if (H5Tconvert(from_type, datatype_to, 1, dst, NULL, H5P_DEFAULT) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                H5Tclose(from_type);
                free(buffer);
                return -1;
            }
        }
        else
        {
            /* use 'buffer' for the conversion buffer */
            if (H5Tconvert(from_type, datatype_to, 1, buffer_offset, NULL, H5P_DEFAULT) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                H5Tclose(from_type);
                free(buffer);
                return -1;
            }
            memcpy(dst, buffer_offset, new_size);
        }
        H5Tclose(from_type);
    }
    free(buffer);

    return 0;
}

int coda_hdf5_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_hdf5_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    return read_basic_type(cursor, dst, dst_size);
}

int coda_hdf5_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_double_array(const coda_cursor *cursor, double *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf5_cursor_read_int8_partial_array(const coda_cursor *cursor, long offset, long length, int8_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_uint8_partial_array(const coda_cursor *cursor, long offset, long length, uint8_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_int16_partial_array(const coda_cursor *cursor, long offset, long length, int16_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_uint16_partial_array(const coda_cursor *cursor, long offset, long length, uint16_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_int32_partial_array(const coda_cursor *cursor, long offset, long length, int32_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_uint32_partial_array(const coda_cursor *cursor, long offset, long length, uint32_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_int64_partial_array(const coda_cursor *cursor, long offset, long length, int64_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_uint64_partial_array(const coda_cursor *cursor, long offset, long length, uint64_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_float_partial_array(const coda_cursor *cursor, long offset, long length, float *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf5_cursor_read_double_partial_array(const coda_cursor *cursor, long offset, long length, double *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}
