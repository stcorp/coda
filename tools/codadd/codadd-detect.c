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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-definition.h"
#include "coda-ascbin-internal.h"
#include "coda-xml-internal.h"

static int INDENT = 0;

static void indent(void)
{
    int i;

    assert(INDENT >= 0);
    for (i = INDENT; i; i--)
    {
        printf("  ");
    }
}

static void generate_escaped_string(const char *str, long length)
{
    int i = 0;

    if (length == 0 || str == NULL)
    {
        return;
    }

    if (length < 0)
    {
        length = strlen(str);
    }

    while (i < length)
    {
        switch (str[i])
        {
            case '\033':       /* windows does not recognize '\e' */
                printf("\\e");
                break;
            case '\a':
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\v':
                printf("\\v");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '"':
                printf("\\\"");
                break;
            default:
                if (!isprint(str[i]))
                {
                    printf("\\%03o", (int)(unsigned char)str[i]);
                }
                else
                {
                    printf("%c", str[i]);
                }
                break;
        }
        i++;
    }
}

static void generate_detection_rule_entry(coda_detection_rule_entry *entry)
{
    if (entry->use_filename)
    {
        printf("filename[%ld:%ld] == \"", (long)entry->offset, (long)(entry->offset + entry->value_length - 1));
        generate_escaped_string(entry->value, entry->value_length);
        printf("\"");
    }
    else if (entry->path != NULL)
    {
        if (entry->value != NULL)
        {
            printf("data == \"");
            generate_escaped_string(entry->value, entry->value_length);
            printf("\"");
        }
        else
        {
            printf("exists(%s)", entry->path);
        }
    }
    else if (entry->value_length > 0)
    {
        if (entry->offset >= 0)
        {
            printf("data[%ld:%ld] == ", (long)entry->offset, (long)(entry->offset + entry->value_length - 1));
            printf("\"");
            generate_escaped_string(entry->value, entry->value_length);
            printf("\"");
        }
        else
        {
            printf("data contains \"");
            generate_escaped_string(entry->value, entry->value_length);
            printf("\"");
        }
    }
    else if (entry->offset >= 0)
    {
        printf("filesize == %ld", (long)entry->offset);
    }
}

static void generate_ascbin_detection_tree(coda_ascbin_detection_node *node, int num_compares)
{
    int i;

    if (node == NULL)
    {
        return;
    }

    if (node->entry != NULL)
    {
        num_compares++;
        indent();
        generate_detection_rule_entry(node->entry);
        if (node->rule)
        {
            coda_product_definition *product_definition = node->rule->product_definition;

            printf(" --> %s %s %d", product_definition->product_type->product_class->name,
                   product_definition->product_type->name, product_definition->version);
            if (node->num_subnodes > 0)
            {
                printf(" {%d+%d tests}", num_compares, node->num_subnodes);
            }
            else
            {
                printf(" {%d tests}", num_compares);
            }
        }
        printf("\n");
    }

    for (i = 0; i < node->num_subnodes; i++)
    {
        INDENT++;
        generate_ascbin_detection_tree(node->subnode[i], num_compares + i);
        INDENT--;
    }

}

static void print_xml_name(const char *xml_name)
{
    char *name;

    /* find the element name within "<namespace> <element_name>"
     * The namespace (and separation character) are optional
     */
    name = (char *)xml_name;
    while (*name != ' ' && *name != '\0')
    {
        name++;
    }
    if (*name == '\0')
    {
        /* the name did not contain a namespace */
        printf("%s", xml_name);
    }
    else
    {
        char *namespace;

        /* print with namespace */
        namespace = strdup(xml_name);
        namespace[name - xml_name - 1] = '\0';
        name++;
        printf("{%s}%s", namespace, name);
        free(namespace);
    }
}

static void generate_xml_detection_tree(coda_xml_detection_node *node, int is_attribute)
{
    int i;

    if (node == NULL)
    {
        return;
    }

    if (node->xml_name != NULL)
    {
        indent();
        printf(is_attribute ? "@" : "/");
        print_xml_name(node->xml_name);
        printf("\n");
        INDENT++;
    }

    if (!is_attribute)
    {
        for (i = 0; i < node->num_attribute_subnodes; i++)
        {
            generate_xml_detection_tree(node->attribute_subnode[i], 1);
        }

        for (i = 0; i < node->num_subnodes; i++)
        {
            generate_xml_detection_tree(node->subnode[i], 0);
        }
    }

    for (i = 0; i < node->num_detection_rules; i++)
    {
        coda_detection_rule *detection_rule = node->detection_rule[i];
        coda_product_definition *product_definition = detection_rule->product_definition;
        int j;

        indent();
        if (detection_rule->entry[0]->value_length > 0)
        {
            generate_detection_rule_entry(detection_rule->entry[0]);
        }
        else
        {
            printf("exists");
        }
        for (j = 1; j < detection_rule->num_entries; j++)
        {
            printf("\n");
            INDENT++;
            indent();
            generate_detection_rule_entry(detection_rule->entry[j]);
        }
        printf(" --> %s %s %d", product_definition->product_type->product_class->name,
               product_definition->product_type->name, product_definition->version);
        printf("\n");
        for (j = 1; j < detection_rule->num_entries; j++)
        {
            INDENT--;
        }
    }

    if (node->xml_name != NULL)
    {
        INDENT--;
    }
}

void generate_detection_tree(coda_format format)
{
    switch (format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            generate_ascbin_detection_tree(coda_ascbin_get_detection_tree(), 0);
            break;
        case coda_format_xml:
            generate_xml_detection_tree(coda_xml_get_detection_tree(), 0);
            break;
        default:
            break;
    }
}
