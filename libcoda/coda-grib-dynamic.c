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

static coda_grib_dynamic_record *empty_record_singleton = NULL;

static void dynamic_record_delete(coda_grib_dynamic_record *type)
{
    int num_fields = 0;

    if (type->definition != NULL)
    {
        num_fields = type->definition->num_fields;
        coda_release_type((coda_type *)type->definition);
    }
    if (type->field_type != NULL)
    {
        int i;

        for (i = 0; i < num_fields; i++)
        {
            if (type->field_type[i] != NULL)
            {
                coda_grib_release_dynamic_type(type->field_type[i]);
            }
        }
        free(type->field_type);
    }
    free(type);
}

static void dynamic_array_delete(coda_grib_dynamic_array *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    if (type->element_type != NULL)
    {
        int i;

        for (i = 0; i < type->num_elements; i++)
        {
            if (type->element_type[i] != NULL)
            {
                coda_grib_release_dynamic_type(type->element_type[i]);
            }
        }
        free(type->element_type);
    }
    free(type);
}

static void dynamic_integer_delete(coda_grib_dynamic_integer *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    free(type);
}

static void dynamic_real_delete(coda_grib_dynamic_real *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    free(type);
}

static void dynamic_text_delete(coda_grib_dynamic_text *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    if (type->text != NULL)
    {
        free(type->text);
    }
    free(type);
}

static void dynamic_raw_delete(coda_grib_dynamic_raw *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    if (type->data != NULL)
    {
        free(type->data);
    }
    free(type);
}

static void dynamic_value_array_delete(coda_grib_dynamic_value_array *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    if (type->base_type != NULL)
    {
        coda_grib_release_dynamic_type(type->base_type);
    }
    free(type);
}

static void dynamic_value_delete(coda_grib_dynamic_type *type)
{
    if (type->definition != NULL)
    {
        coda_release_type((coda_type *)type->definition);
    }
    free(type);
}

void coda_grib_release_dynamic_type(coda_grib_dynamic_type *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (((coda_grib_dynamic_type *)type)->tag)
    {
        case tag_grib_record:
            dynamic_record_delete((coda_grib_dynamic_record *)type);
            break;
        case tag_grib_array:
            dynamic_array_delete((coda_grib_dynamic_array *)type);
            break;
        case tag_grib_integer:
            dynamic_integer_delete((coda_grib_dynamic_integer *)type);
            break;
        case tag_grib_real:
            dynamic_real_delete((coda_grib_dynamic_real *)type);
            break;
        case tag_grib_text:
            dynamic_text_delete((coda_grib_dynamic_text *)type);
            break;
        case tag_grib_raw:
            dynamic_raw_delete((coda_grib_dynamic_raw *)type);
            break;
        case tag_grib_value_array:
            dynamic_value_array_delete((coda_grib_dynamic_value_array *)type);
            break;
        case tag_grib_value:
            dynamic_value_delete((coda_grib_dynamic_type *)type);
            break;
    }
}

coda_grib_dynamic_record *coda_grib_dynamic_record_new(coda_grib_record *definition)
{
    coda_grib_dynamic_record *type;
    int i;

    assert(definition != NULL);
    type = (coda_grib_dynamic_record *)malloc(sizeof(coda_grib_dynamic_record));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_record), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_record_class;
    type->tag = tag_grib_record;
    type->definition = definition;
    definition->retain_count++;
    type->field_type = malloc(definition->num_fields * sizeof(coda_grib_dynamic_type *));
    if (type->field_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       definition->num_fields * sizeof(coda_grib_dynamic_type *), __FILE__, __LINE__);

        dynamic_record_delete(type);
        return NULL;
    }
    for (i = 0; i < definition->num_fields; i++)
    {
        type->field_type[i] = NULL;
    }

    return type;
}

int coda_grib_dynamic_record_set_field(coda_grib_dynamic_record *type, const char *name,
                                       coda_grib_dynamic_type *field_type)
{
    int index;

    index = hashtable_get_index_from_name(type->definition->hash_data, name);
    if (index < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_NAME, "record does not have a field with name '%s'", name);
        return -1;
    }
    if (type->field_type[index] != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field '%s' is already set", name);
        return -1;
    }
    if (type->definition->field[index]->type != field_type->definition)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "trying to add field '%s' of incompatible type", name);
        return -1;
    }
    type->field_type[index] = field_type;
    field_type->retain_count++;

    return 0;
}

int coda_grib_dynamic_record_validate(coda_grib_dynamic_record *type)
{
    int i;

    for (i = 0; i < type->definition->num_fields; i++)
    {
        if (type->field_type[i] == NULL && !type->definition->field[i]->optional)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "non-optional field %s missing",
                           type->definition->field[i]->name);
            return -1;
        }
    }
    return 0;
}

coda_grib_dynamic_array *coda_grib_dynamic_array_new(coda_grib_array *definition)
{
    coda_grib_dynamic_array *type;

    assert(definition != NULL);
    type = (coda_grib_dynamic_array *)malloc(sizeof(coda_grib_dynamic_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_array), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_array_class;
    type->tag = tag_grib_array;
    type->definition = definition;
    definition->retain_count++;
    type->num_elements = 0;
    type->element_type = NULL;

    return type;
}

int coda_grib_dynamic_array_add_element(coda_grib_dynamic_array *type, coda_grib_dynamic_type *element)
{
    if (type->definition->base_type != element->definition)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "trying to add array element '%ld' of incompatible type",
                       type->num_elements);
        return -1;
    }
    if (type->num_elements % BLOCK_SIZE == 0)
    {
        coda_grib_dynamic_type **new_element_type;

        new_element_type = realloc(type->element_type,
                                   (type->num_elements + BLOCK_SIZE) * sizeof(coda_grib_dynamic_type *));
        if (new_element_type == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_elements + BLOCK_SIZE) * sizeof(coda_grib_dynamic_type *), __FILE__, __LINE__);
            return -1;
        }
        type->element_type = new_element_type;
    }
    type->num_elements++;
    type->element_type[type->num_elements - 1] = element;
    element->retain_count++;

    return 0;
}

int coda_grib_dynamic_array_validate(coda_grib_dynamic_array *type)
{
    if (type->definition->num_elements >= 0 && type->num_elements != type->definition->num_elements)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "number of actual array elements (%ld) does not match number of elements from definition (%ld)",
                       type->num_elements, type->definition->num_elements);
    }
    return 0;
}

coda_grib_dynamic_integer *coda_grib_dynamic_integer_new(coda_grib_basic_type *definition, int64_t value)
{
    coda_grib_dynamic_integer *type;

    assert(definition != NULL);
    assert(definition->type_class == coda_integer_class);
    type = (coda_grib_dynamic_integer *)malloc(sizeof(coda_grib_dynamic_integer));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_integer), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_integer_class;
    type->tag = tag_grib_integer;
    type->definition = definition;
    definition->retain_count++;
    type->value = value;

    return type;
}

coda_grib_dynamic_real *coda_grib_dynamic_real_new(coda_grib_basic_type *definition, double value)
{
    coda_grib_dynamic_real *type;

    assert(definition != NULL);
    assert(definition->type_class == coda_real_class);
    type = (coda_grib_dynamic_real *)malloc(sizeof(coda_grib_dynamic_real));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_real), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_real_class;
    type->tag = tag_grib_real;
    type->definition = definition;
    definition->retain_count++;
    type->value = value;

    return type;
}

coda_grib_dynamic_text *coda_grib_dynamic_text_new(coda_grib_basic_type *definition, const char *text)
{
    coda_grib_dynamic_text *type;

    assert(definition != NULL);
    assert(definition->type_class == coda_text_class);
    assert(definition->read_type != coda_native_type_char || strlen(text) == 1);
    type = (coda_grib_dynamic_text *)malloc(sizeof(coda_grib_dynamic_text));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_text), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_real_class;
    type->tag = tag_grib_real;
    type->definition = definition;
    definition->retain_count++;
    type->text = strdup(text);
    if (type->text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        dynamic_text_delete(type);
        return NULL;
    }

    return type;
}

coda_grib_dynamic_raw *coda_grib_dynamic_raw_new(coda_grib_basic_type *definition, long length, const uint8_t *data)
{
    coda_grib_dynamic_raw *type;

    assert(definition != NULL);
    assert(definition->type_class == coda_raw_class);
    type = (coda_grib_dynamic_raw *)malloc(sizeof(coda_grib_dynamic_raw));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_raw), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_raw_class;
    type->tag = tag_grib_raw;
    type->definition = definition;
    definition->retain_count++;
    type->length = length;
    type->data = malloc(length * sizeof(uint8_t));
    if (type->data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(length * sizeof(uint8_t)), __FILE__, __LINE__);
        dynamic_raw_delete(type);
        return NULL;
    }
    memcpy(type->data, data, length);

    return type;
}

static coda_grib_dynamic_type *dynamic_value_new(coda_grib_type *definition)
{
    coda_grib_dynamic_type *type;

    assert(definition != NULL);
    assert(definition->type_class == coda_real_class);
    type = (coda_grib_dynamic_type *)malloc(sizeof(coda_grib_dynamic_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_type), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_real_class;
    type->tag = tag_grib_value;
    type->definition = definition;
    definition->retain_count++;

    return type;
}

coda_grib_dynamic_value_array *coda_grib_dynamic_value_array_new(coda_grib_array *definition, int num_elements,
                                                                 int64_t byte_offset, int element_bit_size,
                                                                 int16_t decimalScaleFactor, int16_t binaryScaleFactor,
                                                                 float referenceValue)
{
    coda_grib_dynamic_value_array *type;

    assert(definition != NULL);
    type = (coda_grib_dynamic_value_array *)malloc(sizeof(coda_grib_dynamic_value_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_dynamic_value_array), __FILE__, __LINE__);
        return NULL;
    }
    type->retain_count = 0;
    type->format = coda_format_grib1;
    type->type_class = coda_array_class;
    type->tag = tag_grib_value_array;
    type->definition = definition;
    definition->retain_count++;
    type->num_elements = num_elements;
    type->base_type = NULL;
    type->bit_offset = 8 * byte_offset;
    type->element_bit_size = element_bit_size;
    type->decimalScaleFactor = decimalScaleFactor;
    type->binaryScaleFactor = binaryScaleFactor;
    type->referenceValue = referenceValue;

    type->base_type = dynamic_value_new(definition->base_type);
    if (type->base_type == NULL)
    {
        dynamic_value_array_delete(type);
        return NULL;
    }

    return type;
}

coda_grib_dynamic_record *coda_grib_empty_dynamic_record()
{
    if (empty_record_singleton == NULL)
    {
        empty_record_singleton = coda_grib_dynamic_record_new(coda_grib_empty_record());
        assert(empty_record_singleton != NULL);
    }

    return (coda_grib_dynamic_record *)empty_record_singleton;
}
