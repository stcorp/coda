/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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
#include "coda-read-bytes.h"
#include "coda-swap2.h"
#include "coda-swap4.h"
#include "coda-swap8.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-cdf-internal.h"

int coda_cdf_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for CDF backend */

    return 0;
}

int coda_cdf_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_dynamic_type *base_type;
    long index;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long i;

    if (coda_type_get_array_dim(cursor->stack[cursor->n - 1].type->definition, &num_dims, dim) != 0)
    {
        return -1;
    }

    /* check the number of dimensions */
    if (num_subs != num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d) (%s:%u)", num_subs,
                       num_dims, __FILE__, __LINE__);
        return -1;
    }

    /* check the dimensions... */
    index = 0;
    for (i = 0; i < num_dims; i++)
    {
        if (subs[i] < 0 || subs[i] >= dim[i])
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           subs[i], dim[i], __FILE__, __LINE__);
            return -1;
        }
        if (i > 0)
        {
            index *= dim[i];
        }
        index += subs[i];
    }

    base_type = (coda_dynamic_type *)((coda_cdf_variable *)cursor->stack[cursor->n - 1].type)->base_type;

    cursor->n++;
    cursor->stack[cursor->n - 1].type = base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_cdf_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_dynamic_type *base_type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        num_elements = ((coda_type_array *)cursor->stack[cursor->n - 1].type->definition)->num_elements;
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    base_type = (coda_dynamic_type *)((coda_cdf_variable *)cursor->stack[cursor->n - 1].type)->base_type;

    cursor->n++;
    cursor->stack[cursor->n - 1].type = base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_cdf_cursor_goto_next_array_element(coda_cursor *cursor)
{
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;
        long index;

        index = cursor->stack[cursor->n - 1].index + 1;
        num_elements = ((coda_type_array *)cursor->stack[cursor->n - 2].type->definition)->num_elements;
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    cursor->stack[cursor->n - 1].index++;

    return 0;
}

int coda_cdf_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_cdf_type *type = (coda_cdf_type *)cursor->stack[cursor->n - 1].type;

    cursor->n++;
    if (type->tag == tag_cdf_variable && ((coda_cdf_variable *)type)->attributes != NULL)
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_cdf_variable *)type)->attributes;
    }
    else
    {
        cursor->stack[cursor->n - 1].type = coda_mem_empty_record(coda_format_cdf);
    }
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_cdf_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    cursor->stack[cursor->n - 1].type = ((coda_cdf_time *)cursor->stack[cursor->n - 1].type)->base_type;

    return 0;
}

int coda_cdf_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    if (cursor->stack[cursor->n - 1].type->definition->type_class == coda_array_class)
    {
        *num_elements = ((coda_type_array *)cursor->stack[cursor->n - 1].type->definition)->num_elements;
    }
    else
    {
        *num_elements = 1;
    }

    return 0;
}

int coda_cdf_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    return coda_type_get_string_length(cursor->stack[cursor->n - 1].type->definition, length);
}

int coda_cdf_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    return coda_type_get_array_dim(cursor->stack[cursor->n - 1].type->definition, num_dims, dim);
}

static int read_array(const coda_cursor *cursor, void *dst)
{
    coda_cdf_variable *variable = (coda_cdf_variable *)cursor->stack[cursor->n - 1].type;
    coda_type_class type_class;
    long record_size = variable->num_values_per_record * variable->value_size;
    int i;

    assert(variable->tag == tag_cdf_variable);
    if (variable->base_type == NULL)
    {
        type_class = variable->definition->type_class;
    }
    else
    {
        type_class = variable->base_type->definition->type_class;
    }

    for (i = 0; i < variable->num_records; i++)
    {
        /* TODO: handle sparse records */
        if (variable->offset[i] < 0)
        {
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "Missing record not supported for CDF variable");
            return -1;
        }
        if (variable->data != NULL)
        {
            memcpy(&((uint8_t *)dst)[i * record_size], &variable->data[variable->offset[i]], record_size);
        }
        else
        {
            if (read_bytes(cursor->product, variable->offset[i], record_size, &((uint8_t *)dst)[i * record_size]) != 0)
            {
                return -1;
            }
        }
    }
    if (type_class != coda_text_class)
    {
#ifdef WORDS_BIGENDIAN
        coda_endianness system_endianness = coda_big_endian;
#else
        coda_endianness system_endianness = coda_little_endian;
#endif
        if (((coda_cdf_product *)cursor->product)->endianness != system_endianness)
        {
            switch (variable->value_size)
            {
                case 1:
                    break;
                case 2:
                    for (i = 0; i < variable->num_records * variable->num_values_per_record; i++)
                    {
                        swap2(&((int16_t *)dst)[i]);
                    }
                    break;
                case 4:
                    for (i = 0; i < variable->num_records * variable->num_values_per_record; i++)
                    {
                        swap4(&((int32_t *)dst)[i]);
                    }
                    break;
                case 8:
                    for (i = 0; i < variable->num_records * variable->num_values_per_record; i++)
                    {
                        swap8(&((int64_t *)dst)[i]);
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }

    return 0;
}

static int read_basic_type(const coda_cursor *cursor, void *dst, long size_boundary)
{
    coda_cdf_variable *variable = (coda_cdf_variable *)cursor->stack[cursor->n - 1].type;
    coda_type_class type_class;
    int index = 0;
    int record_id;
    int element_id;
    int value_size;
    int64_t offset;

    if (((coda_cdf_type *)cursor->stack[cursor->n - 1].type)->tag == tag_cdf_basic_type)
    {
        variable = (coda_cdf_variable *)cursor->stack[cursor->n - 2].type;
        index = cursor->stack[cursor->n - 1].index;
    }
    assert(variable->tag == tag_cdf_variable);
    if (variable->base_type == NULL)
    {
        type_class = variable->definition->type_class;
    }
    else
    {
        type_class = variable->base_type->definition->type_class;
    }

    record_id = index / variable->num_values_per_record;
    element_id = index - record_id * variable->num_values_per_record;
    value_size = variable->value_size;

    /* TODO: handle sparse records */
    if (variable->offset[record_id] < 0)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "Missing record not supported for CDF variable");
        return -1;
    }
    offset = variable->offset[record_id] + element_id * variable->value_size;
    if (size_boundary >= 0 && size_boundary < value_size)
    {
        value_size = size_boundary;
    }
    if (variable->data != NULL)
    {
        memcpy(dst, &variable->data[offset], value_size);
    }
    else
    {
        if (read_bytes(cursor->product, offset, value_size, dst) != 0)
        {
            return -1;
        }
    }
    if (type_class != coda_text_class)
    {
#ifdef WORDS_BIGENDIAN
        coda_endianness other_endianness = coda_little_endian;
#else
        coda_endianness other_endianness = coda_big_endian;
#endif
        if (((coda_cdf_product *)cursor->product)->endianness == other_endianness)
        {
            switch (variable->value_size)
            {
                case 1:
                    break;
                case 2:
                    swap2(dst);
                    break;
                case 4:
                    swap4(dst);
                    break;
                case 8:
                    swap8(dst);
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }

    return 0;
}

int coda_cdf_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    return read_basic_type(cursor, dst, -1);
}

int coda_cdf_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    if (read_basic_type(cursor, dst, dst_size) != 0)
    {
        return -1;
    }
    dst[dst_size - 1] = '\0';
    return 0;
}

int coda_cdf_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_double_array(const coda_cursor *cursor, double *dst)
{
    return read_array(cursor, dst);
}

int coda_cdf_cursor_read_char_array(const coda_cursor *cursor, char *dst)
{
    return read_array(cursor, dst);
}
