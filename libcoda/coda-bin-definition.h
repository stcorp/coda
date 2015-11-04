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

#ifndef CODA_BIN_DEFINITION_H
#define CODA_BIN_DEFINITION_H

#include "coda-ascbin-definition.h"
#include "coda-internal.h"
#include "coda-expr.h"

typedef struct coda_binType_struct coda_binType;
typedef struct coda_binInteger_struct coda_binInteger;
typedef struct coda_binFloat_struct coda_binFloat;
typedef struct coda_binRaw_struct coda_binRaw;
typedef struct coda_binVSFInteger_struct coda_binVSFInteger;
typedef struct coda_binTime_struct coda_binTime;
typedef struct coda_binComplex_struct coda_binComplex;

void coda_bin_release_type(coda_binType *type);

coda_binInteger *coda_bin_integer_new(void);
int coda_bin_integer_set_unit(coda_binInteger *integer, const char *unit);
int coda_bin_integer_set_bit_size(coda_binInteger *integer, long bit_size);
int coda_bin_integer_set_read_type(coda_binInteger *integer, coda_native_type read_type);
int coda_bin_integer_set_conversion(coda_binInteger *integer, coda_Conversion *conversion);
int coda_bin_integer_set_endianness(coda_binInteger *integer, coda_endianness endianness);
int coda_bin_integer_validate(coda_binInteger *integer);

coda_binFloat *coda_bin_float_new(void);
int coda_bin_float_set_unit(coda_binFloat *fl, const char *unit);
int coda_bin_float_set_bit_size(coda_binFloat *fl, long bit_size);
int coda_bin_float_set_read_type(coda_binFloat *fl, coda_native_type read_type);
int coda_bin_float_set_conversion(coda_binFloat *fl, coda_Conversion *conversion);
int coda_bin_float_set_endianness(coda_binFloat *integer, coda_endianness endianness);
int coda_bin_float_validate(coda_binFloat *fl);

coda_binRaw *coda_bin_raw_new(void);
int coda_bin_raw_set_bit_size(coda_binRaw *raw, int64_t bit_size);
int coda_bin_raw_set_bit_size_expression(coda_binRaw *raw, coda_Expr *bit_size_expr);
int coda_bin_raw_set_fixed_value(coda_binRaw *raw, long length, char *fixed_value);
int coda_bin_raw_validate(coda_binRaw *raw);

coda_binVSFInteger *coda_bin_vsf_integer_new(void);
int coda_bin_vsf_integer_set_type(coda_binVSFInteger *integer, coda_binType *base_type);
int coda_bin_vsf_integer_set_scale_factor(coda_binVSFInteger *integer, coda_binType *scale_factor);
int coda_bin_vsf_integer_set_unit(coda_binVSFInteger *integer, const char *unit);
int coda_bin_vsf_integer_validate(coda_binVSFInteger *integer);

coda_binTime *coda_bin_time_new(const char *format);

coda_binComplex *coda_bin_complex_new(void);
int coda_bin_complex_set_type(coda_binComplex *compl, coda_binType *type);
int coda_bin_complex_validate(coda_binComplex *compl);

#endif
