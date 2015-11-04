/*
 * Copyright (C) 2007-2008 S&T, The Netherlands.
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

#include "coda-xml-definition.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void delete_xml_dynamic_attribute(coda_xmlAttributeDynamicType *attribute)
{
    assert(attribute->retain_count == 0);
    if (attribute->type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)attribute->type);
    }
    if (attribute->value != NULL)
    {
        free(attribute->value);
    }
    free(attribute);
}

static void delete_xml_dynamic_attribute_record(coda_xmlAttributeRecordDynamicType *attributes)
{
    assert(attributes->retain_count == 0);
    if (attributes->type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)attributes->type);
    }
    if (attributes->attribute != NULL)
    {
        int i;

        for (i = 0; i < attributes->num_attributes; i++)
        {
            if (attributes->attribute[i] != NULL)
            {
                coda_xml_release_dynamic_type((coda_xmlDynamicType *)attributes->attribute[i]);
            }
        }
        free(attributes->attribute);
    }
    free(attributes);
}

static void delete_xml_dynamic_element(coda_xmlElementDynamicType *element)
{
    assert(element->retain_count == 0);
    if (element->type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)element->type);
    }
    if (element->attributes != NULL)
    {
        coda_xml_release_dynamic_type((coda_xmlDynamicType *)element->attributes);
    }
    if (element->element != NULL)
    {
        int i;

        for (i = 0; i < element->num_elements; i++)
        {
            if (element->element[i] != NULL)
            {
                coda_xml_release_dynamic_type(element->element[i]);
            }
        }
        free(element->element);
    }
    free(element);
}

static void delete_xml_dynamic_array(coda_xmlArrayDynamicType *array)
{
    assert(array->retain_count == 0);
    if (array->type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)array->type);
    }
    if (array->element != NULL)
    {
        int i;

        for (i = 0; i < array->num_elements; i++)
        {
            if (array->element[i] != NULL)
            {
                coda_xml_release_dynamic_type((coda_xmlDynamicType *)array->element[i]);
            }
        }
        free(array->element);
    }
    free(array);
}

void delete_xml_dynamic_root(coda_xmlRootDynamicType *root)
{
    assert(root->retain_count == 0);
    if (root->type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)root->type);
    }
    if (root->element != NULL)
    {
        coda_xml_release_dynamic_type((coda_xmlDynamicType *)root->element);
    }
    free(root);
}

void coda_xml_release_dynamic_type(coda_xmlDynamicType *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (type->tag)
    {
        case tag_xml_root_dynamic:
            delete_xml_dynamic_root((coda_xmlRootDynamicType *)type);
            break;
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
            delete_xml_dynamic_element((coda_xmlElementDynamicType *)type);
            break;
        case tag_xml_array_dynamic:
            delete_xml_dynamic_array((coda_xmlArrayDynamicType *)type);
            break;
        case tag_xml_attribute_dynamic:
            delete_xml_dynamic_attribute((coda_xmlAttributeDynamicType *)type);
            break;
        case tag_xml_attribute_record_dynamic:
            delete_xml_dynamic_attribute_record((coda_xmlAttributeRecordDynamicType *)type);
            break;
    }
}

coda_xmlRootDynamicType *coda_xml_dynamic_root_new(coda_xmlRoot *type)
{
    coda_xmlRootDynamicType *root;

    root = malloc(sizeof(coda_xmlRootDynamicType));
    if (root == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xmlRootDynamicType), __FILE__, __LINE__);
        return NULL;
    }
    root->retain_count = 0;
    root->format = coda_format_xml;
    root->type_class = coda_record_class;
    root->tag = tag_xml_root_dynamic;
    root->type = type;
    type->retain_count++;
    root->element = NULL;

    return root;
}

static coda_xmlAttributeDynamicType *coda_xml_dynamic_attribute_new(coda_xmlAttribute *type, const char *value)
{
    coda_xmlAttributeDynamicType *attribute;

    assert(value != NULL);

    attribute = malloc(sizeof(coda_xmlAttributeDynamicType));
    if (attribute == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xmlAttributeDynamicType), __FILE__, __LINE__);
        return NULL;
    }
    attribute->retain_count = 0;
    attribute->format = coda_format_xml;
    attribute->type_class = coda_text_class;
    attribute->tag = tag_xml_attribute_dynamic;
    attribute->type = type;
    type->retain_count++;
    attribute->value = NULL;

    attribute->value = strdup(value);
    if (attribute->value == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_dynamic_attribute(attribute);
        return NULL;
    }

    return attribute;
}

coda_xmlAttributeRecordDynamicType *coda_xml_dynamic_attribute_record_new(coda_xmlAttributeRecord *type,
                                                                          const char **attr)
{
    coda_xmlAttributeRecordDynamicType *attributes;
    int attribute_index;
    int i;

    attributes = malloc(sizeof(coda_xmlAttributeRecordDynamicType));
    if (attributes == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xmlAttributeRecordDynamicType), __FILE__, __LINE__);
        return NULL;
    }
    attributes->retain_count = 0;
    attributes->format = coda_format_xml;
    attributes->type_class = coda_record_class;
    attributes->tag = tag_xml_attribute_record_dynamic;
    attributes->type = type;
    type->retain_count++;
    attributes->num_attributes = 0;
    attributes->attribute = NULL;

    /* construct an empty attribute list from the definition */
    attributes->attribute = malloc(type->num_attributes * sizeof(coda_xmlAttributeDynamicType *));
    if (attributes->attribute == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       type->num_attributes * sizeof(coda_xmlAttributeDynamicType *), __FILE__, __LINE__);
        delete_xml_dynamic_attribute_record(attributes);
        return NULL;
    }
    attributes->num_attributes = type->num_attributes;
    for (i = 0; i < attributes->num_attributes; i++)
    {
        attributes->attribute[i] = NULL;
    }

    if (attr != NULL)
    {
        /* add attributes to attribute list */
        for (i = 0; attr[2 * i] != NULL; i++)
        {
            attribute_index = hashtable_get_index_from_name(type->attribute_name_hash_data, attr[2 * i]);
            if (attribute_index < 0)
            {
                attribute_index = hashtable_get_index_from_name(type->attribute_name_hash_data,
                                                                coda_element_name_from_xml_name(attr[2 * i]));
            }
            if (attribute_index == -1)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml attribute '%s' is not allowed", attr[2 * i]);
                delete_xml_dynamic_attribute_record(attributes);
                return NULL;
            }

            /* we only include the first attribute when there are multiple attributes with the same attribute name */
            if (attributes->attribute[attribute_index] == NULL)
            {
                attributes->attribute[attribute_index] =
                    coda_xml_dynamic_attribute_new(type->attribute[attribute_index], attr[2 * i + 1]);
                if (attributes->attribute[attribute_index] == NULL)
                {
                    delete_xml_dynamic_attribute_record(attributes);
                    return NULL;
                }
            }
        }
    }

    /* validate attributes */
    for (i = 0; i < attributes->num_attributes; i++)
    {
        if (!type->attribute[i]->optional && attributes->attribute[i] == NULL)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "mandatory xml attribute '%s' is missing", type->attribute[i]->xml_name);
            delete_xml_dynamic_attribute_record(attributes);
            return NULL;
        }
    }

    return attributes;
}

static int coda_xml_dynamic_attribute_record_update(coda_xmlAttributeRecordDynamicType *attributes)
{
    /* update size of attribute list */
    if (attributes->type->num_attributes > attributes->num_attributes)
    {
        coda_xmlAttributeDynamicType **new_attribute;
        int i;

        new_attribute = realloc(attributes->attribute, attributes->type->num_attributes * sizeof(coda_xmlAttribute *));
        if (new_attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           attributes->type->num_attributes * sizeof(coda_xmlAttributeDynamicType *), __FILE__,
                           __LINE__);
            return -1;
        }
        attributes->attribute = new_attribute;
        for (i = attributes->num_attributes; i < attributes->type->num_attributes; i++)
        {
            attributes->attribute[i] = NULL;
        }
        attributes->num_attributes = attributes->type->num_attributes;
    }

    return 0;
}

coda_xmlArrayDynamicType *coda_xml_dynamic_array_new(coda_xmlArray *type)
{
    coda_xmlArrayDynamicType *array;

    assert(type != NULL);

    array = malloc(sizeof(coda_xmlArrayDynamicType));
    if (array == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlArrayDynamicType), __FILE__, __LINE__);
        return NULL;
    }
    array->retain_count = 0;
    array->format = coda_format_xml;
    array->type_class = coda_array_class;
    array->tag = tag_xml_array_dynamic;
    array->type = type;
    type->retain_count++;
    array->num_elements = 0;
    array->element = NULL;

    return array;
}

int coda_xml_dynamic_array_add_element(coda_xmlArrayDynamicType *array, coda_xmlElementDynamicType *element)
{
    coda_xmlElementDynamicType **elementarray;

    assert(array != NULL);
    assert(element != NULL);
    assert(element->type == array->type->base_type);

    elementarray = realloc(array->element, (array->num_elements + 1) * sizeof(coda_xmlElementDynamicType *));
    if (elementarray == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (array->num_elements + 1) * sizeof(coda_xmlElementDynamicType *), __FILE__, __LINE__);
        return -1;
    }
    array->element = elementarray;
    array->element[array->num_elements] = element;
    element->retain_count++;
    array->num_elements++;

    return 0;
}

coda_xmlElementDynamicType *coda_xml_dynamic_element_new(coda_xmlElement *type, const char **attr)
{
    coda_xmlElementDynamicType *element;

    assert(type != NULL);
    assert(attr != NULL);

    element = malloc(sizeof(coda_xmlElementDynamicType));
    if (element == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlElementDynamicType), __FILE__, __LINE__);
        return NULL;
    }
    element->retain_count = 0;
    element->format = coda_format_xml;
    element->type_class = type->type_class;
    switch (type->tag)
    {
        case tag_xml_record:
            element->tag = tag_xml_record_dynamic;
            break;
        case tag_xml_text:
            element->tag = tag_xml_text_dynamic;
            break;
        case tag_xml_ascii_type:
            element->tag = tag_xml_ascii_type_dynamic;
            break;
        default:
            assert(0);
            exit(1);
    }
    element->type = type;
    type->retain_count++;
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

    if (element->tag == tag_xml_record_dynamic && type->num_fields > 0)
    {
        int i;

        /* construct the sub element list from the definition */

        element->element = malloc(type->num_fields * sizeof(coda_xmlDynamicType *));
        if (element->element == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)type->num_fields * sizeof(coda_xmlDynamicType *), __FILE__, __LINE__);
            delete_xml_dynamic_element(element);
            return NULL;
        }
        for (i = 0; i < type->num_fields; i++)
        {
            element->element[i] = NULL;
        }
        element->num_elements = type->num_fields;

        /* create empty arrays for array child elements */
        for (i = 0; i < element->num_elements; i++)
        {
            if (type->field[i]->type->tag == tag_xml_array)
            {
                element->element[i] =
                    (coda_xmlDynamicType *)coda_xml_dynamic_array_new((coda_xmlArray *)type->field[i]->type);
                if (element->element[i] == NULL)
                {
                    delete_xml_dynamic_element(element);
                    return NULL;
                }
            }
        }
    }

    element->attributes = coda_xml_dynamic_attribute_record_new(type->attributes, attr);
    if (element->attributes == NULL)
    {
        delete_xml_dynamic_element(element);
        return NULL;
    }

    return element;
}

int coda_xml_dynamic_element_add_element(coda_xmlElementDynamicType *element, coda_xmlElementDynamicType *sub_element)
{
    int element_index;

    assert(element != NULL);
    assert(sub_element != NULL);

    element_index = hashtable_get_index_from_name(element->type->xml_name_hash_data, sub_element->type->xml_name);
    assert(element_index >= 0 && element_index < element->num_elements);
    if (element->element[element_index] != NULL)
    {
        if (element->element[element_index]->tag == tag_xml_array_dynamic)
        {
            /* add the child element to the array */
            if (coda_xml_dynamic_array_add_element((coda_xmlArrayDynamicType *)element->element[element_index],
                                                   sub_element) != 0)
            {
                return -1;
            }
        }
        else
        {
            coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed more than once within element '%s'",
                           sub_element->type->xml_name, element->type->xml_name);
            return -1;
        }
    }
    else
    {
        element->element[element_index] = (coda_xmlDynamicType *)sub_element;
        sub_element->retain_count++;
    }

    /* couple the child to the parent */
    sub_element->parent = element;

    return 0;
}

int coda_xml_dynamic_element_update(coda_xmlElementDynamicType *element)
{
    int i;

    if (element->type->tag == tag_xml_text)
    {
        element->tag = tag_xml_text_dynamic;
        element->type_class = coda_text_class;

        /* we can remove all sub elements */
        if (element->element != NULL)
        {
            for (i = 0; i < element->num_elements; i++)
            {
                if (element->element[i] != NULL)
                {
                    coda_xml_release_dynamic_type(element->element[i]);
                }
            }
            free(element->element);
            element->element = NULL;
        }
        element->num_elements = 0;
    }
    else if (element->type->tag == tag_xml_record)
    {
        if (element->type->num_fields > element->num_elements)
        {
            coda_xmlDynamicType **new_element;

            /* increase the size for the child elements array until it matches the size in the definition */
            new_element = realloc(element->element, element->type->num_fields * sizeof(coda_xmlDynamicType *));
            if (new_element == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               element->type->num_fields * sizeof(coda_xmlDynamicType *), __FILE__, __LINE__);
                return -1;
            }
            element->element = new_element;
            for (i = element->num_elements; i < element->type->num_fields; i++)
            {
                element->element[i] = NULL;
            }
            element->num_elements = element->type->num_fields;
        }

        /* update the array status for each child element */
        for (i = 0; i < element->num_elements; i++)
        {
            if (element->type->field[i]->type->tag == tag_xml_array)
            {
                coda_xmlArrayDynamicType *array;

                if (element->element[i] == NULL)
                {
                    /* create an empty array */
                    array = coda_xml_dynamic_array_new((coda_xmlArray *)element->type->field[i]->type);
                    if (array == NULL)
                    {
                        return -1;
                    }
                    element->element[i] = (coda_xmlDynamicType *)array;
                }
                else if (element->element[i]->tag != tag_xml_array_dynamic)
                {
                    /* convert the single element into an array of a single element */
                    array = coda_xml_dynamic_array_new((coda_xmlArray *)element->type->field[i]->type);
                    if (array == NULL)
                    {
                        return -1;
                    }
                    if (coda_xml_dynamic_array_add_element(array, (coda_xmlElementDynamicType *)element->element[i]) !=
                        0)
                    {
                        coda_xml_release_dynamic_type((coda_xmlDynamicType *)array);
                        return -1;
                    }
                    coda_xml_release_dynamic_type(element->element[i]);
                    element->element[i] = (coda_xmlDynamicType *)array;
                }
            }
        }
    }

    return coda_xml_dynamic_attribute_record_update(element->attributes);
}

int coda_xml_dynamic_element_validate(coda_xmlElementDynamicType *element)
{
    if (element->tag == tag_xml_record_dynamic)
    {
        int i;

        /* verify occurence of mandatory elements */
        for (i = 0; i < element->num_elements; i++)
        {
            if (element->element[i] == NULL)
            {
                if (!element->type->field[i]->optional)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "mandatory xml element '%s' is missing",
                                   element->type->field[i]->xml_name);
                    return -1;
                }
            }
        }
    }

    return 0;
}
