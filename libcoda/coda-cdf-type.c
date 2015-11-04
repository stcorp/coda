/*
 * Copyright (C) 2007-2012 S[&]T, The Netherlands.
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

#include "coda-cdf-internal.h"

#include <assert.h>
#include <stdlib.h>

void coda_cdf_type_delete(coda_dynamic_type *type)
{
    assert(type != NULL);
    assert(type->backend == coda_backend_cdf);

    if (type->definition != NULL)
    {
        if (type->definition->type_class == coda_array_class)
        {
            coda_cdf_variable *variable = (coda_cdf_variable *)type;

            if (variable->attributes != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)variable->attributes);
            }
            if (variable->base_type != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)variable->base_type);
            }
            if (variable->offset != NULL)
            {
                free(variable->offset);
            }
            if (variable->data != NULL)
            {
                free(variable->data);
            }
        }
        else if (type->definition->type_class == coda_special_class)
        {
            if (((coda_cdf_time *)type)->base_type != NULL)
            {
                coda_dynamic_type_delete(((coda_cdf_time *)type)->base_type);
            }
        }
        coda_type_release(type->definition);
    }
    free(type);
}

static coda_cdf_type *basic_type_new(int32_t data_type, int32_t num_elements)
{
    coda_type_class type_class;
    coda_native_type native_type;
    coda_type *definition;
    coda_cdf_type *type;
    int64_t byte_size;

    switch (data_type)
    {
        case 1:        /* INT1 */
        case 41:       /* BYTE */
            type_class = coda_integer_class;
            native_type = coda_native_type_int8;
            byte_size = 1;
            break;
        case 2:        /* INT2 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int16;
            byte_size = 2;
            break;
        case 4:        /* INT4 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int32;
            byte_size = 4;
            break;
        case 8:        /* INT8 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int64;
            byte_size = 8;
            break;
        case 11:       /* UINT1 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint8;
            byte_size = 1;
            break;
        case 12:       /* UINT2 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint16;
            byte_size = 2;
            break;
        case 14:       /* UINT4 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint32;
            byte_size = 4;
            break;
        case 21:       /* REAL4 */
        case 44:       /* FLOAT */
            type_class = coda_real_class;
            native_type = coda_native_type_float;
            byte_size = 4;
            break;
        case 22:       /* REAL8 */
        case 45:       /* DOUBLE */
            type_class = coda_real_class;
            native_type = coda_native_type_double;
            byte_size = 8;
            break;
        case 51:       /* CHAR */
        case 52:       /* UCHAR */
            type_class = coda_text_class;
            if (num_elements == 1)
            {
                native_type = coda_native_type_char;
            }
            else
            {
                native_type = coda_native_type_string;
            }
            byte_size = num_elements;
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid CDF data type (%d)", data_type);
            return NULL;
    }
    switch (type_class)
    {
        case coda_integer_class:
        case coda_real_class:
            definition = (coda_type *)coda_type_number_new(coda_format_cdf, type_class);
            break;
        case coda_text_class:
            definition = (coda_type *)coda_type_text_new(coda_format_cdf);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (definition == NULL)
    {
        return NULL;
    }
    if (coda_type_set_read_type(definition, native_type) != 0)
    {
        coda_type_release(definition);
        return NULL;
    }
    if (coda_type_set_byte_size(definition, byte_size) != 0)
    {
        coda_type_release(definition);
        return NULL;
    }
    type = (coda_cdf_type *)malloc(sizeof(coda_cdf_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_cdf_type), __FILE__, __LINE__);
        coda_type_release(definition);
        return NULL;
    }
    type->backend = coda_backend_cdf;
    type->definition = definition;
    return type;
}

static coda_cdf_time *time_type_new(int32_t data_type)
{
    coda_cdf_time *type;
    int32_t base_data_type;

    assert(data_type == 31 || data_type == 33);

    type = (coda_cdf_time *)malloc(sizeof(coda_cdf_time));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_cdf_time), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_cdf;
    type->definition = NULL;
    type->base_type = NULL;
    type->data_type = data_type;

    if (data_type == 31)
    {
        /* EPOCH */
        base_data_type = 45;
    }
    else
    {
        /* TIME_TT2000 */
        base_data_type = 8;
    }
    type->definition = coda_type_time_new(coda_format_cdf, NULL);
    if (type->definition == NULL)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->base_type = (coda_dynamic_type *)basic_type_new(base_data_type, 1);
    if (type->base_type == NULL)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_time_set_base_type(type->definition, type->base_type->definition) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    return type;
}

coda_cdf_variable *coda_cdf_variable_new(int32_t data_type, int32_t max_rec, int32_t rec_varys, int32_t num_dims,
                                         int32_t dim[CODA_MAX_NUM_DIMS], int32_t dim_varys[CODA_MAX_NUM_DIMS],
                                         coda_array_ordering array_ordering, int32_t num_elements,
                                         int sparse_rec_method)
{
    coda_cdf_variable *type;
    int i;

    assert(rec_varys || max_rec == 0);

    type = (coda_cdf_variable *)malloc(sizeof(coda_cdf_variable));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_cdf_variable), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_cdf;
    type->definition = NULL;
    type->attributes = NULL;
    type->base_type = NULL;
    type->num_records = 1;
    type->num_values_per_record = 1;
    type->value_size = -1;
    type->sparse_rec_method = sparse_rec_method;
    type->offset = NULL;
    type->data = NULL;

    if (data_type == 31 || data_type == 33)
    {
        type->base_type = (coda_cdf_type *)time_type_new(data_type);
    }
    else
    {
        type->base_type = basic_type_new(data_type, num_elements);
    }
    if (type->base_type == NULL)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->definition = coda_type_array_new(coda_format_cdf);
    if (type->definition == NULL)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_array_set_base_type(type->definition, type->base_type->definition) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (rec_varys)
    {
        if (coda_type_array_add_fixed_dimension(type->definition, max_rec + 1) != 0)
        {
            coda_cdf_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->num_records = max_rec + 1;
    }
    type->value_size = (int)(type->base_type->definition->bit_size / 8);
    for (i = 0; i < num_dims; i++)
    {
        /* make sure that we always add the dimensions using C array ordering */
        int dim_id = (array_ordering == coda_array_ordering_c ? i : num_dims - 1 - i);

        if (dim_varys[dim_id])
        {
            if (coda_type_array_add_fixed_dimension(type->definition, dim[dim_id]) != 0)
            {
                coda_cdf_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            type->num_values_per_record *= dim[dim_id];
        }
    }
    if (coda_type_array_validate(type->definition) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->offset = malloc(type->num_records * sizeof(int64_t));
    if (type->offset == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       type->num_records * sizeof(int64_t), __FILE__, __LINE__);
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    for (i = 0; i < type->num_records; i++)
    {
        type->offset[i] = -1;
    }

    return type;
}

int coda_cdf_variable_add_attribute(coda_cdf_variable *type, const char *real_name, coda_dynamic_type *attribute_type,
                                    int update_definition)
{
    /* add attribute by treating coda_cdf_variable as a coda_mem_type */
    return coda_mem_type_add_attribute((coda_mem_type *)type, real_name, attribute_type, update_definition);
}
