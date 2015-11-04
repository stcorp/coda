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

#ifndef CODA_ASCBIN_H
#define CODA_ASCBIN_H

#include "coda-internal.h"

coda_dynamic_type *coda_ascbin_empty_record(void);

void coda_ascbin_done(void);
int coda_ascbin_open(const char *filename, int64_t file_size, coda_product **product);
int coda_ascbin_close(coda_product *product);

int coda_ascbin_recognize_file(const char *filename, int64_t size, coda_product_definition **definition);

int coda_ascbin_type_get_num_record_fields(const coda_type *type, long *num_fields);
int coda_ascbin_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index);
int coda_ascbin_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type);
int coda_ascbin_type_get_record_field_name(const coda_type *type, long index, const char **name);
int coda_ascbin_type_get_record_field_real_name(const coda_type *type, long index, const char **real_name);
int coda_ascbin_type_get_record_field_hidden_status(const coda_type *type, long index, int *hidden);
int coda_ascbin_type_get_record_field_available_status(const coda_type *type, long index, int *available);
int coda_ascbin_type_get_record_union_status(const coda_type *type, int *is_union);
int coda_ascbin_type_get_array_num_dims(const coda_type *type, int *num_dims);
int coda_ascbin_type_get_array_dim(const coda_type *type, int *num_dims, long dim[]);
int coda_ascbin_type_get_array_base_type(const coda_type *type, coda_type **base_type);

int coda_ascbin_cursor_set_product(coda_cursor *cursor, coda_product *product);
int coda_ascbin_cursor_goto_record_field_by_index(coda_cursor *cursor, long index);
int coda_ascbin_cursor_goto_next_record_field(coda_cursor *cursor);
int coda_ascbin_cursor_goto_available_union_field(coda_cursor *cursor);
int coda_ascbin_cursor_goto_union_field_by_index(coda_cursor *cursor, long index);
int coda_ascbin_cursor_goto_next_union_field(coda_cursor *cursor);
int coda_ascbin_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[]);
int coda_ascbin_cursor_goto_array_element_by_index(coda_cursor *cursor, long index);
int coda_ascbin_cursor_goto_next_array_element(coda_cursor *cursor);
int coda_ascbin_cursor_goto_attributes(coda_cursor *cursor);
int coda_ascbin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size);
int coda_ascbin_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements);
int coda_ascbin_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available);
int coda_ascbin_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index);
int coda_ascbin_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[]);

#endif
