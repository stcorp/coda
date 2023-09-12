/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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

int coda_ascii_reopen_with_definition(coda_product **product, const coda_product_definition *definition)
{
    coda_ascii_product *product_file;

    assert(definition != NULL);
    assert((*product)->format == coda_format_binary);
    assert(definition->format == coda_format_ascii);

    /* copy information of binary raw product to new ascii product */
    product_file = (coda_ascii_product *)malloc(sizeof(coda_ascii_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_ascii_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = definition->format;
    product_file->root_type = (coda_dynamic_type *)definition->root_type;
    product_file->product_definition = definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = (*product)->mem_size;
    (*product)->mem_size = 0;
    product_file->mem_ptr = (*(coda_bin_product **)product)->mem_ptr;
    (*product)->mem_ptr = NULL;

    product_file->use_mmap = (*(coda_bin_product **)product)->use_mmap;
    product_file->fd = (*(coda_bin_product **)product)->fd;
    (*(coda_bin_product **)product)->fd = -1;

#ifdef WIN32
    product_file->file = (*(coda_bin_product **)product)->file;
    (*(coda_bin_product **)product)->file = INVALID_HANDLE_VALUE;
    product_file->file_mapping = (*(coda_bin_product **)product)->file_mapping;
    (*(coda_bin_product **)product)->file_mapping = INVALID_HANDLE_VALUE;
#endif

    product_file->end_of_line = eol_unknown;
    product_file->num_asciilines = -1;
    product_file->asciiline_end_offset = NULL;
    product_file->lastline_ending = eol_unknown;
    product_file->asciilines = NULL;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        free(product_file);
        return -1;
    }

    coda_close(*product);
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
                       "product error detected (inconsistent end-of-line sequence - got %s but expected %s)",
                       eol_type_to_string(end_of_line), eol_type_to_string(product_file->end_of_line));
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
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to start of file (%s)", strerror(errno));
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
        if (read_bytes((coda_product *)product_file, byte_offset, blocksize, buffer) != 0)
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
                asciiline_end_offset[num_asciilines] = (long)byte_offset + i + 1;
                num_asciilines++;
                lastline_ending = eol_unknown;
                if (buffer[i] == '\n')
                {
                    lastline_ending = eol_lf;
                    if (verify_eol_type(product_file, eol_lf) != 0)
                    {
                        free(asciiline_end_offset);
                        return -1;
                    }
                }
                else if (buffer[i] == '\r')
                {
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
