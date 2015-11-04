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

#ifndef CODA_XML_H
#define CODA_XML_H

#include "coda-internal.h"

void coda_xml_done(void);

int coda_xml_recognize_file(const char *filename, int64_t size, coda_product_definition **definition);
int coda_xml_open(const char *filename, int64_t file_size, coda_product **product);
int coda_xml_close(coda_product *product);

int coda_xml_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type, coda_type **type);

int coda_xml_type_has_ascii_content(const coda_type *type, int *has_ascii_content);
int coda_xml_type_get_read_type(const coda_type *type, coda_native_type *read_type);
int coda_xml_type_get_string_length(const coda_type *type, long *length);
int coda_xml_type_get_bit_size(const coda_type *type, int64_t *bit_size);
int coda_xml_type_get_unit(const coda_type *type, const char **unit);
int coda_xml_type_get_fixed_value(const coda_type *type, const char **fixed_value, long *length);
int coda_xml_type_get_num_record_fields(const coda_type *type, long *num_fields);
int coda_xml_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index);
int coda_xml_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type);
int coda_xml_type_get_record_field_name(const coda_type *type, long index, const char **name);
int coda_xml_type_get_record_field_hidden_status(const coda_type *type, long index, int *hidden);
int coda_xml_type_get_record_field_available_status(const coda_type *type, long index, int *available);
int coda_xml_type_get_record_union_status(const coda_type *type, int *is_union);
int coda_xml_type_get_array_num_dims(const coda_type *type, int *num_dims);
int coda_xml_type_get_array_dim(const coda_type *type, int *num_dims, long dim[]);
int coda_xml_type_get_array_base_type(const coda_type *type, coda_type **base_type);
int coda_xml_type_get_special_type(const coda_type *type, coda_special_type *special_type);
int coda_xml_type_get_special_base_type(const coda_type *type, coda_type **base_type);

int coda_xml_cursor_set_product(coda_cursor *cursor, coda_product *product);
int coda_xml_cursor_goto_record_field_by_index(coda_cursor *cursor, long index);
int coda_xml_cursor_goto_next_record_field(coda_cursor *cursor);
int coda_xml_cursor_goto_available_union_field(coda_cursor *cursor);
int coda_xml_cursor_goto_union_field_by_index(coda_cursor *cursor, long index);
int coda_xml_cursor_goto_next_union_field(coda_cursor *cursor);
int coda_xml_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[]);
int coda_xml_cursor_goto_array_element_by_index(coda_cursor *cursor, long index);
int coda_xml_cursor_goto_next_array_element(coda_cursor *cursor);
int coda_xml_cursor_goto_attributes(coda_cursor *cursor);
int coda_xml_cursor_use_base_type_of_special_type(coda_cursor *cursor);
int coda_xml_cursor_get_string_length(const coda_cursor *cursor, long *length);
int coda_xml_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size);
int coda_xml_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements);
int coda_xml_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available);
int coda_xml_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index);
int coda_xml_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[]);

int coda_xml_cursor_read_int8(const coda_cursor *cursor, int8_t *dst);
int coda_xml_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst);
int coda_xml_cursor_read_int16(const coda_cursor *cursor, int16_t *dst);
int coda_xml_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst);
int coda_xml_cursor_read_int32(const coda_cursor *cursor, int32_t *dst);
int coda_xml_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst);
int coda_xml_cursor_read_int64(const coda_cursor *cursor, int64_t *dst);
int coda_xml_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst);
int coda_xml_cursor_read_float(const coda_cursor *cursor, float *dst);
int coda_xml_cursor_read_double(const coda_cursor *cursor, double *dst);
int coda_xml_cursor_read_char(const coda_cursor *cursor, char *dst);
int coda_xml_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size);
int coda_xml_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length);

int coda_xml_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering);
int coda_xml_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering);

#endif
