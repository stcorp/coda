/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

coda_netcdf_basic_type *coda_netcdf_basic_type_new(int nc_type, int64_t offset, int record_var, int length);
int coda_netcdf_basic_type_set_attributes(coda_netcdf_basic_type *type, coda_mem_record *attributes);
int coda_netcdf_basic_type_set_conversion(coda_netcdf_basic_type *type, coda_conversion *conversion);

#endif
