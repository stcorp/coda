/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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
#include "coda-expr.h"

struct coda_ProductVariable_struct
{
    char *name;
    coda_Expr *size_expr;
    coda_Expr *init_expr;
};
typedef struct coda_ProductVariable_struct coda_ProductVariable;

struct coda_DetectionRuleEntry_struct
{
    /* if 'use_filename' is set, it is a filename match and both 'offset' and 'value/length' should be provided */
    /* if only 'offset' is provided, it is a file size test (in bytes); ascii/binary only */
    /* if only 'path' is provided, it is existence test; xml only */
    /* if only 'value/length' is provided, it is a string match on the 4096 detection block; ascii only */
    /* if both 'offset' and 'value/length' are provided, it is a string match on the detection block; ascii/binary only */
    /* if both 'path' and 'value/length' are provided, it is a string match on the element pointed to by path; xml only */
    /* providing both 'path' and 'offset' is not allowed */
    int use_filename;   /* 0 if not set */
    int64_t offset;     /* -1 if not set */
    char *path; /* NULL if not set */
    char *value;        /* NULL if not set */
    long value_length;  /* 0 if not set */
};
typedef struct coda_DetectionRuleEntry_struct coda_DetectionRuleEntry;

struct coda_DetectionRule_struct
{
    int num_entries;
    coda_DetectionRuleEntry **entry;

    struct coda_ProductDefinition_struct *product_definition;
};
typedef struct coda_DetectionRule_struct coda_DetectionRule;

struct coda_ProductDefinition_struct
{
    coda_format format;
    int version;
    char *name;
    char *description;

    int num_detection_rules;
    coda_DetectionRule **detection_rule;

    coda_Type *root_type;

    int num_product_variables;
    coda_ProductVariable **product_variable;
    hashtable *hash_data;

    struct coda_ProductType_struct *product_type;
};

struct coda_ProductType_struct
{
    char *name;
    char *description;

    int num_product_definitions;
    coda_ProductDefinition **product_definition;
    hashtable *hash_data;

    struct coda_ProductClass_struct *product_class;
};
typedef struct coda_ProductType_struct coda_ProductType;

struct coda_ProductClass_struct
{
    char *name;
    char *description;

    char *definition_file;
    int revision;

    int num_named_types;
    coda_Type **named_type;
    hashtable *named_type_hash_data;

    int num_product_types;
    coda_ProductType **product_type;
    hashtable *product_type_hash_data;
};
typedef struct coda_ProductClass_struct coda_ProductClass;

struct coda_DataDictionary_struct
{
    int num_product_classes;
    coda_ProductClass **product_class;
    hashtable *hash_data;

    void *ascbin_detection_tree;
    void *xml_detection_tree;
};
typedef struct coda_DataDictionary_struct coda_DataDictionary;

extern coda_DataDictionary *coda_data_dictionary;

int coda_type_set_name(coda_Type *type, const char *name);
int coda_type_set_description(coda_Type *type, const char *description);

coda_DetectionRuleEntry *coda_detection_rule_entry_with_offset_new(int64_t offset, int use_filename);
coda_DetectionRuleEntry *coda_detection_rule_entry_with_path_new(const char *path);
coda_DetectionRuleEntry *coda_detection_rule_entry_with_size_new(int64_t size);
int coda_detection_rule_entry_set_value(coda_DetectionRuleEntry *match_rule, const char *value, long value_length);
int coda_detection_rule_entry_validate(coda_DetectionRuleEntry *match_rule);
void coda_detection_rule_entry_delete(coda_DetectionRuleEntry *entry);

coda_DetectionRule *coda_detection_rule_new(void);
int coda_detection_rule_add_entry(coda_DetectionRule *detection_rule, coda_DetectionRuleEntry *entry);
void coda_detection_rule_delete(coda_DetectionRule *detection_rule);

coda_ProductVariable *coda_product_variable_new(const char *name);
int coda_product_variable_set_size_expression(coda_ProductVariable *product_variable, coda_Expr *size_expr);
int coda_product_variable_set_init_expression(coda_ProductVariable *product_variable, coda_Expr *init_expr);
int coda_product_variable_validate(coda_ProductVariable *product_variable);
void coda_product_variable_delete(coda_ProductVariable *product_variable);

coda_ProductDefinition *coda_product_definition_new(const char *name, coda_format format, int version);
int coda_product_definition_set_description(coda_ProductDefinition *product_definition, const char *description);
int coda_product_definition_add_detection_rule(coda_ProductDefinition *product_definition,
                                               coda_DetectionRule *detection_rule);
int coda_product_definition_set_root_type(coda_ProductDefinition *product_definition, coda_Type *root_type);
int coda_product_definition_add_product_variable(coda_ProductDefinition *product_definition,
                                                 coda_ProductVariable *product_variable);
int coda_product_definition_validate(coda_ProductDefinition *product_definition);
void coda_product_definition_delete(coda_ProductDefinition *product_definition);

coda_ProductType *coda_product_type_new(const char *name);
int coda_product_type_set_description(coda_ProductType *product_type, const char *description);
int coda_product_type_add_product_definition(coda_ProductType *product_type,
                                             coda_ProductDefinition *product_definition);
coda_ProductDefinition *coda_product_type_get_product_definition_by_version(coda_ProductType *product_type,
                                                                            int version);
void coda_product_type_delete(coda_ProductType *product_type);

coda_ProductClass *coda_product_class_new(const char *name);
int coda_product_class_set_description(coda_ProductClass *product_class, const char *description);
int coda_product_class_set_definition_file(coda_ProductClass *product_class, const char *filepath);
int coda_product_class_set_revision(coda_ProductClass *product_class, int revision);
int coda_product_class_add_named_type(coda_ProductClass *product_class, coda_Type *type);
int coda_product_class_add_product_type(coda_ProductClass *product_class, coda_ProductType *product_type);
coda_Type *coda_product_class_get_named_type(const coda_ProductClass *product_class, const char *name);
int coda_product_class_has_named_type(const coda_ProductClass *product_class, const char *name);
coda_ProductType *coda_product_class_get_product_type(const coda_ProductClass *product_class, const char *name);
int coda_product_class_has_product_type(const coda_ProductClass *product_class, const char *name);
int coda_product_class_get_revision(const coda_ProductClass *product_class);
void coda_product_class_delete(coda_ProductClass *product_class);

int coda_data_dictionary_init(void);
int coda_data_dictionary_add_product_class(coda_ProductClass *product_class);
coda_ProductClass *coda_data_dictionary_get_product_class(const char *name);
int coda_data_dictionary_has_product_class(const char *name);
int coda_data_dictionary_remove_product_class(coda_ProductClass *product_class);
void coda_data_dictionary_done(void);


#endif
