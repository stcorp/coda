/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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

#include "coda-xml-internal.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-definition.h"

int coda_xml_recognize_file(const char *filename, int64_t size, coda_product_definition **definition)
{
    int fd;
    int open_flags;

    size = size;

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    fd = open(filename, open_flags);
    if (fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", filename, strerror(errno));
        return -1;
    }

    if (coda_xml_parse_for_detection(fd, filename, definition) != 0)
    {
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

int coda_xml_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                  coda_product **product)
{
    coda_xml_product *product_file;
    int open_flags;

    product_file = (coda_xml_product *)malloc(sizeof(coda_xml_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xml_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_xml;
    product_file->root_type = NULL;
    product_file->product_definition = definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
#if CODA_USE_QIAP
    product_file->qiap_info = NULL;
#endif
    product_file->use_mmap = 0;
    product_file->fd = -1;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_xml_close((coda_product *)product_file);
        return -1;
    }

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    product_file->fd = open(product_file->filename, open_flags);
    if (product_file->fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename, strerror(errno));
        coda_xml_close((coda_product *)product_file);
        return -1;
    }

    if (coda_xml_parse(product_file) != 0)
    {
        coda_xml_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_xml_close(coda_product *product)
{
    coda_xml_product *product_file = (coda_xml_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete(product_file->root_type);
    }
    if (product_file->fd >= 0)
    {
        close(product_file->fd);
    }

    free(product_file);

    return 0;
}

static void delete_detection_node(coda_xml_detection_node *node)
{
    int i;

    hashtable_delete(node->attribute_hash_data);
    hashtable_delete(node->hash_data);
    if (node->xml_name != NULL)
    {
        free(node->xml_name);
    }
    if (node->detection_rule != NULL)
    {
        /* we don't have to delete the detection rules, since we have only stored references */
        free(node->detection_rule);
    }
    if (node->attribute_subnode != NULL)
    {
        for (i = 0; i < node->num_attribute_subnodes; i++)
        {
            delete_detection_node(node->attribute_subnode[i]);
        }
        free(node->subnode);
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

static coda_xml_detection_node *detection_node_new(const char *xml_name, coda_xml_detection_node *parent)
{
    coda_xml_detection_node *node;

    node = malloc(sizeof(coda_xml_detection_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_xml_detection_node), __FILE__, __LINE__);
        return NULL;
    }
    node->xml_name = NULL;
    node->num_detection_rules = 0;
    node->detection_rule = NULL;
    node->num_attribute_subnodes = 0;
    node->attribute_subnode = NULL;
    node->attribute_hash_data = NULL;
    node->num_subnodes = 0;
    node->subnode = NULL;
    node->hash_data = NULL;
    node->parent = parent;

    if (xml_name != NULL)
    {
        node->xml_name = strdup(xml_name);
        if (node->xml_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            delete_detection_node(node);
            return NULL;
        }
    }

    node->attribute_hash_data = hashtable_new(1);
    if (node->attribute_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_detection_node(node);
        return NULL;
    }

    node->hash_data = hashtable_new(1);
    if (node->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_detection_node(node);
        return NULL;
    }

    return node;
}

coda_xml_detection_node *coda_xml_detection_node_get_attribute_subnode(coda_xml_detection_node *node,
                                                                       const char *xml_name)
{
    int index;

    index = hashtable_get_index_from_name(node->attribute_hash_data, xml_name);
    if (index >= 0)
    {
        return node->attribute_subnode[index];
    }
    return NULL;
}

coda_xml_detection_node *coda_xml_detection_node_get_subnode(coda_xml_detection_node *node, const char *xml_name)
{
    int index;

    index = hashtable_get_index_from_name(node->hash_data, xml_name);
    if (index >= 0)
    {
        return node->subnode[index];
    }
    return NULL;
}

static int detection_node_add_rule(coda_xml_detection_node *node, coda_detection_rule *detection_rule)
{
    coda_detection_rule **new_detection_rule;
    int position;
    int i;

    assert(node != NULL);
    assert(detection_rule != NULL);

    /* determine the position of the new rule based on presedence */
    position = node->num_detection_rules;
    while (position > 0)
    {
        int is_equal = 0;

        if (detection_rule->entry[0]->value == NULL)
        {
            if (node->detection_rule[position - 1]->entry[0]->value != NULL)
            {
                break;
            }
            is_equal = 1;
        }
        else
        {
            is_equal = (node->detection_rule[position - 1]->entry[0]->value != NULL &&
                        strcmp(detection_rule->entry[0]->value,
                               node->detection_rule[position - 1]->entry[0]->value) == 0);
        }
        if (is_equal)
        {
            is_equal = 0;
            if (detection_rule->num_entries > node->detection_rule[position - 1]->num_entries)
            {
                break;
            }
            if (detection_rule->num_entries == node->detection_rule[position - 1]->num_entries)
            {
                is_equal = 1;
                for (i = 1; i < detection_rule->num_entries; i++)
                {
                    if (detection_rule->entry[i]->offset != node->detection_rule[position - 1]->entry[i]->offset ||
                        detection_rule->entry[i]->value_length !=
                        node->detection_rule[position - 1]->entry[i]->value_length ||
                        strcmp(detection_rule->entry[i]->value,
                               node->detection_rule[position - 1]->entry[i]->value) != 0)
                    {
                        is_equal = 0;
                        break;
                    }
                }
                if (is_equal)
                {
                    coda_set_error(CODA_ERROR_DATA_DEFINITION,
                                   "detection rule for product definition %s is shadowed by product definition %s",
                                   detection_rule->product_definition->name,
                                   node->detection_rule[position - 1]->product_definition->name);
                    return -1;
                }
            }
        }
        position--;
    }

    new_detection_rule = realloc(node->detection_rule, (node->num_detection_rules + 1) * sizeof(coda_detection_rule *));
    if (new_detection_rule == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_detection_rules + 1) * sizeof(coda_detection_rule *), __FILE__, __LINE__);
        return -1;
    }
    node->detection_rule = new_detection_rule;
    for (i = node->num_detection_rules; i > position; i--)
    {
        node->detection_rule[i] = node->detection_rule[i - 1];
    }
    node->detection_rule[position] = detection_rule;
    node->num_detection_rules++;

    return 0;
}

static int detection_node_add_attribute_subnode(coda_xml_detection_node *node, const char *xml_name)
{
    coda_xml_detection_node **subnode_list;
    coda_xml_detection_node *subnode;
    int result;

    assert(node != NULL);
    assert(xml_name != NULL);

    subnode = detection_node_new(xml_name, node);
    if (subnode == NULL)
    {
        return -1;
    }
    subnode_list = realloc(node->attribute_subnode,
                           (node->num_attribute_subnodes + 1) * sizeof(coda_xml_detection_node *));
    if (subnode_list == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_attribute_subnodes + 1) * sizeof(coda_xml_detection_node *), __FILE__,
                       __LINE__);
        delete_detection_node(subnode);
        return -1;
    }
    node->attribute_subnode = subnode_list;
    node->attribute_subnode[node->num_attribute_subnodes] = subnode;
    node->num_attribute_subnodes++;

    result = hashtable_add_name(node->attribute_hash_data, subnode->xml_name);
    assert(result == 0);

    return 0;
}

static int detection_node_add_subnode(coda_xml_detection_node *node, const char *xml_name)
{
    coda_xml_detection_node **subnode_list;
    coda_xml_detection_node *subnode;
    int result;

    assert(node != NULL);
    assert(xml_name != NULL);

    subnode = detection_node_new(xml_name, node);
    if (subnode == NULL)
    {
        return -1;
    }
    subnode_list = realloc(node->subnode, (node->num_subnodes + 1) * sizeof(coda_xml_detection_node *));
    if (subnode_list == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(node->num_subnodes + 1) * sizeof(coda_xml_detection_node *), __FILE__, __LINE__);
        delete_detection_node(subnode);
        return -1;
    }
    node->subnode = subnode_list;
    node->subnode[node->num_subnodes] = subnode;
    node->num_subnodes++;

    result = hashtable_add_name(node->hash_data, subnode->xml_name);
    assert(result == 0);

    return 0;
}

void coda_xml_detection_tree_delete(void *detection_tree)
{
    delete_detection_node((coda_xml_detection_node *)detection_tree);
}

int coda_xml_detection_tree_add_rule(void *detection_tree, coda_detection_rule *detection_rule)
{
    coda_xml_detection_node *node;
    const char *xml_name;
    int last_node = 0;
    int next_is_attribute = 0;
    int i;
    char *path;
    char *p;

    if (detection_rule->num_entries == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' should have at least one entry",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->path == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "first xml detection rule for '%s' requires path",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->use_filename)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "first xml detection rule for '%s' can not be based on filenames",
                       detection_rule->product_definition->name);
        return -1;
    }
    if (detection_rule->entry[0]->offset != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "first xml detection rule for '%s' can not be based on offsets",
                       detection_rule->product_definition->name);
        return -1;
    }
    for (i = 1; i < detection_rule->num_entries; i++)
    {
        if (!detection_rule->entry[i]->use_filename)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "xml detection rule %d for '%s' can only be based on filenames", i,
                           detection_rule->product_definition->name);
            return -1;
        }
        if (detection_rule->entry[i]->path != NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule %d for '%s' can not be based on paths", i,
                           detection_rule->product_definition->name);
            return -1;
        }
        if (detection_rule->entry[i]->offset < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule %d for '%s' has an invalid offset", i,
                           detection_rule->product_definition->name);
            return -1;
        }
        if (detection_rule->entry[i]->value_length == 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml detection rule %d for '%s' has an empty entry", i,
                           detection_rule->product_definition->name);
            return -1;
        }
    }

    node = *(coda_xml_detection_node **)detection_tree;
    if (node == NULL)
    {
        node = detection_node_new(NULL, NULL);
        if (node == NULL)
        {
            return -1;
        }
        *(coda_xml_detection_node **)detection_tree = node;
    }

    path = strdup(detection_rule->entry[0]->path);
    if (path == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    p = path;
    if (*p == '/')
    {
        p++;
    }
    if (*p == '@')
    {
        next_is_attribute = 1;
        p++;
    }
    while (!last_node)
    {
        coda_xml_detection_node *subnode;
        int is_attribute = next_is_attribute;

        xml_name = p;
        while (*p != '/' && *p != '@' && *p != '\0')
        {
            if (*p == '{')
            {
                /* parse namespace */
                p++;
                xml_name = p;
                while (*p != '}')
                {
                    if (*p == '\0')
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                                       "xml detection rule for '%s' has invalid path value",
                                       detection_rule->product_definition->name);
                        free(path);
                        return -1;
                    }
                    p++;
                }
                *p = ' ';
            }
            p++;
        }
        next_is_attribute = (*p == '@');
        last_node = (*p == '\0');
        *p = '\0';

        if (is_attribute)
        {
            subnode = coda_xml_detection_node_get_attribute_subnode(node, xml_name);
            if (subnode == NULL)
            {
                if (detection_node_add_attribute_subnode(node, xml_name) != 0)
                {
                    free(path);
                    return -1;
                }
                subnode = coda_xml_detection_node_get_attribute_subnode(node, xml_name);
                if (subnode == NULL)
                {
                    free(path);
                    return -1;
                }
            }
        }
        else
        {
            subnode = coda_xml_detection_node_get_subnode(node, xml_name);
            if (subnode == NULL)
            {
                if (detection_node_add_subnode(node, xml_name) != 0)
                {
                    free(path);
                    return -1;
                }
                subnode = coda_xml_detection_node_get_subnode(node, xml_name);
                if (subnode == NULL)
                {
                    free(path);
                    return -1;
                }
            }
        }
        node = subnode;

        if (!last_node)
        {
            if (is_attribute)
            {
                coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                               "xml detection rule for '%s' has invalid path (attribute should be last item in path)",
                               detection_rule->product_definition->name);
                free(path);
                return -1;
            }
            p++;
        }
    }

    free(path);

    if (detection_node_add_rule(node, detection_rule) != 0)
    {
        return -1;
    }

    return 0;
}

coda_xml_detection_node *coda_xml_get_detection_tree(void)
{
    return (coda_xml_detection_node *)coda_global_data_dictionary->xml_detection_tree;
}
