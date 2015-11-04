/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-definition.h"
#include "coda-ascii-definition.h"

#define ASCII_PARSE_BLOCK_SIZE 4096

const char *eol_type_to_string(eol_type end_of_line)
{
    switch (end_of_line)
    {
        case eol_cr:
            return "CR";
        case eol_lf:
            return "LF";
        case eol_crlf:
            return "CRLF";
        default:
            break;
    }

    assert(0);
    exit(1);
}

static int verify_eol_type(coda_ascbinProductFile *product_file, eol_type end_of_line)
{
    assert(end_of_line != eol_unknown);

    if (product_file->end_of_line == eol_unknown)
    {
        product_file->end_of_line = end_of_line;
        return 0;
    }

    if (product_file->end_of_line != end_of_line)
    {
        coda_set_error(CODA_ERROR_PRODUCT,
                       "product error detected in %s (inconsistent end-of-line sequence - got %s but expected %s)",
                       product_file->filename, eol_type_to_string(end_of_line),
                       eol_type_to_string(product_file->end_of_line));
        return -1;
    }

    return 0;
}

int coda_ascii_init_asciilines(coda_ProductFile *pf)
{
    char buffer[ASCII_PARSE_BLOCK_SIZE + 1];
    coda_ascbinProductFile *product_file = (coda_ascbinProductFile *)pf;
    long num_asciilines = 0;
    long *asciiline_end_offset = NULL;
    int64_t byte_offset = 0;
    char lastchar = '\0';       /* last character of previous block */
    eol_type lastline_ending = eol_unknown;

    assert(product_file->num_asciilines == -1);

    if (!product_file->use_mmap)
    {
        if (lseek(product_file->fd, 0, SEEK_SET) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to start of file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
    }

    for (;;)
    {
        int64_t blocksize = ASCII_PARSE_BLOCK_SIZE;
        long i;

        if (byte_offset + blocksize > product_file->file_size)
        {
            blocksize = product_file->file_size - byte_offset;
        }
        if (blocksize == 0)
        {
            break;
        }
        if (product_file->use_mmap)
        {
            memcpy(buffer, product_file->mmap_ptr + byte_offset, (size_t)blocksize);
        }
        else
        {
            if (read(product_file->fd, buffer, (size_t)blocksize) < 0)
            {
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                               strerror(errno));
                return -1;
            }
        }

        if (lastchar == '\r' && buffer[0] != '\n')
        {
            if (verify_eol_type(product_file, eol_cr) != 0)
            {
                free(asciiline_end_offset);
                return -1;
            }
        }

        for (i = 0; i < blocksize; i++)
        {
            if (i == 0 && lastchar == '\r' && buffer[0] == '\n')
            {
                asciiline_end_offset[num_asciilines - 1]++;
                lastline_ending = eol_crlf;
                if (verify_eol_type(product_file, eol_crlf) != 0)
                {
                    free(asciiline_end_offset);
                    return -1;
                }
            }
            else if (buffer[i] == '\r' || buffer[i] == '\n' || byte_offset + i == product_file->file_size - 1)
            {
                if (num_asciilines % BLOCK_SIZE == 0)
                {
                    long *new_offset;
                    new_offset = realloc(asciiline_end_offset, (num_asciilines + BLOCK_SIZE) * sizeof(long));
                    if (new_offset == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       (num_asciilines + BLOCK_SIZE) * sizeof(long), __FILE__, __LINE__);
                        if (asciiline_end_offset != NULL)
                        {
                            free(asciiline_end_offset);
                        }
                        return -1;
                    }
                    asciiline_end_offset = new_offset;
                }
                asciiline_end_offset[num_asciilines] = (long)byte_offset + i;
                num_asciilines++;
                lastline_ending = eol_unknown;
                if (buffer[i] == '\n')
                {
                    asciiline_end_offset[num_asciilines - 1]++;
                    lastline_ending = eol_lf;
                    if (verify_eol_type(product_file, eol_lf) != 0)
                    {
                        free(asciiline_end_offset);
                        return -1;
                    }
                }
                else if (buffer[i] == '\r')
                {
                    asciiline_end_offset[num_asciilines - 1]++;
                    lastline_ending = eol_cr;
                    if (i < blocksize - 1)
                    {
                        if (buffer[i + 1] == '\n')
                        {
                            lastline_ending = eol_crlf;
                            if (verify_eol_type(product_file, eol_crlf) != 0)
                            {
                                free(asciiline_end_offset);
                                return -1;
                            }
                            asciiline_end_offset[num_asciilines - 1]++;
                            i++;
                        }
                        else
                        {
                            if (verify_eol_type(product_file, eol_cr) != 0)
                            {
                                free(asciiline_end_offset);
                                return -1;
                            }
                        }
                    }
                }
            }
        }

        lastchar = buffer[blocksize - 1];
        byte_offset += blocksize;
    }

    if (lastchar == '\r')
    {
        if (verify_eol_type(product_file, eol_cr) != 0)
        {
            free(asciiline_end_offset);
            return -1;
        }
    }

    product_file->num_asciilines = num_asciilines;
    product_file->asciiline_end_offset = asciiline_end_offset;

    return 0;
}

int coda_ascii_open(const char *filename, int64_t file_size, coda_ProductFile **pf)
{
    return coda_ascbin_open(filename, file_size, coda_format_ascii, pf);
}

int coda_ascii_close(coda_ProductFile *pf)
{
    return coda_ascbin_close(pf);
}

int coda_ascii_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type)
{
    *type = (coda_Type *)dynamic_type;
    return 0;
}
