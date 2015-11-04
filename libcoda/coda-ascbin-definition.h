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

#ifndef CODA_ASCBIN_DEFINITION_H
#define CODA_ASCBIN_DEFINITION_H

#include "coda-internal.h"

typedef struct coda_conversion_struct coda_conversion;

coda_conversion *coda_conversion_new(double numerator, double denominator);
int coda_conversion_set_unit(coda_conversion *conversion, const char *unit);
void coda_conversion_delete(coda_conversion *conversion);

typedef struct coda_ascbin_type_struct coda_ascbin_type;
typedef struct coda_ascbin_field_struct coda_ascbin_field;
typedef struct coda_ascbin_record_struct coda_ascbin_record;
typedef struct coda_ascbin_union_struct coda_ascbin_union;
typedef struct coda_ascbin_array_struct coda_ascbin_array;

coda_ascbin_field *coda_ascbin_field_new(const char *name, const char *real_name);
int coda_ascbin_field_set_type(coda_ascbin_field *field, coda_ascbin_type *type);
int coda_ascbin_field_set_hidden(coda_ascbin_field *field);
int coda_ascbin_field_set_available_expression(coda_ascbin_field *field, coda_expression *available_expr);
int coda_ascbin_field_set_bit_offset_expression(coda_ascbin_field *field, coda_expression *bit_offset_expr);
int coda_ascbin_field_validate(coda_ascbin_field *field);
void coda_ascbin_field_delete(coda_ascbin_field *field);

coda_ascbin_record *coda_ascbin_record_new(coda_format format);
int coda_ascbin_record_set_fast_size_expression(coda_ascbin_record *record, coda_expression *fast_size_expr);
int coda_ascbin_record_add_field(coda_ascbin_record *record, coda_ascbin_field *field);
void coda_ascbin_record_delete(coda_ascbin_record *record);

coda_ascbin_union *coda_ascbin_union_new(coda_format format);
int coda_ascbin_union_set_fast_size_expression(coda_ascbin_union *dd_union, coda_expression *fast_size_expr);
int coda_ascbin_union_set_field_expression(coda_ascbin_union *dd_union, coda_expression *field_expr);
int coda_ascbin_union_add_field(coda_ascbin_union *dd_union, coda_ascbin_field *field);
int coda_ascbin_union_validate(coda_ascbin_union *dd_union);
void coda_ascbin_union_delete(coda_ascbin_union *dd_union);

coda_ascbin_array *coda_ascbin_array_new(coda_format format);
int coda_ascbin_array_set_base_type(coda_ascbin_array *array, coda_ascbin_type *base_type);
int coda_ascbin_array_add_fixed_dimension(coda_ascbin_array *array, long dim);
int coda_ascbin_array_add_variable_dimension(coda_ascbin_array *array, coda_expression *dim_expr);
int coda_ascbin_array_validate(coda_ascbin_array *array);
void coda_ascbin_array_delete(coda_ascbin_array *array);


#endif
