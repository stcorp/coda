/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-read-bits.h"
#ifndef WORDS_BIGENDIAN
#include "coda-swap4.h"
#include "coda-swap8.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-grib-internal.h"
#include "coda-bin.h"

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

int coda_grib_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;

    return 0;
}

int coda_grib_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    /* check the number of dimensions */
    if (num_subs != 1)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (1) (%s:%u)", num_subs,
                       __FILE__, __LINE__);
        return -1;
    }
    return coda_grib_cursor_goto_array_element_by_index(cursor, subs[0]);
}

int coda_grib_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_grib_value_array *type = (coda_grib_value_array *)cursor->stack[cursor->n - 1].type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        if (index < 0 || index >= type->num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, type->num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = type->base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;

    return 0;
}

int coda_grib_cursor_goto_next_array_element(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_grib_cursor_goto_array_element_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_grib_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_format format = cursor->stack[cursor->n - 1].type->definition->format;

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)coda_mem_empty_record(format);
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* virtual types do not have a bit offset */
    return 0;
}

int coda_grib_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    if (cursor->stack[cursor->n - 1].type->definition->type_class == coda_array_class)
    {
        *num_elements = ((coda_grib_value_array *)cursor->stack[cursor->n - 1].type)->num_elements;
    }
    else
    {
        *num_elements = 1;
    }
    return 0;
}

int coda_grib_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    *num_dims = 1;
    return coda_grib_cursor_get_num_elements(cursor, dim);
}

int coda_grib_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_grib_value_array *array;
    long index;

    assert(cursor->n > 1);
    array = (coda_grib_value_array *)cursor->stack[cursor->n - 2].type;
    assert(array->definition->type_class == coda_array_class);
    index = cursor->stack[cursor->n - 1].index;
    if (array->simple_packing)
    {
        int64_t ivalue = 0;
        double fvalue = 0;
        uint8_t *buffer;

        fvalue = array->referenceValue;
        if (array->element_bit_size == 0)
        {
            *((float *)dst) = (float)fvalue;
            return 0;
        }
        if (array->bitmask != NULL)
        {
            uint8_t bm;
            long bm_index;
            long value_index = 0;
            long i;

            bm_index = index >> 3;
            bm = array->bitmask[bm_index];
            if (!((bm >> (7 - (index & 0x7))) & 1))
            {
                /* bitmask value is 0 -> return NaN */
                *((float *)dst) = (float)coda_NaN();
                return 0;
            }

            /* bitmask value is 1 -> update index to be the index in the value array */
            for (i = 0; i < bm_index >> 4; i++)
            {
                /* advance value_index based on cumsum of blocks of 128 bitmap bits (= 16 bytes) */
                value_index += array->bitmask_cumsum128[16 * i + 15];
            }
            if (bm_index % 16 != 0)
            {
                value_index += array->bitmask_cumsum128[bm_index - 1];
            }
            bm = array->bitmask[bm_index];
            for (i = 0; i < (index & 0x7); i++)
            {
                value_index += (bm >> (7 - i)) & 1;
            }
            index = value_index;
        }
        buffer = &((uint8_t *)&ivalue)[8 - bit_size_to_byte_size(array->element_bit_size)];
        if (read_bits(((coda_grib_product *)cursor->product)->raw_product,
                      array->bit_offset + index * array->element_bit_size, array->element_bit_size, buffer) != 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap8(&ivalue);
#endif
        fvalue += ivalue * fpow(2, array->binaryScaleFactor);
        fvalue *= fpow(10, -array->decimalScaleFactor);
        *((float *)dst) = (float)fvalue;
    }
    else
    {
        if (read_bytes(((coda_grib_product *)cursor->product)->raw_product, (array->bit_offset >> 3) + index * 4, 4,
                       dst) != 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_float(dst);
#endif
    }

    return 0;
}

int coda_grib_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    coda_grib_value_array *array = (coda_grib_value_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor element_cursor = *cursor;
        long i;

        element_cursor.n++;
        element_cursor.stack[element_cursor.n - 1].type = array->base_type;
        element_cursor.stack[element_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            element_cursor.stack[element_cursor.n - 1].index = i;
            if (coda_grib_cursor_read_float(&element_cursor, &((float *)dst)[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_grib_cursor_read_float_partial_array(const coda_cursor *cursor, long offset, long length, float *dst)
{
    coda_grib_value_array *array = (coda_grib_value_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor element_cursor = *cursor;
        long i;

        element_cursor.n++;
        element_cursor.stack[element_cursor.n - 1].type = array->base_type;
        element_cursor.stack[element_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < length; i++)
        {
            element_cursor.stack[element_cursor.n - 1].index = offset + i;
            if (coda_grib_cursor_read_float(&element_cursor, &((float *)dst)[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}
