/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#ifndef CODA_EXPR_H
#define CODA_EXPR_H

#include "coda-internal.h"

enum coda_expression_node_type_enum
{
    expr_abs,
    expr_add,
    expr_and,
    expr_array_add,
    expr_array_all,
    expr_array_count,
    expr_array_exists,
    expr_array_index,
    expr_asciiline,
    expr_at,
    expr_bit_offset,
    expr_bit_size,
    expr_byte_offset,
    expr_byte_size,
    expr_bytes,
    expr_ceil,
    expr_constant_boolean,
    expr_constant_float,
    expr_constant_integer,
    expr_constant_rawstring,
    expr_constant_string,
    expr_dim,
    expr_divide,
    expr_equal,
    expr_exists,
    expr_file_size,
    expr_filename,
    expr_float,
    expr_floor,
    expr_for,
    expr_goto_array_element,
    expr_goto_attribute,
    expr_goto_begin,
    expr_goto_field,
    expr_goto_here,
    expr_goto_parent,
    expr_goto_root,
    expr_goto,
    expr_greater_equal,
    expr_greater,
    expr_if,
    expr_index,
    expr_index_var,
    expr_integer,
    expr_isinf,
    expr_ismininf,
    expr_isnan,
    expr_isplusinf,
    expr_length,
    expr_less_equal,
    expr_less,
    expr_logical_and,
    expr_logical_or,
    expr_ltrim,
    expr_max,
    expr_min,
    expr_modulo,
    expr_multiply,
    expr_neg,
    expr_not_equal,
    expr_not,
    expr_num_dims,
    expr_num_elements,
    expr_or,
    expr_power,
    expr_product_class,
    expr_product_format,
    expr_product_type,
    expr_product_version,
    expr_regex,
    expr_round,
    expr_rtrim,
    expr_sequence,
    expr_string,
    expr_strtime,
    expr_substr,
    expr_subtract,
    expr_time,
    expr_trim,
    expr_unbound_array_index,
    expr_variable_exists,
    expr_variable_index,
    expr_variable_set,
    expr_variable_value,
    expr_with
};
typedef enum coda_expression_node_type_enum coda_expression_node_type;

struct coda_expression_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
};

struct coda_expression_bool_constant_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
    int value;
};
typedef struct coda_expression_bool_constant_struct coda_expression_bool_constant;

struct coda_expression_float_constant_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
    double value;
};
typedef struct coda_expression_float_constant_struct coda_expression_float_constant;

struct coda_expression_integer_constant_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
    int64_t value;
};
typedef struct coda_expression_integer_constant_struct coda_expression_integer_constant;

struct coda_expression_string_constant_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
    long length;
    char *value;
};
typedef struct coda_expression_string_constant_struct coda_expression_string_constant;

struct coda_expression_operation_struct
{
    coda_expression_node_type tag;
    coda_expression_type result_type;
    int is_constant;
    char *identifier;
    coda_expression *operand[4];
};
typedef struct coda_expression_operation_struct coda_expression_operation;

/* this routine will delete all input on failure */
coda_expression *coda_expression_new(coda_expression_node_type tag, char *string_value, coda_expression *op1,
                                     coda_expression *op2, coda_expression *op3, coda_expression *op4);

#endif
