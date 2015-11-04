/*
 * Copyright (C) 2009-2014 S[&]T, The Netherlands.
 *
 * This file is part of the QIAP Toolkit.
 *
 * The QIAP Toolkit is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The QIAP Toolkit is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the QIAP Toolkit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CODA_QIAP_H
#define CODA_QIAP_H

/** \file */

#include "qiap.h"

LIBCODA_API int coda_set_option_enable_qiap(int enable);
LIBCODA_API int coda_get_option_enable_qiap(void);

/* can also be set using CODA_QIAP_REPORT environment variable */
/* must be used before calling coda_init() */
LIBCODA_API int coda_qiap_set_report(const char *path);

/* can also be set using CODA_QIAP_LOG environment variable */
/* must be used before calling coda_init() */
LIBCODA_API int coda_qiap_set_action_log(const char *path);

LIBCODA_API int coda_qiap_find_affected_product(coda_product *product, const qiap_quality_issue *quality_issue,
                                                qiap_affected_product **affected_product);


/* the functions below are called by the coda library. Don't call these yourself */
int coda_qiap_init(void);
void coda_qiap_done(void);
void coda_qiap_add_error_message(void);
int coda_qiap_init_actions(coda_product *product);
void coda_qiap_delete_actions(coda_product *product);
int coda_qiap_perform_actions_for_int8(const coda_cursor *cursor, int8_t *dst);
int coda_qiap_perform_actions_for_uint8(const coda_cursor *cursor, uint8_t *dst);
int coda_qiap_perform_actions_for_int16(const coda_cursor *cursor, int16_t *dst);
int coda_qiap_perform_actions_for_uint16(const coda_cursor *cursor, uint16_t *dst);
int coda_qiap_perform_actions_for_int32(const coda_cursor *cursor, int32_t *dst);
int coda_qiap_perform_actions_for_uint32(const coda_cursor *cursor, uint32_t *dst);
int coda_qiap_perform_actions_for_int64(const coda_cursor *cursor, int64_t *dst);
int coda_qiap_perform_actions_for_uint64(const coda_cursor *cursor, uint64_t *dst);
int coda_qiap_perform_actions_for_float(const coda_cursor *cursor, float *dst);
int coda_qiap_perform_actions_for_double(const coda_cursor *cursor, double *dst);
int coda_qiap_perform_actions_for_char(const coda_cursor *cursor, char *dst);
int coda_qiap_perform_actions_for_string(const coda_cursor *cursor, char *dst, long dst_size);
int coda_qiap_perform_actions_for_int8_array(const coda_cursor *cursor, int8_t *dst);
int coda_qiap_perform_actions_for_uint8_array(const coda_cursor *cursor, uint8_t *dst);
int coda_qiap_perform_actions_for_int16_array(const coda_cursor *cursor, int16_t *dst);
int coda_qiap_perform_actions_for_uint16_array(const coda_cursor *cursor, uint16_t *dst);
int coda_qiap_perform_actions_for_int32_array(const coda_cursor *cursor, int32_t *dst);
int coda_qiap_perform_actions_for_uint32_array(const coda_cursor *cursor, uint32_t *dst);
int coda_qiap_perform_actions_for_int64_array(const coda_cursor *cursor, int64_t *dst);
int coda_qiap_perform_actions_for_uint64_array(const coda_cursor *cursor, uint64_t *dst);
int coda_qiap_perform_actions_for_float_array(const coda_cursor *cursor, float *dst);
int coda_qiap_perform_actions_for_double_array(const coda_cursor *cursor, double *dst);
int coda_qiap_perform_actions_for_char_array(const coda_cursor *cursor, char *dst);

#endif
