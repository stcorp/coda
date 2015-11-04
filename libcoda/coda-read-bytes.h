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

#ifndef CODA_READ_BYTES_H
#define CODA_READ_BYTES_H

#include "coda-bin-internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int read_bytes_in_bounds(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    coda_bin_product *product_file = (coda_bin_product *)product;

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

static int read_bytes(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    if (((uint64_t)byte_offset + length) > ((uint64_t)product->file_size))
    {
        coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
        return -1;
    }
    return read_bytes_in_bounds(product, byte_offset, length, dst);
}

#endif
