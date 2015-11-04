/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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
    text_definition->retain_count++;

    return 0;
}

static coda_mem_record *attribute_record_new(coda_type_record *definition, coda_xml_product *product, const char **attr,
                                             int update_definition)
{
    coda_mem_record *attributes;
    int attribute_index;
    int i;

    assert(definition != NULL);
    attributes = coda_mem_record_new(definition, NULL);

    /* add attributes to attribute list */
    for (i = 0; attr[2 * i] != NULL; i++)
    {
        coda_mem_data *attribute;
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
                attribute = coda_mem_string_new(attribute_definition, NULL, (coda_product *)product, attr[2 * i + 1]);
                coda_type_release((coda_type *)attribute_definition);
            }
            else if (attributes->field_type[attribute_index] != NULL)
            {
                /* we only add the first occurence when there are multiple attributes with the same attribute name */
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
            attribute = coda_mem_string_new((coda_type_text *)definition->field[attribute_index]->type, NULL,
                                            (coda_product *)product, attr[2 * i + 1]);
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
    for (i = 0; i < info->depth; i++)
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

    info->depth++;
    if (info->depth >= CODA_CURSOR_MAXDEPTH)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "xml file exceeds maximum supported hierarchical depth (%d)",
                       CODA_CURSOR_MAXDEPTH);
        abort_parser(info);
        return;
    }

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
        if (attr[0] != NULL)
        {
            if (info->update_definition)
            {
                definition->attributes = coda_type_record_new(coda_format_xml);
                if (definition->attributes == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->attributes = (coda_dynamic_type *)attribute_record_new(definition->attributes, info->product,
                                                                             attr, info->update_definition);
                if (info->attributes == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            else
            {
                coda_set_error(CODA_ERROR_PRODUCT, "xml attribute '%s' is not allowed", attr[0]);
                abort_parser(info);
                return;
            }
        }
    }
    else
    {
        info->attributes = (coda_dynamic_type *)attribute_record_new(definition->attributes, info->product, attr,
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
    info.update_definition = (product->product_definition == NULL);
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
    num_blocks = (product->raw_product->file_size / BUFFSIZE);
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
            length = read(((coda_bin_product *)product->raw_product)->fd, buff, BUFFSIZE);
            if (length < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                               strerror(errno));
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
    info.depth = 0;

    parser_info_cleanup(&info);

    return 0;
}


struct detection_parser_info_struct
{
    XML_Parser parser;
    int abort_parser;
    const char *filename;
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

static int detection_match_rule(detection_parser_info *info, coda_detection_rule *detection_rule)
{
    int i;

    /* detection rules for i>0 are based on filenames */
    for (i = 1; i < detection_rule->num_entries; i++)
    {
        coda_detection_rule_entry *entry = detection_rule->entry[i];

        /* match value on filename */
        if (entry->offset + entry->value_length > (int64_t)strlen(info->filename))
        {
            /* filename is not long enough for a match */
            return 0;
        }
        if (memcmp(&info->filename[entry->offset], entry->value, entry->value_length) != 0)
        {
            /* no match */
            return 0;
        }
    }
    return 1;
}

static void XMLCALL detection_character_data_handler(void *data, const char *s, int len)
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

            matchvalue = realloc(info->matchvalue, current_length + len + 1);
            if (matchvalue == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               current_length + len + 1, __FILE__, __LINE__);
                abort_detection_parser(info, 1);
                return;
            }
            memcpy(&matchvalue[current_length], s, len);
            matchvalue[current_length + len] = '\0';
            info->matchvalue = matchvalue;
        }
    }
}

static void XMLCALL detection_start_element_handler(void *data, const char *el, const char **attr)
{
    detection_parser_info *info;

    (void)attr;

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

            if (info->detection_tree->num_attribute_subnodes > 0 && attr[0] != NULL)
            {
                for (i = 0; attr[2 * i] != NULL; i++)
                {
                    subnode = coda_xml_detection_node_get_attribute_subnode(info->detection_tree, attr[2 * i]);
                    if (subnode != NULL)
                    {
                        int j;

                        /* check if a product type matches */
                        for (j = 0; j < subnode->num_detection_rules; j++)
                        {
                            if ((subnode->detection_rule[j]->entry[0]->value == NULL ||
                                 strcmp(subnode->detection_rule[j]->entry[0]->value, attr[2 * i + 1]) == 0) &&
                                detection_match_rule(info, subnode->detection_rule[j]))
                            {
                                /* product type found */
                                info->product_definition = subnode->detection_rule[j]->product_definition;
                                abort_detection_parser(info, 1);
                                return;
                            }
                        }
                    }
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

    (void)el;

    info = (detection_parser_info *)data;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth == 0)
    {
        int i;

        /* check if a product type matches */
        for (i = 0; i < info->detection_tree->num_detection_rules; i++)
        {
            coda_detection_rule *rule = info->detection_tree->detection_rule[i];

            if ((rule->entry[0]->value_length == 0 ||
                 (info->matchvalue != NULL && strcmp(rule->entry[0]->value, info->matchvalue) == 0)) &&
                detection_match_rule(info, rule))
            {
                /* we have a match -> product type found */
                info->product_definition = info->detection_tree->detection_rule[i]->product_definition;
                abort_detection_parser(info, 1);
                return;
            }
        }
        if (info->matchvalue != NULL)
        {
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
    info.product_definition = NULL;

    info.filename = strrchr(filename, '/');
    if (info.filename == NULL)
    {
        info.filename = strrchr(filename, '\\');
    }
    if (info.filename == NULL)
    {
        info.filename = filename;
    }
    else
    {
        info.filename = &info.filename[1];
    }

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, detection_start_element_handler, detection_end_element_handler);
    XML_SetCharacterDataHandler(info.parser, detection_character_data_handler);
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
            if (info.matchvalue != NULL)
            {
                free(info.matchvalue);
                info.matchvalue = NULL;
            }
            XML_ParserFree(info.parser);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    if (info.matchvalue != NULL)
    {
        free(info.matchvalue);
        info.matchvalue = NULL;
    }
    XML_ParserFree(info.parser);

    *definition = info.product_definition;

    return 0;
}
