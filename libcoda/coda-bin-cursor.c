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

#include "coda-ascii-internal.h"
#include "coda-bin-internal.h"
#include "coda-definition.h"
#include "coda-read-bits.h"
#include "coda-read-array.h"
#include "coda-read-partial-array.h"
#include "coda-transpose-array.h"
#include "coda-ascbin.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int coda_bin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->bit_size >= 0)
    {
        *bit_size = type->bit_size;
        return 0;
    }

    switch (type->type_class)
    {
        case coda_record_class:
        case coda_array_class:
            return coda_ascbin_cursor_get_bit_size(cursor, bit_size);
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            if (type->size_expr == NULL)
            {
                *bit_size = -1;
            }
            else
            {
                if (coda_expression_eval_integer(type->size_expr, cursor, bit_size) != 0)
                {
                    coda_add_error_message(" for size expression");
                    coda_cursor_add_to_error_message(cursor);
                    return -1;
                }
                if (type->bit_size == -8)
                {
                    *bit_size *= 8;
                }
                if (*bit_size < 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "calculated size is negative (%ld bits)", (long)*bit_size);
                    coda_cursor_add_to_error_message(cursor);
                    return -1;
                }
            }
            break;
        case coda_special_class:
            {
                coda_cursor spec_cursor;

                spec_cursor = *cursor;
                if (coda_ascbin_cursor_use_base_type_of_special_type(&spec_cursor) != 0)
                {
                    return -1;
                }
                if (coda_cursor_get_bit_size(&spec_cursor, bit_size) != 0)
                {
                    return -1;
                }
            }
            break;
    }

    return 0;
}

int coda_bin_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    int64_t bit_size;

    if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
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

int coda_bin_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    switch (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type)->type_class)
    {
        case coda_array_class:
        case coda_record_class:
            return coda_ascbin_cursor_get_num_elements(cursor, num_elements);
        default:
            *num_elements = 1;  /* non-compound types type */
            break;
    }

    return 0;
}

int coda_bin_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;

    if (bit_size < 0)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 8)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary int8 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 8)
    {
        assert(bit_size_to_byte_size(bit_size) <= 1);
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, (uint8_t *)dst) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 1, dst) != 0)
        {
            return -1;
        }
    }

    if (bit_size < 8)
    {
        uint8_t value = *(uint8_t *)dst;

        if (value & (1 << (bit_size - 1)))
        {
            /* sign bit is set -> set higher significant bits to 1 as well */
            *dst = (int8_t)(value | ~((1 << bit_size) - 1));
        }
    }

    return 0;
}

int coda_bin_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 8)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary uint8 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 8)
    {
        assert(bit_size_to_byte_size(bit_size) <= 1);
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, (uint8_t *)dst) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 1, dst) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int coda_bin_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 16)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary int16 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 16)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[2 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 2, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[2];
            int16_t as_int16;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[1];
        data.as_bytes[1] = ((uint8_t *)dst)[0];
        *dst = data.as_int16;
    }

    if (bit_size < 16)
    {
        uint16_t value = *(uint16_t *)dst;

        if (value & (1 << (bit_size - 1)))
        {
            /* sign bit is set -> set higher significant bits to 1 as well */
            *dst = (int16_t)(value | ~((1 << bit_size) - 1));
        }
    }

    return 0;
}

int coda_bin_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 16)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary uint16 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 16)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[2 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 2, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[2];
            uint16_t as_uint16;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[1];
        data.as_bytes[1] = ((uint8_t *)dst)[0];
        *dst = data.as_uint16;
    }

    return 0;
}

int coda_bin_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 32)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary int32 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }

    if ((bit_offset & 0x7) || bit_size != 32)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[4 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 4, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[4];
            int32_t as_int32;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[3];
        data.as_bytes[1] = ((uint8_t *)dst)[2];
        data.as_bytes[2] = ((uint8_t *)dst)[1];
        data.as_bytes[3] = ((uint8_t *)dst)[0];
        *dst = data.as_int32;
    }

    if (bit_size < 32)
    {
        uint32_t value = *(uint32_t *)dst;

        if (value & (1 << (bit_size - 1)))
        {
            /* sign bit is set -> set higher significant bits to 1 as well */
            *dst = (int32_t)(value | ~((1 << bit_size) - 1));
        }
    }

    return 0;
}

int coda_bin_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 32)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary uint32 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 32)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[4 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 4, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[4];
            uint32_t as_uint32;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[3];
        data.as_bytes[1] = ((uint8_t *)dst)[2];
        data.as_bytes[2] = ((uint8_t *)dst)[1];
        data.as_bytes[3] = ((uint8_t *)dst)[0];
        *dst = data.as_uint32;
    }

    return 0;
}

int coda_bin_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 64)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary int64 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 64)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[8 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 8, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[8];
            int64_t as_int64;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[7];
        data.as_bytes[1] = ((uint8_t *)dst)[6];
        data.as_bytes[2] = ((uint8_t *)dst)[5];
        data.as_bytes[3] = ((uint8_t *)dst)[4];
        data.as_bytes[4] = ((uint8_t *)dst)[3];
        data.as_bytes[5] = ((uint8_t *)dst)[2];
        data.as_bytes[6] = ((uint8_t *)dst)[1];
        data.as_bytes[7] = ((uint8_t *)dst)[0];
        *dst = data.as_int64;
    }

    if (bit_size < 64)
    {
        uint64_t value = *(uint64_t *)dst;

        if (value & (((uint64_t)1) << (bit_size - 1)))
        {
            /* sign bit is set -> set higher significant bits to 1 as well */
            *dst = (int64_t)(value | ~((1 << bit_size) - 1));
        }
    }

    return 0;
}

int coda_bin_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_size = type->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_size == -1)
    {
        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0 || bit_size > 64)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary uint64 integer - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }
    if ((bit_offset & 0x7) || bit_size != 64)
    {
        uint8_t *buffer = (uint8_t *)dst;

        if (endianness == coda_big_endian)
        {
            buffer = &((uint8_t *)dst)[8 - bit_size_to_byte_size(bit_size)];
        }
        *dst = 0;
        if (read_bits(cursor->product, bit_offset, bit_size, buffer) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 8, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[8];
            uint64_t as_uint64;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[7];
        data.as_bytes[1] = ((uint8_t *)dst)[6];
        data.as_bytes[2] = ((uint8_t *)dst)[5];
        data.as_bytes[3] = ((uint8_t *)dst)[4];
        data.as_bytes[4] = ((uint8_t *)dst)[3];
        data.as_bytes[5] = ((uint8_t *)dst)[2];
        data.as_bytes[6] = ((uint8_t *)dst)[1];
        data.as_bytes[7] = ((uint8_t *)dst)[0];
        *dst = data.as_uint64;
    }

    return 0;
}


int coda_bin_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_offset & 0x7)
    {
        if (read_bits(cursor->product, bit_offset, 32, (uint8_t *)dst) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 4, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[4];
            float as_float;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[3];
        data.as_bytes[1] = ((uint8_t *)dst)[2];
        data.as_bytes[2] = ((uint8_t *)dst)[1];
        data.as_bytes[3] = ((uint8_t *)dst)[0];
        *dst = data.as_float;
    }

    return 0;
}

static int read_double(const coda_cursor *cursor, double *dst)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)type)->endianness;

    if (bit_offset & 0x7)
    {
        if (read_bits(cursor->product, bit_offset, 64, (uint8_t *)dst) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(cursor->product, bit_offset >> 3, 8, dst) != 0)
        {
            return -1;
        }
    }

    if (
#ifdef WORDS_BIGENDIAN
           endianness == coda_little_endian
#else
           endianness == coda_big_endian
#endif
        )
    {
        union
        {
            uint8_t as_bytes[8];
            double as_double;
        } data;

        data.as_bytes[0] = ((uint8_t *)dst)[7];
        data.as_bytes[1] = ((uint8_t *)dst)[6];
        data.as_bytes[2] = ((uint8_t *)dst)[5];
        data.as_bytes[3] = ((uint8_t *)dst)[4];
        data.as_bytes[4] = ((uint8_t *)dst)[3];
        data.as_bytes[5] = ((uint8_t *)dst)[2];
        data.as_bytes[6] = ((uint8_t *)dst)[1];
        data.as_bytes[7] = ((uint8_t *)dst)[0];
        *dst = data.as_double;
    }

    return 0;
}

int coda_bin_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    return read_double(cursor, dst);
}

int coda_bin_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    return coda_bin_cursor_read_uint8(cursor, (uint8_t *)dst);
}

int coda_bin_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    int64_t read_size = 0;

    if (type->bit_size < 0)
    {
        int64_t bit_size;

        if (coda_bin_cursor_get_bit_size(cursor, &bit_size) != 0)
        {
            return -1;
        }
        if (bit_size < 0)
        {
            char s1[21];
            char s2[21];

            coda_str64(bit_size, s1);
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s2);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected (invalid bit size (%s) for binary string - "
                           "byte:bit offset = %s:%d)", s1, s2, (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
        if ((bit_size & 0x7) != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "product error detected (text does not have a rounded byte size)");
            return -1;
        }
        read_size = bit_size >> 3;
    }
    else
    {
        read_size = type->bit_size >> 3;
    }
    if (read_size + 1 > dst_size)       /* account for terminating zero */
    {
        read_size = dst_size - 1;
    }
    if (read_size > 0)
    {
        if (read_bits(cursor->product, bit_offset, 8 * read_size, (uint8_t *)dst) != 0)
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

int coda_bin_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    return read_bits(cursor->product, cursor->stack[cursor->n - 1].bit_offset + bit_offset, bit_length, dst);
}

int coda_bin_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    if (cursor->stack[cursor->n - 1].bit_offset & 0x7)
    {
        return coda_bin_cursor_read_bits(cursor, dst, 8 * offset, 8 * length);
    }
    return read_bytes(cursor->product, (cursor->stack[cursor->n - 1].bit_offset >> 3) + offset, length, dst);
}

int coda_bin_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int8, (uint8_t *)dst, sizeof(int8_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint8, (uint8_t *)dst, sizeof(uint8_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int16, (uint8_t *)dst, sizeof(int16_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint16, (uint8_t *)dst, sizeof(uint16_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int32, (uint8_t *)dst, sizeof(int32_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint32, (uint8_t *)dst, sizeof(uint32_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_int64, (uint8_t *)dst, sizeof(int64_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_uint64, (uint8_t *)dst, sizeof(uint64_t),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_float, (uint8_t *)dst, sizeof(float),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_double, (uint8_t *)dst, sizeof(double),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_array(cursor, (read_function)&coda_bin_cursor_read_char, (uint8_t *)dst, sizeof(char),
                          array_ordering);
    }
    assert(type->base_type->format == coda_format_ascii);
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

int coda_bin_cursor_read_int8_partial_array(const coda_cursor *cursor, long offset, long length, int8_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int8, offset, length, (uint8_t *)dst,
                                  sizeof(int8_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int8, offset, length, (uint8_t *)dst,
                              sizeof(int8_t));
}

int coda_bin_cursor_read_uint8_partial_array(const coda_cursor *cursor, long offset, long length, uint8_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint8, offset, length, (uint8_t *)dst,
                                  sizeof(uint8_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint8, offset, length, (uint8_t *)dst,
                              sizeof(uint8_t));
}

int coda_bin_cursor_read_int16_partial_array(const coda_cursor *cursor, long offset, long length, int16_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int16, offset, length, (uint8_t *)dst,
                                  sizeof(int16_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int16, offset, length, (uint8_t *)dst,
                              sizeof(int16_t));
}

int coda_bin_cursor_read_uint16_partial_array(const coda_cursor *cursor, long offset, long length, uint16_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint16, offset, length, (uint8_t *)dst,
                                  sizeof(uint16_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint16, offset, length, (uint8_t *)dst,
                              sizeof(uint16_t));
}

int coda_bin_cursor_read_int32_partial_array(const coda_cursor *cursor, long offset, long length, int32_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int32, offset, length, (uint8_t *)dst,
                                  sizeof(int32_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int32, offset, length, (uint8_t *)dst,
                              sizeof(int32_t));
}

int coda_bin_cursor_read_uint32_partial_array(const coda_cursor *cursor, long offset, long length, uint32_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint32, offset, length, (uint8_t *)dst,
                                  sizeof(uint32_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint32, offset, length, (uint8_t *)dst,
                              sizeof(uint32_t));
}

int coda_bin_cursor_read_int64_partial_array(const coda_cursor *cursor, long offset, long length, int64_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_int64, offset, length, (uint8_t *)dst,
                                  sizeof(int64_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_int64, offset, length, (uint8_t *)dst,
                              sizeof(int64_t));
}

int coda_bin_cursor_read_uint64_partial_array(const coda_cursor *cursor, long offset, long length, uint64_t *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_uint64, offset, length, (uint8_t *)dst,
                                  sizeof(uint64_t));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_uint64, offset, length, (uint8_t *)dst,
                              sizeof(uint64_t));
}

int coda_bin_cursor_read_float_partial_array(const coda_cursor *cursor, long offset, long length, float *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_float, offset, length, (uint8_t *)dst,
                                  sizeof(float));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_float, offset, length, (uint8_t *)dst,
                              sizeof(float));
}

int coda_bin_cursor_read_double_partial_array(const coda_cursor *cursor, long offset, long length, double *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_double, offset, length, (uint8_t *)dst,
                                  sizeof(double));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_double, offset, length, (uint8_t *)dst,
                              sizeof(double));
}

int coda_bin_cursor_read_char_partial_array(const coda_cursor *cursor, long offset, long length, char *dst)
{
    coda_type_array *type = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->base_type->format == coda_format_binary)
    {
        return read_partial_array(cursor, (read_function)&coda_bin_cursor_read_char, offset, length, (uint8_t *)dst,
                                  sizeof(char));
    }
    assert(type->base_type->format == coda_format_ascii);
    return read_partial_array(cursor, (read_function)&coda_ascii_cursor_read_char, offset, length, (uint8_t *)dst,
                              sizeof(char));
}

/** @} */
