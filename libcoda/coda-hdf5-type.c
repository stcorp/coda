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

#include "coda-hdf5-internal.h"

int coda_hdf5_type_get_read_type(const coda_Type *type, coda_native_type *read_type)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_basic_datatype:
            *read_type = ((coda_hdf5BasicDataType *)type)->read_type;
            break;
        case tag_hdf5_compound_datatype:
        case tag_hdf5_attribute:
        case tag_hdf5_attribute_record:
        case tag_hdf5_group:
        case tag_hdf5_dataset:
            *read_type = coda_native_type_not_available;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_type_get_string_length(const coda_Type *type, long *length)
{
    if (((coda_hdf5BasicDataType *)type)->is_variable_string)
    {
        *length = -1;
    }
    else
    {
        *length = H5Tget_size(((coda_hdf5BasicDataType *)type)->datatype_id);
    }

    return 0;
}

int coda_hdf5_type_get_num_record_fields(const coda_Type *type, long *num_fields)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_compound_datatype:
            *num_fields = ((coda_hdf5CompoundDataType *)type)->num_members;
            break;
        case tag_hdf5_attribute_record:
            *num_fields = ((coda_hdf5AttributeRecord *)type)->num_attributes;
            break;
        case tag_hdf5_group:
            *num_fields = (int)((coda_hdf5Group *)type)->num_objects;
            break;
        default:
            assert(0);
            exit(1);
    }
    return 0;
}

int coda_hdf5_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index)
{
    hashtable *hash_data;

    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_compound_datatype:
            hash_data = ((coda_hdf5CompoundDataType *)type)->hash_data;
            break;
        case tag_hdf5_attribute_record:
            hash_data = ((coda_hdf5AttributeRecord *)type)->hash_data;
            break;
        case tag_hdf5_group:
            hash_data = ((coda_hdf5Group *)type)->hash_data;
            break;
        default:
            assert(0);
            exit(1);
    }

    *index = hashtable_get_index_from_name(hash_data, name);
    if (*index >= 0)
    {
        return 0;
    }

    coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
    return -1;
}

int coda_hdf5_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_compound_datatype:
            if (index < 0 || index >= ((coda_hdf5CompoundDataType *)type)->num_members)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf5CompoundDataType *)type)->num_members, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf5CompoundDataType *)type)->member[index];
            break;
        case tag_hdf5_attribute_record:
            if (index < 0 || index >= ((coda_hdf5AttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf5AttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf5AttributeRecord *)type)->attribute[index];
            break;
        case tag_hdf5_group:
            if (index < 0 || (hsize_t)index >= ((coda_hdf5Group *)type)->num_objects)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               (long)((coda_hdf5Group *)type)->num_objects, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf5Group *)type)->object[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_type_get_record_field_name(const coda_Type *type, long index, const char **name)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_compound_datatype:
            if (index < 0 || index >= ((coda_hdf5CompoundDataType *)type)->num_members)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf5CompoundDataType *)type)->num_members, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf5CompoundDataType *)type)->member_name[index];
            break;
        case tag_hdf5_attribute_record:
            if (index < 0 || index >= ((coda_hdf5AttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf5AttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf5AttributeRecord *)type)->attribute_name[index];
            break;
        case tag_hdf5_group:
            if (index < 0 || (hsize_t)index >= ((coda_hdf5Group *)type)->num_objects)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               (long)((coda_hdf5Group *)type)->num_objects, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf5Group *)type)->object_name[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_type_get_array_num_dims(const coda_Type *type, int *num_dims)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_attribute:
            *num_dims = ((coda_hdf5Attribute *)type)->ndims;
            break;
        case tag_hdf5_dataset:
            *num_dims = ((coda_hdf5DataSet *)type)->ndims;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[])
{
    int i;

    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_attribute:
            *num_dims = ((coda_hdf5Attribute *)type)->ndims;
            for (i = 0; i < ((coda_hdf5Attribute *)type)->ndims; i++)
            {
                dim[i] = (long)((coda_hdf5Attribute *)type)->dims[i];
            }
            break;
        case tag_hdf5_dataset:
            *num_dims = ((coda_hdf5DataSet *)type)->ndims;
            for (i = 0; i < ((coda_hdf5DataSet *)type)->ndims; i++)
            {
                dim[i] = (long)((coda_hdf5DataSet *)type)->dims[i];
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf5_type_get_array_base_type(const coda_Type *type, coda_Type **base_type)
{
    switch (((coda_hdf5Type *)type)->tag)
    {
        case tag_hdf5_attribute:
            *base_type = (coda_Type *)((coda_hdf5Attribute *)type)->base_type;
            break;
        case tag_hdf5_dataset:
            *base_type = (coda_Type *)((coda_hdf5DataSet *)type)->base_type;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}
