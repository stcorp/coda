/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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
#include "coda-expr.h"
#include "coda-type.h"

coda_type *typestack[CODA_CURSOR_MAXDEPTH];
long indexstack[CODA_CURSOR_MAXDEPTH + 1];

extern char *ascii_col_sep;
extern int show_type;
extern int show_unit;
extern int show_format;
extern int show_description;
extern int show_quotes;
extern int show_hidden;
extern int show_expressions;
extern int show_parent_types;
extern int show_attributes;
extern int use_special_types;


static void generate_escaped_string(const char *str, int length)
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
            printf("abs(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_add:
            if (precedence < 4)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 4);
            printf(" + ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                printf(")");
            }
            break;
        case expr_array_add:
            printf("add(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_array_all:
            printf("all(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_and:
            if (precedence < 7)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 7);
            printf(" & ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 7);
            if (precedence < 7)
            {
                printf(")");
            }
            break;
        case expr_ceil:
            printf("ceil(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_array_count:
            printf("count(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_array_exists:
            printf("exists(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_array_index:
            printf("index(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_asciiline:
            printf("asciiline");
            break;
        case expr_bit_offset:
            printf("bitoffset(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_bit_size:
            printf("bitsize(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_byte_offset:
            printf("byteoffset(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_byte_size:
            printf("bytesize(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_bytes:
            printf("bytes(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                printf(",");
                generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            }
            printf(")");
            break;
        case expr_constant_boolean:
            if (((coda_expression_bool_constant *)expr)->value)
            {
                printf("true");
            }
            else
            {
                printf("false");
            }
            break;
        case expr_constant_float:
            printf("%f", ((coda_expression_float_constant *)expr)->value);
            break;
        case expr_constant_integer:
            {
                char s[21];

                coda_str64(((coda_expression_integer_constant *)expr)->value, s);
                printf("%s", s);
            }
            break;
        case expr_constant_rawstring:
            {
                int i;

                printf("\"");
                for (i = 0; i < ((coda_expression_string_constant *)expr)->length; i++)
                {
                    printf("%c", ((coda_expression_string_constant *)expr)->value[i]);
                }
                printf("\"");
            }
            break;
        case expr_constant_string:
            printf("\"");
            generate_escaped_string(((coda_expression_string_constant *)expr)->value,
                                    ((coda_expression_string_constant *)expr)->length);
            printf("\"");
            break;
        case expr_divide:
            if (precedence < 3)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            printf(" / ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                printf(")");
            }
            break;
        case expr_equal:
            if (precedence < 6)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 6);
            printf(" == ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                printf(")");
            }
            break;
        case expr_exists:
            printf("exists(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_file_size:
            printf("filesize()");
            break;
        case expr_filename:
            printf("filename()");
            break;
        case expr_float:
            printf("float(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_floor:
            printf("floor(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_for_index:
            printf("i");
            break;
        case expr_for:
            printf("for i = ");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(" to ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                printf(" step ");
                generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            }
            printf(" do ");
            generate_expr(((coda_expression_operation *)expr)->operand[3], 15);
            break;
        case expr_goto_array_element:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            }
            printf("[");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf("]");
            break;
        case expr_goto_attribute:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            }
            printf("@%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_begin:
            printf(":");
            break;
        case expr_goto_field:
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[0]->tag != expr_goto_root)
            {
                printf("/");
            }
            printf("%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_here:
            printf(".");
            break;
        case expr_goto_parent:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                printf("/");
            }
            printf("..");
            break;
        case expr_goto_root:
            printf("/");
            break;
        case expr_goto:
            printf("goto(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_greater_equal:
            if (precedence < 5)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            printf(" >= ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                printf(")");
            }
            break;
        case expr_greater:
            if (precedence < 5)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            printf(" > ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                printf(")");
            }
            break;
        case expr_if:
            printf("if(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            printf(")");
            break;
        case expr_index:
            printf("index(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_integer:
            printf("int(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_isinf:
            printf("isinf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_ismininf:
            printf("ismininf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_isnan:
            printf("isnan(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_isplusinf:
            printf("isplusinf(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_length:
            printf("length(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_less_equal:
            if (precedence < 5)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            printf(" <= ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                printf(")");
            }
            break;
        case expr_less:
            if (precedence < 5)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 5);
            printf(" < ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 5);
            if (precedence < 5)
            {
                printf(")");
            }
            break;
        case expr_logical_and:
            if (precedence < 9)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 9);
            printf(" and ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 9);
            if (precedence < 9)
            {
                printf(")");
            }
            break;
        case expr_logical_or:
            if (precedence < 10)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 10);
            printf(" or ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 10);
            if (precedence < 10)
            {
                printf(")");
            }
            break;
        case expr_ltrim:
            printf("ltrim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_max:
            printf("max(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_min:
            printf("min(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_modulo:
            if (precedence < 3)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            printf(" %% ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                printf(")");
            }
            break;
        case expr_multiply:
            if (precedence < 3)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 3);
            printf(" * ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 3);
            if (precedence < 3)
            {
                printf(")");
            }
            break;
        case expr_neg:
            printf("-");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 1);
            break;
        case expr_not_equal:
            if (precedence < 6)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 6);
            printf(" != ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 6);
            if (precedence < 6)
            {
                printf(")");
            }
            break;
        case expr_not:
            printf("!");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 1);
            break;
        case expr_num_elements:
            printf("numelements(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_or:
            if (precedence < 7)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 7);
            printf(" | ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 7);
            if (precedence < 7)
            {
                printf(")");
            }
            break;
        case expr_power:
            if (precedence < 2)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 2);
            printf(" ^ ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 2);
            if (precedence < 2)
            {
                printf(")");
            }
            break;
        case expr_product_class:
            printf("productclass()");
            break;
        case expr_product_format:
            printf("productformat()");
            break;
        case expr_product_type:
            printf("producttype()");
            break;
        case expr_product_version:
            printf("productversion()");
            break;
        case expr_regex:
            printf("regex(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                printf(", ");
                generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            }
            printf(")");
            break;
        case expr_round:
            printf("round(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_rtrim:
            printf("rtrim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_sequence:
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf("; ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            break;
        case expr_string:
            printf("string(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                printf(", ");
                generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            }
            printf(")");
            break;
        case expr_substr:
            printf("substr(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[2], 15);
            printf(")");
            break;
        case expr_subtract:
            if (precedence < 4)
            {
                printf("(");
            }
            generate_expr(((coda_expression_operation *)expr)->operand[0], 4);
            printf(" - ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 4);
            if (precedence < 4)
            {
                printf(")");
            }
            break;
        case expr_trim:
            printf("trim(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_unbound_array_index:
            printf("unboundindex(");
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(", ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            printf(")");
            break;
        case expr_variable_exists:
            printf("exists($%s, ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_variable_index:
            printf("index($%s, ", ((coda_expression_operation *)expr)->identifier);
            generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
            printf(")");
            break;
        case expr_variable_set:
            printf("$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                printf("[");
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                printf("]");
            }
            printf(" = ");
            generate_expr(((coda_expression_operation *)expr)->operand[1], 15);
            break;
        case expr_variable_value:
            printf("$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                printf("[");
                generate_expr(((coda_expression_operation *)expr)->operand[0], 15);
                printf("]");
            }
            break;
    }
}

static void print_path(int depth)
{
    int i;

    printf("/");
    for (i = 0; i < depth; i++)
    {
        if (indexstack[i + 1] == -1)
        {
            printf("@");
        }
        else
        {
            coda_type *type = typestack[i];

            switch (type->type_class)
            {
                case coda_record_class:
                    {
                        const char *fieldname;

                        coda_type_get_record_field_name(type, indexstack[i + 1], &fieldname);
                        if (i > 0)
                        {
                            printf("/");
                        }
                        printf("%s", fieldname);
                    }
                    break;
                case coda_array_class:
                    {
                        long dim[CODA_MAX_NUM_DIMS];
                        int num_dims;
                        int j;

                        coda_type_get_array_dim(type, &num_dims, dim);
                        printf("[");
                        for (j = 0; j < num_dims; j++)
                        {
                            if (j > 0)
                            {
                                printf(",");
                            }
                            if (dim[j] < 0)
                            {
                                if (show_expressions && ((coda_type_array *)type)->dim_expr[j] != NULL)
                                {
                                    generate_expr(((coda_type_array *)type)->dim_expr[j], 15);
                                }
                                else
                                {
                                    printf("?");
                                }
                            }
                            else
                            {
                                printf("%ld", dim[j]);
                            }
                        }
                        printf("]");
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }
}

static void print_type(coda_type *type, int depth)
{
    coda_type_class type_class;
    int print_details = 0;

    if (depth >= CODA_CURSOR_MAXDEPTH)
    {
        printf("\n  ERROR: depth in type hierarchy (%d) exceeds maximum allowed depth (%d)\n", depth,
               CODA_CURSOR_MAXDEPTH);
        exit(1);
    }

    typestack[depth] = type;

    coda_type_get_class(type, &type_class);
    if (type_class == coda_record_class || type_class == coda_array_class)
    {
        print_details = show_parent_types;
    }
    else if (type_class == coda_special_class)
    {
        print_details = use_special_types;
    }
    else
    {
        print_details = 1;
    }

    if (print_details)
    {
        print_path(depth);
        if (show_type)
        {
            coda_native_type read_type;

            coda_type_get_read_type(type, &read_type);
            printf("%s%s", ascii_col_sep, coda_type_get_native_type_name(read_type));
        }
        if (show_format)
        {
            coda_format format;

            coda_type_get_format(type, &format);
            printf("%s%s", ascii_col_sep, coda_type_get_format_name(format));
        }
        if (show_unit)
        {
            const char *unit;

            printf("%s", ascii_col_sep);
            coda_type_get_unit(type, &unit);
            if (unit != NULL)
            {
                if (show_quotes)
                {
                    printf("\"");
                }
                printf("%s", unit);
                if (show_quotes)
                {
                    printf("\"");
                }
            }
        }
        if (show_description)
        {
            const char *description;

            printf("%s", ascii_col_sep);
            coda_type_get_description(type, &description);
            if (description != NULL)
            {
                if (show_quotes)
                {
                    printf("\"");
                }
                printf("%s", description);
                if (show_quotes)
                {
                    printf("\"");
                }
            }
        }
        printf("\n");
    }

    if (show_attributes)
    {
        coda_type *attributes;

        coda_type_get_attributes(type, &attributes);
        indexstack[depth + 1] = -1;
        print_type(attributes, depth + 1);
    }

    switch (type_class)
    {
        case coda_record_class:
            {
                long num_record_fields;
                long i;

                coda_type_get_num_record_fields(type, &num_record_fields);
                for (i = 0; i < num_record_fields; i++)
                {
                    coda_type *field_type;

                    coda_type_get_record_field_type(type, i, &field_type);

                    if (!show_hidden)
                    {
                        int hidden;

                        coda_type_get_record_field_hidden_status(type, i, &hidden);
                        if (hidden)
                        {
                            continue;
                        }
                    }
                    indexstack[depth + 1] = i;
                    print_type(field_type, depth + 1);
                }
            }
            break;
        case coda_array_class:
            {
                coda_type *base_type;

                coda_type_get_array_base_type(type, &base_type);
                indexstack[depth + 1] = 0;
                print_type(base_type, depth + 1);
            }
            break;
        case coda_special_class:
            if (!use_special_types)
            {
                coda_type *base_type;

                coda_type_get_special_base_type(type, &base_type);
                print_type(base_type, depth);

                break;
            }
            /* otherwise fall through to default */
        default:
            break;
    }
}

static void generate_field_list(const char *product_class_name, const char *product_type_name, int version)
{
    coda_product_class *product_class;
    coda_product_type *product_type;
    coda_product_definition *product_definition;

    product_class = coda_data_dictionary_get_product_class(product_class_name);
    if (product_class == NULL)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    product_type = coda_product_class_get_product_type(product_class, product_type_name);
    if (product_type == NULL)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    product_definition = coda_product_type_get_product_definition_by_version(product_type, version);
    if (product_definition == NULL)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (product_definition->root_type != NULL)
    {
        print_type(product_definition->root_type, 0);
    }
}


static void generate_product_list(const char *product_class_name, const char *product_type_name)
{
    int i;

    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_product_class *product_class = coda_global_data_dictionary->product_class[i];
        int j;

        if (product_class_name != NULL && strcmp(product_class->name, product_class_name) != 0)
        {
            continue;
        }
        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_product_type *product_type = product_class->product_type[j];

            if (product_type_name != NULL && strcmp(product_type->name, product_type_name) != 0)
            {
                continue;
            }
            if (product_type->num_product_definitions > 0)
            {
                int k;

                for (k = 0; k < product_type->num_product_definitions; k++)
                {
                    coda_product_definition *product_definition = product_type->product_definition[k];

                    printf("%s%s%s%s%d", product_class->name, ascii_col_sep, product_type->name, ascii_col_sep,
                           product_definition->version);
                    if (show_format)
                    {
                        printf("%s%s", ascii_col_sep, coda_type_get_format_name(product_definition->format));
                    }
                    if (show_description)
                    {
                        printf("%s", ascii_col_sep);
                        if (product_definition->description != NULL)
                        {
                            if (show_quotes)
                            {
                                printf("\"");
                            }
                            printf("%s", product_definition->description);
                            if (show_quotes)
                            {
                                printf("\"");
                            }
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}

void generate_list(const char *product_class, const char *product_type, int version)
{
    if (version < 0)
    {
        generate_product_list(product_class, product_type);
    }
    else
    {
        generate_field_list(product_class, product_type, version);
    }
}
