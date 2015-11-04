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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-path.h"
#include "coda-internal.h"
#include "coda-expr-internal.h"
#include "coda-ascbin-internal.h"
#include "coda-ascii-internal.h"
#include "coda-bin-internal.h"
#include "coda-xml-internal.h"
#include "coda-definition.h"

#ifdef __GNUC__
static int ff_printf(const char *templ, ...) __attribute__ ((format(printf, 1, 2)));
static int fi_printf(const char *templ, ...) __attribute__ ((format(printf, 1, 2)));
#else
static int ff_printf(const char *templ, ...);
static int fi_printf(const char *templ, ...);
#endif

#define SPLIT_SIZE 80
#define MAX_IDENTIFIER_LENGTH 45

int INDENT = 0;
int DEPTH = 0;
FILE *FFILE;

const char *product_variable_name = NULL;

/* indenting */

static void indent(void)
{
    int i;

    assert(INDENT >= 0);
    for (i = INDENT; i; i--)
    {
        fprintf(FFILE, "  ");
    }
}

static int ff_printf(const char *templ, ...)
{
    int result;
    va_list ap;

    va_start(ap, templ);
    result = vfprintf(FFILE, templ, ap);
    va_end(ap);

    return result;
}

static int fi_printf(const char *templ, ...)
{
    int result;
    va_list ap;

    indent();

    va_start(ap, templ);
    result = vfprintf(FFILE, templ, ap);
    va_end(ap);

    return result;
}

void element_name_and_namespace_from_xml_name(const char *xml_name, char **element_name, char **namespace)
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
        *element_name = strdup(xml_name);
        *namespace = NULL;
    }
    else
    {
        /* skip space */
        name++;
        *element_name = strdup(name);
        *namespace = strdup(xml_name);
        (*namespace)[name - xml_name - 1] = '\0';
    }
}

static void generate_escaped_html_string(const char *str, int length)
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
            case '\033':   /* windows does not recognize '\e' */
                ff_printf("\\e");
                break;
            case '\a':
                ff_printf("\\a");
                break;
            case '\b':
                ff_printf("\\b");
                break;
            case '\f':
                ff_printf("\\f");
                break;
            case '\n':
                ff_printf("\\n");
                break;
            case '\r':
                ff_printf("\\r");
                break;
            case '\t':
                ff_printf("\\t");
                break;
            case '\v':
                ff_printf("\\v");
                break;
            case '\\':
                ff_printf("\\\\");
                break;
            case '"':
                ff_printf("\\\"");
                break;
            case '<':
                ff_printf("&lt;");
                break;
            case '>':
                ff_printf("&gt;");
                break;
            case '&':
                ff_printf("&amp;");
                break;
            case ' ':
                ff_printf("&nbsp;");
                break;
            default:
                if (!isprint(str[i]))
                {
                    ff_printf("\\%03o", (int)(unsigned char)str[i]);
                }
                else
                {
                    ff_printf("%c", str[i]);
                }
                break;
        }
        i++;
    }
}

static void generate_xml_string(const char *str)
{
    if (str == NULL)
    {
        return;
    }
    while (*str != '\0')
    {
        switch (*str)
        {
            case '&':
                ff_printf("&amp;");
                break;
            case '<':
                ff_printf("&lt;");
                break;
            case '>':
                ff_printf("&gt;");
                break;
            default:
                ff_printf("%c", *str);
                break;
        }
        str++;
    }
}

static void generate_html_expr(const coda_Expr *expr, int precedence);
static void generate_html_type(const coda_Type *type, int expand_named_type, int full_width);

static void html_attr_begin(const char *key_name, int *first_attribute)
{
    if (!*first_attribute)
    {
        fi_printf("<br /><br />\n");
    }
    else
    {
        *first_attribute = 0;
    }
    fi_printf("<span class=\"attr_key\">%s</span><span class=\"attr_value\">: ", key_name);
}

static void html_attr_end(void)
{
    ff_printf("</span>\n");
}

static void generate_html_ascbin_attributes(const coda_ascbinType *type, int first_attribute)
{
    int i;

    switch (type->tag)
    {
        case tag_ascbin_record:
            if (((coda_ascbinRecord *)type)->fast_size_expr != NULL)
            {
                html_attr_begin("fast&nbsp;size&nbsp;expr", &first_attribute);
                generate_html_expr(((coda_ascbinRecord *)type)->fast_size_expr, 10);
                html_attr_end();
            }
            break;
        case tag_ascbin_union:
            html_attr_begin("field&nbsp;expr", &first_attribute);
            generate_html_expr(((coda_ascbinUnion *)type)->field_expr, 10);
            html_attr_end();
            if (((coda_ascbinUnion *)type)->fast_size_expr != NULL)
            {
                html_attr_begin("fast&nbsp;size&nbsp;expr", &first_attribute);
                generate_html_expr(((coda_ascbinUnion *)type)->fast_size_expr, 10);
                html_attr_end();
            }
            break;
        case tag_ascbin_array:
            for (i = 0; i < ((coda_ascbinArray *)type)->num_dims; i++)
            {
                if (((coda_ascbinArray *)type)->dim_expr[i] != NULL)
                {
                    char dimstr[10];
                    
                    sprintf(dimstr, "dim_%d", i);
                    html_attr_begin(dimstr, &first_attribute);
                    generate_html_expr(((coda_ascbinArray *)type)->dim_expr[i], 10);
                    html_attr_end();
                }
            }
            break;
    }
}

static void generate_html_ascii_attributes(const coda_asciiType *type, int first_attribute)
{
    int i;

    switch (type->tag)
    {
        case tag_bin_record:
        case tag_bin_union:
        case tag_bin_array:
            generate_html_ascbin_attributes((coda_ascbinType *)type, first_attribute);
            break;
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_asciiNumber *number;
                
                number = (coda_asciiNumber *)type;
                if (number->unit != NULL)
                {
                    html_attr_begin("unit", &first_attribute);
                    ff_printf("\"%s\"", number->unit);
                    html_attr_end();
                }
                if (number->conversion != NULL)
                {
                    html_attr_begin("converted&nbsp;unit", &first_attribute);
                    ff_printf("\"%s\" (multiply by %g/%g)",
                              (number->conversion->unit == NULL ? "" : number->conversion->unit),
                              number->conversion->numerator, number->conversion->denominator);
                    html_attr_end();
                }
                if (number->mappings != NULL)
                {
                    coda_asciiMappings *mappings = number->mappings;
                    for (i = 0; i < mappings->num_mappings; i++)
                    {
                        html_attr_begin("mapping", &first_attribute);
                        ff_printf("\"");
                        generate_escaped_html_string(mappings->mapping[i]->str, mappings->mapping[i]->length);
                        ff_printf("\"&nbsp;-&gt;&nbsp;");
                        if (type->type_class == coda_integer_class)
                        {
                            ff_printf("%lld", ((coda_asciiIntegerMapping *)mappings->mapping[i])->value);
                        }
                        else
                        {
                            ff_printf("%f", ((coda_asciiFloatMapping *)mappings->mapping[i])->value);
                        }
                        html_attr_end();                  
                    }
                }
            }
            break;
        case tag_ascii_text:
            if (((coda_asciiText *)type)->byte_size_expr != NULL)
            {
                html_attr_begin("byte&nbsp;size", &first_attribute);
                generate_html_expr(((coda_asciiText *)type)->byte_size_expr, 10);
                html_attr_end();
            }
            if (((coda_asciiText *)type)->fixed_value != NULL)
            {
                html_attr_begin("fixed&nbsp;value", &first_attribute);
                ff_printf("\"");
                generate_escaped_html_string(((coda_asciiText *)type)->fixed_value,
                                        strlen(((coda_asciiText *)type)->fixed_value));
                ff_printf("\"");
                html_attr_end();
            }
            break;
        case tag_ascii_time:
            html_attr_begin("unit", &first_attribute);
            ff_printf("\"s\"");
            html_attr_end();
            if (((coda_asciiText *)((coda_asciiTime *)type)->base_type)->mappings != NULL)
            {
                coda_asciiMappings *mappings = ((coda_asciiText *)((coda_asciiTime *)type)->base_type)->mappings;

                for (i = 0; i < mappings->num_mappings; i++)
                {
                    html_attr_begin("mapping", &first_attribute);
                    ff_printf("\"");
                    generate_escaped_html_string(mappings->mapping[i]->str, mappings->mapping[i]->length);
                    ff_printf("\"&nbsp;-&gt;&nbsp;%f", ((coda_asciiFloatMapping *)mappings->mapping[i])->value);
                    html_attr_end();                        
                }
            }
            break;
        default:
            break;
    }
}

static void generate_html_bin_attributes(const coda_binType *type, int first_attribute)
{
    switch (type->tag)
    {
        case tag_bin_record:
        case tag_bin_union:
        case tag_bin_array:
            generate_html_ascbin_attributes((coda_ascbinType *)type, first_attribute);
            break;
        case tag_bin_integer:
        case tag_bin_float:
            {
                coda_binNumber *number;
                
                number = (coda_binNumber *)type;
                if (number->unit != NULL)
                {
                    html_attr_begin("unit", &first_attribute);
                    ff_printf("\"%s\"", number->unit);
                    html_attr_end();
                }
                if (number->conversion != NULL)
                {
                    html_attr_begin("converted&nbsp;unit", &first_attribute);
                    ff_printf("\"%s\" (multiply by %g/%g)",
                              (number->conversion->unit == NULL ? "" : number->conversion->unit),
                              number->conversion->numerator, number->conversion->denominator);
                    html_attr_end();
                }
                if (type->bit_size > 8 && ((type->tag == tag_bin_integer &&
                                            ((coda_binInteger *)type)->endianness == coda_little_endian) ||
                                           (type->tag == tag_bin_float &&
                                            ((coda_binFloat *)type)->endianness == coda_little_endian)))
                {
                    html_attr_begin("endianness", &first_attribute);
                    ff_printf("little endian");
                    html_attr_end();
                }
            }
            break;
        case tag_bin_raw:
            if (((coda_binRaw *)type)->bit_size_expr != NULL)
            {
                html_attr_begin("bit&nbsp;size", &first_attribute);
                generate_html_expr(((coda_binRaw *)type)->bit_size_expr, 10);
                html_attr_end();
            }
            if (((coda_binRaw *)type)->fixed_value != NULL)
            {
                html_attr_begin("fixed&nbsp;value", &first_attribute);
                ff_printf("\"");
                generate_escaped_html_string(((coda_binRaw *)type)->fixed_value,
                                        ((coda_binRaw *)type)->fixed_value_length);
                ff_printf("\"");
                html_attr_end();
            }
            break;
        case tag_bin_vsf_integer:
            if (((coda_binVSFInteger *)type)->unit != NULL)
            {
                html_attr_begin("unit", &first_attribute);
                ff_printf("\"%s\"", ((coda_binVSFInteger *)type)->unit);
                html_attr_end();
            }
            break;
        case tag_bin_time:
            html_attr_begin("unit", &first_attribute);
            ff_printf("\"s\"");
            html_attr_end();
            break;
        default:
            break;
    }
}

static void generate_html_xml_attributes(const coda_xmlType *type, int first_attribute)
{
    if (type->tag == tag_xml_record || type->tag == tag_xml_text || type->tag == tag_xml_ascii_type)
    {
        coda_xmlAttributeRecord *attributes = ((coda_xmlElement *)type)->attributes;
        int i;

        for (i = 0; i < attributes->num_attributes; i++)
        {
            if (!first_attribute)
            {
                fi_printf("<br /><br />\n");
            }
            else
            {
                first_attribute = 0;
            }
            fi_printf("<span class=\"attr_key\">xml attribute</span><span class=\"attr_value\">: %s",
                      attributes->attribute[i]->attr_name);
            if (attributes->attribute[i]->fixed_value != NULL)
            {
                ff_printf("=\"");
                generate_escaped_html_string(attributes->attribute[i]->fixed_value,
                                             strlen(attributes->attribute[i]->fixed_value));
                ff_printf("\"");
            }
            if (attributes->attribute[i]->optional)
            {
                ff_printf(" (optional)");
            }
            ff_printf("</span>\n");
        }
    }
}

static void generate_html_type(const coda_Type *type, int expand_named_type, int full_width)
{
    coda_special_type special_type;
    coda_native_type read_type;
    int64_t bit_size;
    int first_attribute = 1;
    int i;

    fi_printf("<table");
    if (!full_width)
    {
        ff_printf(" class=\"top\"");
    }
    ff_printf(">\n");

    /* type name and size */
    fi_printf("<tr>");
    fi_printf("<th ");
    if (type->format != coda_format_ascii && type->format != coda_format_binary)
    {
        ff_printf(" colspan=\"2\"");
    }
    ff_printf("align=\"left\">%s&nbsp;", coda_type_get_format_name(type->format));
    switch (type->type_class)
    {
        case coda_record_class:
            {
                int is_union;
                coda_type_get_record_union_status(type, &is_union);
                if (is_union)
                {
                    ff_printf("union");
                }
                else
                {
                    ff_printf("record");
                }
            }
            break;
        case coda_array_class:
            ff_printf("array");
            if (type->name == NULL || expand_named_type)
            {
                int num_dims;
                long dim[CODA_MAX_NUM_DIMS];

                coda_type_get_array_dim(type, &num_dims, dim);
                ff_printf("[");
                for (i = 0; i < num_dims; i++)
                {
                    if (i > 0)
                    {
                        ff_printf(", ");
                    }
                    if (dim[i] == -1)
                    {
                        ff_printf("<i><b>dim_%d</b></i>", i);
                    }
                    else
                    {
                        ff_printf("<b>%ld</b>", dim[i]);
                    }
                }
                ff_printf("]");
            }
            break;
        case coda_special_class:
            coda_type_get_special_type((coda_Type *)type, &special_type);
            ff_printf("%s", coda_type_get_special_type_name(special_type));
            break;
        default:
            coda_type_get_read_type(type, &read_type);
            ff_printf("%s", coda_type_get_native_type_name(read_type));

            switch (type->format)
            {
                case coda_format_ascii:
                    switch (((coda_asciiType *)type)->tag)
                    {
                        case tag_ascii_line_separator:
                            ff_printf(" [line&nbsp;separator]");
                            break;
                        case tag_ascii_line:
                            ff_printf(" [line]");
                            break;
                        case tag_ascii_white_space:
                            ff_printf(" [white&nbsp;space]");
                            break;
                        case tag_ascii_integer:
                        case tag_ascii_float:
                            if (((coda_asciiNumber *)type)->conversion != NULL)
                            {
                                ff_printf(" (%s)", coda_type_get_native_type_name(coda_native_type_double));
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case coda_format_binary:
                    switch (((coda_binType *)type)->tag)
                    {
                        case tag_bin_integer:
                        case tag_bin_float:
                            if (((coda_binNumber *)type)->conversion != NULL)
                            {
                                ff_printf(" (%s)", coda_type_get_native_type_name(coda_native_type_double));
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
    }
    if (type->name != NULL)
    {
        ff_printf("&nbsp;\"<a class=\"header\" href=\"../types/%s.html\">%s</a>\"", type->name, type->name);
    }
    ff_printf("</th>");
    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        fi_printf("<td style=\"width:10px\" align=\"right\"><i>size</i>:&nbsp;");
        coda_type_get_bit_size(type, &bit_size);
        if (bit_size >= 0)
        {
            long bytes = (long)(bit_size >> 3);
            int bits = (int)(bit_size & 0x7);

            ff_printf("%ld", bytes);
            if (bits > 0)
            {
                ff_printf(":%d", bits);
            }
        }
        else
        {
            ff_printf("variable");
        }
        ff_printf("</td>");
    }
    fi_printf("</tr>\n");

    if (type->name == NULL || expand_named_type)
    {
        fi_printf("<tr valign=\"top\">\n");
        fi_printf("<td colspan=\"2\">\n");

        /* attributes */
        if (type->description != NULL)
        {
            indent();
            generate_xml_string(type->description);
            ff_printf("\n");
            first_attribute = 0;
        }
        switch (type->format)
        {
            case coda_format_ascii:
                generate_html_ascii_attributes((coda_asciiType *)type, first_attribute);
                break;
            case coda_format_binary:
                generate_html_bin_attributes((coda_binType *)type, first_attribute);
                break;
            case coda_format_xml:
                generate_html_xml_attributes((coda_xmlType *)type, first_attribute);
                break;
            case coda_format_hdf4:
            case coda_format_hdf5:
                assert(0);
                exit(1);
        }

        /* base types */
        if (type->format == coda_format_xml && ((coda_xmlType *)type)->tag == tag_xml_ascii_type)
        {
            if (!first_attribute)
            {
                fi_printf("<br />\n");
            }
            fi_printf("<blockquote>\n");
            generate_html_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, 0, 1);
            fi_printf("</blockquote>\n");                    
        }
        else
        {
            switch (type->type_class)
            {
                case coda_record_class:
                    {
                        long num_fields;

                        coda_type_get_num_record_fields(type, &num_fields);
                        if (num_fields == 0)
                        {
                            break;
                        }
                        if (!first_attribute)
                        {
                            fi_printf("<br /><br />\n");
                        }
                        fi_printf("<table class=\"fancy\" border=\"1\" cellspacing=\"0\" width=\"100%%\">\n");
                        fi_printf("<tr><th class=\"subhdr\">id</th><th class=\"subhdr\">field&nbsp;name</th>"
                                  "<th class=\"subhdr\">definition</th></tr>\n");
                        for (i = 0; i < num_fields; i++)
                        {
                            coda_Type *field_type;
                            const char *field_name;
                            int first_field_attribute = 1;
                            int hidden;
                            int available;
                            
                            coda_type_get_record_field_type(type, i, &field_type);
                            coda_type_get_record_field_name(type, i, &field_name);
                            coda_type_get_record_field_hidden_status(type, i, &hidden);
                            coda_type_get_record_field_available_status(type, i, &available);

                            fi_printf("<tr valign=\"top\">");
                            fi_printf("<td>%d</td>", i);                    
                            if (hidden)
                            {
                                fi_printf("<td>%s</td>\n", field_name);
                            }
                            else
                            {
                                fi_printf("<td><b>%s</b></td>\n", field_name);
                            }
                            
                            /* attributes */
                            fi_printf("<td>\n");
                            if (type->format == coda_format_xml)
                            {
                                const char *xml_name;
                                char *element_name;
                                char *namespace;
                                
                                first_field_attribute = 0;
                                fi_printf("<span class=\"attr_key\">xml name</span><span class=\"attr_value\">: <b>");
                                if (((coda_xmlType *)type)->tag == tag_xml_root)
                                {
                                    xml_name = ((coda_xmlRoot *)type)->field->xml_name;
                                }
                                else
                                {
                                    xml_name = ((coda_xmlElement *)type)->field[i]->xml_name;
                                }
                                element_name_and_namespace_from_xml_name(xml_name, &element_name, &namespace);
                                if (namespace != NULL)
                                {
                                    ff_printf("{%s}", namespace);
                                    free(namespace);
                                }
                                ff_printf("%s", element_name);
                                free(element_name);
                                ff_printf("</b></span><br /><br />");
                            }
                            generate_html_type(field_type, 0, 1);
                            if (hidden)
                            {
                                if (first_field_attribute)
                                {
                                    fi_printf("<br />\n");
                                }
                                first_field_attribute = 0;
                                fi_printf("<span class=\"attr_key\">hidden</span><span class=\"attr_value\">: "
                                          "true</span>");
                                fi_printf("<br /><br />\n");
                            }
                            if (available == -1)
                            {
                                if (first_field_attribute)
                                {
                                    fi_printf("<br />\n");
                                }
                                first_field_attribute = 0;
                                fi_printf("<span class=\"attr_key\">available</span><span class=\"attr_value\">: ");
                                switch (type->format)
                                {
                                    case coda_format_ascii:
                                    case coda_format_binary:
                                        generate_html_expr(((coda_ascbinRecord *)type)->field[i]->available_expr, 10);
                                        break;
                                    default:
                                        ff_printf("optional");
                                        break;
                                }
                                ff_printf("</span>");
                                fi_printf("<br /><br />\n");
                            }
                            if (type->format == coda_format_ascii || type->format == coda_format_binary)
                            {
                                if (((coda_ascbinRecord *)type)->field[i]->bit_offset_expr != NULL)
                                {
                                    if (first_field_attribute)
                                    {
                                        fi_printf("<br />\n");
                                    }
                                    first_field_attribute = 0;
                                    fi_printf("<span class=\"attr_key\">bit offset</span>"
                                              "<span class=\"attr_value\">: ");
                                    generate_html_expr(((coda_ascbinRecord *)type)->field[i]->bit_offset_expr, 10);
                                    ff_printf("</span>");
                                    fi_printf("<br /><br />\n");
                                }
                            }
                            fi_printf("</td>\n");
                            fi_printf("</tr>\n");
                        }
                        fi_printf("</table>\n");
                    }
                    break;
                case coda_array_class:
                    {
                        coda_Type *base_type;

                        if (!first_attribute)
                        {
                            fi_printf("<br />\n");
                        }
                        fi_printf("<blockquote>\n");
                        coda_type_get_array_base_type(type, &base_type);
                        generate_html_type(base_type, 0, 1);
                        fi_printf("</blockquote>\n");
                    }
                    break;
                case coda_special_class:
                    {
                        coda_Type *base_type;

                        if (!first_attribute)
                        {
                            fi_printf("<br />\n");
                        }
                        fi_printf("<blockquote>\n");
                        coda_type_get_special_base_type(type, &base_type);
                        generate_html_type(base_type, 0, 1);
                        fi_printf("</blockquote>\n");
                    }
                    break;
                default:
                    break;
            }
        }
        fi_printf("</td>\n");
        fi_printf("</tr>\n");
    }

    fi_printf("</table>\n");
}


/* precedence
 1: unary minus, not
 2: pow
 3: mul, div, mod
 4: add, sub
 5: lt, le, gt, ge
 6: eq, ne
 7: and
 8: or
10: <start>
*/
static void generate_html_expr(const coda_Expr *expr, int precedence)
{
    assert(expr != NULL);
    
    switch (expr->tag)
    {
        case expr_add:
            if (precedence < 4)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 4);
            ff_printf(" + ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                ff_printf(")");
            }
            break;
        case expr_and:
            if (precedence < 7)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 7);
            ff_printf(" <b>and</b> ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 7);
            if (precedence < 7)
            {
                ff_printf(")");
            }
            break;
        case expr_array_add:
            ff_printf("<b>add</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_array_all:
            ff_printf("<b>all</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_array_count:
            ff_printf("<b>count</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_array_exists:
            ff_printf("<b>exists</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_array_index:
            ff_printf("<b>index</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_asciiline:
            ff_printf("<b>asciiline</b>");
            break;
        case expr_bit_offset:
            ff_printf("<b>bitoffset</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_bit_size:
            ff_printf("<b>bitsize</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_byte_offset:
            ff_printf("<b>byteoffset</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_byte_size:
            ff_printf("<b>bytesize</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_bytes:
            ff_printf("<b>bytes</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            if (((coda_ExprOperation *)expr)->operand[0] != NULL)
            {
                ff_printf(",");
                generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            }
            ff_printf(")");
            break;
        case expr_constant_boolean:
            if (((coda_ExprBoolConstant *)expr)->value)
            {
                ff_printf("<b>true</b>");
            }
            else
            {
                ff_printf("<b>false</b>");
            }
            break;
        case expr_constant_double:
            ff_printf("%f", ((coda_ExprDoubleConstant *)expr)->value);
            break;
        case expr_constant_integer:
            ff_printf("%lld", ((coda_ExprIntegerConstant *)expr)->value);
            break;
        case expr_constant_string:
            ff_printf("\"");
            generate_escaped_html_string(((coda_ExprStringConstant *)expr)->value,
                                    ((coda_ExprStringConstant *)expr)->length);
            ff_printf("\"");
            break;
        case expr_divide:
            if (precedence < 3)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 3);
            ff_printf(" / ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                ff_printf(")");
            }
            break;
        case expr_equal:
            if (precedence < 6)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 6);
            ff_printf(" == ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                ff_printf(")");
            }
            break;
        case expr_exists:
            ff_printf("<b>exists</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_file_size:
            ff_printf("<b>filesize</b>()");
            break;
        case expr_filename:
            ff_printf("<b>filename</b>()");
            break;
        case expr_float:
            ff_printf("<b>float</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_for_index:
            ff_printf("<i>i</i>");
            break;
        case expr_for:
            ff_printf("<b>for</b> <i>i</i> = ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(" <b>to</b> ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            if (((coda_ExprOperation *)expr)->operand[2] != NULL)
            {
                ff_printf(" <b>step</b> ");
                generate_html_expr(((coda_ExprOperation *)expr)->operand[2], 10);
            }
            ff_printf(" <b>do</b><br />");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[3], 10);
            break;
        case expr_goto_array_element:
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf("[");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf("]");
            break;
        case expr_goto_attribute:
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf("@%s", ((coda_ExprOperation *)expr)->identifier);
            break;
        case expr_goto_begin:
            ff_printf(":");
            break;
        case expr_goto_field:
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            if (((coda_ExprOperation *)expr)->operand[0]->tag != expr_goto_root)
            {
                ff_printf("/");
            }
            ff_printf("%s", ((coda_ExprOperation *)expr)->identifier);
            break;
        case expr_goto_here:
            ff_printf(".");
            break;
        case expr_goto_parent:
            if (((coda_ExprOperation *)expr)->operand[0] != NULL)
            {
                generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
                ff_printf("/");
            }
            ff_printf("..");
            break;
        case expr_goto_root:
            ff_printf("/");
            break;
        case expr_goto:
            ff_printf("<b>goto</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_greater_equal:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 5);
            ff_printf(" >= ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_greater:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 5);
            ff_printf(" > ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_if:
            ff_printf("<b>if</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[2], 10);
            ff_printf(")");
            break;
        case expr_index:
            ff_printf("<b>index</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_integer:
            ff_printf("<b>int</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_length:
            ff_printf("<b>length</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_less_equal:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 5);
            ff_printf(" <= ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_less:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 5);
            ff_printf(" < ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_max:
            ff_printf("<b>max</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_min:
            ff_printf("<b>min</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(")");
            break;
        case expr_modulo:
            if (precedence < 3)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 3);
            ff_printf(" %% ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                ff_printf(")");
            }
            break;
        case expr_multiply:
            if (precedence < 3)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 3);
            ff_printf(" * ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                ff_printf(")");
            }
            break;
        case expr_neg:
            ff_printf("-");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 1);
            break;
        case expr_not_equal:
            if (precedence < 6)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 6);
            ff_printf(" != ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                ff_printf(")");
            }
            break;
        case expr_not:
            ff_printf("!");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 1);
            break;
        case expr_num_elements:
            ff_printf("<b>numelements</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_or:
            if (precedence < 8)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 8);
            ff_printf(" <b>or</b> ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 8);
            if (precedence < 8)
            {
                ff_printf(")");
            }
            break;
        case expr_power:
            if (precedence < 2)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 2);
            ff_printf(" ^ ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 2);
            if (precedence < 2)
            {
                ff_printf(")");
            }
            break;
        case expr_product_class:
            ff_printf("<b>productclass</b>()");
            break;
        case expr_product_type:
            ff_printf("<b>producttype</b>()");
            break;
        case expr_product_version:
            ff_printf("<b>productversion</b>()");
            break;
        case expr_sequence:
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(";<br />");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            break;
        case expr_string:
            ff_printf("<b>string</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            if (((coda_ExprOperation *)expr)->operand[1] != NULL)
            {
                ff_printf(", ");
                generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            }
            ff_printf(")");
            break;
        case expr_substr:
            ff_printf("<b>substr</b>(");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            ff_printf(", ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[2], 10);
            ff_printf(")");
            break;
        case expr_subtract:
            if (precedence < 4)
            {
                ff_printf("(");
            }
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 4);
            ff_printf(" - ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                ff_printf(")");
            }
            break;
        case expr_variable_exists:
            ff_printf("<b>exists</b>(<i>$%s</i>, ", ((coda_ExprOperation *)expr)->identifier);
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_variable_index:
            ff_printf("<b>index</b>(<i>$%s</i>, ", ((coda_ExprOperation *)expr)->identifier);
            generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
            ff_printf(")");
            break;
        case expr_variable_set:
            ff_printf("<i>$%s</i>", ((coda_ExprOperation *)expr)->identifier);
            if (((coda_ExprOperation *)expr)->operand[0] != NULL)
            {
                ff_printf("[");
                generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
                ff_printf("]");
            }
            ff_printf(" = ");
            generate_html_expr(((coda_ExprOperation *)expr)->operand[1], 10);
            break;
        case expr_variable_value:
            ff_printf("<i>$%s</i>", ((coda_ExprOperation *)expr)->identifier);
            if (((coda_ExprOperation *)expr)->operand[0] != NULL)
            {
                ff_printf("[");
                generate_html_expr(((coda_ExprOperation *)expr)->operand[0], 10);
                ff_printf("]");
            }
            break;
    }
}

static void generate_html_named_type(const char *filename, coda_Type *type)
{

    assert(type->name != NULL);

    FFILE = fopen(filename, "w");
    if (FFILE == NULL)
    {
        fprintf(stderr, "ERROR: could not create %s\n", filename);
        exit(1);
    }

    fi_printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n");
    fi_printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
              "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\n");
    fi_printf("<html>\n\n");
    fi_printf("<head>\n");
    fi_printf("<title>%s</title>\n", type->name);
    fi_printf("<link rel=\"stylesheet\" href=\"../../codadef.css\" type=\"text/css\" />\n");
    fi_printf("</head>\n\n");
    fi_printf("<body>\n");
    fi_printf("<h1>%s</h1>\n", type->name);

    generate_html_type(type, 1, 0);

    fi_printf("</body>\n\n");
    fi_printf("</html>\n");

    fclose(FFILE);
}

static void generate_html_product_definition(const char *filename, coda_ProductDefinition *product_definition)
{
    int i;

    FFILE = fopen(filename, "w");
    if (FFILE == NULL)
    {
        fprintf(stderr, "ERROR: could not create %s\n", filename);
        exit(1);
    }
    
    fi_printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n");
    fi_printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
              "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\n");
    fi_printf("<html>\n\n");
    fi_printf("<head>\n");
    fi_printf("<title>%s v%d</title>\n", product_definition->product_type->name, product_definition->version);
    fi_printf("<link rel=\"stylesheet\" href=\"../../codadef.css\" type=\"text/css\" />\n");
    fi_printf("</head>\n\n");
    fi_printf("<body>\n");
    fi_printf("<h1>%s version %d</h1>\n", product_definition->product_type->name,
              product_definition->version);
    if (product_definition->description != NULL)
    {
        fi_printf("<p>");
        generate_xml_string(product_definition->description);
        ff_printf("</p>\n");
    }

    fi_printf("<h3>root type</h3>\n");
    generate_html_type(product_definition->root_type, 1, 0);

    fi_printf("<h3>detection rule</h3>\n");
    
    if (product_definition->num_detection_rules == 0)
    {
        fi_printf("<p>This product has no detection rule and can not be automatically recognised.</p>\n");        
    }
    else
    {
        fi_printf("<p>This product definition is applicable if a product matches the following rule:</p>\n");
        
        fi_printf("<table class=\"fancy\" border=\"1\" cellspacing=\"0\" width=\"600px\">");
        INDENT++;
        fi_printf("<tr>\n");
        INDENT++;
        fi_printf("<td>\n");
        INDENT++;
        for (i = 0; i < product_definition->num_detection_rules; i++)
        {
            coda_DetectionRule *detection_rule = product_definition->detection_rule[i];
            int j;
            
            for (j = 0; j < detection_rule->num_entries; j++)
            {
                coda_DetectionRuleEntry *entry = detection_rule->entry[j];
                if (entry->use_filename)
                {
                    fi_printf("<b>filename</b>[%ld:%ld] == \"", (long)entry->offset,
                              (long)entry->offset + entry->value_length);
                    generate_escaped_html_string(entry->value, entry->value_length);
                    ff_printf("\"");
                }
                else
                {
                    if (entry->offset != -1)
                    {
                        if (entry->value != NULL)
                        {
                            fi_printf("<b>file</b>[%ld:%ld] == \"", (long)entry->offset,
                                      (long)entry->offset + entry->value_length);
                            generate_escaped_html_string(entry->value, entry->value_length);
                            ff_printf("\"");
                        }
                        else
                        {
                            fi_printf("<b>filesize</b> >= %ld", (long)entry->offset);
                        }
                    }
                    else if (entry->path != NULL)
                    {
                        if (entry->value != NULL)
                        {
                            fi_printf("%s == \"", entry->path);
                            generate_escaped_html_string(entry->value, entry->value_length);
                            ff_printf("\"");
                        }
                        else
                        {
                            fi_printf("%s <b>exists</b>", entry->path);
                        }
                    }
                    else if (entry->value != NULL)
                    {
                        fi_printf("<b>file</b> <b>contains</b> \"");
                        generate_escaped_html_string(entry->value, entry->value_length);
                        ff_printf("\"");
                    }
                    else
                    {
                        assert(0);
                        exit(1);
                    }
                }
                if (j < detection_rule->num_entries -1)
                {
                    ff_printf(" <b>and</b><br />");
                }
                ff_printf("\n");
            }
            if (i < product_definition->num_detection_rules - 1)
            {
                fi_printf("<br /><br /><b>or</b><br /><br />\n");
            }
        }
        INDENT--;
        fi_printf("</tr>\n");
        INDENT--;
        fi_printf("</td>\n");
        INDENT--;
        fi_printf("</table>\n");
    }

    if (product_definition->num_product_variables > 0)
    {
        fi_printf("<h3>product variables</h3>\n");

        fi_printf("<table class=\"fancy\" border=\"1\" cellspacing=\"0\" width=\"600px\">\n");
        fi_printf("<tr><th>name</th><th>size</th><th>initialisation</th></tr>\n");
        for (i = 0; i < product_definition->num_product_variables; i++)
        {
            coda_ProductVariable *variable;

            variable = product_definition->product_variable[i];
            fi_printf("<tr><td id=\"%s_%s\">%s</td><td>", product_definition->name, variable->name, variable->name);
            if (variable->size_expr != NULL)
            {
                ff_printf("[");
                generate_html_expr(variable->size_expr, 10);
                ff_printf("]");
            }
            ff_printf("</td><td>");
            generate_html_expr(variable->init_expr, 10);
            ff_printf("</td></tr>\n");
        }
        fi_printf("</table>\n");
    }

    fi_printf("</body>\n\n");
    fi_printf("</html>\n");

    fclose(FFILE);
}

static int type_uses_type(const coda_Type *type1, const coda_Type *type2, int include_self)
{
    if (type1 == type2 && include_self)
    {
        return 1;
    }

    switch (type1->type_class)
    {
        case coda_record_class:
            {
                long num_fields;
                long i;

                coda_type_get_num_record_fields(type1, &num_fields);
                for (i = 0; i < num_fields; i++)
                {
                    coda_Type *field_type;
                    coda_type_get_record_field_type(type1, i, &field_type);
                    if (type_uses_type(field_type, type2, 1))
                    {
                        return 1;
                    }
                }
            }
            break;
        case coda_array_class:
            {
                coda_Type *base_type;
                coda_type_get_array_base_type(type1, &base_type);
                return type_uses_type(base_type, type2, 1);
            }
        case coda_special_class:
            {
                coda_Type *base_type;
                coda_type_get_special_base_type(type1, &base_type);
                return type_uses_type(base_type, type2, 1);
            }
        default:
            break;
    }

    return 0;
}

static int compare_named_types(const void *t1, const void *t2)
{
    return strcasecmp((*(coda_Type **)t1)->name, (*(coda_Type **)t2)->name);
}

static void generate_html_named_types_index(const char *filename, coda_ProductClass *product_class)
{
    coda_Type **sorted_list;
    int i, j, k;

    FFILE = fopen(filename, "w");
    if (FFILE == NULL)
    {
        fprintf(stderr, "ERROR: could not create %s\n", filename);
        exit(1);
    }
    
    fi_printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n");
    fi_printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
              "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\n");
    fi_printf("<html>\n\n");
    fi_printf("<head>\n");
    fi_printf("<title>%s named types</title>\n", product_class->name);
    fi_printf("<link rel=\"stylesheet\" href=\"../codadef.css\" type=\"text/css\" />\n");
    fi_printf("</head>\n\n");
    fi_printf("<body>\n");
    fi_printf("<h1>%s named types</h1>\n", product_class->name);
    fi_printf("<table class=\"top\">\n");
    fi_printf("<tr><th>named&nbsp;type</th><th>used&nbsp;by</th></tr>\n");

    sorted_list = malloc(product_class->num_named_types * sizeof(coda_Type *));
    assert(sorted_list != NULL);
    memcpy(sorted_list, product_class->named_type, product_class->num_named_types * sizeof(coda_Type *));
    qsort(sorted_list, product_class->num_named_types, sizeof(coda_Type *), compare_named_types);

    for (i = 0; i < product_class->num_named_types; i++)
    {
        coda_Type *type;
        int prod_count;
        int type_count;

        type = sorted_list[i];

        fi_printf("<tr>");

        fi_printf("<td><a href=\"types/%s.html\">%s</a></td>", type->name, type->name);

        fi_printf("<td>");
        prod_count = 0;
        type_count = 0;
        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_ProductType *product_type = product_class->product_type[j];

            for (k = 0; k < product_type->num_product_definitions; k++)
            {
                if (type_uses_type(product_type->product_definition[k]->root_type, type, 1))
                {
                    if (prod_count == 0)
                    {
                        fi_printf("products: ");
                    }
                    else
                    {
                        ff_printf(", ");
                    }
                    ff_printf("<a href=\"products/%s.html\">%s</a>", product_type->product_definition[k]->name,
                              product_type->product_definition[k]->name);
                    prod_count++;
                }
            }
        }
        if (prod_count > 0)
        {
            ff_printf("<br /><br />");
        }

        for (j = 0; j < product_class->num_named_types; j++)
        {
            if (type_uses_type(product_class->named_type[j], type, 0))
            {
                if (type_count == 0)
                {
                    fi_printf("named types: ");
                }
                else
                {
                    ff_printf(", ");
                }
                ff_printf("<a href=\"types/%s.html\">%s</a>", product_class->named_type[j]->name,
                          product_class->named_type[j]->name);
                type_count++;
            }
        }
        if (type_count > 0)
        {
            ff_printf("<br /><br />");
        }
        else if (prod_count == 0)
        {
            fi_printf("<i>none</i><br /><br />");
        }
        fi_printf("</td>");

        fi_printf("</tr>\n");
    }
    fi_printf("</table>\n");

    fi_printf("</body>\n\n");
    fi_printf("</html>\n");

    free(sorted_list);

    fclose(FFILE);
}

static void generate_html_product_class(const char *filename, coda_ProductClass *product_class)
{
    int i;
    
    FFILE = fopen(filename, "w");
    if (FFILE == NULL)
    {
        fprintf(stderr, "ERROR: could not create %s\n", filename);
        exit(1);
    }
    
    fi_printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n");
    fi_printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
              "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
    ff_printf("\n");
    fi_printf("<html>\n\n");
    
    fi_printf("<head>\n");
    fi_printf("<title>%s</title>\n", product_class->name);
    fi_printf("<link rel=\"stylesheet\" href=\"../codadef.css\" type=\"text/css\" />\n");
    fi_printf("</head>\n\n");
    
    fi_printf("<body>\n\n");

    fi_printf("<h1>%s product class</h1>\n", product_class->name);
        
    if (product_class->description != NULL)
    {
        fi_printf("<p>");
        generate_xml_string(product_class->description);
        ff_printf("</p>\n");
    }

    if (product_class->num_named_types > 0)
    {
        fi_printf("<p>An overview of the named types for this product class is provided "
                  "<a href=\"types.html\">here</a>.</p>\n");
    }
    
    fi_printf("<h2>Product overview</h2>\n");
    
    fi_printf("<table class=\"top\">\n");
    INDENT++;
    fi_printf("<tr><th>product&nbsp;type</th><th>description</th><th colspan=\"3\">product&nbsp;definitions</th>"
              "</tr>\n");

    for (i = 0; i < product_class->num_product_types; i++)
    {
        coda_ProductType *product_type = product_class->product_type[i];
        int j;

        if (product_type->num_product_definitions == 0)
        {
            continue;
        }

        fi_printf("<tr><td rowspan=\"%d\">%s</td><td rowspan=\"%d\">", product_type->num_product_definitions + 1,
                  product_type->name, product_type->num_product_definitions + 1);
        if (product_type->description != NULL)
        {
            generate_xml_string(product_type->description);
        }
        ff_printf("</td><th class=\"subhdr\">version</th><th class=\"subhdr\">format</th><th class=\"subhdr\">definition</th></tr>\n");

        if (product_type->num_product_definitions > 0)
        {
            for (j = 0; j < product_type->num_product_definitions; j++)
            {
                coda_ProductDefinition *product_definition = product_type->product_definition[j];
                fi_printf("<tr><td align=\"center\">%d</td><td>%s</td><td><a href=\"products/%s.html\">%s</td></tr>\n",
                          product_definition->version, coda_type_get_format_name(product_definition->format),
                          product_definition->name, product_definition->name);
            }
        }
        ff_printf("\n");
    }
    INDENT--;
    fi_printf("</table>\n");

    
    fi_printf("</body>\n\n");
    fi_printf("</html>");
    
    fclose(FFILE);
}

static void generate_html_index(const char *filename)
{
    int i, j;
    
    FFILE = fopen(filename, "w");
    if (FFILE == NULL)
    {
        fprintf(stderr, "ERROR: could not create %s\n", filename);
        exit(1);
    }
    
    fi_printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n");
    fi_printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
              "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n");
    ff_printf("\n");
    fi_printf("<html>\n\n");
    
    fi_printf("<head>\n");
    fi_printf("<title>Product Format Definitions</title>\n");
    fi_printf("<link rel=\"stylesheet\" href=\"codadef.css\" type=\"text/css\" />\n");
    fi_printf("</head>\n\n");
    
    fi_printf("<body>\n\n");
    fi_printf("<h1>Product Format Definitions</h1>\n");
    
    fi_printf("<p>This documentation contains the product format definitions for all products that are supported. ");
    ff_printf("The definitions that you will find here are complete formal definitions of a product. ");
    ff_printf("This means that every bit of information necessary to be able to read data from a product file is "
              "provided; this includes expressions for e.g. calculating the sizes of arrays, determining the "
              "availabillity of optional data, and automaticaully recognizing the product type of a file. ");
    ff_printf("This information may not always be available (in a formal way) in the official product format "
              "definition documents for a product and the definitions that you will find here may thus sometimes "
              "deviate from these official documents.</p>\n");
    fi_printf("<table class=\"top\">\n");
    fi_printf("<tr><th>product&nbsp;class</th><th>description</th><th>revision</th></tr>\n");
    
    for (i = 0; i < coda_data_dictionary->num_product_classes; i++)
    {
        coda_ProductClass *product_class = coda_data_dictionary->product_class[i];
        int has_products;
        
        has_products = 0;
        for (j = 0; j < product_class->num_product_types; j++)
        {
            if (product_class->product_type[j]->num_product_definitions > 0)
            {
                has_products = 1;
                break;
            }
        }
        if (!has_products)
        {
            continue;
        }
        
        fi_printf("<tr><td><a href=\"%s/index.html\">%s</a></td><td>", product_class->name, product_class->name);
        if (product_class->description != NULL)
        {
            generate_xml_string(product_class->description);
        }
        ff_printf("</td><td>%d</td></tr>\n", product_class->revision);
    }
    fi_printf("</table>\n");

    fi_printf("<p>An explanation of the data types and expressions that are used in this documentation can be found in "
              "the CODA documentation.</p>\n");
        
    fi_printf("</body>\n\n");
    fi_printf("</html>");
    
    fclose(FFILE);
}

static void generate_html(const char *prefixdir)
{
    char *filename;
    int i, j, k;

    filename = (char *)malloc(strlen(prefixdir) + 1000);
    assert(filename != NULL);
    
    sprintf(filename, "%s/index.html", prefixdir);
    generate_html_index(filename);

    for (i = 0; i < coda_data_dictionary->num_product_classes; i++)
    {
        coda_ProductClass *product_class = coda_data_dictionary->product_class[i];
        int has_products;

        has_products = 0;
        for (j = 0; j < product_class->num_product_types; j++)
        {
            if (product_class->product_type[j]->num_product_definitions > 0)
            {
                has_products = 1;
                break;
            }
        }
        if (!has_products)
        {
            continue;
        }

        sprintf(filename, "%s/%s", prefixdir, product_class->name);
        mkdir(filename, 0777);
        
        sprintf(filename, "%s/%s/index.html", prefixdir, product_class->name);
        generate_html_product_class(filename, product_class);        

        sprintf(filename, "%s/%s/products", prefixdir, product_class->name);
        mkdir(filename, 0777);
        
        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_ProductType *product_type = product_class->product_type[j];
            
            for (k = 0; k < product_type->num_product_definitions; k++)
            {
                sprintf(filename, "%s/%s/products/%s.html", prefixdir, product_class->name,
                        product_type->product_definition[k]->name);
                generate_html_product_definition(filename, product_type->product_definition[k]);
            }
        }

        if (product_class->num_named_types > 0)
        {
            sprintf(filename, "%s/%s/types.html", prefixdir, product_class->name);
            generate_html_named_types_index(filename, product_class);
            sprintf(filename, "%s/%s/types", prefixdir, product_class->name);
            mkdir(filename, 0777);
            
            for (j = 0; j < product_class->num_named_types; j++)
            {
                sprintf(filename, "%s/%s/types/%s.html", prefixdir, product_class->name,
                        product_class->named_type[j]->name);
                generate_html_named_type(filename, product_class->named_type[j]);
            }
        }
    }

    free(filename);
}

static void print_version()
{
    printf("codadd %s\n", libcoda_version);
    printf("Copyright (C) 2007-2008 S&T, The Netherlands\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codadd\n");
    printf("        Try to read all product definitions and report any problems\n");
    printf("\n");
    printf("    codadd doc <directory>\n");
    printf("        Generate HTML product format documentation in the specified directory\n");
    printf("\n");
    printf("    codadd -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codadd -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
}

static void set_definition_path(const char *argv0)
{
    char *location;
    
    if (coda_path_for_program(argv0, &location) != 0)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    if (location != NULL)
    {
#ifdef WIN32
        const char *definition_path = "../definitions";
#else
        const char *definition_path = "../share/"PACKAGE"/definitions";
#endif
        char *path;
        
        if (coda_path_from_path(location, 1, definition_path, &path) != 0)
        {
            printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
            exit(1);
        }
        coda_path_free(location);
        coda_set_definition_path(path);
        coda_path_free(path);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
        {
            print_help();
            exit(0);
        }
        
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
        {
            print_version();
            exit(0);
        }
    }

    if (getenv("CODA_DEFINITION") == NULL)
    {
        set_definition_path(argv[0]);
    }
    
    coda_option_read_all_definitions = 1;
    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (argc == 1)
    {
        coda_done();
        exit(0);
    }
    
    if (strcmp(argv[1], "doc") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "ERROR: Incorrect arguments\n");
            print_help();
            exit(1);
        }
        generate_html(argv[2]);
    }
    else
    {
        fprintf(stderr, "ERROR: Incorrect arguments\n");
        print_help();
        exit(1);
    }
    
    coda_done();

    return 0;
}
