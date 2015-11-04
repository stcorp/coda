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

#ifndef CODA_CDF_INTERNAL_H
#define CODA_CDF_INTERNAL_H

#include "coda-cdf.h"

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
#if CODA_USE_QIAP
    void *qiap_info;
#endif

    int use_mmap;       /* indicates whether the file was opened using mmap */
    int fd;     /* file handle when not using mmap */
#ifdef WIN32
    HANDLE file;
    HANDLE file_mapping;
#endif
    const uint8_t *mmap_ptr;

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
