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

#ifndef CODA_ASCII_DEFINITION_H
#define CODA_ASCII_DEFINITION_H

#include "coda-ascbin-definition.h"
#include "coda-internal.h"

typedef struct coda_ascii_type_struct coda_ascii_type;
typedef struct coda_ascii_integer_struct coda_ascii_integer;
typedef struct coda_ascii_float_struct coda_ascii_float;
typedef struct coda_ascii_text_struct coda_ascii_text;
typedef struct coda_ascii_line_separator_struct coda_ascii_line_separator;
typedef struct coda_ascii_line_struct coda_ascii_line;
typedef struct coda_ascii_white_space_struct coda_ascii_white_space;
typedef struct coda_ascii_time_struct coda_ascii_time;

typedef struct coda_ascii_integer_mapping_struct coda_ascii_integer_mapping;
typedef struct coda_ascii_float_mapping_struct coda_ascii_float_mapping;

void coda_ascii_release_type(coda_ascii_type *type);

coda_ascii_integer *coda_ascii_integer_new(void);
int coda_ascii_integer_set_unit(coda_ascii_integer *integer, const char *unit);
int coda_ascii_integer_set_byte_size(coda_ascii_integer *integer, long byte_size);
int coda_ascii_integer_set_read_type(coda_ascii_integer *integer, coda_native_type read_type);
int coda_ascii_integer_set_conversion(coda_ascii_integer *integer, coda_conversion *conversion);
int coda_ascii_integer_add_mapping(coda_ascii_integer *integer, coda_ascii_integer_mapping *mapping);
int coda_ascii_integer_validate(coda_ascii_integer *integer);

coda_ascii_float *coda_ascii_float_new(void);
int coda_ascii_float_set_unit(coda_ascii_float *fl, const char *unit);
int coda_ascii_float_set_byte_size(coda_ascii_float *fl, long byte_size);
int coda_ascii_float_set_read_type(coda_ascii_float *fl, coda_native_type read_type);
int coda_ascii_float_set_conversion(coda_ascii_float *fl, coda_conversion *conversion);
int coda_ascii_float_add_mapping(coda_ascii_float *fl, coda_ascii_float_mapping *mapping);
int coda_ascii_float_validate(coda_ascii_float *fl);

coda_ascii_text *coda_ascii_text_new(void);
int coda_ascii_text_set_byte_size(coda_ascii_text *text, int64_t byte_size);
int coda_ascii_text_set_byte_size_expression(coda_ascii_text *text, coda_expression *byte_size_expr);
int coda_ascii_text_set_read_type(coda_ascii_text *text, coda_native_type read_type);
int coda_ascii_text_set_fixed_value(coda_ascii_text *text, const char *fixed_value);
int coda_ascii_text_validate(coda_ascii_text *text);

coda_ascii_line_separator *coda_ascii_line_separator_new(void);

coda_ascii_line *coda_ascii_line_new(int include_eol);

coda_ascii_white_space *coda_ascii_white_space_new(void);

coda_ascii_time *coda_ascii_time_new(const char *format);
int coda_ascii_time_add_mapping(coda_ascii_time *time, coda_ascii_float_mapping *mapping);

coda_ascii_integer_mapping *coda_ascii_integer_mapping_new(const char *str, int64_t value);
void coda_ascii_integer_mapping_delete(coda_ascii_integer_mapping *mapping);

coda_ascii_float_mapping *coda_ascii_float_mapping_new(const char *str, double value);
void coda_ascii_float_mapping_delete(coda_ascii_float_mapping *mapping);

#endif
