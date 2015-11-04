/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-grib-internal.h"
#include "coda-bin.h"

#define bit_size_to_byte_size(x) (((x) >> 3) + ((((uint8_t)(x)) & 0x7) != 0))

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

#ifndef WORDS_BIGENDIAN
static void swap8(void *value)
{
    union
    {
        uint8_t as_bytes[8];
        int64_t as_int64;
    } v1, v2;

    v1.as_int64 = *(int64_t *)value;

    v2.as_bytes[0] = v1.as_bytes[7];
    v2.as_bytes[1] = v1.as_bytes[6];
    v2.as_bytes[2] = v1.as_bytes[5];
    v2.as_bytes[3] = v1.as_bytes[4];
    v2.as_bytes[4] = v1.as_bytes[3];
    v2.as_bytes[5] = v1.as_bytes[2];
    v2.as_bytes[6] = v1.as_bytes[1];
    v2.as_bytes[7] = v1.as_bytes[0];

    *(int64_t *)value = v2.as_int64;
}
#endif

static int read_bytes(const coda_grib_product *product_file, int64_t byte_offset, int64_t length, void *dst)
{
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

static int read_bits(const coda_grib_product *product, int64_t bit_offset, int64_t bit_length, uint8_t *dst)
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
    int64_t ivalue = 0;
    double fvalue = 0;
    uint8_t *buffer;

    assert(cursor->n > 1);
    array = (coda_grib_value_array *)cursor->stack[cursor->n - 2].type;
    assert(array->definition->type_class == coda_array_class);
    fvalue = array->referenceValue;
    if (array->element_bit_size == 0)
    {
        *((float *)dst) = (float)fvalue;
        return 0;
    }
    index = cursor->stack[cursor->n - 1].index;
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
    read_bits((coda_grib_product *)cursor->product, array->bit_offset + index * array->element_bit_size,
              array->element_bit_size, buffer);
#ifndef WORDS_BIGENDIAN
    swap8(&ivalue);
#endif
    fvalue += ivalue * fpow(2, array->binaryScaleFactor);
    fvalue *= fpow(10, -array->decimalScaleFactor);
    *((float *)dst) = (float)fvalue;

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
