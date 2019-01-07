/*
 * Copyright (C) 2007-2019 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-expr.h"
#include "coda-type.h"
#include "coda-definition.h"

#ifdef __GNUC__
static int ff_printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
static int fi_printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
#else
static int ff_printf(const char *fmt, ...);
static int fi_printf(const char *fmt, ...);
#endif

static int INDENT = 0;
static FILE *FFILE;

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

static int ff_printf(const char *fmt, ...)
{
    int result;
    va_list ap;

    va_start(ap, fmt);
    result = vfprintf(FFILE, fmt, ap);
    va_end(ap);

    return result;
}

static int fi_printf(const char *fmt, ...)
{
    int result;
    va_list ap;

    indent();

    va_start(ap, fmt);
    result = vfprintf(FFILE, fmt, ap);
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
    if (length == 0 || str == NULL)
    {
        return;
    }

    if (length < 0)
    {
        length = (int)strlen(str);
    }

    while (length > 0)
    {
        switch (*str)
        {
            case '\033':       /* windows does not recognize '\e' */
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
                if (!isprint(*str))
                {
                    ff_printf("\\%03o", (int)(unsigned char)*str);
                }
                else
                {
                    ff_printf("%c", *str);
                }
                break;
        }
        str++;
        length--;
    }
}

static void generate_xml_string(const char *str, int length)
{
    if (length == 0 || str == NULL)
    {
        return;
    }

    if (length < 0)
    {
        length = (int)strlen(str);
    }

    while (length > 0)
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
        length--;
    }
}

static void generate_html_type(const coda_type *type, int expand_named_type, int full_width);

static void html_attr_begin(const char *key_name, int *first_attribute)
{
    if (first_attribute != NULL)
    {
        if (!*first_attribute)
        {
            fi_printf("<br /><br />\n");
        }
        else
        {
            *first_attribute = 0;
        }
    }
    fi_printf("<span class=\"attr_key\">%s</span><span class=\"attr_value\">: ", key_name);
}

static void html_attr_end(void)
{
    ff_printf("</span>\n");
}

static void generate_html_attributes(const coda_type *type, int *first_attribute)
{
    long num_fields;
    long i;

    coda_type_get_num_record_fields(type, &num_fields);

    if (!*first_attribute)
    {
        fi_printf("<br />");
    }

    for (i = 0; i < num_fields; i++)
    {
        coda_type *field_type;
        const char *field_name;
        const char *real_name;
        int first_field_attribute = 1;
        int hidden;
        int available;

        coda_type_get_record_field_type(type, i, &field_type);
        coda_type_get_record_field_name(type, i, &field_name);
        coda_type_get_record_field_real_name(type, i, &real_name);
        coda_type_get_record_field_hidden_status(type, i, &hidden);
        coda_type_get_record_field_available_status(type, i, &available);

        if (first_attribute != NULL)
        {
            if (!*first_attribute)
            {
                fi_printf("<br />\n");
            }
            else
            {
                *first_attribute = 0;
            }
        }
        fi_printf("<table style=\"border-style: none\" cellspacing=\"0\" width=\"100%%\">\n");
        fi_printf("<tr valign=\"top\"><td style=\"border-style: none\">");
        html_attr_begin("attribute", NULL);
        if (hidden)
        {
            fi_printf("%s", field_name);
        }
        else
        {
            fi_printf("<b>%s</b>", field_name);
        }

        ff_printf("</td><td style=\"border-width: 2px\">");

        /* attributes */
        if (strcmp(field_name, real_name) != 0)
        {
            html_attr_begin("real name", NULL);
            ff_printf("<b>");
            if (type->format == coda_format_xml)
            {
                char *element_name;
                char *namespace;

                element_name_and_namespace_from_xml_name(real_name, &element_name, &namespace);
                if (namespace != NULL)
                {
                    ff_printf("{%s}", namespace);
                    free(namespace);
                }
                ff_printf("%s", element_name);
                free(element_name);
            }
            else
            {
                ff_printf("%s", real_name);
            }
            ff_printf("</b>");
            html_attr_end();
            ff_printf("<br />");
        }

        generate_html_type(field_type, 0, 1);

        if (hidden)
        {
            if (first_field_attribute)
            {
                fi_printf("<br />\n");
            }
            html_attr_begin("hidden", &first_field_attribute);
            ff_printf("true");
            html_attr_end();
        }
        if (available == -1)
        {
            if (first_field_attribute)
            {
                fi_printf("<br />\n");
            }
            html_attr_begin("available", &first_field_attribute);
            if (((coda_type_record *)type)->field[i]->available_expr != NULL)
            {
                coda_expression_print_html(((coda_type_record *)type)->field[i]->available_expr, ff_printf);
            }
            else
            {
                ff_printf("optional");
            }
            html_attr_end();
        }
        if (((coda_type_record *)type)->field[i]->bit_offset_expr != NULL)
        {
            if (first_field_attribute)
            {
                fi_printf("<br />\n");
            }
            html_attr_begin("bit offset", &first_field_attribute);
            coda_expression_print_html(((coda_type_record *)type)->field[i]->bit_offset_expr, ff_printf);
            html_attr_end();
        }

        html_attr_end();
        ff_printf("</td></tr>\n");
        fi_printf("</table>\n");
    }
}

static void generate_html_type(const coda_type *type, int expand_named_type, int full_width)
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
    coda_type_get_bit_size(type, &bit_size);
    fi_printf("<tr>");
    fi_printf("<th ");
    if (bit_size < 0)
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
            coda_type_get_special_type((coda_type *)type, &special_type);
            ff_printf("%s", coda_type_get_special_type_name(special_type));
            break;
        default:
            coda_type_get_read_type(type, &read_type);
            ff_printf("%s", coda_type_get_native_type_name(read_type));

            if (type->type_class == coda_text_class && type->format == coda_format_ascii)
            {
                switch (((coda_type_text *)type)->special_text_type)
                {
                    case ascii_text_default:
                        break;
                    case ascii_text_line_separator:
                        ff_printf(" [line&nbsp;separator]");
                        break;
                    case ascii_text_line_with_eol:
                    case ascii_text_line_without_eol:
                        ff_printf(" [line]");
                        break;
                    case ascii_text_whitespace:
                        ff_printf(" [white&nbsp;space]");
                        break;
                }
            }
            if (type->type_class == coda_integer_class || type->type_class == coda_real_class)
            {
                if (((coda_type_number *)type)->conversion != NULL)
                {
                    ff_printf(" (%s)", coda_type_get_native_type_name(coda_native_type_double));
                }
            }
    }
    if (type->name != NULL)
    {
        ff_printf("&nbsp;\"<a class=\"header\" href=\"../types/%s.html\">%s</a>\"", type->name, type->name);
    }
    ff_printf("</th>");
    if (bit_size >= 0)
    {
        char s[21];

        fi_printf("<td style=\"width:10px\" align=\"right\"><i>size</i>:&nbsp;");
        coda_str64(bit_size >> 3, s);
        if (bit_size & 0x7)
        {
            ff_printf("%s:%d", s, (int)(bit_size & 0x7));
        }
        else
        {
            ff_printf("%s", s);
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
            generate_xml_string(type->description, -1);
            ff_printf("\n");
            first_attribute = 0;
        }
        if (type->size_expr != NULL)
        {
            if (type->bit_size == -8)
            {
                html_attr_begin("byte&nbsp;size", &first_attribute);
            }
            else
            {
                html_attr_begin("bit&nbsp;size", &first_attribute);
            }
            coda_expression_print_html(type->size_expr, ff_printf);
            html_attr_end();
        }
        switch (type->type_class)
        {
            case coda_record_class:
                {
                    coda_type_record *record = ((coda_type_record *)type);

                    if (record->union_field_expr != NULL)
                    {
                        html_attr_begin("field&nbsp;expr", &first_attribute);
                        coda_expression_print_html(record->union_field_expr, ff_printf);
                        html_attr_end();
                    }
                }
                break;
            case coda_array_class:
                {
                    coda_type_array *array = ((coda_type_array *)type);

                    for (i = 0; i < array->num_dims; i++)
                    {
                        if (array->dim[i] < 0)
                        {
                            char dimstr[10];

                            sprintf(dimstr, "dim_%d", i);
                            html_attr_begin(dimstr, &first_attribute);
                            if (array->dim_expr[i] != NULL)
                            {
                                coda_expression_print_html(array->dim_expr[i], ff_printf);
                            }
                            else
                            {
                                ff_printf("determined automatically from %s file",
                                          coda_type_get_format_name(type->format));
                            }
                            html_attr_end();
                        }
                    }
                }
                break;
            case coda_integer_class:
            case coda_real_class:
                {
                    coda_type_number *number = (coda_type_number *)type;

                    if (number->unit != NULL)
                    {
                        html_attr_begin("unit", &first_attribute);
                        ff_printf("\"%s\"", number->unit);
                        html_attr_end();
                    }
                    if (number->conversion != NULL)
                    {
                        char s[24];
                        int first = 1;

                        html_attr_begin("converted&nbsp;unit", &first_attribute);
                        ff_printf("\"%s\" (", (number->conversion->unit == NULL ? "" : number->conversion->unit));
                        if (number->conversion->numerator != 1.0 || number->conversion->denominator != 1.0)
                        {
                            first = 0;
                            coda_strfl(number->conversion->numerator, s);
                            ff_printf("multiply by %s", s);
                            coda_strfl(number->conversion->denominator, s);
                            ff_printf("/%s", s);
                        }
                        if (number->conversion->add_offset != 0.0)
                        {
                            if (!first)
                            {
                                ff_printf(", ");
                            }
                            first = 0;
                            coda_strfl(number->conversion->add_offset, s);
                            ff_printf("add %s", s);
                        }
                        if (!coda_isNaN(number->conversion->invalid_value))
                        {
                            if (!first)
                            {
                                ff_printf(", ");
                            }
                            coda_strfl(number->conversion->invalid_value, s);
                            ff_printf("set %s to NaN", s);
                        }
                        ff_printf(")");
                        html_attr_end();
                    }
                    if (number->endianness == coda_little_endian)
                    {
                        html_attr_begin("endianness", &first_attribute);
                        ff_printf("little endian");
                        html_attr_end();
                    }
                    if (number->mappings != NULL)
                    {
                        coda_ascii_mappings *mappings = number->mappings;

                        for (i = 0; i < mappings->num_mappings; i++)
                        {
                            html_attr_begin("mapping", &first_attribute);
                            ff_printf("\"");
                            generate_escaped_html_string(mappings->mapping[i]->str, mappings->mapping[i]->length);
                            ff_printf("\"&nbsp;-&gt;&nbsp;");
                            if (type->type_class == coda_integer_class)
                            {
                                char s[21];

                                coda_str64(((coda_ascii_integer_mapping *)mappings->mapping[i])->value, s);
                                ff_printf("%s", s);
                            }
                            else
                            {
                                char s[24];

                                coda_strfl(((coda_ascii_float_mapping *)mappings->mapping[i])->value, s);
                                ff_printf("%s", s);
                            }
                            html_attr_end();
                        }
                    }
                }
                break;
            case coda_text_class:
                {
                    coda_type_text *text = (coda_type_text *)type;

                    if (text->fixed_value != NULL)
                    {
                        html_attr_begin("fixed&nbsp;value", &first_attribute);
                        ff_printf("\"");
                        generate_escaped_html_string(text->fixed_value, (int)strlen(text->fixed_value));
                        ff_printf("\"");
                        html_attr_end();
                    }
                }
                break;
            case coda_raw_class:
                {
                    coda_type_raw *raw = (coda_type_raw *)type;

                    if (raw->fixed_value != NULL)
                    {
                        html_attr_begin("fixed&nbsp;value", &first_attribute);
                        ff_printf("\"");
                        generate_escaped_html_string(raw->fixed_value, raw->fixed_value_length);
                        ff_printf("\"");
                        html_attr_end();
                    }
                }
                break;
            case coda_special_class:
                {
                    coda_type_special *special = (coda_type_special *)type;

                    if (special->unit != NULL)
                    {
                        html_attr_begin("unit", &first_attribute);
                        ff_printf("\"%s\"", special->unit);
                        html_attr_end();
                    }
                    if (special->value_expr != NULL)
                    {
                        html_attr_begin("value", &first_attribute);
                        coda_expression_print_html(special->value_expr, ff_printf);
                        html_attr_end();
                    }
                }
                break;
        }
        if (type->attributes != NULL)
        {
            generate_html_attributes((coda_type *)type->attributes, &first_attribute);
        }
        /* base types */
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
                        coda_type *field_type;
                        const char *field_name;
                        const char *real_name;
                        int first_field_attribute = 1;
                        int hidden;
                        int available;

                        coda_type_get_record_field_type(type, i, &field_type);
                        coda_type_get_record_field_name(type, i, &field_name);
                        coda_type_get_record_field_real_name(type, i, &real_name);
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
                        if (strcmp(field_name, real_name) != 0)
                        {
                            html_attr_begin("real name", NULL);
                            ff_printf("<b>");
                            if (type->format == coda_format_xml)
                            {
                                char *element_name;
                                char *namespace;

                                element_name_and_namespace_from_xml_name(real_name, &element_name, &namespace);
                                if (namespace != NULL)
                                {
                                    ff_printf("{%s}", namespace);
                                    free(namespace);
                                }
                                ff_printf("%s", element_name);
                                free(element_name);
                            }
                            else
                            {
                                ff_printf("%s", real_name);
                            }
                            ff_printf("</b>");
                            html_attr_end();
                            ff_printf("<br /><br />");
                        }

                        generate_html_type(field_type, 0, 1);

                        if (hidden)
                        {
                            if (first_field_attribute)
                            {
                                fi_printf("<br />\n");
                            }
                            html_attr_begin("hidden", &first_field_attribute);
                            ff_printf("true");
                            html_attr_end();
                        }
                        if (available == -1)
                        {
                            if (first_field_attribute)
                            {
                                fi_printf("<br />\n");
                            }
                            html_attr_begin("available", &first_field_attribute);
                            if (((coda_type_record *)type)->field[i]->available_expr != NULL)
                            {
                                coda_expression_print_html(((coda_type_record *)type)->field[i]->available_expr,
                                                           ff_printf);
                            }
                            else
                            {
                                ff_printf("optional");
                            }
                            html_attr_end();
                        }
                        if (((coda_type_record *)type)->field[i]->bit_offset_expr != NULL)
                        {
                            if (first_field_attribute)
                            {
                                fi_printf("<br />\n");
                            }
                            html_attr_begin("bit offset", &first_field_attribute);
                            coda_expression_print_html(((coda_type_record *)type)->field[i]->bit_offset_expr,
                                                       ff_printf);
                            html_attr_end();
                        }
                        fi_printf("</td>\n");
                        fi_printf("</tr>\n");
                    }
                    fi_printf("</table>\n");
                }
                break;
            case coda_array_class:
                {
                    coda_type *base_type;

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
                    coda_type *base_type;

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
        fi_printf("</td>\n");
        fi_printf("</tr>\n");
    }

    fi_printf("</table>\n");
}


static void generate_html_named_type(const char *filename, coda_type *type)
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

static void generate_html_product_definition(const char *filename, coda_product_definition *product_definition)
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
    fi_printf("<h1>%s version %d</h1>\n", product_definition->product_type->name, product_definition->version);
    fi_printf("<h2>%s</h2>\n", product_definition->name);
    if (product_definition->description != NULL)
    {
        fi_printf("<p>");
        generate_xml_string(product_definition->description, -1);
        ff_printf("</p>\n");
    }

    if (product_definition->root_type != NULL)
    {
        fi_printf("<h3>root type</h3>\n");
        generate_html_type(product_definition->root_type, 1, 0);
    }

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
            coda_detection_rule *detection_rule = product_definition->detection_rule[i];
            int path_state = 0;
            int j;

            for (j = 0; j < detection_rule->num_entries; j++)
            {
                if (detection_rule->entry[j]->path != NULL)
                {
                    ff_printf("<b>exists</b>(");
                    generate_escaped_html_string(detection_rule->entry[j]->path, -1);
                    ff_printf(")");
                    path_state = 1;
                }
                if (detection_rule->entry[j]->expression != NULL)
                {
                    if (detection_rule->entry[j]->path != NULL)
                    {
                        ff_printf(" <b>and</b><br />");
                        if (path_state == 1)
                        {
                            ff_printf("<b>at</b>(");
                            generate_escaped_html_string(detection_rule->entry[j]->path, -1);
                            ff_printf(",<br />");
                            path_state = 2;
                        }
                    }
                    coda_expression_print_html(detection_rule->entry[j]->expression, ff_printf);
                }
                if (path_state == 2)
                {
                    if (j == detection_rule->num_entries - 1 || detection_rule->entry[j + 1]->path != NULL)
                    {
                        ff_printf(")");
                    }
                }
                if (j < detection_rule->num_entries - 1)
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
            coda_product_variable *variable;

            variable = product_definition->product_variable[i];
            fi_printf("<tr><td id=\"%s_%s\">%s</td><td>", product_definition->name, variable->name, variable->name);
            if (variable->size_expr != NULL)
            {
                ff_printf("[");
                coda_expression_print_html(variable->size_expr, ff_printf);
                ff_printf("]");
            }
            ff_printf("</td><td>");
            coda_expression_print_html(variable->init_expr, ff_printf);
            ff_printf("</td></tr>\n");
        }
        fi_printf("</table>\n");
    }

    fi_printf("</body>\n\n");
    fi_printf("</html>\n");

    fclose(FFILE);
}

static int type_uses_type(const coda_type *type1, const coda_type *type2, int include_self)
{
    if (type1 == NULL)
    {
        return 0;
    }

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
                    coda_type *field_type;

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
                coda_type *base_type;

                coda_type_get_array_base_type(type1, &base_type);
                return type_uses_type(base_type, type2, 1);
            }
        case coda_special_class:
            {
                coda_type *base_type;

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
    return strcasecmp((*(coda_type **)t1)->name, (*(coda_type **)t2)->name);
}

static void generate_html_named_types_index(const char *filename, coda_product_class *product_class)
{
    coda_type **sorted_list;
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

    sorted_list = malloc(product_class->num_named_types * sizeof(coda_type *));
    assert(sorted_list != NULL);
    memcpy(sorted_list, product_class->named_type, product_class->num_named_types * sizeof(coda_type *));
    qsort(sorted_list, product_class->num_named_types, sizeof(coda_type *), compare_named_types);

    for (i = 0; i < product_class->num_named_types; i++)
    {
        coda_type *type;
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
            coda_product_type *product_type = product_class->product_type[j];

            for (k = 0; k < product_type->num_product_definitions; k++)
            {
                coda_product_definition *product_definition = product_type->product_definition[k];

                if (type_uses_type(product_definition->root_type, type, 1))
                {
                    if (prod_count == 0)
                    {
                        fi_printf("products: ");
                    }
                    else
                    {
                        ff_printf(", ");
                    }
                    ff_printf("<a href=\"products/%s.html\">%s</a>", product_definition->name,
                              product_definition->name);
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

static void generate_html_product_class(const char *filename, coda_product_class *product_class)
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
        generate_xml_string(product_class->description, -1);
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
        coda_product_type *product_type = product_class->product_type[i];
        int j;

        if (product_type->num_product_definitions == 0)
        {
            continue;
        }

        fi_printf("<tr><td rowspan=\"%d\">%s</td><td rowspan=\"%d\">", product_type->num_product_definitions + 1,
                  product_type->name, product_type->num_product_definitions + 1);
        if (product_type->description != NULL)
        {
            generate_xml_string(product_type->description, -1);
        }
        ff_printf
            ("</td><th class=\"subhdr\">version</th><th class=\"subhdr\">format</th><th class=\"subhdr\">definition</th></tr>\n");

        if (product_type->num_product_definitions > 0)
        {
            for (j = 0; j < product_type->num_product_definitions; j++)
            {
                coda_product_definition *product_definition = product_type->product_definition[j];

                fi_printf("<tr><td align=\"center\">%d</td><td>%s</td><td><a href=\"products/%s.html\">%s</td>"
                          "</tr>\n", product_definition->version, coda_type_get_format_name(product_definition->format),
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

    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_product_class *product_class = coda_global_data_dictionary->product_class[i];
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
            generate_xml_string(product_class->description, -1);
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

static void create_directory(const char *filename)
{
#if WIN32
    CreateDirectory(filename, NULL);
#else
    mkdir(filename, 0777);
#endif
}

void generate_html(const char *prefixdir)
{
    char *filename;
    int i, j, k;

    filename = (char *)malloc(strlen(prefixdir) + 1000);
    assert(filename != NULL);

    sprintf(filename, "%s/index.html", prefixdir);
    generate_html_index(filename);

    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_product_class *product_class = coda_global_data_dictionary->product_class[i];
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
        create_directory(filename);
        sprintf(filename, "%s/%s/index.html", prefixdir, product_class->name);
        generate_html_product_class(filename, product_class);

        sprintf(filename, "%s/%s/products", prefixdir, product_class->name);
        create_directory(filename);

        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_product_type *product_type = product_class->product_type[j];

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
            create_directory(filename);

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
