/*
 * Copyright (C) 2009-2011 S[&]T, The Netherlands.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qiap.h"

static void generate_xml_string(FILE *f, const char *str)
{
    long length;

    if (str == NULL)
    {
        return;
    }
    length = strlen(str);

    while (length > 0)
    {
        switch (*str)
        {
            case '&':
                fprintf(f, "&amp;");
                break;
            case '<':
                fprintf(f, "&lt;");
                break;
            case '>':
                fprintf(f, "&gt;");
                break;
            default:
                fprintf(f, "%c", *str);
                break;
        }
        str++;
        length--;
    }
}

static void write_algorithm(FILE *f, qiap_algorithm *algorithm)
{
    fprintf(f, "<qi:Algorithm name=\"");
    generate_xml_string(f, algorithm->name);
    fprintf(f, "\"");
    if (algorithm->reference != NULL)
    {
        fprintf(f, " reference=\"");
        generate_xml_string(f, algorithm->reference);
        fprintf(f, "\"");
    }
    if (algorithm->num_parameters > 0)
    {
        int i;

        fprintf(f, ">\n");
        for (i = 0; i < algorithm->num_parameters; i++)
        {
            fprintf(f, "<qi:Parameter key=\"");
            generate_xml_string(f, algorithm->parameter_key[i]);
            fprintf(f, "\">");
            generate_xml_string(f, algorithm->parameter_value[i]);
            fprintf(f, "</qi:Parameter>");
        }
        fprintf(f, "</qi:Algorithm>\n");
    }
    else
    {
        fprintf(f, "/>\n");
    }
}

static void write_action(FILE *f, qiap_action *action)
{
    fprintf(f, "<qi:Action last-modified=\"");
    generate_xml_string(f, action->last_modification_date);
    fprintf(f, "\" type=\"%s\"", qiap_get_action_type_name(action->action_type));
    if (action->order != 0)
    {
        fprintf(f, " order=\"%ld\"", action->order);
    }
    if (action->action_type == qiap_action_correct_value)
    {
        fprintf(f, ">");
        generate_xml_string(f, action->correction_string);
        fprintf(f, "</qi:Action>\n");
    }
    else if (action->action_type == qiap_action_custom_correction)
    {
        fprintf(f, ">\n");
        write_algorithm(f, action->algorithm);
        fprintf(f, "</qi:Action>\n");
    }
    else
    {
        fprintf(f, "/>\n");
    }
}

static void write_affected_value(FILE *f, qiap_affected_value *affected_value)
{
    int i;

    fprintf(f, "<qi:AffectedValues id=\"%ld\" parameter=\"", affected_value->affected_value_id);
    generate_xml_string(f, affected_value->parameter);
    fprintf(f, "\">\n");
    if (affected_value->extent_string != NULL)
    {
        fprintf(f, "<qi:Extent>");
        generate_xml_string(f, affected_value->extent_string);
        fprintf(f, "</qi:Extent>\n");
    }
    for (i = 0; i < affected_value->num_parameter_values; i++)
    {
        fprintf(f, "<qi:Value>");
        generate_xml_string(f, affected_value->parameter_value_path[i]);
        fprintf(f, "</qi:Value>\n");
    }
    for (i = 0; i < affected_value->num_actions; i++)
    {
        write_action(f, affected_value->action[i]);
    }
    fprintf(f, "</qi:AffectedValues>\n");
}

static void write_affected_product(FILE *f, qiap_affected_product *affected_product)
{
    int i;

    fprintf(f, "<qi:AffectedProducts id=\"%ld\" product_type=\"", affected_product->affected_product_id);
    generate_xml_string(f, affected_product->product_type);
    fprintf(f, "\">\n");
    if (affected_product->extent_string != NULL)
    {
        fprintf(f, "<qi:Extent>");
        generate_xml_string(f, affected_product->extent_string);
        fprintf(f, "</qi:Extent>\n");
    }
    for (i = 0; i < affected_product->num_products; i++)
    {
        fprintf(f, "<qi:Product name=\"");
        generate_xml_string(f, affected_product->product[i]);
        fprintf(f, "\"/>\n");
    }
    for (i = 0; i < affected_product->num_affected_values; i++)
    {
        write_affected_value(f, affected_product->affected_value[i]);
    }
    for (i = 0; i < affected_product->num_actions; i++)
    {
        write_action(f, affected_product->action[i]);
    }
    fprintf(f, "</qi:AffectedProducts>\n");
}

static void write_quality_issue(FILE *f, qiap_quality_issue *quality_issue)
{
    int i;

    fprintf(f, "<qi:QualityIssue id=\"%ld\" last-modified=\"", quality_issue->issue_id);
    generate_xml_string(f, quality_issue->last_modification_date);
    fprintf(f, "\" mission=\"");
    generate_xml_string(f, quality_issue->mission);
    fprintf(f, "\">\n");
    fprintf(f, "<qi:Title>");
    generate_xml_string(f, quality_issue->title);
    fprintf(f, "</qi:Title>\n");
    fprintf(f, "<qi:Description>");
    generate_xml_string(f, quality_issue->description);
    fprintf(f, "</qi:Description>\n");
    if (quality_issue->cause != NULL)
    {
        fprintf(f, "<qi:Cause>");
        generate_xml_string(f, quality_issue->cause);
        fprintf(f, "</qi:Cause>\n");
    }
    if (quality_issue->resolution != NULL)
    {
        fprintf(f, "<qi:Resolution>");
        generate_xml_string(f, quality_issue->resolution);
        fprintf(f, "</qi:Resolution>\n");
    }
    for (i = 0; i < quality_issue->num_affected_products; i++)
    {
        write_affected_product(f, quality_issue->affected_product[i]);
    }
    fprintf(f, "</qi:QualityIssue>\n");
}

LIBQIAP_API int qiap_write_report(const char *filename, const qiap_quality_issue_report *quality_issue_report)
{
    FILE *f = stdout;
    int i;

    if (filename != NULL)
    {
        f = fopen(filename, "w");
        if (f == NULL)
        {
            qiap_set_error(QIAP_ERROR_FILE_OPEN, "failed to open Quality Issue Report file '%s' for writing", filename);
            return -1;
        }
    }
    fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(f, "<qi:QualityIssueReport xmlns:qi=\"http://geca.esa.int/qiap/issue/2008/07\" organisation=\"");
    generate_xml_string(f, quality_issue_report->organisation);
    fprintf(f, "\">\n");
    for (i = 0; i < quality_issue_report->num_quality_issues; i++)
    {
        write_quality_issue(f, quality_issue_report->quality_issue[i]);
    }
    fprintf(f, "</qi:QualityIssueReport>\n");
    if (filename != NULL)
    {
        fclose(f);
    }

    return 0;
}
