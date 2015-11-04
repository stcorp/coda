/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#include "coda-ascbin-internal.h"

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

#define DETECTION_BLOCK_SIZE 4096
#define ASCII_PARSE_BLOCK_SIZE 4096

static void delete_detection_node(coda_ascbin_detection_node *node)
{
    int i;

    if (node->subnode != NULL)
    {
        for (i = 0; i < node->num_subnodes; i++)
        {
            delete_detection_node(node->subnode[i]);
        }
        free(node->subnode);
    }
    free(node);
}

static coda_ascbin_detection_node *detection_node_new(void)
{
    coda_ascbin_detection_node *node;

    node = malloc(sizeof(coda_ascbin_detection_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_detection_node), __FILE__, __LINE__);
        return NULL;
    }
    node->entry = NULL;
    node->rule = NULL;
    node->num_subnodes = 0;
    node->subnode = NULL;

    return node;
}

static int detection_node_add_node(coda_ascbin_detection_node *node, coda_ascbin_detection_node *new_node)
{
    coda_detection_rule_entry *new_entry;
    int i;

    if (node->num_subnodes % BLOCK_SIZE == 0)
    {
        coda_ascbin_detection_node **new_subnode;

        new_subnode = realloc(node->subnode, (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_ascbin_detection_node *));
        if (new_subnode == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_ascbin_detection_node *), __FILE__,
                           __LINE__);
            return -1;
        }
        node->subnode = new_subnode;
    }

    new_entry = new_node->entry;
    node->num_subnodes++;
    for (i = node->num_subnodes - 1; i > 0; i--)
    {
        coda_detection_rule_entry *entry;

        /* check if subnode[i - 1] should be after new_node */
        entry = node->subnode[i - 1]->entry;
        if (new_entry->use_filename && !entry->use_filename)
        {
            /* filename checks go after path/data checks */
            node->subnode[i] = new_node;
            break;
        }
        if (entry->use_filename && !new_entry->use_filename)
        {
            /* filename checks go after path/data checks */
            node->subnode[i] = node->subnode[i - 1];
        }
        else
        {
            if (new_entry->value_length != 0)
            {
                if (entry->value_length != 0)
                {
                    if (entry->offset == -1)
                    {
                        /* value checks with offset go after value checks without offset */
                        node->subnode[i] = new_node;
                        break;
                    }
                    else
                    {
                        /* value checks with shorter values go after value checks with longer values */
                        if (entry->value_length < new_entry->value_length)
                        {
                            node->subnode[i] = node->subnode[i - 1];
                        }
                        else
                        {
                            node->subnode[i] = new_node;
                            break;
                        }
                    }
                }
                else
                {
                    /* size checks go after value checks */
                    node->subnode[i] = node->subnode[i - 1];
                }
            }
            else
            {
                if (entry->value_length != 0)
                {
                    /* size checks go after value checks */
                    node->subnode[i] = new_node;
                    break;
                }
                else
                {
                    /* size checks can go in any order */
                    node->subnode[i] = new_node;
                    break;
                }
            }
        }
    }
    if (i == 0)
    {
        node->subnode[0] = new_node;
    }

    return 0;
}

static coda_ascbin_detection_node *get_node_for_entry(coda_ascbin_detection_node *node,
                                                      coda_detection_rule_entry *entry)
{
    coda_ascbin_detection_node *new_node;
    int i;

    for (i = 0; i < node->num_subnodes; i++)
    {
        coda_detection_rule_entry *current_entry;

        /* check if entries are equal */
        current_entry = node->subnode[i]->entry;
        if (entry->use_filename == current_entry->use_filename && entry->offset == current_entry->offset &&
            entry->value_length == current_entry->value_length)
        {
            if (entry->value_length > 0)
            {
                if (memcmp(entry->value, current_entry->value, entry->value_length) == 0)
                {
                    /* same entry -> use this subnode */
                    return node->subnode[i];
                }
            }
            else
            {
                /* same entry -> use this subnode */
                return node->subnode[i];
            }
        }
    }

    /* create new node */
    new_node = detection_node_new();
    if (new_node == NULL)
    {
        return NULL;
    }
    new_node->entry = entry;
    if (detection_node_add_node(node, new_node) != 0)
    {
        delete_detection_node(new_node);
        return NULL;
    }

    return new_node;
}

void coda_ascbin_detection_tree_delete(void *detection_tree)
{
    delete_detection_node((coda_ascbin_detection_node *)detection_tree);
}

int coda_ascbin_detection_tree_add_rule(void *detection_tree, coda_detection_rule *detection_rule)
{
    coda_ascbin_detection_node *node;
    int i;

    if (detection_rule->num_entries == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' should have at least one entry",
                       detection_rule->product_definition->name);
        return -1;
    }
    for (i = 0; i < detection_rule->num_entries; i++)
    {
        if (detection_rule->entry[i]->path != NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule %d for '%s' can not be based on paths", i,
                           detection_rule->product_definition->name);
            return -1;
        }
        if (detection_rule->entry[i]->offset == -1 && detection_rule->entry[0]->value_length == 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule %d for '%s' has an empty entry", i,
                           detection_rule->product_definition->name);
            return -1;
        }
    }

    node = *(coda_ascbin_detection_node **)detection_tree;
    if (node == NULL)
    {
        node = detection_node_new();
        if (node == NULL)
        {
            return -1;
        }
        *(coda_ascbin_detection_node **)detection_tree = node;
    }
    for (i = 0; i < detection_rule->num_entries; i++)
    {
        node = get_node_for_entry(node, detection_rule->entry[i]);
        if (node == NULL)
        {
            return -1;
        }
    }
    if (node->rule != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' is shadowed by detection rule for '%s'",
                       detection_rule->product_definition->name, node->rule->product_definition->name);
        return -1;
    }
    node->rule = detection_rule;

    return 0;
}

static coda_product_definition *evaluate_detection_node(char *buffer, int blocksize, const char *filename,
                                                        int64_t filesize, coda_ascbin_detection_node *node)
{
    int i;

    if (node == NULL)
    {
        return NULL;
    }
    if (node->entry != NULL)
    {
        coda_detection_rule_entry *entry;

        entry = node->entry;
        if (entry->use_filename)
        {
            /* match value on filename */
            if (entry->offset + entry->value_length > (int64_t)strlen(filename))
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
        coda_product_definition *definition;

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

int coda_ascbin_recognize_file(const char *filename, int64_t size, coda_product_definition **definition)
{
    char buffer[DETECTION_BLOCK_SIZE + 1];
    const char *basefilename;
    coda_ascbin_detection_node *node;
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
        basefilename = strrchr(filename, '\\');
    }
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

int coda_ascbin_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                     coda_product **product)
{
    coda_ascbin_product *product_file;

    if (definition == NULL)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, NULL);
        return -1;
    }

    product_file = (coda_ascbin_product *)malloc(sizeof(coda_ascbin_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_ascbin_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = definition->format;
    product_file->root_type = (coda_dynamic_type *)definition->root_type;
    product_file->product_definition = definition;
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
        coda_ascbin_close((coda_product *)product_file);
        return -1;
    }

    if (coda_option_use_mmap)
    {
        /* Perform an mmap() of the file, filling the following fields:
         *   product->use_mmap = 1
         *   product->file         (windows only )
         *   product->file_mapping (windows only )
         *   product->mmap_ptr     (windows, *nix)
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
            coda_ascbin_close((coda_product *)product_file);
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
            coda_ascbin_close((coda_product *)product_file);
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
            coda_ascbin_close((coda_product *)product_file);
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
            coda_ascbin_close((coda_product *)product_file);
            return -1;
        }

        product_file->mmap_ptr = (uint8_t *)mmap(0, product_file->file_size, PROT_READ, MAP_SHARED, fd, 0);
        if (product_file->mmap_ptr == (uint8_t *)MAP_FAILED)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                           strerror(errno));
            product_file->mmap_ptr = NULL;
            close(fd);
            coda_ascbin_close((coda_product *)product_file);
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
         *   product->use_mmap = 0
         *   product->fd             (windows, *nix)
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
            coda_ascbin_close((coda_product *)product_file);
            return -1;
        }
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_ascbin_close(coda_product *product)
{
    coda_ascbin_product *product_file = (coda_ascbin_product *)product;

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
        coda_type_release(product_file->asciilines);
    }

    free(product_file);

    return 0;
}

coda_ascbin_detection_node *coda_ascbin_get_detection_tree(void)
{
    return (coda_ascbin_detection_node *)coda_global_data_dictionary->ascbin_detection_tree;
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

static int verify_eol_type(coda_ascbin_product *product_file, eol_type end_of_line)
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
    coda_ascbin_product *product_file = (coda_ascbin_product *)product;
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
    product_file->lastline_ending = lastline_ending;

    return 0;
}
