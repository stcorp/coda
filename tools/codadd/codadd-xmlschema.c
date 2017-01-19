/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-definition.h"
#include "coda-expr.h"
#include "coda-type.h"

static void print_attributes(FILE *f, coda_type *type)
{
    coda_type *attributes;
    long num_record_fields;
    long i;

    coda_type_get_attributes(type, &attributes);
    coda_type_get_num_record_fields(attributes, &num_record_fields);
    for (i = 0; i < num_record_fields; i++)
    {
        coda_type_class field_type_class;
        coda_type *field_type;
        const char *real_name;
        int available;

        coda_type_get_record_field_real_name(attributes, i, &real_name);
        coda_type_get_record_field_available_status(attributes, i, &available);
        coda_type_get_record_field_type(attributes, i, &field_type);
        coda_type_get_class(field_type, &field_type_class);
        fprintf(f, "<xs:attribute name=\"%s\"", real_name);
        if (available == 1)
        {
            fprintf(f, " use=\"required\"");
        }
        switch (field_type_class)
        {
            case coda_integer_class:
                fprintf(f, " type=\"xs:integer\"");
                break;
            case coda_real_class:
                fprintf(f, " type=\"xs:float\"");
                break;
            case coda_text_class:
                fprintf(f, " type=\"xs:string\"");
                break;
            default:
                assert(0);
                exit(1);
        }
        fprintf(f, "/>");
    }
}

static void print_xml_element(FILE *f, coda_type *type)
{
    coda_type_class type_class;
    long num_record_fields;
    long i;

    coda_type_get_class(type, &type_class);
    assert(type_class == coda_record_class);

    fprintf(f, "<xs:complexType><xs:sequence>");
    coda_type_get_num_record_fields(type, &num_record_fields);
    for (i = 0; i < num_record_fields; i++)
    {
        coda_type_class field_type_class;
        coda_type *field_type;
        const char *real_name;
        coda_format format;

        coda_type_get_record_field_real_name(type, i, &real_name);
        coda_type_get_record_field_type(type, i, &field_type);
        coda_type_get_class(field_type, &field_type_class);
        coda_type_get_format(field_type, &format);
        fprintf(f, "<xs:element name=\"%s\"", real_name);
        if (field_type_class == coda_array_class && format == coda_format_xml)
        {
            fprintf(f, " minOccurs=\"0\" maxOccurs=\"unbounded\"");
            coda_type_get_array_base_type(field_type, &field_type);
            coda_type_get_class(field_type, &field_type_class);
            coda_type_get_format(field_type, &format);
        }
        if (field_type_class == coda_special_class)
        {
            coda_type_get_special_base_type(field_type, &field_type);
            coda_type_get_class(field_type, &field_type_class);
            coda_type_get_format(field_type, &format);
        }
        if (field_type_class == coda_record_class)
        {
            fprintf(f, ">");
            print_xml_element(f, field_type);
        }
        else
        {
            const char *xsdtype;
            int has_attributes;

            switch (field_type_class)
            {
                case coda_array_class:
                    assert(format != coda_format_xml);
                    xsdtype = "string";
                    break;
                case coda_integer_class:
                    xsdtype = "integer";
                    break;
                case coda_real_class:
                    xsdtype = "float";
                    break;
                case coda_text_class:
                    xsdtype = "string";
                    break;
                default:
                    assert(0);
                    exit(1);
            }
            coda_type_has_attributes(field_type, &has_attributes);
            if (has_attributes)
            {
                fprintf(f, ">");
                fprintf(f, "<xs:complexType>");
                fprintf(f, "<xs:simpleContent>");
                fprintf(f, "<xs:extension base=\"xs:%s\">", xsdtype);
                print_attributes(f, field_type);
                fprintf(f, "</xs:extension>");
                fprintf(f, "</xs:simpleContent>");
                fprintf(f, "</xs:complexType>");
            }
            else
            {
                fprintf(f, " type=\"xs:%s\">", xsdtype);
            }
        }
        fprintf(f, "</xs:element>");
    }
    fprintf(f, "</xs:sequence>");
    print_attributes(f, type);
    fprintf(f, "</xs:complexType>");
}

void generate_xmlschema(const char *output_file_name, const char *product_class_name, const char *product_type_name,
                        int version)
{
    FILE *schema_output = stdout;
    coda_product_class *product_class;
    coda_product_type *product_type;
    coda_product_definition *product_definition;
    coda_type_record *root_type;

    product_class = coda_data_dictionary_get_product_class(product_class_name);
    if (product_class == NULL)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    product_type = coda_product_class_get_product_type(product_class, product_type_name);
    if (product_type == NULL)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    product_definition = coda_product_type_get_product_definition_by_version(product_type, version);
    if (product_definition == NULL)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (product_definition->format != coda_format_xml)
    {
        fprintf(stderr, "ERROR: product is not in XML format\n");
        exit(1);
    }
    if (product_definition->root_type == NULL)
    {
        fprintf(stderr, "ERROR: product does not have a format definition\n");
        exit(1);
    }

    if (output_file_name != NULL)
    {
        schema_output = fopen(output_file_name, "w");
        if (schema_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    assert(product_definition->root_type->type_class == coda_record_class);
    root_type = (coda_type_record *)product_definition->root_type;
    assert(root_type->num_fields == 1);

    fprintf(schema_output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(schema_output, "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n");
    fprintf(schema_output, "<xs:element name=\"%s\">", root_type->field[0]->real_name);
    print_xml_element(schema_output, root_type->field[0]->type);
    fprintf(schema_output, "</xs:element>\n");
    fprintf(schema_output, "</xs:schema>\n");

    if (output_file_name != NULL)
    {
        fclose(schema_output);
    }
}
