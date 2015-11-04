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

#include "coda-ascbin-internal.h"

#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* we use 16K + 16 bytes to also allow detection of HDF5 at superblock offset 16384 */
#define DETECTION_BLOCK_SIZE 16400

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

int coda_ascbin_recognize_file(const char *filename, int64_t size, coda_format *format,
                               coda_product_definition **definition)
{
    char buffer[DETECTION_BLOCK_SIZE + 1];
    const char *basefilename;
    coda_ascbin_detection_node *node;
    int64_t hdf5_offset;
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

    /* detect if we are dealing with a HDF5 product with some header information */
    hdf5_offset = 512;
    while (hdf5_offset + 8 < blocksize)
    {
        if (memcmp(&buffer[hdf5_offset], "\211HDF\r\n\032\n", 8) == 0)
        {
            *format = coda_format_hdf5;
            /* we still continue with the remaining detection functions as normal */
            break;
        }
        hdf5_offset *= 2;
    }

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
    if (*definition != NULL && *format == coda_format_binary)
    {
        *format = (*definition)->format;
    }

    return 0;
}

coda_ascbin_detection_node *coda_ascbin_get_detection_tree(void)
{
    return (coda_ascbin_detection_node *)coda_global_data_dictionary->ascbin_detection_tree;
}
