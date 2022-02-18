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

#include "coda-grib-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* return a ^ b, where a and b are integers and the result is a floating point value */
static double fpow(long a, long b)
{
    double r = 1.0;

    if (b < 0)
    {
        b = -b;
        while (b--)
        {
            r *= a;
        }
        return 1.0 / r;
    }
    while (b--)
    {
        r *= a;
    }
    return r;
}

void coda_grib_type_delete(coda_dynamic_type *type)
{
    assert(type != NULL);
    assert(type->backend == coda_backend_grib);

    if (type->definition->type_class == coda_array_class)
    {
        if (((coda_grib_value_array *)type)->base_type != NULL)
        {
            coda_dynamic_type_delete(((coda_grib_value_array *)type)->base_type);
        }
        if (((coda_grib_value_array *)type)->bitmask != NULL)
        {
            free(((coda_grib_value_array *)type)->bitmask);
        }
        if (((coda_grib_value_array *)type)->bitmask_cumsum128 != NULL)
        {
            free(((coda_grib_value_array *)type)->bitmask_cumsum128);
        }
    }
    if (type->definition != NULL)
    {
        coda_type_release((coda_type *)type->definition);
    }
    free(type);
}

coda_grib_value_array *coda_grib_value_array_new(coda_type_array *definition, long num_elements, int64_t byte_offset)
{
    coda_grib_value_array *type;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (definition->base_type->type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "base type for GRIB value array should be 'real' and not '%s'",
                       coda_type_get_class_name(definition->base_type->type_class));
        return NULL;
    }

    type = (coda_grib_value_array *)malloc(sizeof(coda_grib_value_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_grib_value_array), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_grib;
    type->definition = definition;
    definition->retain_count++;
    type->num_elements = num_elements;
    type->base_type = NULL;
    type->bit_offset = 8 * byte_offset;
    type->simple_packing = 0;
    type->element_bit_size = 32;
    type->decimalScaleFactor = 0;
    type->binaryScaleFactor = 0;
    type->referenceValue = 0.0;
    type->scalefactor = 0.0;
    type->offset = 0.0;
    type->bitmask = NULL;
    type->bitmask_cumsum128 = NULL;

    type->base_type = (coda_dynamic_type *)malloc(sizeof(coda_dynamic_type));
    if (type->base_type == NULL)
    {
        coda_grib_type_delete((coda_dynamic_type *)type);
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_dynamic_type), __FILE__, __LINE__);
        return NULL;
    }
    type->base_type->backend = coda_backend_grib;
    type->base_type->definition = definition->base_type;
    definition->base_type->retain_count++;

    return type;
}

coda_grib_value_array *coda_grib_value_array_simple_packing_new(coda_type_array *definition, long num_elements,
                                                                int64_t byte_offset, int element_bit_size,
                                                                int16_t decimalScaleFactor, int16_t binaryScaleFactor,
                                                                float referenceValue, const uint8_t *bitmask)
{
    coda_grib_value_array *type;
    long bitmask_size;
    long i;

    type = coda_grib_value_array_new(definition, num_elements, byte_offset);
    if (type == NULL)
    {
        return NULL;
    }

    type->simple_packing = 1;
    type->element_bit_size = element_bit_size;
    type->decimalScaleFactor = decimalScaleFactor;
    type->binaryScaleFactor = binaryScaleFactor;
    type->referenceValue = referenceValue;
    type->scalefactor = fpow(2, binaryScaleFactor) * fpow(10, -decimalScaleFactor);
    type->offset = referenceValue * fpow(10, -decimalScaleFactor);
    type->bitmask = NULL;
    type->bitmask_cumsum128 = NULL;

    if (bitmask != NULL)
    {
        bitmask_size = bit_size_to_byte_size(num_elements);
        type->bitmask = (uint8_t *)malloc(bitmask_size * sizeof(uint8_t));
        if (type->bitmask == NULL)
        {
            coda_grib_type_delete((coda_dynamic_type *)type);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(bitmask_size * sizeof(uint8_t)), __FILE__, __LINE__);
            return NULL;
        }
        memcpy(type->bitmask, bitmask, bitmask_size);

        type->bitmask_cumsum128 = (uint8_t *)malloc(bitmask_size * sizeof(uint8_t));
        if (type->bitmask_cumsum128 == NULL)
        {
            coda_grib_type_delete((coda_dynamic_type *)type);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(bitmask_size * sizeof(uint8_t)), __FILE__, __LINE__);
            return NULL;
        }
        for (i = 0; i < bitmask_size; i++)
        {
            uint8_t bm = type->bitmask[i];

            type->bitmask_cumsum128[i] = ((bm >> 7) & 1) + ((bm >> 6) & 1) + ((bm >> 5) & 1) + ((bm >> 4) & 1) +
                ((bm >> 3) & 1) + ((bm >> 2) & 1) + ((bm >> 1) & 1) + (bm & 1);
            if (i % 16 != 0)
            {
                type->bitmask_cumsum128[i] += type->bitmask_cumsum128[i - 1];
            }
        }
    }

    return type;
}
