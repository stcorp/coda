/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-expr.h"

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "coda-ascii.h"

#include "pcre.h"

/** \file */

/** \defgroup coda_expression CODA Expression
 * CODA comes with a powerful expression language that can be used to perform calculations based on product data.
 * This expression system is used internally with the product format definition (codadef) files that CODA uses to
 * interpret products, but it can also be used by you as a user for your own purposes.
 * More information on the CODA expression language and its ascii syntax can be found in the CODA documentation.
 *
 * The example below shows how to evaluate a simple integer expression that does not make use of any product data:
 * \code
 * const char *equation = "1+2";
 * coda_expression *expr;
 * long result;
 *
 * coda_expression_from_string(equation, &expr);
 * coda_expression_eval_integer(expr, NULL, &result);
 * coda_expression_delete(expr);
 * printf("%d\n", result);
 * \endcode
 *
 * However, in most cases you will want to run an expression on actual product data. In the example below the expression
 * expects a cursor that points to a record which has two fields, 'numerator' and 'denominator', and it will return a
 * floating point value with the division of those two field values.
 * \code
 * const char *equation = "float(./numerator)/float(./denominator)";
 * coda_cursor cursor;
 * coda_expression *expr;
 * double result;
 *
 * coda_expression_from_string(equation, &expr);
 *
 * ... loop over all cursors for which you want to calculate the division ....
 * coda_expression_eval_integer(expr, &cursor, &result);
 * printf("%f\n", result);
 * ... end of loop ...
 *
 * coda_expression_delete(expr);
 * \endcode
 *
 * Note that, unlike most other CODA functions, the coda_expression_from_string() and coda_expression_delete() functions
 * do not require that CODA is initialised with coda_init(). This also holds for the coda_expression_eval functions if
 * no cursor is provided as parameter (i.e. when a static evaluation of the expression is performed).
 */

/** \typedef coda_expression
 * CODA Expression
 * \ingroup coda_expression
 */

/** \enum coda_expression_type_enum
 * Result types of CODA expressions.
 * \ingroup coda_expression
 */

/** \typedef coda_expression_type
 * Result types of CODA expressions.
 * \ingroup coda_expression
 */


#define REGEX_MAX_NUM_SUBSTRING 15

static int iswhitespace(char a)
{
    return (a == ' ' || a == '\t' || a == '\n' || a == '\r');
}

/* Gives a ^ b where both a and b are integers */
static int64_t ipow(int64_t a, int64_t b)
{
    int64_t r = 1;

    while (b--)
    {
        r *= a;
    }
    return r;
}

static long decode_escaped_string(char *str)
{
    long from;
    long to;

    if (str == NULL)
    {
        return 0;
    }

    from = 0;
    to = 0;

    while (str[from] != '\0')
    {
        if (str[from] == '\\')
        {
            from++;
            switch (str[from])
            {
                case 'e':
                    str[to++] = '\033'; /* windows does not recognize '\e' */
                    break;
                case 'a':
                    str[to++] = '\a';
                    break;
                case 'b':
                    str[to++] = '\b';
                    break;
                case 'f':
                    str[to++] = '\f';
                    break;
                case 'n':
                    str[to++] = '\n';
                    break;
                case 'r':
                    str[to++] = '\r';
                    break;
                case 't':
                    str[to++] = '\t';
                    break;
                case 'v':
                    str[to++] = '\v';
                    break;
                case '\\':
                    str[to++] = '\\';
                    break;
                case '"':
                    str[to++] = '"';
                    break;
                case '\'':
                    str[to++] = '\'';
                    break;
                default:
                    if (str[from] < '0' || str[from] > '9')
                    {
                        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid escape sequence in string");
                        return -1;
                    }
                    str[to] = (str[from] - '0') * 64;
                    from++;
                    if (str[from] < '0' || str[from] > '9')
                    {
                        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid escape sequence in string");
                        return -1;
                    }
                    str[to] += (str[from] - '0') * 8;
                    from++;
                    if (str[from] < '0' || str[from] > '9')
                    {
                        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid escape sequence in string");
                        return -1;
                    }
                    str[to] += str[from] - '0';
                    to++;
            }
        }
        else
        {
            str[to++] = str[from];
        }
        from++;
    }

    str[to] = '\0';

    return to;
}

static coda_expression *boolean_constant_new(char *string_value)
{
    coda_expression_bool_constant *expr;

    expr = malloc(sizeof(coda_expression_bool_constant));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_bool_constant), __FILE__, __LINE__);
        free(string_value);
        return NULL;
    }
    expr->tag = expr_constant_boolean;
    expr->result_type = coda_expression_boolean;
    expr->is_constant = 1;
    expr->value = (*string_value == 't' || *string_value == 'T');
    free(string_value);

    return (coda_expression *)expr;
}

static coda_expression *float_constant_new(char *string_value)
{
    coda_expression_float_constant *expr;
    double value;

    if (coda_ascii_parse_double(string_value, strlen(string_value), &value, 0) < 0)
    {
        free(string_value);
        return NULL;
    }
    free(string_value);

    expr = malloc(sizeof(coda_expression_float_constant));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_float_constant), __FILE__, __LINE__);
        return NULL;
    }
    expr->tag = expr_constant_float;
    expr->result_type = coda_expression_float;
    expr->is_constant = 1;
    expr->value = value;

    return (coda_expression *)expr;
}

static coda_expression *integer_constant_new(char *string_value)
{
    coda_expression_integer_constant *expr;
    int64_t value;

    if (coda_ascii_parse_int64(string_value, strlen(string_value), &value, 0) < 0)
    {
        free(string_value);
        return NULL;
    }
    free(string_value);

    expr = malloc(sizeof(coda_expression_integer_constant));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_integer_constant), __FILE__, __LINE__);
        return NULL;
    }
    expr->tag = expr_constant_integer;
    expr->result_type = coda_expression_integer;
    expr->is_constant = 1;
    expr->value = value;

    return (coda_expression *)expr;
}

static coda_expression *rawstring_constant_new(char *string_value)
{
    coda_expression_string_constant *expr;

    expr = malloc(sizeof(coda_expression_string_constant));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_string_constant), __FILE__, __LINE__);
        return NULL;
    }
    expr->tag = expr_constant_rawstring;
    expr->result_type = coda_expression_string;
    expr->is_constant = 1;
    expr->length = strlen(string_value);
    expr->value = string_value;

    return (coda_expression *)expr;
}

static coda_expression *string_constant_new(char *string_value)
{
    coda_expression_string_constant *expr;
    long length;

    length = decode_escaped_string(string_value);
    if (length < 0)
    {
        free(string_value);
        return NULL;
    }

    expr = malloc(sizeof(coda_expression_string_constant));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_string_constant), __FILE__, __LINE__);
        return NULL;
    }
    expr->tag = expr_constant_string;
    expr->result_type = coda_expression_string;
    expr->is_constant = 1;
    expr->length = length;
    expr->value = string_value;

    return (coda_expression *)expr;
}

coda_expression *coda_expression_new(coda_expression_node_type tag, char *string_value, coda_expression *op1,
                                     coda_expression *op2, coda_expression *op3, coda_expression *op4)
{
    coda_expression_operation *expr;

    switch (tag)
    {
        case expr_constant_boolean:
            return boolean_constant_new(string_value);
        case expr_constant_float:
            return float_constant_new(string_value);
        case expr_constant_integer:
            return integer_constant_new(string_value);
        case expr_constant_rawstring:
            return rawstring_constant_new(string_value);
        case expr_constant_string:
            return string_constant_new(string_value);
        default:
            break;
    }

    expr = malloc(sizeof(coda_expression_operation));
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_expression_operation), __FILE__, __LINE__);
        if (string_value != NULL)
        {
            free(string_value);
        }
        if (op1 != NULL)
        {
            coda_expression_delete(op1);
        }
        if (op2 != NULL)
        {
            coda_expression_delete(op2);
        }
        if (op3 != NULL)
        {
            coda_expression_delete(op3);
        }
        if (op4 != NULL)
        {
            coda_expression_delete(op4);
        }
        return NULL;
    }
    expr->tag = tag;
    expr->identifier = string_value;
    expr->operand[0] = op1;
    expr->operand[1] = op2;
    expr->operand[2] = op3;
    expr->operand[3] = op4;

    switch (tag)
    {
        case expr_array_all:
        case expr_array_exists:
        case expr_equal:
        case expr_exists:
        case expr_greater_equal:
        case expr_greater:
        case expr_isinf:
        case expr_ismininf:
        case expr_isnan:
        case expr_isplusinf:
        case expr_less_equal:
        case expr_less:
        case expr_logical_and:
        case expr_logical_or:
        case expr_not_equal:
        case expr_not:
        case expr_variable_exists:
            expr->result_type = coda_expression_boolean;
            break;
        case expr_ceil:
        case expr_float:
        case expr_floor:
        case expr_round:
            expr->result_type = coda_expression_float;
            break;
        case expr_and:
        case expr_array_count:
        case expr_array_index:
        case expr_bit_offset:
        case expr_bit_size:
        case expr_byte_offset:
        case expr_byte_size:
        case expr_file_size:
        case expr_for_index:
        case expr_index:
        case expr_integer:
        case expr_length:
        case expr_num_elements:
        case expr_or:
        case expr_product_version:
        case expr_unbound_array_index:
        case expr_variable_index:
        case expr_variable_value:
            expr->result_type = coda_expression_integer;
            break;
        case expr_bytes:
        case expr_filename:
        case expr_ltrim:
        case expr_product_class:
        case expr_product_type:
        case expr_rtrim:
        case expr_string:
        case expr_substr:
        case expr_trim:
            expr->result_type = coda_expression_string;
            break;
        case expr_for:
        case expr_goto:
        case expr_sequence:
        case expr_variable_set:
            expr->result_type = coda_expression_void;
            break;
        case expr_asciiline:
        case expr_goto_array_element:
        case expr_goto_attribute:
        case expr_goto_begin:
        case expr_goto_field:
        case expr_goto_here:
        case expr_goto_parent:
        case expr_goto_root:
            expr->result_type = coda_expression_node;
            break;
        case expr_abs:
        case expr_neg:
            expr->result_type = op1->result_type;
            break;
        case expr_add:
        case expr_divide:
        case expr_max:
        case expr_min:
        case expr_modulo:
        case expr_multiply:
        case expr_power:
        case expr_subtract:
            if (op1->result_type == coda_expression_float || op2->result_type == coda_expression_float)
            {
                /* allow one of the arguments to be an integer */
                expr->result_type = coda_expression_float;
            }
            else
            {
                expr->result_type = op1->result_type;
            }
            break;
        case expr_array_add:
        case expr_if:
            expr->result_type = op2->result_type;
            break;
        case expr_regex:
            if (op3 == NULL)
            {
                expr->result_type = coda_expression_boolean;
            }
            else
            {
                expr->result_type = coda_expression_string;
            }
            break;
        case expr_constant_boolean:
        case expr_constant_float:
        case expr_constant_integer:
        case expr_constant_rawstring:
        case expr_constant_string:
            assert(0);
            exit(1);
    }

    switch (expr->tag)
    {
        case expr_file_size:
        case expr_filename:
        case expr_product_class:
        case expr_product_type:
        case expr_product_version:
        case expr_variable_index:
        case expr_variable_set:
        case expr_variable_value:
            expr->is_constant = 0;
            break;
        default:
            expr->is_constant = expr->result_type != coda_expression_node && (op1 == NULL || op1->is_constant) &&
                (op2 == NULL || op2->is_constant) && (op3 == NULL || op3->is_constant) &&
                (op4 == NULL || op4->is_constant);
    }

    return (coda_expression *)expr;
}

typedef struct eval_info_struct
{
    const coda_cursor *orig_cursor;
    coda_cursor cursor;
    int64_t for_index;
    int64_t variable_index;
    const char *variable_name;
} eval_info;

/* eval status info
  - for index (only 1)
  - variable index (only 1)
  - name of variable
  - product handle
  - original cursor
  - current cursor
*/
static int eval_boolean(eval_info *info, const coda_expression *expr, int *value);
static int eval_float(eval_info *info, const coda_expression *expr, double *value);
static int eval_integer(eval_info *info, const coda_expression *expr, int64_t *value);
static int eval_string(eval_info *info, const coda_expression *expr, long *offset, long *length, char **value);
static int eval_cursor(eval_info *info, const coda_expression *expr);
static int eval_void(eval_info *info, const coda_expression *expr);

static int eval_boolean(eval_info *info, const coda_expression *expr, int *value)
{
    const coda_expression_operation *opexpr;

    if (expr->tag == expr_constant_boolean)
    {
        *value = ((coda_expression_bool_constant *)expr)->value;
        return 0;
    }

    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_equal:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a == b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a == b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                if (len_a != len_b)
                {
                    *value = 0;
                }
                else if (len_a == 0)
                {
                    *value = 1;
                }
                else
                {
                    *value = (memcmp(&a[off_a], &b[off_b], len_a) == 0);
                }
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_not_equal:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a != b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a != b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                if (len_a != len_b)
                {
                    *value = 1;
                }
                else if (len_a == 0)
                {
                    *value = 0;
                }
                else
                {
                    *value = (memcmp(&a[off_a], &b[off_b], len_a) != 0);
                }
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_greater:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a > b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a > b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                long index;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                index = 0;
                while (index < len_a && index < len_b && a[off_a + index] == b[off_b + index])
                {
                    index++;
                }
                *value = (index < len_a &&
                          (index == len_b || ((uint8_t)a[off_a + index]) > ((uint8_t)b[off_b + index])));
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_greater_equal:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a >= b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a >= b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                long index;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                index = 0;
                while (index < len_a && index < len_b && a[off_a + index] == b[off_b + index])
                {
                    index++;
                }
                *value = (index == len_b ||
                          (index < len_a && ((uint8_t)a[off_a + index]) > ((uint8_t)b[off_b + index])));
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_less:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a < b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a < b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                long index;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                index = 0;
                while (index < len_a && index < len_b && a[off_a + index] == b[off_b + index])
                {
                    index++;
                }
                *value = (index < len_b &&
                          (index == len_a || ((uint8_t)a[off_a + index]) < ((uint8_t)b[off_b + index])));
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_less_equal:
            if (opexpr->operand[0]->result_type == coda_expression_float ||
                opexpr->operand[1]->result_type == coda_expression_float)
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a <= b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_integer)
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a <= b);
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long off_a, off_b;
                long len_a, len_b;
                long index;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                index = 0;
                while (index < len_a && index < len_b && a[off_a + index] == b[off_b + index])
                {
                    index++;
                }
                *value = (index == len_a ||
                          (index < len_b && ((uint8_t)a[off_a + index]) < ((uint8_t)b[off_b + index])));
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            else
            {
                assert(0);
                exit(1);
            }
            break;
        case expr_not:
            if (eval_boolean(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = !(*value);
            break;
        case expr_logical_and:
            if (eval_boolean(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            if (*value == 0)
            {
                return 0;
            }
            if (eval_boolean(info, opexpr->operand[1], value) != 0)
            {
                return -1;
            }
            break;
        case expr_logical_or:
            if (eval_boolean(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            if (*value != 0)
            {
                return 0;
            }
            if (eval_boolean(info, opexpr->operand[1], value) != 0)
            {
                return -1;
            }
            break;
        case expr_isnan:
            {
                double a;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                *value = coda_isNaN(a);
            }
            break;
        case expr_isinf:
            {
                double a;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                *value = coda_isInf(a);
            }
            break;
        case expr_isplusinf:
            {
                double a;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                *value = coda_isPlusInf(a);
            }
            break;
        case expr_ismininf:
            {
                double a;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                *value = coda_isMinInf(a);
            }
            break;
        case expr_regex:
            {
                int ovector[(REGEX_MAX_NUM_SUBSTRING + 1) * 3];
                const char *error;
                int erroffset;
                long matchstring_offset;
                long matchstring_length;
                char *matchstring;
                long pattern_offset;
                long pattern_length;
                char *pattern;
                pcre *re;
                int rc;

                if (eval_string(info, opexpr->operand[0], &pattern_offset, &pattern_length, &pattern) != 0)
                {
                    return -1;
                }
                /* pattern needs to be a 0-terminated string and can not be NULL */
                {
                    char *new_pattern;

                    new_pattern = malloc(pattern_length + 1);
                    if (new_pattern == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       (pattern_length + 1), __FILE__, __LINE__);
                        if (pattern != NULL)
                        {
                            free(pattern);
                        }
                        return -1;
                    }
                    if (pattern != NULL)
                    {
                        memcpy(new_pattern, &pattern[pattern_offset], pattern_length);
                        free(pattern);
                    }
                    new_pattern[pattern_length] = '\0';
                    pattern = new_pattern;
                }
                if (eval_string(info, opexpr->operand[1], &matchstring_offset, &matchstring_length, &matchstring) != 0)
                {
                    free(pattern);
                    return -1;
                }

                re = pcre_compile(pattern, PCRE_DOTALL | PCRE_DOLLAR_ENDONLY, &error, &erroffset, NULL);
                if (re == NULL)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION,
                                   "invalid format for regex pattern ('%s' at position %d)", error, erroffset);
                    free(pattern);
                    return -1;
                }
                free(pattern);

                if (matchstring == NULL)
                {
                    /* pcre_exec does not except NULL for an empty matchstring */
                    matchstring = strdup("");
                    if (matchstring == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                       __FILE__, __LINE__);
                        pcre_free(re);
                        return -1;
                    }
                }

                rc = pcre_exec(re, NULL, &matchstring[matchstring_offset], matchstring_length, 0, 0, ovector,
                               (REGEX_MAX_NUM_SUBSTRING + 1) * 3);
                free(matchstring);
                pcre_free(re);
                if (rc < 0 && rc != PCRE_ERROR_NOMATCH)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate regex pattern (error code %d)", rc);
                    return -1;
                }
                if (rc == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "regex pattern contains too many subexpressions");
                    return -1;
                }
                *value = (rc > 0);
            }
            break;
        case expr_exists:
            {
                coda_cursor prev_cursor;
                coda_type_class type_class;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    if (coda_errno != CODA_ERROR_EXPRESSION)
                    {
                        /* could not access path */
                        coda_errno = 0;
                        *value = 0;
                        info->cursor = prev_cursor;
                        return 0;
                    }
                    return -1;
                }
                if (coda_cursor_get_type_class(&info->cursor, &type_class) != 0)
                {
                    return -1;
                }
                if (type_class == coda_special_class)
                {
                    coda_special_type special_type;

                    if (coda_cursor_get_special_type(&info->cursor, &special_type) != 0)
                    {
                        return -1;
                    }
                    if (special_type == coda_special_no_data)
                    {
                        *value = 0;
                        info->cursor = prev_cursor;
                        return 0;
                    }
                }
                *value = 1;
                info->cursor = prev_cursor;
            }
            break;
        case expr_array_all:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        int condition;

                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            return -1;
                        }
                        if (!condition)
                        {
                            *value = 0;
                            return 0;
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                *value = 1;
                info->cursor = prev_cursor;
            }
            break;
        case expr_array_exists:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        int condition;

                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            return -1;
                        }
                        if (condition)
                        {
                            *value = 1;
                            return 0;
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                *value = 0;
                info->cursor = prev_cursor;
            }
            break;
        case expr_variable_exists:
            {
                long size;
                long i;

                assert(info->orig_cursor != NULL);
                if (info->variable_name != NULL)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot perform search within search for product variables");
                    return -1;
                }

                if (coda_product_variable_get_size(info->orig_cursor->product, opexpr->identifier, &size) != 0)
                {
                    return -1;
                }
                info->variable_name = opexpr->identifier;
                for (i = 0; i < size; i++)
                {
                    int condition;

                    info->variable_index = i;
                    if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                    {
                        return -1;
                    }
                    if (condition)
                    {
                        *value = 1;
                        info->variable_name = NULL;
                        return 0;
                    }
                }
                *value = 0;
                info->variable_name = NULL;
            }
            break;
        case expr_if:
            {
                int condition;

                if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                {
                    return -1;
                }
                if (condition)
                {
                    if (eval_boolean(info, opexpr->operand[1], value) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (eval_boolean(info, opexpr->operand[2], value) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int eval_float(eval_info *info, const coda_expression *expr, double *value)
{
    const coda_expression_operation *opexpr;

    /* we allow auto conversion of integer to double */
    if (expr->result_type == coda_expression_integer)
    {
        int64_t intvalue;

        if (eval_integer(info, expr, &intvalue) != 0)
        {
            return -1;
        }
        *value = (double)intvalue;
        return 0;
    }

    if (expr->tag == expr_constant_float)
    {
        *value = ((coda_expression_float_constant *)expr)->value;
        return 0;
    }

    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_float:
            if (opexpr->operand[0]->result_type == coda_expression_node)
            {
                coda_cursor prev_cursor;
                int perform_conversions;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                perform_conversions = coda_get_option_perform_conversions();
                coda_set_option_perform_conversions(0);
                if (coda_cursor_read_double(&info->cursor, value) != 0)
                {
                    coda_set_option_perform_conversions(perform_conversions);
                    return -1;
                }
                coda_set_option_perform_conversions(perform_conversions);
                info->cursor = prev_cursor;
            }
            else if (opexpr->operand[0]->result_type == coda_expression_string)
            {
                long offset;
                long length;
                char *str;

                if (eval_string(info, opexpr->operand[0], &offset, &length, &str) != 0)
                {
                    return -1;
                }
                if (length == 0)
                {
                    coda_set_error(CODA_ERROR_INVALID_FORMAT,
                                   "invalid format for ascii floating point value (no digits)");
                    return -1;
                }
                if (coda_ascii_parse_double(&str[offset], length, value, 0) < 0)
                {
                    free(str);
                    return -1;
                }
                free(str);
            }
            else
            {
                int64_t intvalue;

                if (eval_integer(info, opexpr->operand[0], &intvalue) != 0)
                {
                    return -1;
                }
                *value = (double)intvalue;
            }
            break;
        case expr_neg:
            if (eval_float(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = -(*value);
            break;
        case expr_abs:
            if (eval_float(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = ((*value) >= 0 ? *value : -(*value));
            break;
        case expr_ceil:
            if (eval_float(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = ceil(*value);
            break;
        case expr_floor:
            if (eval_float(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = floor(*value);
            break;
        case expr_round:
            if (eval_float(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            if (*value < 0)
            {
                *value = floor(*value - 0.5);
            }
            else
            {
                *value = floor(*value + 0.5);
            }
            break;
        case expr_add:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a + b;
            }
            break;
        case expr_subtract:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a - b;
            }
            break;
        case expr_multiply:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a * b;
            }
            break;
        case expr_divide:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                if (b == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "division by 0 in expression");
                    return -1;
                }
                *value = a / b;
            }
            break;
        case expr_modulo:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                if (b == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "modulo by 0 in expression");
                    return -1;
                }
                *value = fmod(a, b);
            }
            break;
        case expr_power:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = pow(a, b);
            }
            break;
        case expr_max:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a > b ? a : b);
            }
            break;
        case expr_min:
            {
                double a, b;

                if (eval_float(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a < b ? a : b);
            }
            break;
        case expr_if:
            {
                int condition;

                if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                {
                    return -1;
                }
                if (condition)
                {
                    if (eval_float(info, opexpr->operand[1], value) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (eval_float(info, opexpr->operand[2], value) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case expr_array_add:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                *value = 0;
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        double element_value;

                        if (eval_float(info, opexpr->operand[1], &element_value) != 0)
                        {
                            return -1;
                        }
                        *value += element_value;
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                info->cursor = prev_cursor;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int eval_integer(eval_info *info, const coda_expression *expr, int64_t *value)
{
    const coda_expression_operation *opexpr;

    if (expr->tag == expr_constant_integer)
    {
        *value = ((coda_expression_integer_constant *)expr)->value;
        return 0;
    }

    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_integer:
            if (opexpr->operand[0]->result_type == coda_expression_node)
            {
                coda_cursor prev_cursor;
                coda_native_type read_type;
                int perform_conversions;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                perform_conversions = coda_get_option_perform_conversions();
                coda_set_option_perform_conversions(0);
                if (coda_cursor_get_read_type(&info->cursor, &read_type) != 0)
                {
                    coda_set_option_perform_conversions(perform_conversions);
                    return -1;
                }
                if (read_type == coda_native_type_uint64)
                {
                    uint64_t uvalue;

                    /* read it as an uint64 and then cast it to a int64 */
                    if (coda_cursor_read_uint64(&info->cursor, &uvalue) != 0)
                    {
                        coda_set_option_perform_conversions(perform_conversions);
                        return -1;
                    }
                    *value = (int64_t)uvalue;
                }
                else
                {
                    if (coda_cursor_read_int64(&info->cursor, value) != 0)
                    {
                        coda_set_option_perform_conversions(perform_conversions);
                        return -1;
                    }
                }
                coda_set_option_perform_conversions(perform_conversions);
                info->cursor = prev_cursor;
            }
            else
            {
                long offset;
                long length;
                char *str;

                if (eval_string(info, opexpr->operand[0], &offset, &length, &str) != 0)
                {
                    return -1;
                }
                if (length == 0)
                {
                    coda_set_error(CODA_ERROR_INVALID_FORMAT,
                                   "invalid format for ascii floating point value (no digits)");
                    return -1;
                }
                if (coda_ascii_parse_int64(&str[offset], length, value, 0) < 0)
                {
                    free(str);
                    return -1;
                }
                free(str);
            }
            break;
        case expr_neg:
            if (eval_integer(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = -(*value);
            break;
        case expr_abs:
            if (eval_integer(info, opexpr->operand[0], value) != 0)
            {
                return -1;
            }
            *value = ((*value) >= 0 ? *value : -(*value));
            break;
        case expr_add:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a + b;
            }
            break;
        case expr_subtract:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a - b;
            }
            break;
        case expr_multiply:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a * b;
            }
            break;
        case expr_divide:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                if (b == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "division by 0 in expression");
                    return -1;
                }
                *value = a / b;
            }
            break;
        case expr_modulo:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                if (b == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "modulo by 0 in expression");
                    return -1;
                }
                *value = a % b;
            }
            break;
        case expr_power:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = ipow(a, b);
            }
            break;
        case expr_and:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a & b;
            }
            break;
        case expr_or:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = a | b;
            }
            break;
        case expr_max:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a > b ? a : b);
            }
            break;
        case expr_min:
            {
                int64_t a, b;

                if (eval_integer(info, opexpr->operand[0], &a) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &b) != 0)
                {
                    return -1;
                }
                *value = (a < b ? a : b);
            }
            break;
        case expr_if:
            {
                int condition;

                if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                {
                    return -1;
                }
                if (condition)
                {
                    if (eval_integer(info, opexpr->operand[1], value) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (eval_integer(info, opexpr->operand[2], value) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case expr_array_count:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                *value = 0;
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        int condition;

                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            return -1;
                        }
                        if (condition)
                        {
                            (*value)++;
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_array_add:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                *value = 0;
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        int64_t element_value;

                        if (eval_integer(info, opexpr->operand[1], &element_value) != 0)
                        {
                            return -1;
                        }
                        *value += element_value;
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_array_index:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                *value = 0;
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        int condition;

                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            return -1;
                        }
                        if (condition)
                        {
                            *value = i;
                            info->cursor = prev_cursor;
                            return 0;
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                *value = -1;
                info->cursor = prev_cursor;
            }
            break;
        case expr_unbound_array_index:
            {
                coda_cursor prev_cursor;
                int prev_option;
                int condition = 0;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                prev_option = coda_option_perform_boundary_checks;
                coda_option_perform_boundary_checks = 0;
                if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                {
                    coda_option_perform_boundary_checks = prev_option;
                    return -1;
                }
                *value = 0;
                while (!condition)
                {
                    if (opexpr->operand[2] != NULL)
                    {
                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            coda_option_perform_boundary_checks = prev_option;
                            return -1;
                        }
                    }
                    if (condition)
                    {
                        *value = -1;
                    }
                    else
                    {
                        if (eval_boolean(info, opexpr->operand[1], &condition) != 0)
                        {
                            coda_option_perform_boundary_checks = prev_option;
                            return -1;
                        }
                        if (!condition)
                        {
                            (*value)++;
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                coda_option_perform_boundary_checks = prev_option;
                                return -1;
                            }
                        }
                    }
                }
                coda_option_perform_boundary_checks = prev_option;
                info->cursor = prev_cursor;
            }
            break;
        case expr_length:
            if (opexpr->operand[0]->result_type == coda_expression_node)
            {
                coda_cursor prev_cursor;
                long length;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_string_length(&info->cursor, &length) != 0)
                {
                    return -1;
                }
                *value = length;
                info->cursor = prev_cursor;
            }
            else
            {
                long offset;
                long length;
                char *str;

                if (eval_string(info, opexpr->operand[0], &offset, &length, &str) != 0)
                {
                    return -1;
                }
                if (length > 0)
                {
                    free(str);
                }
                *value = length;
            }
            break;
        case expr_num_elements:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
                *value = num_elements;
            }
            break;
        case expr_bit_size:
            {
                coda_cursor prev_cursor;
                int use_fast_size_expression;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                use_fast_size_expression = coda_get_option_use_fast_size_expressions();
                coda_set_option_use_fast_size_expressions(0);
                if (coda_cursor_get_bit_size(&info->cursor, value) != 0)
                {
                    coda_set_option_use_fast_size_expressions(use_fast_size_expression);
                    return -1;
                }
                coda_set_option_use_fast_size_expressions(use_fast_size_expression);
                info->cursor = prev_cursor;
            }
            break;
        case expr_byte_size:
            {
                coda_cursor prev_cursor;
                int use_fast_size_expression;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                use_fast_size_expression = coda_get_option_use_fast_size_expressions();
                coda_set_option_use_fast_size_expressions(0);
                if (coda_cursor_get_byte_size(&info->cursor, value) != 0)
                {
                    coda_set_option_use_fast_size_expressions(use_fast_size_expression);
                    return -1;
                }
                coda_set_option_use_fast_size_expressions(use_fast_size_expression);
                info->cursor = prev_cursor;
            }
            break;
        case expr_bit_offset:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_file_bit_offset(&info->cursor, value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_byte_offset:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_file_byte_offset(&info->cursor, value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_file_size:
            assert(info->orig_cursor != NULL);
            if (coda_get_product_file_size(info->orig_cursor->product, value) != 0)
            {
                return -1;
            }
            break;
        case expr_product_version:
            {
                int version;

                assert(info->orig_cursor != NULL);
                if (coda_get_product_version(info->orig_cursor->product, &version) != 0)
                {
                    return -1;
                }
                *value = version;
            }
            break;
        case expr_index:
            {
                coda_cursor prev_cursor;
                long index;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_index(&info->cursor, &index) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
                *value = index;
            }
            break;
        case expr_variable_index:
            {
                long size;
                long i;

                assert(info->orig_cursor != NULL);
                if (info->variable_name != NULL)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot perform search within search for product variables");
                    return -1;
                }

                if (coda_product_variable_get_size(info->orig_cursor->product, opexpr->identifier, &size) != 0)
                {
                    return -1;
                }
                info->variable_name = opexpr->identifier;
                for (i = 0; i < size; i++)
                {
                    int condition;

                    info->variable_index = i;
                    if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                    {
                        return -1;
                    }
                    if (condition)
                    {
                        *value = i;
                        info->variable_name = NULL;
                        return 0;
                    }
                }
                *value = -1;
                info->variable_name = NULL;
            }
            break;
        case expr_variable_value:
            {
                int64_t *varptr;
                int64_t index = 0;

                assert(info->orig_cursor != NULL);
                if (opexpr->operand[0] != NULL)
                {
                    if (info->variable_name != NULL && strcmp(opexpr->identifier, info->variable_name) == 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION,
                                       "cannot use index on product variable '%s' when performing a search",
                                       opexpr->identifier);
                        return -1;
                    }
                    if (eval_integer(info, opexpr->operand[0], &index) != 0)
                    {
                        return -1;
                    }
                }
                else if (info->variable_name != NULL && strcmp(info->variable_name, opexpr->identifier) == 0)
                {
                    index = info->variable_index;
                }
                if (coda_product_variable_get_pointer(info->orig_cursor->product, opexpr->identifier, (long)index,
                                                      &varptr) != 0)
                {
                    return -1;
                }
                *value = *varptr;
            }
            break;
        case expr_for_index:
            *value = info->for_index;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int eval_string(eval_info *info, const coda_expression *expr, long *offset, long *length, char **value)
{
    const coda_expression_operation *opexpr;

    if (expr->tag == expr_constant_string || expr->tag == expr_constant_rawstring)
    {
        *offset = 0;
        *length = ((coda_expression_string_constant *)expr)->length;
        if (*length > 0)
        {
            *value = strdup(((coda_expression_string_constant *)expr)->value);
            if (*value == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                               __LINE__);
                return -1;
            }
        }
        else
        {
            *value = NULL;
        }
        return 0;
    }

    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_string:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                *offset = 0;
                if (coda_cursor_get_string_length(&info->cursor, length) != 0)
                {
                    return -1;
                }
                if (opexpr->operand[1] != NULL)
                {
                    int64_t maxlength;

                    if (eval_integer(info, opexpr->operand[1], &maxlength) != 0)
                    {
                        return -1;
                    }
                    if (*length > maxlength)
                    {
                        *length = (long)maxlength;
                    }
                }
                if (*length > 0)
                {
                    *value = malloc(*length + 1);
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length, __FILE__, __LINE__);
                        return -1;
                    }
                    if (coda_cursor_read_string(&info->cursor, *value, *length + 1) != 0)
                    {
                        free(*value);
                        return -1;
                    }
                }
                else
                {
                    *value = NULL;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_bytes:
            {
                coda_cursor prev_cursor;
                int64_t num_bytes;
                int64_t num_bits = -1;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (opexpr->operand[1] != NULL)
                {
                    if (eval_integer(info, opexpr->operand[1], &num_bytes) != 0)
                    {
                        return -1;
                    }
                    if (num_bytes > 0)
                    {
                        num_bits = num_bytes << 3;
                    }
                }
                else
                {
                    if (coda_cursor_get_bit_size(&info->cursor, &num_bits) != 0)
                    {
                        return -1;
                    }
                }
                if (num_bits < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "negative byte size in bytes expression");
                    return -1;
                }
                num_bytes = (num_bits >> 3) + (num_bits & 0x7 ? 1 : 0);
                *offset = 0;
                *length = (long)num_bytes;
                if (num_bytes > 0)
                {
                    *value = malloc((long)num_bytes + 1);       /* add room for zero termination at a later time */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       (long)num_bytes, __FILE__, __LINE__);
                        return -1;
                    }
                    if (coda_cursor_read_bits(&info->cursor, (uint8_t *)*value, 0, num_bits) != 0)
                    {
                        free(*value);
                        return -1;
                    }
                }
                else
                {
                    *value = NULL;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_add:
            {
                long off_a, off_b;
                long len_a, len_b;
                char *a;
                char *b;

                if (eval_string(info, opexpr->operand[0], &off_a, &len_a, &a) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &off_b, &len_b, &b) != 0)
                {
                    free(a);
                    return -1;
                }
                *offset = 0;
                *length = len_a + len_b;
                if (*length > 0)
                {
                    *value = malloc(*length + 1);       /* add room for zero termination at a later time */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length + 1, __FILE__, __LINE__);
                        return -1;
                    }
                    if (len_a > 0)
                    {
                        memcpy(*value, &a[off_a], len_a);
                    }
                    if (len_b > 0)
                    {
                        memcpy(&(*value)[len_a], &b[off_b], len_b);
                    }
                }
                else
                {
                    *value = NULL;
                }
                free(a);
                free(b);
            }
            break;
        case expr_substr:
            {
                int64_t new_offset;
                int64_t new_length;

                if (eval_integer(info, opexpr->operand[0], &new_offset) != 0)
                {
                    return -1;
                }
                if (new_offset < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "negative offset in substr expression");
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &new_length) != 0)
                {
                    return -1;
                }
                if (new_length == 0)
                {
                    *offset = 0;
                    *length = 0;
                    *value = NULL;
                    return 0;
                }
                if (new_length < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "negative length in substr expression");
                    return -1;
                }
                if (eval_string(info, opexpr->operand[2], offset, length, value) != 0)
                {
                    return -1;
                }
                if (*length == 0)
                {
                    return 0;
                }
                if (new_offset >= *length)
                {
                    *offset = 0;
                    *length = 0;
                    free(*value);
                    *value = NULL;
                    return 0;
                }
                *offset += (long)new_offset;
                *length -= (long)new_offset;
                if (new_length < *length)
                {
                    *length = (long)new_length;
                }
            }
            break;
        case expr_ltrim:
            {
                if (eval_string(info, opexpr->operand[0], offset, length, value) != 0)
                {
                    return -1;
                }
                while (*length > 0 && iswhitespace((*value)[*offset]))
                {
                    (*length)--;
                    (*offset)++;
                }
            }
            break;
        case expr_rtrim:
            {
                if (eval_string(info, opexpr->operand[0], offset, length, value) != 0)
                {
                    return -1;
                }
                while (*length > 0 && iswhitespace((*value)[*offset + *length - 1]))
                {
                    (*length)--;
                }
            }
            break;
        case expr_trim:
            {
                if (eval_string(info, opexpr->operand[0], offset, length, value) != 0)
                {
                    return -1;
                }
                while (*length > 0 && iswhitespace((*value)[*offset]))
                {
                    (*length)--;
                    (*offset)++;
                }
                while (*length > 0 && iswhitespace((*value)[*offset + *length - 1]))
                {
                    (*length)--;
                }
            }
            break;
        case expr_array_add:
            {
                coda_cursor prev_cursor;
                long num_elements;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                {
                    return -1;
                }
                *offset = 0;
                *length = 0;
                *value = NULL;
                if (num_elements > 0)
                {
                    long i;

                    if (coda_cursor_goto_first_array_element(&info->cursor) != 0)
                    {
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        long el_offset;
                        long el_length;
                        char *el_value;

                        if (eval_string(info, opexpr->operand[1], &el_offset, &el_length, &el_value) != 0)
                        {
                            return -1;
                        }
                        if (el_length > 0)
                        {
                            char *new_value;

                            new_value = realloc(*value, *length + el_length);
                            if (new_value == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %ld bytes) (%s:%u)",
                                               *length + el_length, __FILE__, __LINE__);
                                free(el_value);
                                return -1;
                            }
                            memcpy(&new_value[*length], &el_value[el_offset], el_length);
                            free(el_value);
                            *length += el_length;
                            *value = new_value;
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&info->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                    }
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_if:
            {
                int condition;

                if (eval_boolean(info, opexpr->operand[0], &condition) != 0)
                {
                    return -1;
                }
                if (condition)
                {
                    if (eval_string(info, opexpr->operand[1], offset, length, value) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (eval_string(info, opexpr->operand[2], offset, length, value) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case expr_filename:
            {
                const char *filepath;
                const char *filename;

                assert(info->orig_cursor != NULL);
                if (coda_get_product_filename(info->orig_cursor->product, &filepath) != 0)
                {
                    return -1;
                }
                filename = filepath;
                while (*filepath != '\0')
                {
                    if (*filepath == '/')
                    {
                        filename = &filepath[1];
                    }
                    filepath++;
                }
                *offset = 0;
                *length = strlen(filename);
                *value = malloc(*length + 1);   /* add room for zero termination at a later time */
                if (*value == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                   *length, __FILE__, __LINE__);
                    return -1;
                }
                memcpy(*value, filename, *length);
            }
            break;
        case expr_regex:
            {
                int ovector[(REGEX_MAX_NUM_SUBSTRING + 1) * 3];
                const char *error;
                int erroffset;
                long matchstring_offset;
                long matchstring_length;
                char *matchstring;
                long pattern_offset;
                long pattern_length;
                char *pattern = NULL;
                pcre *re;
                int index = 0;
                int rc;

                if (eval_string(info, opexpr->operand[0], &pattern_offset, &pattern_length, &pattern) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], &matchstring_offset, &matchstring_length, &matchstring) != 0)
                {
                    if (pattern != NULL)
                    {
                        free(pattern);
                    }
                    return -1;
                }

                if (pattern_length > 0)
                {
                    pattern[pattern_offset + pattern_length] = '\0';
                    re = pcre_compile(&pattern[pattern_offset], PCRE_DOTALL | PCRE_DOLLAR_ENDONLY, &error, &erroffset,
                                      NULL);
                }
                else
                {
                    re = pcre_compile("", PCRE_DOTALL | PCRE_DOLLAR_ENDONLY, &error, &erroffset, NULL);
                }
                if (re == NULL)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION,
                                   "invalid format for regex pattern ('%s' at position %d)", error, erroffset);
                    if (pattern != NULL)
                    {
                        free(pattern);
                    }
                    return -1;
                }
                if (pattern != NULL)
                {
                    free(pattern);
                }

                /* determine substring index of substring that we need to return */
                if (opexpr->operand[2]->result_type == coda_expression_integer)
                {
                    int64_t intvalue;

                    /* get subexpression by index */
                    if (eval_integer(info, opexpr->operand[2], &intvalue) != 0)
                    {
                        pcre_free(re);
                        return -1;
                    }
                    index = (int)intvalue;
                }
                else
                {
                    long substrname_offset;
                    long substrname_length;
                    char *substrname;

                    /* get subexpression by name */
                    if (eval_string(info, opexpr->operand[2], &substrname_offset, &substrname_length, &substrname) != 0)
                    {
                        pcre_free(re);
                        return -1;
                    }
                    if (length == 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION,
                                       "invalid substring name parameter for regex (empty string)");
                        if (substrname != NULL)
                        {
                            free(substrname);
                        }
                        pcre_free(re);
                        return -1;
                    }
                    index = pcre_get_stringnumber(re, substrname);
                    if (index < 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION,
                                       "invalid substring name parameter for regex (substring name not in pattern)");
                        free(substrname);
                        pcre_free(re);
                        return -1;
                    }
                    free(substrname);
                }

                if (matchstring == NULL)
                {
                    /* pcre_exec does not except NULL for an empty matchstring */
                    matchstring = strdup("");
                    if (matchstring == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                       __FILE__, __LINE__);
                        pcre_free(re);
                        return -1;
                    }
                }

                rc = pcre_exec(re, NULL, &matchstring[matchstring_offset], matchstring_length, 0, 0, ovector,
                               (REGEX_MAX_NUM_SUBSTRING + 1) * 3);
                pcre_free(re);
                if (rc < 0 && rc != PCRE_ERROR_NOMATCH)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate regex pattern (error code %d)", rc);
                    free(matchstring);
                    return -1;
                }
                if (rc == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "regex pattern contains too many subexpressions");
                    free(matchstring);
                    return -1;
                }
                if (index >= rc)
                {
                    /* there was no match for this subexpression -> return empty string */
                    *offset = 0;
                    *length = 0;
                    *value = NULL;
                    free(matchstring);
                }
                else
                {
                    *offset = ovector[2 * index];
                    *length = ovector[2 * index + 1] - ovector[2 * index];
                    *value = matchstring;
                }
            }
            break;
        case expr_product_class:
            {
                const char *product_class;

                assert(info->orig_cursor != NULL);
                if (coda_get_product_class(info->orig_cursor->product, &product_class) != 0)
                {
                    return -1;
                }
                *offset = 0;
                *length = 0;
                if (product_class != NULL)
                {
                    *length = strlen(product_class);
                    *value = malloc(*length + 1);       /* add room for zero termination at the end */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length, __FILE__, __LINE__);
                        return -1;
                    }
                    memcpy(*value, product_class, *length);
                }
            }
            break;
        case expr_product_type:
            {
                const char *product_type;

                assert(info->orig_cursor != NULL);
                if (coda_get_product_type(info->orig_cursor->product, &product_type) != 0)
                {
                    return -1;
                }
                *offset = 0;
                *length = 0;
                if (product_type != NULL)
                {
                    *length = strlen(product_type);
                    *value = malloc(*length + 1);       /* add room for zero termination at the end */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length, __FILE__, __LINE__);
                        return -1;
                    }
                    memcpy(*value, product_type, *length);
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }
    return 0;
}

static int eval_void(eval_info *info, const coda_expression *expr)
{
    const coda_expression_operation *opexpr;

    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_for:
            {
                int64_t prev_index;
                int64_t from;
                int64_t to;
                int64_t step = 1;

                prev_index = info->for_index;
                if (eval_integer(info, opexpr->operand[0], &from) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &to) != 0)
                {
                    return -1;
                }
                if (opexpr->operand[2] != NULL)
                {
                    if (eval_integer(info, opexpr->operand[2], &step) != 0)
                    {
                        return -1;
                    }
                    if (step == 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION, "step is 0 in for loop in expression");
                        return -1;
                    }
                }
                if (step > 0)
                {
                    for (info->for_index = from; info->for_index <= to; info->for_index += step)
                    {
                        if (eval_void(info, opexpr->operand[3]) != 0)
                        {
                            return -1;
                        }
                    }
                }
                else
                {
                    for (info->for_index = from; info->for_index >= to; info->for_index += step)
                    {
                        if (eval_void(info, opexpr->operand[3]) != 0)
                        {
                            return -1;
                        }
                    }
                }
                info->for_index = prev_index;
            }
            break;
        case expr_goto:
            if (eval_cursor(info, opexpr->operand[0]) != 0)
            {
                return -1;
            }
            break;
        case expr_sequence:
            if (eval_void(info, opexpr->operand[0]) != 0)
            {
                return -1;
            }
            if (eval_void(info, opexpr->operand[1]) != 0)
            {
                return -1;
            }
            break;
        case expr_variable_set:
            {
                int64_t *varptr;
                int64_t index = 0;
                int64_t value;

                assert(info->orig_cursor != NULL);
                if (opexpr->operand[0] != NULL)
                {
                    if (eval_integer(info, opexpr->operand[0], &index) != 0)
                    {
                        return -1;
                    }
                }
                if (eval_integer(info, opexpr->operand[1], &value) != 0)
                {
                    return -1;
                }
                if (coda_product_variable_get_pointer
                    (info->orig_cursor->product, opexpr->identifier, (long)index, &varptr) != 0)
                {
                    return -1;
                }
                *varptr = value;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int eval_cursor(eval_info *info, const coda_expression *expr)
{
    const coda_expression_operation *opexpr;

    assert(info->orig_cursor != NULL);
    opexpr = (const coda_expression_operation *)expr;
    switch (opexpr->tag)
    {
        case expr_goto_here:
            /* do nothing */
            break;
        case expr_goto_begin:
            info->cursor = *info->orig_cursor;
            break;
        case expr_goto_root:
            if (coda_cursor_set_product(&info->cursor, info->orig_cursor->product) != 0)
            {
                return -1;
            }
            break;
        case expr_goto_field:
            {
                coda_type_class type_class;

                if (opexpr->operand[0] != NULL)
                {
                    if (eval_cursor(info, opexpr->operand[0]) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_cursor_get_type_class(&info->cursor, &type_class) != 0)
                {
                    return -1;
                }
                if (type_class == coda_special_class)
                {
                    /* for special types we use the base type for traversing records */
                    if (coda_cursor_use_base_type_of_special_type(&info->cursor) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_cursor_goto_record_field_by_name(&info->cursor, opexpr->identifier) != 0)
                {
                    return -1;
                }
            }
            break;
        case expr_goto_array_element:
            {
                int64_t index;

                if (opexpr->operand[0] != NULL)
                {
                    if (eval_cursor(info, opexpr->operand[0]) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (coda_cursor_set_product(&info->cursor, info->orig_cursor->product) != 0)
                    {
                        return -1;
                    }
                }
                if (eval_integer(info, opexpr->operand[1], &index) != 0)
                {
                    return -1;
                }
                if (!coda_option_perform_boundary_checks)
                {
                    long num_elements;
                    coda_type_class type_class;

                    /* if boundary checking is disabled globally, we still want to perform boundary checks on
                     * expressions since these can also go wrong when files are corrupted. Therefore, if the
                     * coda_cursor_goto_array_element_by_index() does not perform such a check itself, we perform a
                     * boundary check here.
                     */

                    if (coda_cursor_get_type_class(&info->cursor, &type_class) != 0)
                    {
                        return -1;
                    }
                    if (type_class != coda_array_class)
                    {
                        coda_set_error(CODA_ERROR_INVALID_TYPE,
                                       "cursor does not refer to an array (current type is %s)",
                                       coda_type_get_class_name(type_class));
                        return -1;
                    }

                    if (coda_cursor_get_num_elements(&info->cursor, &num_elements) != 0)
                    {
                        return -1;
                    }
                    if (index < 0 || index >= num_elements)
                    {
                        coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS,
                                       "array index (%ld) exceeds array range [0:%ld) (%s:%u)", (long)index,
                                       num_elements, __FILE__, __LINE__);
                        return -1;
                    }
                }
                if (coda_cursor_goto_array_element_by_index(&info->cursor, (long)index) != 0)
                {
                    return -1;
                }
            }
            break;
        case expr_goto_parent:
            if (opexpr->operand[0] != NULL)
            {
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
            }
            if (coda_cursor_goto_parent(&info->cursor) != 0)
            {
                return -1;
            }
            break;
        case expr_goto_attribute:
            if (opexpr->operand[0] != NULL)
            {
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
            }
            else
            {
                if (coda_cursor_set_product(&info->cursor, info->orig_cursor->product) != 0)
                {
                    return -1;
                }
            }
            if (coda_cursor_goto_attributes(&info->cursor) != 0)
            {
                return -1;
            }
            if (coda_cursor_goto_record_field_by_name(&info->cursor, opexpr->identifier) != 0)
            {
                return -1;
            }
            break;
        case expr_asciiline:
            if (coda_ascii_cursor_set_asciilines(&info->cursor, info->orig_cursor->product) != 0)
            {
                return -1;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_expression_eval_void(const coda_expression *expr, const coda_cursor *cursor)
{
    eval_info info;

    if (expr->result_type != coda_expression_void)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not a 'void' expression");
        return -1;
    }
    if (cursor == NULL && !expr->is_constant)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument may not be NULL if expression is not constant");
        return -1;
    }

    info.orig_cursor = cursor;
    if (cursor != NULL)
    {
        info.cursor = *cursor;
    }
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    return eval_void(&info, expr);
}

/** \addtogroup coda_expression
 * @{
 */

/** \fn int coda_expression_from_string(const char *exprstring, coda_expression **expr)
 * \brief Create a new CODA expression object by parsing a string containing a CODA expression.
 * The string should contain a valid CODA expression.
 * The returned expression object should be cleaned up using coda_expression_delete() after it has been used.
 * \param exprstring A string containing the string representation of the CODA expression 
 * \param expr Pointer to the variable where the expression object will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */

/** Delete the CODA expression object.
 * \param expr A CODA expression object
 */
LIBCODA_API void coda_expression_delete(coda_expression *expr)
{
    switch (expr->tag)
    {
        case expr_constant_boolean:
        case expr_constant_float:
        case expr_constant_integer:
            break;
        case expr_constant_rawstring:
        case expr_constant_string:
            if (((coda_expression_string_constant *)expr)->value != NULL)
            {
                free(((coda_expression_string_constant *)expr)->value);
            }
            break;
        default:
            {
                coda_expression_operation *opexpr;
                int i;

                opexpr = (coda_expression_operation *)expr;
                if (opexpr->identifier != NULL)
                {
                    free(opexpr->identifier);
                }
                for (i = 0; i < 4; i++)
                {
                    if (opexpr->operand[i] != NULL)
                    {
                        coda_expression_delete(opexpr->operand[i]);
                    }
                }
            }
            break;
    }
    free(expr);
}

/** Return whether an expression is constant or not.
 * An expression is constant if it does not depend on the contents of a product and if the expression evaluation
 * function can be called with cursor=NULL.
 * \param expr A CODA expression object
 * \return
 *   \arg \c 0, Expression will depend on the contents of a product.
 *   \arg \c 1, Expression is constant.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_is_constant(const coda_expression *expr)
{
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid expression argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    return expr->is_constant;
}

/** Retrieve the result type of a CODA expression.
 * \param expr A CODA expression object
 * \param type Pointer to the variable where the result type of the CODA expression will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_get_type(const coda_expression *expr, coda_expression_type *type)
{
    if (expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid expression argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    *type = expr->result_type;
    return 0;
}

/** Evaluate a boolean expression.
 * The expression object should be a coda_expression_bool expression.
 * The function will evaluate the expression at the given cursor position and return the resulting boolean value (which
 * will be 0 for False and 1 for True).
 * \param expr A boolean expression object
 * \param cursor Cursor pointing to a location in the product where the boolean expression should be evaluated (can be
 * NULL for constant expressions).
 * \param value Pointer to the variable where the resulting boolean value will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_eval_bool(const coda_expression *expr, const coda_cursor *cursor, int *value)
{
    eval_info info;

    if (expr->result_type != coda_expression_boolean)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not a 'boolean' expression");
        return -1;
    }
    if (cursor == NULL && !expr->is_constant)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument may not be NULL if expression is not constant");
        return -1;
    }

    info.orig_cursor = cursor;
    if (cursor != NULL)
    {
        info.cursor = *cursor;
    }
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    return eval_boolean(&info, expr, value);
}

/** Evaluate an integer expression.
 * The expression object should be a coda_expression_integer expression.
 * The function will evaluate the expression at the given cursor position and return the resulting integer value.
 * \param expr An integer expression object
 * \param cursor Cursor pointing to a location in the product where the integer expression should be evaluated (can be
 * NULL for constant expressions).
 * \param value Pointer to the variable where the resulting integer value will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_eval_integer(const coda_expression *expr, const coda_cursor *cursor, int64_t *value)
{
    eval_info info;

    if (expr->result_type != coda_expression_integer)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not an 'integer' expression");
        return -1;
    }
    if (cursor == NULL && !expr->is_constant)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument may not be NULL if expression is not constant");
        return -1;
    }

    info.orig_cursor = cursor;
    if (cursor != NULL)
    {
        info.cursor = *cursor;
    }
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    return eval_integer(&info, expr, value);
}

/** Evaluate a floating point expression.
 * The function will evaluate the expression at the given cursor position and return the resulting floating point value.
 * The expression object should be a coda_expression_float expression.
 * \param expr A floating point expression object
 * \param cursor Cursor pointing to a location in the product where the floating point expression should be evaluated
 * (can be NULL for constant expressions).
 * \param value Pointer to the variable where the resulting floating point value will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_eval_float(const coda_expression *expr, const coda_cursor *cursor, double *value)
{
    eval_info info;

    if (expr->result_type != coda_expression_float)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not a 'double' expression");
        return -1;
    }
    if (cursor == NULL && !expr->is_constant)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument may not be NULL if expression is not constant");
        return -1;
    }

    info.orig_cursor = cursor;
    if (cursor != NULL)
    {
        info.cursor = *cursor;
    }
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    return eval_float(&info, expr, value);
}

/** Evaluate a string expression.
 * The function will evaluate the expression at the given cursor position (if provided) and return the resulting string
 * and length.
 * If length is 0 then no string will be returned and \c value will be set to NULL.
 * If a string is returned then it will be zero terminated. However, in the case where the string itself also contains
 * zero characters, strlen() can not be used and the \c length parameter will give the actual string length of \c value.
 * The expression object should be a coda_expression_string expression.
 * \note The caller of this function is responsible for freeing the memory of the result that is stored in \c value.
 * It is recommended to do this with coda_free().
 * \param expr A string expression object
 * \param cursor Cursor pointing to a location in the product where the string expression should be evaluated (can be
 * NULL for constant expressions).
 * \param value Pointer to the variable where the result string will be stored (will be NULL if length == 0).
 * \param length Pointer to the variable where the length of the result string will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_eval_string(const coda_expression *expr, const coda_cursor *cursor, char **value,
                                            long *length)
{
    eval_info info;
    long offset;

    if (expr->result_type != coda_expression_string)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not a 'string' expression");
        return -1;
    }
    if (cursor == NULL && !expr->is_constant)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument may not be NULL if expression is not constant");
        return -1;
    }

    info.orig_cursor = cursor;
    if (cursor != NULL)
    {
        info.cursor = *cursor;
    }
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    if (eval_string(&info, expr, &offset, length, value) != 0)
    {
        return -1;
    }

    if (*length > 0)
    {
        if (offset != 0)
        {
            char *truncated_value;

            truncated_value = malloc(*length + 1);
            if (truncated_value == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                               *length, __FILE__, __LINE__);
                return -1;
            }
            memcpy(truncated_value, &(*value)[offset], *length);
            free(*value);
            *value = truncated_value;
        }
        (*value)[*length] = '\0';
    }
    else
    {
        if (*value != NULL)
        {
            free(*value);
        }
        *value = NULL;
    }

    return 0;
}

/** Evaluate a node expression.
 * The function will moves the cursor to a different position in a product based on the node expression.
 * The expression object should be a coda_expression_node expression.
 * \param expr A node expression object
 * \param cursor Cursor pointing to a location in the product where the node expression should be evaluated.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_eval_node(const coda_expression *expr, coda_cursor *cursor)
{
    eval_info info;

    if (expr->result_type != coda_expression_node)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "expression is not a 'node' expression");
        return -1;
    }
    if (cursor == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "cursor argument may not be NULL for evaluation of node expression");
        return -1;
    }

    info.orig_cursor = cursor;
    info.cursor = *cursor;
    info.for_index = 0;
    info.variable_index = 0;
    info.variable_name = NULL;

    if (eval_cursor(&info, expr) != 0)
    {
        return -1;
    }

    *cursor = info.cursor;

    return 0;
}

/** @} */
