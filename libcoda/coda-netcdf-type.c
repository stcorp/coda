/*
 * Copyright (C) 2007-2018 S[&]T, The Netherlands.
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

#include "coda-netcdf-internal.h"

#include <assert.h>
#include <stdlib.h>

void coda_netcdf_type_delete(coda_dynamic_type *type)
{
    assert(type != NULL);
    assert(type->backend == coda_backend_netcdf);

    if (type->definition->type_class == coda_array_class)
    {
        if (((coda_netcdf_array *)type)->base_type != NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_netcdf_array *)type)->base_type);
        }
    }
    if (((coda_netcdf_type *)type)->attributes != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)((coda_netcdf_type *)type)->attributes);
    }
    if (type->definition != NULL)
    {
        coda_type_release((coda_type *)type->definition);
    }
    free(type);
}

coda_netcdf_array *coda_netcdf_array_new(int num_dims, long dim[CODA_MAX_NUM_DIMS], coda_netcdf_basic_type *base_type)
{
    coda_netcdf_array *type;
    int i;

    type = malloc(sizeof(coda_netcdf_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_array), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_netcdf;
    type->definition = NULL;
    type->attributes = NULL;
    type->base_type = NULL;

    type->definition = coda_type_array_new(coda_format_netcdf);
    if (type->definition == NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_array_set_base_type(type->definition, base_type->definition) != 0)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    for (i = 0; i < num_dims; i++)
    {
        if (coda_type_array_add_fixed_dimension(type->definition, dim[i]) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    type->base_type = base_type;

    return type;
}

int coda_netcdf_array_set_attributes(coda_netcdf_array *type, coda_mem_record *attributes)
{
    assert(type->attributes == NULL);
    if (coda_type_set_attributes((coda_type *)type->definition, attributes->definition) != 0)
    {
        return -1;
    }
    type->attributes = attributes;

    return 0;
}

coda_netcdf_basic_type *coda_netcdf_basic_type_new(int nc_type, int64_t offset, int record_var, int length)
{
    coda_netcdf_basic_type *type;
    coda_native_type read_type;
    int byte_size;

    type = malloc(sizeof(coda_netcdf_basic_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_netcdf_basic_type), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_netcdf;
    type->definition = NULL;
    type->attributes = NULL;
    type->offset = offset;
    type->record_var = record_var;

    switch (nc_type)
    {
        case 1:
            read_type = coda_native_type_int8;
            byte_size = 1;
            type->definition = (coda_type *)coda_type_number_new(coda_format_netcdf, coda_integer_class);
            break;
        case 2:
            read_type = (length > 1) ? coda_native_type_string : coda_native_type_char;
            byte_size = length;
            type->definition = (coda_type *)coda_type_text_new(coda_format_netcdf);
            break;
        case 3:
            read_type = coda_native_type_int16;
            byte_size = 2;
            type->definition = (coda_type *)coda_type_number_new(coda_format_netcdf, coda_integer_class);
            break;
        case 4:
            read_type = coda_native_type_int32;
            byte_size = 4;
            type->definition = (coda_type *)coda_type_number_new(coda_format_netcdf, coda_integer_class);
            break;
        case 5:
            read_type = coda_native_type_float;
            byte_size = 4;
            type->definition = (coda_type *)coda_type_number_new(coda_format_netcdf, coda_real_class);
            break;
        case 6:
            read_type = coda_native_type_double;
            byte_size = 8;
            type->definition = (coda_type *)coda_type_number_new(coda_format_netcdf, coda_real_class);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (type->definition == NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_read_type(type->definition, read_type) != 0)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_byte_size(type->definition, byte_size) != 0)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

int coda_netcdf_basic_type_set_conversion(coda_netcdf_basic_type *type, coda_conversion *conversion)
{
    assert(type->definition->type_class == coda_integer_class || type->definition->type_class == coda_real_class);
    return coda_type_number_set_conversion((coda_type_number *)type->definition, conversion);
}

int coda_netcdf_basic_type_set_attributes(coda_netcdf_basic_type *type, coda_mem_record *attributes)
{
    assert(type->attributes == NULL);
    if (coda_type_set_attributes(type->definition, attributes->definition) != 0)
    {
        return -1;
    }
    type->attributes = attributes;

    return 0;
}
