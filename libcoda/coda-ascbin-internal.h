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

#ifndef CODA_ASCBIN_INTERNAL_H
#define CODA_ASCBIN_INTERNAL_H

#include "coda-ascbin.h"
#include "coda-definition.h"

struct coda_ascbin_detection_node_struct
{
    /* detection rule entry at this node; will be NULL for root node */
    coda_detection_rule_entry *entry;

    coda_detection_rule *rule;  /* the matching rule when 'entry' matches and none of the subnodes match */

    /* sub nodes of this node */
    int num_subnodes;
    struct coda_ascbin_detection_node_struct **subnode;
};
typedef struct coda_ascbin_detection_node_struct coda_ascbin_detection_node;

coda_ascbin_detection_node *coda_ascbin_get_detection_tree(void);

#endif
