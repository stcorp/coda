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

#include "coda-mem-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void coda_mem_type_delete(coda_dynamic_type *type)
{
    int i;

    assert(type != NULL);
    assert(type->backend == coda_backend_memory);

    switch (type->definition->type_class)
    {
        case coda_record_class:
            if (((coda_mem_record *)type)->field_type != NULL)
            {
                for (i = 0; i < ((coda_mem_record *)type)->num_fields; i++)
                {
                    if (((coda_mem_record *)type)->field_type[i] != NULL)
                    {
                        coda_dynamic_type_delete(((coda_mem_record *)type)->field_type[i]);
                    }
                }
                free(((coda_mem_record *)type)->field_type);
            }
            break;
        case coda_array_class:
            if (((coda_mem_array *)type)->element != NULL)
            {
                for (i = 0; i < ((coda_mem_array *)type)->num_elements; i++)
                {
                    if (((coda_mem_array *)type)->element[i] != NULL)
                    {
                        coda_dynamic_type_delete(((coda_mem_array *)type)->element[i]);
                    }
                }
                free(((coda_mem_array *)type)->element);
            }
            break;
        case coda_integer_class:
            break;
        case coda_real_class:
            break;
        case coda_text_class:
            if (((coda_mem_text *)type)->text != NULL)
            {
                free(((coda_mem_text *)type)->text);
            }
            break;
        case coda_raw_class:
            if (((coda_mem_raw *)type)->data != NULL)
            {
                free(((coda_mem_raw *)type)->data);
            }
            break;
        case coda_special_class:
            if (((coda_mem_special *)type)->base_type != NULL)
            {
                coda_dynamic_type_delete(((coda_mem_special *)type)->base_type);
            }
            break;
    }
    if (((coda_mem_type *)type)->attributes != NULL)
    {
        coda_dynamic_type_delete(((coda_mem_type *)type)->attributes);
    }
    if (type->definition != NULL)
    {
        coda_type_release((coda_type *)type->definition);
    }
    free(type);
}

int coda_mem_type_update(coda_dynamic_type **type, coda_type **definition)
{
    int i;

    assert((*type)->definition == *definition);

    switch ((*definition)->type_class)
    {
        case coda_record_class:
            {
                coda_mem_record *record_type = (coda_mem_record *)*type;

                if (record_type->num_fields < record_type->definition->num_fields)
                {
                    coda_dynamic_type **new_field_type;

                    /* increase the size for the child elements array until it matches the size in the definition */
                    new_field_type = realloc(record_type->field_type,
                                             record_type->definition->num_fields * sizeof(coda_dynamic_type *));
                    if (new_field_type == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       record_type->definition->num_fields * sizeof(coda_dynamic_type *), __FILE__,
                                       __LINE__);

                        return -1;
                    }
                    record_type->field_type = new_field_type;
                    for (i = record_type->num_fields; i < record_type->definition->num_fields; i++)
                    {
                        record_type->field_type[i] = NULL;
                    }
                    record_type->num_fields = record_type->definition->num_fields;
                }
                for (i = 0; i < record_type->definition->num_fields; i++)
                {
                    if (record_type->field_type[i] == NULL)
                    {
                        if (!record_type->definition->field[i]->optional)
                        {
                            record_type->definition->field[i]->optional = 1;
                        }
                    }
                    else
                    {
                        if (coda_dynamic_type_update(&record_type->field_type[i],
                                                     &record_type->definition->field[i]->type) != 0)
                        {
                            return -1;
                        }
                    }
                }
            }
            break;
        case coda_array_class:
            {
                coda_mem_array *array_type = (coda_mem_array *)*type;
                coda_type *element_definition = array_type->definition->base_type;

                for (i = 0; i < array_type->num_elements; i++)
                {
                    if (coda_dynamic_type_update(&array_type->element[i], &element_definition) != 0)
                    {
                        return -1;
                    }
                }
                /* we don't(/shouldn't have to) support modification of the base type definition of the array */
                assert(element_definition == array_type->definition->base_type);
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            break;
        case coda_special_class:
            if (coda_dynamic_type_update(&((coda_mem_special *)*type)->base_type,
                                         &((coda_type_special *)*definition)->base_type) != 0)
            {
                return -1;
            }
            break;
    }

    if (((coda_mem_type *)*type)->attributes == NULL && (*type)->definition->attributes != NULL)
    {
        ((coda_mem_type *)*type)->attributes =
            (coda_dynamic_type *)coda_mem_record_new((*type)->definition->attributes);
        if (((coda_mem_type *)*type)->attributes == NULL)
        {
            return -1;
        }
    }
    if (((coda_mem_type *)*type)->attributes != NULL)
    {
        if (coda_dynamic_type_update((coda_dynamic_type **)&((coda_mem_type *)*type)->attributes,
                                     (coda_type **)&(*type)->definition->attributes) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int create_attributes_record(coda_mem_type *type)
{
    if (type->definition->attributes != NULL)
    {
        type->attributes = (coda_dynamic_type *)coda_mem_record_new(type->definition->attributes);
        if (type->attributes == NULL)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_type_add_attribute(coda_mem_type *type, const char *real_name, coda_dynamic_type *attribute_type,
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
            type->attributes = (coda_dynamic_type *)coda_mem_record_new(type->definition->attributes);
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
        assert(((coda_type *)type->definition->attributes) == type->attributes->definition);
    }

    attributes = (coda_mem_record *)type->attributes;

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

int coda_mem_type_set_attributes(coda_mem_type *type, coda_dynamic_type *attributes, int update_definition)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (attributes == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attributes argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->attributes != NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attributes are already set (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (update_definition)
    {
        assert(attributes->definition->type_class == coda_record_class);
        if (coda_type_set_attributes(type->definition, (coda_type_record *)attributes->definition) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (((coda_type *)type->definition->attributes) != attributes->definition)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "trying to set attributes of incompatible type (%s:%u)",
                           __FILE__, __LINE__);
            return -1;
        }
    }

    type->attributes = attributes;

    return 0;
}

coda_mem_record *coda_mem_record_new(coda_type_record *definition)
{
    coda_mem_record *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    type = (coda_mem_record *)malloc(sizeof(coda_mem_record));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_record), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->num_fields = 0;
    type->field_type = NULL;
    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (definition->num_fields > 0)
    {
        long i;

        type->field_type = malloc(definition->num_fields * sizeof(coda_dynamic_type *));
        if (type->field_type == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           definition->num_fields * sizeof(coda_dynamic_type *), __FILE__, __LINE__);

            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < definition->num_fields; i++)
        {
            type->field_type[i] = NULL;
        }
        type->num_fields = definition->num_fields;
    }

    return type;
}

int coda_mem_record_add_field(coda_mem_record *type, const char *real_name, coda_dynamic_type *field_type,
                              int update_definition)
{
    long index = -1;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (update_definition)
    {
        if (coda_type_record_create_field(type->definition, real_name, field_type->definition) != 0)
        {
            return -1;
        }
        index = type->definition->num_fields - 1;
        if (type->num_fields < type->definition->num_fields)
        {
            coda_dynamic_type **new_field_type;
            long i;

            new_field_type = realloc(type->field_type, type->definition->num_fields * sizeof(coda_dynamic_type *));
            if (new_field_type == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               type->definition->num_fields * sizeof(coda_dynamic_type *), __FILE__, __LINE__);

                return -1;
            }
            type->field_type = new_field_type;
            for (i = type->num_fields; i < type->definition->num_fields; i++)
            {
                type->field_type[i] = NULL;
            }
            type->num_fields = type->definition->num_fields;
        }
    }
    else
    {
        index = hashtable_get_index_from_name(type->definition->real_name_hash_data, real_name);
        if (index < 0)
        {
            coda_set_error(CODA_ERROR_INVALID_NAME, "record does not have a field with name '%s' (%s:%u)", real_name,
                           __FILE__, __LINE__);
            return -1;
        }
        if (type->field_type[index] != NULL)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field '%s' is already set (%s:%u)", real_name, __FILE__,
                           __LINE__);
            return -1;
        }
        if (type->definition->field[index]->type != field_type->definition)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "trying to add field '%s' of incompatible type (%s:%u)",
                           real_name, __FILE__, __LINE__);
            return -1;
        }
    }
    type->field_type[index] = field_type;

    return 0;
}

int coda_mem_record_validate(coda_mem_record *type)
{
    long i;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    assert(type->num_fields == type->definition->num_fields);
    for (i = 0; i < type->num_fields; i++)
    {
        if (type->field_type[i] == NULL && !type->definition->field[i]->optional)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "mandatory field '%s' is missing",
                           type->definition->field[i]->name);
            return -1;
        }
    }
    return 0;
}

coda_mem_array *coda_mem_array_new(coda_type_array *definition)
{
    coda_mem_array *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    type = (coda_mem_array *)malloc(sizeof(coda_mem_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_array), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->num_elements = 0;
    type->element = NULL;

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (type->definition->num_elements > 0)
    {
        long i;

        type->element = malloc(BLOCK_SIZE * sizeof(coda_dynamic_type *));
        if (type->element == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           BLOCK_SIZE * sizeof(coda_dynamic_type *), __FILE__, __LINE__);
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->num_elements = type->definition->num_elements;
        for (i = 0; i < type->num_elements; i++)
        {
            type->element[i] = NULL;
        }
    }

    return type;
}

int coda_mem_array_set_element(coda_mem_array *type, long index, coda_dynamic_type *element)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (index < 0 || index >= type->num_elements)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "array index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       type->num_elements, __FILE__, __LINE__);
        return -1;
    }
    if (element == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "element argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->element[index] != NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "array element '%ld' is already set (%s:%u)", index, __FILE__,
                       __LINE__);
        return -1;
    }
    if (type->definition->base_type != element->definition)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "trying to set array element '%ld' of incompatible type (%s:%u)",
                       type->num_elements, __FILE__, __LINE__);
        return -1;
    }
    type->element[index] = element;

    return 0;
}

int coda_mem_array_add_element(coda_mem_array *type, coda_dynamic_type *element)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (element == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "element argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->definition->base_type != element->definition)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "trying to add array element '%ld' of incompatible type (%s:%u)",
                       type->num_elements, __FILE__, __LINE__);
        return -1;
    }
    if (type->num_elements % BLOCK_SIZE == 0)
    {
        coda_dynamic_type **new_element;

        new_element = realloc(type->element, (type->num_elements + BLOCK_SIZE) * sizeof(coda_dynamic_type *));
        if (new_element == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_elements + BLOCK_SIZE) * sizeof(coda_dynamic_type *), __FILE__, __LINE__);
            return -1;
        }
        type->element = new_element;
    }
    type->num_elements++;
    type->element[type->num_elements - 1] = element;

    return 0;
}

int coda_mem_array_validate(coda_mem_array *type)
{
    long i;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->definition->num_elements >= 0 && type->num_elements != type->definition->num_elements)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "number of actual array elements (%ld) does not match number of elements from definition (%ld)",
                       type->num_elements, type->definition->num_elements);
    }
    for (i = 0; i < type->num_elements; i++)
    {
        if (type->element[i] == NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "array element '%ld' is missing", i);
            return -1;
        }
    }
    return 0;
}

coda_mem_integer *coda_mem_integer_new(coda_type_number *definition, int64_t value)
{
    coda_mem_integer *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->type_class != coda_integer_class)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition is not an integer (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    type = (coda_mem_integer *)malloc(sizeof(coda_mem_integer));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_integer), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->value = value;

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_mem_real *coda_mem_real_new(coda_type_number *definition, double value)
{
    coda_mem_real *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition is not a floating point number (%s:%u)", __FILE__,
                       __LINE__);
        return NULL;
    }
    type = (coda_mem_real *)malloc(sizeof(coda_mem_real));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_real), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->value = value;

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_mem_text *coda_mem_char_new(coda_type_text *definition, char value)
{
    char text[2];

    text[0] = value;
    text[1] = '\0';

    return coda_mem_text_new(definition, text);
}

coda_mem_text *coda_mem_text_new(coda_type_text *definition, const char *text)
{
    coda_mem_text *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->bit_size >= 0)
    {
        long length;

        length = (long)(definition->bit_size >> 3) + (definition->bit_size & 0x7 ? 1 : 0);
        if (length != (long)strlen(text))
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length of text (%ld) does not match that of definition (%ld) "
                           "(%s:%u)", strlen(text), length, __FILE__, __LINE__);
            return NULL;
        }
    }
    if (definition->read_type == coda_native_type_char && strlen(text) != 1)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length of text (%ld) should be 1 for 'char' text (%s:%u)",
                       strlen(text), __FILE__, __LINE__);
        return NULL;
    }
    type = (coda_mem_text *)malloc(sizeof(coda_mem_text));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_text), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->text = strdup(text);
    if (type->text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_mem_raw *coda_mem_raw_new(coda_type_raw *definition, long length, const uint8_t *data)
{
    coda_mem_raw *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (length > 0 && data == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "data argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->bit_size >= 0)
    {
        long definition_length;

        definition_length = (long)(definition->bit_size >> 3) + (definition->bit_size & 0x7 ? 1 : 0);
        if (definition_length != length)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length of raw data (%ld) does not match that of definition "
                           "(%ld) (%s:%u)", length, definition_length, __FILE__, __LINE__);
            return NULL;
        }
    }
    type = (coda_mem_raw *)malloc(sizeof(coda_mem_raw));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_raw), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->length = length;
    type->data = NULL;
    if (length > 0)
    {
        type->data = malloc(length * sizeof(uint8_t));
        if (type->data == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(length * sizeof(uint8_t)), __FILE__, __LINE__);
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        memcpy(type->data, data, length);
    }

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_mem_time *coda_mem_time_new(coda_type_special *definition, double value, coda_dynamic_type *base_type)
{
    coda_mem_time *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->type_class != coda_special_class)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition is not a special type (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (((coda_type_special *)definition)->special_type != coda_special_time)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition is not a time type (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    type = (coda_mem_time *)malloc(sizeof(coda_mem_time));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_time), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->attributes = NULL;
    type->base_type = base_type;
    type->value = value;

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_mem_special *coda_mem_no_data_new(coda_format format)
{
    coda_mem_special *type;
    coda_type_raw *base_definition;

    type = (coda_mem_special *)malloc(sizeof(coda_mem_special));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_special), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = NULL;
    type->attributes = NULL;
    type->base_type = NULL;

    type->definition = coda_type_no_data_singleton(format);
    if (type->definition == NULL)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->definition->retain_count++;
    base_definition = (coda_type_raw *)((coda_type_special *)type->definition)->base_type;
    type->base_type = (coda_dynamic_type *)coda_mem_raw_new(base_definition, 0, NULL);
    if (type->base_type == NULL)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (create_attributes_record((coda_mem_type *)type) != 0)
    {
        coda_mem_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}
