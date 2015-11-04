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
static int ff_printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
static int fi_printf(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
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
        length = strlen(str);
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
        length = strlen(str);
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

/* precedence
 1: unary minus, not
 2: pow
 3: mul, div, mod
 4: add, sub
 5: lt, le, gt, ge
 6: eq, ne
 7: and
 8: or
 9: logical_and
 10: logical_or
 15: <start>
 */
static void generate_expr(const coda_expression *expr, int precedence)
{
    assert(expr != NULL);

    switch (expr->tag)
    {
        case expr_abs:
            ff_printf("abs(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_add:
            if (precedence < 4)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 4);
            ff_printf(" + ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                ff_printf(")");
            }
            break;
        case expr_array_add:
            ff_printf("add(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_array_all:
            ff_printf("all(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_and:
            if (precedence < 7)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 7);
            ff_printf(" &amp; ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 7);
            if (precedence < 7)
            {
                ff_printf(")");
            }
            break;
        case expr_ceil:
            ff_printf("ceil(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_array_count:
            ff_printf("count(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_array_exists:
            ff_printf("exists(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_array_index:
            ff_printf("index(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_asciiline:
            ff_printf("asciiline");
            break;
        case expr_bit_offset:
            ff_printf("bitoffset(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_bit_size:
            ff_printf("bitsize(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_byte_offset:
            ff_printf("byteoffset(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_byte_size:
            ff_printf("bytesize(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_bytes:
            ff_printf("bytes(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                ff_printf(",");
                generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            }
            ff_printf(")");
            break;
        case expr_constant_boolean:
            if (((coda_expression_bool_constant *)expr)->value)
            {
                ff_printf("true");
            }
            else
            {
                ff_printf("false");
            }
            break;
        case expr_constant_float:
            {
                char s[24];

                coda_strfl(((coda_expression_float_constant *)expr)->value, s);
                ff_printf("%s", s);
            }
            break;
        case expr_constant_integer:
            {
                char s[21];

                coda_str64(((coda_expression_integer_constant *)expr)->value, s);
                ff_printf("%s", s);
            }
            break;
        case expr_constant_rawstring:
            {
                int i;

                ff_printf("\"");
                for (i = 0; i < ((coda_expression_string_constant *)expr)->length; i++)
                {
                    ff_printf("%c", ((coda_expression_string_constant *)expr)->value[i]);
                }
                ff_printf("\"");
            }
            break;
        case expr_constant_string:
            ff_printf("\"");
            generate_escaped_string(((coda_expression_string_constant *)expr)->value,
                                    ((coda_expression_string_constant *)expr)->length);
            ff_printf("\"");
            break;
        case expr_divide:
            if (precedence < 3)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            ff_printf(" / ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
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
            generate_expr(((coda_expression_operation *)expr)->operand[0], 6);
            ff_printf(" == ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                ff_printf(")");
            }
            break;
        case expr_exists:
            ff_printf("exists(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_file_size:
            ff_printf("filesize()");
            break;
        case expr_filename:
            ff_printf("filename()");
            break;
        case expr_float:
            ff_printf("float(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_floor:
            ff_printf("floor(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_for:
            ff_printf("for %s = ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(" to ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                ff_printf(" step ");
                generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            }
            ff_printf(" do ");
            generate_expr(((coda_expression_operation *)expr)->operand[3], 15);
            break;
        case expr_goto_array_element:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            }
            ff_printf("[");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf("]");
            break;
        case expr_goto_attribute:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            }
            ff_printf("@%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_begin:
            ff_printf(":");
            break;
        case expr_goto_field:
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[0]->tag != expr_goto_root)
            {
                ff_printf("/");
            }
            ff_printf("%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_here:
            ff_printf(".");
            break;
        case expr_goto_parent:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                ff_printf("/");
            }
            ff_printf("..");
            break;
        case expr_goto_root:
            ff_printf("/");
            break;
        case expr_goto:
            ff_printf("goto(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_greater_equal:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            ff_printf(" &gt;= ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
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
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            ff_printf(" &gt; ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_if:
            ff_printf("if(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            ff_printf(")");
            break;
        case expr_index:
            ff_printf("index(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_index_var:
            ff_printf("%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_integer:
            ff_printf("int(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_isinf:
            ff_printf("isinf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_ismininf:
            ff_printf("ismininf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_isnan:
            ff_printf("isnan(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_isplusinf:
            ff_printf("isplusinf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_length:
            ff_printf("length(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_less_equal:
            if (precedence < 5)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            ff_printf(" &lt;= ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
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
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            ff_printf(" &lt; ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                ff_printf(")");
            }
            break;
        case expr_logical_and:
            if (precedence < 9)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 9);
            ff_printf(" and ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 9);
            if (precedence < 9)
            {
                ff_printf(")");
            }
            break;
        case expr_logical_or:
            if (precedence < 10)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 10);
            ff_printf(" or ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 10);
            if (precedence < 10)
            {
                ff_printf(")");
            }
            break;
        case expr_ltrim:
            ff_printf("ltrim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_max:
            ff_printf("max(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_min:
            ff_printf("min(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_modulo:
            if (precedence < 3)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            ff_printf(" %% ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
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
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            ff_printf(" * ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                ff_printf(")");
            }
            break;
        case expr_neg:
            ff_printf("-");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 1);
            break;
        case expr_not_equal:
            if (precedence < 6)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 6);
            ff_printf(" != ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                ff_printf(")");
            }
            break;
        case expr_not:
            ff_printf("!");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 1);
            break;
        case expr_num_elements:
            ff_printf("numelements(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_or:
            if (precedence < 7)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 7);
            ff_printf(" | ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 7);
            if (precedence < 7)
            {
                ff_printf(")");
            }
            break;
        case expr_power:
            if (precedence < 2)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 2);
            ff_printf(" ^ ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 2);
            if (precedence < 2)
            {
                ff_printf(")");
            }
            break;
        case expr_product_class:
            ff_printf("productclass()");
            break;
        case expr_product_format:
            ff_printf("productformat()");
            break;
        case expr_product_type:
            ff_printf("producttype()");
            break;
        case expr_product_version:
            ff_printf("productversion()");
            break;
        case expr_regex:
            ff_printf("regex(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                ff_printf(", ");
                generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            }
            ff_printf(")");
            break;
        case expr_round:
            ff_printf("round(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_rtrim:
            ff_printf("rtrim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_sequence:
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf("; ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            break;
        case expr_string:
            ff_printf("str(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                ff_printf(", ");
                generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            }
            ff_printf(")");
            break;
        case expr_strtime:
            ff_printf("strtime(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                ff_printf(", ");
                generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            }
            ff_printf(")");
            break;
        case expr_substr:
            ff_printf("substr(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            ff_printf(")");
            break;
        case expr_subtract:
            if (precedence < 4)
            {
                ff_printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 4);
            ff_printf(" - ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                ff_printf(")");
            }
            break;
        case expr_time:
            ff_printf("time(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_trim:
            ff_printf("trim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_unbound_array_index:
            ff_printf("unboundindex(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
        case expr_variable_exists:
            ff_printf("exists($%s, ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_variable_index:
            ff_printf("index($%s, ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(")");
            break;
        case expr_variable_set:
            ff_printf("$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                ff_printf("[");
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                ff_printf("]");
            }
            ff_printf(" = ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            break;
        case expr_variable_value:
            ff_printf("$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                ff_printf("[");
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                ff_printf("]");
            }
            break;
        case expr_with:
            ff_printf("with(%s = ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            ff_printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            ff_printf(")");
            break;
    }
}

static void generate_type(const coda_type *type, const char *xmlname, coda_format parent_format)
{
    coda_type_class type_class;
    coda_special_type special_type = coda_special_no_data;
    coda_format format;
    const char *type_name = "";
    const char *description;
    coda_type *attributes;
    int is_compound = 0;
    int is_union = 0;
    int wrapped_type = 0;
    long i;

    coda_type_get_class(type, &type_class);
    coda_type_get_format(type, &format);
    coda_type_get_description(type, &description);
    coda_type_get_attributes(type, &attributes);

    if (parent_format == coda_format_xml && format != coda_format_xml)
    {
        fi_printf("<cd:Type");
        wrapped_type = 1;
        if (xmlname != NULL)
        {
            ff_printf(" namexml=\"%s\"", xmlname);
        }
        ff_printf(">\n");
        INDENT++;
        if (type->attributes != NULL)
        {
            for (i = 0; i < type->attributes->num_fields; i++)
            {
                coda_type_record_field *field = type->attributes->field[i];

                fi_printf("<cd:Attribute name=\"%s\"", field->real_name == NULL ? field->name : field->real_name);
                if (field->optional)
                {
                    ff_printf(">\n");
                    INDENT++;
                    fi_printf("<cd:Optional/>\n");
                    INDENT--;
                    fi_printf("</cd:Attribute>\n");
                }
                else
                {
                    ff_printf("/>\n");
                }
            }
        }
    }
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
            type_name = "Text";
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
    if (parent_format == coda_format_xml && xmlname != NULL && !wrapped_type)
    {
        if (type_class == coda_record_class || type_class == coda_text_class)
        {
            ff_printf(" namexml=\"%s\"", xmlname);
        }
    }
    if (description != NULL)
    {
        fic_printf(&is_compound, "<cd:Description>");
        generate_xml_string(description, -1);
        ff_printf("</cd:Description>\n");
    }
    if (type->bit_size >= 0)
    {
        char s[21];

        coda_str64(type->bit_size, s);
        fic_printf(&is_compound, "<cd:BitSize>%s</cd:BitSize>\n", s);
    }
    else if (type->size_expr != NULL)
    {
        fic_printf(&is_compound, "<cd:%s>", (type->bit_size == -8 ? "ByteSize" : "BitSize"));
        generate_expr(type->size_expr, 15);
        ff_printf("</cd:%s>\n", (type->bit_size == -8 ? "ByteSize" : "BitSize"));
    }
    if (type->attributes != NULL && !wrapped_type)
    {
        for (i = 0; i < type->attributes->num_fields; i++)
        {
            coda_type_record_field *field = type->attributes->field[i];

            fic_printf(&is_compound, "<cd:Attribute name=\"%s\"",
                       field->real_name == NULL ? field->name : field->real_name);
            if (field->optional)
            {
                ff_printf(">\n");
                INDENT++;
                fi_printf("<cd:Optional/>\n");
                INDENT--;
                fi_printf("</cd:Attribute>\n");
            }
            else
            {
                ff_printf("/>\n");
            }
        }
    }

    switch (type_class)
    {
        case coda_record_class:
            {
                long num_record_fields;

                if (is_union)
                {
                    fic_printf(&is_compound, "<cd:FieldExpression>");
                    generate_expr(((coda_type_record *)type)->union_field_expr, 15);
                    ff_printf("</cd:FieldExpression>\n");
                }

                coda_type_get_num_record_fields(type, &num_record_fields);
                for (i = 0; i < num_record_fields; i++)
                {
                    coda_type_record_field *field = ((coda_type_record *)type)->field[i];

                    fic_printf(&is_compound, "<cd:Field name=\"%s\">\n", field->name);
                    INDENT++;
                    generate_type(field->type, field->real_name == NULL ? field->name : field->real_name, format);
                    if (field->hidden)
                    {
                        fi_printf("<cd:Hidden/>\n");
                    }
                    if (field->optional)
                    {
                        if (field->available_expr != NULL)
                        {
                            fi_printf("<cd:Available>");
                            generate_expr(field->available_expr, 15);
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
                        generate_expr(field->bit_offset_expr, 15);
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
                        generate_expr(array_type->dim_expr[i], 15);
                        ff_printf("</cd:Dimension>\n");
                    }
                    else
                    {
                        fic_printf(&is_compound, "<cd:Dimension/>\n");
                    }
                }
                generate_type(array_type->base_type, xmlname, format);
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
                /* TODO: Add 'Mappings' */
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
                /* TODO: Add 'Special Text Type' */
                /* TODO: Add 'Mappings' */
            }
            break;
        case coda_raw_class:
            {
                coda_type_raw *raw_type = (coda_type_raw *)type;

                if (raw_type->fixed_value != NULL)
                {
                    fic_printf(&is_compound, "<cd:FixedValue>");
                    generate_escaped_string(raw_type->fixed_value, raw_type->fixed_value_length);
                    ff_printf("</cd:NativeType>\n");
                }
                /* TODO: Add 'Special Text Type' */
                /* TODO: Add 'Mappings' */
            }
            break;
        case coda_special_class:
            {
                /* TODO: Add 'Time' attributes */
                if (special_type != coda_special_time)
                {
                    generate_type(((coda_type_special *)type)->base_type, NULL, format);
                }
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
    if (wrapped_type)
    {
        INDENT--;
        fi_printf("</cd:Type>\n");
    }
}

static void generate_product_variable(const coda_product_variable *variable)
{
    fi_printf("<cd:ProductVariable name=\"%s\">\n", variable->name);
    INDENT++;
    if (variable->size_expr)
    {
        fi_printf("<cd:Dimension>");
        generate_expr(variable->size_expr, 15);
        ff_printf("</cd:Dimension>\n");
    }
    fi_printf("<cd:Init>");
    generate_expr(variable->init_expr, 15);
    ff_printf("</cd:Init>\n");
    INDENT--;
    fi_printf("</cd:ProductVariable>\n");
}

static void generate_product_definition(const coda_product *pf)
{
    const coda_product_definition *definition = pf->product_definition;
    coda_type *type = coda_get_type_for_dynamic_type(pf->root_type);
    coda_format format = pf->format;
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
    t = time(NULL);
    ttm = localtime(&t);
    if (ttm)
    {
        sprintf(currentdate, "%04d-%02d-%02d", ttm->tm_year + 1900, ttm->tm_mon + 1, ttm->tm_mday);
    }
    fi_printf("<cd:ProductDefinition id=\"%s\" format=\"%s\" last-modified=\"%s\" "
              "xmlns:cd=\"http://www.stcorp.nl/coda/definition/2008/07\">\n", name,
              coda_type_get_format_name(pf->format), currentdate);
    INDENT++;
    generate_type(type, NULL, format);
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
    coda_product *pf;
    int result;

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

    result = coda_open(file_name, &pf);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(file_name, &pf);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        fflush(stderr);
        exit(1);
    }

    fi_printf("<?xml version=\"1.0\"?>\n");
    generate_product_definition(pf);

    coda_close(pf);
}
