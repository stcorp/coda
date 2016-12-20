/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include "coda-mem-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void coda_mem_type_delete(coda_dynamic_type *type)
{
    int i;

    assert(type != NULL);
    assert(type->backend == coda_backend_memory);

    switch (((coda_mem_type *)type)->tag)
    {
        case tag_mem_record:
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
        case tag_mem_array:
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
        case tag_mem_data:
            break;
        case tag_mem_special:
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

/* run on product root type after setting up dynamic type tree (without having used definition from data dictionary!)
 * this function will then internally be called recursively to update the whole dynamic type tree.
 * certain aspects of the definition (e.g. optional availability of fields) may also still be modified.
 */
int coda_mem_type_update(coda_dynamic_type **type, coda_type *definition)
{
    coda_mem_type *mem_type;
    int i;

    if ((*type)->backend == coda_backend_ascii || (*type)->backend == coda_backend_binary)
    {
        assert(coda_get_type_for_dynamic_type(*type) == definition);
        return 0;
    }

    assert((*type)->backend == coda_backend_memory);

    if ((*type)->definition != definition)
    {
        if (definition->type_class == coda_array_class && (*type)->definition->type_class != coda_array_class)
        {
            assert(definition->format == coda_format_xml);

            /* convert the single element into an array of a single element */
            mem_type = (coda_mem_type *)coda_mem_array_new((coda_type_array *)definition, NULL);
            if (mem_type == NULL)
            {
                return -1;
            }
            /* make sure that the array element is updated to allow it to be added to the array */
            if (coda_mem_type_update(type, ((coda_type_array *)definition)->base_type) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)mem_type);
                return -1;
            }
            if (coda_mem_array_add_element((coda_mem_array *)mem_type, *type) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)mem_type);
                return -1;
            }
            *type = (coda_dynamic_type *)mem_type;

            /* finally update the array itself (for e.g. attributes) */
            return coda_mem_type_update(type, definition);
        }

        if ((*type)->definition->type_class == coda_record_class && definition->type_class == coda_text_class)
        {
            assert((*type)->definition->format == coda_format_xml);
            assert(((coda_type_record *)(*type)->definition)->num_fields == 0);

            /* convert record to text */
            mem_type = (coda_mem_type *)coda_mem_string_new((coda_type_text *)definition, NULL, NULL, NULL);
            mem_type->attributes = ((coda_mem_record *)*type)->attributes;
            ((coda_mem_type *)*type)->attributes = NULL;
            coda_dynamic_type_delete(*type);
            *type = (coda_dynamic_type *)mem_type;
        }
        else
        {
            assert(0);
            exit(1);
        }
    }

    mem_type = (coda_mem_type *)*type;

    switch (mem_type->tag)
    {
        case tag_mem_record:
            {
                coda_mem_record *record_type = (coda_mem_record *)mem_type;

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
                        if (coda_mem_type_update(&record_type->field_type[i],
                                                 record_type->definition->field[i]->type) != 0)
                        {
                            return -1;
                        }
                    }
                }
            }
            break;
        case tag_mem_array:
            {
                for (i = 0; i < ((coda_mem_array *)mem_type)->num_elements; i++)
                {
                    if (coda_mem_type_update(&((coda_mem_array *)mem_type)->element[i],
                                             ((coda_mem_array *)mem_type)->definition->base_type) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case tag_mem_data:
            break;
        case tag_mem_special:
            if (coda_mem_type_update(&((coda_mem_special *)mem_type)->base_type,
                                     ((coda_mem_special *)mem_type)->definition->base_type) != 0)
            {
                return -1;
            }
            break;
    }

    if (mem_type->attributes == NULL && mem_type->definition->attributes != NULL)
    {
        mem_type->attributes = (coda_dynamic_type *)coda_mem_record_new(mem_type->definition->attributes, NULL);
        if (mem_type->attributes == NULL)
        {
            return -1;
        }
    }
    if (mem_type->attributes != NULL)
    {
        if (coda_mem_type_update((coda_dynamic_type **)&mem_type->attributes,
                                 (coda_type *)mem_type->definition->attributes) != 0)
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
        type->attributes = (coda_dynamic_type *)coda_mem_record_new(type->definition->attributes, NULL);
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
            type->attributes = (coda_dynamic_type *)coda_mem_record_new(type->definition->attributes, NULL);
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

coda_mem_record *coda_mem_record_new(coda_type_record *definition, coda_dynamic_type *attributes)
{
    coda_mem_record *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->is_union && definition->union_field_expr != NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "union definition with field expression is not allowed for memory backend");
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
    type->tag = tag_mem_record;
    type->attributes = attributes;
    type->num_fields = 0;
    type->field_type = NULL;

    if (type->attributes == NULL)
    {
        if (create_attributes_record((coda_mem_type *)type) != 0)
        {
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
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

coda_mem_array *coda_mem_array_new(coda_type_array *definition, coda_dynamic_type *attributes)
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
    type->tag = tag_mem_array;
    type->attributes = attributes;
    type->num_elements = 0;
    type->element = NULL;

    if (attributes == NULL)
    {
        if (create_attributes_record((coda_mem_type *)type) != 0)
        {
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    if (type->definition->num_elements > 0)
    {
        long i;

        type->element = malloc(type->definition->num_elements * sizeof(coda_dynamic_type *));
        if (type->element == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           type->definition->num_elements * sizeof(coda_dynamic_type *), __FILE__, __LINE__);
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

coda_mem_data *coda_mem_data_new(coda_type *definition, coda_dynamic_type *attributes, coda_product *product,
                                 long length, const uint8_t *data)
{
    coda_mem_data *type;
    long current_num_blocks;
    long new_num_blocks;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    assert(length >= 0);
    if (length > 0 && data == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "data argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->bit_size >= 0)
    {
        long expected_length;

        expected_length = (long)(definition->bit_size >> 3) + (definition->bit_size & 0x7 ? 1 : 0);
        if (expected_length != length)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "length of data (%ld) does not match that of definition (%ld)", length,
                           expected_length);
            return NULL;
        }
    }
    if (definition->read_type == coda_native_type_char && length != 1)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length of text (%ld) should be 1 for 'char' text (%s:%u)", length,
                       __FILE__, __LINE__);
        return NULL;
    }

    type = (coda_mem_data *)malloc(sizeof(coda_mem_data));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_data), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->tag = tag_mem_data;
    type->attributes = attributes;
    type->length = length;
    type->offset = 0;

    if (length > 0)
    {
        if (product == NULL)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product argument is NULL (%s:%u)", __FILE__, __LINE__);
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        /* num_blocks = ((mem_size - 1) / block_size) + 1, since we need to round up */
        current_num_blocks = (long)(product->mem_size == 0 ? 0 : ((product->mem_size - 1) / DATA_BLOCK_SIZE) + 1);
        new_num_blocks =
            (long)(length == 0 ? current_num_blocks : ((product->mem_size + length - 1) / DATA_BLOCK_SIZE) + 1);
        if (new_num_blocks > current_num_blocks)
        {
            uint8_t *new_mem_ptr;

            new_mem_ptr = (uint8_t *)realloc(product->mem_ptr, new_num_blocks * DATA_BLOCK_SIZE);
            if (new_mem_ptr == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                               new_num_blocks * DATA_BLOCK_SIZE, __FILE__, __LINE__);
                coda_mem_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            product->mem_ptr = new_mem_ptr;
        }
        type->offset = product->mem_size;
        memcpy(&product->mem_ptr[product->mem_size], data, (size_t)length);
        product->mem_size += length;
    }

    if (type->attributes == NULL)
    {
        if (create_attributes_record((coda_mem_type *)type) != 0)
        {
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    return type;
}

coda_mem_data *coda_mem_int8_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                 int8_t value)
{
    assert(definition->bit_size == 8);
    assert(definition->read_type == coda_native_type_int8);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(int8_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_uint8_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  uint8_t value)
{
    assert(definition->bit_size == 8);
    assert(definition->read_type == coda_native_type_uint8);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(uint8_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_int16_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int16_t value)
{
    assert(definition->bit_size == 16);
    assert(definition->read_type == coda_native_type_int16);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(int16_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_uint16_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint16_t value)
{
    assert(definition->bit_size == 16);
    assert(definition->read_type == coda_native_type_uint16);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(uint16_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_int32_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int32_t value)
{
    assert(definition->bit_size == 32);
    assert(definition->read_type == coda_native_type_int32);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(int32_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_uint32_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint32_t value)
{
    assert(definition->bit_size == 32);
    assert(definition->read_type == coda_native_type_uint32);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(uint32_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_int64_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int64_t value)
{
    assert(definition->bit_size == 64);
    assert(definition->read_type == coda_native_type_int64);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(int64_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_uint64_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint64_t value)
{
    assert(definition->bit_size == 64);
    assert(definition->read_type == coda_native_type_uint64);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(uint64_t), (uint8_t *)&value);
}

coda_mem_data *coda_mem_float_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  float value)
{
    assert(definition->bit_size == 32);
    assert(definition->read_type == coda_native_type_float);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(float), (uint8_t *)&value);
}

coda_mem_data *coda_mem_double_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   double value)
{
    assert(definition->bit_size == 64);
    assert(definition->read_type == coda_native_type_double);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(double), (uint8_t *)&value);
}

coda_mem_data *coda_mem_char_new(coda_type_text *definition, coda_dynamic_type *attributes, coda_product *product,
                                 char value)
{
    assert(definition->bit_size == 8);
    assert(definition->read_type == coda_native_type_char);
    return coda_mem_data_new((coda_type *)definition, attributes, product, sizeof(char), (uint8_t *)&value);
}

coda_mem_data *coda_mem_string_new(coda_type_text *definition, coda_dynamic_type *attributes, coda_product *product,
                                   const char *str)
{
    assert(definition->read_type == coda_native_type_string);
    return coda_mem_data_new((coda_type *)definition, attributes, product, str == NULL ? 0 : strlen(str),
                             (const uint8_t *)str);
}

coda_mem_data *coda_mem_raw_new(coda_type_raw *definition, coda_dynamic_type *attributes, coda_product *product,
                                long length, const uint8_t *data)
{
    assert(definition->type_class == coda_raw_class);
    return coda_mem_data_new((coda_type *)definition, attributes, product, length, data);
}

coda_mem_special *coda_mem_time_new(coda_type_special *definition, coda_dynamic_type *attributes,
                                    coda_dynamic_type *base_type)
{
    coda_mem_special *type;

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
    if (((coda_type_special *)definition)->base_type != base_type->definition)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition of base type should be the same as base type of "
                       "definition (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    type = (coda_mem_special *)malloc(sizeof(coda_mem_special));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_mem_special), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_memory;
    type->definition = definition;
    definition->retain_count++;
    type->tag = tag_mem_special;
    type->attributes = attributes;
    type->base_type = base_type;

    if (type->attributes == NULL)
    {
        if (create_attributes_record((coda_mem_type *)type) != 0)
        {
            coda_mem_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
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
    type->tag = tag_mem_special;
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
    type->base_type = (coda_dynamic_type *)coda_mem_raw_new(base_definition, NULL, NULL, 0, NULL);
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
