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

#ifndef CODA_XML_DATA_H
#define CODA_XML_DATA_H

#include "coda-internal.h"
#include "coda-xml-definition.h"

typedef struct coda_xmlDynamicType_struct coda_xmlDynamicType;
typedef struct coda_xmlRootDynamicType_struct coda_xmlRootDynamicType;
typedef struct coda_xmlElementDynamicType_struct coda_xmlElementDynamicType;
typedef struct coda_xmlArrayDynamicType_struct coda_xmlArrayDynamicType;
typedef struct coda_xmlAttributeDynamicType_struct coda_xmlAttributeDynamicType;
typedef struct coda_xmlAttributeRecordDynamicType_struct coda_xmlAttributeRecordDynamicType;

void coda_xml_release_dynamic_type(coda_xmlDynamicType *type);

coda_xmlRootDynamicType *coda_xml_dynamic_root_new(coda_xmlRoot *type);

coda_xmlElementDynamicType *coda_xml_dynamic_element_new(coda_xmlElement *type, const char **attr);
int coda_xml_dynamic_element_add_element(coda_xmlElementDynamicType *element, coda_xmlElementDynamicType *sub_element);
int coda_xml_dynamic_element_update(coda_xmlElementDynamicType *element);
int coda_xml_dynamic_element_validate(coda_xmlElementDynamicType *element);

coda_xmlArrayDynamicType *coda_xml_dynamic_array_new(coda_xmlArray *type);
int coda_xml_dynamic_array_add_element(coda_xmlArrayDynamicType *array, coda_xmlElementDynamicType *element);

coda_xmlAttributeRecordDynamicType *coda_xml_dynamic_attribute_record_new(coda_xmlAttributeRecord *type,
                                                                          const char **attr);

coda_xmlDynamicType *coda_xml_empty_dynamic_attribute_record(void);

#endif
