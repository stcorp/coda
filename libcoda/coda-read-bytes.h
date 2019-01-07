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

#ifndef CODA_READ_BYTES_H
#define CODA_READ_BYTES_H

#include "coda-bin-internal.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* be careful not to bypass this function and try to access mem_ptr directly by casting its contents to
 * e.g. int16, int32, etc. This will not work since many platforms require these types of data to start
 * at a word aligned boundary memory address.
 * For such data types to be read from mem_ptr, you will first have to copy the value into a proper
 * word aligned memory address (which is the pointer you pass as 'dst').
 * Accessing data as a char array from mem_ptr can however be done safely (and can thus potentially be
 * done without using this function, if there is a need for it).
 */

static int read_bytes(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    if (product->mem_ptr != NULL)
    {
        if (((uint64_t)byte_offset + length) > ((uint64_t)product->mem_size))
        {
            if (product->format == coda_format_ascii || product->format == coda_format_binary)
            {
                coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
                return -1;
            }
            else
            {
                char accessed_str[21];
                char offset_str[21];
                char size_str[21];

                coda_str64(length, accessed_str);
                coda_str64(byte_offset, offset_str);
                coda_str64(product->mem_size, size_str);
                coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read %s bytes at position %s in block of "
                               "size %s", accessed_str, offset_str, size_str);
                return -1;
            }
        }
        memcpy(dst, product->mem_ptr + byte_offset, (size_t)length);
    }
    else
    {
        assert(product->format == coda_format_ascii || product->format == coda_format_binary);
        if (((uint64_t)byte_offset + length) > ((uint64_t)product->file_size))
        {
            coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
            return -1;
        }
#if HAVE_PREAD
        if (pread(((coda_bin_product *)product)->fd, dst, (size_t)length, (off_t)byte_offset) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
            return -1;
        }
#else
        if (lseek(((coda_bin_product *)product)->fd, (off_t)byte_offset, SEEK_SET) < 0)
        {
            char byte_offset_str[21];

            coda_str64(byte_offset, byte_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s (%s)", byte_offset_str,
                           strerror(errno));
            return -1;
        }
        if (read(((coda_bin_product *)product)->fd, dst, (size_t)length) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
            return -1;
        }
#endif
    }

    return 0;
}

#endif
