/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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
    double scalefactor; /* combination of binaryScaleFactor and decimalScaleFactor */
    double offset;      /* combination of referenceValue and decimalScaleFactor */
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
} coda_grib_product;


coda_grib_value_array *coda_grib_value_array_new(coda_type_array *definition, long num_elements, int64_t byte_offset);
coda_grib_value_array *coda_grib_value_array_simple_packing_new(coda_type_array *definition, long num_elements,
                                                                int64_t byte_offset, int element_bit_size,
                                                                int16_t decimalScaleFactor, int16_t binaryScaleFactor,
                                                                float referenceValue, const uint8_t *bitmask);

#endif
