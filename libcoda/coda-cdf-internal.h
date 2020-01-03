/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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

#ifndef CODA_CDF_INTERNAL_H
#define CODA_CDF_INTERNAL_H

#include "coda-cdf.h"

#include "coda-bin-internal.h"
#include "coda-mem-internal.h"

typedef enum cdf_type_tag_enum
{
    tag_cdf_basic_type,
    tag_cdf_time,
    tag_cdf_variable
} cdf_type_tag;

typedef struct coda_cdf_type_struct
{
    coda_backend backend;
    coda_type *definition;
    cdf_type_tag tag;
} coda_cdf_type;

typedef struct coda_cdf_time_struct
{
    coda_backend backend;
    coda_type_special *definition;
    cdf_type_tag tag;
    coda_dynamic_type *base_type;
    int32_t data_type;
} coda_cdf_time;

typedef struct coda_cdf_variable_struct
{
    coda_backend backend;
    coda_type_array *definition;
    cdf_type_tag tag;
    coda_mem_record *attributes;
    coda_cdf_type *base_type;
    int num_records;
    int num_values_per_record;
    int value_size;
    int sparse_rec_method;      /* 0: no sparse records, 1: padded sparse records, 2: previous sparse records */
    int64_t *offset;    /* file offset for each record - will be offset into 'data' if 'data != NULL' */
    int8_t *data;
} coda_cdf_variable;

typedef struct coda_cdf_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_mem_record *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    uint8_t *mem_ptr;

    /* 'cdf' product specific fields */
    coda_product *raw_product;
    int32_t cdf_version;
    int32_t cdf_release;
    int32_t cdf_increment;
    coda_endianness endianness;
    coda_array_ordering array_ordering;
    int has_md5_chksum;
    int32_t rnum_dims;
    int32_t rdim_sizes[CODA_MAX_NUM_DIMS];
} coda_cdf_product;

coda_dynamic_type *coda_cdf_variable_new(int32_t data_type, int32_t max_rec, int32_t rec_varys, int32_t num_dims,
                                         int32_t dim[CODA_MAX_NUM_DIMS], int32_t dim_varys[CODA_MAX_NUM_DIMS],
                                         coda_array_ordering array_ordering, int32_t num_elements,
                                         int sparse_rec_method, coda_cdf_variable **variable);

int coda_cdf_variable_add_attribute(coda_cdf_variable *type, const char *real_name, coda_dynamic_type *attribute_type,
                                    int update_definition);

#endif
