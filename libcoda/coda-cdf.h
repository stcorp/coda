/*
 * Copyright (C) 2007-2014 S[&]T, The Netherlands.
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

#ifndef CODA_CDF_H
#define CODA_CDF_H

#include "coda-internal.h"

int coda_cdf_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                  coda_product **product);
int coda_cdf_close(coda_product *product);

void coda_cdf_type_delete(coda_dynamic_type *type);

int coda_cdf_cursor_set_product(coda_cursor *cursor, coda_product *product);
int coda_cdf_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[]);
int coda_cdf_cursor_goto_array_element_by_index(coda_cursor *cursor, long index);
int coda_cdf_cursor_goto_next_array_element(coda_cursor *cursor);
int coda_cdf_cursor_goto_attributes(coda_cursor *cursor);
int coda_cdf_cursor_use_base_type_of_special_type(coda_cursor *cursor);
int coda_cdf_cursor_get_string_length(const coda_cursor *cursor, long *length);
int coda_cdf_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements);
int coda_cdf_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[]);

int coda_cdf_cursor_read_int8(const coda_cursor *cursor, int8_t *dst);
int coda_cdf_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst);
int coda_cdf_cursor_read_int16(const coda_cursor *cursor, int16_t *dst);
int coda_cdf_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst);
int coda_cdf_cursor_read_int32(const coda_cursor *cursor, int32_t *dst);
int coda_cdf_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst);
int coda_cdf_cursor_read_int64(const coda_cursor *cursor, int64_t *dst);
int coda_cdf_cursor_read_float(const coda_cursor *cursor, float *dst);
int coda_cdf_cursor_read_double(const coda_cursor *cursor, double *dst);
int coda_cdf_cursor_read_char(const coda_cursor *cursor, char *dst);
int coda_cdf_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size);

int coda_cdf_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst);
int coda_cdf_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst);
int coda_cdf_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst);
int coda_cdf_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst);
int coda_cdf_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst);
int coda_cdf_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst);
int coda_cdf_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst);
int coda_cdf_cursor_read_float_array(const coda_cursor *cursor, float *dst);
int coda_cdf_cursor_read_double_array(const coda_cursor *cursor, double *dst);
int coda_cdf_cursor_read_char_array(const coda_cursor *cursor, char *dst);

#endif
