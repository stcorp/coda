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

#include "coda-xml-internal.h"

#include "coda-definition.h"
#include "coda-xml-definition.h"
#include "coda-xml-dynamic.h"

#include "expat.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define BUFFSIZE        8192

struct element_dictionary_struct
{
    int num_elements;
    coda_xml_element **element;
    hashtable *hash_data;
};
typedef struct element_dictionary_struct element_dictionary;

static void delete_element_dictionary(element_dictionary *dictionary)
{
    hashtable_delete(dictionary->hash_data);
    if (dictionary->element != NULL)
    {
        int i;

        for (i = 0; i < dictionary->num_elements; i++)
        {
            coda_xml_release_type((coda_xml_type *)dictionary->element[i]);
        }
        free(dictionary->element);
    }
    free(dictionary);
}

static element_dictionary *new_element_dictionary(void)
{
    element_dictionary *dictionary;

    dictionary = malloc(sizeof(element_dictionary));
    if (dictionary == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(element_dictionary), __FILE__, __LINE__);
        return NULL;
    }
    dictionary->num_elements = 0;
    dictionary->element = NULL;
    dictionary->hash_data = hashtable_new(0);
    if (dictionary->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_element_dictionary(dictionary);
        return NULL;
    };

    return dictionary;
}

static int element_dictionary_add_element(element_dictionary *dictionary, coda_xml_element *element)
{
    coda_xml_element **new_element;

    new_element = realloc(dictionary->element, (dictionary->num_elements + 1) * sizeof(coda_xml_element *));
    if (new_element == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (dictionary->num_elements + 1) * sizeof(coda_xml_element *), __FILE__, __LINE__);
        return -1;
    }
    dictionary->element = new_element;

    dictionary->element[dictionary->num_elements] = element;
    dictionary->num_elements++;

    if (hashtable_add_name(dictionary->hash_data, element->xml_name) != 0)
    {
        assert(0);
        exit(1);
    }

    return 0;
}

static coda_xml_element *get_element_definition(element_dictionary *dictionary, const char *xml_name)
{
    long index;

    index = hashtable_get_index_from_name(dictionary->hash_data, xml_name);
    if (index < 0)
    {
        return NULL;
    }

    return dictionary->element[index];
}

static int is_whitespace(const char *s, int len)
{
    int i;

    assert(s != NULL);

    for (i = 0; i < len; i++)
    {
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        {
            return 0;
        }
    }

    return 1;
}

struct parser_info_struct
{
    XML_Parser parser;
    int abort_parser;
    coda_xml_product *product;
    coda_xml_root_dynamic_type *root;
    coda_xml_element_dynamic_type *element;
    int unparsed_depth; /* keep track of how deep we are in an XML element that we interpret as text */
    element_dictionary *dictionary;
};
typedef struct parser_info_struct parser_info;

static void abort_parser(parser_info *info)
{
    XML_StopParser(info->parser, 0);
    /* we need to explicitly check in the end handlers for parsing abort because expat may still call the end handler
     * after an abort in the start handler */
    info->abort_parser = 1;
}

static void XMLCALL interpret_start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info;
    coda_xml_element *definition;
    coda_xml_element_dynamic_type *element;
    int i;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth++;
        return;
    }

    definition = get_element_definition(info->dictionary, el);
    if (definition == NULL)
    {
        /* create a dynamic definition (start of with an empty record definition, this may change later) */
        definition = coda_xml_record_new(el);
        if (definition == NULL)
        {
            abort_parser(info);
            return;
        }

        /* add definition to element dictionary */
        if (element_dictionary_add_element(info->dictionary, definition) != 0)
        {
            coda_xml_release_type((coda_xml_type *)definition);
            abort_parser(info);
            return;
        }
    }

    /* add attributes to definition if needed */
    for (i = 0; attr[2 * i] != NULL; i++)
    {
        int attribute_index;
        char *name;

        name = coda_identifier_from_name(coda_element_name_from_xml_name(attr[2 * i]), NULL);
        if (name == NULL)
        {
            abort_parser(info);
            return;
        }
        attribute_index = hashtable_get_index_from_name(definition->attributes->attribute_name_hash_data, attr[2 * i]);
        if (attribute_index < 0)
        {
            /* if there is already another attribute with the same identifier then we ignore this attribute! */
            if (hashtable_get_index_from_name(definition->attributes->name_hash_data, name) < 0)
            {
                coda_xml_attribute *attribute;

                /* add attribute to definition */
                attribute = coda_xml_attribute_new(attr[2 * i]);
                if (attribute == NULL)
                {
                    abort_parser(info);
                    free(name);
                    return;
                }
                /* all attributes for dynamic interpreted XML are optional */
                if (coda_xml_attribute_set_optional(attribute) != 0)
                {
                    abort_parser(info);
                    free(name);
                    coda_xml_release_type((coda_xml_type *)attribute);
                    return;
                }
                if (coda_xml_element_add_attribute(definition, attribute) != 0)
                {
                    abort_parser(info);
                    free(name);
                    coda_xml_release_type((coda_xml_type *)attribute);
                    return;
                }
                coda_xml_release_type((coda_xml_type *)attribute);
            }
        }
        free(name);
    }

    if (info->element != NULL)
    {
        coda_xml_element *type;
        int element_index;

        type = info->element->type;

        if (type->type_class != coda_record_class)
        {
            /* all sub elements of the parent element will be ignored because the parent element is not a record */
            info->unparsed_depth = 1;
            return;
        }

        /* check if the element is already in the dynamic definition of the parent. if not, add the element */
        element_index = hashtable_get_index_from_name(type->xml_name_hash_data, definition->xml_name);
        if (element_index < 0)
        {
            coda_xml_field *field;
            char *name;

            name = coda_identifier_from_name(coda_element_name_from_xml_name(definition->xml_name), NULL);
            if (hashtable_get_index_from_name(type->name_hash_data, name) >= 0)
            {
                /* if there is already another element with the same identifier then we ignore this element! */
                info->unparsed_depth = 1;
                free(name);
                return;
            }

            /* add this element to the parent definition */
            field = coda_xml_field_new(name);
            if (field == NULL)
            {
                abort_parser(info);
                free(name);
                return;
            }
            free(name);
            if (coda_xml_field_set_type(field, (coda_xml_type *)definition) != 0)
            {
                abort_parser(info);
                coda_xml_field_delete(field);
                return;
            }
            if (coda_xml_record_add_field(type, field) != 0)
            {
                abort_parser(info);
                coda_xml_field_delete(field);
                return;
            }
            element_index = hashtable_get_index_from_name(type->xml_name_hash_data, definition->xml_name);
            assert(element_index >= 0);
        }
        else
        {
            /* verify the namespace of the child element */
            if (strcasecmp(definition->xml_name, info->element->type->field[element_index]->xml_name) != 0)
            {
                /* the namespace of the new element differens from that of the previous element with the same element
                 * name. we will therefore ignore this new element (the behavior is similar to the situation above
                 * where identifier names are the same).
                 */
                info->unparsed_depth = 1;
                return;
            }

            /* check whether we need turn the field for this element into an array */
            if (info->element->element[element_index] != NULL)
            {
                /* we already have a similar named element */
                if (type->field[element_index]->type->tag != tag_xml_array)
                {
                    /* convert the field into an array of elements */
                    if (coda_xml_field_convert_to_array(type->field[element_index]) != 0)
                    {
                        abort_parser(info);
                        return;
                    }
                }
            }
        }

        if (type->type_class == coda_record_class)
        {
            coda_xml_element_dynamic_type *ancestor;

            /* CODA does not allow elements that contain itself, so we turn such elements into text elements.
             * we check recursion for the child element here.
             * this is only necessary if it was not already turned into a text element (i.e. it is still a record)
             */
            ancestor = info->element->parent;
            while (ancestor != NULL)
            {
                if (ancestor->type == type)
                {
                    /* we found a recursion -> turn the definition into a text element */
                    coda_xml_record_convert_to_text(type);
                }
                ancestor = ancestor->parent;
            }
        }

        /* update the element with any changes in the definition */
        if (coda_xml_dynamic_element_update(info->element) != 0)
        {
            abort_parser(info);
            return;
        }

        if (info->element->type_class != coda_record_class)
        {
            /* all sub elements will be ignored because the parent element is no longer a record */
            info->unparsed_depth = 1;
            return;
        }
    }

    /* create a new element */
    element = coda_xml_dynamic_element_new(definition, attr);
    if (element == NULL)
    {
        abort_parser(info);
        return;
    }
    element->outer_bit_offset = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser);
    element->inner_bit_offset = element->outer_bit_offset + 8 * (int64_t)XML_GetCurrentByteCount(info->parser);

    if (info->element != NULL)
    {
        if (coda_xml_dynamic_element_add_element(info->element, element) != 0)
        {
            abort_parser(info);
            return;
        }
        coda_release_type((coda_type *)element);
    }
    else
    {
        assert(info->root->element == NULL);
        info->root->element = element;
    }
    info->element = element;
}

static void XMLCALL interpret_end_element_handler(void *data, const char *el)
{
    coda_xml_element_dynamic_type *element;
    parser_info *info;

    el = el;

    info = (parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth--;
        return;
    }

    assert(info->element != NULL);
    element = info->element;

    if (element->retain_count == -1)
    {
        info->element = element->parent;

        /* this element was ignored because an element with a similar name already existed -> we delete it here */
        element->retain_count = 0;
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)element);
    }
    else
    {
        if (element->cdata_delta_offset > 0)
        {
            /* we use the CDATA content as content for this element -> update the delta value for the CDATA size */
            /* the size of the CDATA content was temporarily stored in inner_bit_size */
            element->cdata_delta_size = (int32_t)(element->inner_bit_size -
                                                  8 * (int64_t)XML_GetCurrentByteIndex(info->parser));
        }
        else
        {
            /* no CDATA -> reset the CDATA delta values  */
            element->cdata_delta_offset = 0;
            element->cdata_delta_size = 0;
        }
        element->inner_bit_size = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser) - element->inner_bit_offset;
        element->outer_bit_size = 8 * (int64_t)(XML_GetCurrentByteIndex(info->parser) +
                                                XML_GetCurrentByteCount(info->parser)) - element->outer_bit_offset;
        /* apply the CDATA delta value to the inner offset and size */
        element->inner_bit_offset += element->cdata_delta_offset;
        element->inner_bit_size += element->cdata_delta_size;

        info->element = element->parent;
    }
}

static void XMLCALL interpret_character_data_handler(void *data, const char *s, int len)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (!is_whitespace(s, len))
    {
        /* the XML parser should already give an error for any non-whitespace data outside the root element.
         * this means we should always have a root element when we get here.
         */
        assert(info->element != NULL);

        if (info->element->type->tag != tag_xml_text)
        {
            /* the parent element no longer consists purely of other elements so we turn it into a text element */
            coda_xml_record_convert_to_text(info->element->type);
            if (coda_xml_dynamic_element_update(info->element) != 0)
            {
                abort_parser(info);
                return;
            }
        }

        if (info->element->cdata_delta_offset == 0)
        {
            /* we have non-whitespace character data before any CDATA element so disable CDATA from here on */
            info->element->cdata_delta_offset = -1;
        }
        else if (info->element->cdata_delta_offset > 0 && info->element->cdata_delta_size != 0)
        {
            /* we have non-whitespace character data after a CDATA element so disable the CDATA */
            info->element->cdata_delta_offset = -1;
        }
    }
}

static void XMLCALL interpret_start_cdata_section_handler(void *data)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    /* the XML parser should already give an error when a CDATA section outside the root element is encountered.
     * this means we should always have a root element when we get here.
     */
    assert(info->element != NULL);

    if (info->element->type->type_class != coda_text_class)
    {
        /* the parent element no longer consists purely of other elements so we turn it into a text element */
        coda_xml_record_convert_to_text(info->element->type);
        if (coda_xml_dynamic_element_update(info->element) != 0)
        {
            abort_parser(info);
            return;
        }
    }

    if (info->element->cdata_delta_offset == 0)
    {
        info->element->cdata_delta_offset = (int32_t)
            (8 * (int64_t)(XML_GetCurrentByteIndex(info->parser) + XML_GetCurrentByteCount(info->parser)) -
             info->element->inner_bit_offset);
    }
    else if (info->element->cdata_delta_offset > 0)
    {
        /* this is a second CDATA section; we only allow single CDATA sections */
        info->element->cdata_delta_offset = -1;
    }
}

static void XMLCALL interpret_end_cdata_section_handler(void *data)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (info->element->cdata_delta_offset > 0)
    {
        /* temporarily store the CDATA inner size in the inner_bit_size field of the element */
        info->element->inner_bit_size = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser) -
            (info->element->cdata_delta_offset + info->element->inner_bit_size);
        /* set cdata_delta_size to -1 to indicate that our CDATA section has finished */
        info->element->cdata_delta_size = -1;
    }
}

static void XMLCALL interpret_skipped_entity_handler(void *data, const char *entity_name, int is_parameter_entity)
{
    data = data;
    entity_name = entity_name;
    if (!is_parameter_entity)
    {
        /* We need to treat this as character data -> call the character data handler with some dummy
         * non-whitespace string
         */
        interpret_character_data_handler(data, "&entity;", 8);
    }
}

static int XMLCALL not_standalone_handler(void *data)
{
    data = data;

    /* return an error if this is not a standalone file */
    return XML_STATUS_ERROR;
}

static int update_elements(coda_xml_element_dynamic_type *element)
{
    int i;

    if (coda_xml_dynamic_element_update(element) != 0)
    {
        return -1;
    }
    for (i = 0; i < element->num_elements; i++)
    {
        if (element->element[i] != NULL)
        {
            switch (element->element[i]->tag)
            {
                case tag_xml_record_dynamic:
                case tag_xml_text_dynamic:
                    if (update_elements((coda_xml_element_dynamic_type *)element->element[i]) != 0)
                    {
                        return -1;
                    }
                    break;
                case tag_xml_array_dynamic:
                    {
                        coda_xml_array_dynamic_type *array;
                        int j;

                        array = (coda_xml_array_dynamic_type *)element->element[i];
                        for (j = 0; j < array->num_elements; j++)
                        {
                            if (update_elements(array->element[j]) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }

    return 0;
}

int coda_xml_parse_and_interpret(coda_xml_product *product)
{
    char buff[BUFFSIZE];
    parser_info info;
    coda_xml_root *root;
    coda_xml_field *root_field;
    char *field_name;

    info.parser = XML_ParserCreateNS(NULL, ' ');
    if (info.parser == NULL)
    {
        coda_set_error(CODA_ERROR_XML, "could not create XML parser");
        return -1;
    }
    info.abort_parser = 0;
    info.product = product;
    root = coda_xml_root_new();
    if (root == NULL)
    {
        XML_ParserFree(info.parser);
        return -1;
    }
    info.root = coda_xml_dynamic_root_new(root);
    coda_xml_release_type((coda_xml_type *)root);
    if (info.root == NULL)
    {
        XML_ParserFree(info.parser);
        return -1;
    }
    info.element = NULL;
    info.unparsed_depth = 0;
    info.dictionary = new_element_dictionary();
    if (info.dictionary == NULL)
    {
        XML_ParserFree(info.parser);
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
        return -1;
    }

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, interpret_start_element_handler, interpret_end_element_handler);
    XML_SetCharacterDataHandler(info.parser, interpret_character_data_handler);
    XML_SetCdataSectionHandler(info.parser, interpret_start_cdata_section_handler, interpret_end_cdata_section_handler);
    XML_SetSkippedEntityHandler(info.parser, interpret_skipped_entity_handler);
    XML_SetNotStandaloneHandler(info.parser, not_standalone_handler);

    for (;;)
    {
        int length;
        int result;

        length = read(product->fd, buff, BUFFSIZE);
        if (length < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            XML_ParserFree(info.parser);
            coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
            delete_element_dictionary(info.dictionary);
            return -1;
        }

        coda_errno = 0;
        result = XML_Parse(info.parser, buff, length, (length == 0));
        if (result == XML_STATUS_ERROR || coda_errno != 0)
        {
            char s[21];

            if (coda_errno == 0)
            {
                coda_set_error(CODA_ERROR_XML, "xml parse error: %s", XML_ErrorString(XML_GetErrorCode(info.parser)));
            }
            coda_str64(XML_GetCurrentByteIndex(info.parser), s);
            coda_add_error_message(" (line: %lu, byte offset: %s)", (long)XML_GetCurrentLineNumber(info.parser), s);
            XML_ParserFree(info.parser);
            coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
            delete_element_dictionary(info.dictionary);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    XML_ParserFree(info.parser);
    delete_element_dictionary(info.dictionary);

    if (update_elements(info.root->element) != 0)
    {
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
        return -1;
    }
    /* link definition of root type to definition of first element */
    field_name = coda_identifier_from_name(coda_element_name_from_xml_name(info.root->element->type->xml_name), NULL);
    root_field = coda_xml_field_new(field_name);
    if (root_field == NULL)
    {
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
        free(field_name);
        return -1;
    }
    free(field_name);
    if (coda_xml_field_set_type(root_field, (coda_xml_type *)info.root->element->type) != 0)
    {
        coda_xml_field_delete(root_field);
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
        return -1;
    }
    if (coda_xml_root_set_field(root, root_field) != 0)
    {
        coda_xml_field_delete(root_field);
        coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
        return -1;
    }

    product->root_type = (coda_dynamic_type *)info.root;

    return 0;
}


static void XMLCALL definition_start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info;
    coda_xml_element *definition;
    coda_xml_element_dynamic_type *element;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth++;
        return;
    }

    if (info->element != NULL)
    {
        coda_xml_type *type;
        int element_index;

        if (info->element->type->type_class != coda_record_class)
        {
            /* all subelements of the parent element will be ignored because the parent element is not a record */
            info->unparsed_depth = 1;
            return;
        }

        /* check if a definition for this element is available */
        element_index = hashtable_get_index_from_name(info->element->type->xml_name_hash_data, el);
        if (element_index < 0)
        {
            element_index = hashtable_get_index_from_name(info->element->type->xml_name_hash_data,
                                                          coda_element_name_from_xml_name(el));
        }
        if (element_index < 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed within element '%s'", el,
                           info->element->type->xml_name);
            abort_parser(info);
            return;
        }
        type = info->element->type->field[element_index]->type;
        if (type->tag == tag_xml_array)
        {
            /* take the array element definition */
            definition = ((coda_xml_array *)type)->base_type;
        }
        else
        {
            definition = (coda_xml_element *)type;
        }
    }
    else
    {
        /* use the root definition from the product type  */
        definition = (coda_xml_element *)info->root->type->field->type;

        /* check if the current element equals the root element from the definition */
        if (strcmp(definition->xml_name, coda_element_name_from_xml_name(el)) != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "incorrect root element ('%s') for product", el);
            abort_parser(info);
            return;
        }
    }

    /* create a new element */
    element = coda_xml_dynamic_element_new(definition, attr);
    if (element == NULL)
    {
        abort_parser(info);
        return;
    }
    element->outer_bit_offset = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser);
    element->inner_bit_offset = element->outer_bit_offset + 8 * (int64_t)XML_GetCurrentByteCount(info->parser);

    if (info->element != NULL)
    {
        if (coda_xml_dynamic_element_add_element(info->element, element) != 0)
        {
            abort_parser(info);
            return;
        }
    }
    else
    {
        assert(info->root->element == NULL);
        info->root->element = element;
    }
    info->element = element;
}

static void XMLCALL definition_end_element_handler(void *data, const char *el)
{
    coda_xml_element_dynamic_type *element;
    parser_info *info;

    el = el;

    info = (parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth--;
        return;
    }

    assert(info->element != NULL);
    element = info->element;

    if (coda_xml_dynamic_element_validate(element) != 0)
    {
        abort_parser(info);
        return;
    }

    if (element->cdata_delta_offset > 0)
    {
        /* we use the CDATA content as content for this element -> update the delta value for the CDATA size */
        /* the size of the CDATA content was temporarily stored in inner_bit_size */
        element->cdata_delta_size = (int32_t)(element->inner_bit_size -
                                              8 * (int64_t)XML_GetCurrentByteIndex(info->parser));
    }
    else
    {
        /* no CDATA -> reset the CDATA delta values  */
        element->cdata_delta_offset = 0;
        element->cdata_delta_size = 0;
    }
    element->inner_bit_size = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser) - element->inner_bit_offset;
    element->outer_bit_size = 8 * (int64_t)(XML_GetCurrentByteIndex(info->parser) +
                                            XML_GetCurrentByteCount(info->parser)) - element->outer_bit_offset;
    /* apply the CDATA delta value to the inner offset and size */
    element->inner_bit_offset += element->cdata_delta_offset;
    element->inner_bit_size += element->cdata_delta_size;

    info->element = element->parent;
}

static void XMLCALL definition_character_data_handler(void *data, const char *s, int len)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (!is_whitespace(s, len))
    {
        /* the XML parser should already give an error for any non-whitespace data outside the root element.
         * this means we should always have a root element when we get here.
         */
        assert(info->element != NULL);

        if (info->element->tag == tag_xml_record_dynamic)
        {
            char s[21];

            abort_parser(info);
            coda_str64(XML_GetCurrentByteIndex(info->parser), s);
            coda_set_error(CODA_ERROR_PRODUCT, "non-whitespace character data not allowed for element '%s' "
                           "(line: %lu, byte offset: %s)", info->element->type->xml_name,
                           (long)XML_GetCurrentLineNumber(info->parser), s);
            return;
        }
        if (info->element->cdata_delta_offset == 0)
        {
            /* we have non-whitespace character data before any CDATA element so disable CDATA from here on */
            info->element->cdata_delta_offset = -1;
        }
        else if (info->element->cdata_delta_offset > 0 && info->element->cdata_delta_size != 0)
        {
            /* we have non-whitespace character data after a CDATA element so disable the CDATA */
            info->element->cdata_delta_offset = -1;
        }
    }
}

static void XMLCALL definition_start_cdata_section_handler(void *data)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    /* the XML parser should already give an error when a CDATA section outside the root element is encountered.
     * this means we should always have a root element when we get here.
     */
    assert(info->element != NULL);

    if (info->element->type->tag == tag_xml_record_dynamic)
    {
        char s[21];

        abort_parser(info);
        coda_str64(XML_GetCurrentByteIndex(info->parser), s);
        coda_set_error(CODA_ERROR_PRODUCT, "CDATA content not allowed for element '%s' "
                       "(line: %lu, byte offset: %s)", info->element->type->xml_name,
                       (long)XML_GetCurrentLineNumber(info->parser), s);
        return;
    }

    if (info->element->cdata_delta_offset == 0)
    {
        info->element->cdata_delta_offset = (int32_t)
            (8 * (int64_t)(XML_GetCurrentByteIndex(info->parser) + XML_GetCurrentByteCount(info->parser)) -
             info->element->inner_bit_offset);
    }
    else if (info->element->cdata_delta_offset > 0)
    {
        /* this is a second CDATA section; we only allow single CDATA sections */
        info->element->cdata_delta_offset = -1;
    }
}

static void XMLCALL definition_end_cdata_section_handler(void *data)
{
    parser_info *info;

    info = (parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (info->element->cdata_delta_offset > 0)
    {
        /* temporarily store the CDATA inner size in the inner_bit_size field of the element */
        info->element->inner_bit_size = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser) -
            (info->element->cdata_delta_offset + info->element->inner_bit_size);
        /* set cdata_delta_size to -1 to indicate that our CDATA section has finished */
        info->element->cdata_delta_size = -1;
    }
}

static void XMLCALL definition_skipped_entity_handler(void *data, const char *entity_name, int is_parameter_entity)
{
    data = data;
    entity_name = entity_name;
    if (!is_parameter_entity)
    {
        /* We need to treat this as character data -> call the character data handler with some dummy
         * non-whitespace string
         */
        definition_character_data_handler(data, "&entity;", 8);
    }
}

int coda_xml_parse_with_definition(coda_xml_product *product)
{
    char buff[BUFFSIZE];
    parser_info info;

    info.parser = XML_ParserCreateNS(NULL, ' ');
    if (info.parser == NULL)
    {
        coda_set_error(CODA_ERROR_XML, "could not create XML parser");
        return -1;
    }
    info.abort_parser = 0;
    info.product = product;
    info.root = coda_xml_dynamic_root_new((coda_xml_root *)product->product_definition->root_type);
    if (info.root == NULL)
    {
        XML_ParserFree(info.parser);
        return -1;
    }
    info.element = NULL;
    info.unparsed_depth = 0;
    info.dictionary = NULL;

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, definition_start_element_handler, definition_end_element_handler);
    XML_SetCharacterDataHandler(info.parser, definition_character_data_handler);
    XML_SetCdataSectionHandler(info.parser, definition_start_cdata_section_handler,
                               definition_end_cdata_section_handler);
    XML_SetSkippedEntityHandler(info.parser, definition_skipped_entity_handler);
    XML_SetNotStandaloneHandler(info.parser, not_standalone_handler);

    for (;;)
    {
        int length;
        int result;

        length = read(product->fd, buff, BUFFSIZE);
        if (length < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            XML_ParserFree(info.parser);
            coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
            return -1;
        }

        coda_errno = 0;
        result = XML_Parse(info.parser, buff, length, (length == 0));
        if (result == XML_STATUS_ERROR || coda_errno != 0)
        {
            char s[21];

            if (coda_errno == 0)
            {
                coda_set_error(CODA_ERROR_XML, "xml parse error: %s", XML_ErrorString(XML_GetErrorCode(info.parser)));
            }
            coda_str64(XML_GetCurrentByteIndex(info.parser), s);
            coda_add_error_message(" (line: %lu, byte offset: %s)", (long)XML_GetCurrentLineNumber(info.parser), s);
            XML_ParserFree(info.parser);
            coda_xml_release_dynamic_type((coda_xml_dynamic_type *)info.root);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    XML_ParserFree(info.parser);

    product->root_type = (coda_dynamic_type *)info.root;

    return 0;
}


struct detection_parser_info_struct
{
    XML_Parser parser;
    int abort_parser;
    int is_root_element;        /* do we have the xml root element? */
    int unparsed_depth; /* keep track of how deep we are in the XML hierarchy after leaving the detection tree */
    char *matchvalue;
    coda_xml_detection_node *detection_tree;
    coda_product_definition *product_definition;
};
typedef struct detection_parser_info_struct detection_parser_info;

static void abort_detection_parser(detection_parser_info *info, int code)
{
    XML_StopParser(info->parser, 0);
    /* we use code=1 for abnormal termination and code=2 for normal termination (i.e. further parsing is not needed) */
    info->abort_parser = code;
}

static void XMLCALL detection_string_handler(void *data, const char *s, int len)
{
    detection_parser_info *info;

    info = (detection_parser_info *)data;
    if (info->unparsed_depth == 0)
    {
        if (info->matchvalue == NULL)
        {
            info->matchvalue = malloc(len + 1);
            if (info->matchvalue == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)len + 1, __FILE__, __LINE__);
                abort_detection_parser(info, 1);
                return;
            }
            memcpy(info->matchvalue, s, len);
            info->matchvalue[len] = '\0';
        }
        else
        {
            char *matchvalue;
            long current_length = strlen(info->matchvalue);

            matchvalue = malloc(current_length + len + 1);
            if (matchvalue == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               current_length + len + 1, __FILE__, __LINE__);
                abort_detection_parser(info, 1);
                return;
            }
            memcpy(matchvalue, info->matchvalue, current_length);
            memcpy(&matchvalue[current_length], s, len);
            matchvalue[current_length + len] = '\0';
            free(info->matchvalue);
            info->matchvalue = matchvalue;
        }
    }
}

static void XMLCALL detection_start_element_handler(void *data, const char *el, const char **attr)
{
    detection_parser_info *info;

    attr = attr;

    info = (detection_parser_info *)data;
    if (info->unparsed_depth == 0)
    {
        coda_xml_detection_node *subnode;

        subnode = coda_xml_detection_node_get_subnode(info->detection_tree, el);
        if (subnode != NULL)
        {
            int i;

            /* go one step deeper into the expression node tree */
            info->detection_tree = subnode;
            info->is_root_element = 0;

            /* check if a product type matches */
            for (i = 0; i < info->detection_tree->num_detection_rules; i++)
            {
                if (info->detection_tree->detection_rule[i]->entry[0]->value == NULL)
                {
                    /* we don't have to match the value, only the path matters -> product type found */
                    info->product_definition = info->detection_tree->detection_rule[i]->product_definition;
                    abort_detection_parser(info, 1);
                    return;
                }
            }

            /* reset matchvalue */
            if (info->matchvalue != NULL)
            {
                free(info->matchvalue);
                info->matchvalue = NULL;
            }
        }
        else if (info->is_root_element)
        {
            /* if we can't find a match for the root element we can skip further parsing */
            abort_detection_parser(info, 2);
            return;
        }
        else
        {
            info->unparsed_depth = 1;
        }
    }
    else
    {
        info->unparsed_depth++;
    }
}

static void XMLCALL detection_end_element_handler(void *data, const char *el)
{
    detection_parser_info *info;

    el = el;

    info = (detection_parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth == 0)
    {
        if (info->matchvalue != NULL)
        {
            int i;

            /* check if a product type matches */
            for (i = 0; i < info->detection_tree->num_detection_rules; i++)
            {
                if (info->detection_tree->detection_rule[i]->entry[0]->value != NULL)
                {
                    if (strcmp(info->detection_tree->detection_rule[i]->entry[0]->value, info->matchvalue) == 0)
                    {
                        /* we have a match -> product type found */
                        info->product_definition = info->detection_tree->detection_rule[i]->product_definition;
                        abort_detection_parser(info, 1);
                        return;
                    }
                }
            }
            free(info->matchvalue);
            info->matchvalue = NULL;
        }
        /* go one step back in the expression node tree */
        info->detection_tree = info->detection_tree->parent;
    }
    else
    {
        info->unparsed_depth--;
    }
}

int coda_xml_parse_for_detection(int fd, const char *filename, coda_product_definition **definition)
{
    detection_parser_info info;
    char buff[BUFFSIZE];

    info.detection_tree = coda_xml_get_detection_tree();
    if (info.detection_tree == NULL)
    {
        return 0;
    }

    info.parser = XML_ParserCreateNS(NULL, ' ');
    if (info.parser == NULL)
    {
        coda_set_error(CODA_ERROR_XML, "could not create XML parser");
        return -1;
    }
    info.abort_parser = 0;
    info.is_root_element = 1;   /* first element that gets parsed is the root element */
    info.unparsed_depth = 0;
    info.matchvalue = NULL;
    info.detection_tree = coda_xml_get_detection_tree();
    info.product_definition = NULL;

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, detection_start_element_handler, detection_end_element_handler);
    XML_SetCharacterDataHandler(info.parser, detection_string_handler);
    XML_SetNotStandaloneHandler(info.parser, not_standalone_handler);

    for (;;)
    {
        int length;
        int result;

        length = read(fd, buff, BUFFSIZE);
        if (length < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", filename, strerror(errno));
            XML_ParserFree(info.parser);
            return -1;
        }

        coda_errno = 0;
        result = XML_Parse(info.parser, buff, length, (length == 0));
        if (info.matchvalue != NULL)
        {
            free(info.matchvalue);
            info.matchvalue = NULL;
        }
        if (info.product_definition != NULL || info.abort_parser == 2)
        {
            break;
        }
        if (result == XML_STATUS_ERROR || coda_errno != 0)
        {
            char s[21];

            if (coda_errno == 0)
            {
                coda_set_error(CODA_ERROR_XML, "xml parse error: %s", XML_ErrorString(XML_GetErrorCode(info.parser)));
            }
            coda_str64(XML_GetCurrentByteIndex(info.parser), s);
            coda_add_error_message(" (line: %lu, byte offset: %s)", (long)XML_GetCurrentLineNumber(info.parser), s);
            XML_ParserFree(info.parser);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    XML_ParserFree(info.parser);

    *definition = info.product_definition;

    return 0;
}
