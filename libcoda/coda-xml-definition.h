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

#ifndef CODA_XML_DEFINITION_H
#define CODA_XML_DEFINITION_H

#include "coda-internal.h"
#include "coda-expr.h"
#include "coda-ascii-definition.h"

typedef struct coda_xmlType_struct coda_xmlType;
typedef struct coda_xmlRoot_struct coda_xmlRoot;
typedef struct coda_xmlField_struct coda_xmlField;
typedef struct coda_xmlElement_struct coda_xmlElement;
typedef struct coda_xmlArray_struct coda_xmlArray;
typedef struct coda_xmlAttribute_struct coda_xmlAttribute;
typedef struct coda_xmlAttributeRecord_struct coda_xmlAttributeRecord;
typedef struct coda_xmlProductTypeVersion_struct coda_xmlProductTypeVersion;
typedef struct coda_xmlProductType_struct coda_xmlProductType;

void coda_xml_release_type(coda_xmlType *type);

int coda_xml_element_add_attribute(coda_xmlElement *element, coda_xmlAttribute *attribute);

coda_xmlRoot *coda_xml_root_new(void);
int coda_xml_root_set_field(coda_xmlRoot *root, coda_xmlField *field);
int coda_xml_root_validate(coda_xmlRoot *root);

coda_xmlElement *coda_xml_record_new(const char *xml_name);
int coda_xml_record_add_field(coda_xmlElement *element, coda_xmlField *field);
void coda_xml_record_convert_to_text(coda_xmlElement *element);

coda_xmlElement *coda_xml_text_new(const char *xml_name);

coda_xmlElement *coda_xml_ascii_type_new(const char *xml_name);
int coda_xml_ascii_type_set_type(coda_xmlElement *element, coda_asciiType *type);
int coda_xml_ascii_type_validate(coda_xmlElement *element);

coda_xmlField *coda_xml_field_new(const char *name);
int coda_xml_field_set_type(coda_xmlField *field, coda_xmlType *type);
int coda_xml_field_set_hidden(coda_xmlField *field);
int coda_xml_field_set_optional(coda_xmlField *field);
int coda_xml_field_validate(coda_xmlField *field);
int coda_xml_field_convert_to_array(coda_xmlField *field);
void coda_xml_field_delete(coda_xmlField *field);

coda_xmlArray *coda_xml_array_new(void);
int coda_xml_array_set_base_type(coda_xmlArray *array, coda_xmlElement *base_type);
int coda_xml_array_validate(coda_xmlArray *array);

coda_xmlAttribute *coda_xml_attribute_new(const char *xml_name);
int coda_xml_attribute_set_fixed_value(coda_xmlAttribute *attribute, const char *fixed_value);
int coda_xml_attribute_set_optional(coda_xmlAttribute *attribute);

#endif
