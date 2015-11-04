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

#ifndef CODA_GRIB_INTERNAL_H
#define CODA_GRIB_INTERNAL_H

#include "coda-grib.h"
#include "coda-type.h"
#include "coda-bin-internal.h"

typedef struct coda_grib_value_array_struct
{
    coda_backend backend;
    coda_type_array *definition;

    long num_elements;
    coda_dynamic_type *base_type;
    int64_t bit_offset;

    int simple_packing; /* if 0, data is interpreted directly as float values, otherwise simple packing is used */
    int element_bit_size;
    int16_t decimalScaleFactor;
    int16_t binaryScaleFactor;
    float referenceValue;
    uint8_t *bitmask;
    uint8_t *bitmask_cumsum128;
} coda_grib_value_array;


typedef struct coda_grib_product_struct
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

    /* 'grib' product specific fields */
    coda_product *raw_product;
    int grib_version;
    long record_size;
} coda_grib_product;


coda_grib_value_array *coda_grib_value_array_new(coda_type_array *definition, long num_elements, int64_t byte_offset);
coda_grib_value_array *coda_grib_value_array_simple_packing_new(coda_type_array *definition, long num_elements,
                                                                int64_t byte_offset, int element_bit_size,
                                                                int16_t decimalScaleFactor, int16_t binaryScaleFactor,
                                                                float referenceValue, const uint8_t *bitmask);

#endif
