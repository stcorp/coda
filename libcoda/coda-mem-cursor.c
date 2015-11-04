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

#include "coda-mem-internal.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

int coda_mem_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_mem_record *type;

    type = (coda_mem_record *)cursor->stack[cursor->n - 1].type;
    if (index < 0 || index >= type->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       type->num_fields, __FILE__, __LINE__);
        return -1;
    }

    cursor->n++;
    if (type->field_type[index] != NULL)
    {
        cursor->stack[cursor->n - 1].type = type->field_type[index];
    }
    else
    {
        cursor->stack[cursor->n - 1].type = coda_no_data_singleton(type->definition->format);
    }
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;

    return 0;
}

int coda_mem_cursor_goto_next_record_field(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_mem_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }

    return 0;
}

int coda_mem_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    /* check the number of dimensions */
    if (num_subs != 1)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (1) (%s:%u)", num_subs,
                       __FILE__, __LINE__);
        return -1;
    }
    return coda_mem_cursor_goto_array_element_by_index(cursor, subs[0]);
}

int coda_mem_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_mem_array *type = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

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
    cursor->stack[cursor->n - 1].type = type->element[index];
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;

    return 0;
}

int coda_mem_cursor_goto_next_array_element(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_mem_cursor_goto_array_element_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }

    return 0;
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
    cursor->stack[cursor->n - 1].type = ((coda_mem_special *)cursor->stack[cursor->n - 1].type)->base_type;

    return 0;
}

int coda_mem_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    *length = strlen(((coda_mem_text *)cursor->stack[cursor->n - 1].type)->text);
    return 0;
}

int coda_mem_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_dynamic_type *type = cursor->stack[cursor->n - 1].type;

    if (type->definition->type_class == coda_raw_class)
    {
        *bit_size = 8 * ((coda_mem_raw *)cursor->stack[cursor->n - 1].type)->length;
    }
    else if (type->definition->type_class == coda_special_class &&
             ((coda_type_special *)type->definition)->special_type == coda_special_no_data)
    {
        *bit_size = 0;
    }
    else
    {
        *bit_size = -1;
    }

    return 0;
}

int coda_mem_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    switch (cursor->stack[cursor->n - 1].type->definition->type_class)
    {
        case coda_record_class:
            *num_elements = ((coda_mem_record *)cursor->stack[cursor->n - 1].type)->num_fields;
            break;
        case coda_array_class:
            *num_elements = ((coda_mem_array *)cursor->stack[cursor->n - 1].type)->num_elements;
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
        case coda_special_class:
            *num_elements = 1;
            break;
    }
    return 0;
}

int coda_mem_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available)
{
    coda_mem_record *type;

    type = (coda_mem_record *)cursor->stack[cursor->n - 1].type;
    if (index < 0 || index >= type->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       type->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *available = (type->field_type[index] != NULL);
    return 0;
}

int coda_mem_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    *num_dims = 1;
    dim[0] = ((coda_mem_array *)cursor->stack[cursor->n - 1].type)->num_elements;
    return 0;
}


int coda_mem_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    *dst = (int8_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    *dst = (uint8_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    *dst = (int16_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    *dst = (uint16_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    *dst = (int32_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    *dst = (uint32_t)((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    *dst = ((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    *dst = ((coda_mem_integer *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    *dst = (float)((coda_mem_real *)cursor->stack[cursor->n - 1].type)->value;
    return 0;
}

int coda_mem_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    if (cursor->stack[cursor->n - 1].type->definition->type_class == coda_special_class)
    {
        if (((coda_type_special *)cursor->stack[cursor->n - 1].type->definition)->special_type != coda_special_time)
        {
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
            return -1;
        }
        *dst = ((coda_mem_time *)cursor->stack[cursor->n - 1].type)->value;
    }
    else
    {
        *dst = ((coda_mem_real *)cursor->stack[cursor->n - 1].type)->value;
    }
    return 0;
}

int coda_mem_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    switch (cursor->stack[cursor->n - 1].type->definition->type_class)
    {
        case coda_text_class:
            *dst = ((coda_mem_text *)cursor->stack[cursor->n - 1].type)->text[0];
            break;
        case coda_raw_class:
            *dst = ((coda_mem_text *)cursor->stack[cursor->n - 1].type)->text[0];
            break;
        default:
            assert(0);
            exit(1);
    }
    return 0;
}

int coda_mem_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    strncpy(dst, ((coda_mem_text *)cursor->stack[cursor->n - 1].type)->text, dst_size - 1);
    dst[dst_size - 1] = '\0';
    return 0;
}

int coda_mem_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
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
    return coda_mem_cursor_read_bytes(cursor, dst, bit_offset >> 3, bit_length >> 3);
}

int coda_mem_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    coda_mem_type *type = (coda_mem_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->type_class)
    {
        case coda_special_class:
            if (((coda_type_special *)type->definition)->special_type == coda_special_no_data)
            {
                coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the size of the raw type");
                return -1;
            }
            break;
        case coda_raw_class:
            {
                coda_mem_raw *raw_type = (coda_mem_raw *)type;

                if (offset + length > raw_type->length)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the size of the raw type");
                    return -1;
                }
                memcpy(dst, &raw_type->data[offset], (size_t)length);
            }
            return 0;
        default:
            break;
    }
    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bytes data type");
    return -1;
}

int coda_mem_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_int8(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_uint8(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_int16(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_uint16(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_int32(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
    return 0;
}

int coda_mem_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_uint32(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_int64(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_uint64(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_float(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_double_array(const coda_cursor *cursor, double *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_double(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

int coda_mem_cursor_read_char_array(const coda_cursor *cursor, char *dst)
{
    coda_mem_array *array = (coda_mem_array *)cursor->stack[cursor->n - 1].type;

    if (array->num_elements > 0)
    {
        coda_cursor array_cursor = *cursor;
        long i;

        array_cursor.n++;
        array_cursor.stack[array_cursor.n - 1].bit_offset = -1;
        for (i = 0; i < array->num_elements; i++)
        {
            array_cursor.stack[array_cursor.n - 1].index = i;
            array_cursor.stack[array_cursor.n - 1].type = array->element[i];
            if (coda_cursor_read_char(&array_cursor, &dst[i]) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}
