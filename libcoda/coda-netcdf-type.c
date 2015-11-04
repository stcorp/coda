/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#include "coda-netcdf-internal.h"

int coda_netcdf_type_get_read_type(const coda_type *type, coda_native_type *read_type)
{
    switch (((coda_netcdf_type *)type)->tag)
    {
        case tag_netcdf_basic_type:
            if (coda_option_perform_conversions && ((coda_netcdf_basic_type *)type)->has_conversion)
            {
                *read_type = coda_native_type_double;
            }
            else
            {
                *read_type = ((coda_netcdf_basic_type *)type)->read_type;
            }
            break;
        case tag_netcdf_root:
        case tag_netcdf_array:
        case tag_netcdf_attribute_record:
            *read_type = coda_native_type_not_available;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_netcdf_type_get_string_length(const coda_type *type, long *length)
{
    *length = ((coda_netcdf_basic_type *)type)->byte_size;
    return 0;
}

int coda_netcdf_type_get_num_record_fields(const coda_type *type, long *num_fields)
{
    switch (((coda_netcdf_type *)type)->tag)
    {
        case tag_netcdf_root:
            *num_fields = ((coda_netcdf_root *)type)->num_variables;
            break;
        case tag_netcdf_attribute_record:
            *num_fields = ((coda_netcdf_attribute_record *)type)->num_attributes;
            break;
        default:
            assert(0);
            exit(1);
    }
    return 0;
}

int coda_netcdf_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index)
{
    hashtable *hash_data;

    switch (((coda_netcdf_type *)type)->tag)
    {
        case tag_netcdf_root:
            hash_data = ((coda_netcdf_root *)type)->hash_data;
            break;
        case tag_netcdf_attribute_record:
            hash_data = ((coda_netcdf_attribute_record *)type)->hash_data;
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

int coda_netcdf_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type)
{
    switch (((coda_netcdf_type *)type)->tag)
    {
        case tag_netcdf_root:
            if (index < 0 || index >= ((coda_netcdf_root *)type)->num_variables)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_netcdf_root *)type)->num_variables, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_type *)((coda_netcdf_root *)type)->variable[index];
            break;
        case tag_netcdf_attribute_record:
            if (index < 0 || index >= ((coda_netcdf_attribute_record *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_netcdf_attribute_record *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_type *)((coda_netcdf_attribute_record *)type)->attribute[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_netcdf_type_get_record_field_name(const coda_type *type, long index, const char **name)
{
    switch (((coda_netcdf_type *)type)->tag)
    {
        case tag_netcdf_root:
            if (index < 0 || index >= ((coda_netcdf_root *)type)->num_variables)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_netcdf_root *)type)->num_variables, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_netcdf_root *)type)->variable_name[index];
            break;
        case tag_netcdf_attribute_record:
            if (index < 0 || index >= ((coda_netcdf_attribute_record *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_netcdf_attribute_record *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_netcdf_attribute_record *)type)->attribute_name[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_netcdf_type_get_array_num_dims(const coda_type *type, int *num_dims)
{
    *num_dims = ((coda_netcdf_array *)type)->num_dims;
    return 0;
}

int coda_netcdf_type_get_array_dim(const coda_type *type, int *num_dims, long dim[])
{
    int i;

    *num_dims = ((coda_netcdf_array *)type)->num_dims;
    for (i = 0; i < *num_dims; i++)
    {
        dim[i] = ((coda_netcdf_array *)type)->dim[i];
    }

    return 0;
}

int coda_netcdf_type_get_array_base_type(const coda_type *type, coda_type **base_type)
{
    *base_type = (coda_type *)((coda_netcdf_array *)type)->base_type;
    return 0;
}
