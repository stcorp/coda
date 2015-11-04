/*
 * Copyright (C) 2007-2008 S&T, The Netherlands.
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
#include "coda-xml-definition.h"
#include "coda-xml-dynamic.h"
#include "coda-ascbin-internal.h"
#include "coda-ascii-internal.h"
#include "coda-definition.h"

typedef enum xml_type_tag_enum
{
    tag_xml_root,
    tag_xml_record,
    tag_xml_array,
    tag_xml_text,
    tag_xml_ascii_type,
    tag_xml_attribute,
    tag_xml_attribute_record
} xml_type_tag;

typedef enum xml_dynamic_tag_enum
{
    tag_xml_root_dynamic,
    tag_xml_record_dynamic,
    tag_xml_array_dynamic,
    tag_xml_text_dynamic,
    tag_xml_ascii_type_dynamic,
    tag_xml_attribute_dynamic,
    tag_xml_attribute_record_dynamic
} xml_dynamic_tag;

/* coda_Type
 * \ -- coda_xmlType
 *      \ -- coda_xmlRoot
 *       |-- coda_xmlElement (record, text, ascii_type)
 *       |-- coda_xmlArray
 *       |-- coda_xmlAttribute
 *       |-- coda_xmlAttributeRecord
 * coda_DynamicType
 * \ -- coda_xmlDynamicType
 *      \ -- coda_xmlRootDynamicType
 *       |-- coda_xmlElementDynamicType (record, text, ascii_type)
 *       |-- coda_xmlArrayDynamicType
 *       |-- coda_xmlAttributeDynamicType
 *       |-- coda_xmlAttributeRecordDynamicType
 * coda_xmlField
 */

struct coda_xmlType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
};

struct coda_xmlRoot_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
    coda_xmlField *field;
};

struct coda_xmlField_struct
{
    const char *xml_name;       /* this is a reference to the xml_name from a coda_xmlElement */
    char *name; /* the field name */

    coda_xmlType *type; /* xmlArray or xmlElement */

    uint8_t optional;
    uint8_t hidden;
};

struct coda_xmlElement_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
    char *xml_name;     /* the xml name is a concatenation of namespace and element name, separated by a ' ' */

    struct coda_xmlAttributeRecord_struct *attributes;

    /* data for record */
    int num_fields;
    coda_xmlField **field;
    hashtable *xml_name_hash_data;
    hashtable *name_hash_data;

    /* data for ascii type */
    coda_asciiType *ascii_type;
};

struct coda_xmlArray_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
    coda_xmlElement *base_type;
};

struct coda_xmlAttribute_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
    char *xml_name;     /* the xml name is a concatenation of namespace and attribute name, separated by a ' ' */
    char *attr_name;    /* the attribute name converted to a proper CODA identifier */
    char *fixed_value;  /* an optional fixed value for this attribute */
    uint8_t optional;
};

struct coda_xmlAttributeRecord_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    xml_type_tag tag;
    int num_attributes;
    coda_xmlAttribute **attribute;
    hashtable *attribute_name_hash_data;
    hashtable *name_hash_data;
};


struct coda_xmlDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_dynamic_tag tag;
};

struct coda_xmlRootDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_dynamic_tag tag;
    coda_xmlRoot *type; /* reference to definition for this element */

    struct coda_xmlElementDynamicType_struct *element;  /* root xml element */
};

struct coda_xmlElementDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_dynamic_tag tag;
    coda_xmlElement *type;      /* reference to definition for this element */

    int64_t outer_bit_offset;   /* absolute bit offset in file of the start of this element */
    int64_t inner_bit_offset;   /* absolute bit offset in file of the start of the content of this element */
    int64_t outer_bit_size;     /* bit size of total element, including start and end tag */
    int64_t inner_bit_size;     /* bit size of total content between start and end tag */
    int32_t cdata_delta_offset; /* delta on bit offset if the content consists of a single CDATA element */
    int32_t cdata_delta_size;   /* delta on bit size if the content consists of a single CDATA element */

    struct coda_xmlAttributeRecordDynamicType_struct *attributes;

    long num_elements;
    coda_xmlDynamicType **element;      /* xmlArrayDynamicType or xmlElementDynamicType */

    /* pointer to parent element (only used during xml parsing) */
    struct coda_xmlElementDynamicType_struct *parent;
};

struct coda_xmlArrayDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_dynamic_tag tag;
    coda_xmlArray *type;        /* reference to definition for this element */

    long num_elements;
    coda_xmlElementDynamicType **element;
};

struct coda_xmlAttributeDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_type_tag tag;
    coda_xmlAttribute *type;    /* reference to definition for this element */

    char *value;
};

struct coda_xmlAttributeRecordDynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;

    xml_dynamic_tag tag;
    coda_xmlAttributeRecord *type;      /* reference to definition for this element */

    long num_attributes;
    coda_xmlAttributeDynamicType **attribute;
};

struct coda_xmlDetectionNode_struct
{
    /* xml name of this node */
    char *xml_name;

    /* detection rules at this node */
    int num_detection_rules;
    coda_DetectionRule **detection_rule;

    /* sub nodes of this node */
    int num_subnodes;
    struct coda_xmlDetectionNode_struct **subnode;
    hashtable *hash_data;

    struct coda_xmlDetectionNode_struct *parent;
};
typedef struct coda_xmlDetectionNode_struct coda_xmlDetectionNode;

coda_xmlDetectionNode *coda_xml_get_detection_tree(void);
coda_xmlDetectionNode *coda_xml_detection_node_get_subnode(coda_xmlDetectionNode *node, const char *xml_name);

struct coda_xmlProductFile_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_DynamicType *root_type;
    coda_ProductDefinition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;

    int use_mmap;       /* this field is needed for when the ascii backend wants to read data - the value is always 0 */
    int fd;
};
typedef struct coda_xmlProductFile_struct coda_xmlProductFile;

int coda_xml_parse_and_interpret(coda_xmlProductFile *pf);
int coda_xml_parse_with_definition(coda_xmlProductFile *pf);
int coda_xml_parse_for_detection(int fd, const char *filename, coda_ProductDefinition **definition);

#endif
