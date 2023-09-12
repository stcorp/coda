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

#include "coda-bin-internal.h"
#include "coda-definition.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


int coda_bin_product_open(coda_bin_product *product)
{
    product->use_mmap = 0;
    product->fd = -1;
#ifdef WIN32
    product->file_mapping = INVALID_HANDLE_VALUE;
    product->file = INVALID_HANDLE_VALUE;
#endif

    if (coda_option_use_mmap && product->file_size > 0)
    {
        /* Perform an mmap() of the file, filling the following fields:
         *   product->use_mem_ptr = 1
         *   product->file         (windows only )
         *   product->file_mapping (windows only )
         *   product->mem_ptr      (windows, *nix)
         */
#ifdef WIN32
        product->use_mmap = 1;
        product->file = CreateFile(product->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (product->file == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                coda_set_error(CODA_ERROR_FILE_NOT_FOUND, "could not find %s", product->filename);
            }
            else
            {
                LPVOID lpMsgBuf;

                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
                {
                    /* Set error without additional information */
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", product->filename);
                }
                else
                {
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product->filename,
                                   (LPCTSTR) lpMsgBuf);
                    LocalFree(lpMsgBuf);
                }
            }
            return -1;  /* indicate failure */
        }

        /* Try to do file mapping */
        product->file_mapping = CreateFileMapping(product->file, NULL, PAGE_READONLY, 0,
                                                  (int32_t)product->file_size, 0);
        if (product->file_mapping == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            return -1;
        }

        product->mem_ptr = (uint8_t *)MapViewOfFile(product->file_mapping, FILE_MAP_READ, 0, 0, 0);
        if (product->mem_ptr == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            return -1;
        }
#else
        int fd;

        product->use_mmap = 1;
        fd = open(product->filename, O_RDONLY);
        if (fd < 0)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product->filename, strerror(errno));
            return -1;
        }

        product->mem_ptr = (uint8_t *)mmap(0, product->file_size, PROT_READ, MAP_SHARED, fd, 0);
        if (product->mem_ptr == (uint8_t *)MAP_FAILED)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product->filename,
                           strerror(errno));
            product->mem_ptr = NULL;
            close(fd);
            return -1;
        }

        /* close file descriptor (the file handle is not needed anymore) */
        close(fd);
#endif
        product->mem_size = product->file_size;
    }
    else
    {
        int open_flags;

        /* Perform a normal open of the file, filling the following fields:
         *   product->use_mem_ptr = 0
         *   product->fd             (windows, *nix)
         */
        open_flags = O_RDONLY;
#ifdef WIN32
        open_flags |= _O_BINARY;
#endif
        product->fd = open(product->filename, open_flags);
        if (product->fd < 0)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product->filename, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int coda_bin_product_close(coda_bin_product *product)
{
    if (product->use_mmap)
    {
#ifdef WIN32
        if (product->mem_ptr != NULL)
        {
            UnmapViewOfFile(product->mem_ptr);
            product->mem_ptr = NULL;
        }
        if (product->file_mapping != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product->file_mapping);
            product->file_mapping = INVALID_HANDLE_VALUE;
        }
        if (product->file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product->file);
            product->file = INVALID_HANDLE_VALUE;
        }
#else
        if (product->mem_ptr != NULL)
        {
            munmap((void *)product->mem_ptr, product->file_size);
            product->mem_ptr = NULL;
        }
#endif
        product->use_mmap = 0;
    }
    else
    {
        if (product->fd >= 0)
        {
            close(product->fd);
            product->fd = -1;
        }
    }

    return 0;
}

int coda_bin_open(const char *filename, int64_t file_size, coda_product **product)
{
    coda_bin_product *product_file;

    product_file = (coda_bin_product *)malloc(sizeof(coda_bin_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_bin_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_binary;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;

    product_file->use_mmap = 0;
    product_file->fd = -1;

    product_file->root_type = (coda_dynamic_type *)coda_type_raw_file_singleton();
    if (product_file->root_type == NULL)
    {
        coda_bin_close((coda_product *)product_file);
        return -1;
    }

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_bin_close((coda_product *)product_file);
        return -1;
    }

    if (coda_bin_product_open(product_file) != 0)
    {
        coda_bin_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_bin_reopen_with_definition(coda_product **product, const coda_product_definition *definition)
{
    coda_bin_product *product_file = *(coda_bin_product **)product;

    assert(definition != NULL);
    assert(product_file->format == coda_format_binary);
    assert(definition->format == coda_format_binary);

    product_file->root_type = (coda_dynamic_type *)definition->root_type;
    product_file->product_definition = definition;

    return 0;
}

int coda_bin_close(coda_product *product)
{
    coda_bin_product *product_file = (coda_bin_product *)product;

    if (coda_bin_product_close(product_file) != 0)
    {
        return -1;
    }

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    free(product_file);

    return 0;
}
