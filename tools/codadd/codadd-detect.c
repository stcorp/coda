/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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
