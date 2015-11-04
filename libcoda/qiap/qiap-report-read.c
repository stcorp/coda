/*
 * Copyright (C) 2009-2013 S[&]T, The Netherlands.
 *
 * This file is part of the QIAP Toolkit.
 *
 * The QIAP Toolkit is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The QIAP Toolkit is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the QIAP Toolkit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "qiap.h"

#include "expat.h"
#include "hashtable.h"

typedef void (*child_parser_finished_handler) (void *parent_parser_info,
                                               qiap_quality_issue_report *quality_issue_report);

#define QUALITY_ISSUE_NAMESPACE "http://geca.esa.int/qiap/issue/2008/07"

typedef enum xml_element_tag_enum
{
    no_element = -1,
    element_Action,
    element_AffectedProducts,
    element_AffectedValues,
    element_Algorithm,
    element_Cause,
    element_Description,
    element_Extent,
    element_Instrument,
    element_Parameter,
    element_Product,
    element_QualityIssueReport,
    element_QualityIssue,
    element_Resolution,
    element_Title,
    element_Value,
} xml_element_tag;

static const char *xml_element_name[] = {
    QUALITY_ISSUE_NAMESPACE " Action",
    QUALITY_ISSUE_NAMESPACE " AffectedProducts",
    QUALITY_ISSUE_NAMESPACE " AffectedValues",
    QUALITY_ISSUE_NAMESPACE " Algorithm",
    QUALITY_ISSUE_NAMESPACE " Cause",
    QUALITY_ISSUE_NAMESPACE " Description",
    QUALITY_ISSUE_NAMESPACE " Extent",
    QUALITY_ISSUE_NAMESPACE " Instrument",
    QUALITY_ISSUE_NAMESPACE " Parameter",
    QUALITY_ISSUE_NAMESPACE " Product",
    QUALITY_ISSUE_NAMESPACE " QualityIssueReport",
    QUALITY_ISSUE_NAMESPACE " QualityIssue",
    QUALITY_ISSUE_NAMESPACE " Resolution",
    QUALITY_ISSUE_NAMESPACE " Title",
    QUALITY_ISSUE_NAMESPACE " Value",
};

#define num_xml_elements ((int)(sizeof(xml_element_name)/sizeof(char *)))


struct node_info_struct
{
    xml_element_tag tag;
    char *char_data;
    struct node_info_struct *parent;
};
typedef struct node_info_struct node_info;

struct parser_info_struct
{
    XML_Parser parser;
    node_info *node;
    hashtable *hash_data;
    int abort_parser;
    int unparsed_depth; /* keep track of how deep we are in the XML hierarchy within 'unparsed' XML elements */

    /* parent parser info */
    void *parent_parser_user_data;
    child_parser_finished_handler parser_finished;

    /* custom parameters */
    qiap_quality_issue_report *quality_issue_report;
    qiap_quality_issue *quality_issue;
    qiap_affected_product *affected_product;
    qiap_affected_value *affected_value;
    qiap_action *action;
    qiap_algorithm *algorithm;
    char *parameter_key;
};
typedef struct parser_info_struct parser_info;


static int parse_long(const char *buffer, long *dst)
{
    long length;
    int integer_length;
    long value;
    int negative = 0;

    length = strlen(buffer);

    while (length > 0 && *buffer == ' ')
    {
        buffer++;
        length--;
    }

    if (*buffer == '+' || *buffer == '-')
    {
        negative = (*buffer == '-');
        buffer++;
        length--;
    }

    value = 0;
    integer_length = 0;
    while (length > 0)
    {
        long digit;

        if (*buffer < '0' || *buffer > '9')
        {
            break;
        }
        digit = *buffer - '0';
        if (value > (LONG_MAX - digit) / 10)
        {
            qiap_set_error(QIAP_ERROR_XML, "integer value too large");
            return -1;
        }
        value = 10 * value + digit;
        integer_length++;
        buffer++;
        length--;
    }
    if (integer_length == 0)
    {
        qiap_set_error(QIAP_ERROR_XML, "invalid integer value (no digits)");
        return -1;
    }
    if (length != 0)
    {
        while (length > 0 && *buffer == ' ')
        {
            buffer++;
            length--;
        }
        if (length != 0)
        {
            qiap_set_error(QIAP_ERROR_XML, "invalid format for integer value");
            return -1;
        }
    }

    if (negative)
    {
        value = -value;
    }

    *dst = value;

    return 0;
}

static const char *get_attribute_value(const char **attr, const char *name)
{
    while (*attr != NULL)
    {
        if (strcmp(attr[0], name) == 0)
        {
            return attr[1];
        }
        attr = &attr[2];
    }
    return NULL;
}

static const char *get_mandatory_attribute_value(const char **attr, const char *name, xml_element_tag tag)
{
    const char *value;

    value = get_attribute_value(attr, name);
    if (value == NULL)
    {
        qiap_set_error(QIAP_ERROR_XML, "mandatory attribute '%s' missing for element '%s'", name,
                       xml_element_name[tag]);
    }
    return value;
}

static void abort_parser(parser_info *info)
{
    XML_StopParser(info->parser, 0);
    /* we need to explicitly check in the end handlers for parsing abort because expat may still call the end handler
     * after an abort in the start handler */
    info->abort_parser = 1;
}

static void XMLCALL string_handler(void *data, const char *s, int len)
{
    parser_info *info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (info->node->char_data == NULL)
    {
        info->node->char_data = (char *)malloc(len + 1);
        assert(info->node->char_data != NULL);
        memcpy(info->node->char_data, s, len);
        info->node->char_data[len] = '\0';
    }
    else
    {
        char *char_data;
        long current_length = strlen(info->node->char_data);

        char_data = (char *)malloc(current_length + len + 1);
        assert(char_data != NULL);
        memcpy(char_data, info->node->char_data, current_length);
        memcpy(&char_data[current_length], s, len);
        char_data[current_length + len] = '\0';
        free(info->node->char_data);
        info->node->char_data = char_data;
    }
}

static void push_node(parser_info *info, xml_element_tag tag)
{
    node_info *node;

    node = (node_info *)malloc(sizeof(node_info));
    assert(node != NULL);
    node->tag = tag;
    node->char_data = NULL;
    node->parent = info->node;
    info->node = node;
}

static void pop_node(parser_info *info)
{
    node_info *node = info->node;

    assert(node != NULL);
    if (node->char_data != NULL)
    {
        free(node->char_data);
    }
    info->node = node->parent;
    free(node);
}

static void XMLCALL start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info = (parser_info *)data;
    xml_element_tag tag;
    int has_char_data = 0;
    int allowed = 1;

    attr = attr;

    if (info->unparsed_depth > 0)
    {
        /* We are inside an unsupported element -> ignore this element */
        info->unparsed_depth++;
        return;
    }

    tag = (xml_element_tag)hashtable_get_index_from_name(info->hash_data, el);
    if (tag < 0)
    {
        qiap_set_error(QIAP_ERROR_XML, "element %s not allowed", el);
        abort_parser(info);
        return;
    }

    switch (tag)
    {
        case no_element:
            assert(0);
        case element_Action:
            allowed = (info->node->tag == element_AffectedProducts || info->node->tag == element_AffectedValues);
            if (allowed)
            {
                const char *last_modification_date;
                const char *action_type_string;
                const char *order_string;
                qiap_action_type action_type = -1;

                last_modification_date = get_mandatory_attribute_value(attr, "last-modified", info->node->tag);
                if (last_modification_date == NULL)
                {
                    abort_parser(info);
                    return;
                }
                action_type_string = get_mandatory_attribute_value(attr, "type", info->node->tag);
                if (action_type_string == NULL)
                {
                    abort_parser(info);
                    return;
                }
                if (strcmp(action_type_string, "discard product") == 0)
                {
                    action_type = qiap_action_discard_product;
                }
                else if (strcmp(action_type_string, "discard value") == 0)
                {
                    action_type = qiap_action_discard_value;
                }
                else if (strcmp(action_type_string, "correct value") == 0)
                {
                    action_type = qiap_action_correct_value;
                }
                else if (strcmp(action_type_string, "custom correction") == 0)
                {
                    action_type = qiap_action_custom_correction;
                }
                info->action = qiap_action_new(last_modification_date, action_type);
                if (info->action == NULL)
                {
                    abort_parser(info);
                    return;
                }
                order_string = get_attribute_value(attr, "order");
                if (order_string != NULL)
                {
                    long order;

                    if (parse_long(order_string, &order) != 0)
                    {
                        abort_parser(info);
                        return;
                    }
                    if (qiap_action_set_order(info->action, order) != 0)
                    {
                        abort_parser(info);
                        return;
                    }
                }

                has_char_data = (action_type == qiap_action_correct_value);
            }
            break;
        case element_AffectedProducts:
            allowed = (info->node->tag == element_QualityIssue);
            if (allowed)
            {
                const char *affected_product_id_string;
                const char *product_type;
                long affected_product_id;

                affected_product_id_string = get_mandatory_attribute_value(attr, "id", info->node->tag);
                if (affected_product_id_string == NULL)
                {
                    abort_parser(info);
                    return;
                }
                if (parse_long(affected_product_id_string, &affected_product_id) != 0)
                {
                    abort_parser(info);
                    return;
                }
                product_type = get_mandatory_attribute_value(attr, "product_type", info->node->tag);
                if (product_type == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->affected_product = qiap_affected_product_new(affected_product_id, product_type);
                if (info->affected_product == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_AffectedValues:
            allowed = (info->node->tag == element_AffectedProducts);
            if (allowed)
            {
                const char *affected_value_id_string;
                const char *parameter;
                long affected_value_id;

                affected_value_id_string = get_mandatory_attribute_value(attr, "id", info->node->tag);
                if (affected_value_id_string == NULL)
                {
                    abort_parser(info);
                    return;
                }
                if (parse_long(affected_value_id_string, &affected_value_id) != 0)
                {
                    abort_parser(info);
                    return;
                }
                parameter = get_mandatory_attribute_value(attr, "parameter", info->node->tag);
                if (parameter == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->affected_value = qiap_affected_value_new(affected_value_id, parameter);
                if (info->affected_value == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_Algorithm:
            allowed = (info->node->tag == element_Action && info->action->action_type == qiap_action_custom_correction);
            if (allowed)
            {
                const char *name;
                const char *reference;

                name = get_mandatory_attribute_value(attr, "name", info->node->tag);
                if (name == NULL)
                {
                    abort_parser(info);
                    return;
                }
                reference = get_attribute_value(attr, "reference");
                info->algorithm = qiap_algorithm_new(name, reference);
                if (info->algorithm == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_Cause:
            allowed = (info->node->tag == element_QualityIssue);
            has_char_data = 1;
            break;
        case element_Description:
            allowed = (info->node->tag == element_QualityIssue);
            has_char_data = 1;
            break;
        case element_Extent:
            allowed = (info->node->tag == element_AffectedProducts || info->node->tag == element_AffectedValues);
            has_char_data = 1;
            break;
        case element_Instrument:
            allowed = (info->node->tag == element_QualityIssue);
            has_char_data = 1;
            break;
        case element_Parameter:
            allowed = (info->node->tag == element_Algorithm);
            has_char_data = 1;
            if (allowed)
            {
                const char *key;

                key = get_mandatory_attribute_value(attr, "key", info->node->tag);
                if (key == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->parameter_key = strdup(key);
                if (info->parameter_key == NULL)
                {
                    qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_Product:
            allowed = (info->node->tag == element_AffectedProducts);
            if (allowed)
            {
                const char *name;

                name = get_mandatory_attribute_value(attr, "name", info->node->tag);
                if (name == NULL)
                {
                    abort_parser(info);
                    return;
                }
                if (qiap_affected_product_add_product(info->affected_product, name) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_QualityIssueReport:
            allowed = (info->node->tag == no_element);
            if (allowed)
            {
                const char *organisation;

                organisation = get_mandatory_attribute_value(attr, "organisation", info->node->tag);
                if (organisation == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->quality_issue_report = qiap_quality_issue_report_new(organisation);
                if (info->quality_issue_report == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_QualityIssue:
            allowed = (info->node->tag == element_QualityIssueReport);
            if (allowed)
            {
                const char *issue_id_string;
                const char *last_modification_date;
                const char *mission;
                long issue_id;

                issue_id_string = get_mandatory_attribute_value(attr, "id", info->node->tag);
                if (issue_id_string == NULL)
                {
                    abort_parser(info);
                    return;
                }
                if (parse_long(issue_id_string, &issue_id) != 0)
                {
                    abort_parser(info);
                    return;
                }
                last_modification_date = get_mandatory_attribute_value(attr, "last-modified", info->node->tag);
                if (last_modification_date == NULL)
                {
                    abort_parser(info);
                    return;
                }
                mission = get_mandatory_attribute_value(attr, "mission", info->node->tag);
                if (mission == NULL)
                {
                    abort_parser(info);
                    return;
                }
                info->quality_issue = qiap_quality_issue_new(issue_id, last_modification_date, mission);
                if (info->quality_issue == NULL)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_Resolution:
            allowed = (info->node->tag == element_QualityIssue);
            has_char_data = 1;
            break;
        case element_Title:
            allowed = (info->node->tag == element_QualityIssue);
            has_char_data = 1;
            break;
        case element_Value:
            allowed = (info->node->tag == element_AffectedValues);
            has_char_data = 1;
            break;
    }

    if (!allowed)
    {
        if (info->node->tag == no_element)
        {
            qiap_set_error(QIAP_ERROR_XML, "element %s not allowed as root", xml_element_name[tag]);
        }
        else
        {
            qiap_set_error(QIAP_ERROR_XML, "element %s not allowed as child of %s", xml_element_name[tag],
                           xml_element_name[info->node->tag]);
        }
        abort_parser(info);
        return;
    }

    push_node(info, tag);

    if (has_char_data)
    {
        XML_SetCharacterDataHandler(info->parser, string_handler);
    }
}

static void XMLCALL end_element_handler(void *data, const char *el)
{
    parser_info *info = (parser_info *)data;

    el = el;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth--;
        return;
    }

    switch (info->node->tag)
    {
        case no_element:
            assert(0);
        case element_Action:
            if (info->action->action_type == qiap_action_correct_value)
            {
                if (qiap_action_set_correction(info->action,
                                               info->node->char_data == NULL ? "" : info->node->char_data) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            if (info->affected_value != NULL)
            {
                if (qiap_affected_value_add_action(info->affected_value, info->action) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            else
            {
                if (qiap_affected_product_add_action(info->affected_product, info->action) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            info->action = NULL;
            break;
        case element_AffectedProducts:
            if (qiap_quality_issue_add_affected_product(info->quality_issue, info->affected_product) != 0)
            {
                abort_parser(info);
                return;
            }
            info->affected_product = NULL;
            break;
        case element_AffectedValues:
            if (qiap_affected_product_add_affected_value(info->affected_product, info->affected_value) != 0)
            {
                abort_parser(info);
                return;
            }
            info->affected_value = NULL;
            break;
        case element_Algorithm:
            if (qiap_action_set_algorithm(info->action, info->algorithm) != 0)
            {
                abort_parser(info);
                return;
            }
            info->algorithm = NULL;
            break;
        case element_Cause:
            if (qiap_quality_issue_set_cause(info->quality_issue,
                                             info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
        case element_Description:
            if (qiap_quality_issue_set_description(info->quality_issue,
                                                   info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
        case element_Extent:
            if (info->affected_value != NULL)
            {
                if (qiap_affected_value_set_extent(info->affected_value,
                                                   info->node->char_data == NULL ? "" : info->node->char_data) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            else
            {
                if (qiap_affected_product_set_extent(info->affected_product,
                                                     info->node->char_data == NULL ? "" : info->node->char_data) != 0)
                {
                    abort_parser(info);
                    return;
                }
            }
            break;
        case element_Instrument:
            if (qiap_quality_issue_set_instrument(info->quality_issue,
                                                  info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
        case element_Parameter:
            if (qiap_algorithm_add_parameter(info->algorithm, info->parameter_key,
                                             info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            free(info->parameter_key);
            info->parameter_key = NULL;
            break;
        case element_Product:
            break;
        case element_QualityIssueReport:
            if (info->parser_finished != NULL)
            {
                /* parent parser is responsible for calling qiap_report_parser_info_delete() */
                qiap_quality_issue_report *quality_issue_report = info->quality_issue_report;

                info->quality_issue_report = NULL;
                info->parser_finished(info->parent_parser_user_data, quality_issue_report);
                return;
            }
            break;
        case element_QualityIssue:
            if (info->quality_issue->title == NULL)
            {
                qiap_set_error(QIAP_ERROR_XML, "mandatory element Title missing for QualityIssue");
                abort_parser(info);
                return;
            }
            if (info->quality_issue->description == NULL)
            {
                qiap_set_error(QIAP_ERROR_XML, "mandatory element Description missing for QualityIssue");
                abort_parser(info);
                return;
            }
            if (qiap_quality_issue_report_add_quality_issue(info->quality_issue_report, info->quality_issue) != 0)
            {
                abort_parser(info);
                return;
            }
            info->quality_issue = NULL;
            break;
        case element_Resolution:
            if (qiap_quality_issue_set_resolution(info->quality_issue,
                                                  info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
        case element_Title:
            if (qiap_quality_issue_set_title(info->quality_issue,
                                             info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
        case element_Value:
            if (qiap_affected_value_add_value(info->affected_value,
                                              info->node->char_data == NULL ? "" : info->node->char_data) != 0)
            {
                abort_parser(info);
                return;
            }
            break;
    }

    pop_node(info);

    XML_SetCharacterDataHandler(info->parser, NULL);
}

static void parser_info_init(parser_info **user_info, XML_Parser parser)
{
    parser_info *info;
    int i;

    info = malloc(sizeof(parser_info));
    assert(info != NULL);

    info->node = NULL;
    info->parser = parser;
    info->hash_data = NULL;
    info->abort_parser = 0;
    info->unparsed_depth = 0;

    info->parent_parser_user_data = NULL;
    info->parser_finished = NULL;

    info->quality_issue_report = NULL;
    info->quality_issue = NULL;
    info->affected_product = NULL;
    info->affected_value = NULL;
    info->action = NULL;
    info->algorithm = NULL;
    info->parameter_key = NULL;

    info->hash_data = hashtable_new(1);
    assert(info->hash_data != NULL);
    for (i = 0; i < num_xml_elements; i++)
    {
        if (hashtable_add_name(info->hash_data, xml_element_name[i]) != 0)
        {
            assert(0);
        }
    }

    push_node(info, no_element);

    *user_info = info;
}

void qiap_report_parser_info_delete(parser_info *info)
{
    while (info->node != NULL)
    {
        pop_node(info);
    }
    if (info->hash_data != NULL)
    {
        hashtable_delete(info->hash_data);
    }
    if (info->quality_issue_report != NULL)
    {
        qiap_quality_issue_report_delete(info->quality_issue_report);
    }
    if (info->quality_issue != NULL)
    {
        qiap_quality_issue_delete(info->quality_issue);
    }
    if (info->affected_product != NULL)
    {
        qiap_affected_product_delete(info->affected_product);
    }
    if (info->affected_value != NULL)
    {
        qiap_affected_value_delete(info->affected_value);
    }
    if (info->action != NULL)
    {
        qiap_action_delete(info->action);
    }
    if (info->algorithm != NULL)
    {
        qiap_algorithm_delete(info->algorithm);
    }
    if (info->parameter_key != NULL)
    {
        free(info->parameter_key);
    }
    free(info);
}

void qiap_report_init_parser(XML_Parser parser, void *parent_parser_user_data,
                             child_parser_finished_handler finished_handler, parser_info **user_info)
{
    parser_info *info;

    parser_info_init(&info, parser);
    info->parent_parser_user_data = parent_parser_user_data;
    info->parser_finished = finished_handler;

    XML_SetUserData(info->parser, info);
    XML_SetElementHandler(info->parser, start_element_handler, end_element_handler);

    *user_info = info;
}

LIBQIAP_API int qiap_read_report(const char *filename, qiap_quality_issue_report **quality_issue_report)
{
    XML_Parser parser;
    char buffer[4096];
    parser_info *info;
    int fd;

    parser = XML_ParserCreateNS(NULL, ' ');
    assert(parser != NULL);

    qiap_report_init_parser(parser, NULL, NULL, &info);

    if ((fd = open(filename, O_RDONLY)) == -1)
    {
        qiap_set_error(QIAP_ERROR_FILE_OPEN, "failed to open Quality Issue Report file '%s'", filename);
        qiap_report_parser_info_delete(info);
        XML_ParserFree(parser);
        return -1;
    }

    for (;;)
    {
        int length;

        length = read(fd, buffer, sizeof(buffer));
        if (length < 0)
        {
            qiap_set_error(QIAP_ERROR_FILE_READ, "could not read data from Quality Issue Report file");
            close(fd);
            qiap_report_parser_info_delete(info);
            XML_ParserFree(parser);
            return -1;
        }

        qiap_errno = 0;
        if (XML_Parse(parser, buffer, length, (length == 0)) != XML_STATUS_OK)
        {
            if (qiap_errno == 0)
            {
                qiap_set_error(QIAP_ERROR_XML, "parse error (%s)", XML_ErrorString(XML_GetErrorCode(parser)));
            }
            qiap_add_error_message(" at line %ld in Quality Issue Report file", (long)XML_GetCurrentLineNumber(parser));
            close(fd);
            qiap_report_parser_info_delete(info);
            XML_ParserFree(parser);
            return -1;
        }

        if (length == 0)
        {
            /* end of file */
            break;
        }
    }

    *quality_issue_report = info->quality_issue_report;
    info->quality_issue_report = NULL;

    close(fd);
    qiap_report_parser_info_delete(info);
    XML_ParserFree(parser);

    return 0;
}
