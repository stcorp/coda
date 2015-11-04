/*
 * Copyright (C) 2007-2008 S&T, The Netherlands.
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

#ifndef CODA_ASCBIN_DEFINITION_H
#define CODA_ASCBIN_DEFINITION_H

#include "coda-internal.h"
#include "coda-expr.h"

typedef struct coda_Conversion_struct coda_Conversion;

coda_Conversion *coda_conversion_new(double numerator, double denominator);
int coda_conversion_set_unit(coda_Conversion *conversion, const char *unit);
void coda_conversion_delete(coda_Conversion *conversion);

typedef struct coda_ascbinType_struct coda_ascbinType;
typedef struct coda_ascbinField_struct coda_ascbinField;
typedef struct coda_ascbinRecord_struct coda_ascbinRecord;
typedef struct coda_ascbinUnion_struct coda_ascbinUnion;
typedef struct coda_ascbinArray_struct coda_ascbinArray;

coda_ascbinField *coda_ascbin_field_new(const char *name);
int coda_ascbin_field_set_type(coda_ascbinField *field, coda_ascbinType *type);
int coda_ascbin_field_set_hidden(coda_ascbinField *field);
int coda_ascbin_field_set_available_expression(coda_ascbinField *field, coda_Expr *available_expr);
int coda_ascbin_field_set_bit_offset_expression(coda_ascbinField *field, coda_Expr *bit_offset_expr);
int coda_ascbin_field_validate(coda_ascbinField *field);
void coda_ascbin_field_delete(coda_ascbinField *field);

coda_ascbinRecord *coda_ascbin_record_new(coda_format format);
int coda_ascbin_record_set_fast_size_expression(coda_ascbinRecord *record, coda_Expr *fast_size_expr);
int coda_ascbin_record_add_field(coda_ascbinRecord *record, coda_ascbinField *field);
void coda_ascbin_record_delete(coda_ascbinRecord *record);

coda_ascbinUnion *coda_ascbin_union_new(coda_format format);
int coda_ascbin_union_set_fast_size_expression(coda_ascbinUnion *dd_union, coda_Expr *fast_size_expr);
int coda_ascbin_union_set_field_expression(coda_ascbinUnion *dd_union, coda_Expr *field_expr);
int coda_ascbin_union_add_field(coda_ascbinUnion *dd_union, coda_ascbinField *field);
int coda_ascbin_union_validate(coda_ascbinUnion *dd_union);
void coda_ascbin_union_delete(coda_ascbinUnion *dd_union);

coda_ascbinArray *coda_ascbin_array_new(coda_format format);
int coda_ascbin_array_set_base_type(coda_ascbinArray *array, coda_ascbinType *base_type);
int coda_ascbin_array_add_fixed_dimension(coda_ascbinArray *array, long dim);
int coda_ascbin_array_add_variable_dimension(coda_ascbinArray *array, coda_Expr *dim_expr);
int coda_ascbin_array_validate(coda_ascbinArray *array);
void coda_ascbin_array_delete(coda_ascbinArray *array);


#endif
