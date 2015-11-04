/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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
#include "coda-ascbin-definition.h"

typedef enum eol_type_enum
{
    eol_unknown,
    eol_lf,
    eol_cr,
    eol_crlf
} eol_type;

typedef enum ascbin_type_tag_enum
{
    tag_ascbin_record,  /* coda_record_class */
    tag_ascbin_union,   /* coda_record_class */
    tag_ascbin_array    /* coda_array_class */
} ascbin_type_tag;

struct coda_Conversion_struct
{
    char *unit;
    double numerator;
    double denominator;
};

struct coda_ascbinType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascbin_type_tag tag;
    int64_t bit_size;   /* -1 means it's variable and needs to be calculated */
};

struct coda_ascbinField_struct
{
    char *name;
    coda_ascbinType *type;
    int hidden;
    coda_Expr *available_expr;
    int64_t bit_offset; /* relative bit offset from start of record (if -1, use bit_offset_expr or calculate) */
    coda_Expr *bit_offset_expr; /* dynamic relative bit offset from the start of the record */
};

struct coda_ascbinRecord_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascbin_type_tag tag;
    int64_t bit_size;
    coda_Expr *fast_size_expr;
    hashtable *hash_data;
    long num_fields;
    coda_ascbinField **field;
    int has_hidden_fields;
    int has_available_expr_fields;
};

struct coda_ascbinUnion_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascbin_type_tag tag;
    int64_t bit_size;
    coda_Expr *fast_size_expr;
    hashtable *hash_data;
    long num_fields;
    coda_ascbinField **field;
    int has_hidden_fields;
    int has_available_expr_fields;
    coda_Expr *field_expr;      /* returns index in range [0..num_fields) */
};

struct coda_ascbinArray_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascbin_type_tag tag;
    int64_t bit_size;
    coda_ascbinType *base_type;
    long num_elements;
    int num_dims;
    long *dim;  /* -1 means it's variable and the value needs to be retrieved from dim_expr */
    coda_Expr **dim_expr;
};


struct coda_ascbinDetectionNode_struct
{
    /* detection rule entry at this node; will be NULL for root node */
    coda_DetectionRuleEntry *entry;

    coda_DetectionRule *rule;   /* the matching rule when 'entry' matches and none of the subnodes match */

    /* sub nodes of this node */
    int num_subnodes;
    struct coda_ascbinDetectionNode_struct **subnode;
};
typedef struct coda_ascbinDetectionNode_struct coda_ascbinDetectionNode;

coda_ascbinDetectionNode *coda_ascbin_get_detection_tree(void);

struct coda_ascbinProductFile_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_DynamicType *root_type;
    coda_ProductDefinition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;

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
    coda_DynamicType *asciilines;
};
typedef struct coda_ascbinProductFile_struct coda_ascbinProductFile;

#endif
