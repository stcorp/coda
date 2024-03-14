/*
 * Copyright (C) 2007-2024 S[&]T, The Netherlands.
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


static int INDENT = 0;

static void indent(void)
{
    int i;

    assert(INDENT >= 0);
    for (i = INDENT; i; i--)
    {
        printf("  ");
    }
}

static void generate_detection_tree_sub(coda_detection_node *node, int num_compares)
{
    int i;

    if (node == NULL)
    {
        return;
    }

    if (node->path != NULL || node->expression != NULL)
    {
        if (node->path != NULL)
        {
            indent();
            printf("%s exists", node->path);
        }
        else
        {
            num_compares++;
            indent();
            coda_expression_print(node->expression, printf);
        }
        if (node->rule)
        {
            coda_product_definition *product_definition = node->rule->product_definition;

            printf(" --> %s %s %d", product_definition->product_type->product_class->name,
                   product_definition->product_type->name, product_definition->version);
            if (node->num_subnodes > 0)
            {
                printf(" {%d+%d tests}", num_compares, node->num_subnodes);
            }
            else
            {
                printf(" {%d tests}", num_compares);
            }
        }
        printf("\n");
    }

    for (i = 0; i < node->num_subnodes; i++)
    {
        INDENT++;
        generate_detection_tree_sub(node->subnode[i], num_compares + i);
        INDENT--;
    }
}

void generate_detection_tree(coda_format format)
{
    generate_detection_tree_sub(coda_data_dictionary_get_detection_tree(format), 0);
}
