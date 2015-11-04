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

#ifndef CODA_ASCBIN_INTERNAL_H
#define CODA_ASCBIN_INTERNAL_H

#include "coda-ascbin.h"
#include "coda-definition.h"

typedef enum eol_type_enum
{
    eol_unknown,
    eol_lf,
    eol_cr,
    eol_crlf
} eol_type;

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

struct coda_ascbin_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
#if CODA_USE_QIAP
    void *qiap_info;
#endif

    int use_mmap;       /* indicates whether the file was opened using mmap */
    int fd;     /* file handle when not using mmap */
#ifdef WIN32
    HANDLE file;
    HANDLE file_mapping;
#endif
    const uint8_t *mmap_ptr;

    eol_type end_of_line;
    long num_asciilines;
    long *asciiline_end_offset; /* byte offset of the termination of the line (eol or eof) */
    eol_type lastline_ending;
    coda_type *asciilines;
};
typedef struct coda_ascbin_product_struct coda_ascbin_product;

#endif
