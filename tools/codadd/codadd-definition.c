/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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
#include <time.h>

#include "coda-internal.h"
#include "coda-expr.h"
#include "coda-type.h"
#include "coda-definition.h"

#ifdef __GNUC__
static int ff_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static int fi_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#else
static int ff_printf(const char *fmt, ...);
static int fi_printf(const char *fmt, ...);
#endif

static int INDENT = 0;
static FILE *FFILE;

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

static int fic_printf(int *compound_element, const char *fmt, ...)
{
    int result;
    va_list ap;

    if (!(*compound_element))
    {
        ff_printf(">\n");
        *compound_element = 1;
        INDENT++;
    }

    indent();

    va_start(ap, fmt);
    result = vfprintf(FFILE, fmt, ap);
    va_end(ap);

    return result;
}

static void generate_escaped_string(const char *str, int length)
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

static void generate_type(const coda_type *type, coda_format parent_format)
{
    coda_type_class type_class;
    coda_special_type special_type = coda_special_no_data;
    coda_format format;
    const char *type_name = "";
    const char *description;
    coda_type *attributes;
    int is_compound = 0;
    int is_union = 0;
    long i;

    coda_type_get_class(type, &type_class);
    coda_type_get_format(type, &format);
    coda_type_get_description(type, &description);
    coda_type_get_attributes(type, &attributes);

    switch (type_class)
    {
        case coda_record_class:
            coda_type_get_record_union_status(type, &is_union);
            type_name = is_union ? "Union" : "Record";
            break;
        case coda_array_class:
            type_name = "Array";
            break;
        case coda_integer_class:
            type_name = "Integer";
            break;
        case coda_real_class:
            type_name = "Float";
            break;
        case coda_text_class:
            switch (((coda_type_text *)type)->special_text_type)
            {
                case ascii_text_default:
                    type_name = "Text";
                    break;
                case ascii_text_line_separator:
                    type_name = "AsciiLineSeparator";
                    break;
                case ascii_text_line_with_eol:
                    /* this should only be used internally for product_file->asciilines */
                    assert(0);
                    exit(1);
                case ascii_text_line_without_eol:
                    type_name = "AsciiLine";
                    break;
                case ascii_text_whitespace:
                    type_name = "AsciiWhiteSpace";
                    break;
            }
            break;
        case coda_raw_class:
            type_name = "Raw";
            break;
        case coda_special_class:
            {
                coda_type_get_special_type(type, &special_type);
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                        type_name = "VSFInteger";
                        break;
                    case coda_special_time:
                        type_name = "Time";
                        break;
                    case coda_special_complex:
                        type_name = "Complex";
                        break;
                    case coda_special_no_data:
                        assert(0);
                        exit(1);
                }
            }
            break;
    }
    fi_printf("<cd:%s", type_name);
    if (format != parent_format)
    {
        ff_printf(" format=\"%s\"", coda_type_get_format_name(format));
    }
    if (type_class == coda_special_class && special_type == coda_special_time)
    {
        ff_printf(" timeformat=\"");
        coda_expression_print_xml(((coda_type_special *)type)->value_expr, ff_printf);
        ff_printf("\"");
    }
    if (description != NULL)
    {
        fic_printf(&is_compound, "<cd:Description>");
        generate_xml_string(description, -1);
        ff_printf("</cd:Description>\n");
    }
    if ((format == coda_format_ascii || format == coda_format_binary) &&
        (type_class == coda_integer_class || type_class == coda_real_class || type_class == coda_text_class ||
         type_class == coda_raw_class))
    {
        if (type->bit_size >= 0)
        {
            char s[21];

            if (type->bit_size % 8 == 0)
            {
                coda_str64(type->bit_size / 8, s);
                fic_printf(&is_compound, "<cd:ByteSize>%s</cd:ByteSize>\n", s);
            }
            else
            {
                coda_str64(type->bit_size, s);
                fic_printf(&is_compound, "<cd:BitSize>%s</cd:BitSize>\n", s);
            }
        }
        else if (type->size_expr != NULL)
        {
            fic_printf(&is_compound, "<cd:%s>", (type->bit_size == -8 ? "ByteSize" : "BitSize"));
            coda_expression_print_xml(type->size_expr, ff_printf);
            ff_printf("</cd:%s>\n", (type->bit_size == -8 ? "ByteSize" : "BitSize"));
        }
    }
    if (type_class == coda_record_class && type->size_expr != NULL)
    {
        assert(type->bit_size != -8);
        fic_printf(&is_compound, "<cd:BitSize>");
        coda_expression_print_xml(type->size_expr, ff_printf);
        ff_printf("</cd:BitSize>\n");
    }
    if (type->attributes != NULL)
    {
        for (i = 0; i < type->attributes->num_fields; i++)
        {
            coda_type_record_field *field = type->attributes->field[i];

            fic_printf(&is_compound, "<cd:Attribute name=\"%s\"", field->name);
            if (field->real_name != NULL && strcmp(field->real_name, field->name) != 0)
            {
                ff_printf(" real_name=\"");
                generate_xml_string(field->real_name, -1);
                ff_printf("\"");
            }
            ff_printf(">\n");
            INDENT++;
            generate_type(field->type, format);
            if (field->hidden)
            {
                fi_printf("<cd:Hidden/>\n");
            }
            if (field->optional)
            {
                if (field->available_expr != NULL)
                {
                    fi_printf("<cd:Available>");
                    coda_expression_print_xml(field->available_expr, ff_printf);
                    ff_printf("</cd:Available>\n");
                }
                else
                {
                    fi_printf("<cd:Optional/>\n");
                }
            }
            INDENT--;
            fi_printf("</cd:Attribute>\n");
        }
    }

    switch (type_class)
    {
        case coda_record_class:
            {
                long num_record_fields;

                if (is_union && ((coda_type_record *)type)->union_field_expr != NULL)
                {
                    fic_printf(&is_compound, "<cd:FieldExpression>");
                    coda_expression_print_xml(((coda_type_record *)type)->union_field_expr, ff_printf);
                    ff_printf("</cd:FieldExpression>\n");
                }

                coda_type_get_num_record_fields(type, &num_record_fields);
                for (i = 0; i < num_record_fields; i++)
                {
                    coda_type_record_field *field = ((coda_type_record *)type)->field[i];

                    fic_printf(&is_compound, "<cd:Field name=\"%s\"", field->name);
                    if (field->real_name != NULL && strcmp(field->real_name, field->name) != 0)
                    {
                        ff_printf(" real_name=\"");
                        generate_xml_string(field->real_name, -1);
                        ff_printf("\"");
                    }
                    ff_printf(">\n");
                    INDENT++;
                    generate_type(field->type, format);
                    if (field->hidden)
                    {
                        fi_printf("<cd:Hidden/>\n");
                    }
                    if (field->optional)
                    {
                        if (field->available_expr != NULL)
                        {
                            fi_printf("<cd:Available>");
                            coda_expression_print_xml(field->available_expr, ff_printf);
                            ff_printf("</cd:Available>\n");
                        }
                        else
                        {
                            fi_printf("<cd:Optional/>\n");
                        }
                    }
                    if (field->bit_offset >= 0)
                    {
                        char s[21];

                        coda_str64(field->bit_offset, s);
                        fi_printf("<cd:BitOffset>%s</cd:BitOffset>\n", s);
                    }
                    else if (field->bit_offset_expr != NULL)
                    {
                        fi_printf("<cd:BitOffset>");
                        coda_expression_print_xml(field->bit_offset_expr, ff_printf);
                        ff_printf("</cd:BitOffset>\n");
                    }
                    INDENT--;
                    fi_printf("</cd:Field>\n");
                }
            }
            break;
        case coda_array_class:
            {
                coda_type_array *array_type = (coda_type_array *)type;

                for (i = 0; i < array_type->num_dims; i++)
                {
                    if (array_type->dim[i] >= 0)
                    {
                        char s[21];

                        coda_str64(array_type->dim[i], s);
                        fic_printf(&is_compound, "<cd:Dimension>%s</cd:Dimension>\n", s);
                    }
                    else if (array_type->dim_expr[i] != NULL)
                    {
                        fic_printf(&is_compound, "<cd:Dimension>");
                        coda_expression_print_xml(array_type->dim_expr[i], ff_printf);
                        ff_printf("</cd:Dimension>\n");
                    }
                    else
                    {
                        fic_printf(&is_compound, "<cd:Dimension/>\n");
                    }
                }
                if (!is_compound)
                {
                    ff_printf(">\n");
                    is_compound = 1;
                    INDENT++;
                }
                generate_type(array_type->base_type, format);
            }
            break;
        case coda_integer_class:
        case coda_real_class:
            {
                coda_type_number *number_type = (coda_type_number *)type;

                fic_printf(&is_compound, "<cd:NativeType>%s</cd:NativeType>\n",
                           coda_type_get_native_type_name(type->read_type));
                if (number_type->unit != NULL)
                {
                    fi_printf("<cd:Unit>");
                    generate_xml_string(number_type->unit, -1);
                    ff_printf("</cd:Unit>\n");
                }
                if (number_type->endianness == coda_little_endian)
                {
                    fi_printf("<cd:LittleEndian/>\n");
                }
                if (number_type->conversion != NULL)
                {
                    fi_printf("<cd:Conversion numerator=\"%g\" denominator=\"%g\"",
                              number_type->conversion->numerator, number_type->conversion->denominator);
                    if (number_type->conversion->add_offset != 0)
                    {
                        ff_printf(" offset=\"%g\"", number_type->conversion->add_offset);
                    }
                    if (!coda_isNaN(number_type->conversion->invalid_value))
                    {
                        ff_printf(" invalid=\"%g\"", number_type->conversion->invalid_value);
                    }
                    if (number_type->conversion->unit != NULL)
                    {
                        ff_printf(">\n");
                        INDENT++;
                        fi_printf("<cd:Unit>");
                        generate_xml_string(number_type->conversion->unit, -1);
                        ff_printf("</cd:Unit>\n");
                        INDENT--;
                        fi_printf("</cd:Conversion>\n");
                    }
                    else
                    {
                        ff_printf("/>\n");
                    }
                }
                if (number_type->mappings != NULL)
                {
                    for (i = 0; i < number_type->mappings->num_mappings; i++)
                    {
                        coda_ascii_mapping *mapping = number_type->mappings->mapping[i];

                        fi_printf("<cd:Mapping string=\"");
                        generate_escaped_string(mapping->str, mapping->length);
                        ff_printf("\" value=\"");
                        if (type_class == coda_integer_class)
                        {
                            char s[21];

                            coda_str64(((coda_ascii_integer_mapping *)mapping)->value, s);
                            ff_printf("%s", s);
                        }
                        else
                        {
                            char s[24];

                            coda_strfl(((coda_ascii_float_mapping *)mapping)->value, s);
                            ff_printf("%s", s);
                        }
                        ff_printf("\"/>");
                    }
                }
            }
            break;
        case coda_text_class:
            {
                coda_type_text *text_type = (coda_type_text *)type;

                if (type->read_type != coda_native_type_string)
                {
                    fic_printf(&is_compound, "<cd:NativeType>%s</cd:NativeType>\n",
                               coda_type_get_native_type_name(type->read_type));
                }
                if (text_type->fixed_value != NULL)
                {
                    fic_printf(&is_compound, "<cd:FixedValue>");
                    generate_escaped_string(text_type->fixed_value, -1);
                    ff_printf("</cd:FixedValue>\n");
                }
            }
            break;
        case coda_raw_class:
            {
                coda_type_raw *raw_type = (coda_type_raw *)type;

                if (raw_type->fixed_value != NULL)
                {
                    fic_printf(&is_compound, "<cd:FixedValue>");
                    generate_escaped_string(raw_type->fixed_value, raw_type->fixed_value_length);
                    ff_printf("</cd:FixedValue>\n");
                }
            }
            break;
        case coda_special_class:
            {
                if (special_type == coda_special_vsf_integer && ((coda_type_special *)type)->unit != NULL)
                {
                    fi_printf("<cd:Unit>");
                    generate_xml_string(((coda_type_special *)type)->unit, -1);
                    ff_printf("</cd:Unit>\n");
                }
                if (!is_compound)
                {
                    ff_printf(">\n");
                    is_compound = 1;
                    INDENT++;
                }
                generate_type(((coda_type_special *)type)->base_type, format);
            }
            break;
    }
    if (is_compound)
    {
        INDENT--;
        fi_printf("</cd:%s>\n", type_name);
    }
    else
    {
        ff_printf("/>\n");
    }
}

static void generate_product_variable(const coda_product_variable *variable)
{
    fi_printf("<cd:ProductVariable name=\"%s\">\n", variable->name);
    INDENT++;
    if (variable->size_expr)
    {
        fi_printf("<cd:Dimension>");
        coda_expression_print_xml(variable->size_expr, ff_printf);
        ff_printf("</cd:Dimension>\n");
    }
    fi_printf("<cd:Init>");
    coda_expression_print_xml(variable->init_expr, ff_printf);
    ff_printf("</cd:Init>\n");
    INDENT--;
    fi_printf("</cd:ProductVariable>\n");
}

static void generate_product_definition(const coda_product *product)
{
    const coda_product_definition *definition = product->product_definition;
    coda_type *type = coda_get_type_for_dynamic_type(product->root_type);
    coda_format format = product->format;
    const char *name = "untitled";
    char currentdate[15] = "";
    struct tm *ttm;
    time_t t;
    int i;

    if (definition != NULL)
    {
        name = definition->name;
        format = definition->format;
        type = definition->root_type;
    }
    if (type == NULL)
    {
        format = product->format;
        type = product->root_type->definition;
    }
    t = time(NULL);
    ttm = localtime(&t);
    if (ttm)
    {
        sprintf(currentdate, "%04d-%02d-%02d", ttm->tm_year + 1900, ttm->tm_mon + 1, ttm->tm_mday);
    }
    fi_printf("<cd:ProductDefinition id=\"%s\" format=\"%s\" last-modified=\"%s\" "
              "xmlns:cd=\"http://www.stcorp.nl/coda/definition/2008/07\">\n", name,
              coda_type_get_format_name(product->format), currentdate);
    INDENT++;
    generate_type(type, format);
    if (definition != NULL)
    {
        for (i = 0; i < definition->num_product_variables; i++)
        {
            generate_product_variable(definition->product_variable[i]);
        }
    }
    INDENT--;
    fi_printf("</cd:ProductDefinition>\n");
}

void generate_definition(const char *output_file_name, const char *file_name)
{
    coda_product *product;
    int result;

    result = coda_open(file_name, &product);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(file_name, &product);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        fflush(stderr);
        exit(1);
    }

    FFILE = stdout;
    if (output_file_name != NULL)
    {
        FFILE = fopen(output_file_name, "w");
        if (FFILE == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            fflush(stderr);
            exit(1);
        }
    }
    ff_printf("<?xml version=\"1.0\"?>\n");
    generate_product_definition(product);

    coda_close(product);
}
