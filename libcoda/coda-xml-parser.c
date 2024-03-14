/*
 * Copyright (C) 2007-2024 S[&]T, The Netherlands.
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

#include "coda-xml-internal.h"
#include "coda-mem-internal.h"

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

static int convert_to_text(coda_type **definition)
{
    coda_type *text_definition;

    assert((*definition)->type_class == coda_record_class && (*definition)->format == coda_format_xml);

    text_definition = (coda_type *)coda_type_text_new(coda_format_xml);
    if (text_definition == NULL)
    {
        return -1;
    }
    if ((*definition)->attributes != NULL)
    {
        text_definition->attributes = (*definition)->attributes;
        text_definition->attributes->retain_count++;
    }
    coda_type_release(*definition);
    *definition = text_definition;

    return 0;
}

static coda_mem_record *attribute_record_new(coda_type_record *definition, coda_xml_product *product, const char *el,
                                             const char **attr, int update_definition)
{
    coda_mem_record *attributes;
    coda_mem_data *attribute;
    int update_mem_record = update_definition;
    int attribute_index;
    int i;

    assert(definition != NULL);
    assert(!definition->is_union);
    attributes = coda_mem_record_new(definition, NULL);

    if (el != coda_element_name_from_xml_name(el))
    {
        /* store the namespace part of the full xml name as an 'xmlns' attribute */
        attribute_index = hashtable_get_index_from_name(definition->real_name_hash_data, "xmlns");
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
                attribute = coda_mem_data_new((coda_type *)attribute_definition, NULL, (coda_product *)product,
                                              (long)(coda_element_name_from_xml_name(el) - el - 1),
                                              (const uint8_t *)el);
                coda_type_release((coda_type *)attribute_definition);
            }
            else
            {
                assert(attributes->field_type[attribute_index] == NULL);
                attribute = coda_mem_data_new(definition->field[attribute_index]->type, NULL, (coda_product *)product,
                                              (long)(coda_element_name_from_xml_name(el) - el - 1),
                                              (const uint8_t *)el);
                update_mem_record = 0;
            }
            if (attribute == NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
            if (coda_mem_record_add_field(attributes, "xmlns", (coda_dynamic_type *)attribute, update_mem_record) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attribute);
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
        }
        else if (attribute_index >= 0)
        {
            attribute = coda_mem_data_new(definition->field[attribute_index]->type, NULL, (coda_product *)product,
                                          (long)(coda_element_name_from_xml_name(el) - el - 1), (const uint8_t *)el);
            if (attribute == NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
            if (coda_mem_record_add_field(attributes, "xmlns", (coda_dynamic_type *)attribute, update_mem_record) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attribute);
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                return NULL;
            }
        }
    }

    /* add attributes to attribute list */
    for (i = 0; attr[2 * i] != NULL; i++)
    {
        const char *real_name = attr[2 * i];

        update_mem_record = update_definition;
        attribute_index = hashtable_get_index_from_name(definition->real_name_hash_data, real_name);
        if (attribute_index < 0)
        {
            attribute_index = hashtable_get_index_from_name(definition->real_name_hash_data,
                                                            coda_element_name_from_xml_name(real_name));
            if (attribute_index >= 0)
            {
                real_name = coda_element_name_from_xml_name(real_name);
            }
        }
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
                attribute = coda_mem_string_new(attribute_definition, NULL, (coda_product *)product, attr[2 * i + 1]);
                coda_type_release((coda_type *)attribute_definition);
            }
            else if (attributes->field_type[attribute_index] != NULL)
            {
                /* we only add the first occurrence when there are multiple attributes with the same attribute name */
                continue;
            }
            else
            {
                attribute = coda_mem_string_new((coda_type_text *)definition->field[attribute_index]->type, NULL,
                                                (coda_product *)product, attr[2 * i + 1]);
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
            attribute = coda_mem_data_new(definition->field[attribute_index]->type, NULL, (coda_product *)product,
                                          (long)strlen(attr[2 * i + 1]), (uint8_t *)attr[2 * i + 1]);
        }
        if (attribute == NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            return NULL;
        }

        if (coda_mem_record_add_field(attributes, real_name, (coda_dynamic_type *)attribute, update_mem_record) != 0)
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

struct parser_info_struct
{
    XML_Parser parser;
    int abort_parser;
    coda_xml_product *product;
    int depth;
    coda_type **definition[CODA_CURSOR_MAXDEPTH];
    coda_mem_record *record[CODA_CURSOR_MAXDEPTH];
    long index[CODA_CURSOR_MAXDEPTH];
    const char *xml_name[CODA_CURSOR_MAXDEPTH];
    coda_dynamic_type *attributes;
    int update_definition;      /* 1: we are interpreting the XML file dynamically; 0: external definition is used */
    long value_length;  /* number of used characters within value buffer */
    long value_size;    /* allocated size for value buffer */
    char *value;
};
typedef struct parser_info_struct parser_info;

static void parser_info_cleanup(parser_info *info)
{
    int i;

    if (info->parser != NULL)
    {
        XML_ParserFree(info->parser);
    }
    for (i = 0; i <= info->depth; i++)
    {
        if (info->record[i] != NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)info->record[i]);
        }
    }
    if (info->attributes != NULL)
    {
        coda_dynamic_type_delete(info->attributes);
    }
    if (info->value != NULL)
    {
        free(info->value);
    }
}

static void parser_info_init(parser_info *info)
{
    info->parser = NULL;
    info->abort_parser = 0;
    info->product = NULL;
    info->depth = -1;
    info->attributes = NULL;
    info->update_definition = 0;
    info->value_length = 0;
    info->value_size = 0;
    info->value = NULL;
}

static void abort_parser(parser_info *info)
{
    XML_StopParser(info->parser, 0);
    /* we need to explicitly check in the end handlers for parsing abort because expat may still call the end handler
     * after an abort in the start handler */
    info->abort_parser = 1;
}

static int XMLCALL not_standalone_handler(void *data)
{
    (void)data;

    /* return an error if this is not a standalone file */
    return XML_STATUS_ERROR;
}

static void XMLCALL start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info;
    coda_type *definition;
    coda_mem_record *parent;
    int index;

    info = (parser_info *)data;

    if (info->record[info->depth] != NULL)
    {
        if (info->record[info->depth]->definition->format != coda_format_xml)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' not allowed inside %s data",
                           info->xml_name[info->depth],
                           coda_type_get_format_name(info->record[info->depth]->definition->format));
            abort_parser(info);
            return;
        }
    }
    else
    {
        coda_set_error(CODA_ERROR_PRODUCT, "mixed content for element '%s' is not supported",
                       info->xml_name[info->depth]);
        abort_parser(info);
        return;
    }

    info->value_length = 0;

    if (info->depth >= CODA_CURSOR_MAXDEPTH - 1)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "xml file exceeds maximum supported hierarchical depth (%d)",
                       CODA_CURSOR_MAXDEPTH);
        abort_parser(info);
        return;
    }
    info->depth++;

    info->record[info->depth] = NULL;
    parent = info->record[info->depth - 1];
    index = hashtable_get_index_from_name(parent->definition->real_name_hash_data, el);
    if (index < 0)
    {
        index = hashtable_get_index_from_name(parent->definition->real_name_hash_data,
                                              coda_element_name_from_xml_name(el));
    }
    if (index < 0)
    {
        if (info->update_definition)
        {
            /* all xml elements start out as empty records */
            definition = (coda_type *)coda_type_record_new(coda_format_xml);
            if (definition == NULL)
            {
                abort_parser(info);
                return;
            }
            if (coda_type_record_create_field(parent->definition, el, definition) != 0)
            {
                coda_type_release(definition);
                abort_parser(info);
                return;
            }
            coda_type_release(definition);

            if (coda_mem_type_update((coda_dynamic_type **)&parent, (coda_type *)parent->definition) != 0)
            {
                abort_parser(info);
                return;
            }
            /* updating the parent should only have changed the fields, and not the main record */
            assert(parent == info->record[info->depth - 1]);
            index = hashtable_get_index_from_name(parent->definition->real_name_hash_data, el);
            assert(index >= 0);
        }
        else
        {
            if (info->depth == 1)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed as root element", el);
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed within element '%s'", el,
                               info->xml_name[info->depth - 1]);
            }
            abort_parser(info);
            return;
        }
    }
    info->index[info->depth] = index;
    info->definition[info->depth] = &parent->definition->field[index]->type;
    if (coda_type_get_record_field_real_name((coda_type *)parent->definition, index, &info->xml_name[info->depth]) != 0)
    {
        abort_parser(info);
        return;
    }
    definition = *info->definition[info->depth];

    if (definition->type_class == coda_array_class)
    {
        /* use the base type when the definition points to an array of xml elements */
        if (definition->format == coda_format_xml)
        {
            if (parent->field_type[index] == NULL)
            {
                parent->field_type[index] = (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)definition,
                                                                                    NULL);
                if (parent->field_type[index] == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            /* take the array element definition */
            info->definition[info->depth] = &((coda_type_array *)definition)->base_type;
            definition = *info->definition[info->depth];
        }
    }
    else if (parent->field_type[index] != NULL)
    {
        if (info->update_definition)
        {
            coda_mem_array *array;
            coda_type_array *array_definition;

            /* change scalar to array in definition */
            array_definition = coda_type_array_new(coda_format_xml);
            if (array_definition == NULL)
            {
                abort_parser(info);
                return;
            }
            if (coda_type_array_set_base_type(array_definition, definition) != 0)
            {
                coda_type_release((coda_type *)array_definition);
                abort_parser(info);
                return;
            }
            *info->definition[info->depth] = (coda_type *)array_definition;
            coda_type_release(definition);
            if (coda_type_array_add_variable_dimension(array_definition, NULL) != 0)
            {
                abort_parser(info);
                return;
            }

            /* create the array and add the existing element */
            array = coda_mem_array_new(array_definition, NULL);
            if (array == NULL)
            {
                abort_parser(info);
                return;
            }
            if (coda_mem_array_add_element(array, parent->field_type[index]) != 0)
            {
                abort_parser(info);
                return;
            }
            parent->field_type[index] = (coda_dynamic_type *)array;

            /* take the array element definition */
            info->definition[info->depth] = &array_definition->base_type;
            definition = *info->definition[info->depth];
        }
        else
        {
            coda_set_error(CODA_ERROR_PRODUCT, "xml element '%s' is not allowed more than once within element '%s'",
                           el, info->xml_name[info->depth - 1]);
            abort_parser(info);
            return;
        }
    }

    /* create attributes record */
    if (definition->attributes == NULL)
    {
        info->attributes = NULL;
        if (info->update_definition)
        {
            if (attr[0] != NULL || el != coda_element_name_from_xml_name(el))
            {
                definition->attributes = coda_type_record_new(coda_format_xml);
                if (definition->attributes == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->attributes = (coda_dynamic_type *)attribute_record_new(definition->attributes, info->product,
                                                                             el, attr, info->update_definition);
                if (info->attributes == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
        }
        else
        {
            if (attr[0] != NULL)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml attribute '%s' is not allowed", attr[0]);
                abort_parser(info);
                return;
            }
        }
    }
    else
    {
        info->attributes = (coda_dynamic_type *)attribute_record_new(definition->attributes, info->product, el, attr,
                                                                     info->update_definition);
        if (info->attributes == NULL)
        {
            abort_parser(info);
            return;
        }
    }

    /* xml records are already created here in order to allow adding child xml elements */
    if (definition->format == coda_format_xml && definition->type_class == coda_record_class)
    {
        int i;

        assert(!((coda_type_record *)definition)->is_union);
        info->record[info->depth] = coda_mem_record_new((coda_type_record *)definition, info->attributes);
        if (info->record[info->depth] == NULL)
        {
            abort_parser(info);
            return;
        }
        /* create empty arrays for array child elements */
        for (i = 0; i < info->record[info->depth]->num_fields; i++)
        {
            if (((coda_type_record *)definition)->field[i]->type->type_class == coda_array_class &&
                ((coda_type_record *)definition)->field[i]->type->format == coda_format_xml)
            {
                coda_type *array_definition = ((coda_type_record *)definition)->field[i]->type;

                info->record[info->depth]->field_type[i] =
                    (coda_dynamic_type *)coda_mem_array_new((coda_type_array *)array_definition, NULL);
                if (info->record[info->depth]->field_type[i] == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
        }
        info->attributes = NULL;
    }
}

static void XMLCALL end_element_handler(void *data, const char *el)
{
    parser_info *info = (parser_info *)data;
    coda_mem_record *parent;
    coda_mem_type *type;
    int index;

    (void)el;

    if (info->abort_parser)
    {
        return;
    }

    /* we are dealing with a record if info->record[info->depth] != NULL */
    if (info->record[info->depth] != NULL && info->value_length > 0 && !is_whitespace(info->value, info->value_length))
    {
        assert(info->update_definition);        /* other case is already handled in character_data_handler() */
        if (((coda_type_record *)info->record[info->depth]->definition)->num_fields > 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "mixed content for element '%s' is not supported",
                           info->xml_name[info->depth]);
            abort_parser(info);
            return;
        }
        /* convert definition from record to text */
        info->attributes = info->record[info->depth]->attributes;
        info->record[info->depth]->attributes = NULL;
        if (convert_to_text(info->definition[info->depth]) != 0)
        {
            abort_parser(info);
            return;
        }
        /* delete the record we created in start_element_handler() */
        coda_dynamic_type_delete((coda_dynamic_type *)info->record[info->depth]);
        info->record[info->depth] = NULL;
    }

    if (info->record[info->depth] == NULL)
    {
        coda_type *definition = *info->definition[info->depth];

        if (definition->type_class == coda_special_class)
        {
            coda_dynamic_type *base_type;

            assert(!info->update_definition);

            base_type = (coda_dynamic_type *)coda_mem_data_new(((coda_type_special *)definition)->base_type, NULL,
                                                               (coda_product *)info->product, info->value_length,
                                                               (uint8_t *)info->value);
            if (base_type == NULL)
            {
                abort_parser(info);
                return;
            }

            type = (coda_mem_type *)coda_mem_time_new((coda_type_special *)definition, info->attributes, base_type);
            if (type == NULL)
            {
                coda_dynamic_type_delete(base_type);
                abort_parser(info);
                return;
            }
        }
        else
        {
            type = (coda_mem_type *)coda_mem_data_new(definition, info->attributes, (coda_product *)info->product,
                                                      info->value_length, (uint8_t *)info->value);
            if (type == NULL)
            {
                abort_parser(info);
                return;
            }
        }
        info->attributes = NULL;
    }
    else
    {
        if (!info->update_definition)
        {
            int i;

            if (coda_mem_record_validate(info->record[info->depth]) != 0)
            {
                abort_parser(info);
                return;
            }

            /* also validate all fields that are arrays of xml elements */
            for (i = 0; i < info->record[info->depth]->num_fields; i++)
            {
                coda_dynamic_type *field_type = info->record[info->depth]->field_type[i];

                if (field_type != NULL)
                {
                    if (field_type->definition->type_class == coda_array_class &&
                        field_type->definition->format == coda_format_xml)
                    {
                        if (coda_mem_array_validate((coda_mem_array *)field_type) != 0)
                        {
                            abort_parser(info);
                            return;
                        }
                        /* if the array is empty and the field is optional then remove the array */
                        if (((coda_mem_array *)field_type)->num_elements == 0 &&
                            info->record[info->depth]->definition->field[i]->optional)
                        {
                            coda_mem_type_delete(field_type);
                            info->record[info->depth]->field_type[i] = NULL;
                        }
                    }
                }
            }
        }
        type = (coda_mem_type *)info->record[info->depth];
        info->record[info->depth] = NULL;
    }

    assert(info->attributes == NULL);

    parent = info->record[info->depth - 1];
    index = info->index[info->depth];
    if (parent->field_type[index] != NULL)
    {
        /* add the child element to the array */
        assert(parent->field_type[index]->definition->type_class == coda_array_class &&
               parent->field_type[index]->definition->format == coda_format_xml);
        if (coda_mem_array_add_element((coda_mem_array *)parent->field_type[index], (coda_dynamic_type *)type) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)type);
            abort_parser(info);
            return;
        }
    }
    else
    {
        parent->field_type[index] = (coda_dynamic_type *)type;
    }

    info->depth--;
    info->value_length = 0;
}

static void XMLCALL character_data_handler(void *data, const char *s, int len)
{
    parser_info *info = (parser_info *)data;

    if (!info->update_definition && info->record[info->depth] != NULL && !is_whitespace(s, len))
    {
        coda_set_error(CODA_ERROR_PRODUCT, "non-whitespace character data not allowed for element '%s'",
                       info->xml_name[info->depth]);
        abort_parser(info);
        return;
    }

    /* add character data to our string value */
    if (info->value_length + len > info->value_size)
    {
        char *new_value;

        new_value = realloc(info->value, info->value_length + len);
        if (new_value == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                           info->value_length + len, __FILE__, __LINE__);
            abort_parser(info);
            return;
        }
        info->value = new_value;
        info->value_size = info->value_length + len;
    }
    memcpy(&info->value[info->value_length], s, len);
    info->value_length += len;
}

int coda_xml_parse(coda_xml_product *product)
{
    char buff[BUFFSIZE];
    parser_info info;
    long num_blocks;
    long i;

    parser_info_init(&info);
    info.parser = XML_ParserCreateNS(NULL, ' ');
    if (info.parser == NULL)
    {
        coda_set_error(CODA_ERROR_XML, "could not create XML parser");
        return -1;
    }
    info.product = product;
    info.update_definition = (product->product_definition == NULL || product->product_definition->root_type == NULL);
    /* the root of the product is always a record, which will contain the top-level xml element as a field */
    if (info.update_definition)
    {
        coda_type_record *definition;

        definition = coda_type_record_new(coda_format_xml);
        if (definition == NULL)
        {
            XML_ParserFree(info.parser);
            return -1;
        }
        info.record[0] = coda_mem_record_new(definition, NULL);
        coda_type_release((coda_type *)definition);
    }
    else
    {
        assert(product->product_definition->root_type->type_class == coda_record_class);
        assert(!((coda_type_record *)product->product_definition->root_type)->is_union);
        info.record[0] = coda_mem_record_new((coda_type_record *)product->product_definition->root_type, NULL);
    }
    if (info.record[0] == NULL)
    {
        parser_info_cleanup(&info);
        return -1;
    }
    info.definition[0] = (coda_type **)&info.record[0]->definition;
    info.index[0] = -1;
    info.xml_name[0] = NULL;
    info.depth = 0;

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, start_element_handler, end_element_handler);
    XML_SetCharacterDataHandler(info.parser, character_data_handler);
    XML_SetNotStandaloneHandler(info.parser, not_standalone_handler);

    /* we also need to parse in blocks for mmap-ed files since the file size may exceed MAX_INT */
    num_blocks = (long)(product->raw_product->file_size / BUFFSIZE);
    if (product->raw_product->file_size > num_blocks * BUFFSIZE)
    {
        num_blocks++;
    }
    for (i = 0; i < num_blocks; i++)
    {
        const char *buff_ptr;
        int length;
        int result;

        if (((coda_bin_product *)product->raw_product)->use_mmap)
        {
            if (i < num_blocks - 1)
            {
                length = BUFFSIZE;
            }
            else
            {
                length = (int)(product->raw_product->file_size - (num_blocks - 1) * BUFFSIZE);
            }
            buff_ptr = (const char *)&(product->raw_product->mem_ptr[i * BUFFSIZE]);
        }
        else
        {
            if (lseek(((coda_bin_product *)product->raw_product)->fd, (off_t)i * BUFFSIZE, SEEK_SET) < 0)
            {
                char byte_offset_str[21];

                coda_str64(i * BUFFSIZE, byte_offset_str);
                coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s (%s)", byte_offset_str,
                               strerror(errno));
                parser_info_cleanup(&info);
                return -1;
            }
            length = read(((coda_bin_product *)product->raw_product)->fd, buff, BUFFSIZE);
            if (length < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
                parser_info_cleanup(&info);
                return -1;
            }
            buff_ptr = buff;
        }

        coda_errno = 0;
        result = XML_Parse(info.parser, buff_ptr, length, (i == num_blocks - 1));
        if (result == XML_STATUS_ERROR || coda_errno != 0)
        {
            char s[21];

            if (coda_errno == 0)
            {
                coda_set_error(CODA_ERROR_XML, "xml parse error: %s", XML_ErrorString(XML_GetErrorCode(info.parser)));
            }
            coda_str64(XML_GetCurrentByteIndex(info.parser), s);
            coda_add_error_message(" (line: %lu, byte offset: %s)", (long)XML_GetCurrentLineNumber(info.parser), s);
            parser_info_cleanup(&info);
            return -1;
        }
    }

    XML_ParserFree(info.parser);
    info.parser = NULL;

    if (info.update_definition)
    {
        if (coda_mem_type_update((coda_dynamic_type **)&info.record[0], (coda_type *)info.record[0]->definition) != 0)
        {
            parser_info_cleanup(&info);
            return -1;
        }
    }

    product->root_type = (coda_dynamic_type *)info.record[0];
    info.depth = -1;

    parser_info_cleanup(&info);

    return 0;
}
