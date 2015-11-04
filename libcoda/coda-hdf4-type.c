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

#include "coda-hdf4-internal.h"

int coda_hdf4_type_get_read_type(const coda_Type *type, coda_native_type *read_type)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_basic_type:
            if (coda_option_perform_conversions && ((coda_hdf4BasicType *)type)->has_conversion)
            {
                *read_type = coda_native_type_double;
            }
            else
            {
                *read_type = ((coda_hdf4BasicType *)type)->read_type;
            }
            break;
        case tag_hdf4_basic_type_array:
        case tag_hdf4_attributes:
        case tag_hdf4_file_attributes:
        case tag_hdf4_root:
        case tag_hdf4_GRImage:
        case tag_hdf4_SDS:
        case tag_hdf4_Vdata:
        case tag_hdf4_Vdata_field:
        case tag_hdf4_Vgroup:
            *read_type = coda_native_type_not_available;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_type_get_string_length(const coda_Type *type, long *length)
{
    /* HDF4 does not support strings as basic types, only char data. We will therefore always return a size of 1 */
    assert(((coda_hdf4Type *)type)->tag == tag_hdf4_basic_type &&
           ((coda_hdf4BasicType *)type)->read_type == coda_native_type_char);
    *length = 1;

    return 0;
}

int coda_hdf4_type_get_num_record_fields(const coda_Type *type, long *num_fields)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_root:
            *num_fields = ((coda_hdf4Root *)type)->num_entries;
            break;
        case tag_hdf4_attributes:
            *num_fields = ((coda_hdf4Attributes *)type)->num_attributes;
            break;
        case tag_hdf4_file_attributes:
            *num_fields = ((coda_hdf4FileAttributes *)type)->num_attributes;
            break;
        case tag_hdf4_Vdata:
            *num_fields = ((coda_hdf4Vdata *)type)->num_fields;
            break;
        case tag_hdf4_Vgroup:
            *num_fields = ((coda_hdf4Vgroup *)type)->num_entries;
            break;
        default:
            assert(0);
            exit(1);
    }
    return 0;
}

int coda_hdf4_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index)
{
    hashtable *hash_data;

    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_root:
            hash_data = ((coda_hdf4Root *)type)->hash_data;
            break;
        case tag_hdf4_attributes:
            hash_data = ((coda_hdf4Attributes *)type)->hash_data;
            break;
        case tag_hdf4_file_attributes:
            hash_data = ((coda_hdf4FileAttributes *)type)->hash_data;
            break;
        case tag_hdf4_Vdata:
            hash_data = ((coda_hdf4Vdata *)type)->hash_data;
            break;
        case tag_hdf4_Vgroup:
            hash_data = ((coda_hdf4Vgroup *)type)->hash_data;
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

int coda_hdf4_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_root:
            if (index < 0 || index >= ((coda_hdf4Root *)type)->num_entries)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Root *)type)->num_entries, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf4Root *)type)->entry[index];
            break;
        case tag_hdf4_attributes:
            if (index < 0 || index >= ((coda_hdf4Attributes *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Attributes *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf4Attributes *)type)->attribute[index];
            break;
        case tag_hdf4_file_attributes:
            if (index < 0 || index >= ((coda_hdf4FileAttributes *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4FileAttributes *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf4FileAttributes *)type)->attribute[index];
            break;
        case tag_hdf4_Vdata:
            if (index < 0 || index >= ((coda_hdf4Vdata *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Vdata *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf4Vdata *)type)->field[index];
            break;
        case tag_hdf4_Vgroup:
            if (index < 0 || index >= ((coda_hdf4Vgroup *)type)->num_entries)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Vgroup *)type)->num_entries, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_hdf4Vgroup *)type)->entry[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_type_get_record_field_name(const coda_Type *type, long index, const char **name)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_root:
            if (index < 0 || index >= ((coda_hdf4Root *)type)->num_entries)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Root *)type)->num_entries, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf4Root *)type)->entry_name[index];
            break;
        case tag_hdf4_attributes:
            if (index < 0 || index >= ((coda_hdf4Attributes *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Attributes *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf4Attributes *)type)->attribute_name[index];
            break;
        case tag_hdf4_file_attributes:
            if (index < 0 || index >= ((coda_hdf4FileAttributes *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4FileAttributes *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf4FileAttributes *)type)->attribute_name[index];
            break;
        case tag_hdf4_Vdata:
            if (index < 0 || index >= ((coda_hdf4Vdata *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Vdata *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf4Vdata *)type)->field_name[index];
            break;
        case tag_hdf4_Vgroup:
            if (index < 0 || index >= ((coda_hdf4Vgroup *)type)->num_entries)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_hdf4Vgroup *)type)->num_entries, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_hdf4Vgroup *)type)->entry_name[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_type_get_array_num_dims(const coda_Type *type, int *num_dims)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_basic_type_array:
            *num_dims = 1;
            break;
        case tag_hdf4_GRImage:
            if (((coda_hdf4GRImage *)type)->ncomp != 1)
            {
                *num_dims = 3;
            }
            else
            {
                *num_dims = 2;
            }
            break;
        case tag_hdf4_SDS:
            *num_dims = ((coda_hdf4SDS *)type)->rank;
            break;
        case tag_hdf4_Vdata_field:
            if (((coda_hdf4VdataField *)type)->order > 1)
            {
                *num_dims = 2;
            }
            else
            {
                *num_dims = 1;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[])
{
    int i;

    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_basic_type_array:
            *num_dims = 1;
            dim[0] = ((coda_hdf4BasicTypeArray *)type)->count;
            break;
        case tag_hdf4_GRImage:
            /* The C interface to GRImage data uses fortran array ordering, so we swap the dimensions */
            dim[0] = ((coda_hdf4GRImage *)type)->dim_sizes[1];
            dim[1] = ((coda_hdf4GRImage *)type)->dim_sizes[0];
            if (((coda_hdf4GRImage *)type)->ncomp != 1)
            {
                *num_dims = 3;
                dim[2] = ((coda_hdf4GRImage *)type)->ncomp;
            }
            else
            {
                *num_dims = 2;
            }
            break;
        case tag_hdf4_SDS:
            *num_dims = ((coda_hdf4SDS *)type)->rank;
            for (i = 0; i < ((coda_hdf4SDS *)type)->rank; i++)
            {
                dim[i] = ((coda_hdf4SDS *)type)->dimsizes[i];
            }
            break;
        case tag_hdf4_Vdata_field:
            if (((coda_hdf4VdataField *)type)->order > 1)
            {
                *num_dims = 2;
                dim[1] = ((coda_hdf4VdataField *)type)->order;
            }
            else
            {
                *num_dims = 1;
            }
            dim[0] = ((coda_hdf4VdataField *)type)->num_records;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_type_get_array_base_type(const coda_Type *type, coda_Type **base_type)
{
    switch (((coda_hdf4Type *)type)->tag)
    {
        case tag_hdf4_basic_type_array:
            *base_type = (coda_Type *)((coda_hdf4BasicTypeArray *)type)->basic_type;
            break;
        case tag_hdf4_GRImage:
            *base_type = (coda_Type *)((coda_hdf4GRImage *)type)->basic_type;
            break;
        case tag_hdf4_SDS:
            *base_type = (coda_Type *)((coda_hdf4SDS *)type)->basic_type;
            break;
        case tag_hdf4_Vdata_field:
            *base_type = (coda_Type *)((coda_hdf4VdataField *)type)->basic_type;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}
