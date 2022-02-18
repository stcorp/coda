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

#include "coda-internal.h"
#include "coda-definition.h"
#include "coda-expr.h"

#include <stdlib.h>
#include <string.h>

void coda_detection_rule_entry_delete(coda_detection_rule_entry *entry)
{
    if (entry->path != NULL)
    {
        free(entry->path);
    }
    if (entry->expression != NULL)
    {
        coda_expression_delete(entry->expression);
    }
    free(entry);
}

coda_detection_rule_entry *coda_detection_rule_entry_new(const char *path)
{
    coda_detection_rule_entry *entry;

    if (path != NULL)
    {
        coda_expression_type type;
        coda_expression *expr;

        if (coda_expression_from_string(path, &expr) != 0)
        {
            return NULL;
        }
        if (coda_expression_get_type(expr, &type) != 0)
        {
            coda_expression_delete(expr);
            return NULL;
        }
        coda_expression_delete(expr);
        if (type != coda_expression_node)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "not a valid path for detection rule");
            return NULL;
        }
    }

    entry = malloc(sizeof(coda_detection_rule_entry));
    if (entry == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_detection_rule_entry), __FILE__, __LINE__);
        return NULL;
    }
    entry->path = NULL;
    entry->expression = NULL;

    if (path != NULL)
    {
        entry->path = strdup(path);
        if (entry->path == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            free(entry);
            return NULL;
        }
    }

    return entry;
}

int coda_detection_rule_entry_set_expression(coda_detection_rule_entry *entry, coda_expression *expression)
{
    if (entry->expression != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection entry already has an expression");
        return -1;
    }
    entry->expression = expression;

    return 0;
}

void coda_detection_rule_delete(coda_detection_rule *detection_rule)
{
    if (detection_rule->entry != NULL)
    {
        int i;

        for (i = 0; i < detection_rule->num_entries; i++)
        {
            if (detection_rule->entry[i] != NULL)
            {
                coda_detection_rule_entry_delete(detection_rule->entry[i]);
            }
        }
        free(detection_rule->entry);
    }
    free(detection_rule);
}

coda_detection_rule *coda_detection_rule_new(void)
{
    coda_detection_rule *detection_rule;

    detection_rule = malloc(sizeof(coda_detection_rule));
    if (detection_rule == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_detection_rule), __FILE__, __LINE__);
        return NULL;
    }
    detection_rule->num_entries = 0;
    detection_rule->entry = NULL;
    detection_rule->product_definition = NULL;

    return detection_rule;
}

int coda_detection_rule_add_entry(coda_detection_rule *detection_rule, coda_detection_rule_entry *entry)
{
    coda_detection_rule_entry **new_entry;

    if (entry->path == NULL && entry->expression == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection entry should have a path and/or an expression");
        return -1;
    }

    new_entry = realloc(detection_rule->entry, (detection_rule->num_entries + 1) * sizeof(coda_detection_rule_entry *));
    if (new_entry == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (detection_rule->num_entries + 1) * sizeof(coda_detection_rule_entry *), __FILE__, __LINE__);
        return -1;
    }
    detection_rule->entry = new_entry;
    detection_rule->num_entries++;
    detection_rule->entry[detection_rule->num_entries - 1] = entry;

    return 0;
}

static void delete_detection_node(coda_detection_node *node)
{
    int i;

    if (node->path != NULL)
    {
        free(node->path);
    }
    if (node->subnode != NULL)
    {
        for (i = 0; i < node->num_subnodes; i++)
        {
            delete_detection_node(node->subnode[i]);
        }
        free(node->subnode);
    }
    free(node);
}

static coda_detection_node *detection_node_new(void)
{
    coda_detection_node *node;

    node = malloc(sizeof(coda_detection_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_detection_node), __FILE__, __LINE__);
        return NULL;
    }
    node->path = NULL;
    node->expression = NULL;
    node->rule = NULL;
    node->num_subnodes = 0;
    node->subnode = NULL;

    return node;
}

static int detection_node_add_node(coda_detection_node *node, coda_detection_node *new_node)
{
    int i;

    if (node->num_subnodes % BLOCK_SIZE == 0)
    {
        coda_detection_node **new_subnode;

        new_subnode = realloc(node->subnode, (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_detection_node *));
        if (new_subnode == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_detection_node *), __FILE__, __LINE__);
            return -1;
        }
        node->subnode = new_subnode;
    }
    node->subnode[node->num_subnodes] = new_node;
    node->num_subnodes++;

    /* make sure 'path' tests come before 'expression' tests and attribute paths come before other paths */
    i = node->num_subnodes - 1;
    while (i > 0 && node->subnode[i]->path != NULL &&
           (node->subnode[i - 1]->expression != NULL ||
            (node->subnode[i - 1]->path != NULL && node->subnode[i]->path[0] == '@' &&
             node->subnode[i - 1]->path[0] != '@')))
    {
        coda_detection_node *swapnode;

        swapnode = node->subnode[i];
        node->subnode[i] = node->subnode[i - 1];
        node->subnode[i - 1] = swapnode;
        i--;
    }

    return 0;
}

/* returns 0 when equal, and 1 when not equal
 * pos = -1 if one of path1/path2 is NULL, otherwise pos is the position at which the first difference is found
 */
static int pathcmp(const char *path1, const char *path2, int *pos)
{
    *pos = 0;
    while (path1[*pos] != '\0' && path1[*pos] == path2[*pos])
    {
        (*pos)++;
    }
    if (path1[*pos] == path2[*pos])
    {
        return 0;
    }

    return 1;
}

static coda_detection_node *get_node_for_entry(coda_detection_node *node, char *subpath,
                                               coda_detection_rule_entry *entry)
{
    coda_detection_node *new_node;
    int pos;
    int i;

    if (subpath != NULL && subpath[0] == '\0')
    {
        subpath = NULL;
    }

    for (i = 0; i < node->num_subnodes; i++)
    {
        if (subpath != NULL)
        {
            /* match path */
            if (node->subnode[i]->path == NULL)
            {
                continue;
            }
            /* check if there is a common root path */
            if (pathcmp(node->subnode[i]->path, subpath, &pos) == 0)
            {
                if (entry->expression != NULL)
                {
                    return get_node_for_entry(node->subnode[i], NULL, entry);
                }
                return node->subnode[i];
            }

            if (node->subnode[i]->path[pos] == '\0')
            {
                if (subpath[pos] == '/' || subpath[pos] == '@' || subpath[pos] == '[')
                {
                    /* entry belongs in subnode */
                    return get_node_for_entry(node->subnode[i], &subpath[pos + (subpath[pos] == '/')], entry);
                }
            }
            else
            {
                /* reduce pos if there is a common '/' '[' part at the end */
                if (pos > 1 && (subpath[pos - 1] == '/' || subpath[pos - 1] == '['))
                {
                    pos--;
                }
                if (pos > 0 && (subpath[pos] == '/' || subpath[pos] == '@' || subpath[pos] == '[') &&
                    (node->subnode[i]->path[pos] == '/' || node->subnode[i]->path[pos] == '@' ||
                     node->subnode[i]->path[pos] == '['))
                {
                    int nodepath_pos = pos + (node->subnode[i]->path[pos] == '/');
                    int subpath_pos = pos + (subpath[pos] == '/');
                    int j;

                    /* common initial paths -> split up subnode and add entry to new subnode */
                    new_node = detection_node_new();
                    if (new_node == NULL)
                    {
                        return NULL;
                    }
                    new_node->path = malloc(pos + 1);
                    if (new_node->path == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       pos, __FILE__, __LINE__);
                        return NULL;
                    }
                    memcpy(new_node->path, subpath, pos);
                    new_node->path[pos] = '\0';
                    if (detection_node_add_node(new_node, node->subnode[i]) != 0)
                    {
                        delete_detection_node(new_node);
                        return NULL;
                    }

                    /* remove common initial path part from existing sub node */
                    j = nodepath_pos;
                    while (node->subnode[i]->path[j] != '\0')
                    {
                        node->subnode[i]->path[j - nodepath_pos] = node->subnode[i]->path[j];
                        j++;
                    }
                    node->subnode[i]->path[j - nodepath_pos] = '\0';

                    node->subnode[i] = new_node;
                    return get_node_for_entry(node->subnode[i], &subpath[subpath_pos], entry);
                }
            }
        }
        else
        {
            /* match expressions */
            if (node->subnode[i]->path != NULL)
            {
                continue;
            }
            if (coda_expression_is_equal(entry->expression, node->subnode[i]->expression))
            {
                /* same entry -> use this subnode */
                return node->subnode[i];
            }
        }
    }

    /* create new node */
    new_node = detection_node_new();
    if (new_node == NULL)
    {
        return NULL;
    }
    if (subpath != NULL)
    {
        new_node->path = strdup(subpath);
        if (new_node->path == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return NULL;
        }
    }
    else
    {
        new_node->expression = entry->expression;
    }
    if (detection_node_add_node(node, new_node) != 0)
    {
        delete_detection_node(new_node);
        return NULL;
    }

    if (subpath != NULL && entry->expression != NULL)
    {
        return get_node_for_entry(new_node, NULL, entry);
    }

    return new_node;
}

void coda_detection_tree_delete(void *detection_tree)
{
    delete_detection_node((coda_detection_node *)detection_tree);
}

int coda_detection_tree_add_rule(void *detection_tree, coda_detection_rule *detection_rule)
{
    coda_detection_node *node;
    int i;

    if (detection_rule->num_entries == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' should have at least one entry",
                       detection_rule->product_definition->name);
        return -1;
    }

    node = *(coda_detection_node **)detection_tree;
    if (node == NULL)
    {
        node = detection_node_new();
        if (node == NULL)
        {
            return -1;
        }
        *(coda_detection_node **)detection_tree = node;
    }
    for (i = 0; i < detection_rule->num_entries; i++)
    {
        node = get_node_for_entry(node, detection_rule->entry[i]->path, detection_rule->entry[i]);
        if (node == NULL)
        {
            return -1;
        }
    }
    if (node->rule != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' is shadowed by detection rule for '%s'",
                       detection_rule->product_definition->name, node->rule->product_definition->name);
        return -1;
    }
    node->rule = detection_rule;

    return 0;
}

int coda_evaluate_detection_node(coda_detection_node *node, coda_cursor *cursor, coda_product_definition **definition)
{
    coda_cursor subcursor = *cursor;
    int i;

    *definition = NULL;
    if (node == NULL)
    {
        return 0;
    }
    if (node->path != NULL)
    {
        if (coda_cursor_goto(&subcursor, node->path) != 0)
        {
            /* treat failures as 'does not exist' */
            coda_errno = 0;
            return 0;
        }
    }
    else if (node->expression != NULL)
    {
        int result;

        if (coda_expression_eval_bool(node->expression, &subcursor, &result) != 0)
        {
            /* treat failures as 'mismatches' */
            coda_errno = 0;
            return 0;
        }
        if (result == 0)
        {
            return 0;
        }
    }

    for (i = 0; i < node->num_subnodes; i++)
    {
        if (coda_evaluate_detection_node(node->subnode[i], &subcursor, definition) != 0)
        {
            return -1;
        }
        if (*definition != NULL)
        {
            return 0;
        }
    }

    if (node->rule != NULL)
    {
        *definition = node->rule->product_definition;
    }

    return 0;
}
