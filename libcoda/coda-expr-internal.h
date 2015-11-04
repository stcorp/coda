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

#ifndef CODA_EXPR_INTERNAL_H
#define CODA_EXPR_INTERNAL_H

#include "coda-expr.h"

enum coda_exprType_enum
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
    expr_bit_offset,
    expr_bit_size,
    expr_byte_offset,
    expr_byte_size,
    expr_bytes,
    expr_ceil,
    expr_constant_boolean,
    expr_constant_double,
    expr_constant_integer,
    expr_constant_string,
    expr_divide,
    expr_equal,
    expr_exists,
    expr_file_size,
    expr_filename,
    expr_float,
    expr_floor,
    expr_for_index,
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
    expr_num_elements,
    expr_or,
    expr_power,
    expr_product_class,
    expr_product_type,
    expr_product_version,
    expr_round,
    expr_rtrim,
    expr_sequence,
    expr_string,
    expr_substr,
    expr_subtract,
    expr_trim,
    expr_unbound_array_index,
    expr_variable_exists,
    expr_variable_index,
    expr_variable_set,
    expr_variable_value
};
typedef enum coda_exprType_enum coda_exprType;

enum coda_exprResultType_enum
{
    expr_result_boolean,
    expr_result_double,
    expr_result_integer,
    expr_result_string,
    expr_result_void,
    expr_result_node
};
typedef enum coda_exprResultType_enum coda_exprResultType;

struct coda_Expr_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
};

struct coda_ExprBoolConstant_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
    int value;
};
typedef struct coda_ExprBoolConstant_struct coda_ExprBoolConstant;

struct coda_ExprDoubleConstant_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
    double value;
};
typedef struct coda_ExprDoubleConstant_struct coda_ExprDoubleConstant;

struct coda_ExprIntegerConstant_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
    int64_t value;
};
typedef struct coda_ExprIntegerConstant_struct coda_ExprIntegerConstant;

struct coda_ExprStringConstant_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
    long length;
    char *value;
};
typedef struct coda_ExprStringConstant_struct coda_ExprStringConstant;

struct coda_ExprOperation_struct
{
    coda_exprType tag;
    coda_exprResultType result_type;
    char *identifier;
    coda_Expr *operand[4];
};
typedef struct coda_ExprOperation_struct coda_ExprOperation;

/* this routines will delete all input on failure */
coda_Expr *coda_expr_new(coda_exprType tag, char *string_value, coda_Expr *op1, coda_Expr *op2, coda_Expr *op3,
                         coda_Expr *op4);

#endif
