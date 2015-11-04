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

#include "coda.h"

typedef struct coda_tree_node_struct coda_tree_node;
struct coda_tree_node_struct
{
    const coda_type *type;
    int num_items;
    void **item;
    coda_tree_node *all_children;       /* node that contains items that are applicable for all indices */
    int num_indexed_children;
    long *index;        /* can be '-1' for attributes */
    coda_tree_node **indexed_child;
};

coda_tree_node *coda_tree_node_new(const coda_type *type);
void coda_tree_node_delete(coda_tree_node *node, void (*free_item) (void *));
int coda_tree_node_add_item_for_path(coda_tree_node *node, const char *path, void *item, int leaf_only);
int coda_tree_node_get_item_for_cursor(coda_tree_node *node, coda_cursor *cursor, void **item);
