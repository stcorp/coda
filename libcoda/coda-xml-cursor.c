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

#include "coda-xml-internal.h"
#include "coda-ascbin.h"
#include "coda-read-bytes.h"
#include "coda-ascii.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int coda_xml_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = 0;
    return 0;
}

int coda_xml_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_xml_type *type;
    coda_dynamic_type *field_type;
    int64_t bit_offset;

    type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;
    if (type->definition->format == coda_format_ascii)
    {
        /* defer to the ascii backend */
        return coda_ascbin_cursor_goto_record_field_by_index(cursor, index);
    }

    switch (type->tag)
    {
        case tag_xml_root:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_dynamic_type *)((coda_xml_root *)type)->element;
            bit_offset = 0;
            break;
        case tag_xml_element:
            if (index < 0 || index >= ((coda_type_record *)type->definition)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               ((coda_type_record *)type->definition)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_dynamic_type *)((coda_xml_element *)type)->element[index];
            bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
            break;
        default:
            assert(0);
            exit(1);
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].index = index;
    if (field_type == NULL)
    {
        cursor->stack[cursor->n - 1].bit_offset = -1;
        cursor->stack[cursor->n - 1].type = coda_no_data_singleton(coda_format_xml);
    }
    else
    {
        cursor->stack[cursor->n - 1].bit_offset = bit_offset;
        cursor->stack[cursor->n - 1].type = field_type;
    }

    return 0;
}

int coda_xml_cursor_goto_next_record_field(coda_cursor *cursor)
{
    coda_xml_type *type;

    type = (coda_xml_type *)cursor->stack[cursor->n - 2].type;
    if (type->definition->format == coda_format_ascii)
    {
        /* defer to the ascii backend */
        return coda_ascbin_cursor_goto_next_record_field(cursor);
    }

    cursor->n--;
    if (coda_xml_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_xml_cursor_goto_available_union_field(coda_cursor *cursor)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    cursor->stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascbin_cursor_goto_available_union_field(cursor);
}

int coda_xml_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    cursor->stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascbin_cursor_goto_array_element(cursor, num_subs, subs);
}

int coda_xml_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    cursor->stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascbin_cursor_goto_array_element_by_index(cursor, index);
}

int coda_xml_cursor_goto_next_array_element(coda_cursor *cursor)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 2].type;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    return coda_ascbin_cursor_goto_next_array_element(cursor);
}

int coda_xml_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    cursor->n++;
    if (type->tag == tag_xml_element && ((coda_xml_element *)type)->attributes != NULL)
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_xml_element *)type)->attributes;
    }
    else
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)coda_mem_empty_record(coda_format_xml);
    }
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for attributes */

    return 0;
}

int coda_xml_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    cursor->stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_use_base_type_of_special_type(cursor);
}

int coda_xml_cursor_has_ascii_content(const coda_cursor *cursor, int *has_ascii_content)
{
    *has_ascii_content = (((coda_xml_type *)cursor->stack[cursor->n - 1].type)->tag == tag_xml_element);
    return 0;
}

int coda_xml_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascii_cursor_get_string_length(&sub_cursor, length, ((coda_xml_element *)type)->inner_bit_size);
    }

    if (type->tag == tag_xml_root)
    {
        *length = (long)(cursor->product->file_size >> 3);
    }
    else
    {
        *length = (long)(((coda_xml_element *)type)->inner_bit_size >> 3);
    }

    return 0;
}

int coda_xml_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascii_cursor_get_bit_size(&sub_cursor, bit_size, ((coda_xml_element *)type)->inner_bit_size);
    }

    switch (type->tag)
    {
        case tag_xml_root:
            *bit_size = cursor->product->file_size;
            break;
        case tag_xml_element:
            *bit_size = ((coda_xml_element *)type)->inner_bit_size;
            break;
    }

    return 0;
}

int coda_xml_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascii_cursor_get_num_elements(&sub_cursor, num_elements);
    }

    switch (type->tag)
    {
        case tag_xml_root:
            *num_elements = 1;
            break;
        case tag_xml_element:
            if (((coda_xml_element *)type)->definition->type_class == coda_record_class)
            {
                *num_elements = ((coda_type_record *)type->definition)->num_fields;
            }
            else
            {
                *num_elements = 1;
            }
            break;
    }

    return 0;
}

int coda_xml_cursor_get_file_bit_offset(const coda_cursor *cursor, int64_t *bit_offset)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    switch (type->tag)
    {
        case tag_xml_root:
            *bit_offset = 0;
            break;
        case tag_xml_element:
            *bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
            break;
    }

    return 0;
}

int coda_xml_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascbin_cursor_get_record_field_available_status(&sub_cursor, index, available);
    }

    if (type->tag == tag_xml_root)
    {
        if (index != 0)
        {
            coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                           __FILE__, __LINE__);
            return -1;
        }
        *available = ((coda_xml_root *)type)->element != NULL;
    }
    else
    {
        long num_elements;

        num_elements = ((coda_type_record *)type->definition)->num_fields;
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                           num_elements, __FILE__, __LINE__);
            return -1;
        }
        *available = ((coda_xml_element *)type)->element[index] != NULL;
    }

    return 0;
}

int coda_xml_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascbin_cursor_get_available_union_field_index(&sub_cursor, index);
}

int coda_xml_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascbin_cursor_get_array_dim(&sub_cursor, num_dims, dim);
}

int coda_xml_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_int8(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_uint8(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_int16(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_uint16(&sub_cursor, dst, type->inner_bit_size);
    return -1;
}

int coda_xml_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_int32(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_uint32(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_int64(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_uint64(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_float(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
    return coda_ascii_cursor_read_double(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascii_cursor_read_char(&sub_cursor, dst, ((coda_xml_element *)type)->inner_bit_size);
    }

    return read_bytes(cursor->product, ((coda_xml_element *)type)->inner_bit_offset >> 3, 1, (uint8_t *)dst);
}

int coda_xml_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;
    long read_size;

    assert(type->tag == tag_xml_element);
    if (type->definition->format == coda_format_ascii)
    {
        coda_cursor sub_cursor = *cursor;

        /* defer to the ascii backend */
        sub_cursor.stack[cursor->n - 1].bit_offset = ((coda_xml_element *)type)->inner_bit_offset;
        return coda_ascii_cursor_read_string(&sub_cursor, dst, dst_size, ((coda_xml_element *)type)->inner_bit_size);
    }

    read_size = (long)(((coda_xml_element *)type)->inner_bit_size >> 3);
    if (read_size + 1 > dst_size)
    {
        read_size = dst_size - 1;
    }
    if (read_size > 0)
    {
        if (read_bytes(cursor->product, ((coda_xml_element *)type)->inner_bit_offset >> 3, read_size, (uint8_t *)dst)
            != 0)
        {
            return -1;
        }
        dst[read_size] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }

    return 0;
}


int coda_xml_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    if (bit_length & 0x7)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "cannot read this data using a bitsize that is not a multiple of 8");
        return -1;
    }
    if (bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "cannot read this data using a bit offset that is not a multiple of 8");
        return -1;
    }
    return coda_xml_cursor_read_bytes(cursor, dst, bit_offset >> 3, bit_length >> 3);
}

int coda_xml_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    coda_xml_type *type = (coda_xml_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_xml_root)
    {
        return read_bytes(cursor->product, offset, length, dst);
    }
    return read_bytes(cursor->product, (((coda_xml_element *)type)->inner_bit_offset >> 3) + offset, length, dst);
}

int coda_xml_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_int8_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_uint8_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_int16_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_uint16_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_int32_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_uint32_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_int64_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_uint64_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_float_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_double_array(const coda_cursor *cursor, double *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_double_array(&sub_cursor, dst, type->inner_bit_size);
}

int coda_xml_cursor_read_char_array(const coda_cursor *cursor, char *dst)
{
    coda_xml_element *type = (coda_xml_element *)cursor->stack[cursor->n - 1].type;
    coda_cursor sub_cursor = *cursor;

    /* defer to the ascii backend */
    assert(type->definition->format == coda_format_ascii);
    sub_cursor.stack[cursor->n - 1].bit_offset = type->inner_bit_offset;
    return coda_ascii_cursor_read_char_array(&sub_cursor, dst, type->inner_bit_size);
}
