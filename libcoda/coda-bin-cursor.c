/*
 * Copyright (C) 2007-2012 S[&]T, The Netherlands.
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

#include "coda-ascbin-internal.h"
#include "coda-bin.h"

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

int coda_bin_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    coda_type_special *type = (coda_type_special *)cursor->stack[cursor->n - 1].type;

    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)type->base_type;

    return 0;
}

int coda_bin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_type *type;

    type = (coda_type *)cursor->stack[cursor->n - 1].type;
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
            if (coda_expression_eval_integer(type->size_expr, cursor, bit_size) != 0)
            {
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
            break;
        case coda_special_class:
            {
                coda_cursor spec_cursor;

                spec_cursor = *cursor;
                if (coda_bin_cursor_use_base_type_of_special_type(&spec_cursor) != 0)
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
    switch (((coda_type *)cursor->stack[cursor->n - 1].type)->type_class)
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

#define bit_size_to_byte_size(x) (((x) >> 3) + ((((uint8_t)(x)) & 0x7) != 0))

typedef int (*read_function) (const coda_cursor *, void *);

/* calculates a * 10 ^ b, with a of type double and b of type long */
static double a_pow10_b(double a, long b)
{
    register long i = labs(b);
    double val = 1.0;

    while (--i >= 0)
    {
        val *= 10;
    }

    if (b < 0)
    {
        return a / val;
    }
    return a * val;
}

static int read_bytes(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    coda_ascbin_product *product_file = (coda_ascbin_product *)product;

    if (((uint64_t)byte_offset + length) > ((uint64_t)product_file->file_size))
    {
        coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
        return -1;
    }
    if (product_file->use_mmap)
    {
        memcpy(dst, product_file->mmap_ptr + byte_offset, (size_t)length);
    }
    else
    {
#if HAVE_PREAD
        if (pread(product_file->fd, dst, (size_t)length, (off_t)byte_offset) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#else
        if (lseek(product_file->fd, (off_t)byte_offset, SEEK_SET) < 0)
        {
            char byte_offset_str[21];

            coda_str64(byte_offset, byte_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                           byte_offset_str, product_file->filename, strerror(errno));
            return -1;
        }
        if (read(product_file->fd, dst, (size_t)length) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#endif
    }

    return 0;
}

static int read_bits(coda_product *product, int64_t bit_offset, int64_t bit_length, uint8_t *dst)
{
    unsigned long bit_shift;
    int64_t padded_bit_length;

    /* we read bits by treating them as big endian numbers.
     * This means that
     *
     *      src[0]     |    src[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  . . a b c d e f|g h i j k . . .
     *
     * will be read and shifted to get
     *
     *      dst[0]     |    dst[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  . . . . . a b c|d e f g h i j k
     * 
     * If the value is a number then on little endian machines the value needs to be converted to:
     *
     *      dst[0]     |    dst[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  d e f g h i j k|0 0 0 0 0 a b c
     *
     * Note that endian conversion does not happen within this function but happens in the functions that call
     * read_bits().
     *
     * In theory we could also implement support for bitdata stored in lsb (least significant bit) to msb order.
     * However, such a feature is currently NOT implemented!
     * If we ever implement such a feature it should look like this:
     * If the format of the source is (note the reverse order in which we display the bits!):
     *
     *      src[0]     |    src[1]
     *  0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7
     *  . . a b c d e f|g h i j k . . .
     *
     * then this will be read as
     *
     *      tmp[0]     |    tmp[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  f e d c b a . .|. . . k j i h g
     *
     * we can then perform a shift of bits (shifting 2 least significant bits from the right byte to the left byte)
     * to get the little endian result
     *
     *      dst[0]     |    dst[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  h g f e d c b a|. . . . . k j i
     *
     * On big endian machines this can then be turned into a big endian number
     *
     *      dst[0]     |    dst[1]
     *  7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
     *  0 0 0 0 0 k j i|h g f e d c b a
     */

    /* The padded bit length is the number of 'padding' bits plus the bit length.
     * The 'padding' bits are the bits between the start of byte (i.e. starting at the most significant bit) and the
     * start of requested bits (in the (big endian) example above, bits 7 and 6 of src[0] are the padding bits).
     */
    padded_bit_length = (bit_offset & 0x7) + bit_length;
    bit_shift = (unsigned long)(-padded_bit_length & 0x7);
    if (padded_bit_length <= 8)
    {
        /* all bits are located within a single byte, so we will use an optimized approach to extract the bits */
        if (read_bytes(product, bit_offset >> 3, 1, dst) != 0)
        {
            return -1;
        }
        if (bit_shift != 0)
        {
            *dst >>= bit_shift;
        }
        if ((bit_length & 0x7) != 0)
        {
            *dst &= ((1 << bit_length) - 1);
        }
    }
    else if (bit_shift == 0)
    {
        /* no shifting needed for the source bytes */
        if (bit_length & 0x7)
        {
            unsigned long trailing_bit_length;
            uint8_t buffer;

            /* the first byte contains trailing bits and is not copied in full */
            if (read_bytes(product, bit_offset >> 3, 1, &buffer) != 0)
            {
                return -1;
            }
            trailing_bit_length = (unsigned long)(bit_length & 0x7);
            *dst = buffer & ((1 << trailing_bit_length) - 1);
            dst++;
            bit_offset += trailing_bit_length;
            bit_length -= trailing_bit_length;
        }
        if (bit_length > 0)
        {
            /* use a plain copy for the remaining bytes */
            if (read_bytes(product, bit_offset >> 3, bit_length >> 3, dst) != 0)
            {
                return -1;
            }
        }
    }
    else
    {
        uint8_t buffer[4];
        union
        {
            uint8_t as_bytes[4];
            uint32_t as_uint32;
        } data;

        /* we need to shift each byte */

        /* we first copy the part modulo 24 bits (so the rest can be processed in chuncks of 24 bits each) */
        if (bit_length % 24 != 0)
        {
            unsigned long mod24_bit_length;
            unsigned long num_bytes_read;
            unsigned long num_bytes_set;
            unsigned long i;

            mod24_bit_length = (unsigned long)(bit_length % 24);
            num_bytes_read = bit_size_to_byte_size(((unsigned long)(bit_offset & 0x7)) + mod24_bit_length);
            num_bytes_set = bit_size_to_byte_size(mod24_bit_length);
            if (read_bytes(product, bit_offset >> 3, num_bytes_read, buffer) != 0)
            {
                return -1;
            }
            data.as_uint32 = 0;
            for (i = 0; i < num_bytes_read; i++)
            {
#ifdef WORDS_BIGENDIAN
                data.as_bytes[i] = buffer[i];
#else
                data.as_bytes[3 - i] = buffer[i];
#endif
            }
            data.as_uint32 = (data.as_uint32 >> (bit_shift + 8 * (4 - num_bytes_read))) & ((1 << mod24_bit_length) - 1);
            for (i = 0; i < num_bytes_set; i++)
            {
#ifdef WORDS_BIGENDIAN
                dst[i] = data.as_bytes[(4 - num_bytes_set) + i];
#else
                dst[i] = data.as_bytes[(num_bytes_set - 1) - i];
#endif
            }
            dst += num_bytes_set;
            bit_offset += mod24_bit_length;
            bit_length -= mod24_bit_length;
        }

        /* we copy the remaining data in chunks of 24 bits (3 bytes) at a time */
        while (bit_length > 0)
        {
#ifdef WORDS_BIGENDIAN
            if (read_bytes(product, bit_offset >> 3, 4, data.as_bytes) != 0)
            {
                return -1;
            }
            data.as_uint32 >>= bit_shift;
            dst[0] = data.as_bytes[1];
            dst[1] = data.as_bytes[2];
            dst[2] = data.as_bytes[3];
#else
            if (read_bytes(product, bit_offset >> 3, 4, buffer) != 0)
            {
                return -1;
            }
            data.as_bytes[0] = buffer[3];
            data.as_bytes[1] = buffer[2];
            data.as_bytes[2] = buffer[1];
            data.as_bytes[3] = buffer[0];
            data.as_uint32 >>= bit_shift;
            dst[0] = data.as_bytes[2];
            dst[1] = data.as_bytes[1];
            dst[2] = data.as_bytes[0];
#endif
            dst += 3;
            bit_offset += 24;
            bit_length -= 24;
        }
    }

    return 0;
}

static int read_binary_envisat_datetime(coda_product *product, int64_t byte_offset, double *dst)
{
    int32_t days;
    uint32_t seconds;
    uint32_t musecs;

    if (read_bytes(product, byte_offset, 4, &days) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 4, 4, &seconds) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 8, 4, &musecs) != 0)
    {
        return -1;
    }
#if !defined(WORDS_BIGENDIAN)
    {
        union
        {
            uint8_t as_bytes[4];
            int32_t as_int32;
        } data;

        data.as_bytes[0] = ((uint8_t *)&days)[3];
        data.as_bytes[1] = ((uint8_t *)&days)[2];
        data.as_bytes[2] = ((uint8_t *)&days)[1];
        data.as_bytes[3] = ((uint8_t *)&days)[0];
        days = data.as_int32;
    }
    {
        union
        {
            uint8_t as_bytes[4];
            uint32_t as_uint32;
        } data;

        data.as_bytes[0] = ((uint8_t *)&seconds)[3];
        data.as_bytes[1] = ((uint8_t *)&seconds)[2];
        data.as_bytes[2] = ((uint8_t *)&seconds)[1];
        data.as_bytes[3] = ((uint8_t *)&seconds)[0];
        seconds = data.as_uint32;

        data.as_bytes[0] = ((uint8_t *)&musecs)[3];
        data.as_bytes[1] = ((uint8_t *)&musecs)[2];
        data.as_bytes[2] = ((uint8_t *)&musecs)[1];
        data.as_bytes[3] = ((uint8_t *)&musecs)[0];
        musecs = data.as_uint32;
    }
#endif

    *dst = days * 86400.0 + seconds + musecs / 1000000.0;

    return 0;
}

static int read_binary_gome_datetime(coda_product *product, int64_t byte_offset, double *dst)
{
    const int DAYS_1950_2000 = 18262;
    int32_t days;
    int32_t msecs;

    if (read_bytes(product, byte_offset, 4, &days) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 4, 4, &msecs) != 0)
    {
        return -1;
    }
#if !defined(WORDS_BIGENDIAN)
    {
        union
        {
            uint8_t as_bytes[4];
            int32_t as_int32;
        } data;

        data.as_bytes[0] = ((uint8_t *)&days)[3];
        data.as_bytes[1] = ((uint8_t *)&days)[2];
        data.as_bytes[2] = ((uint8_t *)&days)[1];
        data.as_bytes[3] = ((uint8_t *)&days)[0];
        days = data.as_int32;

        data.as_bytes[0] = ((uint8_t *)&msecs)[3];
        data.as_bytes[1] = ((uint8_t *)&msecs)[2];
        data.as_bytes[2] = ((uint8_t *)&msecs)[1];
        data.as_bytes[3] = ((uint8_t *)&msecs)[0];
        msecs = data.as_int32;
    }
#endif

    *dst = (days - DAYS_1950_2000) * 86400.0 + msecs / 1000.0;

    return 0;
}

static int read_binary_eps_datetime(coda_product *product, int64_t byte_offset, double *dst)
{
    uint16_t days;
    uint32_t msecs;

    if (read_bytes(product, byte_offset, 2, &days) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 2, 4, &msecs) != 0)
    {
        return -1;
    }
#if !defined(WORDS_BIGENDIAN)
    {
        union
        {
            uint8_t as_bytes[2];
            uint16_t as_uint16;
        } data;

        data.as_bytes[0] = ((uint8_t *)&days)[1];
        data.as_bytes[1] = ((uint8_t *)&days)[0];
        days = data.as_uint16;
    }
    {
        union
        {
            uint8_t as_bytes[4];
            uint32_t as_uint32;
        } data;

        data.as_bytes[0] = ((uint8_t *)&msecs)[3];
        data.as_bytes[1] = ((uint8_t *)&msecs)[2];
        data.as_bytes[2] = ((uint8_t *)&msecs)[1];
        data.as_bytes[3] = ((uint8_t *)&msecs)[0];
        msecs = data.as_uint32;
    }
#endif

    *dst = days * 86400.0 + msecs / 1000.0;

    return 0;
}

static int read_binary_eps_datetime_long(coda_product *product, int64_t byte_offset, double *dst)
{
    uint16_t days;
    uint32_t msecs;
    uint16_t musecs;

    if (read_bytes(product, byte_offset, 2, &days) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 2, 4, &msecs) != 0)
    {
        return -1;
    }
    if (read_bytes(product, byte_offset + 6, 2, &musecs) != 0)
    {
        return -1;
    }
    {
        union
        {
            uint8_t as_bytes[2];
            uint16_t as_uint16;
        } data;

        data.as_bytes[0] = ((uint8_t *)&days)[1];
        data.as_bytes[1] = ((uint8_t *)&days)[0];
        days = data.as_uint16;

        data.as_bytes[0] = ((uint8_t *)&musecs)[1];
        data.as_bytes[1] = ((uint8_t *)&musecs)[0];
        musecs = data.as_uint16;
    }
    {
        union
        {
            uint8_t as_bytes[4];
            uint32_t as_uint32;
        } data;

        data.as_bytes[0] = ((uint8_t *)&msecs)[3];
        data.as_bytes[1] = ((uint8_t *)&msecs)[2];
        data.as_bytes[2] = ((uint8_t *)&msecs)[1];
        data.as_bytes[3] = ((uint8_t *)&msecs)[0];
        msecs = data.as_uint32;
    }
#endif

    *dst = days * 86400.0 + msecs / 1000.0 + musecs / 1000000.0;

    return 0;
}

int coda_bin_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary int8 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary uint8 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary int16 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
            return -1;
        }
    }

    if (bit_size < 16)
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary uint16 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary int32 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary uint32 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary int64 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_size = ((coda_type *)cursor->stack[cursor->n - 1].type)->bit_size;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
            coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s1);
            coda_set_error(CODA_ERROR_PRODUCT,
                           "possible product error detected in %s (invalid bit size (%s) for binary uint64 integer - "
                           "byte:bit offset = %s:%d)", cursor->product->filename, s1, s2,
                           (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
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
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    coda_endianness endianness = ((coda_type_number *)cursor->stack[cursor->n - 1].type)->endianness;

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

static int read_vsf_integer(const coda_cursor *cursor, double *dst)
{
    coda_cursor vsf_cursor;
    int32_t scale_factor;
    double base_value;

    vsf_cursor = *cursor;
    if (coda_bin_cursor_use_base_type_of_special_type(&vsf_cursor) != 0)
    {
        return -1;
    }
    if (coda_cursor_goto_record_field_by_name(&vsf_cursor, "scale_factor") != 0)
    {
        return -1;
    }
    if (coda_cursor_read_int32(&vsf_cursor, &scale_factor) != 0)
    {
        return -1;
    }
    coda_cursor_goto_parent(&vsf_cursor);
    if (coda_cursor_goto_record_field_by_name(&vsf_cursor, "value") != 0)
    {
        return -1;
    }
    if (coda_cursor_read_double(&vsf_cursor, &base_value) != 0)
    {
        return -1;
    }

    /* Apply scaling factor */
    *dst = a_pow10_b(base_value, (long)-scale_factor);

    return 0;
}

static int read_time(const coda_cursor *cursor, double *dst)
{
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;

    switch (((coda_type_special *)cursor->stack[cursor->n - 1].type)->time_type)
    {
        case datetime_binary_envisat:
            return read_binary_envisat_datetime(cursor->product, bit_offset >> 3, dst);
        case datetime_binary_gome:
            return read_binary_gome_datetime(cursor->product, bit_offset >> 3, dst);
        case datetime_binary_eps:
            return read_binary_eps_datetime(cursor->product, bit_offset >> 3, dst);
        case datetime_binary_eps_long:
            return read_binary_eps_datetime_long(cursor->product, bit_offset >> 3, dst);
        default:
            break;
    }

    assert(0);
    exit(1);
}

int coda_bin_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    if (((coda_type *)cursor->stack[cursor->n - 1].type)->type_class == coda_special_class)
    {
        switch (((coda_type_special *)cursor->stack[cursor->n - 1].type)->special_type)
        {
            case coda_special_vsf_integer:
                return read_vsf_integer(cursor, dst);
            case coda_special_time:
                return read_time(cursor, dst);
            default:
                coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
                return -1;
        }
    }

    return read_double(cursor, dst);
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

int coda_bin_cursor_read_double_pair(const coda_cursor *cursor, double *dst)
{
    coda_cursor pair_cursor;

    if (((coda_type *)cursor->stack[cursor->n - 1].type)->type_class != coda_special_class ||
        ((coda_type_special *)cursor->stack[cursor->n - 1].type)->special_type != coda_special_complex)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a paired double data type");
        return -1;
    }

    pair_cursor = *cursor;
    if (coda_cursor_use_base_type_of_special_type(&pair_cursor) != 0)
    {
        return -1;
    }
    if (coda_cursor_goto_record_field_by_index(&pair_cursor, 0) != 0)
    {
        return -1;
    }
    if (coda_cursor_read_double(&pair_cursor, &dst[0]) != 0)
    {
        return -1;
    }
    if (coda_cursor_goto_next_record_field(&pair_cursor) != 0)
    {
        return -1;
    }
    if (coda_cursor_read_double(&pair_cursor, &dst[1]) != 0)
    {
        return -1;
    }

    return 0;
}

/** @} */
