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
#include <string.h>

#include "coda-grib-internal.h"

static coda_grib_record *empty_record_singleton = NULL;

void coda_grib_release_type(coda_type *type);

static void coda_grib_record_field_delete(coda_grib_record_field *field)
{
    if (field->name != NULL)
    {
        free(field->name);
    }
    if (field->real_name != NULL)
    {
        free(field->real_name);
    }
    if (field->type != NULL)
    {
        coda_release_type((coda_type *)field->type);
    }
    if (field->available_expr != NULL)
    {
        coda_expression_delete(field->available_expr);
    }
    free(field);
}

static void coda_grib_record_delete(coda_grib_record *type)
{
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->hash_data != NULL)
    {
        hashtable_delete(type->hash_data);
    }
    if (type->num_fields > 0)
    {
        int i;

        for (i = 0; i < type->num_fields; i++)
        {
            coda_grib_record_field_delete(type->field[i]);
        }
        free(type->field);
    }
    free(type);
}

static void coda_grib_array_delete(coda_grib_array *type)
{
    int i;

    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->base_type != NULL)
    {
        coda_release_type((coda_type *)type->base_type);
    }
    for (i = 0; i < type->num_dims; i++)
    {
        if (type->dim_expr[i] != NULL)
        {
            coda_expression_delete(type->dim_expr[i]);
        }
    }
    free(type);
}

static void coda_grib_basic_type_delete(coda_grib_basic_type *type)
{
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    free(type);
}

void coda_grib_release_type(coda_type *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (type->type_class)
    {
        case coda_record_class:
            coda_grib_record_delete((coda_grib_record *)type);
            break;
        case coda_array_class:
            coda_grib_array_delete((coda_grib_array *)type);
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            coda_grib_basic_type_delete((coda_grib_basic_type *)type);
            break;
        default:
            assert(0);
            exit(1);
    }
}

coda_grib_record_field *coda_grib_record_field_new(const char *name)
{
    coda_grib_record_field *field;

    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field name '%s' is not a valid identifier for field definition",
                       name);
        return NULL;
    }

    field = (coda_grib_record_field *)malloc(sizeof(coda_grib_record_field));
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_record_field), __FILE__, __LINE__);
        return NULL;
    }
    field->name = NULL;
    field->real_name = NULL;
    field->type = NULL;
    field->hidden = 0;
    field->optional = 0;
    field->available_expr = NULL;

    field->name = strdup(name);
    if (field->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_grib_record_field_delete(field);
        return NULL;
    }

    return field;
}

int coda_grib_record_field_set_type(coda_grib_record_field *field, coda_grib_type *type)
{
    assert(type != NULL);
    if (field->type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a type");
        return -1;
    }
    field->type = type;
    type->retain_count++;
    return 0;
}

int coda_grib_record_field_set_hidden(coda_grib_record_field *field)
{
    field->hidden = 1;
    return 0;
}

int coda_grib_record_field_set_optional(coda_grib_record_field *field)
{
    field->optional = 1;
    return 0;
}

int coda_grib_record_field_validate(coda_grib_record_field *field)
{
    if (field->type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing type for field definition");
        return -1;
    }
    return 0;
}

coda_grib_record *coda_grib_record_new(void)
{
    coda_grib_record *type;

    type = (coda_grib_record *)malloc(sizeof(coda_grib_record));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_record), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_record_class;
    type->name = NULL;
    type->description = NULL;
    type->read_type = coda_native_type_not_available;
    type->bit_size = -1;
    type->hash_data = NULL;
    type->num_fields = 0;
    type->field = NULL;
    type->has_hidden_fields = 0;
    type->has_available_expr_fields = 0;

    type->hash_data = hashtable_new(0);
    if (type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_grib_record_delete(type);
        return NULL;
    }

    return type;
}

coda_grib_record *coda_grib_empty_record(void)
{
    if (empty_record_singleton == NULL)
    {
        empty_record_singleton = coda_grib_record_new();
        assert(empty_record_singleton != NULL);
    }

    return empty_record_singleton;
}

int coda_grib_record_add_field(coda_grib_record *type, coda_grib_record_field *field)
{
    if (type->num_fields % BLOCK_SIZE == 0)
    {
        coda_grib_record_field **new_field;

        new_field = realloc(type->field, (type->num_fields + BLOCK_SIZE) * sizeof(coda_grib_record_field *));
        if (new_field == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_fields + BLOCK_SIZE) * sizeof(coda_grib_record_field *), __FILE__, __LINE__);
            return -1;
        }
        type->field = new_field;
    }
    if (hashtable_add_name(type->hash_data, field->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate field with name %s for record definition", field->name);
        return -1;
    }
    type->num_fields++;
    type->field[type->num_fields - 1] = field;

    return 0;
}

coda_grib_array *coda_grib_array_new(void)
{
    coda_grib_array *type;

    type = (coda_grib_array *)malloc(sizeof(coda_grib_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_array), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_array_class;
    type->name = NULL;
    type->description = NULL;
    type->read_type = coda_native_type_not_available;
    type->bit_size = -1;
    type->base_type = NULL;
    type->num_elements = 1;
    type->num_dims = 0;

    return type;
}

int coda_grib_array_set_base_type(coda_grib_array *type, coda_grib_type *base_type)
{
    if (type->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "array already has a base type");
        return -1;
    }
    type->base_type = base_type;
    base_type->retain_count++;

    return 0;
}

int coda_grib_array_add_fixed_dimension(coda_grib_array *type, long dim)
{
    if (type->num_dims == CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "maximum number of dimensions (%d) exceeded for array definition",
                       CODA_MAX_NUM_DIMS);
        return -1;
    }
    if (dim < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "dimension size (%ld) cannot be negative", dim);
        return -1;
    }
    type->dim[type->num_dims] = dim;
    type->dim_expr[type->num_dims] = NULL;
    type->num_dims++;

    /* update num_elements */
    if (type->num_elements != -1)
    {
        type->num_elements *= dim;
    }

    return 0;
}

int coda_grib_array_add_variable_dimension(coda_grib_array *type, coda_expression *dim_expr)
{
    /* using dim_expr=NULL is allowed for some backends */
    if (type->num_dims == CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "maximum number of dimensions (%d) exceeded for array definition",
                       CODA_MAX_NUM_DIMS);
        return -1;
    }
    type->dim[type->num_dims] = -1;
    type->dim_expr[type->num_dims] = dim_expr;
    type->num_dims++;
    type->num_elements = -1;
    type->bit_size = -1;

    return 0;
}

int coda_grib_array_validate(coda_grib_array *type)
{
    if (type->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing base type for array definition");
        return -1;
    }
    if (type->num_dims == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "number of dimensions is 0 for array definition");
        return -1;
    }
    return 0;
}

coda_grib_basic_type *coda_grib_basic_type_new(coda_type_class type_class)
{
    coda_grib_basic_type *type;

    if (type_class != coda_integer_class && type_class != coda_real_class && type_class != coda_text_class &&
        type_class != coda_raw_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid type class (%s) for basic type",
                       coda_type_get_class_name(type_class));
        return NULL;
    }

    type = (coda_grib_basic_type *)malloc(sizeof(coda_grib_basic_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_basic_type), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = type_class;
    type->name = NULL;
    type->description = NULL;
    /* use the default read type */
    switch (type_class)
    {
        case coda_integer_class:
            type->read_type = coda_native_type_int64;
            break;
        case coda_real_class:
            type->read_type = coda_native_type_double;
            break;
        case coda_text_class:
            type->read_type = coda_native_type_string;
            break;
        case coda_raw_class:
            type->read_type = coda_native_type_bytes;
            break;
        default:
            assert(0);
            exit(1);
    }
    type->bit_size = -1;

    return type;
}

int coda_grib_basic_type_set_bit_size(coda_grib_basic_type *type, int64_t bit_size)
{
    if (bit_size <= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size may not be <= 0");
        return -1;
    }
    type->bit_size = bit_size;
    return 0;
}

int coda_grib_basic_type_set_read_type(coda_grib_basic_type *type, coda_native_type read_type)
{
    switch (type->type_class)
    {
        case coda_integer_class:
            if (read_type != coda_native_type_int8 && read_type != coda_native_type_uint8 &&
                read_type != coda_native_type_int16 && read_type != coda_native_type_uint16 &&
                read_type != coda_native_type_int32 && read_type != coda_native_type_uint32 &&
                read_type != coda_native_type_int64 && read_type != coda_native_type_uint64)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for integer definition",
                               coda_type_get_native_type_name(read_type));
                return -1;
            }
            break;
        case coda_real_class:
            if (read_type != coda_native_type_float && read_type != coda_native_type_double)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for float definition",
                               coda_type_get_native_type_name(read_type));
                return -1;
            }
            break;
        case coda_text_class:
            if (read_type != coda_native_type_char && read_type != coda_native_type_string)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for text definition",
                               coda_type_get_native_type_name(read_type));
                return -1;
            }
            break;
        case coda_raw_class:
            if (read_type != coda_native_type_bytes)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for raw definition",
                               coda_type_get_native_type_name(read_type));
                return -1;
            }
            break;
        case coda_record_class:
        case coda_array_class:
        case coda_special_class:
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "setting read type for %s type definition not allowed",
                           coda_type_get_class_name(type->type_class));
            return -1;
    }
    type->read_type = read_type;
    if (type->bit_size == -1)
    {
        /* automatically set bit size to default value */
        switch (read_type)
        {
            case coda_native_type_int8:
            case coda_native_type_uint8:
            case coda_native_type_char:
                type->bit_size = 8;
                break;
            case coda_native_type_int16:
            case coda_native_type_uint16:
                type->bit_size = 16;
                break;
            case coda_native_type_int32:
            case coda_native_type_uint32:
            case coda_native_type_float:
                type->bit_size = 32;
                break;
            case coda_native_type_int64:
            case coda_native_type_uint64:
            case coda_native_type_double:
                type->bit_size = 64;
                break;
            case coda_native_type_string:
            case coda_native_type_bytes:
            case coda_native_type_not_available:
                /* don't set bit_size */
                break;
        }
    }
    return 0;
}

int coda_grib_basic_type_validate(coda_grib_basic_type *type)
{
    if (type->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for number type definition");
        return -1;
    }
    return 0;
}


int coda_grib_type_get_read_type(const coda_type *type, coda_native_type *read_type)
{
    *read_type = ((coda_grib_type *)type)->read_type;
    return 0;
}

int coda_grib_type_get_string_length(const coda_type *type, long *length)
{
    if (((coda_grib_basic_type *)type)->bit_size >= 0)
    {
        *length = ((coda_grib_basic_type *)type)->bit_size * 8;
    }
    else
    {
        *length = -1;
    }
    return 0;
}

int coda_grib_type_get_num_record_fields(const coda_type *type, long *num_fields)
{
    *num_fields = ((coda_grib_record *)type)->num_fields;
    return 0;
}

int coda_grib_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index)
{
    *index = hashtable_get_index_from_name(((coda_grib_record *)type)->hash_data, name);
    if (*index >= 0)
    {
        return 0;
    }

    coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
    return -1;
}

int coda_grib_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type)
{
    if (index < 0 || index >= ((coda_grib_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_grib_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *field_type = (coda_type *)((coda_grib_record *)type)->field[index]->type;
    return 0;
}

int coda_grib_type_get_record_field_name(const coda_type *type, long index, const char **name)
{
    if (index < 0 || index >= ((coda_grib_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_grib_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *name = ((coda_grib_record *)type)->field[index]->name;
    return 0;
}

int coda_grib_type_get_record_field_hidden_status(const coda_type *type, long index, int *hidden)
{
    if (index < 0 || index >= ((coda_grib_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_grib_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *hidden = ((coda_grib_record *)type)->field[index]->hidden;
    return 0;
}

int coda_grib_type_get_record_field_available_status(const coda_type *type, long index, int *available)
{
    if (index < 0 || index >= ((coda_grib_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_grib_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    if (((coda_grib_record *)type)->field[index]->optional)
    {
        *available = -1;
    }
    else
    {
        *available = 1;
    }
    return 0;
}

int coda_grib_type_get_array_num_dims(const coda_type *type, int *num_dims)
{
    *num_dims = ((coda_grib_array *)type)->num_dims;
    return 0;
}

int coda_grib_type_get_array_dim(const coda_type *type, int *num_dims, long dim[])
{
    int i;

    *num_dims = ((coda_grib_array *)type)->num_dims;
    for (i = 0; i < *num_dims; i++)
    {
        dim[i] = ((coda_grib_array *)type)->dim[i];
    }

    return 0;
}

int coda_grib_type_get_array_base_type(const coda_type *type, coda_type **base_type)
{
    *base_type = (coda_type *)((coda_grib_array *)type)->base_type;
    return 0;
}
