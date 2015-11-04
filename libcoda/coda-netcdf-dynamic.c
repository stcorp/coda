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

#include "coda-netcdf-internal.h"

void coda_netcdf_release_type(coda_type *type);

static void delete_netcdfAttributeRecord(coda_netcdf_attribute_record *type)
{
    int i;

    hashtable_delete(type->hash_data);
    if (type->attribute != NULL)
    {
        for (i = 0; i < type->num_attributes; i++)
        {
            if (type->attribute[i] != NULL)
            {
                coda_netcdf_release_type((coda_type *)type->attribute[i]);
            }
        }
        free(type->attribute);
    }
    if (type->attribute_name != NULL)
    {
        for (i = 0; i < type->num_attributes; i++)
        {
            if (type->attribute_name[i] != NULL)
            {
                free(type->attribute_name[i]);
            }
        }
        free(type->attribute_name);
    }
    if (type->attribute_real_name != NULL)
    {
        for (i = 0; i < type->num_attributes; i++)
        {
            if (type->attribute_real_name[i] != NULL)
            {
                free(type->attribute_real_name[i]);
            }
        }
        free(type->attribute_real_name);
    }
    free(type);
}

static void delete_netcdfBasicType(coda_netcdf_basic_type *type)
{
    if (type->attributes != NULL)
    {
        delete_netcdfAttributeRecord(type->attributes);
    }
    free(type);
}

static void delete_netcdfRoot(coda_netcdf_root *type)
{
    int i;

    hashtable_delete(type->hash_data);
    if (type->variable != NULL)
    {
        for (i = 0; i < type->num_variables; i++)
        {
            coda_netcdf_release_type((coda_type *)type->variable[i]);
        }
        free(type->variable);
    }
    if (type->variable_name != NULL)
    {
        for (i = 0; i < type->num_variables; i++)
        {
            free(type->variable_name[i]);
        }
        free(type->variable_name);
    }
    if (type->variable_real_name != NULL)
    {
        for (i = 0; i < type->num_variables; i++)
        {
            free(type->variable_real_name[i]);
        }
        free(type->variable_real_name);
    }
    if (type->attributes != NULL)
    {
        delete_netcdfAttributeRecord(type->attributes);
    }
    free(type);
}

static void delete_netcdfArray(coda_netcdf_array *type)
{
    if (type->attributes != NULL)
    {
        delete_netcdfAttributeRecord(type->attributes);
    }
    coda_netcdf_release_type((coda_type *)type->base_type);
    free(type);
}

void coda_netcdf_release_type(coda_type *type)
{
    if (type != NULL)
    {
        switch (((coda_netcdf_type *)type)->tag)
        {
            case tag_netcdf_root:
                delete_netcdfRoot((coda_netcdf_root *)type);
                break;
            case tag_netcdf_array:
                delete_netcdfArray((coda_netcdf_array *)type);
                break;
            case tag_netcdf_basic_type:
                delete_netcdfBasicType((coda_netcdf_basic_type *)type);
                break;
            case tag_netcdf_attribute_record:
                delete_netcdfAttributeRecord((coda_netcdf_attribute_record *)type);
                break;
            default:
                assert(0);
                exit(1);
        }
    }
}

void coda_netcdf_release_dynamic_type(coda_dynamic_type *type)
{
    coda_netcdf_release_type((coda_type *)type);
}

coda_netcdf_root *coda_netcdf_root_new(void)
{
    coda_netcdf_root *type;

    type = malloc(sizeof(coda_netcdf_root));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_root), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_netcdf;
    type->type_class = coda_record_class;
    type->name = NULL;
    type->description = NULL;
    type->tag = tag_netcdf_root;
    type->num_variables = 0;
    type->variable = NULL;
    type->variable_name = NULL;
    type->variable_real_name = NULL;
    type->hash_data = hashtable_new(0);
    type->attributes = NULL;
    if (type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_netcdfRoot(type);
        return NULL;
    }

    return type;
}

int coda_netcdf_root_add_variable(coda_netcdf_root *type, const char *name, coda_netcdf_type *variable)
{
    char *variable_name;

    variable_name = coda_identifier_from_name(name, type->hash_data);
    if (variable_name == NULL)
    {
        return -1;
    }
    if (hashtable_add_name(type->hash_data, variable_name) != 0)
    {
        assert(0);
        exit(1);
    }
    if (type->num_variables % BLOCK_SIZE == 0)
    {
        coda_netcdf_type **new_variable;
        char **new_name;

        new_variable = realloc(type->variable, (type->num_variables + BLOCK_SIZE) * sizeof(coda_netcdf_type *));
        if (new_variable == NULL)
        {
            free(variable_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_variables + BLOCK_SIZE) * sizeof(coda_netcdf_type *), __FILE__, __LINE__);
            return -1;
        }
        type->variable = new_variable;
        new_name = realloc(type->variable_name, (type->num_variables + BLOCK_SIZE) * sizeof(char *));
        if (new_name == NULL)
        {
            free(variable_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_variables + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        type->variable_name = new_name;
        new_name = realloc(type->variable_real_name, (type->num_variables + BLOCK_SIZE) * sizeof(char *));
        if (new_name == NULL)
        {
            free(variable_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_variables + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        type->variable_real_name = new_name;
    }
    type->num_variables++;
    type->variable[type->num_variables - 1] = variable;
    type->variable_name[type->num_variables - 1] = variable_name;
    type->variable_real_name[type->num_variables - 1] = strdup(name);
    if (type->variable_real_name[type->num_variables - 1] == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_netcdf_root_add_attributes(coda_netcdf_root *type, coda_netcdf_attribute_record *attributes)
{
    assert(type->attributes == NULL);
    type->attributes = attributes;

    return 0;
}

coda_netcdf_array *coda_netcdf_array_new(int num_dims, long dim[CODA_MAX_NUM_DIMS], coda_netcdf_basic_type *base_type)
{
    coda_netcdf_array *type;
    int i;

    type = malloc(sizeof(coda_netcdf_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_array), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_netcdf;
    type->type_class = coda_array_class;
    type->name = NULL;
    type->description = NULL;
    type->tag = tag_netcdf_array;
    type->num_dims = num_dims;
    type->num_elements = 1;
    for (i = 0; i < num_dims; i++)
    {
        type->dim[i] = dim[i];
        type->num_elements *= dim[i];
    }
    type->base_type = base_type;
    type->attributes = NULL;

    return type;
}

int coda_netcdf_array_add_attributes(coda_netcdf_array *type, coda_netcdf_attribute_record *attributes)
{
    assert(type->attributes == NULL);
    type->attributes = attributes;

    return 0;
}

coda_netcdf_basic_type *coda_netcdf_basic_type_new(int nc_type, int64_t offset, int record_var, int length)
{
    coda_netcdf_basic_type *type;

    type = malloc(sizeof(coda_netcdf_basic_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_basic_type), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_netcdf;
    type->name = NULL;
    type->description = NULL;
    type->tag = tag_netcdf_basic_type;
    type->offset = offset;
    type->record_var = record_var;
    type->has_conversion = 0;
    type->add_offset = 0.0;
    type->scale_factor = 1.0;
    type->has_fill_value = 0;
    type->fill_value = coda_NaN();
    type->attributes = NULL;
    switch (nc_type)
    {
        case 1:
            type->type_class = coda_integer_class;
            type->read_type = coda_native_type_int8;
            type->byte_size = 1;
            break;
        case 2:
            type->type_class = coda_text_class;
            type->read_type = (length > 1) ? coda_native_type_string : coda_native_type_char;
            type->byte_size = length;
            break;
        case 3:
            type->type_class = coda_integer_class;
            type->read_type = coda_native_type_int16;
            type->byte_size = 2;
            break;
        case 4:
            type->type_class = coda_integer_class;
            type->read_type = coda_native_type_int32;
            type->byte_size = 4;
            break;
        case 5:
            type->type_class = coda_real_class;
            type->read_type = coda_native_type_float;
            type->byte_size = 4;
            break;
        case 6:
            type->type_class = coda_real_class;
            type->read_type = coda_native_type_double;
            type->byte_size = 8;
            break;
        default:
            assert(0);
            exit(1);
    }

    return type;
}

int coda_netcdf_basic_type_add_attributes(coda_netcdf_basic_type *type, coda_netcdf_attribute_record *attributes)
{
    assert(type->attributes == NULL);
    type->attributes = attributes;

    return 0;
}

coda_netcdf_attribute_record *coda_netcdf_attribute_record_new(void)
{
    coda_netcdf_attribute_record *type;

    type = malloc(sizeof(coda_netcdf_attribute_record));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_attribute_record), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_netcdf;
    type->type_class = coda_record_class;
    type->name = NULL;
    type->description = NULL;
    type->tag = tag_netcdf_attribute_record;
    type->num_attributes = 0;
    type->attribute = NULL;
    type->attribute_name = NULL;
    type->attribute_real_name = NULL;
    type->hash_data = hashtable_new(0);
    if (type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_netcdfAttributeRecord(type);
        return NULL;
    }

    return type;
}

int coda_netcdf_attribute_record_add_attribute(coda_netcdf_attribute_record *type, const char *name,
                                               coda_netcdf_type *attribute)
{
    char *attribute_name;

    attribute_name = coda_identifier_from_name(name, type->hash_data);
    if (attribute_name == NULL)
    {
        return -1;
    }
    if (hashtable_add_name(type->hash_data, attribute_name) != 0)
    {
        assert(0);
        exit(1);
    }
    if (type->num_attributes % BLOCK_SIZE == 0)
    {
        coda_netcdf_type **new_attribute;
        char **new_name;

        new_attribute = realloc(type->attribute, (type->num_attributes + BLOCK_SIZE) * sizeof(coda_netcdf_type *));
        if (new_attribute == NULL)
        {
            free(attribute_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_attributes + BLOCK_SIZE) * sizeof(coda_netcdf_type *), __FILE__, __LINE__);
            return -1;
        }
        type->attribute = new_attribute;
        new_name = realloc(type->attribute_name, (type->num_attributes + BLOCK_SIZE) * sizeof(char *));
        if (new_name == NULL)
        {
            free(attribute_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_attributes + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        type->attribute_name = new_name;
        new_name = realloc(type->attribute_real_name, (type->num_attributes + BLOCK_SIZE) * sizeof(char *));
        if (new_name == NULL)
        {
            free(attribute_name);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_attributes + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        type->attribute_real_name = new_name;
    }
    type->num_attributes++;
    type->attribute[type->num_attributes - 1] = attribute;
    type->attribute_name[type->num_attributes - 1] = attribute_name;
    type->attribute_real_name[type->num_attributes - 1] = strdup(name);
    if (type->attribute_real_name[type->num_attributes - 1] == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}
