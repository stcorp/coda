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

#ifndef CODA_DEFINITION_H
#define CODA_DEFINITION_H

#include "coda-internal.h"
#include "coda-type.h"

struct coda_product_variable_struct
{
    char *name;
    coda_expression *size_expr;
    coda_expression *init_expr;
};
typedef struct coda_product_variable_struct coda_product_variable;

struct coda_detection_rule_entry_struct
{
    /* either path and/or expression needs to be != NULL */
    char *path;
    coda_expression *expression;
};
typedef struct coda_detection_rule_entry_struct coda_detection_rule_entry;

struct coda_detection_rule_struct
{
    int num_entries;
    coda_detection_rule_entry **entry;

    struct coda_product_definition_struct *product_definition;
};
typedef struct coda_detection_rule_struct coda_detection_rule;

struct coda_product_definition_struct
{
    coda_format format;
    int version;
    char *name;
    char *description;

    int num_detection_rules;
    coda_detection_rule **detection_rule;

    int initialized;    /* have the root type and product variables been set? */

    coda_type *root_type;

    int num_product_variables;
    coda_product_variable **product_variable;
    hashtable *hash_data;

    struct coda_product_type_struct *product_type;
};

struct coda_product_type_struct
{
    char *name;
    char *description;

    int num_product_definitions;
    coda_product_definition **product_definition;
    hashtable *hash_data;

    struct coda_product_class_struct *product_class;
};
typedef struct coda_product_type_struct coda_product_type;

struct coda_product_class_struct
{
    char *name;
    char *description;

    char *definition_file;
    int revision;

    int num_named_types;
    coda_type **named_type;
    hashtable *named_type_hash_data;

    int num_product_types;
    coda_product_type **product_type;
    hashtable *product_type_hash_data;
};
typedef struct coda_product_class_struct coda_product_class;

struct coda_detection_node_struct
{
    /* relative path to reach this node from the parent node
     * if not NULL, it will be used as an 'exists' condition before evaluating the rule or any sub nodes
     * 'path' and 'expression' can not be both != NULL
     */
    char *path;

    /* detection expression; will be NULL for root node */
    const coda_expression *expression;

    coda_detection_rule *rule;  /* the matching rule when 'expression' or 'path' matches and none of the subnodes match */

    /* sub nodes of this node */
    int num_subnodes;
    struct coda_detection_node_struct **subnode;
};
typedef struct coda_detection_node_struct coda_detection_node;

struct coda_data_dictionary_struct
{
    int num_product_classes;
    coda_product_class **product_class;
    hashtable *hash_data;

    coda_detection_node *detection_tree[CODA_NUM_FORMATS];
};
typedef struct coda_data_dictionary_struct coda_data_dictionary;

extern coda_data_dictionary *coda_global_data_dictionary;

coda_detection_rule_entry *coda_detection_rule_entry_new(const char *path);
int coda_detection_rule_entry_set_expression(coda_detection_rule_entry *entry, coda_expression *expression);
void coda_detection_rule_entry_delete(coda_detection_rule_entry *entry);

coda_detection_rule *coda_detection_rule_new(void);
int coda_detection_rule_add_entry(coda_detection_rule *detection_rule, coda_detection_rule_entry *entry);
void coda_detection_rule_delete(coda_detection_rule *detection_rule);

int coda_evaluate_detection_node(coda_detection_node *node, coda_cursor *cursor, coda_product_definition **definition);

coda_product_variable *coda_product_variable_new(const char *name);
int coda_product_variable_set_size_expression(coda_product_variable *product_variable, coda_expression *size_expr);
int coda_product_variable_set_init_expression(coda_product_variable *product_variable, coda_expression *init_expr);
int coda_product_variable_validate(coda_product_variable *product_variable);
void coda_product_variable_delete(coda_product_variable *product_variable);

coda_product_definition *coda_product_definition_new(const char *name, coda_format format, int version);
int coda_product_definition_set_description(coda_product_definition *product_definition, const char *description);
int coda_product_definition_add_detection_rule(coda_product_definition *product_definition,
                                               coda_detection_rule *detection_rule);
int coda_product_definition_set_root_type(coda_product_definition *product_definition, coda_type *root_type);
int coda_product_definition_add_product_variable(coda_product_definition *product_definition,
                                                 coda_product_variable *product_variable);
int coda_product_definition_validate(coda_product_definition *product_definition);
void coda_product_definition_delete(coda_product_definition *product_definition);

coda_product_type *coda_product_type_new(const char *name);
int coda_product_type_set_description(coda_product_type *product_type, const char *description);
int coda_product_type_add_product_definition(coda_product_type *product_type,
                                             coda_product_definition *product_definition);
coda_product_definition *coda_product_type_get_product_definition_by_version(const coda_product_type *product_type,
                                                                             int version);
coda_product_definition *coda_product_type_get_latest_product_definition(const coda_product_type *product_type);
void coda_product_type_delete(coda_product_type *product_type);

coda_product_class *coda_product_class_new(const char *name);
int coda_product_class_set_description(coda_product_class *product_class, const char *description);
int coda_product_class_set_definition_file(coda_product_class *product_class, const char *filepath);
int coda_product_class_set_revision(coda_product_class *product_class, int revision);
int coda_product_class_add_named_type(coda_product_class *product_class, coda_type *type);
int coda_product_class_add_product_type(coda_product_class *product_class, coda_product_type *product_type);
coda_type *coda_product_class_get_named_type(const coda_product_class *product_class, const char *name);
int coda_product_class_has_named_type(const coda_product_class *product_class, const char *name);
coda_product_type *coda_product_class_get_product_type(const coda_product_class *product_class, const char *name);
int coda_product_class_has_product_type(const coda_product_class *product_class, const char *name);
int coda_product_class_get_revision(const coda_product_class *product_class);
void coda_product_class_delete(coda_product_class *product_class);

int coda_data_dictionary_init(void);
int coda_data_dictionary_add_product_class(coda_product_class *product_class);
coda_product_class *coda_data_dictionary_get_product_class(const char *name);
int coda_data_dictionary_has_product_class(const char *name);
int coda_data_dictionary_remove_product_class(coda_product_class *product_class);
int coda_data_dictionary_get_definition(const char *product_class, const char *product_type, int version,
                                        coda_product_definition **definition);
coda_detection_node *coda_data_dictionary_get_detection_tree(coda_format format);
int coda_data_dictionary_find_definition_for_product(coda_product *product, coda_product_definition **definition);
void coda_data_dictionary_done(void);

#endif
