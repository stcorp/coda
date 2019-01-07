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

#ifndef CODA_READ_BITS_H
#define CODA_READ_BITS_H

#include "coda-read-bytes.h"

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

#endif
