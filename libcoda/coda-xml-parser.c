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

struct parser_info_struct
{
    XML_Parser parser;
    int abort_parser;
    coda_xml_product *product;
    coda_xml_root *root;
    coda_xml_element *element;
    int update_definition;      /* 1: we are interpreting the XML file dynamically; 0: external definition is used */
    int unparsed_depth; /* keep track of how deep we are in an XML element that we interpret as text */
};
typedef struct parser_info_struct parser_info;

static void abort_parser(parser_info *info)
{
    XML_StopParser(info->parser, 0);
    /* we need to explicitly check in the end handlers for parsing abort because expat may still call the end handler
     * after an abort in the start handler */
    info->abort_parser = 1;
}

static int XMLCALL not_standalone_handler(void *data)
{
    data = data;

    /* return an error if this is not a standalone file */
    return XML_STATUS_ERROR;
}

static void XMLCALL start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info;
    int64_t outer_bit_offset;
    int64_t inner_bit_offset;

    info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth++;
        return;
    }

    outer_bit_offset = 8 * (int64_t)XML_GetCurrentByteIndex(info->parser);
    inner_bit_offset = outer_bit_offset + 8 * (int64_t)XML_GetCurrentByteCount(info->parser);
    if (info->element != NULL)
    {
        if (info->element->definition->type_class != coda_record_class ||
            info->element->definition->format != coda_format_xml)
        {
            /* all subelements of the parent element will be ignored because the parent element is not an xml record */
            info->unparsed_depth = 1;
            return;
        }
        if (coda_xml_element_add_element(info->element, el, attr, outer_bit_offset, inner_bit_offset,
                                         info->update_definition, &info->element) != 0)
        {
            abort_parser(info);
            return;
        }
    }
    else
    {
        /* use the root definition from the product type  */
        if (coda_xml_root_add_element(info->root, el, attr, outer_bit_offset, inner_bit_offset,
                                      info->update_definition) != 0)
        {
            abort_parser(info);
            return;
        }
        info->element = info->root->element;
    }
}

static void XMLCALL end_element_handler(void *data, const char *el)
{
    coda_xml_element *element;
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

    if (!info->update_definition)
    {
        if (coda_xml_element_validate(element) != 0)
        {
            abort_parser(info);
            return;
        }
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

static void XMLCALL character_data_handler(void *data, const char *s, int len)
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

        if (info->element->definition->type_class == coda_record_class)
        {
            if (info->update_definition)
            {
                if (coda_xml_element_convert_to_text(info->element) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            else
            {
                char s[21];

                abort_parser(info);
                coda_str64(XML_GetCurrentByteIndex(info->parser), s);
                coda_set_error(CODA_ERROR_PRODUCT, "non-whitespace character data not allowed for element '%s' "
                               "(line: %lu, byte offset: %s)", info->element->xml_name,
                               (long)XML_GetCurrentLineNumber(info->parser), s);
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

static void XMLCALL start_cdata_section_handler(void *data)
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

    if (info->element->definition->type_class == coda_record_class)
    {
        if (info->update_definition)
        {
            if (coda_xml_element_convert_to_text(info->element) != 0)
            {
                abort_parser(info);
                return;
            }
        }
        else
        {
            char s[21];

            abort_parser(info);
            coda_str64(XML_GetCurrentByteIndex(info->parser), s);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "CDATA content not allowed for element '%s' (line: %lu, byte offset: %s)",
                           info->element->xml_name, (long)XML_GetCurrentLineNumber(info->parser), s);
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

static void XMLCALL end_cdata_section_handler(void *data)
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

static void XMLCALL skipped_entity_handler(void *data, const char *entity_name, int is_parameter_entity)
{
    data = data;
    entity_name = entity_name;
    if (!is_parameter_entity)
    {
        /* We need to treat this as character data -> call the character data handler with some dummy
         * non-whitespace string
         */
        character_data_handler(data, "&entity;", 8);
    }
}

int coda_xml_parse(coda_xml_product *product)
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
    info.update_definition = (product->product_definition == NULL);
    if (info.update_definition)
    {
        coda_type_record *definition;

        definition = coda_type_record_new(coda_format_xml);
        if (definition == NULL)
        {
            XML_ParserFree(info.parser);
            return -1;
        }
        info.root = coda_xml_root_new(definition);
        coda_type_release((coda_type *)definition);
    }
    else
    {
        info.root = coda_xml_root_new((coda_type_record *)product->product_definition->root_type);
    }
    if (info.root == NULL)
    {
        XML_ParserFree(info.parser);
        return -1;
    }
    info.element = NULL;
    info.unparsed_depth = 0;

    XML_SetUserData(info.parser, &info);
    XML_SetParamEntityParsing(info.parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    XML_SetElementHandler(info.parser, start_element_handler, end_element_handler);
    XML_SetCharacterDataHandler(info.parser, character_data_handler);
    XML_SetCdataSectionHandler(info.parser, start_cdata_section_handler, end_cdata_section_handler);
    XML_SetSkippedEntityHandler(info.parser, skipped_entity_handler);
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
            coda_dynamic_type_delete((coda_dynamic_type *)info.root);
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
            coda_dynamic_type_delete((coda_dynamic_type *)info.root);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    XML_ParserFree(info.parser);

    if (info.update_definition)
    {
        if (coda_dynamic_type_update((coda_dynamic_type **)&info.root, (coda_type **)&info.root->definition) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)info.root);
            return -1;
        }
    }

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
