/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CODA_XML_INTERNAL_H
#define CODA_XML_INTERNAL_H

#include "coda-xml.h"
#include "coda-bin-internal.h"
#include "coda-mem-internal.h"
#include "coda-definition.h"

typedef struct coda_xml_type_struct
{
    coda_backend backend;
    coda_type *definition;
} coda_xml_type;

typedef struct coda_xml_element_struct
{
    coda_backend backend;
    coda_type *definition;

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
    uint8_t *mem_ptr;

    /* 'xml' product specific fields */
    coda_product *raw_product;
};
typedef struct coda_xml_product_struct coda_xml_product;

int coda_xml_parse(coda_xml_product *product);

int coda_xml_element_add_element(coda_xml_element *parent, coda_xml_product *product, const char *el, const char **attr,
                                 int64_t outer_bit_offset, int64_t inner_bit_offset, int update_definition,
                                 coda_xml_element **new_element);
int coda_xml_element_convert_to_text(coda_xml_element *element);
int coda_xml_element_validate(coda_xml_element *element);

#endif
