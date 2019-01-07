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

#include "coda-mem-internal.h"
#include "coda-ascii-internal.h"
#include "coda-ascbin.h"
#include "coda-bin-internal.h"
#include "coda-read-array.h"
#include "coda-read-partial-array.h"
#include "coda-transpose-array.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void coda_mem_cursor_update_offset(coda_cursor *cursor)
{
    if (((coda_mem_type *)cursor->stack[cursor->n - 1].type)->tag == tag_mem_data)
    {
        cursor->stack[cursor->n - 1].bit_offset = 8 * ((coda_mem_data *)cursor->stack[cursor->n - 1].type)->offset;
    }
}

int coda_mem_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_record)
    {
        if (index < 0 || index >= ((coda_mem_record *)type)->num_fields)
        {
            coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                           ((coda_mem_record *)type)->num_fields);
            return -1;
        }
        cursor->n++;
        if (((coda_mem_record *)type)->field_type[index] != NULL)
        {
            cursor->stack[cursor->n - 1].type = ((coda_mem_record *)type)->field_type[index];
        }
        else
        {
            cursor->stack[cursor->n - 1].type = coda_no_data_singleton(type->definition->format);
        }
        cursor->stack[cursor->n - 1].index = index;
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_record_field_by_index(cursor, index);
}

int coda_mem_cursor_goto_next_record_field(coda_cursor *cursor)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 2].type;

    if (type->tag == tag_mem_record)
    {
        long index = cursor->stack[cursor->n - 1].index + 1;

        if (index < 0 || index >= ((coda_mem_record *)type)->num_fields)
        {
            coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                           ((coda_mem_record *)type)->num_fields);
            return -1;
        }
        if (((coda_mem_record *)type)->field_type[index] != NULL)
        {
            cursor->stack[cursor->n - 1].type = ((coda_mem_record *)type)->field_type[index];
        }
        else
        {
            cursor->stack[cursor->n - 1].type = coda_no_data_singleton(type->definition->format);
        }
        cursor->stack[cursor->n - 1].index = index;
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_next_record_field(cursor);
}

int coda_mem_cursor_goto_available_union_field(coda_cursor *cursor)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_record)
    {
        long index;

        if (coda_mem_cursor_get_available_union_field_index(cursor, &index) != 0)
        {
            return -1;
        }
        cursor->stack[cursor->n - 1].type = ((coda_mem_record *)type)->field_type[index];
        cursor->stack[cursor->n - 1].index = index;
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_available_union_field(cursor);
}

int coda_mem_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        /* check the number of dimensions */
        if (num_subs != 1)
        {
            coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                           "number of dimensions argument (%d) does not match rank of array (1)", num_subs);
            return -1;
        }
        /* check the range for index */
        if (coda_option_perform_boundary_checks)
        {
            if (subs[0] < 0 || subs[0] >= ((coda_mem_array *)type)->num_elements)
            {
                coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", subs[0],
                               ((coda_mem_array *)type)->num_elements);
                return -1;
            }
        }
        cursor->n++;
        cursor->stack[cursor->n - 1].type = ((coda_mem_array *)type)->element[subs[0]];
        cursor->stack[cursor->n - 1].index = subs[0];
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_array_element(cursor, num_subs, subs);
}

int coda_mem_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        /* check the range for index */
        if (coda_option_perform_boundary_checks)
        {
            if (index < 0 || index >= ((coda_mem_array *)type)->num_elements)
            {
                coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                               ((coda_mem_array *)type)->num_elements);
                return -1;
            }
        }
        cursor->n++;
        cursor->stack[cursor->n - 1].type = ((coda_mem_array *)type)->element[index];
        cursor->stack[cursor->n - 1].index = index;
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_array_element_by_index(cursor, index);
}

int coda_mem_cursor_goto_next_array_element(coda_cursor *cursor)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 2].type;

    if (type->tag == tag_mem_array)
    {
        long index = cursor->stack[cursor->n - 1].index + 1;

        if (index < 0 || index >= ((coda_mem_array *)type)->num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           ((coda_mem_array *)type)->num_elements);
            return -1;
        }
        if (((coda_mem_array *)type)->element[index] != NULL)
        {
            cursor->stack[cursor->n - 1].type = ((coda_mem_array *)type)->element[index];
        }
        else
        {
            cursor->stack[cursor->n - 1].type = coda_no_data_singleton(type->definition->format);
        }
        cursor->stack[cursor->n - 1].index = index;
        cursor->stack[cursor->n - 1].bit_offset = -1;

        return 0;
    }

    assert(type->tag == tag_mem_data);
    return coda_ascbin_cursor_goto_next_array_element(cursor);
}

int coda_mem_cursor_goto_attributes(coda_cursor *cursor)
{
    cursor->n++;
    if (((coda_mem_type *)cursor->stack[cursor->n - 2].type)->attributes != NULL)
    {
        cursor->stack[cursor->n - 1].type =
            (coda_dynamic_type *)((coda_mem_type *)cursor->stack[cursor->n - 2].type)->attributes;
    }
    else
    {
        cursor->stack[cursor->n - 1].type =
            coda_mem_empty_record(cursor->stack[cursor->n - 2].type->definition->format);
    }
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;

    return 0;
}

int coda_mem_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_special)
    {
        cursor->stack[cursor->n - 1].type = ((coda_mem_special *)cursor->stack[cursor->n - 1].type)->base_type;
        return 0;
    }

    assert(type->tag == tag_mem_data);
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_type_special *)type->definition)->base_type;

    return 0;
}

int coda_mem_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    int64_t bit_size;

    if (coda_mem_cursor_get_bit_size(cursor, &bit_size) != 0)
    {
        return -1;
    }
    if (bit_size < 0)
    {
        *length = -1;
    }
    else
    {
        *length = (long)(bit_size >> 3);
    }

    return 0;
}

int coda_mem_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_special)
    {
        coda_cursor sub_cursor = *cursor;

        if (coda_cursor_use_base_type_of_special_type(&sub_cursor) != 0)
        {
            return -1;
        }
        return coda_cursor_get_bit_size(&sub_cursor, bit_size);
    }

    if (type->tag == tag_mem_data)
    {
        if (type->definition->format == coda_format_ascii)
        {
            if (coda_ascii_cursor_get_bit_size(cursor, bit_size) != 0)
            {
                return -1;
            }
        }
        else if (coda_bin_cursor_get_bit_size(cursor, bit_size) != 0)
        {
            return -1;
        }
        if (*bit_size < 0)
        {
            *bit_size = 8 * ((coda_mem_data *)type)->length;
        }
    }
    else
    {
        *bit_size = -1;
    }

    return 0;
}

int coda_mem_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    switch (type->tag)
    {
        case tag_mem_record:
            *num_elements = ((coda_mem_record *)cursor->stack[cursor->n - 1].type)->num_fields;
            break;
        case tag_mem_array:
            *num_elements = ((coda_mem_array *)cursor->stack[cursor->n - 1].type)->num_elements;
            break;
        case tag_mem_data:
            if (type->definition->format == coda_format_ascii)
            {
                return coda_ascii_cursor_get_num_elements(cursor, num_elements);
            }
            return coda_bin_cursor_get_num_elements(cursor, num_elements);
        case tag_mem_special:
            *num_elements = 1;
            break;
    }
    return 0;
}

int coda_mem_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_data)
    {
        return coda_ascbin_cursor_get_record_field_available_status(cursor, index, available);
    }

    assert(type->tag == tag_mem_record);
    if (index < 0 || index >= ((coda_mem_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                       ((coda_mem_record *)type)->num_fields);
        return -1;
    }
    *available = (((coda_mem_record *)type)->field_type[index] != NULL);

    return 0;

}

int coda_mem_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_record)
    {
        long i;

        /* return first available field */
        for (i = 0; i < ((coda_mem_record *)type)->num_fields; i++)
        {
            if (((coda_mem_record *)type)->field_type[i] != NULL)
            {
                *index = i;
                return 0;
            }
        }

        coda_set_error(CODA_ERROR_PRODUCT, "union has no available fields");
        return -1;
    }

    assert(((coda_mem_type *)cursor->stack[cursor->n - 1].type)->tag == tag_mem_data);
    return coda_ascbin_cursor_get_available_union_field_index(cursor, index);
}

int coda_mem_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;
    coda_mem_array *array;

    if (type->tag == tag_mem_data)
    {
        return coda_ascbin_cursor_get_array_dim(cursor, num_dims, dim);
    }

    assert(type->tag == tag_mem_array);
    array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;
    if (array->definition->num_elements >= 0)
    {
        int i;

        *num_dims = array->definition->num_dims;
        for (i = 0; i < array->definition->num_dims; i++)
        {
            dim[i] = array->definition->dim[i];
        }
    }
    else
    {
        *num_dims = 1;
        dim[0] = array->num_elements;
    }

    return 0;
}


int coda_mem_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_int8(cursor, dst);
    }
    return coda_bin_cursor_read_int8(cursor, dst);
}

int coda_mem_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_uint8(cursor, dst);
    }
    return coda_bin_cursor_read_uint8(cursor, dst);
}

int coda_mem_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_int16(cursor, dst);
    }
    return coda_bin_cursor_read_int16(cursor, dst);
}

int coda_mem_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_uint16(cursor, dst);
    }
    return coda_bin_cursor_read_uint16(cursor, dst);
}

int coda_mem_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_int32(cursor, dst);
    }
    return coda_bin_cursor_read_int32(cursor, dst);
}

int coda_mem_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_uint32(cursor, dst);
    }
    return coda_bin_cursor_read_uint32(cursor, dst);
}

int coda_mem_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_int64(cursor, dst);
    }
    return coda_bin_cursor_read_int64(cursor, dst);
}

int coda_mem_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_uint64(cursor, dst);
    }
    return coda_bin_cursor_read_uint64(cursor, dst);
}

int coda_mem_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_float(cursor, dst);
    }
    return coda_bin_cursor_read_float(cursor, dst);
}

int coda_mem_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii)
    {
        return coda_ascii_cursor_read_double(cursor, dst);
    }
    return coda_bin_cursor_read_double(cursor, dst);
}

int coda_mem_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_mem_data);
    if (type->definition->format == coda_format_ascii || type->definition->format == coda_format_xml)
    {
        return coda_ascii_cursor_read_char(cursor, dst);
    }
    return coda_bin_cursor_read_char(cursor, dst);
}

int coda_mem_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    coda_mem_data *type = (coda_mem_data *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_special)
    {
        coda_cursor sub_cursor = *cursor;

        if (coda_cursor_use_base_type_of_special_type(&sub_cursor) != 0)
        {
            return -1;
        }
        return coda_cursor_read_string(&sub_cursor, dst, dst_size);
    }

    assert(type->tag == tag_mem_data);
    return coda_ascii_cursor_read_string(cursor, dst, dst_size);
}

int coda_mem_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_special)
    {
        coda_cursor sub_cursor = *cursor;

        if (coda_cursor_use_base_type_of_special_type(&sub_cursor) != 0)
        {
            return -1;
        }
        return coda_cursor_read_bits(&sub_cursor, dst, bit_offset, bit_length);
    }

    if (type->tag == tag_mem_data)
    {
        if (type->definition->format == coda_format_ascii)
        {
            return coda_ascii_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        }
        return coda_bin_cursor_read_bits(cursor, dst, bit_offset, bit_length);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bits data type");
    return -1;
}

int coda_mem_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_special)
    {
        coda_cursor sub_cursor = *cursor;

        if (coda_cursor_use_base_type_of_special_type(&sub_cursor) != 0)
        {
            return -1;
        }
        return coda_cursor_read_bytes(&sub_cursor, dst, offset, length);
    }

    if (type->tag == tag_mem_data)
    {
        if (type->definition->format == coda_format_ascii || type->definition->format == coda_format_xml)
        {
            return coda_ascii_cursor_read_bytes(cursor, dst, offset, length);
        }
        return coda_bin_cursor_read_bytes(cursor, dst, offset, length);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bytes data type");
    return -1;
}

int coda_mem_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_int8, (uint8_t *)dst, sizeof(int8_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int8, (uint8_t *)dst, sizeof(int8_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_int8, (uint8_t *)dst, sizeof(int8_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(int8_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_uint8, (uint8_t *)dst, sizeof(uint8_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint8, (uint8_t *)dst, sizeof(uint8_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_uint8, (uint8_t *)dst, sizeof(uint8_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(uint8_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_int16, (uint8_t *)dst, sizeof(int16_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int16, (uint8_t *)dst, sizeof(int16_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_int16, (uint8_t *)dst, sizeof(int16_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(int16_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_uint16, (uint8_t *)dst, sizeof(uint16_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint16, (uint8_t *)dst, sizeof(uint16_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_uint16, (uint8_t *)dst, sizeof(uint16_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(uint16_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_int32, (uint8_t *)dst, sizeof(int32_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int32, (uint8_t *)dst, sizeof(int32_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_int32, (uint8_t *)dst, sizeof(int32_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(int32_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_uint32, (uint8_t *)dst, sizeof(uint32_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint32, (uint8_t *)dst, sizeof(uint32_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_uint32, (uint8_t *)dst, sizeof(uint32_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(uint32_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_int64, (uint8_t *)dst, sizeof(int64_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int64, (uint8_t *)dst, sizeof(int64_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_int64, (uint8_t *)dst, sizeof(int64_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(int64_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_uint64, (uint8_t *)dst, sizeof(uint64_t),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint64, (uint8_t *)dst, sizeof(uint64_t),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_uint64, (uint8_t *)dst, sizeof(uint64_t),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(uint64_t)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_float, (uint8_t *)dst, sizeof(float),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_float, (uint8_t *)dst, sizeof(float),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_float, (uint8_t *)dst, sizeof(float),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(float)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_double, (uint8_t *)dst, sizeof(double),
                          array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_double, (uint8_t *)dst, sizeof(double),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_double, (uint8_t *)dst, sizeof(double),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(double)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_array(cursor, (read_function)&coda_cursor_read_char, (uint8_t *)dst, sizeof(char), array_ordering);
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_char, (uint8_t *)dst, sizeof(char),
                          array_ordering);
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    if (read_array(cursor, (read_function)&coda_ascii_cursor_read_char, (uint8_t *)dst, sizeof(char),
                   coda_array_ordering_c) != 0)
    {
        return -1;
    }
    if (array_ordering != coda_array_ordering_c)
    {
        if (transpose_array(cursor, dst, sizeof(char)) != 0)
        {
            return -1;
        }
    }
    return 0;
}

int coda_mem_cursor_read_int8_partial_array(const coda_cursor *cursor, long offset, long length, int8_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_int8, offset, length, (uint8_t *)dst,
                                  sizeof(int8_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int8, offset, length, (uint8_t *)dst,
                                  sizeof(int8_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int8, offset, length, (uint8_t *)dst,
                              sizeof(int8_t));
}

int coda_mem_cursor_read_uint8_partial_array(const coda_cursor *cursor, long offset, long length, uint8_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_uint8, offset, length, (uint8_t *)dst,
                                  sizeof(uint8_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint8, offset, length, (uint8_t *)dst,
                                  sizeof(uint8_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint8, offset, length, (uint8_t *)dst,
                              sizeof(uint8_t));
}

int coda_mem_cursor_read_int16_partial_array(const coda_cursor *cursor, long offset, long length, int16_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_int16, offset, length, (uint8_t *)dst,
                                  sizeof(int16_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int16, offset, length, (uint8_t *)dst,
                                  sizeof(int16_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int16, offset, length, (uint8_t *)dst,
                              sizeof(int16_t));
}

int coda_mem_cursor_read_uint16_partial_array(const coda_cursor *cursor, long offset, long length, uint16_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_uint16, offset, length, (uint8_t *)dst,
                                  sizeof(uint16_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint16, offset, length, (uint8_t *)dst,
                                  sizeof(uint16_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint16, offset, length, (uint8_t *)dst,
                              sizeof(uint16_t));
}

int coda_mem_cursor_read_int32_partial_array(const coda_cursor *cursor, long offset, long length, int32_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_int32, offset, length, (uint8_t *)dst,
                                  sizeof(int32_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int32, offset, length, (uint8_t *)dst,
                                  sizeof(int32_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int32, offset, length, (uint8_t *)dst,
                              sizeof(int32_t));
}

int coda_mem_cursor_read_uint32_partial_array(const coda_cursor *cursor, long offset, long length, uint32_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_uint32, offset, length, (uint8_t *)dst,
                                  sizeof(uint32_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint32, offset, length, (uint8_t *)dst,
                                  sizeof(uint32_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint32, offset, length, (uint8_t *)dst,
                              sizeof(uint32_t));
}

int coda_mem_cursor_read_int64_partial_array(const coda_cursor *cursor, long offset, long length, int64_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_int64, offset, length, (uint8_t *)dst,
                                  sizeof(int64_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int64, offset, length, (uint8_t *)dst,
                                  sizeof(int64_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int64, offset, length, (uint8_t *)dst,
                              sizeof(int64_t));
}

int coda_mem_cursor_read_uint64_partial_array(const coda_cursor *cursor, long offset, long length, uint64_t *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_uint64, offset, length, (uint8_t *)dst,
                                  sizeof(uint64_t));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint64, offset, length, (uint8_t *)dst,
                                  sizeof(uint64_t));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint64, offset, length, (uint8_t *)dst,
                              sizeof(uint64_t));
}

int coda_mem_cursor_read_float_partial_array(const coda_cursor *cursor, long offset, long length, float *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_float, offset, length, (uint8_t *)dst,
                                  sizeof(float));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_float, offset, length, (uint8_t *)dst,
                                  sizeof(float));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_float, offset, length, (uint8_t *)dst,
                              sizeof(float));
}

int coda_mem_cursor_read_double_partial_array(const coda_cursor *cursor, long offset, long length, double *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_double, offset, length, (uint8_t *)dst,
                                  sizeof(double));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_double, offset, length, (uint8_t *)dst,
                                  sizeof(double));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_double, offset, length, (uint8_t *)dst,
                              sizeof(double));
}

int coda_mem_cursor_read_char_partial_array(const coda_cursor *cursor, long offset, long length, char *dst)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_mem_array)
    {
        return read_partial_array(cursor, (read_function)&coda_cursor_read_char, offset, length, (uint8_t *)dst,
                                  sizeof(char));
    }
    assert(type->tag == tag_mem_data);
    if (((coda_type_array *)type->definition)->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_char, offset, length, (uint8_t *)dst,
                                  sizeof(char));
    }
    assert(((coda_type_array *)type->definition)->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_char, offset, length, (uint8_t *)dst,
                              sizeof(char));
}
