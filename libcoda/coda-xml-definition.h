/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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
#include "coda-ascii-definition.h"

typedef struct coda_xml_type_struct coda_xml_type;
typedef struct coda_xml_root_struct coda_xml_root;
typedef struct coda_xml_field_struct coda_xml_field;
typedef struct coda_xml_element_struct coda_xml_element;
typedef struct coda_xml_array_struct coda_xml_array;
typedef struct coda_xml_attribute_struct coda_xml_attribute;
typedef struct coda_xml_attribute_record_struct coda_xml_attribute_record;
typedef struct coda_xml_product_type_version_struct coda_xml_product_type_version;
typedef struct coda_xml_product_type_struct coda_xml_product_type;

void coda_xml_release_type(coda_xml_type *type);

int coda_xml_element_add_attribute(coda_xml_element *element, coda_xml_attribute *attribute);

coda_xml_root *coda_xml_root_new(void);
int coda_xml_root_set_field(coda_xml_root *root, coda_xml_field *field);
int coda_xml_root_validate(coda_xml_root *root);

coda_xml_element *coda_xml_record_new(const char *xml_name);
int coda_xml_record_add_field(coda_xml_element *element, coda_xml_field *field);
void coda_xml_record_convert_to_text(coda_xml_element *element);

coda_xml_element *coda_xml_text_new(const char *xml_name);

coda_xml_element *coda_xml_ascii_type_new(const char *xml_name);
int coda_xml_ascii_type_set_type(coda_xml_element *element, coda_ascii_type *type);
int coda_xml_ascii_type_validate(coda_xml_element *element);

coda_xml_field *coda_xml_field_new(const char *name);
int coda_xml_field_set_type(coda_xml_field *field, coda_xml_type *type);
int coda_xml_field_set_hidden(coda_xml_field *field);
int coda_xml_field_set_optional(coda_xml_field *field);
int coda_xml_field_validate(coda_xml_field *field);
int coda_xml_field_convert_to_array(coda_xml_field *field);
void coda_xml_field_delete(coda_xml_field *field);

coda_xml_array *coda_xml_array_new(void);
int coda_xml_array_set_base_type(coda_xml_array *array, coda_xml_element *base_type);
int coda_xml_array_validate(coda_xml_array *array);

coda_xml_attribute *coda_xml_attribute_new(const char *xml_name);
int coda_xml_attribute_set_fixed_value(coda_xml_attribute *attribute, const char *fixed_value);
int coda_xml_attribute_set_optional(coda_xml_attribute *attribute);

#endif
