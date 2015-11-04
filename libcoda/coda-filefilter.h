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

#ifndef CODA_FILEFILTER_H
#define CODA_FILEFILTER_H

#include "coda-internal.h"

/* filefilter expression tree declarations */
typedef enum
{
    ff_boolean_constant,
    ff_double_constant,
    ff_string_constant,
    ff_operator,
    ff_function
} ff_expr_types;

typedef enum
{
    ff_error_type,
    ff_void_type,
    ff_boolean_type,
    ff_double_type,
    ff_string_type
} ff_basic_types;

typedef struct
{
    int operator_id;
    int num_operands;
    struct ff_expr_struct *operand[1];  /* expandable */
} ff_expr_operator;

typedef struct
{
    char *name;
    int num_arguments;
    struct ff_expr_struct *argument[1]; /* expandable */
} ff_expr_function;

typedef struct ff_expr_struct
{
    ff_expr_types type;

    /* union must be last entry in ff_expr because of possible expandable expr types */
    union
    {
        int boolean_constant;
        double double_constant;
        const char *string_constant;
        ff_expr_operator oper;
        ff_expr_function function;
    } value;
} ff_expr;

typedef struct ff_result_struct
{
    ff_basic_types type;

    union
    {
        int boolean_value;
        double double_value;
        const char *string_value;
    } value;
} ff_result;

/* expression evaluation declarations */
ff_result coda_filefilter_eval_expr(coda_Cursor *cursor, ff_expr *expr);

/* parser declarations */
int coda_filefilter_parse(void);
int coda_filefilter_error(char *error);
extern ff_expr *coda_filefilter_tree;

#endif
