/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CODA_ASCBIN_H
#define CODA_ASCBIN_H

#include "coda-internal.h"

int coda_ascbin_cursor_set_product(coda_cursor *cursor, coda_product *product);
int coda_ascbin_cursor_goto_record_field_by_index(coda_cursor *cursor, long index);
int coda_ascbin_cursor_goto_next_record_field(coda_cursor *cursor);
int coda_ascbin_cursor_goto_available_union_field(coda_cursor *cursor);
int coda_ascbin_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[]);
int coda_ascbin_cursor_goto_array_element_by_index(coda_cursor *cursor, long index);
int coda_ascbin_cursor_goto_next_array_element(coda_cursor *cursor);
int coda_ascbin_cursor_goto_attributes(coda_cursor *cursor);
int coda_ascbin_cursor_use_base_type_of_special_type(coda_cursor *cursor);
int coda_ascbin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size);
int coda_ascbin_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements);
int coda_ascbin_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available);
int coda_ascbin_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index);
int coda_ascbin_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[]);

#endif
