/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

#include "coda-cdf-internal.h"

#include <assert.h>
#include <stdlib.h>

void coda_cdf_type_delete(coda_dynamic_type *type)
{
    assert(type != NULL);
    assert(type->backend == coda_backend_cdf);

    switch (((coda_cdf_type *)type)->tag)
    {
        case tag_cdf_basic_type:
            break;
        case tag_cdf_time:
            if (((coda_cdf_time *)type)->base_type != NULL)
            {
                coda_dynamic_type_delete(((coda_cdf_time *)type)->base_type);
            }
            break;
        case tag_cdf_variable:
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
            break;
    }
    if (type->definition != NULL)
    {
        coda_type_release(type->definition);
    }
    free(type);
}

static int basic_type_init(coda_cdf_type *type, int32_t data_type, int32_t num_elements)
{
    coda_type_class type_class;
    coda_native_type native_type;
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
            return -1;
    }
    switch (type_class)
    {
        case coda_integer_class:
        case coda_real_class:
            type->definition = (coda_type *)coda_type_number_new(coda_format_cdf, type_class);
            break;
        case coda_text_class:
            type->definition = (coda_type *)coda_type_text_new(coda_format_cdf);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (type->definition == NULL)
    {
        return -1;
    }
    if (coda_type_set_read_type(type->definition, native_type) != 0)
    {
        return -1;
    }
    if (coda_type_set_byte_size(type->definition, byte_size) != 0)
    {
        return -1;
    }
    return 0;
}

static coda_cdf_type *basic_type_new(int32_t data_type, int32_t num_elements)
{
    coda_cdf_type *type;

    type = (coda_cdf_type *)malloc(sizeof(coda_cdf_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_cdf_type), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_cdf;
    type->definition = NULL;
    type->tag = tag_cdf_basic_type;
    if (basic_type_init(type, data_type, num_elements) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    return type;
}

static coda_cdf_time *time_type_new(int32_t data_type, coda_cdf_type *base_type)
{
    char *exprstr;
    coda_cdf_time *type;
    coda_expression *expr;

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
    type->tag = tag_cdf_time;
    type->data_type = data_type;
    type->base_type = NULL;

    if (data_type == 31)
    {
        /* CDF_EPOCH */
        exprstr = "float(.) * 1e-3 - 63113904000.0";
    }
    else
    {
        /* CDF_TIME_TT2000 */
        exprstr = "float(.) * 1e-9 - 43200.0";
    }
    if (coda_expression_from_string(exprstr, &expr) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->definition = coda_type_time_new(coda_format_cdf, expr);
    if (type->definition == NULL)
    {
        coda_expression_delete(expr);
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_time_set_base_type(type->definition, base_type->definition) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->base_type = (coda_dynamic_type *)base_type;

    return type;
}

coda_dynamic_type *coda_cdf_variable_new(int32_t data_type, int32_t max_rec, int32_t rec_varys, int32_t num_dims,
                                         int32_t dim[CODA_MAX_NUM_DIMS], int32_t dim_varys[CODA_MAX_NUM_DIMS],
                                         coda_array_ordering array_ordering, int32_t num_elements,
                                         int sparse_rec_method, coda_cdf_variable **variable)
{
    coda_cdf_variable *type;
    int is_scalar = 0;
    int time_type = -1;
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
    type->tag = tag_cdf_variable;
    type->attributes = NULL;
    type->base_type = NULL;
    type->num_records = 1;
    type->num_values_per_record = 1;
    type->value_size = -1;
    type->sparse_rec_method = sparse_rec_method;
    type->offset = NULL;
    type->data = NULL;

    if (!rec_varys)
    {
        is_scalar = 1;
        for (i = 0; i < num_dims; i++)
        {
            if (dim_varys[i])
            {
                is_scalar = 0;
                break;
            }
        }
    }
    if (data_type == 31 || data_type == 33)
    {
        time_type = data_type;
        if (data_type == 31)
        {
            /* EPOCH */
            data_type = 45;
        }
        else
        {
            /* TIME_TT2000 */
            data_type = 8;
        }
    }

    if (is_scalar)
    {
        if (basic_type_init((coda_cdf_type *)type, data_type, num_elements) != 0)
        {
            coda_cdf_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->value_size = (int)(type->definition->bit_size / 8);
    }
    else
    {
        type->base_type = basic_type_new(data_type, num_elements);
        if (type->base_type == NULL)
        {
            coda_cdf_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (time_type != -1)
        {
            coda_cdf_time *base_type;

            base_type = time_type_new(time_type, type->base_type);
            if (base_type == NULL)
            {
                coda_cdf_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            type->base_type = (coda_cdf_type *)base_type;
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

    *variable = type;

    if (is_scalar && time_type != -1)
    {
        coda_cdf_time *special_type;

        special_type = time_type_new(time_type, (coda_cdf_type *)type);
        if (special_type == NULL)
        {
            coda_cdf_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        return (coda_dynamic_type *)special_type;
    }

    return (coda_dynamic_type *)type;
}

int coda_cdf_variable_add_attribute(coda_cdf_variable *type, const char *real_name, coda_dynamic_type *attribute_type,
                                    int update_definition)
{
    coda_mem_record *attributes;
    long index = -1;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (real_name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "real_name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (attribute_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attribute_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (type->attributes == NULL)
    {
        if (update_definition)
        {
            if (type->definition->attributes == NULL)
            {
                type->definition->attributes = coda_type_record_new(type->definition->format);
                if (type->definition->attributes == NULL)
                {
                    return -1;
                }
            }
            type->attributes = coda_mem_record_new(type->definition->attributes, NULL);
            if (type->attributes == NULL)
            {
                return -1;
            }
        }
        else
        {
            coda_set_error(CODA_ERROR_INVALID_NAME, "type does not have an attribute with name '%s' (%s:%u)", real_name,
                           __FILE__, __LINE__);
            return -1;
        }
    }
    else
    {
        if (type->attributes->backend != coda_backend_memory)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cannot add attribute (%s:%u)", __FILE__, __LINE__);
            return -1;
        }
        assert(type->definition->attributes == type->attributes->definition);
    }

    attributes = type->attributes;

    index = hashtable_get_index_from_name(attributes->definition->real_name_hash_data, real_name);
    if (update_definition)
    {
        if (index < 0 || (index < attributes->num_fields && attributes->field_type[index] != NULL))
        {
            if (coda_type_record_create_field(attributes->definition, real_name, attribute_type->definition) != 0)
            {
                return -1;
            }
            index = attributes->definition->num_fields - 1;
        }
        if (attributes->num_fields < attributes->definition->num_fields)
        {
            coda_dynamic_type **new_field_type;
            long i;

            new_field_type = realloc(attributes->field_type,
                                     attributes->definition->num_fields * sizeof(coda_dynamic_type *));
            if (new_field_type == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               attributes->definition->num_fields * sizeof(coda_dynamic_type *), __FILE__, __LINE__);

                return -1;
            }
            attributes->field_type = new_field_type;
            for (i = attributes->num_fields; i < attributes->definition->num_fields; i++)
            {
                attributes->field_type[i] = NULL;
            }
            attributes->num_fields = attributes->definition->num_fields;
        }
    }
    else
    {
        if (index < 0)
        {
            coda_set_error(CODA_ERROR_INVALID_NAME, "type does not have an attribute with name '%s' (%s:%u)", real_name,
                           __FILE__, __LINE__);
            return -1;
        }
        if (attributes->field_type[index] != NULL)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attribute '%s' is already set (%s:%u)", real_name, __FILE__,
                           __LINE__);
            return -1;
        }
        if (attributes->definition->field[index]->type != attribute_type->definition)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "trying to add attribute '%s' of incompatible type (%s:%u)",
                           real_name, __FILE__, __LINE__);
            return -1;
        }
    }
    attributes->field_type[index] = attribute_type;

    return 0;
}
