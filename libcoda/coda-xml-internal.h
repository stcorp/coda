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

#ifndef CODA_XML_INTERNAL_H
#define CODA_XML_INTERNAL_H

#include "coda-xml.h"
#include "coda-bin-internal.h"
#include "coda-mem-internal.h"
#include "coda-definition.h"

typedef enum xml_type_tag_enum
{
    tag_xml_root,
    tag_xml_element     /* record, text, or any ascii format type */
} xml_type_tag;

typedef struct coda_xml_type_struct
{
    coda_backend backend;
    coda_type *definition;
    xml_type_tag tag;
} coda_xml_type;

typedef struct coda_xml_root_struct
{
    coda_backend backend;
    coda_type_record *definition;
    xml_type_tag tag;

    struct coda_xml_element_struct *element;    /* root xml element */
} coda_xml_root;

typedef struct coda_xml_element_struct
{
    coda_backend backend;
    coda_type *definition;
    xml_type_tag tag;

    char *xml_name;     /* the xml name is a concatenation of namespace and element name, separated by a ' ' */

    int64_t outer_bit_offset;   /* absolute bit offset in file of the start of this element */
    int64_t inner_bit_offset;   /* absolute bit offset in file of the start of the content of this element */
    int64_t outer_bit_size;     /* bit size of total element, including start and end tag */
    int64_t inner_bit_size;     /* bit size of total content between start and end tag */
    int32_t cdata_delta_offset; /* delta on bit offset if the content consists of a single CDATA element */
    int32_t cdata_delta_size;   /* delta on bit size if the content consists of a single CDATA element */

    coda_mem_record *attributes;

    /* data for record */
    long num_elements;
    coda_dynamic_type **element;        /* xml_element, mem_array or mem_special */

    /* pointer to parent element (only used during xml parsing) */
    struct coda_xml_element_struct *parent;
} coda_xml_element;

struct coda_xml_detection_node_struct
{
    /* xml name of this node */
    char *xml_name;

    /* detection rules at this node */
    int num_detection_rules;
    coda_detection_rule **detection_rule;

    /* attribute sub nodes of this node */
    int num_attribute_subnodes;
    struct coda_xml_detection_node_struct **attribute_subnode;
    hashtable *attribute_hash_data;

    /* sub nodes of this node */
    int num_subnodes;
    struct coda_xml_detection_node_struct **subnode;
    hashtable *hash_data;

    struct coda_xml_detection_node_struct *parent;
};
typedef struct coda_xml_detection_node_struct coda_xml_detection_node;

coda_xml_detection_node *coda_xml_get_detection_tree(void);
coda_xml_detection_node *coda_xml_detection_node_get_attribute_subnode(coda_xml_detection_node *node,
                                                                       const char *xml_name);
coda_xml_detection_node *coda_xml_detection_node_get_subnode(coda_xml_detection_node *node, const char *xml_name);

struct coda_xml_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    const uint8_t *mem_ptr;
#if CODA_USE_QIAP
    void *qiap_info;
#endif

    /* 'xml' product specific fields */
    coda_product *raw_product;
};
typedef struct coda_xml_product_struct coda_xml_product;

int coda_xml_parse(coda_xml_product *product);
int coda_xml_parse_for_detection(int fd, const char *filename, coda_product_definition **definition);

coda_xml_root *coda_xml_root_new(coda_type_record *definition);
int coda_xml_root_add_element(coda_xml_root *root, coda_xml_product *product, const char *el, const char **attr,
                              int64_t outer_bit_offset, int64_t inner_bit_offset, int update_definition);
int coda_xml_element_add_element(coda_xml_element *parent, coda_xml_product *product, const char *el, const char **attr,
                                 int64_t outer_bit_offset, int64_t inner_bit_offset, int update_definition,
                                 coda_xml_element **new_element);
int coda_xml_element_convert_to_text(coda_xml_element *element);
int coda_xml_element_validate(coda_xml_element *element);

#endif
