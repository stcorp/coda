/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/** Create root node for a new node tree
 * \param root_type Pointer to the root type of the product for which this node tree is applicable
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
coda_tree_node *coda_tree_node_new(const coda_type *root_type)
{
    coda_tree_node *node;

    node = malloc(sizeof(coda_tree_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_tree_node), __FILE__, __LINE__);
        return NULL;
    }
    node->type = root_type;
    node->num_items = 0;
    node->item = NULL;
    node->all_children = NULL;
    node->num_indexed_children = 0;
    node->index = NULL;
    node->indexed_child = NULL;

    return node;
}

/** Delete node and all subnodes
 * \param node Pointer to the node that should be deleted
 * \param free_time Optional deallocation function that will be called on all items that are attached to nodes
 */
void coda_tree_node_delete(coda_tree_node *node, void (*free_item) (void *))
{
    int i;

    if (node->all_children != NULL)
    {
        coda_tree_node_delete(node->all_children, free_item);
    }
    if (node->index != NULL)
    {
        free(node->index);
    }
    if (node->indexed_child != NULL)
    {
        for (i = 0; i < node->num_indexed_children; i++)
        {
            if (node->indexed_child[i] != NULL)
            {
                coda_tree_node_delete(node->indexed_child[i], free_item);
            }
        }
        free(node->indexed_child);
    }
    if (node->item != NULL)
    {
        if (free_item != NULL)
        {
            for (i = 0; i < node->num_items; i++)
            {
                if (node->item[i] != NULL)
                {
                    free_item(node->item[i]);
                }
            }
        }
        free(node->item);
    }
    free(node);
}

static int tree_node_add_item(coda_tree_node *node, void *item)
{
    void **new_item;

    new_item = realloc(node->item, (node->num_items + 1) * sizeof(void *));
    if (new_item == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_items + 1) * sizeof(void *), __FILE__, __LINE__);
        return -1;
    }
    node->item = new_item;

    node->item[node->num_items] = item;
    node->num_items++;

    return 0;
}

static int tree_node_get_node_for_all(coda_tree_node *node, coda_tree_node **sub_node, int create)
{
    if (node->all_children == NULL && create)
    {
        coda_type_class type_class;
        const coda_type *type = node->type;
        coda_type *sub_type;

        coda_type_get_class(type, &type_class);
        if (type_class == coda_special_class)
        {
            /* use base type */
            if (coda_type_get_special_base_type(type, &sub_type) != 0)
            {
                return -1;
            }
            type = sub_type;
            coda_type_get_class(type, &type_class);
        }
        assert(type_class == coda_array_class);
        if (coda_type_get_array_base_type(type, &sub_type) != 0)
        {
            return -1;
        }
        node->all_children = coda_tree_node_new(sub_type);
        if (node->all_children == NULL)
        {
            return -1;
        }
    }
    *sub_node = node->all_children;

    return 0;
}

static int tree_node_get_node_for_index(coda_tree_node *node, long index, coda_tree_node **sub_node, int create)
{
    int bottom = 0;
    int top = node->num_indexed_children - 1;

    if (node->num_indexed_children > 0)
    {
        while (top != bottom)
        {
            int middle = (bottom + top) / 2;

            if (index <= node->index[middle])
            {
                top = middle;
            }
            if (index > node->index[middle])
            {
                bottom = middle + 1;
            }
        }
    }

    if (node->num_indexed_children > 0 && index == node->index[top])
    {
        *sub_node = node->indexed_child[top];
    }
    else if (create)
    {
        coda_tree_node **new_indexed_child;
        coda_tree_node *child_node;
        const coda_type *type = node->type;
        coda_type *sub_type;
        long *new_index;
        int i;

        new_index = realloc(node->index, (node->num_indexed_children + 1) * sizeof(long));
        if (new_index == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(node->num_indexed_children + 1) * sizeof(long), __FILE__, __LINE__);
            return -1;
        }
        node->index = new_index;
        new_indexed_child = realloc(node->indexed_child, (node->num_indexed_children + 1) * sizeof(coda_tree_node *));
        if (new_indexed_child == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(node->num_indexed_children + 1) * sizeof(coda_tree_node *), __FILE__, __LINE__);
            return -1;
        }
        node->indexed_child = new_indexed_child;

        if (index == -1)
        {
            if (coda_type_get_attributes(type, &sub_type) != 0)
            {
                return -1;
            }
        }
        else
        {
            coda_type_class type_class;

            coda_type_get_class(type, &type_class);
            if (type_class == coda_special_class)
            {
                /* use base type */
                if (coda_type_get_special_base_type(type, &sub_type) != 0)
                {
                    return -1;
                }
                type = sub_type;
                coda_type_get_class(type, &type_class);
            }
            if (type_class == coda_array_class)
            {
                if (coda_type_get_array_base_type(type, &sub_type) != 0)
                {
                    return -1;
                }
            }
            else
            {
                if (coda_type_get_record_field_type(type, index, &sub_type) != 0)
                {
                    return -1;
                }
            }
        }
        child_node = coda_tree_node_new(sub_type);
        if (child_node == NULL)
        {
            return -1;
        }
        *sub_node = child_node;

        /* add node using bubble sort */
        for (i = 0; i < node->num_indexed_children; i++)
        {
            if (index < node->index[i])
            {
                long tmp_index = node->index[i];
                coda_tree_node *tmp_node = node->indexed_child[i];

                node->index[i] = index;
                node->indexed_child[i] = child_node;
                index = tmp_index;
                child_node = tmp_node;
            }
        }
        node->index[node->num_indexed_children] = index;
        node->indexed_child[node->num_indexed_children] = child_node;
        node->num_indexed_children++;
    }
    else
    {
        *sub_node = NULL;
    }

    return 0;
}

/** Add an item to the tree
 * The item will be added to the tree at the location as indicated by 'path'.
 * \param node Pointer to the root node of the tree
 * \param path String using CODA node expression syntax to define the product location where the item is applicable
 * \param item Pointer to the item that will be attached
 * \param leaf_only Set to 1 if an error should be raised if 'path' ends up pointing to an array or record (0 otherwise)
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
int coda_tree_node_add_item_for_path(coda_tree_node *node, const char *path, void *item, int leaf_only)
{
    coda_type_class type_class;
    int start = 0;

    if (path[start] == '/')
    {
        /* skip leading '/' if it is not followed by a record field name */
        if (path[start + 1] == '\0' || path[start + 1] == '/' || path[start + 1] == '[' || path[start + 1] == '@')
        {
            start++;
        }
    }

    /* find node */
    while (path[start] != '\0')
    {
        if (path[start] == '@')
        {
            /* attribute */
            if (tree_node_get_node_for_index(node, -1, &node, 1) != 0)
            {
                return -1;
            }
            start++;
        }
        else
        {
            long index;

            coda_type_get_class(node->type, &type_class);
            if (type_class == coda_special_class)
            {
                coda_type *type;

                /* use base type */
                if (coda_type_get_special_base_type(node->type, &type) != 0)
                {
                    return -1;
                }
                coda_type_get_class(type, &type_class);
            }

            if (path[start] == '[')
            {
                int end;

                /* array index */
                if (type_class != coda_array_class)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "not an array '%.*s' (type is %s)", start, path,
                                   coda_type_get_class_name(type_class));
                }
                start++;
                end = start;
                while (path[end] != '\0' && path[end] != ']')
                {
                    end++;
                }
                if (path[end] == '\0')
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid path '%s' (missing ']')", path);
                    return -1;
                }
                if (start == end)
                {
                    /* add item to all array elements */
                    if (tree_node_get_node_for_all(node, &node, 1) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    int result;
                    int n;

                    /* add item to specific array element */
                    result = sscanf(&path[start], "%ld%n", &index, &n);
                    if (result != 1 || n != end - start)
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid array index '%.*s' in path", end - start,
                                       &path[start]);
                        return -1;
                    }
                    if (tree_node_get_node_for_index(node, index, &node, 1) != 0)
                    {
                        return -1;
                    }
                }
                start = end + 1;
            }
            else
            {
                long end;

                if (path[start] != '/')
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid path '%s' (missing '/'?)", path);
                    return -1;
                }

                /* record field */
                if (type_class != coda_record_class)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "not a record '%.*s' (type is %s)", start, path,
                                   coda_type_get_class_name(type_class));
                }
                start++;
                end = start;
                while (path[end] != '\0' && path[end] != '/' && path[end] != '[' && path[end] != '@')
                {
                    end++;
                }
                if (coda_type_get_record_field_index_from_name_n(node->type, &path[start], end - start, &index) != 0)
                {
                    return -1;
                }
                if (tree_node_get_node_for_index(node, index, &node, 1) != 0)
                {
                    return -1;
                }
                start = end;
            }
        }
    }

    coda_type_get_class(node->type, &type_class);
    if (leaf_only && (type_class == coda_array_class || type_class == coda_record_class))
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "trying to add item to path '%s', which is not a leaf item", path);
        return -1;
    }

    /* add item to node */
    return tree_node_add_item(node, item);
}


static int get_item_for_cursor(coda_tree_node *node, int depth, coda_cursor *cursor, void **item)
{
    if (node == NULL)
    {
        *item = NULL;
        return 0;
    }

    if (depth < cursor->n - 1)
    {
        /* specific indexed array elements take precedence over an 'all array elements' reference */
        if (node->num_indexed_children > 0)
        {
            long bottom = 0;
            long top = node->num_indexed_children - 1;
            long index = cursor->stack[depth + 1].index;

            while (top != bottom)
            {
                long middle = (bottom + top) / 2;

                if (index <= node->index[middle])
                {
                    top = middle;
                }
                if (index > node->index[middle])
                {
                    bottom = middle + 1;
                }
            }
            if (index == node->index[top])
            {
                if (get_item_for_cursor(node->indexed_child[top], depth + 1, cursor, item) != 0)
                {
                    return -1;
                }
                if (item != NULL)
                {
                    return 0;
                }
            }
        }
        if (node->all_children != NULL)
        {
            if (get_item_for_cursor(node->all_children, depth + 1, cursor, item) != 0)
            {
                return -1;
            }
            if (item != NULL)
            {
                return 0;
            }
        }
    }
    else if (node->num_items > 0)
    {
        /* return the last item in the list */
        *item = node->item[node->num_items - 1];
        return 0;
    }

    /* nothing found */
    *item = NULL;
    return 0;
}

/** Retrieve the item located at the given cursor position
 * If multiple items exist at the current position then the last item in the list will be returned.
 * Items attached to paths with an explicit array index (e.g. /foo[0]/bar) will take precedence over items that are
 * attached to all elements of an array (e.g. /foo[]/bar).
 * \param node Pointer to the root node of the tree
 * \param cursor A valid cursor
 * \param item Pointer to the item that is attached to the cursor position, or NULL if no item was found
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
int coda_tree_node_get_item_for_cursor(coda_tree_node *node, coda_cursor *cursor, void **item)
{
    return get_item_for_cursor(node, 0, cursor, item);
}
