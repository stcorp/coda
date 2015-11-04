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

#include "coda-bin-internal.h"

#include "coda-definition.h"
#include "coda-bin-definition.h"

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
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define DETECTION_BLOCK_SIZE 4096

static coda_ProductDefinition *evaluate_detection_node(char *buffer, int blocksize, const char *filename,
                                                       int64_t filesize, coda_ascbinDetectionNode *node)
{
    int i;

    if (node == NULL)
    {
        return NULL;
    }
    if (node->entry != NULL)
    {
        coda_DetectionRuleEntry *entry;

        entry = node->entry;
        if (entry->use_filename)
        {
            /* match value on filename */
            if (entry->offset + entry->value_length > strlen(filename))
            {
                /* filename is not long enough for a match */
                return NULL;
            }
            if (memcmp(&filename[entry->offset], entry->value, entry->value_length) != 0)
            {
                /* no match */
                return NULL;
            }
        }
        else if (entry->offset != -1)
        {
            if (entry->value_length > 0)
            {
                /* match value at offset */
                if (entry->offset + entry->value_length > blocksize)
                {
                    /* we can't match data outside the detection block */
                    return NULL;
                }
                if (memcmp(&buffer[entry->offset], entry->value, entry->value_length) != 0)
                {
                    /* no match */
                    return NULL;
                }
            }
            else
            {
                /* file size check */
                if (entry->offset != filesize)
                {
                    /* wrong file size */
                    return NULL;
                }
            }
        }
        else if (entry->value_length > 0)
        {
            /* position independent string match on block */
            if (strstr(buffer, entry->value) == NULL)
            {
                /* value does not occur as string in buffer */
                return NULL;
            }
        }
        else
        {
            assert(0);
            exit(1);
        }
    }

    for (i = 0; i < node->num_subnodes; i++)
    {
        coda_ProductDefinition *definition;

        definition = evaluate_detection_node(buffer, blocksize, filename, filesize, node->subnode[i]);
        if (definition != NULL)
        {
            return definition;
        }
    }

    if (node->rule != NULL)
    {
        return node->rule->product_definition;
    }

    return NULL;
}

int coda_ascbin_recognize_file(const char *filename, int64_t size, coda_ProductDefinition **definition)
{
    char buffer[DETECTION_BLOCK_SIZE + 1];
    const char *basefilename;
    coda_ascbinDetectionNode *node;
    int64_t blocksize;
    int open_flags;
    int fd;

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    fd = open(filename, open_flags);
    if (fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", filename, strerror(errno));
        return -1;
    }

    blocksize = size;
    if (blocksize > DETECTION_BLOCK_SIZE)
    {
        blocksize = DETECTION_BLOCK_SIZE;
    }

    if (read(fd, buffer, (long)blocksize) == -1)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", filename, strerror(errno));
        close(fd);
        return -1;
    }
    buffer[blocksize] = '\0';   /* terminate with zero for cases where the buffer is used as a string */

    basefilename = strrchr(filename, '/');
    if (basefilename == NULL)
    {
        basefilename = filename;
    }
    else
    {
        basefilename = &basefilename[1];
    }

    close(fd);

    node = coda_ascbin_get_detection_tree();
    *definition = evaluate_detection_node(buffer, (int)blocksize, basefilename, size, node);

    return 0;
}

int coda_ascbin_open(const char *filename, int64_t file_size, coda_ProductFile **pf)
{
    coda_ascbinProductFile *product_file;
    coda_ProductDefinition *product_definition;

    if (coda_ascbin_recognize_file(filename, file_size, &product_definition) != 0)
    {
        return -1;
    }
    if (product_definition == NULL)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, NULL);
        return -1;
    }
    if (product_definition->root_type == NULL)
    {
        if (coda_read_product_definition(product_definition) != 0)
        {
            return -1;
        }
    }

    product_file = (coda_ascbinProductFile *)malloc(sizeof(coda_ascbinProductFile));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_ascbinProductFile), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = product_definition->format;
    product_file->root_type = (coda_DynamicType *)product_definition->root_type;
    product_file->product_definition = product_definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->use_mmap = 0;
    product_file->fd = -1;
    product_file->mmap_ptr = NULL;
#ifdef WIN32
    product_file->file_mapping = INVALID_HANDLE_VALUE;
    product_file->file = INVALID_HANDLE_VALUE;
#endif
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
        coda_ascbin_close((coda_ProductFile *)product_file);
        return -1;
    }

    if (coda_option_use_mmap)
    {
        /* Perform an mmap() of the file, filling the following fields:
         *   pf->use_mmap = 1
         *   pf->file         (windows only )
         *   pf->file_mapping (windows only )
         *   pf->mmap_ptr     (windows, *nix)
         */
#ifdef WIN32
        product_file->use_mmap = 1;
        product_file->file = CreateFile(product_file->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, NULL);
        if (product_file->file == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                coda_set_error(CODA_ERROR_FILE_NOT_FOUND, "could not find %s", product_file->filename);
            }
            else
            {
                LPVOID lpMsgBuf;

                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
                {
                    /* Set error without additional information */
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", product_file->filename);
                }
                else
                {
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename,
                                   (LPCTSTR) lpMsgBuf);
                    LocalFree(lpMsgBuf);
                }
            }
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;  /* indicate failure */
        }

        /* Try to do file mapping */
        product_file->file_mapping = CreateFileMapping(product_file->file, NULL, PAGE_READONLY, 0,
                                                       (int32_t)product_file->file_size, 0);
        if (product_file->file_mapping == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product_file->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;
        }

        product_file->mmap_ptr = (uint8_t *)MapViewOfFile(product_file->file_mapping, FILE_MAP_READ, 0, 0, 0);
        if (product_file->mmap_ptr == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product_file->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;
        }
#else
        int fd;

        product_file->use_mmap = 1;
        fd = open(product_file->filename, O_RDONLY);
        if (fd < 0)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename,
                           strerror(errno));
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;
        }

        product_file->mmap_ptr = (uint8_t *)mmap(0, product_file->file_size, PROT_READ, MAP_SHARED, fd, 0);
        if (product_file->mmap_ptr == (uint8_t *)MAP_FAILED)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                           strerror(errno));
            product_file->mmap_ptr = NULL;
            close(fd);
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;
        }

        /* close file descriptor (the file handle is not needed anymore) */
        close(fd);
#endif
    }
    else
    {
        int open_flags;

        /* Perform a normal open of the file, filling the following fields:
         *   pf->use_mmap = 0
         *   pf->fd             (windows, *nix)
         */
        open_flags = O_RDONLY;
#ifdef WIN32
        open_flags |= _O_BINARY;
#endif
        product_file->fd = open(product_file->filename, open_flags);
        if (product_file->fd < 0)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename,
                           strerror(errno));
            coda_ascbin_close((coda_ProductFile *)product_file);
            return -1;
        }
    }

    *pf = (coda_ProductFile *)product_file;

    return 0;
}

int coda_ascbin_close(coda_ProductFile *pf)
{
    coda_ascbinProductFile *product_file = (coda_ascbinProductFile *)pf;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    if (product_file->use_mmap)
    {
#ifdef WIN32
        if (product_file->mmap_ptr != NULL)
        {
            UnmapViewOfFile(product_file->mmap_ptr);
        }
        if (product_file->file_mapping != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product_file->file_mapping);
        }
        if (product_file->file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product_file->file);
        }
#else
        if (product_file->mmap_ptr != NULL)
        {
            munmap((void *)product_file->mmap_ptr, product_file->file_size);
        }
#endif
    }
    else
    {
        if (product_file->fd >= 0)
        {
            close(product_file->fd);
        }
    }

    if (product_file->asciiline_end_offset != NULL)
    {
        free(product_file->asciiline_end_offset);
    }
    if (product_file->asciilines != NULL)
    {
        coda_release_dynamic_type(product_file->asciilines);
    }

    free(product_file);

    return 0;
}

coda_ascbinDetectionNode *coda_ascbin_get_detection_tree(void)
{
    return (coda_ascbinDetectionNode *)coda_data_dictionary->ascbin_detection_tree;
}
