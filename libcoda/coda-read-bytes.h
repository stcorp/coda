/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            return -1;
        }
#else
        if (lseek(((coda_bin_product *)product)->fd, (off_t)byte_offset, SEEK_SET) < 0)
        {
            char byte_offset_str[21];

            coda_str64(byte_offset, byte_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                           byte_offset_str, product->filename, strerror(errno));
            return -1;
        }
        if (read(((coda_bin_product *)product)->fd, dst, (size_t)length) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product->filename,
                           strerror(errno));
            return -1;
        }
#endif
    }

    return 0;
}

#endif
