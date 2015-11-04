/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
 *
 * This file is part of CODA.
 *
 * CODA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CODA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CODA; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

int coda_hdf5_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    cursor->pf = pf;
    cursor->n = 1;
    cursor->stack[0].type = pf->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *field_type;

    if (coda_hdf5_type_get_record_field_type((coda_Type *)cursor->stack[cursor->n - 1].type, index, &field_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)field_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */
    return 0;
}

int coda_hdf5_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    cursor->n--;
    if (coda_hdf5_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_hdf5_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    coda_Type *base_type;
    long offset_elements;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long i;

    if (coda_hdf5_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, &num_dims, dim) != 0)
    {
        return -1;
    }

    /* check the number of dimensions */
    if (num_subs != num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d) (%s:%u)", num_subs,
                       num_dims, __FILE__, __LINE__);
        return -1;
    }

    /* check the dimensions... */
    offset_elements = 0;
    for (i = 0; i < num_dims; i++)
    {
        if (subs[i] < 0 || subs[i] >= dim[i])
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           subs[i], dim[i], __FILE__, __LINE__);
            return -1;
        }
        if (i > 0)
        {
            offset_elements *= dim[i];
        }
        offset_elements += subs[i];
    }

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *base_type;

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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_goto_next_array_element(coda_Cursor *cursor)
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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    cursor->stack[cursor->n - 1].index++;

    return 0;
}

int coda_hdf5_cursor_goto_attributes(coda_Cursor *cursor)
{
    coda_hdf5Type *type;

    type = (coda_hdf5Type *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_hdf5_basic_datatype:
        case tag_hdf5_compound_datatype:
        case tag_hdf5_attribute:
        case tag_hdf5_attribute_record:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_hdf5_empty_attribute_record();
            break;
        case tag_hdf5_group:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf5Group *)type)->attributes;
            break;
        case tag_hdf5_dataset:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf5DataSet *)type)->attributes;
            break;
        default:
            assert(0);
            exit(1);
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF5 backend */

    return 0;
}

int coda_hdf5_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    coda_hdf5Type *type;

    type = (coda_hdf5Type *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_hdf5_basic_datatype:
            *num_elements = 1;
            break;
        case tag_hdf5_compound_datatype:
            *num_elements = (long)((coda_hdf5CompoundDataType *)type)->num_members;
            break;
        case tag_hdf5_attribute:
            *num_elements = (long)((coda_hdf5Attribute *)type)->num_elements;
            break;
        case tag_hdf5_attribute_record:
            *num_elements = ((coda_hdf5AttributeRecord *)type)->num_attributes;
            break;
        case tag_hdf5_group:
            *num_elements = (long)((coda_hdf5Group *)type)->num_objects;
            break;
        case tag_hdf5_dataset:
            *num_elements = (long)((coda_hdf5DataSet *)type)->num_elements;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_cursor_get_string_length(const coda_Cursor *cursor, long *length)
{
    coda_hdf5BasicDataType *base_type;

    base_type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    if (base_type->is_variable_string)
    {
        coda_hdf5DataSet *dataset;
        long array_index;
        hsize_t size;

        /* in CODA, variable strings should only exist when the parent is a dataset */
        dataset = (coda_hdf5DataSet *)cursor->stack[cursor->n - 2].type;
        assert(dataset->tag == tag_hdf5_dataset);
        array_index = cursor->stack[cursor->n - 1].index;
        if (dataset->ndims > 0)
        {
            hsize_t coord[CODA_MAX_NUM_DIMS];
            int i;

            for (i = dataset->ndims - 1; i >= 0; i--)
            {
                coord[i] = array_index % dataset->dims[i];
                array_index = array_index / (int)dataset->dims[i];
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

        return 0;
    }
    return coda_hdf5_type_get_string_length((coda_Type *)cursor->stack[cursor->n - 1].type, length);
}

int coda_hdf5_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    return coda_hdf5_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, num_dims, dim);
}

/* since hdf5 also provides some type conversions of its own we pass a value for 'from_type' that matches
 * 'to_type' as closely as possible. The 'from_type' parameter does therefore not have to match the real storage
 * type of the data.
 */
static int coda_hdf5_read_array(const coda_Cursor *cursor, void *dst, coda_native_type from_type,
                                coda_native_type to_type, coda_array_ordering array_ordering)
{
    coda_hdf5DataType *base_type;
    hid_t mem_type_id;
    char *buffer;
    int conversion_needed;
    long num_elements;
    int element_to_size;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long i;

    assert(from_type != coda_native_type_string);

    if (coda_hdf5_cursor_get_num_elements(cursor, &num_elements) != 0)
    {
        return -1;
    }
    if (num_elements <= 0)
    {
        /* no data to be read */
        return 0;
    }
    if (coda_hdf5_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, &num_dims, dim) != 0)
    {
        return -1;
    }

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag == tag_hdf5_attribute)
    {
        base_type = ((coda_hdf5Attribute *)cursor->stack[cursor->n - 1].type)->base_type;
    }
    else
    {
        base_type = ((coda_hdf5DataSet *)cursor->stack[cursor->n - 1].type)->base_type;
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
        get_hdf5_type_and_size(from_type, &mem_type_id, &element_to_size);
        /* we make a copy to ease the cleanup process */
        mem_type_id = H5Tcopy(mem_type_id);
    }

    conversion_needed = (from_type != to_type || (num_dims > 1 && array_ordering != coda_array_ordering_c));
    if (conversion_needed)
    {
        int to_size;

        to_size = element_to_size * num_elements;
        buffer = malloc(to_size);
        if (buffer == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)to_size, __FILE__, __LINE__);
            H5Tclose(mem_type_id);
            return -1;
        }
    }
    else
    {
        buffer = (char *)dst;
    }

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag == tag_hdf5_attribute)
    {
        if (H5Aread(((coda_hdf5Attribute *)cursor->stack[cursor->n - 1].type)->attribute_id, mem_type_id, buffer) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            if (conversion_needed)
            {
                free(buffer);
            }
            H5Tclose(mem_type_id);
            return -1;
        }
    }
    else
    {
        if (H5Dread(((coda_hdf5DataSet *)cursor->stack[cursor->n - 1].type)->dataset_id, mem_type_id, H5S_ALL, H5S_ALL,
                    H5P_DEFAULT, buffer) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            if (conversion_needed)
            {
                free(buffer);
            }
            H5Tclose(mem_type_id);
            return -1;
        }
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
            if (conversion_needed)
            {
                free(buffer);
            }
            return -1;
        }
        get_hdf5_type_and_size(from_type, &mem_type_id, &native_element_size);
        assert(native_element_size == element_to_size);
        if (H5Tconvert(super, mem_type_id, num_elements, buffer, NULL, H5P_DEFAULT) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            if (conversion_needed)
            {
                free(buffer);
            }
            return -1;
        }
    }

    if (!conversion_needed)
    {
        return 0;
    }

    if (num_dims <= 1 || array_ordering == coda_array_ordering_c)
    {
        /* requested array ordering is the same as that of the source */

        for (i = 0; i < num_elements; i++)
        {
            switch (to_type)
            {
                case coda_native_type_float:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((float *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((float *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((float *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((float *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_int32:
                            ((float *)dst)[i] = (float)((int32_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((float *)dst)[i] = (float)((uint32_t *)buffer)[i];
                            break;
                        case coda_native_type_int64:
                            ((float *)dst)[i] = (float)((int64_t *)buffer)[i];
                            break;
                        case coda_native_type_uint64:
                            ((float *)dst)[i] = (float)(int64_t)((uint64_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_double:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((double *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((double *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((double *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((double *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_int32:
                            ((double *)dst)[i] = ((int32_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((double *)dst)[i] = ((uint32_t *)buffer)[i];
                            break;
                        case coda_native_type_int64:
                            ((double *)dst)[i] = (double)((int64_t *)buffer)[i];
                            break;
                        case coda_native_type_uint64:
                            ((double *)dst)[i] = (double)(int64_t)((uint64_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* requested array ordering differs from that of the source */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        assert(num_elements == incr[num_dims]);

        c_index = 0;
        fortran_index = 0;
        for (;;)
        {
            do
            {
                switch (to_type)
                {
                    case coda_native_type_int8:
                        assert(from_type == coda_native_type_int8);
                        ((int8_t *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                        break;
                    case coda_native_type_uint8:
                        assert(from_type == coda_native_type_uint8);
                        ((uint8_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                        break;
                    case coda_native_type_int16:
                        assert(from_type == coda_native_type_int16);
                        ((int16_t *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                        break;
                    case coda_native_type_uint16:
                        assert(from_type == coda_native_type_uint16);
                        ((uint16_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                        break;
                    case coda_native_type_int32:
                        assert(from_type == coda_native_type_int32);
                        ((int32_t *)dst)[fortran_index] = ((int32_t *)buffer)[c_index];
                        break;
                    case coda_native_type_uint32:
                        assert(from_type == coda_native_type_uint32);
                        ((uint32_t *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                        break;
                    case coda_native_type_int64:
                        assert(from_type == coda_native_type_int64);
                        ((int64_t *)dst)[fortran_index] = ((int64_t *)buffer)[c_index];
                        break;
                    case coda_native_type_uint64:
                        assert(from_type == coda_native_type_uint64);
                        ((uint64_t *)dst)[fortran_index] = ((uint64_t *)buffer)[c_index];
                        break;
                    case coda_native_type_float:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((float *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((float *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((float *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((float *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((float *)dst)[fortran_index] = (float)((int32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((float *)dst)[fortran_index] = (float)((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int64:
                                ((float *)dst)[fortran_index] = (float)((int64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint64:
                                ((float *)dst)[fortran_index] = (float)(int64_t)((uint64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_float:
                                ((float *)dst)[fortran_index] = ((float *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_double:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((double *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((double *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((double *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((double *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((double *)dst)[fortran_index] = ((int32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((double *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int64:
                                ((double *)dst)[fortran_index] = (double)((int64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint64:
                                ((double *)dst)[fortran_index] = (double)(int64_t)((uint64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_double:
                                ((double *)dst)[fortran_index] = ((double *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    default:
                        assert(0);
                        exit(1);
                }

                c_index++;
                fortran_index += increment;

            } while (fortran_index < num_elements);

            if (c_index == num_elements)
            {
                break;
            }

            fortran_index += incr[num_dims - 2] - incr[num_dims];
            i = num_dims - 3;
            while (i >= 0 && fortran_index >= incr[i + 2])
            {
                fortran_index += incr[i] - incr[i + 2];
                i--;
            }
        }
    }

    free(buffer);
    return 0;
}

static int coda_hdf5_read_basic_type(const coda_Cursor *cursor, coda_native_type to_type, void *dst, long dst_size)
{
    coda_hdf5BasicDataType *base_type;
    coda_hdf5CompoundDataType *compound_type = NULL;
    hid_t datatype_to;
    char *buffer = NULL;
    char *buffer_offset;        /* position in buffer where our value is stored */
    int is_compound_member;
    int array_depth;
    long array_index;
    long compound_index = -1;
    long size = -1;

    assert(cursor->n > 1);
    base_type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;

    /* if the parent is a compound data type then this is a compound member */
    is_compound_member = (((coda_hdf5Type *)cursor->stack[cursor->n - 2].type)->tag == tag_hdf5_compound_datatype);
    if (is_compound_member)
    {
        assert(cursor->n > 2);
        compound_index = cursor->stack[cursor->n - 1].index;
        compound_type = (coda_hdf5CompoundDataType *)cursor->stack[cursor->n - 2].type;
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
    assert(((coda_hdf5Type *)cursor->stack[array_depth].type)->tag == tag_hdf5_attribute ||
           ((coda_hdf5Type *)cursor->stack[array_depth].type)->tag == tag_hdf5_dataset);

    array_index = cursor->stack[array_depth + 1].index;

    if (!base_type->is_variable_string)
    {
        size = H5Tget_size(datatype_to);
    }

    if (((coda_hdf5Type *)cursor->stack[array_depth].type)->tag == tag_hdf5_attribute)
    {
        coda_hdf5Attribute *attribute;

        /* for attributes only the full array can be read (i.e. there is no data space that can be set) */

        attribute = (coda_hdf5Attribute *)cursor->stack[array_depth].type;

        buffer = malloc((size_t)attribute->num_elements * size);
        if (buffer == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)attribute->num_elements * size, __FILE__, __LINE__);
            return -1;
        }

        if (H5Aread(attribute->attribute_id, datatype_to, buffer) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            free(buffer);
            return -1;
        }

        buffer_offset = &buffer[array_index * size];
    }
    else
    {
        coda_hdf5DataSet *dataset;
        hid_t mem_space_id;

        dataset = (coda_hdf5DataSet *)cursor->stack[array_depth].type;

        if (dataset->ndims > 0)
        {
            hsize_t coord[CODA_MAX_NUM_DIMS];
            int i;

            for (i = dataset->ndims - 1; i >= 0; i--)
            {
                coord[i] = array_index % dataset->dims[i];
                array_index = array_index / (int)dataset->dims[i];
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

            if (H5Dvlen_get_buf_size(dataset->dataset_id, base_type->datatype_id, dataset->dataspace_id,
                                     &buffer_size) < 0)
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

            if (H5Dread(dataset->dataset_id, datatype_to, mem_space_id, dataset->dataspace_id, H5P_DEFAULT,
                        &vlen_ptr) < 0)
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
    }

    /* convert buffer to to_type and store result in dst */

    if (base_type->read_type == coda_native_type_string)
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
            from_type = base_type->datatype_id;
        }
        get_hdf5_type_and_size(to_type, &datatype_to, &new_size);
        if (new_size > size)
        {
            /* use 'dst' for the conversion buffer */
            memcpy(dst, buffer_offset, size);
            if (H5Tconvert(from_type, datatype_to, 1, dst, NULL, H5P_DEFAULT) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
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
                free(buffer);
                return -1;
            }
            memcpy(dst, buffer_offset, new_size);
        }
    }
    free(buffer);

    return 0;
}

int coda_hdf5_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_int8, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_uint8, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_int16, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_uint16, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_int32, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_uint32, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_int64, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_uint64, dst, -1);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_hdf5_cursor_read_float(const coda_Cursor *cursor, float *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int8, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint8, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int16, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint16, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int32, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint32, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int64, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint64, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (float)(int64_t)value;
            }
            break;
        case coda_native_type_float:
        case coda_native_type_double:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_float, dst, -1);
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf5_cursor_read_double(const coda_Cursor *cursor, double *dst)
{
    coda_hdf5BasicDataType *type;

    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
        return -1;
    }

    type = (coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int8, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint8, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int16, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint16, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int32, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint32, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_int64, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (coda_hdf5_read_basic_type(cursor, coda_native_type_uint64, &value, -1) != 0)
                {
                    return -1;
                }
                *dst = (double)(int64_t)value;
            }
            break;
        case coda_native_type_float:
        case coda_native_type_double:
            return coda_hdf5_read_basic_type(cursor, coda_native_type_double, dst, -1);
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf5_cursor_read_char(const coda_Cursor *cursor, char *dst)
{
    cursor = cursor;
    dst = dst;

    /* HDF5 does not have char types */
    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
    return -1;
}

int coda_hdf5_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size)
{
    if (((coda_hdf5Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a string data type");
        return -1;
    }

    if (((coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type)->read_type != coda_native_type_string)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a string data type",
                       coda_type_get_native_type_name(((coda_hdf5BasicDataType *)cursor->stack[cursor->n - 1].type)->
                                                      read_type));
        return -1;
    }

    return coda_hdf5_read_basic_type(cursor, coda_native_type_string, dst, dst_size);
}

int coda_hdf5_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int8, coda_native_type_int8, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_uint8, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int16, coda_native_type_int16, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_uint16, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int32, coda_native_type_int32, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_uint32, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int64, coda_native_type_int64, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_uint64, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int8, coda_native_type_float, array_ordering);
        case coda_native_type_uint8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_float, array_ordering);
        case coda_native_type_int16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int16, coda_native_type_float, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_float, array_ordering);
        case coda_native_type_int32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int32, coda_native_type_float, array_ordering);
        case coda_native_type_uint32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_float, array_ordering);
        case coda_native_type_int64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int64, coda_native_type_float, array_ordering);
        case coda_native_type_uint64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_float, array_ordering);
        case coda_native_type_float:
        case coda_native_type_double:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_float, coda_native_type_float, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_double_array(const coda_Cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf5_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    if (((coda_hdf5Type *)base_type)->tag != tag_hdf5_basic_datatype)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
        return -1;
    }

    switch (((coda_hdf5BasicDataType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int8, coda_native_type_double, array_ordering);
        case coda_native_type_uint8:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_double, array_ordering);
        case coda_native_type_int16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int16, coda_native_type_double, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_double, array_ordering);
        case coda_native_type_int32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int32, coda_native_type_double, array_ordering);
        case coda_native_type_uint32:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_double, array_ordering);
        case coda_native_type_int64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_int64, coda_native_type_double, array_ordering);
        case coda_native_type_uint64:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_double, array_ordering);
        case coda_native_type_float:
        case coda_native_type_double:
            return coda_hdf5_read_array(cursor, dst, coda_native_type_double, coda_native_type_double, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                   coda_type_get_native_type_name(((coda_hdf5BasicDataType *)base_type)->read_type));
    return -1;
}

int coda_hdf5_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    cursor = cursor;
    dst = dst;
    array_ordering = array_ordering;

    /* HDF5 does not have single char types */
    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
    return -1;
}
