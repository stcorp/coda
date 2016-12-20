/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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
ff_result coda_filefilter_eval_expr(coda_cursor *cursor, ff_expr *expr);

/* parser declarations */
int coda_filefilter_parse(void);
int coda_filefilter_error(char *error);
extern ff_expr *coda_filefilter_tree;

#endif
