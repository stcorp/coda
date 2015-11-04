/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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

#include "coda.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C"
{
#endif
/* *INDENT-ON* */

typedef struct coda_Expr_struct coda_Expr;

void coda_expr_delete(coda_Expr *expr);

int coda_expr_from_string(const char *exprstring, coda_Expr **expr);

int coda_expr_eval(const coda_Expr *expr, coda_Cursor *cursor);

/* NOTE: If you pass a cursor=NULL the functions below will try to evaluate the expression as a constant expression.
 * If this succeeds you will get the value and a return value 0. If the expression depends on data from a file you will
 * not get the value and a return value 1. You will get a return value -1 on error.
 */
int coda_expr_eval_bool(const coda_Expr *expr, const coda_Cursor *cursor, int *value);
int coda_expr_eval_integer(const coda_Expr *expr, const coda_Cursor *cursor, int64_t *value);
int coda_expr_eval_double(const coda_Expr *expr, const coda_Cursor *cursor, double *value);

/* NOTE: this function will allocate the string value, but the caller should free it */
int coda_expr_eval_string(const coda_Expr *expr, const coda_Cursor *cursor, long *length, char **value);

int coda_expr_get_result_type(const coda_Expr *expr, coda_native_type *result_type);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
