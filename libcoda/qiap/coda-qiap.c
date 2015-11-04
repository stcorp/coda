/*
 * Copyright (C) 2009-2012 S[&]T, The Netherlands.
 *
 * This file is part of the QIAP Toolkit.
 *
 * The QIAP Toolkit is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The QIAP Toolkit is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the QIAP Toolkit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "coda-internal.h"
#include "qiap.h"

static qiap_quality_issue_report *quality_issue_report = NULL;
static char *coda_qiap_report = NULL;
static char *coda_qiap_log = NULL;
static int init_counter = 0;

static int enable_qiap = 1;


typedef struct tree_node_struct tree_node;
struct tree_node_struct
{
    int num_items;
    void **item;
    tree_node *all_children;    /* node that contains items that are applicable for all indices */
    int num_indexed_children;
    long *index;
    tree_node **indexed_child;
};

typedef struct coda_qiap_action_struct coda_qiap_action;
struct coda_qiap_action_struct
{
    long issue_id;
    long affected_product_id;
    long affected_value_id;
    coda_expression *extent;
    qiap_action *action;
};

/** \defgroup coda_qiap QIAP-specific CODA Interface
 * The QIAP-specific CODA interface contains a set of QIAP functions that are provided as part of the CODA library.
 * The allow setting QIAP-specific setting in CODA and provide general QIAP-specific support functions.
 *
 * Note that QIAP support is enabled in the CODA library by default.
 * This can be disabled (or re-enabled) using the coda_set_option_enable_qiap() function.
 * For CODA to perform any QIAP actions, it will need to be provided a reference to a QIAP Quality Issue Report
 * (stored as an xml file). This reference can be provided by setting a CODA_QIAP_REPORT enviroment variable that
 * points to the location of the file (in the form of a full local file path) or by using the coda_qiap_set_report()
 * function.
 *
 * To have CODA write log entries for each action performed (both discard and corrective actions), set either the
 * CODA_QIAP_LOG environment to point to a log file that CODA will append to, or use the coda_qiap_set_action_log()
 * function.
 */

/** \addtogroup coda_qiap
 * @{
 */

static coda_qiap_action *coda_qiap_action_new(long issue_id, long affected_product_id, long affected_value_id,
                                              coda_expression *extent, qiap_action *action)
{
    coda_qiap_action *item;

    item = malloc(sizeof(coda_qiap_action));
    if (item == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_qiap_action), __FILE__, __LINE__);
        return NULL;
    }
    item->issue_id = issue_id;
    item->affected_product_id = affected_product_id;
    item->affected_value_id = affected_value_id;
    item->extent = extent;
    item->action = action;

    return item;
}

static void coda_qiap_action_delete(void *action)
{
    free(action);
}

static int coda_cursor_get_qiap_info(const coda_cursor *cursor, tree_node **qiap_info)
{
    coda_product *product;

    if (coda_cursor_get_product_file(cursor, &product) != 0)
    {
        return -1;
    }
    *qiap_info = product->qiap_info;
    return 0;
}

static int log_action(const coda_cursor *cursor, coda_qiap_action *action)
{
    coda_product *product;
    const char *filename;
    FILE *f;

    if (coda_qiap_log == NULL)
    {
        return 0;
    }
    if (coda_cursor_get_product_file(cursor, &product) != 0)
    {
        return -1;
    }
    if (coda_get_product_filename(product, &filename) != 0)
    {
        return -1;
    }
    f = fopen(coda_qiap_log, "a");
    if (f == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open QIAP action log file '%s' (%s)", coda_qiap_log,
                       strerror(errno));
        return -1;
    }
    fprintf(f, "%s: issue=%ld, product=%ld, value=%ld, type=\"%s\", last-modified=%s\n", filename, action->issue_id,
            action->affected_product_id, action->affected_value_id,
            qiap_get_action_type_name(action->action->action_type), action->action->last_modification_date);
    fclose(f);

    return 0;
}

static tree_node *tree_node_new(void)
{
    tree_node *node;

    node = malloc(sizeof(tree_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(tree_node), __FILE__, __LINE__);
        return NULL;
    }
    node->num_items = 0;
    node->item = NULL;
    node->all_children = NULL;
    node->num_indexed_children = 0;
    node->index = NULL;
    node->indexed_child = NULL;

    return node;
}

static void tree_node_delete(tree_node *node, void (*free_item) (void *))
{
    int i;

    if (node->all_children != NULL)
    {
        tree_node_delete(node->all_children, free_item);
    }
    if (node->index != NULL)
    {
        free(node->index);
    }
    if (node->indexed_child != NULL)
    {
        for (i = 0; i < node->num_indexed_children; i++)
        {
            if (node->indexed_child[i] != NULL)
            {
                tree_node_delete(node->indexed_child[i], free_item);
            }
        }
        free(node->indexed_child);
    }
    if (node->item != NULL)
    {
        if (free_item != NULL)
        {
            for (i = 0; i < node->num_items; i++)
            {
                if (node->item[i] != NULL)
                {
                    free_item(node->item[i]);
                }
            }
        }
        free(node->item);
    }
    free(node);
}

static int tree_node_add_item(tree_node *node, void *item)
{
    void **new_item;

    new_item = realloc(node->item, (node->num_items + 1) * sizeof(void *));
    if (new_item == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_items + 1) * sizeof(void *), __FILE__, __LINE__);
        return -1;
    }
    node->item = new_item;

    node->item[node->num_items] = item;
    node->num_items++;

    return 0;
}

static int tree_node_get_node_for_all(tree_node *node, tree_node **sub_node, int create)
{
    if (node->all_children == NULL && create)
    {
        node->all_children = tree_node_new();
        if (node->all_children == NULL)
        {
            return -1;
        }
    }
    *sub_node = node->all_children;

    return 0;
}

static int tree_node_get_node_for_index(tree_node *node, long index, tree_node **sub_node, int create)
{
    int bottom = 0;
    int top = node->num_indexed_children - 1;

    if (node->num_indexed_children > 0)
    {
        while (top != bottom)
        {
            int middle = (bottom + top) / 2;

            if (index <= node->index[middle])
            {
                top = middle;
            }
            if (index > node->index[middle])
            {
                bottom = middle + 1;
            }
        }
    }

    if (node->num_indexed_children > 0 && index == node->index[top])
    {
        *sub_node = node->indexed_child[top];
    }
    else if (create)
    {
        tree_node **new_indexed_child;
        tree_node *child_node;
        long *new_index;
        int i;

        new_index = realloc(node->index, (node->num_indexed_children + 1) * sizeof(long));
        if (new_index == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(node->num_indexed_children + 1) * sizeof(long), __FILE__, __LINE__);
            return -1;
        }
        node->index = new_index;
        new_indexed_child = realloc(node->indexed_child, (node->num_indexed_children + 1) * sizeof(tree_node *));
        if (new_indexed_child == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(node->num_indexed_children + 1) * sizeof(tree_node *), __FILE__, __LINE__);
            return -1;
        }
        node->indexed_child = new_indexed_child;

        child_node = tree_node_new();
        if (child_node == NULL)
        {
            return -1;
        }
        *sub_node = child_node;

        /* add node using bubble sort */
        for (i = 0; i < node->num_indexed_children; i++)
        {
            if (index < node->index[i])
            {
                long tmp_index = node->index[i];
                tree_node *tmp_node = node->indexed_child[i];

                node->index[i] = index;
                node->indexed_child[i] = child_node;
                index = tmp_index;
                child_node = tmp_node;
            }
        }
        node->index[node->num_indexed_children] = index;
        node->indexed_child[node->num_indexed_children] = child_node;
        node->num_indexed_children++;
    }
    else
    {
        *sub_node = NULL;
    }

    return 0;
}

static int tree_node_add_item_for_path(tree_node *node, const coda_type *type, const char *path, void *item,
                                       int leaf_only)
{
    coda_type_class type_class;
    int start = 0;

    if (path[start] == '/')
    {
        /* skip leading '/' if it is not followed by a record field name */
        if (path[start + 1] == '\0' || path[start + 1] == '/' || path[start + 1] == '[' || path[start + 1] == '@')
        {
            start++;
        }
    }

    /* find node */
    while (path[start] != '\0')
    {
        coda_type *child_type = NULL;

        if (path[start] == '@')
        {
            /* attribute */
            if (coda_type_get_attributes(type, &child_type) != 0)
            {
                return -1;
            }
            if (tree_node_get_node_for_index(node, -1, &node, 1) != 0)
            {
                return -1;
            }
            start++;
        }
        else
        {
            long index;

            coda_type_get_class(type, &type_class);
            if (type_class == coda_special_class)
            {
                /* use base type */
                if (coda_type_get_special_base_type(type, &child_type) != 0)
                {
                    return -1;
                }
                coda_type_get_class(type, &type_class);
            }

            if (path[start] == '[')
            {
                int end;

                /* array index */
                if (type_class != coda_array_class)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "not an array '%.*s' (type is %s)", start, path,
                                   coda_type_get_class_name(type_class));
                }
                start++;
                end = start;
                while (path[end] != '\0' && path[end] != ']')
                {
                    end++;
                }
                if (path[end] == '\0')
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid path '%s' (missing ']')", path);
                    return -1;
                }
                if (start == end)
                {
                    /* add item to all array elements */
                    if (tree_node_get_node_for_all(node, &node, 1) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    /* add item to specific array element */
                    if (sscanf(&path[start], "%ld", &index) != 1)
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid array index '%.*s' in path", end - start,
                                       &path[start]);
                        return -1;
                    }
                    if (tree_node_get_node_for_index(node, index, &node, 1) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_type_get_array_base_type(type, &child_type) != 0)
                {
                    return -1;
                }
                start = end + 1;
            }
            else
            {
                long end;

                if (path[start] != '/')
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid path '%s' (missing '/'?)", path);
                    return -1;
                }

                /* record field */
                if (type_class != coda_record_class)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "not a record '%.*s' (type is %s)", start, path,
                                   coda_type_get_class_name(type_class));
                }
                start++;
                end = start;
                while (path[end] != '\0' && path[end] != '/' && path[end] != '[' && path[end] != '@')
                {
                    end++;
                }
                if (coda_type_get_record_field_index_from_name_n(type, &path[start], end - start, &index) != 0)
                {
                    return -1;
                }
                if (coda_type_get_record_field_type(type, index, &child_type) != 0)
                {
                    return -1;
                }
                if (tree_node_get_node_for_index(node, index, &node, 1) != 0)
                {
                    return -1;
                }
                start = end;
            }
        }
        type = child_type;
    }

    coda_type_get_class(type, &type_class);
    if (leaf_only && (type_class == coda_array_class || type_class == coda_record_class))
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "trying to add item to path '%s', which is not a leaf item", path);
        return -1;
    }

    /* add item to node */
    return tree_node_add_item(node, item);
}

static int tree_node_get_item_for_cursor(tree_node *node, int depth, const coda_cursor *cursor,
                                         coda_qiap_action **action)
{
    if (node->num_items > 0)
    {
        int i;

        for (i = 0; i < node->num_items; i++)
        {
            coda_qiap_action *item = (coda_qiap_action *)node->item[i];
            int affected = 1;

            if (item->extent != NULL)
            {
                coda_cursor local_cursor = *cursor;

                local_cursor.n = depth + 1;
                enable_qiap = 0;
                if (coda_expression_eval_bool(item->extent, &local_cursor, &affected) != 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate extent expression for QIAP issue=%ld, "
                                   "product=%ld, value=%ld (%s)", item->issue_id, item->affected_product_id,
                                   item->affected_value_id, coda_errno_to_string(coda_errno));
                    enable_qiap = 1;
                    return -1;
                }
                enable_qiap = 1;
            }

            if (affected)
            {
                if (item->action->action_type == qiap_action_discard_value)
                {
                    qiap_set_error(QIAP_ERROR_DISCARD, "item should be discarded");
                    coda_set_error(CODA_ERROR_QIAP, NULL);
                    log_action(cursor, item);
                    return -1;
                }
                else
                {
                    assert(item->action->action_type == qiap_action_correct_value);
                    assert(depth == cursor->n - 1);
                    /* return action if it has a higher precedence than any previously found action */
                    if (*action == NULL || (item->action->order > (*action)->action->order))
                    {
                        *action = item;
                    }
                }
            }
        }
    }

    if (depth < cursor->n - 1)
    {
        if (node->all_children != NULL)
        {
            if (tree_node_get_item_for_cursor(node->all_children, depth + 1, cursor, action) != 0)
            {
                return -1;
            }
        }
        if (node->num_indexed_children > 0)
        {
            long bottom = 0;
            long top = node->num_indexed_children - 1;
            long index = cursor->stack[depth + 1].index;

            while (top != bottom)
            {
                long middle = (bottom + top) / 2;

                if (index <= node->index[middle])
                {
                    top = middle;
                }
                if (index > node->index[middle])
                {
                    bottom = middle + 1;
                }
            }
            if (index == node->index[top])
            {
                if (index == node->index[top])
                {
                    if (tree_node_get_item_for_cursor(node->indexed_child[top], depth + 1, cursor, action) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

static int tree_node_has_items_for_array_cursor(tree_node *node, int depth, const coda_cursor *cursor)
{
    if (node->num_items > 0)
    {
        int i;

        for (i = 0; i < node->num_items; i++)
        {
            coda_qiap_action *item = (coda_qiap_action *)node->item[i];
            int affected = 1;

            if (item->extent != NULL)
            {
                coda_cursor local_cursor = *cursor;

                local_cursor.n = depth + 1;
                enable_qiap = 0;
                if (coda_expression_eval_bool(item->extent, &local_cursor, &affected) != 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate extent expression for QIAP issue=%ld, "
                                   "product=%ld, value=%ld (%s)", item->issue_id, item->affected_product_id,
                                   item->affected_value_id, coda_errno_to_string(coda_errno));
                    enable_qiap = 1;
                    return -1;
                }
                enable_qiap = 1;
            }

            if (affected)
            {
                assert(item->action->action_type == qiap_action_discard_value);
                qiap_set_error(QIAP_ERROR_DISCARD, "item should be discarded");
                coda_set_error(CODA_ERROR_QIAP, NULL);
                log_action(cursor, item);
                return -1;
            }
        }
    }

    if (depth < cursor->n - 1)
    {
        if (node->all_children != NULL)
        {
            if (tree_node_has_items_for_array_cursor(node->all_children, depth + 1, cursor) != 0)
            {
                return -1;
            }
        }
        if (node->num_indexed_children > 0)
        {
            long bottom = 0;
            long top = node->num_indexed_children - 1;
            long index = cursor->stack[depth + 1].index;

            while (top != bottom)
            {
                long middle = (bottom + top) / 2;

                if (index <= node->index[middle])
                {
                    top = middle;
                }
                if (index > node->index[middle])
                {
                    bottom = middle + 1;
                }
            }
            if (index == node->index[top])
            {
                if (index == node->index[top])
                {
                    if (tree_node_has_items_for_array_cursor(node->indexed_child[top], depth + 1, cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        int i;

        if (node->all_children != NULL && node->all_children->num_items > 0)
        {
            return 1;
        }
        for (i = 0; i < node->num_indexed_children; i++)
        {
            if (node->index[i] >= 0 && node->indexed_child[i]->num_items > 0)
            {
                return 1;
            }
        }
    }

    return 0;
}

void coda_qiap_add_error_message(void)
{
    coda_add_error_message("[QIAP] %s", qiap_errno_to_string(qiap_errno));
}

static int get_product_name(const coda_product *product, const char **product_name)
{
    const char *filename;

    if (coda_get_product_filename(product, &filename) != 0)
    {
        return -1;
    }
    *product_name = strrchr(filename, '/');
    if (*product_name != NULL)
    {
        *product_name = &(*product_name)[1];
    }
    else
    {
        *product_name = filename;
    }

    return 0;
}

/** Determine if a product file is affected by the given quality issue and return the associated qiap_affected_product
 *  data structure.
 * If the product was affected then \a affected_product will be set to a valid handle, otherwise the variable will be
 * set to NULL. The return code of the function will be 0 indepedent of whether the product was affected by the issue.
 * \param product Pointer to a product file handle. 
 * \param quality_issue Pointer to a quality issue handle.
 * \param affected_product Pointer to the variable where a reference to the affected_product structure will be stored
 *        (if the product is not affected by the issue the variable will be set to NULL).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
LIBCODA_API int coda_qiap_find_affected_product(coda_product *product, const qiap_quality_issue *quality_issue,
                                                qiap_affected_product **affected_product)
{
    const char *product_class = NULL;
    const char *product_type = NULL;
    int i;

    assert(product != NULL);
    assert(affected_product != NULL);

    if (coda_get_product_class(product, &product_class) != 0)
    {
        return -1;
    }
    if (coda_get_product_type(product, &product_type) != 0)
    {
        return -1;
    }
    if (product_class == NULL || product_type == NULL)
    {
        *affected_product = NULL;
        return 0;
    }

    if (strncmp(quality_issue->mission, product_class, strlen(quality_issue->mission)) != 0)
    {
        /* mission not affected */
        *affected_product = NULL;
        return 0;
    }

    for (i = 0; i < quality_issue->num_affected_products; i++)
    {
        if (strcmp(quality_issue->affected_product[i]->product_type, product_type) == 0)
        {
            if (quality_issue->affected_product[i]->extent != NULL)
            {
                coda_cursor cursor;
                int affected;

                if (coda_cursor_set_product(&cursor, product) != 0)
                {
                    return -1;
                }
                enable_qiap = 0;
                if (coda_expression_eval_bool(quality_issue->affected_product[i]->extent, &cursor, &affected) != 0)
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "could not evaluate extent expression for QIAP issue=%ld, "
                                   "product=%ld (%s)", quality_issue->issue_id,
                                   quality_issue->affected_product[i]->affected_product_id,
                                   coda_errno_to_string(coda_errno));
                    enable_qiap = 1;
                    return -1;
                }
                enable_qiap = 0;
                if (affected)
                {
                    *affected_product = quality_issue->affected_product[i];
                    return 0;
                }
            }
            else if (quality_issue->affected_product[i]->num_products > 0)
            {
                const char *product_name;
                int j;

                if (get_product_name(product, &product_name) != 0)
                {
                    return -1;
                }
                for (j = 0; j < quality_issue->affected_product[i]->num_products; j++)
                {
                    if (strcmp(quality_issue->affected_product[i]->product[j], product_name) == 0)
                    {
                        /* product is affected */
                        *affected_product = quality_issue->affected_product[i];
                        return 0;
                    }
                }
            }
            else
            {
                /* product is affected */
                *affected_product = quality_issue->affected_product[i];
                return 0;
            }
        }
    }

    /* product is not affected */
    *affected_product = NULL;
    return 0;
}

static int perform_actions_for_integer(const coda_cursor *cursor, coda_native_type native_type, void *dst)
{
    coda_expression_type expression_type;
    coda_qiap_action *action = NULL;
    tree_node *qiap_info;
    int64_t value;

    if (coda_cursor_get_qiap_info(cursor, &qiap_info) != 0)
    {
        return -1;
    }
    if (!enable_qiap || qiap_info == NULL)
    {
        return 0;
    }

    if (tree_node_get_item_for_cursor(qiap_info, 0, cursor, &action) != 0)
    {
        return -1;
    }
    if (action == NULL)
    {
        return 0;
    }

    if (coda_expression_get_type(action->action->correction, &expression_type) != 0)
    {
        return -1;
    }
    if (expression_type != coda_expression_integer)
    {
        coda_set_error(CODA_ERROR_QIAP, "[QIAP] trying to apply corrective action of type %s to data of type integer",
                       coda_expression_get_type_name(expression_type));
        return -1;
    }
    enable_qiap = 0;
    if (coda_expression_eval_integer(action->action->correction, cursor, &value) != 0)
    {
        qiap_set_error(QIAP_ERROR_CODA, NULL);
        coda_set_error(CODA_ERROR_QIAP, NULL);
        enable_qiap = 1;
        return -1;
    }
    enable_qiap = 1;
    switch (native_type)
    {
        case coda_native_type_int8:
            *((int8_t *)dst) = (int8_t)value;
            break;
        case coda_native_type_uint8:
            *((uint8_t *)dst) = (uint8_t)value;
            break;
        case coda_native_type_int16:
            *((int16_t *)dst) = (int16_t)value;
            break;
        case coda_native_type_uint16:
            *((uint16_t *)dst) = (uint16_t)value;
            break;
        case coda_native_type_int32:
            *((int32_t *)dst) = (int32_t)value;
            break;
        case coda_native_type_uint32:
            *((uint32_t *)dst) = (uint32_t)value;
            break;
        case coda_native_type_int64:
            *((int64_t *)dst) = value;
            break;
        case coda_native_type_uint64:
            *((uint64_t *)dst) = *((uint64_t *)&value);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (log_action(cursor, action) != 0)
    {
        return -1;
    }
    return 1;
}

static int perform_actions_for_float(const coda_cursor *cursor, coda_native_type native_type, void *dst)
{
    coda_expression_type expression_type;
    coda_qiap_action *action = NULL;
    tree_node *qiap_info;
    double value;

    if (coda_cursor_get_qiap_info(cursor, &qiap_info) != 0)
    {
        return -1;
    }
    if (!enable_qiap || qiap_info == NULL)
    {
        return 0;
    }

    if (tree_node_get_item_for_cursor(qiap_info, 0, cursor, &action) != 0)
    {
        return -1;
    }
    if (action == NULL)
    {
        return 0;
    }

    if (coda_expression_get_type(action->action->correction, &expression_type) != 0)
    {
        return -1;
    }
    if (expression_type != coda_expression_float)
    {
        coda_set_error(CODA_ERROR_QIAP, "[QIAP] trying to apply corrective action of type %s to data of type float",
                       coda_expression_get_type_name(expression_type));
        return -1;
    }
    enable_qiap = 0;
    if (coda_expression_eval_float(action->action->correction, cursor, &value) != 0)
    {
        qiap_set_error(QIAP_ERROR_CODA, NULL);
        coda_set_error(CODA_ERROR_QIAP, NULL);
        enable_qiap = 1;
        return -1;
    }
    enable_qiap = 1;
    switch (native_type)
    {
        case coda_native_type_float:
            *((float *)dst) = (float)value;
            break;
        case coda_native_type_double:
            *((double *)dst) = value;
            break;
        default:
            assert(0);
            exit(1);
    }
    if (log_action(cursor, action) != 0)
    {
        return -1;
    }
    return 1;
}

static int perform_actions_for_string(const coda_cursor *cursor, coda_native_type native_type, char *dst,
                                      long dst_length)
{
    coda_expression_type expression_type;
    coda_qiap_action *action = NULL;
    tree_node *qiap_info;
    long value_length;
    char *value = NULL;

    if (coda_cursor_get_qiap_info(cursor, &qiap_info) != 0)
    {
        return -1;
    }
    if (!enable_qiap || qiap_info == NULL)
    {
        return 0;
    }

    if (tree_node_get_item_for_cursor(qiap_info, 0, cursor, &action) != 0)
    {
        return -1;
    }
    if (action == NULL)
    {
        return 0;
    }

    if (coda_expression_get_type(action->action->correction, &expression_type) != 0)
    {
        return -1;
    }
    if (expression_type != coda_expression_string)
    {
        coda_set_error(CODA_ERROR_QIAP, "[QIAP] trying to apply corrective action of type %s to data of type string",
                       coda_expression_get_type_name(expression_type));
        return -1;
    }
    enable_qiap = 0;
    if (coda_expression_eval_string(action->action->correction, cursor, &value, &value_length) != 0)
    {
        qiap_set_error(QIAP_ERROR_CODA, NULL);
        coda_set_error(CODA_ERROR_QIAP, NULL);
        enable_qiap = 1;
        return -1;
    }
    enable_qiap = 1;
    switch (native_type)
    {
        case coda_native_type_char:
            *dst = (value_length > 0 ? *value : '\0');
            break;
        case coda_native_type_string:
            if (value_length == 0)
            {
                /* dst_length is guaranteed to be > 0 */
                *dst = '\0';
            }
            else
            {
                long target_length = dst_length - 1;

                if (target_length > value_length)
                {
                    target_length = value_length;
                }
                memcpy(dst, value, target_length);
                dst[target_length] = '\0';
            }
            break;
        default:
            assert(0);
            exit(1);
    }
    if (log_action(cursor, action) != 0)
    {
        return -1;
    }
    return 1;
}

int coda_qiap_perform_actions_for_int8(const coda_cursor *cursor, int8_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_int8, dst);
}

int coda_qiap_perform_actions_for_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_uint8, dst);
}

int coda_qiap_perform_actions_for_int16(const coda_cursor *cursor, int16_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_int16, dst);
}

int coda_qiap_perform_actions_for_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_uint16, dst);
}

int coda_qiap_perform_actions_for_int32(const coda_cursor *cursor, int32_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_int32, dst);
}

int coda_qiap_perform_actions_for_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_uint32, dst);
}

int coda_qiap_perform_actions_for_int64(const coda_cursor *cursor, int64_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_int64, dst);
}

int coda_qiap_perform_actions_for_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    return perform_actions_for_integer(cursor, coda_native_type_uint64, dst);
}

int coda_qiap_perform_actions_for_float(const coda_cursor *cursor, float *dst)
{
    return perform_actions_for_float(cursor, coda_native_type_float, dst);
}

int coda_qiap_perform_actions_for_double(const coda_cursor *cursor, double *dst)
{
    return perform_actions_for_float(cursor, coda_native_type_double, dst);
}

int coda_qiap_perform_actions_for_char(const coda_cursor *cursor, char *dst)
{
    return perform_actions_for_string(cursor, coda_native_type_char, dst, -1);
}

int coda_qiap_perform_actions_for_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    return perform_actions_for_string(cursor, coda_native_type_string, dst, dst_size);
}

static int perform_actions_for_array(const coda_cursor *cursor, coda_native_type native_type, void *dst)
{
    tree_node *qiap_info;
    int result;

    if (coda_cursor_get_qiap_info(cursor, &qiap_info) != 0)
    {
        return -1;
    }
    if (!enable_qiap || qiap_info == NULL)
    {
        return 0;
    }
    result = tree_node_has_items_for_array_cursor(qiap_info, 0, cursor);
    if (result < 0)
    {
        return -1;
    }
    if (result == 1)
    {
        long num_elements;

        if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }
        if (num_elements > 0)
        {
            coda_cursor local_cursor = *cursor;
            long i;

            if (coda_cursor_goto_first_array_element(&local_cursor) != 0)
            {
                return -1;
            }
            for (i = 0; i < num_elements; i++)
            {
                switch (native_type)
                {
                    case coda_native_type_int8:
                        result = coda_cursor_read_int8(&local_cursor, &(((int8_t *)dst)[i]));
                        break;
                    case coda_native_type_uint8:
                        result = coda_cursor_read_uint8(&local_cursor, &(((uint8_t *)dst)[i]));
                        break;
                    case coda_native_type_int16:
                        result = coda_cursor_read_int16(&local_cursor, &(((int16_t *)dst)[i]));
                        break;
                    case coda_native_type_uint16:
                        result = coda_cursor_read_uint16(&local_cursor, &(((uint16_t *)dst)[i]));
                        break;
                    case coda_native_type_int32:
                        result = coda_cursor_read_int32(&local_cursor, &(((int32_t *)dst)[i]));
                        break;
                    case coda_native_type_uint32:
                        result = coda_cursor_read_uint32(&local_cursor, &(((uint32_t *)dst)[i]));
                        break;
                    case coda_native_type_int64:
                        result = coda_cursor_read_int64(&local_cursor, &(((int64_t *)dst)[i]));
                        break;
                    case coda_native_type_uint64:
                        result = coda_cursor_read_uint64(&local_cursor, &(((uint64_t *)dst)[i]));
                        break;
                    case coda_native_type_float:
                        result = coda_cursor_read_float(&local_cursor, &(((float *)dst)[i]));
                        break;
                    case coda_native_type_double:
                        result = coda_cursor_read_double(&local_cursor, &(((double *)dst)[i]));
                        break;
                    case coda_native_type_char:
                        result = coda_cursor_read_char(&local_cursor, &(((char *)dst)[i]));
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
                if (result < 0)
                {
                    return -1;
                }
                if (i < num_elements - 1)
                {
                    if (coda_cursor_goto_next_array_element(&local_cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

int coda_qiap_perform_actions_for_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_int8, dst);
}

int coda_qiap_perform_actions_for_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_uint8, dst);
}

int coda_qiap_perform_actions_for_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_int16, dst);
}

int coda_qiap_perform_actions_for_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_uint16, dst);
}

int coda_qiap_perform_actions_for_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_int32, dst);
}

int coda_qiap_perform_actions_for_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_uint32, dst);
}

int coda_qiap_perform_actions_for_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_int64, dst);
}

int coda_qiap_perform_actions_for_uint64_array(const coda_cursor *cursor, uint64_t *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_uint64, dst);
}

int coda_qiap_perform_actions_for_float_array(const coda_cursor *cursor, float *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_float, dst);
}

int coda_qiap_perform_actions_for_double_array(const coda_cursor *cursor, double *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_double, dst);
}

int coda_qiap_perform_actions_for_char_array(const coda_cursor *cursor, char *dst)
{
    return perform_actions_for_array(cursor, coda_native_type_char, dst);
}

/** Set the location of the QIAP Issue Report that should be used.
 * This function should be called before coda_init() is called.
 *
 * The path should be a full path to the QIAP Issue Report file.
 *
 * Specifying a path using this function will prevent CODA from using the CODA_QIAP_REPORT environment variable.
 * If you still want CODA to acknowledge the CODA_QIAP_REPORT environment variable then use something like this in your
 * code:
 * \code
 * if (getenv("CODA_QIAP_REPORT") == NULL)
 * {
 *     coda_qiap_set_report("<path to QIAP Issue Report>");
 * }
 * \endcode
 *
 * \param path  Full path to QIAP Issue Report file
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
LIBCODA_API int coda_qiap_set_report(const char *path)
{
    if (coda_qiap_report != NULL)
    {
        free(coda_qiap_report);
        coda_qiap_report = NULL;
    }
    if (path == NULL)
    {
        return 0;
    }
    coda_qiap_report = strdup(path);
    if (coda_qiap_report == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set the location where log messages of performed QIAP actions should be written.
 * This function should be called before coda_init() is called.
 *
 * The path should be a full path to a file where the log messages will be written.
 * If the file does not yet exist, it will be created.
 * Note that log messages will only be written once a file is closed using coda_close().
 *
 * Specifying a log location using this function will prevent CODA from using the CODA_QIAP_LOG environment variable.
 * If you still want CODA to acknowledge the CODA_QIAP_LOG environment variable then use something like this in your
 * code:
 * \code
 * if (getenv("CODA_QIAP_LOG") == NULL)
 * {
 *     coda_qiap_set_action_log("<QIAP action log file>");
 * }
 * \endcode
 *
 * \param path  Full path to QIAP action log file
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
LIBCODA_API int coda_qiap_set_action_log(const char *path)
{
    if (coda_qiap_log != NULL)
    {
        free(coda_qiap_log);
        coda_qiap_log = NULL;
    }
    if (path == NULL)
    {
        return 0;
    }
    coda_qiap_log = strdup(path);
    if (coda_qiap_log == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_qiap_init_actions(coda_product *product)
{
    if (enable_qiap && quality_issue_report != NULL)
    {
        coda_type *root_type;
        int i;

        if (coda_get_product_root_type(product, &root_type) != 0)
        {
            return -1;
        }

        product->qiap_info = tree_node_new();
        if (product->qiap_info == NULL)
        {
            return -1;
        }

        for (i = 0; i < quality_issue_report->num_quality_issues; i++)
        {
            qiap_quality_issue *quality_issue = quality_issue_report->quality_issue[i];
            qiap_affected_product *affected_product = NULL;
            int has_value_actions = 0;
            int j;

            if (coda_qiap_find_affected_product(product, quality_issue, &affected_product) != 0)
            {
                return -1;
            }
            if (affected_product == NULL)
            {
                /* product not affected -> skip issue */
                continue;
            }

            for (j = 0; j < affected_product->num_affected_values; j++)
            {
                qiap_affected_value *affected_value = affected_product->affected_value[j];
                int k;

                for (k = 0; k < affected_value->num_actions; k++)
                {
                    qiap_action *action = affected_value->action[k];
                    int v;

                    if (action->action_type == qiap_action_discard_value ||
                        action->action_type == qiap_action_correct_value)
                    {
                        coda_qiap_action *item;

                        if (affected_value->extent != NULL || affected_value->num_parameter_values == 0)
                        {
                            item = coda_qiap_action_new(quality_issue->issue_id, affected_product->affected_product_id,
                                                        affected_value->affected_value_id, affected_value->extent,
                                                        action);
                            if (item == NULL)
                            {
                                return -1;
                            }
                            if (tree_node_add_item_for_path(product->qiap_info, root_type, affected_value->parameter,
                                                            item, action->action_type == qiap_action_correct_value)
                                != 0)
                            {
                                coda_qiap_action_delete(item);
                                coda_add_error_message(" for action on '%s' (value_id=%ld, issue_id=%ld)",
                                                       affected_value->parameter, affected_value->affected_value_id,
                                                       quality_issue->issue_id);
                                return -1;
                            }
                        }
                        for (v = 0; v < affected_value->num_parameter_values; v++)
                        {
                            item = coda_qiap_action_new(quality_issue->issue_id, affected_product->affected_product_id,
                                                        affected_value->affected_value_id, NULL, action);
                            if (item == NULL)
                            {
                                return -1;
                            }
                            if (tree_node_add_item_for_path(product->qiap_info, root_type,
                                                            affected_value->parameter_value_path[v], item,
                                                            action->action_type == qiap_action_correct_value) != 0)
                            {
                                coda_qiap_action_delete(item);
                                coda_add_error_message(" for action on '%s' (value_id=%ld, issue_id=%ld)",
                                                       affected_value->parameter, affected_value->affected_value_id,
                                                       quality_issue->issue_id);
                                return -1;
                            }
                        }

                        has_value_actions = 1;
                    }
                }
            }

            for (j = 0; j < affected_product->num_actions; j++)
            {
                if (affected_product->action[j]->action_type == qiap_action_discard_product && !has_value_actions)
                {
                    qiap_set_error(QIAP_ERROR_DISCARD, "product should be discarded");
                    coda_set_error(CODA_ERROR_QIAP, NULL);
                    return -1;
                }
            }
        }
    }

    return 0;
}

void coda_qiap_delete_actions(coda_product *product)
{
    if (product->qiap_info != NULL)
    {
        tree_node_delete(product->qiap_info, coda_qiap_action_delete);
        product->qiap_info = NULL;
    }
}

/** Enable/Disable the use of QIAP.
 * \param enable
 *   \arg 0: Disable QIAP.
 *   \arg 1: Enable QIAP.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check coda_errno).
 */
LIBCODA_API int coda_set_option_enable_qiap(int enable)
{
    if (enable != 0 && enable != 1)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid (%s:%u)", enable, __FILE__,
                       __LINE__);
        return -1;
    }

    enable_qiap = enable;

    return 0;
}

/** Retrieve the current setting on whether QIAP is enabled.
 * \see coda_set_option_enable_qiap()
 * \return
 *   \arg \c 0, QIAP is disabled.
 *   \arg \c 1, QIAP is enabled.
 */
LIBCODA_API int coda_get_option_enable_qiap(void)
{
    return enable_qiap;
}

int coda_qiap_init(void)
{
    if (init_counter == 0 && enable_qiap)
    {
        if (coda_qiap_report == NULL)
        {
            if (getenv("CODA_QIAP_REPORT") != NULL)
            {
                coda_qiap_report = strdup(getenv("CODA_QIAP_REPORT"));
                if (coda_qiap_report == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    return -1;
                }
            }
        }
        if (coda_qiap_log == NULL)
        {
            if (getenv("CODA_QIAP_LOG") != NULL)
            {
                coda_qiap_log = strdup(getenv("CODA_QIAP_LOG"));
                if (coda_qiap_log == NULL)
                {
                    if (coda_qiap_report != NULL)
                    {
                        free(coda_qiap_report);
                        coda_qiap_report = NULL;
                    }
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    return -1;
                }
            }
        }
        if (coda_qiap_report != NULL)
        {
            assert(quality_issue_report == NULL);
            if (qiap_read_report(coda_qiap_report, &quality_issue_report) != 0)
            {
                free(coda_qiap_report);
                coda_qiap_report = NULL;
                if (coda_qiap_log != NULL)
                {
                    free(coda_qiap_log);
                    coda_qiap_log = NULL;
                }
                return -1;
            }
        }
    }
    init_counter++;

    return 0;
}

void coda_qiap_done(void)
{
    if (init_counter > 0)
    {
        init_counter--;
        if (init_counter == 0)
        {
            if (coda_qiap_report != NULL)
            {
                free(coda_qiap_report);
                coda_qiap_report = NULL;
            }
            if (coda_qiap_log != NULL)
            {
                free(coda_qiap_log);
                coda_qiap_log = NULL;
            }
            if (quality_issue_report != NULL)
            {
                qiap_quality_issue_report_delete(quality_issue_report);
                quality_issue_report = NULL;
            }
        }
    }
}

/** @} */
