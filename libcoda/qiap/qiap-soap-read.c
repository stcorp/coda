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

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "qiap.h"

#include "expat.h"
#include "hashtable.h"

typedef void (*child_parser_finished_handler) (void *parent_parser_info,
                                               qiap_quality_issue_report *quality_issue_report);

#define MAX_IDLE_TIME 7

#define SOAP_NAMESPACE "http://www.w3.org/2003/05/soap-envelope"
#define QUALITY_ISSUE_NAMESPACE "http://geca.esa.int/qiap/issue/2008/07"
#define XML_NAMESPACE "http://www.w3.org/XML/1998/namespace"

typedef enum xml_element_tag_enum
{
    no_element = -1,
    element_Body,
    element_Code,
    element_Detail,
    element_Envelope,
    element_Fault,
    element_Header,
    element_Reason,
    element_Role,
    element_Text
} xml_element_tag;

static const char *xml_element_name[] = {
    SOAP_NAMESPACE " Body",
    SOAP_NAMESPACE " Code",
    SOAP_NAMESPACE " Detail",
    SOAP_NAMESPACE " Envelope",
    SOAP_NAMESPACE " Fault",
    SOAP_NAMESPACE " Header",
    SOAP_NAMESPACE " Reason",
    SOAP_NAMESPACE " Role",
    SOAP_NAMESPACE " Text",
};

const char *xml_lang_attribute_name = XML_NAMESPACE " lang";

#define num_xml_elements ((int)(sizeof(xml_element_name)/sizeof(char *)))

typedef struct read_status_struct
{
    int fd;
    char *buffer;
    long size;  /* memory size of the buffer */
    long num_read;      /* number of bytes in the buffer filled with data from server */
    long line_length;   /* length of the first line in the buffer (incl. CRLF ) */
} read_status;

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

    /* custom parameters */
    int expect_error;
    void *qiap_report_parser_info;
    qiap_quality_issue_report *quality_issue_report;
    char *faultstring;
};
typedef struct parser_info_struct parser_info;

void qiap_report_init_parser(XML_Parser parser, void *parent_parser_user_data,
                             child_parser_finished_handler finished_handler, void **user_info);
void qiap_report_parser_info_delete(void *user_info);

static void report_parser_finished_handler(void *parent_parser_info, qiap_quality_issue_report *quality_issue_report);

static int read_status_init(read_status *status, int fd)
{
    status->fd = fd;
    status->buffer = malloc(2048);      /* init with twice the size of our maximum read block */
    if (status->buffer == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate 1024 bytes) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    status->size = 2048;
    status->num_read = 0;
    status->line_length = 0;

    return 0;
}

static void read_status_done(read_status *status)
{
    free(status->buffer);
}

static long read_data(int fd, void *buf, size_t count)
{
    long length;
    int repeat = 1;
    time_t t1 = time(NULL);

    while (repeat)
    {
        length = recv(fd, buf, count, 0);
        repeat = 0;
        if (length == -1)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                time_t t2 = time(NULL);

                if (t2 - t1 > MAX_IDLE_TIME)
                {
                    qiap_set_error(QIAP_ERROR_SERVER, "connection timeout");
                    return -1;
                }

                /* sleep for 1 millisecond, after which we try again */
#ifdef WIN32
                Sleep(1);
#else
                usleep(1);
#endif
                repeat = 1;
            }
            else
            {
                qiap_set_error(QIAP_ERROR_SERVER, "error receiving data from server - %s", strerror(errno));
                return -1;
            }
        }
        if (qiap_option_debug && length > 0)
        {
            fwrite(buf, length, 1, stdout);
        }
    }

    return length;
}

static void skip_line(read_status *status)
{
    if (status->line_length > 0)
    {
        if (status->num_read - status->line_length > 0)
        {
            memmove(status->buffer, &status->buffer[status->line_length], status->num_read - status->line_length);
        }
        status->num_read -= status->line_length;
        status->line_length = 0;
    }
}

static int read_line(read_status *status, const char **line)
{
    /* skip previous line */
    skip_line(status);

    for (;;)
    {
        long length;
        long p;

        for (p = 0; p < status->num_read - 2; p++)
        {
            if (status->buffer[p] == '\r' && status->buffer[p + 1] == '\n')
            {
                /* found end of line */
                status->line_length = p + 2;
                status->buffer[p] = '\0';
                *line = status->buffer;
                return 0;
            }
        }
        /* not found -> read more bytes */
        if (status->num_read + 1024 > status->size)
        {
            char *new_buffer;

            /* increase buffer size */
            new_buffer = realloc(status->buffer, status->size + 1024);
            if (new_buffer == NULL)
            {
                qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                               status->size + 1024, __FILE__, __LINE__);
                return -1;
            }
            status->buffer = new_buffer;
            status->size += 1024;
        }
        length = read_data(status->fd, &status->buffer[status->num_read], 1024);
        if (length < 0)
        {
            break;
        }
        if (length == 0)
        {
            qiap_set_error(QIAP_ERROR_SERVER, "error receiving data from server - unexpected end of data");
            break;
        }
        status->num_read += length;
    }

    return -1;
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
    int ignore = 0;

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
        case element_Body:
            allowed = (info->node->tag == element_Envelope);
            if (!info->expect_error)
            {
                qiap_report_init_parser(info->parser, info, report_parser_finished_handler,
                                        &info->qiap_report_parser_info);
            }
            break;
        case element_Code:
            allowed = (info->node->tag == element_Fault);
            ignore = 1;
            break;
        case element_Detail:
            allowed = (info->node->tag == element_Fault);
            ignore = 1;
            break;
        case element_Envelope:
            allowed = (info->node->tag == no_element);
            break;
        case element_Fault:
            allowed = (info->node->tag == element_Body && info->expect_error);
            break;
        case element_Header:
            allowed = (info->node->tag == element_Envelope);
            ignore = 1;
            break;
        case element_Reason:
            allowed = (info->node->tag == element_Fault);
            break;
        case element_Role:
            allowed = (info->node->tag == element_Fault);
            ignore = 1;
            break;
        case element_Text:
            allowed = (info->node->tag == element_Reason);
            if (allowed)
            {
                const char *lang;

                lang = get_mandatory_attribute_value(attr, xml_lang_attribute_name, info->node->tag);
                if (lang == NULL)
                {
                    abort_parser(info);
                    return;
                }
                /* we only support english error messages */
                if (strcmp(lang, "en") == 0)
                {
                    has_char_data = 1;
                }
                else
                {
                    ignore = 1;
                }
            }
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

    if (ignore)
    {
        info->unparsed_depth = 1;
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
        case element_Code:
        case element_Detail:
        case element_Header:
        case element_Role:
            assert(0);
        case element_Body:
        case element_Envelope:
        case element_Fault:
        case element_Reason:
            break;
        case element_Text:
            info->faultstring = strdup(info->node->char_data);
            if (info->faultstring == NULL)
            {
                qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                               __FILE__, __LINE__);
                abort_parser(info);
                return;
            }
            break;
    }

    pop_node(info);

    XML_SetCharacterDataHandler(info->parser, NULL);
}

static void report_parser_finished_handler(void *parent_parser_info, qiap_quality_issue_report *quality_issue_report)
{
    parser_info *info = (parser_info *)parent_parser_info;

    info->quality_issue_report = quality_issue_report;

    qiap_report_parser_info_delete(info->qiap_report_parser_info);
    info->qiap_report_parser_info = NULL;

    XML_SetUserData(info->parser, &info);
    XML_SetElementHandler(info->parser, start_element_handler, end_element_handler);
    XML_SetCharacterDataHandler(info->parser, NULL);
}

static void parser_info_init(parser_info *info)
{
    int i;

    info->node = NULL;
    info->parser = NULL;
    info->hash_data = NULL;
    info->abort_parser = 0;
    info->unparsed_depth = 0;

    info->qiap_report_parser_info = NULL;
    info->quality_issue_report = NULL;
    info->faultstring = NULL;

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
}

static void parser_info_delete(parser_info *info)
{
    while (info->node != NULL)
    {
        pop_node(info);
    }
    if (info->parser != NULL)
    {
        XML_ParserFree(info->parser);
    }
    if (info->hash_data != NULL)
    {
        hashtable_delete(info->hash_data);
    }
    if (info->qiap_report_parser_info != NULL)
    {
        qiap_report_parser_info_delete(info->qiap_report_parser_info);
    }
    if (info->quality_issue_report != NULL)
    {
        qiap_report_parser_info_delete(info->quality_issue_report);
    }
    if (info->faultstring != NULL)
    {
        free(info->faultstring);
    }
}

static int parse_http_header(read_status *status, int *http_response_code)
{
    const char *line;

    /* status line */
    if (read_line(status, &line) != 0)
    {
        return -1;
    }
    if (strncmp(line, "HTTP/", 5) != 0)
    {
        qiap_set_error(QIAP_ERROR_SERVER, "invalid response from server (invalid HTTP response)");
        return -1;
    }
    while (*line != ' ' && *line != '\0')
    {
        line++;
    }
    if (sscanf(line, "%d", http_response_code) != 1)
    {
        qiap_set_error(QIAP_ERROR_SERVER, "invalid response from server (invalid HTTP response)");
        return -1;
    }
    if (*http_response_code != 200 && *http_response_code != 500)
    {
        qiap_set_error(QIAP_ERROR_SERVER, "server returned HTTP error: %s", line);
        return -1;
    }

    /* header */
    for (;;)
    {
        const char *line;

        if (read_line(status, &line) != 0)
        {
            return -1;
        }
        if (line[0] == '\0')
        {
            /* empty line finishes header section */
            break;
        }
    }
    /* skip last line */
    skip_line(status);

    return 0;
}

int qiap_handle_soap_response(int fd, qiap_quality_issue_report **quality_issue_report)
{
    read_status status;
    int http_response_code;
    parser_info info;
    int length;

    /* receive result */
    if (read_status_init(&status, fd) != 0)
    {
        return -1;
    }

    if (qiap_option_debug)
    {
        printf("------------ RESPONSE -------------\n");
    }

    if (parse_http_header(&status, &http_response_code) != 0)
    {
        read_status_done(&status);
        return -1;
    }

    parser_info_init(&info);

    info.parser = XML_ParserCreateNS(NULL, ' ');
    assert(info.parser != NULL);
    info.expect_error = (http_response_code == 500);

    XML_SetUserData(info.parser, &info);
    XML_SetElementHandler(info.parser, start_element_handler, end_element_handler);

    length = status.num_read;
    if (length == 0)
    {
        length = read_data(fd, status.buffer, 1024);
    }
    while (length != 0)
    {
        if (length < 0)
        {
            parser_info_delete(&info);
            read_status_done(&status);
            return -1;
        }

        qiap_errno = 0;
        if (XML_Parse(info.parser, status.buffer, length, (length == 0)) != XML_STATUS_OK)
        {
            if (qiap_errno == 0)
            {
                qiap_set_error(QIAP_ERROR_XML, "parse error (%s)", XML_ErrorString(XML_GetErrorCode(info.parser)));
            }
            qiap_add_error_message(" at line %ld in server response message",
                                   (long)XML_GetCurrentLineNumber(info.parser));
            parser_info_delete(&info);
            read_status_done(&status);
            return -1;
        }

        length = read_data(fd, status.buffer, 1024);
    }

    if (info.expect_error)
    {
        if (info.faultstring == NULL)
        {
            qiap_set_error(QIAP_ERROR_SERVER,
                           "invalid response from server (english error description missing for error condition)");
        }
        else
        {
            qiap_set_error(QIAP_ERROR_SERVER, "server returned error message (%s)", info.faultstring);
        }
        parser_info_delete(&info);
        read_status_done(&status);
        return -1;
    }

    *quality_issue_report = info.quality_issue_report;
    info.quality_issue_report = NULL;
    parser_info_delete(&info);
    read_status_done(&status);

    return 0;
}
