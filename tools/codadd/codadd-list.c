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

coda_type *typestack[CODA_CURSOR_MAXDEPTH];
long indexstack[CODA_CURSOR_MAXDEPTH + 1];

extern char *ascii_col_sep;
extern int show_type;
extern int show_unit;
extern int show_format;
extern int show_description;
extern int show_quotes;
extern int show_hidden;
extern int show_expressions;
extern int show_parent_types;
extern int show_attributes;
extern int use_special_types;


static void print_path(int depth)
{
    int i;

    printf("/");
    for (i = 0; i < depth; i++)
    {
        if (indexstack[i + 1] == -1)
        {
            printf("@");
        }
        else
        {
            coda_type *type = typestack[i];

            switch (type->type_class)
            {
                case coda_record_class:
                    {
                        const char *fieldname;

                        coda_type_get_record_field_name(type, indexstack[i + 1], &fieldname);
                        if (i > 0 && indexstack[i] != -1)
                        {
                            printf("/");
                        }
                        printf("%s", fieldname);
                    }
                    break;
                case coda_array_class:
                    {
                        long dim[CODA_MAX_NUM_DIMS];
                        int num_dims;
                        int j;

                        coda_type_get_array_dim(type, &num_dims, dim);
                        printf("[");
                        for (j = 0; j < num_dims; j++)
                        {
                            if (j > 0)
                            {
                                printf(",");
                            }
                            if (dim[j] < 0)
                            {
                                if (show_expressions && ((coda_type_array *)type)->dim_expr[j] != NULL)
                                {
                                    coda_expression_print(((coda_type_array *)type)->dim_expr[j], printf);
                                }
                                else
                                {
                                    printf("?");
                                }
                            }
                            else
                            {
                                printf("%ld", dim[j]);
                            }
                        }
                        printf("]");
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }
}

static void print_type(coda_type *type, int depth)
{
    coda_type_class type_class;
    int print_details = 0;

    if (depth >= CODA_CURSOR_MAXDEPTH)
    {
        printf("\n");
        fprintf(stderr, "ERROR: depth in type hierarchy (%d) exceeds maximum allowed depth (%d)\n", depth,
                CODA_CURSOR_MAXDEPTH);
        exit(1);
    }

    typestack[depth] = type;

    coda_type_get_class(type, &type_class);
    if (type_class == coda_record_class || type_class == coda_array_class)
    {
        print_details = show_parent_types;
    }
    else if (type_class == coda_special_class)
    {
        print_details = use_special_types;
    }
    else
    {
        print_details = 1;
    }

    if (print_details)
    {
        print_path(depth);
        if (show_type)
        {
            coda_native_type read_type;

            coda_type_get_read_type(type, &read_type);
            printf("%s%s", ascii_col_sep, coda_type_get_native_type_name(read_type));
        }
        if (show_format)
        {
            coda_format format;

            coda_type_get_format(type, &format);
            printf("%s%s", ascii_col_sep, coda_type_get_format_name(format));
        }
        if (show_unit)
        {
            const char *unit;

            printf("%s", ascii_col_sep);
            coda_type_get_unit(type, &unit);
            if (unit != NULL)
            {
                if (show_quotes)
                {
                    printf("\"");
                }
                printf("%s", unit);
                if (show_quotes)
                {
                    printf("\"");
                }
            }
        }
        if (show_description)
        {
            const char *description;

            printf("%s", ascii_col_sep);
            coda_type_get_description(type, &description);
            if (description != NULL)
            {
                if (show_quotes)
                {
                    printf("\"");
                }
                printf("%s", description);
                if (show_quotes)
                {
                    printf("\"");
                }
            }
        }
        printf("\n");
    }

    if (show_attributes)
    {
        int has_attributes;

        coda_type_has_attributes(type, &has_attributes);
        if (has_attributes)
        {
            coda_type *attributes;

            coda_type_get_attributes(type, &attributes);
            indexstack[depth + 1] = -1;
            print_type(attributes, depth + 1);
        }
    }

    switch (type_class)
    {
        case coda_record_class:
            {
                long num_record_fields;
                long i;

                coda_type_get_num_record_fields(type, &num_record_fields);
                for (i = 0; i < num_record_fields; i++)
                {
                    coda_type *field_type;

                    coda_type_get_record_field_type(type, i, &field_type);

                    if (!show_hidden)
                    {
                        int hidden;

                        coda_type_get_record_field_hidden_status(type, i, &hidden);
                        if (hidden)
                        {
                            continue;
                        }
                    }
                    indexstack[depth + 1] = i;
                    print_type(field_type, depth + 1);
                }
            }
            break;
        case coda_array_class:
            {
                coda_type *base_type;

                coda_type_get_array_base_type(type, &base_type);
                indexstack[depth + 1] = 0;
                print_type(base_type, depth + 1);
            }
            break;
        case coda_special_class:
            if (!use_special_types)
            {
                coda_type *base_type;

                coda_type_get_special_base_type(type, &base_type);
                print_type(base_type, depth);

                break;
            }
            /* otherwise fall through to default */
        default:
            break;
    }
}

static void generate_field_list(const char *product_class_name, const char *product_type_name, int version)
{
    coda_product_class *product_class;
    coda_product_type *product_type;
    coda_product_definition *product_definition;

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

    if (product_definition->root_type != NULL)
    {
        print_type(product_definition->root_type, 0);
    }
}


static void generate_product_list(const char *product_class_name, const char *product_type_name)
{
    int i;

    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_product_class *product_class = coda_global_data_dictionary->product_class[i];
        int j;

        if (product_class_name != NULL && strcmp(product_class->name, product_class_name) != 0)
        {
            continue;
        }
        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_product_type *product_type = product_class->product_type[j];

            if (product_type_name != NULL && strcmp(product_type->name, product_type_name) != 0)
            {
                continue;
            }
            if (product_type->num_product_definitions > 0)
            {
                int k;

                for (k = 0; k < product_type->num_product_definitions; k++)
                {
                    coda_product_definition *product_definition = product_type->product_definition[k];

                    printf("%s%s%s%s%d", product_class->name, ascii_col_sep, product_type->name, ascii_col_sep,
                           product_definition->version);
                    if (show_format)
                    {
                        printf("%s%s", ascii_col_sep, coda_type_get_format_name(product_definition->format));
                    }
                    if (show_description)
                    {
                        printf("%s", ascii_col_sep);
                        if (product_definition->description != NULL)
                        {
                            if (show_quotes)
                            {
                                printf("\"");
                            }
                            printf("%s", product_definition->description);
                            if (show_quotes)
                            {
                                printf("\"");
                            }
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}

void generate_list(const char *product_class, const char *product_type, int version)
{
    if (version < 0)
    {
        generate_product_list(product_class, product_type);
    }
    else
    {
        generate_field_list(product_class, product_type, version);
    }
}
