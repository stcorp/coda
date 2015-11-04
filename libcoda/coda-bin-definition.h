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

#ifndef CODA_BIN_DEFINITION_H
#define CODA_BIN_DEFINITION_H

#include "coda-ascbin-definition.h"
#include "coda-internal.h"

typedef struct coda_bin_type_struct coda_bin_type;
typedef struct coda_bin_integer_struct coda_bin_integer;
typedef struct coda_bin_float_struct coda_bin_float;
typedef struct coda_bin_raw_struct coda_bin_raw;
typedef struct coda_bin_vsf_integer_struct coda_bin_vsf_integer;
typedef struct coda_bin_time_struct coda_bin_time;
typedef struct coda_bin_complex_struct coda_bin_complex;

void coda_bin_release_type(coda_bin_type *type);

coda_bin_integer *coda_bin_integer_new(void);
int coda_bin_integer_set_unit(coda_bin_integer *integer, const char *unit);
int coda_bin_integer_set_bit_size(coda_bin_integer *integer, long bit_size);
int coda_bin_integer_set_bit_size_expression(coda_bin_integer *integer, coda_expression *bit_size_expr);
int coda_bin_integer_set_read_type(coda_bin_integer *integer, coda_native_type read_type);
int coda_bin_integer_set_conversion(coda_bin_integer *integer, coda_conversion *conversion);
int coda_bin_integer_set_endianness(coda_bin_integer *integer, coda_endianness endianness);
int coda_bin_integer_validate(coda_bin_integer *integer);

coda_bin_float *coda_bin_float_new(void);
int coda_bin_float_set_unit(coda_bin_float *fl, const char *unit);
int coda_bin_float_set_bit_size(coda_bin_float *fl, long bit_size);
int coda_bin_float_set_read_type(coda_bin_float *fl, coda_native_type read_type);
int coda_bin_float_set_conversion(coda_bin_float *fl, coda_conversion *conversion);
int coda_bin_float_set_endianness(coda_bin_float *integer, coda_endianness endianness);
int coda_bin_float_validate(coda_bin_float *fl);

coda_bin_raw *coda_bin_raw_new(void);
int coda_bin_raw_set_bit_size(coda_bin_raw *raw, int64_t bit_size);
int coda_bin_raw_set_bit_size_expression(coda_bin_raw *raw, coda_expression *bit_size_expr);
int coda_bin_raw_set_fixed_value(coda_bin_raw *raw, long length, char *fixed_value);
int coda_bin_raw_validate(coda_bin_raw *raw);

coda_bin_vsf_integer *coda_bin_vsf_integer_new(void);
int coda_bin_vsf_integer_set_type(coda_bin_vsf_integer *integer, coda_bin_type *base_type);
int coda_bin_vsf_integer_set_scale_factor(coda_bin_vsf_integer *integer, coda_bin_type *scale_factor);
int coda_bin_vsf_integer_set_unit(coda_bin_vsf_integer *integer, const char *unit);
int coda_bin_vsf_integer_validate(coda_bin_vsf_integer *integer);

coda_bin_time *coda_bin_time_new(const char *format);

coda_bin_complex *coda_bin_complex_new(void);
int coda_bin_complex_set_type(coda_bin_complex *compl, coda_bin_type *type);
int coda_bin_complex_validate(coda_bin_complex *compl);

#endif
