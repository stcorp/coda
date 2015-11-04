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

#ifndef CODA_XML_DATA_H
#define CODA_XML_DATA_H

#include "coda-internal.h"
#include "coda-xml-definition.h"

typedef struct coda_xml_dynamic_type_struct coda_xml_dynamic_type;
typedef struct coda_xml_root_dynamic_type_struct coda_xml_root_dynamic_type;
typedef struct coda_xml_element_dynamic_type_struct coda_xml_element_dynamic_type;
typedef struct coda_xml_array_dynamic_type_struct coda_xml_array_dynamic_type;
typedef struct coda_xml_attribute_dynamic_type_struct coda_xml_attribute_dynamic_type;
typedef struct coda_xml_attribute_record_dynamic_type_struct coda_xml_attribute_record_dynamic_type;

void coda_xml_release_dynamic_type(coda_xml_dynamic_type *type);

coda_xml_root_dynamic_type *coda_xml_dynamic_root_new(coda_xml_root *type);

coda_xml_element_dynamic_type *coda_xml_dynamic_element_new(coda_xml_element *type, const char **attr);
int coda_xml_dynamic_element_add_element(coda_xml_element_dynamic_type *element,
                                         coda_xml_element_dynamic_type *sub_element);
int coda_xml_dynamic_element_update(coda_xml_element_dynamic_type *element);
int coda_xml_dynamic_element_validate(coda_xml_element_dynamic_type *element);

coda_xml_array_dynamic_type *coda_xml_dynamic_array_new(coda_xml_array *type);
int coda_xml_dynamic_array_add_element(coda_xml_array_dynamic_type *array, coda_xml_element_dynamic_type *element);

coda_xml_attribute_record_dynamic_type *coda_xml_dynamic_attribute_record_new(coda_xml_attribute_record *type,
                                                                              const char **attr);

coda_xml_dynamic_type *coda_xml_empty_dynamic_attribute_record(void);

#endif
