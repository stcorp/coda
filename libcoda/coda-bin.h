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

#ifndef CODA_BIN_H
#define CODA_BIN_H

#include "coda-internal.h"

coda_dynamic_type *coda_bin_no_data_singleton(void);

void coda_bin_done(void);

int coda_bin_close(coda_product *product);

int coda_bin_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type, coda_type **type);

int coda_bin_type_get_read_type(const coda_type *type, coda_native_type *read_type);
int coda_bin_type_get_bit_size(const coda_type *type, int64_t *bit_size);
int coda_bin_type_get_unit(const coda_type *type, const char **unit);
int coda_bin_type_get_fixed_value(const coda_type *type, const char **fixed_value, long *length);
int coda_bin_type_get_special_type(const coda_type *type, coda_special_type *special_type);
int coda_bin_type_get_special_base_type(const coda_type *type, coda_type **base_type);

int coda_bin_cursor_use_base_type_of_special_type(coda_cursor *cursor);
int coda_bin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size);
int coda_bin_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements);

int coda_bin_cursor_read_int8(const coda_cursor *cursor, int8_t *dst);
int coda_bin_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst);
int coda_bin_cursor_read_int16(const coda_cursor *cursor, int16_t *dst);
int coda_bin_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst);
int coda_bin_cursor_read_int32(const coda_cursor *cursor, int32_t *dst);
int coda_bin_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst);
int coda_bin_cursor_read_int64(const coda_cursor *cursor, int64_t *dst);
int coda_bin_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst);
int coda_bin_cursor_read_float(const coda_cursor *cursor, float *dst);
int coda_bin_cursor_read_double(const coda_cursor *cursor, double *dst);
int coda_bin_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length);
int coda_bin_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length);
int coda_bin_cursor_read_double_pair(const coda_cursor *cursor, double *dst);

int coda_bin_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_double_pairs_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering);
int coda_bin_cursor_read_double_split_array(const coda_cursor *cursor, double *dst_1, double *dst_2,
                                            coda_array_ordering array_ordering);

#endif
