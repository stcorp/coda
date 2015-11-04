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

#ifndef CODA_ASCII_DEFINITION_H
#define CODA_ASCII_DEFINITION_H

#include "coda-ascbin-definition.h"
#include "coda-internal.h"
#include "coda-expr.h"

typedef struct coda_asciiType_struct coda_asciiType;
typedef struct coda_asciiInteger_struct coda_asciiInteger;
typedef struct coda_asciiFloat_struct coda_asciiFloat;
typedef struct coda_asciiText_struct coda_asciiText;
typedef struct coda_asciiLineSeparator_struct coda_asciiLineSeparator;
typedef struct coda_asciiLine_struct coda_asciiLine;
typedef struct coda_asciiWhiteSpace_struct coda_asciiWhiteSpace;
typedef struct coda_asciiTime_struct coda_asciiTime;

typedef struct coda_asciiIntegerMapping_struct coda_asciiIntegerMapping;
typedef struct coda_asciiFloatMapping_struct coda_asciiFloatMapping;

void coda_ascii_release_type(coda_asciiType *type);

coda_asciiInteger *coda_ascii_integer_new(void);
int coda_ascii_integer_set_unit(coda_asciiInteger *integer, const char *unit);
int coda_ascii_integer_set_byte_size(coda_asciiInteger *integer, long byte_size);
int coda_ascii_integer_set_read_type(coda_asciiInteger *integer, coda_native_type read_type);
int coda_ascii_integer_set_conversion(coda_asciiInteger *integer, coda_Conversion *conversion);
int coda_ascii_integer_add_mapping(coda_asciiInteger *integer, coda_asciiIntegerMapping *mapping);
int coda_ascii_integer_validate(coda_asciiInteger *integer);

coda_asciiFloat *coda_ascii_float_new(void);
int coda_ascii_float_set_unit(coda_asciiFloat *fl, const char *unit);
int coda_ascii_float_set_byte_size(coda_asciiFloat *fl, long byte_size);
int coda_ascii_float_set_read_type(coda_asciiFloat *fl, coda_native_type read_type);
int coda_ascii_float_set_conversion(coda_asciiFloat *fl, coda_Conversion *conversion);
int coda_ascii_float_add_mapping(coda_asciiFloat *fl, coda_asciiFloatMapping *mapping);
int coda_ascii_float_validate(coda_asciiFloat *fl);

coda_asciiText *coda_ascii_text_new(void);
int coda_ascii_text_set_byte_size(coda_asciiText *text, int64_t byte_size);
int coda_ascii_text_set_byte_size_expression(coda_asciiText *text, coda_Expr *byte_size_expr);
int coda_ascii_text_set_read_type(coda_asciiText *text, coda_native_type read_type);
int coda_ascii_text_set_fixed_value(coda_asciiText *text, const char *fixed_value);
int coda_ascii_text_validate(coda_asciiText *text);

coda_asciiLineSeparator *coda_ascii_line_separator_new(void);

coda_asciiLine *coda_ascii_line_new(int include_eol);

coda_asciiWhiteSpace *coda_ascii_white_space_new(void);

coda_asciiTime *coda_ascii_time_new(const char *format);
int coda_ascii_time_add_mapping(coda_asciiTime *time, coda_asciiFloatMapping *mapping);

coda_asciiIntegerMapping *coda_ascii_integer_mapping_new(const char *str, int64_t value);
void coda_ascii_integer_mapping_delete(coda_asciiIntegerMapping *mapping);

coda_asciiFloatMapping *coda_ascii_float_mapping_new(const char *str, double value);
void coda_ascii_float_mapping_delete(coda_asciiFloatMapping *mapping);

#endif
