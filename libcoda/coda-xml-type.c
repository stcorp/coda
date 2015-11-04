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

#include "coda-xml-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void coda_xml_type_delete(coda_dynamic_type *type)
{
    long i;

    assert(type != NULL);
    assert(type->backend == coda_backend_xml);

    switch (((coda_xml_type *)type)->tag)
    {
        case tag_xml_root:
            if (((coda_xml_root *)type)->element != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)((coda_xml_root *)type)->element);
            }
            break;
        case tag_xml_element:
            if (((coda_xml_element *)type)->xml_name != NULL)
            {
                free(((coda_xml_element *)type)->xml_name);
            }
            if (((coda_xml_element *)type)->attributes != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)((coda_xml_element *)type)->attributes);
            }
            if (((coda_xml_element *)type)->element != NULL)
            {
                assert(type->definition->type_class == coda_record_class);
                for (i = 0; i < ((coda_xml_element *)type)->num_elements; i++)
                {
                    if (((coda_xml_element *)type)->element[i] != NULL)
                    {
                        coda_dynamic_type_delete((coda_dynamic_type *)((coda_xml_element *)type)->element[i]);
                    }
                }
                free(((coda_xml_element *)type)->element);
            }
            break;
    }
    if (type->definition != NULL)
    {
        coda_type_release(type->definition);
    }
    free(type);
}

int coda_xml_type_update(coda_dynamic_type **type, coda_type **definition)
{
    long i;

    assert((*type)->backend == coda_backend_xml);

    if (((coda_xml_type *)*type)->tag == tag_xml_root)
    {
        coda_xml_root *root = (coda_xml_root *)*type;

        assert(((coda_type *)root->definition) == *definition);
        if (coda_dynamic_type_update((coda_dynamic_type **)&root->element,
                                     (coda_type **)&root->definition->field[0]->type) != 0)
        {
            return -1;
        }
    }
    else
    {
        coda_xml_element *element = (coda_xml_element *)*type;

        if (element->definition != *definition)
        {
            if ((*definition)->type_class == coda_array_class && (*definition)->format == coda_format_xml &&
                element->backend != coda_backend_memory)
            {
                /* convert the single element into an array of a single element */
                coda_mem_array *array = coda_mem_array_new((coda_type_array *)*definition);

                if (array == NULL)
                {
                    return -1;
                }
                /* first make sure that the array element is updated */
                if (coda_dynamic_type_update((coda_dynamic_type **)&element,
                                             &((coda_type_array *)*definition)->base_type) != 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)array);
                    return -1;
                }
                if (coda_mem_array_add_element(array, (coda_dynamic_type *)element) != 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)array);
                    return -1;
                }
                *type = (coda_dynamic_type *)array;
                /* finally update the array itself (for e.g. attributes) */
                return coda_dynamic_type_update(type, definition);
            }

            if ((*definition)->type_class == coda_text_class)
            {
                assert(element->definition->type_class == coda_record_class);
                coda_type_release(element->definition);
                element->definition = *definition;
                element->definition->retain_count++;
            }
            else
            {
                assert(element->definition->type_class == coda_text_class);
                /* this case handles updating the root xml element, where 'parent == NULL' */
                coda_type_release(*definition);
                *definition = element->definition;
                (*definition)->retain_count++;
            }
        }

        if (element->definition->type_class == coda_record_class && element->definition->format == coda_format_xml)
        {
            coda_type_record *record_definition;

            record_definition = (coda_type_record *)element->definition;
            if (element->num_elements < record_definition->num_fields)
            {
                coda_dynamic_type **new_element;

                /* increase the size for the child elements array until it matches the size in the definition */
                new_element = realloc(element->element, record_definition->num_fields * sizeof(coda_dynamic_type *));
                if (new_element == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   record_definition->num_fields * sizeof(coda_dynamic_type *), __FILE__, __LINE__);
                    return -1;
                }
                element->element = new_element;
                for (i = element->num_elements; i < record_definition->num_fields; i++)
                {
                    element->element[i] = NULL;
                }
                element->num_elements = record_definition->num_fields;
            }

            for (i = 0; i < element->num_elements; i++)
            {
                if (element->element[i] == NULL)
                {
                    /* update the optional flag for each child element */
                    if (!record_definition->field[i]->optional)
                    {
                        record_definition->field[i]->optional = 1;
                    }
                }
                else
                {
                    /* update child element */
                    if (coda_dynamic_type_update(&element->element[i], &record_definition->field[i]->type) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
        else
        {
            /* we can remove all sub elements */
            if (element->element != NULL)
            {
                for (i = 0; i < element->num_elements; i++)
                {
                    if (element->element[i] != NULL)
                    {
                        coda_dynamic_type_delete(element->element[i]);
                    }
                }
                free(element->element);
                element->element = NULL;
            }
            element->num_elements = 0;
        }

        if (element->attributes == NULL && element->definition->attributes != NULL)
        {
            element->attributes = coda_mem_record_new(element->definition->attributes);
            if (element->attributes == NULL)
            {
                return -1;
            }
        }
        if (element->attributes != NULL)
        {
            if (coda_dynamic_type_update((coda_dynamic_type **)&element->attributes,
                                         (coda_type **)&element->definition->attributes) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

static coda_mem_record *attribute_record_new(coda_type_record *definition, const char **attr, int update_definition)
{
    coda_mem_record *attributes;
    int attribute_index;
    int i;

    assert(definition != NULL);
    attributes = coda_mem_record_new(definition);

    /* add attributes to attribute list */
    for (i = 0; attr[2 * i] != NULL; i++)
    {
        coda_mem_text *attribute;
        int update_mem_record = update_definition;

        attribute_index = hashtable_get_index_from_name(definition->real_name_hash_data, attr[2 * i]);
        if (update_definition)
        {
            if (attribute_index < 0)
            {
                coda_type_text *attribute_definition;

                attribute_definition = coda_type_text_new(coda_format_xml);
                if (attribute_definition == NULL)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                    return NULL;
                }
                attribute = coda_mem_text_new(attribute_definition, attr[2 * i + 1]);
                coda_type_release((coda_type *)attribute_definition);
            }
            else if (attributes->field_type[attribute_index] != NULL)
            {
                /* we only add the first occurence when there are multiple attributes with the same attribute name */
                continue;
            }
            else
            {
                attribute = coda_mem_text_new((coda_type_text *)definition->field[attribute_index]->type,
                                              attr[2 * i + 1]);
                update_mem_record = 0;
            }
        }
        else
        {
            if (attribute_index == -1)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml attribute '%s' is not allowed", attr[2 * i]);
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
            attribute = coda_mem_text_new((coda_type_text *)definition->field[attribute_index]->type, attr[2 * i + 1]);
        }
        if (attribute == NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            return NULL;
        }

        if (coda_mem_record_add_field(attributes, attr[2 * i], (coda_dynamic_type *)attribute, update_mem_record) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attribute);
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            return NULL;
        }
    }

    for (i = 0; i < definition->num_fields; i++)
    {
        if (!definition->field[i]->optional && attributes->field_type[i] == NULL)
        {
            if (update_definition)
            {
                definition->field[i]->optional = 1;
            }
            else
            {
                const char *real_name;

                coda_type_get_record_field_real_name((coda_type *)definition, i, &real_name);
                coda_set_error(CODA_ERROR_PRODUCT, "mandatory xml attribute '%s' is missing", real_name);
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
        }
    }

    return attributes;
}

coda_xml_root *coda_xml_root_new(coda_type_record *definition)
{
    coda_xml_root *root;

    root = malloc(sizeof(coda_xml_root));
    if (root == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xml_root), __FILE__, __LINE__);
        return NULL;
    }
    root->backend = coda_backend_xml;
    root->definition = definition;
    definition->retain_count++;
    root->tag = tag_xml_root;
    root->element = NULL;

    return root;
}

static coda_xml_element *xml_element_new(coda_type *definition, const char *xml_name, const char **attr,
                                         int update_definition)
{
    coda_xml_element *element;

    assert(definition != NULL);
    assert(xml_name != NULL);
    assert(attr != NULL);

    element = malloc(sizeof(coda_xml_element));
    if (element == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xml_element), __FILE__, __LINE__);
        return NULL;
    }
    element->backend = coda_backend_xml;
    element->definition = definition;
    definition->retain_count++;
    element->tag = tag_xml_element;
    element->xml_name = NULL;
    element->inner_bit_offset = 0;
    element->inner_bit_size = 0;
    element->outer_bit_offset = 0;
    element->outer_bit_size = 0;
    element->cdata_delta_offset = 0;
    element->cdata_delta_size = 0;
    element->attributes = NULL;
    element->num_elements = 0;
    element->element = NULL;
    element->parent = NULL;

    element->xml_name = strdup(xml_name);
    if (element->xml_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_xml_type_delete((coda_dynamic_type *)element);
        return NULL;
    }

    if (definition->type_class == coda_record_class && ((coda_type_record *)definition)->num_fields > 0)
    {
        long num_fields;
        long i;

        /* construct the sub element list from the definition */
        num_fields = ((coda_type_record *)definition)->num_fields;
        element->element = malloc(num_fields * sizeof(coda_xml_type *));
        if (element->element == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           num_fields * sizeof(coda_xml_type *), __FILE__, __LINE__);
            coda_xml_type_delete((coda_dynamic_type *)element);
            return NULL;
        }
        for (i = 0; i < num_fields; i++)
        {
            element->element[i] = NULL;
        }
        element->num_elements = num_fields;

        /* create empty arrays for array child elements */
        for (i = 0; i < num_fields; i++)
        {
            if (((coda_type_record *)definition)->field[i]->type->type_class == coda_array_class &&
                ((coda_type_record *)definition)->field[i]->type->format == coda_format_xml)
            {
                coda_type *array_definition = ((coda_type_record *)definition)->field[i]->type;

                element->element[i] = (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)array_definition);
                if (element->element[i] == NULL)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return NULL;
                }
            }
        }
    }

    if (definition->attributes == NULL)
    {
        if (attr[0] != NULL)
        {
            if (update_definition)
            {
                definition->attributes = coda_type_record_new(coda_format_xml);
                if (definition->attributes == NULL)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return NULL;
                }
                element->attributes = attribute_record_new(definition->attributes, attr, update_definition);
                if (element->attributes == NULL)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return NULL;
                }
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml attribute '%s' is not allowed", attr[0]);
                coda_xml_type_delete((coda_dynamic_type *)element);
                return NULL;
            }
        }
    }
    else
    {
        element->attributes = attribute_record_new(definition->attributes, attr, update_definition);
        if (element->attributes == NULL)
        {
            coda_xml_type_delete((coda_dynamic_type *)element);
            return NULL;
        }
    }

    return element;
}

int coda_xml_root_add_element(coda_xml_root *root, const char *el, const char **attr, int64_t outer_bit_offset,
                              int64_t inner_bit_offset, int update_definition)
{
    coda_xml_element *element;
    coda_type *element_definition;
    int index;

    assert(root != NULL);
    assert(el != NULL);

    /* check if a definition for this element is available */
    index = hashtable_get_index_from_name(root->definition->real_name_hash_data, el);
    if (index < 0)
    {
        index = hashtable_get_index_from_name(root->definition->real_name_hash_data,
                                              coda_element_name_from_xml_name(el));
    }
    if (index < 0)
    {
        if (update_definition)
        {
            /* all xml elements start out as empty records */
            element_definition = (coda_type *)coda_type_record_new(coda_format_xml);
            if (element_definition == NULL)
            {
                return -1;
            }
            if (coda_type_record_create_field(root->definition, el, element_definition) != 0)
            {
                coda_type_release(element_definition);
                return -1;
            }
            coda_type_release(element_definition);
        }
        else
        {
            coda_set_error(CODA_ERROR_PRODUCT, "incorrect root element '%s' for product", el);
            return -1;
        }
    }
    assert(root->definition->num_fields == 1);
    element_definition = root->definition->field[0]->type;
    /* the root element can not be an array of xml elements */
    assert(element_definition->type_class != coda_array_class || element_definition->format != coda_format_xml);

    /* create a new element */
    element = xml_element_new(element_definition, el, attr, update_definition);
    if (element == NULL)
    {
        return -1;
    }
    element->outer_bit_offset = outer_bit_offset;
    element->inner_bit_offset = inner_bit_offset;
    root->element = element;

    return 0;
}

int coda_xml_element_add_element(coda_xml_element *parent, const char *el, const char **attr, int64_t outer_bit_offset,
                                 int64_t inner_bit_offset, int update_definition, coda_xml_element **new_element)
{
    coda_xml_element *element;
    coda_type *element_definition;
    long index;

    assert(parent != NULL);
    assert(el != NULL);
    assert(parent->definition->type_class == coda_record_class);
    assert(parent->definition->format == coda_format_xml);

    /* check if a definition for this element is available */
    index = hashtable_get_index_from_name(((coda_type_record *)parent->definition)->real_name_hash_data, el);
    if (index < 0)
    {
        index = hashtable_get_index_from_name(((coda_type_record *)parent->definition)->real_name_hash_data,
                                              coda_element_name_from_xml_name(el));
    }
    if (index < 0)
    {
        if (update_definition)
        {
            coda_dynamic_type **new_element;
            coda_type_record *record_definition = (coda_type_record *)parent->definition;
            long i;

            /* all xml elements start out as empty records */
            element_definition = (coda_type *)coda_type_record_new(coda_format_xml);
            if (element_definition == NULL)
            {
                return -1;
            }
            if (coda_type_record_create_field(record_definition, el, element_definition) != 0)
            {
                coda_type_release(element_definition);
                return -1;
            }
            coda_type_release(element_definition);
            new_element = realloc(parent->element, record_definition->num_fields * sizeof(coda_dynamic_type *));
            if (new_element == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               record_definition->num_fields * sizeof(coda_dynamic_type *), __FILE__, __LINE__);
                return -1;
            }
            parent->element = new_element;
            for (i = parent->num_elements; i < record_definition->num_fields; i++)
            {
                parent->element[i] = NULL;
            }
            parent->num_elements = record_definition->num_fields;
            index = record_definition->num_fields - 1;
        }
        else
        {
            coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed within element '%s'", el,
                           parent->xml_name);
            return -1;
        }
    }
    element_definition = ((coda_type_record *)parent->definition)->field[index]->type;
    if (element_definition->type_class == coda_array_class && element_definition->format == coda_format_xml)
    {
        /* take the array element definition */
        element_definition = ((coda_type_array *)element_definition)->base_type;
    }

    /* create a new element */
    element = xml_element_new(element_definition, el, attr, update_definition);
    if (element == NULL)
    {
        return -1;
    }
    element->outer_bit_offset = outer_bit_offset;
    element->inner_bit_offset = inner_bit_offset;

    if (parent->element[index] != NULL)
    {
        if (parent->element[index]->definition->type_class == coda_array_class &&
            parent->element[index]->definition->format == coda_format_xml)
        {
            /* add the child element to the array */
            if (coda_mem_array_add_element((coda_mem_array *)parent->element[index], (coda_dynamic_type *)element) != 0)
            {
                coda_xml_type_delete((coda_dynamic_type *)element);
                return -1;
            }
        }
        else
        {
            if (update_definition)
            {
                coda_mem_array *array;
                coda_type_array *array_definition;

                /* change scalar to array in definition */
                array_definition = coda_type_array_new(coda_format_xml);
                if (array_definition == NULL)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
                if (coda_type_array_set_base_type(array_definition, element->definition) != 0)
                {
                    coda_type_release((coda_type *)array_definition);
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
                if (coda_type_array_add_variable_dimension(array_definition, NULL) != 0)
                {
                    coda_type_release((coda_type *)array_definition);
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
                ((coda_type_record *)parent->definition)->field[index]->type = (coda_type *)array_definition;
                coda_type_release(element->definition);
                array = coda_mem_array_new(array_definition);
                if (array == NULL)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
                if (coda_mem_array_add_element(array, parent->element[index]) != 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)array);
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
                parent->element[index] = (coda_dynamic_type *)array;
                /* add the child element to the array */
                if (coda_mem_array_add_element(array, (coda_dynamic_type *)element) != 0)
                {
                    coda_xml_type_delete((coda_dynamic_type *)element);
                    return -1;
                }
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed more than once within element '%s'",
                               element->xml_name, parent->xml_name);
                coda_xml_type_delete((coda_dynamic_type *)element);
                return -1;
            }
        }
    }
    else
    {
        parent->element[index] = (coda_dynamic_type *)element;
    }

    /* couple the child to the parent */
    element->parent = parent;

    *new_element = element;
    return 0;
}

int coda_xml_element_convert_to_text(coda_xml_element *element)
{
    coda_type *definition;

    assert(element->definition->type_class == coda_record_class && element->definition->format == coda_format_xml);

    definition = (coda_type *)coda_type_text_new(coda_format_xml);
    if (definition == NULL)
    {
        return -1;
    }
    if (element->definition->attributes != NULL)
    {
        definition->attributes = element->definition->attributes;
        element->definition->attributes->retain_count++;
    }
    coda_type_release(element->definition);
    element->definition = definition;

    if (element->parent != NULL)
    {
        coda_type **definition_handle;
        long index;

        index = hashtable_get_index_from_name(((coda_type_record *)element->parent->definition)->real_name_hash_data,
                                              element->xml_name);
        assert(index >= 0);
        definition_handle = &((coda_type_record *)element->parent->definition)->field[index]->type;
        if ((*definition_handle)->type_class == coda_array_class)
        {
            definition_handle = &((coda_type_array *)(*definition_handle))->base_type;
        }
        coda_type_release(*definition_handle);
        *definition_handle = definition;
        definition->retain_count++;
    }

    if (element->element != NULL)
    {
        long i;

        for (i = 0; i < element->num_elements; i++)
        {
            coda_dynamic_type_delete(element->element[i]);
        }
        free(element->element);
        element->element = NULL;
    }
    element->num_elements = 0;

    return 0;
}

int coda_xml_element_validate(coda_xml_element *element)
{
    if (element->definition->type_class == coda_record_class)
    {
        coda_type_record *definition = (coda_type_record *)element->definition;
        int i;

        /* verify occurence of mandatory elements */
        for (i = 0; i < definition->num_fields; i++)
        {
            if (element->element[i] == NULL)
            {
                if (!definition->field[i]->optional)
                {
                    const char *real_name;

                    coda_type_get_record_field_real_name((coda_type *)definition, i, &real_name);
                    coda_set_error(CODA_ERROR_PRODUCT, "mandatory xml element '%s' is missing", real_name);
                    return -1;
                }
            }
        }
    }

    return 0;
}
