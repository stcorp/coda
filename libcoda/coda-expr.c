/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-expr.h"

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "coda-ascii.h"
#include "ipow.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

/** \defgroup coda_expression CODA Expression
 * CODA comes with a powerful expression language that can be used to perform calculations based on product data.
 * This expression system is used internally with the product format definition (codadef) files that CODA uses to
 * interpret products, but it can also be used by you as a user for your own purposes.
 * More information on the CODA expression language and its ascii syntax can be found in the CODA documentation.
 *
 * The example below shows how to evaluate a simple integer expression that does not make use of any product data:
 * \code{.c}
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
 * \code{.c}
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

/** \typedef coda_expression_type
 * Result types of CODA expressions.
 * \ingroup coda_expression
 */


#define REGEX_MAX_NUM_SUBSTRING 15

#ifndef CODA_MAX_RECURSION_DEPTH
#define CODA_MAX_RECURSION_DEPTH 10000
#endif

static int iswhitespace(char a)
{
    return (a == ' ' || a == '\t' || a == '\n' || a == '\r');
}

static int compare_strings(long off_a, long len_a, char *a, long off_b, long len_b, char *b)
{
    long index = 0;

    while (index < len_a && index < len_b && a[off_a + index] == b[off_b + index])
    {
        index++;
    }
    if (index == len_a)
    {
        if (index == len_b)
        {
            return 0;
        }
        return -1;
    }
    if (index == len_b || ((uint8_t)a[off_a + index]) > ((uint8_t)b[off_b + index]))
    {
        return 1;
    }
    return -1;
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
    expr->recursion_depth = 0;
    expr->value = (*string_value == 't' || *string_value == 'T');
    free(string_value);

    return (coda_expression *)expr;
}

static coda_expression *float_constant_new(char *string_value)
{
    coda_expression_float_constant *expr;
    double value;

    if (coda_ascii_parse_double(string_value, (long)strlen(string_value), &value, 0) < 0)
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
    expr->recursion_depth = 0;
    expr->value = value;

    return (coda_expression *)expr;
}

static coda_expression *integer_constant_new(char *string_value)
{
    coda_expression_integer_constant *expr;
    int64_t value;

    if (coda_ascii_parse_int64(string_value, (long)strlen(string_value), &value, 0) < 0)
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
    expr->recursion_depth = 0;
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
    expr->recursion_depth = 0;
    expr->length = (long)strlen(string_value);
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
    expr->recursion_depth = 0;
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

    if (tag == expr_neg)
    {
        /* turn constant numbers into negative constant values */
        if (op1->tag == expr_constant_float)
        {
            ((coda_expression_float_constant *)op1)->value = -((coda_expression_float_constant *)op1)->value;
            return op1;
        }
        if (op1->tag == expr_constant_integer)
        {
            ((coda_expression_integer_constant *)op1)->value = -((coda_expression_integer_constant *)op1)->value;
            return op1;
        }
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
        case expr_power:
        case expr_ceil:
        case expr_float:
        case expr_floor:
        case expr_round:
        case expr_time:
            expr->result_type = coda_expression_float;
            break;
        case expr_and:
        case expr_array_count:
        case expr_array_index:
        case expr_bit_offset:
        case expr_bit_size:
        case expr_byte_offset:
        case expr_byte_size:
        case expr_dim:
        case expr_file_size:
        case expr_index:
        case expr_index_var:
        case expr_integer:
        case expr_length:
        case expr_num_dims:
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
        case expr_product_format:
        case expr_product_type:
        case expr_rtrim:
        case expr_string:
        case expr_strtime:
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
        case expr_array_max:
        case expr_array_min:
        case expr_at:
        case expr_if:
        case expr_with:
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
        case expr_product_format:
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

    expr->recursion_depth = 0;
    if (op1 != NULL)
    {
        if (op1->recursion_depth + 1 > expr->recursion_depth)
        {
            expr->recursion_depth = op1->recursion_depth + 1;
        }
    }
    if (op2 != NULL)
    {
        if (op2->recursion_depth + 1 > expr->recursion_depth)
        {
            expr->recursion_depth = op2->recursion_depth + 1;
        }
    }
    if (op3 != NULL)
    {
        if (op3->recursion_depth + 1 > expr->recursion_depth)
        {
            expr->recursion_depth = op3->recursion_depth + 1;
        }
    }
    if (op4 != NULL)
    {
        if (op4->recursion_depth + 1 > expr->recursion_depth)
        {
            expr->recursion_depth = op4->recursion_depth + 1;
        }
    }
    if (expr->recursion_depth > CODA_MAX_RECURSION_DEPTH)
    {
        coda_set_error(CODA_ERROR_EXPRESSION, "maximum recursion depth (%ld) reached", CODA_MAX_RECURSION_DEPTH);
        coda_expression_delete((coda_expression *)expr);
        return NULL;
    }

    return (coda_expression *)expr;
}

typedef struct eval_info_struct
{
    const coda_cursor *orig_cursor;
    coda_cursor cursor;
    int64_t index[3];
    int64_t variable_index;
    const char *variable_name;
} eval_info;

static void init_eval_info(eval_info *info, const coda_cursor *cursor)
{
    info->orig_cursor = cursor;
    if (cursor != NULL)
    {
        info->cursor = *cursor;
    }
    info->index[0] = 0;
    info->index[1] = 0;
    info->index[2] = 0;
    info->variable_index = 0;
    info->variable_name = NULL;
}

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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) == 0);
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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) != 0);
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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) > 0);
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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) >= 0);
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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) < 0);
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
                *value = (compare_strings(off_a, len_a, a, off_b, len_b, b) <= 0);
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
                int errorcode;
                PCRE2_SIZE erroffset;
                long matchstring_offset;
                long matchstring_length;
                char *matchstring;
                long pattern_offset;
                long pattern_length;
                char *pattern;
                pcre2_match_data *match_data;
                pcre2_code *re;
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
                    re = pcre2_compile((PCRE2_SPTR8)&pattern[pattern_offset], pattern_length,
                                       PCRE2_DOTALL | PCRE2_DOLLAR_ENDONLY, &errorcode, &erroffset, NULL);
                }
                else
                {
                    re = pcre2_compile((PCRE2_SPTR8)"", 0, PCRE2_DOTALL | PCRE2_DOLLAR_ENDONLY, &errorcode, &erroffset,
                                       NULL);
                }
                if (pattern != NULL)
                {
                    free(pattern);
                }
                if (re == NULL)
                {
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
                    coda_set_error(CODA_ERROR_EXPRESSION,
                                   "invalid format for regex pattern ('%s' at position %d)", buffer, erroffset);
                    if (matchstring != NULL)
                    {
                        free(matchstring);
                    }
                    return -1;
                }

                if (matchstring == NULL)
                {
                    /* pcre2_match does not except NULL for an empty matchstring */
                    matchstring = strdup("");
                    if (matchstring == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                       __FILE__, __LINE__);
                        pcre2_code_free(re);
                        return -1;
                    }
                }

                match_data = pcre2_match_data_create_from_pattern(re, NULL);
                if (match_data == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "could not allocate regexp match data (%s:%u)",
                                   __FILE__, __LINE__);
                    free(matchstring);
                    pcre2_code_free(re);
                    return -1;
                }
                rc = pcre2_match(re, (PCRE2_SPTR8)&matchstring[matchstring_offset], matchstring_length, 0, 0,
                                 match_data, NULL);
                free(matchstring);
                pcre2_code_free(re);
                pcre2_match_data_free(match_data);
                if (rc < 0 && rc != PCRE2_ERROR_NOMATCH)
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
        case expr_at:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (eval_boolean(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_with:
            {
                int64_t prev_index;
                int index_id = opexpr->identifier[0] - 'i';

                prev_index = info->index[index_id];
                if (eval_integer(info, opexpr->operand[0], &info->index[index_id]) != 0)
                {
                    return -1;
                }
                if (eval_boolean(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->index[index_id] = prev_index;
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
                *value = ceil(*value - 0.5);
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
                if (opexpr->operand[1]->result_type == coda_expression_integer)
                {
                    int64_t intvalue;

                    /* try to use a more accurate algorithm if the exponent is a small integer */
                    if (eval_integer(info, opexpr->operand[1], &intvalue) != 0)
                    {
                        return -1;
                    }
                    if (intvalue >= -64 && intvalue <= 64)
                    {
                        *value = ipow(a, (int)intvalue);
                    }
                    else
                    {
                        *value = pow(a, (double)intvalue);
                    }
                }
                else
                {
                    if (eval_float(info, opexpr->operand[1], &b) != 0)
                    {
                        return -1;
                    }
                    *value = pow(a, b);
                }
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
        case expr_time:
            {
                long off_timestr, off_format;
                long len_timestr, len_format;
                char *timestr;
                char *format;

                if (eval_string(info, opexpr->operand[0], &off_timestr, &len_timestr, &timestr) != 0)
                {
                    return -1;
                }
                if (len_timestr < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "negative length for time string");
                    return -1;
                }
                if (len_timestr == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "time string is empty");
                    return -1;
                }
                timestr[off_timestr + len_timestr] = '\0';      /* add terminating zero */
                if (eval_string(info, opexpr->operand[1], &off_format, &len_format, &format) != 0)
                {
                    free(timestr);
                    return -1;
                }
                if (len_format < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "negative length for time format");
                    return -1;
                }
                if (len_format == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "time format is empty");
                    return -1;
                }
                format[off_format + len_format] = '\0'; /* add terminating zero */
                if (coda_time_string_to_double(&format[off_format], &timestr[off_timestr], value) != 0)
                {
                    free(format);
                    free(timestr);
                    return -1;
                }
                free(format);
                free(timestr);
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
        case expr_array_max:
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
                *value = coda_NaN();
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
                        if (i == 0 || element_value > *value)
                        {
                            *value = element_value;
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
        case expr_array_min:
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
                *value = coda_NaN();
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
                        if (i == 0 || element_value < *value)
                        {
                            *value = element_value;
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
        case expr_at:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_with:
            {
                int64_t prev_index;
                int index_id = opexpr->identifier[0] - 'i';

                prev_index = info->index[index_id];
                if (eval_integer(info, opexpr->operand[0], &info->index[index_id]) != 0)
                {
                    return -1;
                }
                if (eval_float(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->index[index_id] = prev_index;
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
            else if (opexpr->operand[0]->result_type == coda_expression_boolean)
            {
                int bvalue;

                if (eval_boolean(info, opexpr->operand[0], &bvalue) != 0)
                {
                    return -1;
                }
                *value = (int64_t)bvalue;
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
        case expr_array_max:
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
                        if (i == 0 || element_value > *value)
                        {
                            *value = element_value;
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
        case expr_array_min:
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
                        if (i == 0 || element_value < *value)
                        {
                            *value = element_value;
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
        case expr_dim:
            {
                coda_cursor prev_cursor;
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;
                int64_t dim_id;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], &dim_id) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_array_dim(&info->cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (dim_id < 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "dimension index (%ld) is negative", (long)dim_id);
                    return -1;
                }
                if (dim_id >= num_dims)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "dimension index (%ld) exceeds number of dimensions (%d)",
                                   (long)dim_id, num_dims);
                    return -1;
                }
                info->cursor = prev_cursor;
                *value = dim[dim_id];
            }
            break;
        case expr_num_dims:
            {
                coda_cursor prev_cursor;
                coda_type *type;
                int num_dims;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_type(&info->cursor, &type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_array_num_dims(type, &num_dims) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
                *value = num_dims;
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
        case expr_index_var:
            *value = info->index[opexpr->identifier[0] - 'i'];
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
        case expr_at:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_with:
            {
                int64_t prev_index;
                int index_id = opexpr->identifier[0] - 'i';

                prev_index = info->index[index_id];
                if (eval_integer(info, opexpr->operand[0], &info->index[index_id]) != 0)
                {
                    return -1;
                }
                if (eval_integer(info, opexpr->operand[1], value) != 0)
                {
                    return -1;
                }
                info->index[index_id] = prev_index;
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

/* string values (if not equal to NULL) are always one byte longer than 'length' to allow for a terminating zero */
static int eval_string(eval_info *info, const coda_expression *expr, long *offset, long *length, char **value)
{
    const coda_expression_operation *opexpr;

    if (expr->tag == expr_constant_string || expr->tag == expr_constant_rawstring)
    {
        *offset = 0;
        *length = ((coda_expression_string_constant *)expr)->length;
        if (*length > 0)
        {
            *value = malloc(*length + 1);
            if (*value == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                               __LINE__);
                return -1;
            }
            memcpy(*value, ((coda_expression_string_constant *)expr)->value, *length);
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
            if (opexpr->operand[0]->result_type == coda_expression_node)
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
                    *value = malloc(*length + 1);       /* add room for zero termination */
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
            else
            {
                int64_t intvalue;
                char s[21];

                if (eval_integer(info, opexpr->operand[0], &intvalue) != 0)
                {
                    return -1;
                }
                coda_str64(intvalue, s);
                *value = strdup(s);
                if (*value == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    return -1;
                }
                *offset = 0;
                *length = (long)strlen(s);
            }
            break;
        case expr_bytes:
            {
                coda_cursor prev_cursor;
                int64_t byte_offset = 0;
                int64_t num_bytes;
                int64_t num_bits = -1;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (opexpr->operand[2] != NULL)
                {
                    if (eval_integer(info, opexpr->operand[1], &byte_offset) != 0)
                    {
                        return -1;
                    }
                    if (eval_integer(info, opexpr->operand[2], &num_bytes) != 0)
                    {
                        return -1;
                    }
                    if (num_bytes > 0)
                    {
                        num_bits = num_bytes << 3;
                    }
                }
                else if (opexpr->operand[1] != NULL)
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
                    if (coda_cursor_read_bits(&info->cursor, (uint8_t *)*value, byte_offset * 8, num_bits) != 0)
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
                if (len_a > 0)
                {
                    free(a);
                }
                if (len_b > 0)
                {
                    free(b);
                }
            }
            break;
        case expr_min:
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
                if (compare_strings(off_a, len_a, a, off_b, len_b, b) <= 0)
                {
                    *offset = off_a;
                    *length = len_a;
                    *value = a;
                    if (len_b > 0)
                    {
                        free(b);
                    }
                }
                else
                {
                    *offset = off_b;
                    *length = len_b;
                    *value = b;
                    if (len_a > 0)
                    {
                        free(a);
                    }
                }
            }
            break;
        case expr_max:
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
                if (compare_strings(off_a, len_a, a, off_b, len_b, b) >= 0)
                {
                    *offset = off_a;
                    *length = len_a;
                    *value = a;
                    if (len_b > 0)
                    {
                        free(b);
                    }
                }
                else
                {
                    *offset = off_b;
                    *length = len_b;
                    *value = b;
                    if (len_a > 0)
                    {
                        free(a);
                    }
                }
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

                            /* add room for zero termination at a later time */
                            new_value = realloc(*value, *length + el_length + 1);
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
        case expr_array_min:
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
                        if (compare_strings(el_offset, el_length, el_value, *offset, *length, *value) < 0)
                        {
                            if (*length > 0)
                            {
                                free(value);
                            }
                            *offset = el_offset;
                            *length = el_length;
                            *value = el_value;
                        }
                        else if (el_length > 0)
                        {
                            free(el_value);
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
        case expr_array_max:
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
                        if (compare_strings(el_offset, el_length, el_value, *offset, *length, *value) > 0)
                        {
                            if (*length > 0)
                            {
                                free(*value);
                            }
                            *offset = el_offset;
                            *length = el_length;
                            *value = el_value;
                        }
                        else if (el_length > 0)
                        {
                            free(el_value);
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
                    if (*filepath == '/' || *filepath == '\\')
                    {
                        filename = &filepath[1];
                    }
                    filepath++;
                }
                *offset = 0;
                *length = (long)strlen(filename);
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
                int errorcode;
                PCRE2_SIZE erroffset;
                long matchstring_offset;
                long matchstring_length;
                char *matchstring;
                long pattern_offset;
                long pattern_length;
                char *pattern = NULL;
                pcre2_match_data *match_data;
                pcre2_code *re;
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
                    re = pcre2_compile((PCRE2_SPTR8)&pattern[pattern_offset], pattern_length,
                                       PCRE2_DOTALL | PCRE2_DOLLAR_ENDONLY, &errorcode, &erroffset, NULL);
                }
                else
                {
                    re = pcre2_compile((PCRE2_SPTR8)"", 0, PCRE2_DOTALL | PCRE2_DOLLAR_ENDONLY, &errorcode, &erroffset,
                                       NULL);
                }
                if (pattern != NULL)
                {
                    free(pattern);
                }
                if (re == NULL)
                {
                    PCRE2_UCHAR buffer[256];
                    pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
                    coda_set_error(CODA_ERROR_EXPRESSION,
                                   "invalid format for regex pattern ('%s' at position %d)", buffer, erroffset);
                    if (matchstring != NULL)
                    {
                        free(matchstring);
                    }
                    return -1;
                }

                /* determine substring index of substring that we need to return */
                if (opexpr->operand[2]->result_type == coda_expression_integer)
                {
                    int64_t intvalue;

                    /* get subexpression by index */
                    if (eval_integer(info, opexpr->operand[2], &intvalue) != 0)
                    {
                        pcre2_code_free(re);
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
                        pcre2_code_free(re);
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
                        pcre2_code_free(re);
                        return -1;
                    }
                    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR8)substrname);
                    if (index < 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION,
                                       "invalid substring name parameter for regex (substring name not in pattern)");
                        free(substrname);
                        pcre2_code_free(re);
                        return -1;
                    }
                    free(substrname);
                }

                if (matchstring == NULL)
                {
                    /* pcre2_match does not except NULL for an empty matchstring */
                    matchstring = strdup("");
                    if (matchstring == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                       __FILE__, __LINE__);
                        pcre2_code_free(re);
                        return -1;
                    }
                }

                match_data = pcre2_match_data_create_from_pattern(re, NULL);
                if (match_data == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "could not allocate regexp match data (%s:%u)",
                                   __FILE__, __LINE__);
                    free(matchstring);
                    pcre2_code_free(re);
                    return -1;
                }
                rc = pcre2_match(re, (PCRE2_SPTR8)&matchstring[matchstring_offset], matchstring_length, 0, 0,
                                 match_data, NULL);
                pcre2_code_free(re);
                if (rc < 0 && rc != PCRE2_ERROR_NOMATCH)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate regex pattern (error code %d)", rc);
                    free(matchstring);
                    pcre2_match_data_free(match_data);
                    return -1;
                }
                if (rc == 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "regex pattern contains too many subexpressions");
                    free(matchstring);
                    pcre2_match_data_free(match_data);
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
                    PCRE2_SIZE *ovector;

                    ovector = pcre2_get_ovector_pointer(match_data);
                    *offset = (long)ovector[2 * index];
                    *length = (long)(ovector[2 * index + 1] - ovector[2 * index]);
                    *value = matchstring;
                }
                pcre2_match_data_free(match_data);
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
                    *length = (long)strlen(product_class);
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
        case expr_product_format:
            {
                coda_format format;
                const char *product_format;

                assert(info->orig_cursor != NULL);
                if (coda_get_product_format(info->orig_cursor->product, &format) != 0)
                {
                    return -1;
                }
                product_format = coda_type_get_format_name(format);
                *offset = 0;
                *length = 0;
                if (product_format != NULL)
                {
                    *length = (long)strlen(product_format);
                    *value = malloc(*length + 1);       /* add room for zero termination at the end */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length, __FILE__, __LINE__);
                        return -1;
                    }
                    memcpy(*value, product_format, *length);
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
                    *length = (long)strlen(product_type);
                    *value = malloc(*length + 1);       /* add room for zero termination at the end */
                    if (*value == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                       *length + 1, __FILE__, __LINE__);
                        return -1;
                    }
                    memcpy(*value, product_type, *length);
                }
            }
            break;
        case expr_strtime:
            {
                double timevalue;
                long off_format;
                long len_format;
                char *format;

                if (eval_float(info, opexpr->operand[0], &timevalue) != 0)
                {
                    return -1;
                }
                if (opexpr->operand[1] != NULL)
                {
                    if (eval_string(info, opexpr->operand[1], &off_format, &len_format, &format) != 0)
                    {
                        return -1;
                    }
                    if (len_format < 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION, "negative length for time format");
                        return -1;
                    }
                    if (len_format == 0)
                    {
                        coda_set_error(CODA_ERROR_EXPRESSION, "empty time format");
                        return -1;
                    }
                    format[off_format + len_format] = '\0';     /* add terminating zero */
                }
                else
                {
                    format = "yyyy-MM-dd'T'HH:mm:ss.SSSSSS";
                    len_format = (long)strlen(format);
                    off_format = 0;
                }
                *value = malloc(len_format + 1);        /* add room for zero termination at the end */
                if (*value == NULL)
                {
                    if (opexpr->operand[1] != NULL)
                    {
                        free(format);
                    }
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                                   *length + 1, __FILE__, __LINE__);
                    return -1;
                }
                if (coda_time_double_to_string(timevalue, &format[off_format], *value) != 0)
                {
                    if (opexpr->operand[1] != NULL)
                    {
                        free(format);
                    }
                    return -1;
                }
                *offset = 0;
                *length = (long)strlen(*value);
                if (opexpr->operand[1] != NULL)
                {
                    free(format);
                }
            }
            break;
        case expr_at:
            {
                coda_cursor prev_cursor;

                assert(info->orig_cursor != NULL);
                prev_cursor = info->cursor;
                if (eval_cursor(info, opexpr->operand[0]) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], offset, length, value) != 0)
                {
                    return -1;
                }
                info->cursor = prev_cursor;
            }
            break;
        case expr_with:
            {
                int64_t prev_index;
                int index_id = opexpr->identifier[0] - 'i';

                prev_index = info->index[index_id];
                if (eval_integer(info, opexpr->operand[0], &info->index[index_id]) != 0)
                {
                    return -1;
                }
                if (eval_string(info, opexpr->operand[1], offset, length, value) != 0)
                {
                    return -1;
                }
                info->index[index_id] = prev_index;
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
                int index_id = opexpr->identifier[0] - 'i';

                prev_index = info->index[index_id];
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
                    for (info->index[index_id] = from; info->index[index_id] <= to; info->index[index_id] += step)
                    {
                        if (eval_void(info, opexpr->operand[3]) != 0)
                        {
                            return -1;
                        }
                    }
                }
                else
                {
                    for (info->index[index_id] = from; info->index[index_id] >= to; info->index[index_id] += step)
                    {
                        if (eval_void(info, opexpr->operand[3]) != 0)
                        {
                            return -1;
                        }
                    }
                }
                info->index[index_id] = prev_index;
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
                if (opexpr->identifier != NULL)
                {
                    if (coda_cursor_goto_record_field_by_name(&info->cursor, opexpr->identifier) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    int64_t index;

                    if (eval_integer(info, opexpr->operand[1], &index) != 0)
                    {
                        return -1;
                    }
                    if (coda_cursor_goto_record_field_by_index(&info->cursor, (long)index) != 0)
                    {
                        return -1;
                    }
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
                        coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)",
                                       (long)index, num_elements);
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
            if (info->orig_cursor->product->format != coda_format_ascii)
            {
                coda_set_error(CODA_ERROR_EXPRESSION, "'asciiline' not allowed for %s files",
                               coda_type_get_format_name(info->orig_cursor->product->format));
                return -1;
            }
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

    init_eval_info(&info, cursor);
    return eval_void(&info, expr);
}

static void print_escaped_string(const char *str, int length, int (*print)(const char *, ...), int xml, int html)
{
    int i = 0;

    if (length == 0 || str == NULL)
    {
        return;
    }

    if (length < 0)
    {
        length = (int)strlen(str);
    }

    while (i < length)
    {
        switch (str[i])
        {
            case '\033':       /* windows does not recognize '\e' */
                print("\\e");
                break;
            case '\a':
                print("\\a");
                break;
            case '\b':
                print("\\b");
                break;
            case '\f':
                print("\\f");
                break;
            case '\n':
                print("\\n");
                break;
            case '\r':
                print("\\r");
                break;
            case '\t':
                print("\\t");
                break;
            case '\v':
                print("\\v");
                break;
            case '\\':
                print("\\\\");
                break;
            case '"':
                print(xml ? "\\&quot;" : "\\\"");
                break;
            case '<':
                print((xml || html) ? "&lt;" : "<");
                break;
            case '>':
                print((xml || html) ? "&gt;" : ">");
                break;
            case '&':
                print((xml || html) ? "&amp;" : "&");
                break;
            case ' ':
                print((xml || html) ? "&nbsp;" : " ");
                break;
            default:
                if (!isprint(str[i]))
                {
                    print("\\%03o", (int)(unsigned char)str[i]);
                }
                else
                {
                    print("%c", str[i]);
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
static int print_expression(const coda_expression *expr, int (*print)(const char *, ...), int xml, int html,
                            int precedence)
{
    assert(expr != NULL);

    switch (expr->tag)
    {
        case expr_abs:
            print(html ? "<b>abs</b>(" : "abs(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_add:
            if (precedence < 4)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 4);
            print(" + ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 4);
            if (precedence < 4)
            {
                print(")");
            }
            break;
        case expr_array_add:
            print(html ? "<b>add</b>(" : "add(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_array_max:
            print(html ? "<b>max</b>(" : "max(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_array_min:
            print(html ? "<b>min</b>(" : "min(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_array_all:
            print(html ? "<b>all</b>(" : "all(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_and:
            if (precedence < 7)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 7);
            print((html || xml) ? " &amp; " : " & ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 7);
            if (precedence < 7)
            {
                print(")");
            }
            break;
        case expr_ceil:
            print(html ? "<b>ceil</b>(" : "ceil(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_array_count:
            print(html ? "<b>count</b>(" : "count(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_array_exists:
            print(html ? "<b>exists</b>(" : "exists(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_array_index:
            print(html ? "<b>index</b>(" : "index(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_asciiline:
            print(html ? "<b>asciiline</b>" : "asciiline");
            break;
        case expr_bit_offset:
            print(html ? "<b>bitoffset</b>(" : "bitoffset(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_bit_size:
            print(html ? "<b>bitsize</b>(" : "bitsize(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_byte_offset:
            print(html ? "<b>byteoffset</b>(" : "byteoffset(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_byte_size:
            print(html ? "<b>bytesize</b>(" : "bytesize(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_bytes:
            print(html ? "<b>bytes</b>(" : "bytes(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                print(",");
                print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            }
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                print(",");
                print_expression(((coda_expression_operation *)expr)->operand[2], print, xml, html, 15);
            }
            print(")");
            break;
        case expr_constant_boolean:
            if (((coda_expression_bool_constant *)expr)->value)
            {
                print(html ? "<b>true</b>" : "true");
            }
            else
            {
                print(html ? "<b>false</b>" : "false");
            }
            break;
        case expr_constant_float:
            {
                char s[24];

                coda_strfl(((coda_expression_float_constant *)expr)->value, s);
                print("%s", s);
            }
            break;
        case expr_constant_integer:
            {
                char s[21];

                coda_str64(((coda_expression_integer_constant *)expr)->value, s);
                print("%s", s);
            }
            break;
        case expr_constant_rawstring:
        case expr_constant_string:
            print(xml ? "&quot;" : "\"");
            print_escaped_string(((coda_expression_string_constant *)expr)->value,
                                 ((coda_expression_string_constant *)expr)->length, print, xml, html);
            print(xml ? "&quot;" : "\"");
            break;
        case expr_dim:
            print(html ? "<b>dim</b>(" : "dim(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(",");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_divide:
            if (precedence < 3)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 3);
            print(" / ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 3);
            if (precedence < 3)
            {
                print(")");
            }
            break;
        case expr_equal:
            if (precedence < 6)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 6);
            print(" == ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 6);
            if (precedence < 6)
            {
                print(")");
            }
            break;
        case expr_exists:
            print(html ? "<b>exists</b>(" : "exists(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_file_size:
            print(html ? "<b>filesize</b>()" : "filesize()");
            break;
        case expr_filename:
            print(html ? "<b>filename</b>()" : "filename()");
            break;
        case expr_float:
            print(html ? "<b>float</b>(" : "float(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_floor:
            print(html ? "<b>floor</b>(" : "floor(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_for:
            print(html ? "<b>for</b> <i>%s</i> = " : "for %s = ", ((coda_expression_operation *)expr)->identifier);
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(html ? " <b>to</b> " : " to ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                print(html ? " <b>step</b> " : " step ");
                print_expression(((coda_expression_operation *)expr)->operand[2], print, xml, html, 15);
            }
            print(html ? " <b>do</b><br />" : " do ");
            print_expression(((coda_expression_operation *)expr)->operand[3], print, xml, html, 15);
            break;
        case expr_goto_array_element:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            }
            print("[");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print("]");
            break;
        case expr_goto_attribute:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            }
            print("@%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_begin:
            print(":");
            break;
        case expr_goto_field:
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[0]->tag != expr_goto_root)
            {
                print("/");
            }
            print("%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_goto_here:
            print(".");
            break;
        case expr_goto_parent:
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
                print("/");
            }
            print("..");
            break;
        case expr_goto_root:
            print("/");
            break;
        case expr_goto:
            print(html ? "<b>goto</b>(" : "goto(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_greater_equal:
            if (precedence < 5)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 5);
            print((html || xml) ? " &gt;= " : " >= ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 5);
            if (precedence < 5)
            {
                print(")");
            }
            break;
        case expr_greater:
            if (precedence < 5)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 5);
            print((html || xml) ? " &gt; " : " > ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 5);
            if (precedence < 5)
            {
                print(")");
            }
            break;
        case expr_if:
            print("if(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[2], print, xml, html, 15);
            print(")");
            break;
        case expr_index:
            print(html ? "<b>index</b>(" : "index(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_index_var:
            print(html ? "<i>%s</i>" : "%s", ((coda_expression_operation *)expr)->identifier);
            break;
        case expr_integer:
            print(html ? "<b>int</b>(" : "int(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_isinf:
            print(html ? "<b>isinf</b>(" : "isinf(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_ismininf:
            print(html ? "<b>ismininf</b>(" : "ismininf(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_isnan:
            print(html ? "<b>isnan</b>(" : "isnan(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_isplusinf:
            print(html ? "<b>isplusinf</b>(" : "isplusinf(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_length:
            print(html ? "<b>length</b>(" : "length(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_less_equal:
            if (precedence < 5)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 5);
            print((html || xml) ? " &lt;= " : " <= ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 5);
            if (precedence < 5)
            {
                print(")");
            }
            break;
        case expr_less:
            if (precedence < 5)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 5);
            print((html || xml) ? " &lt; " : " < ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 5);
            if (precedence < 5)
            {
                print(")");
            }
            break;
        case expr_logical_and:
            if (precedence < 9)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 9);
            print(html ? " <b>and</b> " : " and ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 9);
            if (precedence < 9)
            {
                print(")");
            }
            break;
        case expr_logical_or:
            if (precedence < 10)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 10);
            print(html ? " <b>or</b> " : " or ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 10);
            if (precedence < 10)
            {
                print(")");
            }
            break;
        case expr_ltrim:
            print(html ? "<b>ltrim</b>(" : "ltrim(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_max:
            print(html ? "<b>max</b>(" : "max(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_min:
            print(html ? "<b>min</b>(" : "min(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_modulo:
            if (precedence < 3)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 3);
            print(" %% ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 3);
            if (precedence < 3)
            {
                print(")");
            }
            break;
        case expr_multiply:
            if (precedence < 3)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 3);
            print(" * ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 3);
            if (precedence < 3)
            {
                print(")");
            }
            break;
        case expr_neg:
            print("-");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 1);
            break;
        case expr_not_equal:
            if (precedence < 6)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 6);
            print(" != ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 6);
            if (precedence < 6)
            {
                print(")");
            }
            break;
        case expr_not:
            print("!");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 1);
            break;
        case expr_num_dims:
            print(html ? "<b>numdims</b>(" : "numdims(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_num_elements:
            print(html ? "<b>numelements</b>(" : "numelements(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_or:
            if (precedence < 7)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 7);
            print(" | ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 7);
            if (precedence < 7)
            {
                print(")");
            }
            break;
        case expr_power:
            if (precedence < 2)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 2);
            print(" ^ ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 2);
            if (precedence < 2)
            {
                print(")");
            }
            break;
        case expr_product_class:
            print(html ? "<b>productclass</b>()" : "productclass()");
            break;
        case expr_product_format:
            print(html ? "<b>productformat</b>()" : "productformat()");
            break;
        case expr_product_type:
            print(html ? "<b>producttype</b>()" : "producttype()");
            break;
        case expr_product_version:
            print(html ? "<b>productversion</b>()" : "productversion()");
            break;
        case expr_regex:
            print(html ? "<b>regex</b>(" : "regex(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[2] != NULL)
            {
                print(", ");
                print_expression(((coda_expression_operation *)expr)->operand[2], print, xml, html, 15);
            }
            print(")");
            break;
        case expr_round:
            print(html ? "<b>round</b>(" : "round(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_rtrim:
            print(html ? "<b>rtrim</b>(" : "rtrim(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_sequence:
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(html ? ";<br />" : "; ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            break;
        case expr_string:
            print(html ? "<b>str</b>(" : "str(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                print(", ");
                print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            }
            print(")");
            break;
        case expr_strtime:
            print(html ? "<b>strtime</b>(" : "strtime(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            if (((coda_expression_operation *)expr)->operand[1] != NULL)
            {
                print(", ");
                print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            }
            print(")");
            break;
        case expr_substr:
            print(html ? "<b>substr</b>(" : "substr(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[2], print, xml, html, 15);
            print(")");
            break;
        case expr_subtract:
            if (precedence < 4)
            {
                print("(");
            }
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 4);
            print(" - ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 4);
            if (precedence < 4)
            {
                print(")");
            }
            break;
        case expr_time:
            print(html ? "<b>time</b>(" : "time(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_trim:
            print(html ? "<b>trim</b>(" : "trim(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_unbound_array_index:
            print(html ? "<b>unboundindex</b>(" : "unboundindex(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_variable_exists:
            print(html ? "<b>exists</b>(<i>$%s</i>, " : "exists($%s, ",
                  ((coda_expression_operation *)expr)->identifier);
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_variable_index:
            print(html ? "<b>index</b>(<i>$%s</i>, " : "index($%s, ", ((coda_expression_operation *)expr)->identifier);
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(")");
            break;
        case expr_variable_set:
            print(html ? "<i>$%s</i>" : "$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                print("[");
                print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
                print("]");
            }
            print(" = ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            break;
        case expr_variable_value:
            print("$%s", ((coda_expression_operation *)expr)->identifier);
            if (((coda_expression_operation *)expr)->operand[0] != NULL)
            {
                print("[");
                print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
                print("]");
            }
            break;
        case expr_at:
            print(html ? "<b>at</b>(" : "at(");
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
        case expr_with:
            print(html ? "<b>with</b>(<i>%s</i> = " : "with(%s = ", ((coda_expression_operation *)expr)->identifier);
            print_expression(((coda_expression_operation *)expr)->operand[0], print, xml, html, 15);
            print(", ");
            print_expression(((coda_expression_operation *)expr)->operand[1], print, xml, html, 15);
            print(")");
            break;
    }

    return 0;
}

int coda_expression_print_html(const coda_expression *expr, int (*print)(const char *, ...))
{
    return print_expression(expr, print, 1, 1, 15);
}

int coda_expression_print_xml(const coda_expression *expr, int (*print)(const char *, ...))
{
    return print_expression(expr, print, 1, 0, 15);
}

/** \addtogroup coda_expression
 * @{
 */

/** Write the full expression using a printf compatible function.
 * This function will produce a string representation of the expression itself
 * (it won't evaluate the expression, nor write its result).
 * The \a print function parameter should be a function that resembles printf().
 * The printed string representation is something that can be passed to #coda_expression_from_string()
 * again to reproduce the expression object.
 * \param expr Pointer to the expression object.
 * \param print Reference to a printf compatible function.
 * \return
 *   \arg \c  0, Succes.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_expression_print(const coda_expression *expr, int (*print)(const char *, ...))
{
    return print_expression(expr, print, 0, 0, 15);
}

/** \fn int coda_expression_from_string(const char *exprstring, coda_expression **expr)
 * Create a new CODA expression object by parsing a string containing a CODA expression.
 * The string should contain a valid CODA expression.
 * The returned expression object should be cleaned up using coda_expression_delete() after it has been used.
 * \param exprstring A string containing the string representation of the CODA expression 
 * \param expr Pointer to the variable where the expression object will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */


/** Returns the name of an expression type.
 * \param type CODA expression type
 * \return if the type is known a string containing the name of the type, otherwise the string "unknown".
 */
LIBCODA_API const char *coda_expression_get_type_name(coda_expression_type type)
{
    switch (type)
    {
        case coda_expression_boolean:
            return "boolean";
        case coda_expression_integer:
            return "integer";
        case coda_expression_float:
            return "float";
        case coda_expression_string:
            return "string";
        case coda_expression_node:
            return "node";
        case coda_expression_void:
            return "void";
    }

    return "unknown";
}


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

/** Determines whether two expressions are equal or not
 * This function will return 1 if both expressions are equal, and 0 if they are not.
 *
 * The comparison will not be on the evaluated result of the expressions but on the operators and operands of the
 * expressions themselves.  For two expressions to be equal, all operands to an operation need to be equal and
 * operands need to be provided in the same order.
 * So the expression '1!=2' will not be considered equal to the expression '2!=1'.
 *
 * Providing two NULL pointers will be considered as a case of equal expressions.
 *
 * \param expr1 A CODA expression object
 * \param expr2 A CODA expression object
 * \return
 *   \arg \c 0, expr1 and expr2 consist of the exact same sequence of operators and operands.
 *   \arg \c 1, expr1 and expr2 differ in terms of operators and operands.
 */
LIBCODA_API int coda_expression_is_equal(const coda_expression *expr1, const coda_expression *expr2)
{
    if (expr1 == NULL)
    {
        return expr2 == NULL;
    }
    if (expr2 == NULL)
    {
        return 0;
    }

    if (expr1->tag != expr2->tag)
    {
        return 0;
    }

    switch (expr1->tag)
    {
        case expr_constant_boolean:
            return ((coda_expression_bool_constant *)expr1)->value == ((coda_expression_bool_constant *)expr2)->value;
        case expr_constant_float:
            return ((coda_expression_float_constant *)expr1)->value == ((coda_expression_float_constant *)expr2)->value;
        case expr_constant_integer:
            return ((coda_expression_integer_constant *)expr1)->value ==
                ((coda_expression_integer_constant *)expr2)->value;
        case expr_constant_rawstring:
        case expr_constant_string:
            if (((coda_expression_string_constant *)expr1)->length !=
                ((coda_expression_string_constant *)expr2)->length)
            {
                return 0;
            }
            return memcmp(((coda_expression_string_constant *)expr1)->value,
                          ((coda_expression_string_constant *)expr2)->value,
                          ((coda_expression_string_constant *)expr1)->length) == 0;
        default:
            {
                coda_expression_operation *op1 = (coda_expression_operation *)expr1;
                coda_expression_operation *op2 = (coda_expression_operation *)expr2;
                int i;

                if (op1->identifier != NULL)
                {
                    if (op2->identifier == NULL)
                    {
                        return 0;
                    }
                    if (strcmp(op1->identifier, op2->identifier) != 0)
                    {
                        return 0;
                    }
                }
                else if (op2->identifier != NULL)
                {
                    return 0;
                }
                for (i = 0; i < 4; i++)
                {
                    if (!coda_expression_is_equal(op1->operand[i], op2->operand[i]))
                    {
                        return 0;
                    }
                }
            }
    }

    return 1;
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

    init_eval_info(&info, cursor);
    if (eval_boolean(&info, expr, value) != 0)
    {
        if (cursor != NULL && coda_cursor_compare(cursor, &info.cursor) != 0)
        {
            coda_cursor_add_to_error_message(&info.cursor);
        }
        return -1;
    }

    return 0;
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

    init_eval_info(&info, cursor);
    if (eval_integer(&info, expr, value) != 0)
    {
        if (cursor != NULL && coda_cursor_compare(cursor, &info.cursor) != 0)
        {
            coda_cursor_add_to_error_message(&info.cursor);
        }
        return -1;
    }

    return 0;
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

    init_eval_info(&info, cursor);
    if (eval_float(&info, expr, value) != 0)
    {
        if (cursor != NULL && coda_cursor_compare(cursor, &info.cursor) != 0)
        {
            coda_cursor_add_to_error_message(&info.cursor);
        }
        return -1;
    }

    return 0;
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

    init_eval_info(&info, cursor);
    if (eval_string(&info, expr, &offset, length, value) != 0)
    {
        if (cursor != NULL && coda_cursor_compare(cursor, &info.cursor) != 0)
        {
            coda_cursor_add_to_error_message(&info.cursor);
        }
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

    init_eval_info(&info, cursor);
    if (eval_cursor(&info, expr) != 0)
    {
        if (cursor != NULL && coda_cursor_compare(cursor, &info.cursor) != 0)
        {
            coda_cursor_add_to_error_message(&info.cursor);
        }
        return -1;
    }

    *cursor = info.cursor;

    return 0;
}

/** @} */
