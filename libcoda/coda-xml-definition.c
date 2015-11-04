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
#include "coda-xml-dynamic.h"
#include "coda-definition.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static coda_xmlAttributeRecord *empty_attribute_record_singleton = NULL;
static coda_xmlAttributeRecordDynamicType *empty_dynamic_attribute_record_singleton = NULL;

static void delete_xml_attribute(coda_xmlAttribute *attribute)
{
    assert(attribute->retain_count == 0);
    if (attribute->name != NULL)
    {
        free(attribute->name);
    }
    if (attribute->description != NULL)
    {
        free(attribute->description);
    }
    if (attribute->xml_name != NULL)
    {
        free(attribute->xml_name);
    }
    if (attribute->attr_name != NULL)
    {
        free(attribute->attr_name);
    }
    if (attribute->fixed_value != NULL)
    {
        free(attribute->fixed_value);
    }
    free(attribute);
}

static void delete_xml_attribute_record(coda_xmlAttributeRecord *attribute_record)
{
    assert(attribute_record->retain_count == 0);
    if (attribute_record->name != NULL)
    {
        free(attribute_record->name);
    }
    if (attribute_record->description != NULL)
    {
        free(attribute_record->description);
    }
    if (attribute_record->attribute != NULL)
    {
        int i;

        for (i = 0; i < attribute_record->num_attributes; i++)
        {
            coda_xml_release_type((coda_xmlType *)attribute_record->attribute[i]);
        }
        free(attribute_record->attribute);
    }
    if (attribute_record->attribute_name_hash_data != NULL)
    {
        delete_hashtable(attribute_record->attribute_name_hash_data);
    }
    if (attribute_record->name_hash_data != NULL)
    {
        delete_hashtable(attribute_record->name_hash_data);
    }
    free(attribute_record);
}

void coda_xml_field_delete(coda_xmlField *field)
{
    if (field->name != NULL)
    {
        free(field->name);
    }
    if (field->type != NULL)
    {
        coda_xml_release_type(field->type);
    }
    free(field);
}

static void delete_xml_root(coda_xmlRoot *root)
{
    assert(root->retain_count == 0);
    if (root->name != NULL)
    {
        free(root->name);
    }
    if (root->description != NULL)
    {
        free(root->description);
    }
    if (root->field != NULL)
    {
        coda_xml_field_delete(root->field);
    }
    free(root);
}

static void delete_xml_element(coda_xmlElement *element)
{
    assert(element->retain_count == 0);
    if (element->name != NULL)
    {
        free(element->name);
    }
    if (element->description != NULL)
    {
        free(element->description);
    }
    if (element->xml_name != NULL)
    {
        free(element->xml_name);
    }
    if (element->attributes != NULL)
    {
        coda_xml_release_type((coda_xmlType *)element->attributes);
    }
    if (element->field != NULL)
    {
        int i;

        for (i = 0; i < element->num_fields; i++)
        {
            coda_xml_field_delete(element->field[i]);
        }
        free(element->field);
    }
    if (element->xml_name_hash_data != NULL)
    {
        delete_hashtable(element->xml_name_hash_data);
    }
    if (element->name_hash_data != NULL)
    {
        delete_hashtable(element->name_hash_data);
    }
    if (element->ascii_type != NULL)
    {
        coda_release_type((coda_Type *)element->ascii_type);
    }
    free(element);
}

static void delete_xml_array(coda_xmlArray *array)
{
    assert(array->retain_count == 0);
    if (array->name != NULL)
    {
        free(array->name);
    }
    if (array->description != NULL)
    {
        free(array->description);
    }
    if (array->base_type != NULL)
    {
        coda_xml_release_type((coda_xmlType *)array->base_type);
    }
    free(array);
}

void coda_xml_release_type(coda_xmlType *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (type->tag)
    {
        case tag_xml_root:
            delete_xml_root((coda_xmlRoot *)type);
            break;
        case tag_xml_record:
        case tag_xml_text:
        case tag_xml_ascii_type:
            delete_xml_element((coda_xmlElement *)type);
            break;
        case tag_xml_array:
            delete_xml_array((coda_xmlArray *)type);
            break;
        case tag_xml_attribute:
            delete_xml_attribute((coda_xmlAttribute *)type);
            break;
        case tag_xml_attribute_record:
            delete_xml_attribute_record((coda_xmlAttributeRecord *)type);
            break;
    }
}

static coda_xmlAttributeRecord *attribute_record_new(void)
{
    coda_xmlAttributeRecord *record;

    record = malloc(sizeof(coda_xmlAttributeRecord));
    if (record == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlAttributeRecord), __FILE__, __LINE__);
        return NULL;
    }
    record->retain_count = 0;
    record->format = coda_format_xml;
    record->type_class = coda_record_class;
    record->name = NULL;
    record->description = NULL;
    record->tag = tag_xml_attribute_record;
    record->num_attributes = 0;
    record->attribute = NULL;
    record->attribute_name_hash_data = NULL;
    record->name_hash_data = NULL;

    record->attribute_name_hash_data = new_hashtable(1);
    if (record->attribute_name_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_attribute_record(record);
        return NULL;
    }
    record->name_hash_data = new_hashtable(0);
    if (record->name_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_attribute_record(record);
        return NULL;
    }

    return record;
}

static int attribute_record_add_attribute(coda_xmlAttributeRecord *attributes, coda_xmlAttribute *attribute)
{
    coda_xmlAttribute **new_attribute;

    if (hashtable_get_index_from_name(attributes->attribute_name_hash_data, attribute->xml_name) >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute with XML name '%s' already exists", attribute->xml_name);
        return -1;
    }
    if (hashtable_get_index_from_name(attributes->name_hash_data, attribute->attr_name) >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute with name '%s' already exists", attribute->attr_name);
        return -1;
    }

    new_attribute = realloc(attributes->attribute, (attributes->num_attributes + 1) * sizeof(coda_xmlAttribute *));
    if (new_attribute == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (attributes->num_attributes + 1) * sizeof(coda_xmlAttribute *), __FILE__, __LINE__);
        return -1;
    }
    attributes->attribute = new_attribute;

    attributes->attribute[attributes->num_attributes] = attribute;
    attribute->retain_count++;
    attributes->num_attributes++;

    if (hashtable_add_name(attributes->attribute_name_hash_data, attribute->xml_name) != 0)
    {
        assert(0);
        exit(1);
    }
    if (hashtable_add_name(attributes->name_hash_data, attribute->attr_name) != 0)
    {
        assert(0);
        exit(1);
    }

    return 0;
}

int coda_xml_element_add_attribute(coda_xmlElement *element, coda_xmlAttribute *attribute)
{
    return attribute_record_add_attribute(element->attributes, attribute);
}

coda_xmlRoot *coda_xml_root_new(void)
{
    coda_xmlRoot *root;

    root = malloc(sizeof(coda_xmlRoot));
    if (root == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlRoot), __FILE__, __LINE__);
        return NULL;
    }
    root->retain_count = 0;
    root->format = coda_format_xml;
    root->type_class = coda_record_class;
    root->name = NULL;
    root->description = NULL;
    root->tag = tag_xml_root;
    root->field = NULL;

    return root;
}

int coda_xml_root_set_field(coda_xmlRoot *root, coda_xmlField *field)
{
    if (root->field != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "root already has a field");
        return -1;
    }
    root->field = field;
    return 0;
}

int coda_xml_root_validate(coda_xmlRoot *root)
{
    if (root->field != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing field for XML root definition");
        return -1;
    }
    return 0;
}

static coda_xmlElement *new_xml_element(const char *xml_name)
{
    coda_xmlElement *element;

    assert(xml_name != NULL);

    element = malloc(sizeof(coda_xmlElement));
    if (element == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlElement), __FILE__, __LINE__);
        return NULL;
    }
    element->retain_count = 0;
    element->format = coda_format_xml;
    element->name = NULL;
    element->description = NULL;
    element->xml_name = NULL;
    element->attributes = NULL;
    element->num_fields = 0;
    element->field = NULL;
    element->xml_name_hash_data = NULL;
    element->name_hash_data = NULL;
    element->ascii_type = NULL;

    element->xml_name = strdup(xml_name);
    if (element->xml_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_element(element);
        return NULL;
    }

    element->attributes = attribute_record_new();
    if (element->attributes == NULL)
    {
        delete_xml_element(element);
        return NULL;
    }

    return element;
}

coda_xmlElement *coda_xml_record_new(const char *xml_name)
{
    coda_xmlElement *element;

    element = new_xml_element(xml_name);
    if (element == NULL)
    {
        return NULL;
    }
    element->type_class = coda_record_class;
    element->tag = tag_xml_record;
    element->xml_name_hash_data = new_hashtable(1);
    if (element->xml_name_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_element(element);
        return NULL;
    }
    element->name_hash_data = new_hashtable(0);
    if (element->name_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_element(element);
        return NULL;
    }

    return element;
}

int coda_xml_record_add_field(coda_xmlElement *element, coda_xmlField *field)
{
    coda_xmlField **new_field;

    assert(element->tag == tag_xml_record);

    if (hashtable_get_index_from_name(element->xml_name_hash_data, field->xml_name) >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "sub element with XML name '%s' already exists for this XML type",
                       field->xml_name);
        return -1;
    }
    if (hashtable_get_index_from_name(element->name_hash_data, field->name) >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "sub element with field name '%s' already exists for this XML type",
                       field->name);
        return -1;
    }

    new_field = realloc(element->field, (element->num_fields + 1) * sizeof(coda_xmlField *));
    if (new_field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(element->num_fields + 1) * sizeof(coda_xmlField *), __FILE__, __LINE__);
        return -1;
    }
    element->field = new_field;
    element->field[element->num_fields] = field;
    element->num_fields++;

    if (hashtable_add_name(element->xml_name_hash_data, field->xml_name) != 0)
    {
        assert(0);
        exit(1);
    }
    if (hashtable_add_name(element->name_hash_data, field->name) != 0)
    {
        assert(0);
        exit(1);
    }

    return 0;
}

void coda_xml_record_convert_to_text(coda_xmlElement *element)
{
    assert(element->tag == tag_xml_record);

    element->type_class = coda_text_class;
    element->tag = tag_xml_text;
    if (element->field != NULL)
    {
        int i;

        for (i = 0; i < element->num_fields; i++)
        {
            coda_xml_field_delete(element->field[i]);
        }
        free(element->field);
        element->field = NULL;
    }
    element->num_fields = 0;
    if (element->xml_name_hash_data != NULL)
    {
        delete_hashtable(element->xml_name_hash_data);
        element->xml_name_hash_data = NULL;
    }
    if (element->name_hash_data != NULL)
    {
        delete_hashtable(element->name_hash_data);
        element->name_hash_data = NULL;
    }
}

coda_xmlElement *coda_xml_text_new(const char *xml_name)
{
    coda_xmlElement *element;

    element = new_xml_element(xml_name);
    if (element == NULL)
    {
        return NULL;
    }
    element->type_class = coda_text_class;
    element->tag = tag_xml_text;

    return element;
}

coda_xmlElement *coda_xml_ascii_type_new(const char *xml_name)
{
    coda_xmlElement *element;

    element = new_xml_element(xml_name);
    if (element == NULL)
    {
        return NULL;
    }
    element->type_class = coda_text_class;
    element->tag = tag_xml_ascii_type;

    return element;
}

int coda_xml_ascii_type_set_type(coda_xmlElement *element, coda_asciiType *type)
{
    if (element->ascii_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a sub type");
        return -1;
    }
    assert(element->tag == tag_xml_ascii_type);
    element->type_class = type->type_class;
    element->ascii_type = type;
    type->retain_count++;
    return 0;
}

int coda_xml_ascii_type_validate(coda_xmlElement *element)
{
    assert(element->tag == tag_xml_ascii_type);
    if (element->ascii_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing base type for XML ascii type definition");
        return -1;
    }
    return 0;
}

coda_xmlField *coda_xml_field_new(const char *name)
{
    coda_xmlField *field;

    assert(name != NULL);

    field = malloc(sizeof(coda_xmlField));
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlField), __FILE__, __LINE__);
        return NULL;
    }

    field->name = NULL;
    field->xml_name = NULL;
    field->type = NULL;
    field->optional = 0;
    field->hidden = 0;

    field->name = strdup(name);
    if (field->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_xml_field_delete(field);
        return NULL;
    }

    return field;
}

int coda_xml_field_set_type(coda_xmlField *field, coda_xmlType *type)
{
    if (field->type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a type");
        return -1;
    }
    switch (type->tag)
    {
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
            field->xml_name = ((coda_xmlElement *)type)->xml_name;
            break;
        case tag_xml_array_dynamic:
            field->xml_name = ((coda_xmlArray *)type)->base_type->xml_name;
            break;
        default:
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid type for XML field '%s' definition", field->name);
            return -1;
    }
    field->type = type;
    type->retain_count++;

    return 0;
}

int coda_xml_field_set_hidden(coda_xmlField *field)
{
    field->hidden = 1;
    return 0;
}

int coda_xml_field_set_optional(coda_xmlField *field)
{
    field->optional = 1;
    return 0;
}

int coda_xml_field_validate(coda_xmlField *field)
{
    if (field->type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing type for XML field '%s' definition", field->name);
        return -1;
    }
    return 0;
}

int coda_xml_field_convert_to_array(coda_xmlField *field)
{
    coda_xmlArray *array;

    assert(field->type->tag != tag_xml_array);

    array = coda_xml_array_new();
    if (array == NULL)
    {
        return -1;
    }
    if (coda_xml_array_set_base_type(array, (coda_xmlElement *)field->type) != 0)
    {
        return -1;
    }
    coda_xml_release_type(field->type);
    field->type = (coda_xmlType *)array;

    return 0;
}

coda_xmlArray *coda_xml_array_new(void)
{
    coda_xmlArray *array;

    array = malloc(sizeof(coda_xmlArray));
    if (array == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlArray), __FILE__, __LINE__);
        return NULL;
    }
    array->retain_count = 0;
    array->format = coda_format_xml;
    array->type_class = coda_array_class;
    array->name = NULL;
    array->description = NULL;
    array->tag = tag_xml_array;
    array->base_type = NULL;

    return array;
}

int coda_xml_array_set_base_type(coda_xmlArray *array, coda_xmlElement *base_type)
{
    if (array->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "array already has a base type");
        return -1;
    }
    if (base_type->tag != tag_xml_record && base_type->tag != tag_xml_text && base_type->tag != tag_xml_ascii_type)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid base type for XML array definition");
        return -1;
    }
    array->base_type = base_type;
    base_type->retain_count++;

    return 0;
}

int coda_xml_array_validate(coda_xmlArray *array)
{
    if (array->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing base type for XML array definition");
        return -1;
    }
    return 0;
}

coda_xmlAttribute *coda_xml_attribute_new(const char *xml_name)
{
    coda_xmlAttribute *attribute;

    assert(xml_name != NULL);

    attribute = malloc(sizeof(coda_xmlAttribute));
    if (attribute == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlAttribute), __FILE__, __LINE__);
        return NULL;
    }
    attribute->retain_count = 0;
    attribute->format = coda_format_xml;
    attribute->type_class = coda_text_class;
    attribute->name = NULL;
    attribute->description = NULL;
    attribute->tag = tag_xml_attribute;
    attribute->xml_name = NULL;
    attribute->attr_name = NULL;
    attribute->fixed_value = NULL;
    attribute->optional = 0;

    attribute->xml_name = strdup(xml_name);
    if (attribute->xml_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        delete_xml_attribute(attribute);
        return NULL;
    }

    attribute->attr_name = coda_identifier_from_name(coda_element_name_from_xml_name(xml_name), NULL);
    if (attribute->attr_name == NULL)
    {
        delete_xml_attribute(attribute);
        return NULL;
    }

    return attribute;
}

int coda_xml_attribute_set_fixed_value(coda_xmlAttribute *attribute, const char *fixed_value)
{
    if (attribute->fixed_value != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute already has a fixed value");
        return -1;
    }
    if (fixed_value == NULL)
    {
        return 0;
    }
    attribute->fixed_value = strdup(fixed_value);
    if (attribute->fixed_value == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_xml_attribute_set_optional(coda_xmlAttribute *attribute)
{
    attribute->optional = 1;
    return 0;
}


static void delete_detection_node(coda_xmlDetectionNode *node)
{
    int i;

    delete_hashtable(node->hash_data);
    if (node->xml_name != NULL)
    {
        free(node->xml_name);
    }
    if (node->detection_rule != NULL)
    {
        /* we don't have to delete the detection rules, since we have only stored references */
        free(node->detection_rule);
    }
    if (node->subnode != NULL)
    {
        for (i = 0; i < node->num_subnodes; i++)
        {
            delete_detection_node(node->subnode[i]);
        }
        free(node->subnode);
    }
    free(node);
}

static coda_xmlDetectionNode *detection_node_new(const char *xml_name, coda_xmlDetectionNode *parent)
{
    coda_xmlDetectionNode *node;

    node = malloc(sizeof(coda_xmlDetectionNode));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xmlDetectionNode), __FILE__, __LINE__);
        return NULL;
    }
    node->xml_name = NULL;
    node->num_detection_rules = 0;
    node->detection_rule = NULL;
    node->num_subnodes = 0;
    node->subnode = NULL;
    node->hash_data = NULL;
    node->parent = parent;

    if (xml_name != NULL)
    {
        node->xml_name = strdup(xml_name);
        if (node->xml_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            delete_detection_node(node);
            return NULL;
        }
    }

    node->hash_data = new_hashtable(1);
    if (node->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_detection_node(node);
        return NULL;
    }

    return node;
}

coda_xmlDetectionNode *coda_xml_detection_node_get_subnode(coda_xmlDetectionNode *node, const char *xml_name)
{
    int index;

    index = hashtable_get_index_from_name(node->hash_data, xml_name);
    if (index >= 0)
    {
        return node->subnode[index];
    }
    return NULL;
}

static int detection_node_add_rule(coda_xmlDetectionNode *node, coda_DetectionRule *detection_rule)
{
    coda_DetectionRule **new_detection_rule;

    assert(node != NULL);
    assert(detection_rule != NULL);

    /* verify that other rules at this node do not shadow the new rule */
    if (node->num_detection_rules > 0)
    {
        int i;

        for (i = 0; i < node->num_detection_rules; i++)
        {
            if (node->detection_rule[i]->entry[0]->value == NULL)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION,
                               "detection rule for product definition %s is shadowed by product definition %s",
                               detection_rule->product_definition->name,
                               node->detection_rule[i]->product_definition->name);
                return -1;
            }
            else if (detection_rule->entry[0]->value != NULL)
            {
                if (strcmp(detection_rule->entry[0]->value, node->detection_rule[i]->entry[0]->value) == 0)
                {
                    coda_set_error(CODA_ERROR_DATA_DEFINITION,
                                   "detection rule for product definition %s is shadowed by product definition %s",
                                   detection_rule->product_definition->name,
                                   node->detection_rule[i]->product_definition->name);
                    return -1;
                }
            }
        }
    }

    new_detection_rule = realloc(node->detection_rule, (node->num_detection_rules + 1) * sizeof(coda_DetectionRule *));
    if (new_detection_rule == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_detection_rules + 1) * sizeof(coda_DetectionRule *), __FILE__, __LINE__);
        return -1;
    }
    node->detection_rule = new_detection_rule;
    node->detection_rule[node->num_detection_rules] = detection_rule;
    node->num_detection_rules++;

    return 0;
}

static int detection_node_add_subnode(coda_xmlDetectionNode *node, const char *xml_name)
{
    coda_xmlDetectionNode **subnode_list;
    coda_xmlDetectionNode *subnode;
    int result;

    assert(node != NULL);
    assert(xml_name != NULL);

    subnode = detection_node_new(xml_name, node);
    if (subnode == NULL)
    {
        return -1;
    }
    subnode_list = realloc(node->subnode, (node->num_subnodes + 1) * sizeof(coda_xmlDetectionNode *));
    if (subnode_list == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_subnodes + 1) * sizeof(coda_xmlDetectionNode *), __FILE__, __LINE__);
        delete_detection_node(subnode);
        return -1;
    }
    node->subnode = subnode_list;
    node->subnode[node->num_subnodes] = subnode;
    node->num_subnodes++;

    result = hashtable_add_name(node->hash_data, subnode->xml_name);
    assert(result == 0);

    return 0;
}

void coda_xml_detection_tree_delete(void *detection_tree)
{
    delete_detection_node((coda_xmlDetectionNode *)detection_tree);
}

int coda_xml_detection_tree_add_rule(void *detection_tree, coda_DetectionRule *detection_rule)
{
    coda_xmlDetectionNode *node;
    const char *xml_name;
    int last_node = 0;
    char *path;
    char *p;

    if (detection_rule->num_entries != 1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule for '%s' should only have one entry",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->use_filename)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule for '%s' can not be based on filenames",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->offset != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule for '%s' can not be based on offsets",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->path == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule for '%s' requires path",
                       detection_rule->product_definition->name);
        return -1;
    }

    node = *(coda_xmlDetectionNode **)detection_tree;
    if (node == NULL)
    {
        node = detection_node_new(NULL, NULL);
        if (node == NULL)
        {
            return -1;
        }
        *(coda_xmlDetectionNode **)detection_tree = node;
    }

    path = strdup(detection_rule->entry[0]->path);
    if (path == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    p = path;
    if (*p == '/')
    {
        p++;
    }
    while (!last_node)
    {
        coda_xmlDetectionNode *subnode;

        xml_name = p;
        while (*p != '/' && *p != '\0')
        {
            if (*p == '{')
            {
                /* parse namespace */
                p++;
                xml_name = p;
                while (*p != '}')
                {
                    if (*p == '\0')
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                                       "xml detection rule for '%s' has invalid path value",
                                       detection_rule->product_definition->name);
                        free(path);
                        return -1;
                    }
                    p++;
                }
                *p = ' ';
            }
            p++;
        }
        last_node = (*p == '\0');
        *p = '\0';

        subnode = coda_xml_detection_node_get_subnode(node, xml_name);
        if (subnode == NULL)
        {
            if (detection_node_add_subnode(node, xml_name) != 0)
            {
                free(path);
                return -1;
            }
            subnode = coda_xml_detection_node_get_subnode(node, xml_name);
            if (subnode == NULL)
            {
                free(path);
                return -1;
            }
        }
        node = subnode;

        if (!last_node)
        {
            p++;
        }
    }

    free(path);

    if (detection_node_add_rule(node, detection_rule) != 0)
    {
        return -1;
    }

    return 0;
}

void coda_xml_done()
{
    if (empty_attribute_record_singleton != NULL)
    {
        coda_xml_release_type((coda_xmlType *)empty_attribute_record_singleton);
        empty_attribute_record_singleton = NULL;
    }
    if (empty_dynamic_attribute_record_singleton != NULL)
    {
        coda_xml_release_dynamic_type((coda_xmlDynamicType *)empty_dynamic_attribute_record_singleton);
        empty_dynamic_attribute_record_singleton = NULL;
    }
}

coda_xmlDynamicType *coda_xml_empty_dynamic_attribute_record()
{
    if (empty_attribute_record_singleton == NULL)
    {
        empty_attribute_record_singleton = attribute_record_new();
        if (empty_attribute_record_singleton == NULL)
        {
            return NULL;
        }
    }
    if (empty_dynamic_attribute_record_singleton == NULL)
    {
        empty_dynamic_attribute_record_singleton =
            coda_xml_dynamic_attribute_record_new(empty_attribute_record_singleton, NULL);
        if (empty_dynamic_attribute_record_singleton == NULL)
        {
            return NULL;
        }
    }
    return (coda_xmlDynamicType *)empty_dynamic_attribute_record_singleton;
}
