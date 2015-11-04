/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

int coda_grib_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_grib_dynamic_record *type;

    type = (coda_grib_dynamic_record *)cursor->stack[cursor->n - 1].type;
    if (index < 0 || index >= type->definition->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       type->definition->num_fields, __FILE__, __LINE__);
        return -1;
    }

    cursor->n++;
    if (type->field_type[index] != NULL)
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)type->field_type[index];
    }
    else
    {
        cursor->stack[cursor->n - 1].type = coda_bin_no_data_singleton();
    }
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;

    return 0;
}

int coda_grib_cursor_goto_next_record_field(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_grib_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
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
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_grib_dynamic_type *base_type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        if (type->tag == tag_grib_value_array)
        {
            num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
        }
        else
        {
            num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
        }
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    if (type->tag == tag_grib_value_array)
    {
        base_type = ((coda_grib_dynamic_value_array *)type)->base_type;
    }
    else
    {
        base_type = ((coda_grib_dynamic_array *)type)->element_type[index];
    }
    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)base_type;
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
    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)coda_grib_empty_dynamic_record();
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* virtual types do not have a bit offset */
    return 0;
}

int coda_grib_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    return coda_grib_type_get_string_length((coda_type *)cursor->stack[cursor->n - 1].type, length);
}

int coda_grib_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->tag)
    {
        case tag_grib_record:
            *num_elements = ((coda_grib_dynamic_record *)type)->definition->num_fields;
            break;
        case tag_grib_array:
            *num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
            break;
        case tag_grib_value_array:
            *num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
            break;
        case tag_grib_integer:
        case tag_grib_real:
        case tag_grib_text:
        case tag_grib_raw:
        case tag_grib_value:
            *num_elements = 1;
            break;
    }
    return 0;
}

int coda_grib_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->tag)
    {
        case tag_grib_raw:
            *bit_size = 8 * (int64_t)((coda_grib_dynamic_raw *)type)->length;
            break;
        case tag_grib_record:
        case tag_grib_array:
        case tag_grib_value_array:
        case tag_grib_integer:
        case tag_grib_real:
        case tag_grib_text:
        case tag_grib_value:
            *bit_size = -1;
            break;
    }
    return 0;
}

int coda_grib_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available)
{
    coda_grib_dynamic_record *type;

    type = (coda_grib_dynamic_record *)cursor->stack[cursor->n - 1].type;
    if (index < 0 || index >= type->definition->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       type->definition->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *available = (type->field_type[index] != NULL);
    return 0;
}

int coda_grib_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    *num_dims = 1;
    if (type->tag == tag_grib_value_array)
    {
        dim[0] = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        dim[0] = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    return 0;
}

static int read_basic_type(const coda_cursor *cursor, void *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_grib_value)
    {
        coda_grib_dynamic_value_array *array;
        long index;
        int64_t ivalue = 0;
        double fvalue = 0;
        uint8_t *buffer;

        assert(cursor->n > 1);
        array = (coda_grib_dynamic_value_array *)cursor->stack[cursor->n - 2].type;
        assert(array->tag == tag_grib_value_array);
        fvalue = array->referenceValue;
        if (array->element_bit_size == 0)
        {
            *((float *)dst) = fvalue;
            return 0;
        }
        index = cursor->stack[cursor->n - 1].index;
        buffer = &((uint8_t *)&ivalue)[8 - bit_size_to_byte_size(array->element_bit_size)];
        read_bits((coda_grib_product *)cursor->product, array->bit_offset + index * array->element_bit_size,
                  array->element_bit_size, buffer);
#ifndef WORDS_BIGENDIAN
        swap8(&ivalue);
#endif
        fvalue += ivalue * fpow(2, array->binaryScaleFactor);
        fvalue *= fpow(10, -array->decimalScaleFactor);
        *((float *)dst) = fvalue;
    }
    else
    {
        switch (type->definition->read_type)
        {
            case coda_native_type_int8:
                *(uint8_t *)dst = (uint8_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_uint8:
                *(uint8_t *)dst = (uint8_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_int16:
                *(uint16_t *)dst = (uint16_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_uint16:
                *(uint16_t *)dst = (uint16_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_int32:
                *(int32_t *)dst = (int32_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_uint32:
                *(uint32_t *)dst = (uint32_t)((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_int64:
                *(int64_t *)dst = ((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_uint64:
                *(uint64_t *)dst = ((coda_grib_dynamic_integer *)type)->value;
                break;
            case coda_native_type_float:
                *(float *)dst = (float)((coda_grib_dynamic_real *)type)->value;
                break;
            case coda_native_type_double:
                *(double *)dst = ((coda_grib_dynamic_real *)type)->value;
                break;
            case coda_native_type_char:
                *(char *)dst = ((coda_grib_dynamic_text *)type)->text[0];
                break;
            default:
                assert(0);
                exit(1);
        }
    }

    return 0;
}

static int read_array(const coda_cursor *cursor, void *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        coda_grib_dynamic_value_array *array = (coda_grib_dynamic_value_array *)type;

        assert(read_type == coda_native_type_float);
        if (array->num_elements > 0)
        {
            coda_cursor element_cursor = *cursor;

            element_cursor.n++;
            element_cursor.stack[element_cursor.n - 1].type = (coda_dynamic_type *)array->base_type;
            element_cursor.stack[element_cursor.n - 1].bit_offset = -1;
            for (i = 0; i < array->num_elements; i++)
            {
                element_cursor.stack[element_cursor.n - 1].index = i;
                if (read_basic_type(&element_cursor, &((float *)dst)[i]) != 0)
                {
                    return -1;
                }
            }
        }
    }
    else
    {
        coda_grib_dynamic_array *array = (coda_grib_dynamic_array *)type;

        num_elements = array->num_elements;
        switch (read_type)
        {
            case coda_native_type_int8:
                for (i = 0; i < num_elements; i++)
                {
                    ((uint8_t *)dst)[i] = (uint8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_uint8:
                for (i = 0; i < num_elements; i++)
                {
                    ((int8_t *)dst)[i] = (int8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_int16:
                for (i = 0; i < num_elements; i++)
                {
                    ((uint8_t *)dst)[i] = (uint8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_uint16:
                for (i = 0; i < num_elements; i++)
                {
                    ((int8_t *)dst)[i] = (int8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_int32:
                for (i = 0; i < num_elements; i++)
                {
                    ((uint8_t *)dst)[i] = (uint8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_uint32:
                for (i = 0; i < num_elements; i++)
                {
                    ((int8_t *)dst)[i] = (int8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_int64:
                for (i = 0; i < num_elements; i++)
                {
                    ((uint8_t *)dst)[i] = (uint8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_uint64:
                for (i = 0; i < num_elements; i++)
                {
                    ((int8_t *)dst)[i] = (int8_t)((coda_grib_dynamic_integer *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_float:
                for (i = 0; i < num_elements; i++)
                {
                    ((float *)dst)[i] = (float)((coda_grib_dynamic_real *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_double:
                for (i = 0; i < num_elements; i++)
                {
                    ((double *)dst)[i] = ((coda_grib_dynamic_real *)array->element_type[i])->value;
                }
                break;
            case coda_native_type_char:
                for (i = 0; i < num_elements; i++)
                {
                    ((char *)dst)[i] = ((coda_grib_dynamic_text *)array->element_type[i])->text[0];
                }
                break;
            default:
                assert(0);
                exit(1);
        }
    }

    return 0;
}

int coda_grib_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
            return read_basic_type(cursor, dst);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_uint8:
            return read_basic_type(cursor, dst);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int16_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int16_t)(*(uint8_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            if (type->definition->read_type == coda_native_type_uint8)
            {
                *dst = (uint16_t)(*(uint8_t *)dst);
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int32_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int32_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (int32_t)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (int32_t)(*(uint16_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_uint8:
                    *dst = (uint32_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (uint32_t)(*(uint16_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int64_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int64_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (int64_t)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (int64_t)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (int64_t)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (int64_t)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_uint8:
                    *dst = (uint64_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (uint64_t)(*(uint16_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (uint64_t)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_float:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_int8:
                    *dst = (float)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (float)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (float)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (float)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (float)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (float)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        case coda_native_type_int64:
        case coda_native_type_uint64:
            {
                uint64_t buffer;

                if (read_basic_type(cursor, &buffer) != 0)
                {
                    return -1;
                }
                if (type->definition->read_type == coda_native_type_int64)
                {
                    *dst = (float)(*(int64_t *)&buffer);
                }
                else
                {
                    *dst = (float)(int64_t)(*(uint64_t *)&buffer);
                }
            }
            return 0;
        case coda_native_type_double:
            {
                double buffer;

                if (read_basic_type(cursor, &buffer) != 0)
                {
                    return -1;
                }
                else
                {
                    *dst = (float)buffer;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    switch (type->definition->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_float:
        case coda_native_type_double:
            if (read_basic_type(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->definition->read_type)
            {
                case coda_native_type_int8:
                    *dst = (double)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (double)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (double)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (double)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (double)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (double)(*(uint32_t *)dst);
                    break;
                case coda_native_type_int64:
                    *dst = (double)(*(int64_t *)dst);
                    break;
                case coda_native_type_uint64:
                    *dst = (double)(int64_t)(*(uint64_t *)dst);
                    break;
                case coda_native_type_float:
                    *dst = (double)(*(float *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;

    if (type->tag == tag_grib_text && type->definition->read_type == coda_native_type_char)
    {
        return read_basic_type(cursor, dst);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                   coda_type_get_native_type_name(type->definition->read_type));
    return -1;
}

int coda_grib_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    coda_grib_dynamic_text *type = (coda_grib_dynamic_text *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_grib_text);
    if (dst_size > 0)
    {
        strncpy(dst, type->text, dst_size - 1);
    }
    dst[dst_size - 1] = '\0';
    return 0;
}

int coda_grib_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    coda_grib_dynamic_raw *type = (coda_grib_dynamic_raw *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_grib_raw);
    if (offset + length > type->length)
    {
        coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the size of the raw type");
        return -1;
    }
    memcpy(dst, &type->data[offset], length);
    return 0;
}

int coda_grib_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;

    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;

    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int16_t *)dst)[i] = (int16_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int16_t *)dst)[i] = (int16_t)((uint8_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 2) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint16_t *)dst)[i] = (uint16_t)((uint8_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 2) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((uint16_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 4) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint32_t *)dst)[i] = (uint32_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint32_t *)dst)[i] = (uint32_t)((uint16_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 4) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_float:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((float *)dst)[i] = (float)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((float *)dst)[i] = (float)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_double:
            {
                void *buffer;

                /* we need to read data with 8 byte element size, while a float has only 4 bytes */
                /* we therefore read the data first into a buffer that can fit the full array of 8 byte elements */
                buffer = malloc(num_elements * 8);
                if (buffer == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   num_elements * 8, __FILE__, __LINE__);
                    return -1;
                }
                if (read_array(cursor, buffer) != 0)
                {
                    free(buffer);
                    return -1;
                }
                switch (read_type)
                {
                    case coda_native_type_int64:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)((int64_t *)buffer)[i];
                        }
                        break;
                    case coda_native_type_uint64:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)(int64_t)((uint64_t *)buffer)[i];
                        }
                        break;
                    case coda_native_type_double:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)((double *)buffer)[i];
                        }
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
                free(buffer);
                if (array_ordering != coda_array_ordering_c)
                {
                    long dim[CODA_MAX_NUM_DIMS];
                    int num_dims;

                    if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                    {
                        return -1;
                    }
                    if (coda_array_transpose(dst, num_dims, dim, 4) != 0)
                    {
                        return -1;
                    }
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;
    long num_elements;
    long i;

    if (type->tag == tag_grib_value_array)
    {
        num_elements = ((coda_grib_dynamic_value_array *)type)->num_elements;
    }
    else
    {
        num_elements = ((coda_grib_dynamic_array *)type)->num_elements;
    }

    switch (read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_float:
        case coda_native_type_double:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int64:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((double *)dst)[i] = (double)((int64_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint64:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((double *)dst)[i] = (double)(int64_t)((uint64_t *)dst)[i];
                    }
                    break;
                case coda_native_type_float:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((float *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}

int coda_grib_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_grib_dynamic_type *type = (coda_grib_dynamic_type *)cursor->stack[cursor->n - 1].type;
    coda_native_type read_type = ((coda_grib_array *)type->definition)->base_type->read_type;

    switch (read_type)
    {
        case coda_native_type_char:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                if (coda_grib_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }
                if (coda_array_transpose(dst, num_dims, dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                   coda_type_get_native_type_name(read_type));
    return -1;
}
