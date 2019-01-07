/*
 * Copyright (C) 2007-2019 S[&]T, The Netherlands.
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

#ifndef CODA_NETCDF_INTERNAL_H
#define CODA_NETCDF_INTERNAL_H

#include "coda-netcdf.h"
#include "coda-bin-internal.h"
#include "coda-mem-internal.h"

typedef struct coda_netcdf_type_struct
{
    coda_backend backend;
    coda_type *definition;
    coda_mem_record *attributes;
} coda_netcdf_type;

typedef struct coda_netcdf_array_struct
{
    coda_backend backend;
    coda_type_array *definition;
    coda_mem_record *attributes;
    struct coda_netcdf_basic_type_struct *base_type;
} coda_netcdf_array;

typedef struct coda_netcdf_basic_type_struct
{
    coda_backend backend;
    coda_type *definition;
    coda_mem_record *attributes;
    int64_t offset;
    int record_var;
} coda_netcdf_basic_type;

typedef struct coda_netcdf_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    uint8_t *mem_ptr;

    /* 'netcdf' product specific fields */
    coda_product *raw_product;
    int netcdf_version;
    long record_size;
} coda_netcdf_product;

coda_netcdf_array *coda_netcdf_array_new(int num_dims, long dim[CODA_MAX_NUM_DIMS], coda_netcdf_basic_type *base_type);
int coda_netcdf_array_set_attributes(coda_netcdf_array *type, coda_mem_record *attributes);

coda_netcdf_basic_type *coda_netcdf_basic_type_new(int nc_type, int64_t offset, int record_var, int64_t length);
int coda_netcdf_basic_type_set_attributes(coda_netcdf_basic_type *type, coda_mem_record *attributes);
int coda_netcdf_basic_type_set_conversion(coda_netcdf_basic_type *type, coda_conversion *conversion);

#endif
