/*
 * Copyright (C) 2007-2014 S[&]T, The Netherlands.
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
#include "coda-read-bytes.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define ASCII_PARSE_BLOCK_SIZE 4096

int coda_ascii_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                    coda_product **product)
{
    coda_ascii_product *product_file;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, NULL);
        return -1;
    }

    product_file = (coda_ascii_product *)malloc(sizeof(coda_ascii_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_ascii_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = definition->format;
    product_file->root_type = (coda_dynamic_type *)definition->root_type;
    product_file->product_definition = definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
#if CODA_USE_QIAP
    product_file->qiap_info = NULL;
#endif
    product_file->use_mmap = 0;
    product_file->fd = -1;

    product_file->end_of_line = eol_unknown;
    product_file->num_asciilines = -1;
    product_file->asciiline_end_offset = NULL;
    product_file->lastline_ending = eol_unknown;
    product_file->asciilines = NULL;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_ascii_close((coda_product *)product_file);
        return -1;
    }

    if (coda_bin_product_open((coda_bin_product *)product_file) != 0)
    {
        coda_ascii_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_ascii_close(coda_product *product)
{
    coda_ascii_product *product_file = (coda_ascii_product *)product;

    if (coda_bin_product_close((coda_bin_product *)product_file) != 0)
    {
        return -1;
    }

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    if (product_file->asciiline_end_offset != NULL)
    {
        free(product_file->asciiline_end_offset);
    }
    if (product_file->asciilines != NULL)
    {
        coda_type_release(product_file->asciilines);
    }

    free(product_file);

    return 0;
}

static char *eol_type_to_string(eol_type end_of_line)
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

static int verify_eol_type(coda_ascii_product *product_file, eol_type end_of_line)
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

int coda_ascii_init_asciilines(coda_product *product)
{
    char buffer[ASCII_PARSE_BLOCK_SIZE + 1];
    coda_ascii_product *product_file = (coda_ascii_product *)product;
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
        if (read_bytes(product, byte_offset, blocksize, buffer) != 0)
        {
            return -1;
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
    product_file->lastline_ending = lastline_ending;

    return 0;
}
