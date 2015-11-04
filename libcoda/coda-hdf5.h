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

#ifndef CODA_HDF5_H
#define CODA_HDF5_H

#include "coda-internal.h"

int coda_hdf5_init(void);
void coda_hdf5_done(void);

void coda_hdf5_add_error_message(void);

int coda_hdf5_open(const char *filename, int64_t file_size, coda_ProductFile **pf);
int coda_hdf5_close(coda_ProductFile *pf);

int coda_hdf5_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type);

int coda_hdf5_type_get_read_type(const coda_Type *type, coda_native_type *read_type);
int coda_hdf5_type_get_string_length(const coda_Type *type, long *length);
int coda_hdf5_type_get_num_record_fields(const coda_Type *type, long *num_fields);
int coda_hdf5_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index);
int coda_hdf5_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type);
int coda_hdf5_type_get_record_field_name(const coda_Type *type, long index, const char **name);
int coda_hdf5_type_get_array_num_dims(const coda_Type *type, int *num_dims);
int coda_hdf5_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[]);
int coda_hdf5_type_get_array_base_type(const coda_Type *type, coda_Type **base_type);

int coda_hdf5_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf);
int coda_hdf5_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index);
int coda_hdf5_cursor_goto_next_record_field(coda_Cursor *cursor);
int coda_hdf5_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[]);
int coda_hdf5_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index);
int coda_hdf5_cursor_goto_next_array_element(coda_Cursor *cursor);
int coda_hdf5_cursor_goto_attributes(coda_Cursor *cursor);
int coda_hdf5_cursor_get_string_length(const coda_Cursor *cursor, long *length);
int coda_hdf5_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements);
int coda_hdf5_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[]);

int coda_hdf5_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst);
int coda_hdf5_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst);
int coda_hdf5_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst);
int coda_hdf5_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst);
int coda_hdf5_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst);
int coda_hdf5_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst);
int coda_hdf5_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst);
int coda_hdf5_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst);
int coda_hdf5_cursor_read_float(const coda_Cursor *cursor, float *dst);
int coda_hdf5_cursor_read_double(const coda_Cursor *cursor, double *dst);
int coda_hdf5_cursor_read_char(const coda_Cursor *cursor, char *dst);
int coda_hdf5_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size);

int coda_hdf5_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_double_array(const coda_Cursor *cursor, double *dst, coda_array_ordering array_ordering);
int coda_hdf5_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering);

#endif
