/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

THREAD_LOCAL coda_data_dictionary *coda_global_data_dictionary = NULL;

void coda_detection_tree_delete(void *detection_tree);
int coda_detection_tree_add_rule(void *detection_tree, coda_detection_rule *detection_rule);

static int data_dictionary_add_detection_rule(coda_detection_rule *detection_rule);
static int data_dictionary_rebuild_detection_tree(void);

void coda_product_variable_delete(coda_product_variable *product_variable)
{
    assert(product_variable != NULL);

    if (product_variable->name != NULL)
    {
        free(product_variable->name);
    }
    if (product_variable->size_expr != NULL)
    {
        coda_expression_delete(product_variable->size_expr);
    }
    if (product_variable->init_expr != NULL)
    {
        coda_expression_delete(product_variable->init_expr);
    }

    free(product_variable);
}

coda_product_variable *coda_product_variable_new(const char *name)
{
    coda_product_variable *product_variable;

    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "name '%s' is not a valid identifier for product variable definition", name);
        return NULL;
    }

    product_variable = (coda_product_variable *)malloc(sizeof(coda_product_variable));
    if (product_variable == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_product_variable), __FILE__, __LINE__);
        return NULL;
    }
    product_variable->name = NULL;
    product_variable->size_expr = NULL;
    product_variable->init_expr = NULL;

    product_variable->name = strdup(name);
    if (product_variable->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_variable_delete(product_variable);
        return NULL;
    }

    return product_variable;
}

int coda_product_variable_set_size_expression(coda_product_variable *product_variable, coda_expression *size_expr)
{
    assert(size_expr != NULL);
    if (product_variable->size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product variable already has a size expression");
        return -1;
    }
    product_variable->size_expr = size_expr;
    return 0;
}

int coda_product_variable_set_init_expression(coda_product_variable *product_variable, coda_expression *init_expr)
{
    assert(init_expr != NULL);
    if (product_variable->init_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product variable already has an init expression");
        return -1;
    }
    product_variable->init_expr = init_expr;
    return 0;
}

int coda_product_variable_validate(coda_product_variable *product_variable)
{
    if (product_variable->init_expr == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing init expression for product variable definition");
        return -1;
    }
    return 0;
}

static int init_product_variable(coda_product *product, int index)
{
    coda_cursor cursor;
    int64_t value = 1;

    /* initialize the product variable */

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }
    if (product->product_definition->product_variable[index]->size_expr != NULL)
    {
        if (coda_expression_eval_integer
            (product->product_definition->product_variable[index]->size_expr, &cursor, &value) != 0)
        {
            coda_add_error_message(" while determining length of product variable %s",
                                   product->product_definition->product_variable[index]->name);
            return -1;
        }
    }

    product->product_variable_size[index] = (int)value;
    product->product_variable[index] = (int64_t *)malloc((size_t)value * sizeof(int64_t));
    if (product->product_variable[index] == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)value * sizeof(int64_t), __FILE__, __LINE__);
        return -1;
    }
    memset(product->product_variable[index], 0, (size_t)value * sizeof(int64_t));
    if (coda_expression_eval_void(product->product_definition->product_variable[index]->init_expr, &cursor) != 0)
    {
        coda_add_error_message(" while initializing product variable %s",
                               product->product_definition->product_variable[index]->name);
        return -1;
    }

    return 0;
}

int coda_product_variable_get_size(coda_product *product, const char *name, long *size)
{
    long index;

    assert(product != NULL && name != NULL && size != NULL);
    assert(product->product_variable != NULL);

    index = hashtable_get_index_from_name(product->product_definition->hash_data, name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "product type %s (version %d) does not contain a product variable with name %s",
                       product->product_definition->product_type->name, product->product_definition->version, name);
        return -1;
    }

    if (product->product_variable[index] == NULL)
    {
        if (init_product_variable(product, index) != 0)
        {
            return -1;
        }
    }
    *size = product->product_variable_size[index];

    return 0;
}

int coda_product_variable_get_pointer(coda_product *product, const char *name, long i, int64_t **ptr)
{
    long index;

    assert(product != NULL && name != NULL && ptr != NULL);

    index = hashtable_get_index_from_name(product->product_definition->hash_data, name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "product type %s (version %d) does not contain a product variable with name %s",
                       product->product_definition->product_type->name, product->product_definition->version, name);
        return -1;
    }

    if (product->product_variable[index] == NULL)
    {
        if (init_product_variable(product, index) != 0)
        {
            return -1;
        }
    }
    if (i < 0 || i >= product->product_variable_size[index])
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "request for index (%ld) exceeds size of product variable %s",
                       i, name);
        return -1;
    }

    *ptr = &(product->product_variable[index][i]);

    return 0;
}

void coda_product_definition_delete(coda_product_definition *product_definition)
{
    int i;

    if (product_definition->detection_rule != NULL)
    {
        for (i = 0; i < product_definition->num_detection_rules; i++)
        {
            coda_detection_rule_delete(product_definition->detection_rule[i]);
        }
        free(product_definition->detection_rule);
    }
    if (product_definition->name != NULL)
    {
        free(product_definition->name);
    }
    if (product_definition->description != NULL)
    {
        free(product_definition->description);
    }
    if (product_definition->root_type != NULL)
    {
        coda_type_release(product_definition->root_type);
    }
    if (product_definition->hash_data != NULL)
    {
        hashtable_delete(product_definition->hash_data);
    }
    if (product_definition->product_variable != NULL)
    {
        for (i = 0; i < product_definition->num_product_variables; i++)
        {
            coda_product_variable_delete(product_definition->product_variable[i]);
        }
        free(product_definition->product_variable);
    }
    free(product_definition);
}

coda_product_definition *coda_product_definition_new(const char *name, coda_format format, int version)
{
    coda_product_definition *product_definition;

    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "name '%s' is not a valid identifier for product definition", name);
        return NULL;
    }

    product_definition = malloc(sizeof(coda_product_definition));
    if (product_definition == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_product_definition), __FILE__, __LINE__);
        return NULL;
    }
    product_definition->format = format;
    product_definition->version = version;
    product_definition->name = NULL;
    product_definition->description = NULL;
    product_definition->num_detection_rules = 0;
    product_definition->detection_rule = NULL;
    product_definition->initialized = 0;
    product_definition->root_type = NULL;
    product_definition->hash_data = NULL;
    product_definition->num_product_variables = 0;
    product_definition->product_variable = NULL;
    product_definition->product_type = NULL;

    product_definition->name = strdup(name);
    if (product_definition->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_definition_delete(product_definition);
        return NULL;
    }

    product_definition->hash_data = hashtable_new(1);
    if (product_definition->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_definition_delete(product_definition);
        return NULL;
    }

    return product_definition;
}

int coda_product_definition_set_description(coda_product_definition *product_definition, const char *description)
{
    char *new_description = NULL;

    if (product_definition->description != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product definition already has a description");
        return -1;
    }
    if (description != NULL)
    {
        new_description = strdup(description);
        if (new_description == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    product_definition->description = new_description;

    return 0;
}

int coda_product_definition_set_root_type(coda_product_definition *product_definition, coda_type *root_type)
{
    assert(root_type != NULL);
    if (product_definition->root_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product definition already has a root type");
        return -1;
    }
    product_definition->root_type = root_type;
    root_type->retain_count++;
    return 0;
}

int coda_product_definition_add_detection_rule(coda_product_definition *product_definition,
                                               coda_detection_rule *detection_rule)
{
    coda_detection_rule **new_detection_rule;

    if (detection_rule->num_entries == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' should have at least one entry",
                       product_definition->name);
        return -1;
    }

    detection_rule->product_definition = product_definition;

    if (data_dictionary_add_detection_rule(detection_rule) != 0)
    {
        return -1;
    }

    new_detection_rule = realloc(product_definition->detection_rule,
                                 (product_definition->num_detection_rules + 1) * sizeof(coda_detection_rule *));
    if (new_detection_rule == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (product_definition->num_detection_rules + 1) * sizeof(coda_detection_rule *), __FILE__,
                       __LINE__);
        return -1;
    }
    product_definition->detection_rule = new_detection_rule;
    product_definition->num_detection_rules++;
    product_definition->detection_rule[product_definition->num_detection_rules - 1] = detection_rule;

    return 0;
}

int coda_product_definition_add_product_variable(coda_product_definition *product_definition,
                                                 coda_product_variable *product_variable)
{
    if (hashtable_add_name(product_definition->hash_data, product_variable->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate product variable %s for product definition %s",
                       product_variable->name, product_definition->name);
        return -1;
    }
    if (product_definition->num_product_variables % BLOCK_SIZE == 0)
    {
        coda_product_variable **new_product_variable;

        new_product_variable = realloc(product_definition->product_variable,
                                       (product_definition->num_product_variables + BLOCK_SIZE) *
                                       sizeof(coda_product_variable *));
        if (new_product_variable == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (product_definition->num_product_variables + BLOCK_SIZE) *
                           sizeof(coda_product_variable *), __FILE__, __LINE__);
            return -1;
        }
        product_definition->product_variable = new_product_variable;
    }
    product_definition->num_product_variables++;
    product_definition->product_variable[product_definition->num_product_variables - 1] = product_variable;

    return 0;
}

int coda_product_definition_validate(coda_product_definition *product_definition)
{
    if (product_definition->format == coda_format_ascii || product_definition->format == coda_format_binary)
    {
        if (product_definition->root_type == NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing root type for product type version definition");
            return -1;
        }
    }

    product_definition->initialized = 1;

    return 0;
}

void coda_product_type_delete(coda_product_type *product_type)
{
    int i;

    if (product_type->name != NULL)
    {
        free(product_type->name);
    }
    if (product_type->description != NULL)
    {
        free(product_type->description);
    }
    if (product_type->hash_data != NULL)
    {
        hashtable_delete(product_type->hash_data);
    }
    if (product_type->product_definition != NULL)
    {
        for (i = 0; i < product_type->num_product_definitions; i++)
        {
            coda_product_definition_delete(product_type->product_definition[i]);
        }
        free(product_type->product_definition);
    }
    free(product_type);
}

coda_product_type *coda_product_type_new(const char *name)
{
    coda_product_type *product_type;

    product_type = malloc(sizeof(coda_product_type));
    if (product_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_product_type), __FILE__, __LINE__);
        return NULL;
    }
    product_type->name = NULL;
    product_type->description = NULL;
    product_type->num_product_definitions = 0;
    product_type->hash_data = NULL;
    product_type->product_definition = NULL;

    product_type->name = strdup(name);
    if (product_type->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_type_delete(product_type);
        return NULL;
    }

    product_type->hash_data = hashtable_new(1);
    if (product_type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_type_delete(product_type);
        return NULL;
    }

    return product_type;
}

int coda_product_type_set_description(coda_product_type *product_type, const char *description)
{
    char *new_description = NULL;

    if (product_type->description != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product type already has a description");
        return -1;
    }
    if (description != NULL)
    {
        new_description = strdup(description);
        if (new_description == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    product_type->description = new_description;

    return 0;
}

int coda_product_type_add_product_definition(coda_product_type *product_type,
                                             coda_product_definition *product_definition)
{
    int i;

    if (product_definition->product_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product definition %s can not be used by more than one product "
                       "type (%s and %s)", product_definition->name, product_definition->product_type->name,
                       product_type->name);
        return -1;
    }
    for (i = 0; i < product_type->num_product_definitions; i++)
    {
        if (product_type->product_definition[i]->version == product_definition->version)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "multiple product definitions with version number %d for "
                           "product type %s", product_definition->version, product_type->name);
            return -1;
        }
    }
    if (hashtable_add_name(product_type->hash_data, product_definition->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate product definition %s for product type %s",
                       product_definition->name, product_type->name);
        return -1;
    }
    if (product_type->num_product_definitions % BLOCK_SIZE == 0)
    {
        coda_product_definition **new_product_definition;

        new_product_definition = realloc(product_type->product_definition,
                                         (product_type->num_product_definitions + BLOCK_SIZE) *
                                         sizeof(coda_product_definition *));
        if (new_product_definition == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (product_type->num_product_definitions + BLOCK_SIZE) *
                           sizeof(coda_product_definition *), __FILE__, __LINE__);
            return -1;
        }
        product_type->product_definition = new_product_definition;
    }
    product_type->num_product_definitions++;
    product_type->product_definition[product_type->num_product_definitions - 1] = product_definition;

    product_definition->product_type = product_type;

    return 0;
}

coda_product_definition *coda_product_type_get_product_definition_by_version(const coda_product_type *product_type,
                                                                             int version)
{
    int i;

    for (i = 0; i < product_type->num_product_definitions; i++)
    {
        if (product_type->product_definition[i]->version == version)
        {
            return product_type->product_definition[i];
        }
    }

    coda_set_error(CODA_ERROR_DATA_DEFINITION, "product type %s does not contain a definition with version %d",
                   product_type->name, version);
    return NULL;
}

coda_product_definition *coda_product_type_get_latest_product_definition(const coda_product_type *product_type)
{
    int max_version;
    int max_i;
    int i;

    if (product_type->num_product_definitions == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product type %s does not contain any definitions",
                       product_type->name);
        return NULL;
    }
    max_version = product_type->product_definition[0]->version;
    max_i = 0;
    for (i = 1; i < product_type->num_product_definitions; i++)
    {
        if (product_type->product_definition[i]->version > max_version)
        {
            max_version = product_type->product_definition[i]->version;
            max_i = i;
        }
    }

    return product_type->product_definition[max_i];
}

void coda_product_class_delete(coda_product_class *product_class)
{
    int i;

    if (product_class->name != NULL)
    {
        free(product_class->name);
    }
    if (product_class->description != NULL)
    {
        free(product_class->description);
    }
    if (product_class->definition_file != NULL)
    {
        free(product_class->definition_file);
    }
    if (product_class->named_type_hash_data != NULL)
    {
        hashtable_delete(product_class->named_type_hash_data);
    }
    if (product_class->product_type_hash_data != NULL)
    {
        hashtable_delete(product_class->product_type_hash_data);
    }
    if (product_class->named_type != NULL)
    {
        for (i = 0; i < product_class->num_named_types; i++)
        {
            coda_type_release(product_class->named_type[i]);
        }
        free(product_class->named_type);
    }
    if (product_class->product_type != NULL)
    {
        for (i = 0; i < product_class->num_product_types; i++)
        {
            coda_product_type_delete(product_class->product_type[i]);
        }
        free(product_class->product_type);
    }
    free(product_class);
}

coda_product_class *coda_product_class_new(const char *name)
{
    coda_product_class *product_class;

    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "name '%s' is not a valid identifier for product class definition",
                       name);
        return NULL;
    }

    product_class = malloc(sizeof(coda_product_class));
    assert(product_class != NULL);
    product_class->name = NULL;
    product_class->description = NULL;
    product_class->definition_file = NULL;
    product_class->revision = 0;
    product_class->num_named_types = 0;
    product_class->named_type = NULL;
    product_class->named_type_hash_data = NULL;
    product_class->num_product_types = 0;
    product_class->product_type = NULL;
    product_class->product_type_hash_data = NULL;

    product_class->name = strdup(name);
    if (product_class->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_class_delete(product_class);
        return NULL;
    }

    product_class->named_type_hash_data = hashtable_new(1);
    if (product_class->named_type_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_class_delete(product_class);
        return NULL;
    }

    product_class->product_type_hash_data = hashtable_new(1);
    if (product_class->product_type_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_product_class_delete(product_class);
        return NULL;
    }

    return product_class;
}

int coda_product_class_set_description(coda_product_class *product_class, const char *description)
{
    char *new_description = NULL;

    if (product_class->description != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product class already has a description");
        return -1;
    }
    if (description != NULL)
    {
        new_description = strdup(description);
        if (new_description == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    product_class->description = new_description;

    return 0;
}

int coda_product_class_set_definition_file(coda_product_class *product_class, const char *filepath)
{
    char *new_definition_file = NULL;

    if (product_class->definition_file != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product class already has a definition file");
        return -1;
    }
    if (filepath != NULL)
    {
        new_definition_file = strdup(filepath);
        if (new_definition_file == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    product_class->definition_file = new_definition_file;

    return 0;
}

int coda_product_class_set_revision(coda_product_class *product_class, int revision)
{
    product_class->revision = revision;
    return 0;
}

int coda_product_class_add_product_type(coda_product_class *product_class, coda_product_type *product_type)
{
    if (hashtable_add_name(product_class->product_type_hash_data, product_type->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate product type %s for product class %s", product_type->name,
                       product_class->name);
        return -1;
    }
    if (product_class->num_product_types % BLOCK_SIZE == 0)
    {
        coda_product_type **new_product_type;

        new_product_type = realloc(product_class->product_type, (product_class->num_product_types + BLOCK_SIZE) *
                                   sizeof(coda_product_type *));
        if (new_product_type == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (product_class->num_product_types + BLOCK_SIZE) * sizeof(coda_product_type *), __FILE__,
                           __LINE__);
            return -1;
        }
        product_class->product_type = new_product_type;
    }
    product_class->num_product_types++;
    product_class->product_type[product_class->num_product_types - 1] = product_type;

    product_type->product_class = product_class;

    return 0;
}

coda_product_type *coda_product_class_get_product_type(const coda_product_class *product_class, const char *name)
{
    int index;

    index = hashtable_get_index_from_name(product_class->product_type_hash_data, name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product class %s does not contain a product type with name %s",
                       product_class->name, name);
        return NULL;
    }

    return product_class->product_type[index];
}

int coda_product_class_has_product_type(const coda_product_class *product_class, const char *name)
{
    return (hashtable_get_index_from_name(product_class->product_type_hash_data, name) != -1);
}


int coda_product_class_add_named_type(coda_product_class *product_class, coda_type *type)
{
    if (hashtable_add_name(product_class->named_type_hash_data, type->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate named type %s for product class %s", type->name,
                       product_class->name);
        return -1;
    }
    if (product_class->num_named_types % BLOCK_SIZE == 0)
    {
        coda_type **new_named_type;

        new_named_type = realloc(product_class->named_type, (product_class->num_named_types + BLOCK_SIZE) *
                                 sizeof(coda_type *));
        assert(new_named_type != NULL);
        product_class->named_type = new_named_type;
    }
    product_class->num_named_types++;
    product_class->named_type[product_class->num_named_types - 1] = type;
    type->retain_count++;

    return 0;
}

coda_type *coda_product_class_get_named_type(const coda_product_class *product_class, const char *name)
{
    int index;

    index = hashtable_get_index_from_name(product_class->named_type_hash_data, name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "product class %s does not contain a named type with name %s",
                       product_class->name, name);
        return NULL;
    }

    return product_class->named_type[index];
}

int coda_product_class_has_named_type(const coda_product_class *product_class, const char *name)
{
    return (hashtable_get_index_from_name(product_class->named_type_hash_data, name) != -1);
}

int coda_product_class_get_revision(const coda_product_class *product_class)
{
    return product_class->revision;
}

static void delete_data_dictionary(coda_data_dictionary *data_dictionary)
{
    int i;

    assert(data_dictionary != NULL);
    if (data_dictionary->hash_data != NULL)
    {
        hashtable_delete(data_dictionary->hash_data);
    }
    if (data_dictionary->product_class != NULL)
    {
        for (i = 0; i < data_dictionary->num_product_classes; i++)
        {
            coda_product_class_delete(data_dictionary->product_class[i]);
        }
        free(data_dictionary->product_class);
    }
    for (i = 0; i < CODA_NUM_FORMATS; i++)
    {
        if (data_dictionary->detection_tree[i] != NULL)
        {
            coda_detection_tree_delete(data_dictionary->detection_tree[i]);
        }
    }

    free(data_dictionary);
}

static coda_data_dictionary *coda_data_dictionary_new(void)
{
    coda_data_dictionary *data_dictionary;
    int i;

    data_dictionary = malloc(sizeof(coda_data_dictionary));
    if (data_dictionary == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_data_dictionary), __FILE__, __LINE__);
        return NULL;
    }
    data_dictionary->num_product_classes = 0;
    data_dictionary->product_class = NULL;
    data_dictionary->hash_data = NULL;
    for (i = 0; i < CODA_NUM_FORMATS; i++)
    {
        data_dictionary->detection_tree[i] = NULL;
    }

    data_dictionary->hash_data = hashtable_new(1);
    if (data_dictionary->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        delete_data_dictionary(data_dictionary);
        return NULL;
    }

    return data_dictionary;
}

static int data_dictionary_rebuild_product_class_hash_data(void)
{
    int i;

    hashtable_delete(coda_global_data_dictionary->hash_data);
    coda_global_data_dictionary->hash_data = hashtable_new(1);
    if (coda_global_data_dictionary->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        if (hashtable_add_name
            (coda_global_data_dictionary->hash_data, coda_global_data_dictionary->product_class[i]->name) != 0)
        {
            assert(0);
            exit(1);
        }
    }

    return 0;
}

int coda_data_dictionary_add_product_class(coda_product_class *product_class)
{
    int i;

    if (coda_global_data_dictionary == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "coda has not been initialized");
        return -1;
    }

    if (hashtable_add_name(coda_global_data_dictionary->hash_data, product_class->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate product class %s", product_class->name);
        return -1;
    }
    if (coda_global_data_dictionary->num_product_classes % BLOCK_SIZE == 0)
    {
        coda_product_class **new_product_class;

        new_product_class = realloc(coda_global_data_dictionary->product_class,
                                    (coda_global_data_dictionary->num_product_classes + BLOCK_SIZE) *
                                    sizeof(coda_product_class *));
        if (new_product_class == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (coda_global_data_dictionary->num_product_classes + BLOCK_SIZE) *
                           sizeof(coda_product_class *), __FILE__, __LINE__);
            return -1;
        }
        coda_global_data_dictionary->product_class = new_product_class;
    }
    /* add sorted */
    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        if (strcmp(product_class->name, coda_global_data_dictionary->product_class[i]->name) < 0)
        {
            coda_product_class *temp = product_class;

            product_class = coda_global_data_dictionary->product_class[i];
            coda_global_data_dictionary->product_class[i] = temp;
        }
    }
    coda_global_data_dictionary->num_product_classes++;
    coda_global_data_dictionary->product_class[coda_global_data_dictionary->num_product_classes - 1] = product_class;

    /* rebuild hashtable (since we use a sorted insert) */
    if (data_dictionary_rebuild_product_class_hash_data() != 0)
    {
        return -1;
    }

    return 0;
}

coda_product_class *coda_data_dictionary_get_product_class(const char *name)
{
    int index;

    if (coda_global_data_dictionary == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "coda has not been initialized");
        return NULL;
    }

    index = hashtable_get_index_from_name(coda_global_data_dictionary->hash_data, name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "data dictionary does not contain a product class with name %s",
                       name);
        return NULL;
    }

    return coda_global_data_dictionary->product_class[index];
}

int coda_data_dictionary_has_product_class(const char *name)
{
    if (coda_global_data_dictionary == NULL)
    {
        return 0;
    }
    return (hashtable_get_index_from_name(coda_global_data_dictionary->hash_data, name) != -1);
}

int coda_data_dictionary_remove_product_class(coda_product_class *product_class)
{
    int index;
    int i;

    if (coda_global_data_dictionary == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "coda has not been initialized");
        return -1;
    }

    index = hashtable_get_index_from_name(coda_global_data_dictionary->hash_data, product_class->name);
    if (index == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "data dictionary does not contain a product class with name %s",
                       product_class->name);
        return -1;
    }
    for (i = index + 1; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_global_data_dictionary->product_class[i - 1] = coda_global_data_dictionary->product_class[i];
    }
    coda_global_data_dictionary->num_product_classes--;
    coda_product_class_delete(product_class);

    /* rebuild hashtable */
    if (data_dictionary_rebuild_product_class_hash_data() != 0)
    {
        return -1;
    }

    /* rebuild detection tree */
    if (data_dictionary_rebuild_detection_tree() != 0)
    {
        return -1;
    }

    return 0;
}

static int data_dictionary_add_detection_rule(coda_detection_rule *detection_rule)
{
    coda_format format = detection_rule->product_definition->format;

    if (format == coda_format_ascii)
    {
        format = coda_format_binary;
    }

    return coda_detection_tree_add_rule(&coda_global_data_dictionary->detection_tree[format], detection_rule);
}

static int data_dictionary_rebuild_detection_tree(void)
{
    int i, j, k, l;

    for (i = 0; i < CODA_NUM_FORMATS; i++)
    {
        if (coda_global_data_dictionary->detection_tree[i] != NULL)
        {
            coda_detection_tree_delete(coda_global_data_dictionary->detection_tree[i]);
            coda_global_data_dictionary->detection_tree[i] = NULL;
        }
    }

    for (i = 0; i < coda_global_data_dictionary->num_product_classes; i++)
    {
        coda_product_class *product_class = coda_global_data_dictionary->product_class[i];

        for (j = 0; j < product_class->num_product_types; j++)
        {
            coda_product_type *product_type = product_class->product_type[j];

            for (k = 0; k < product_type->num_product_definitions; k++)
            {
                coda_product_definition *product_definition = product_type->product_definition[k];

                for (l = 0; l < product_definition->num_detection_rules; l++)
                {
                    if (data_dictionary_add_detection_rule(product_definition->detection_rule[l]) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

int coda_data_dictionary_get_definition(const char *product_class_name, const char *product_type_name, int version,
                                        coda_product_definition **definition)
{
    const coda_product_class *product_class;
    const coda_product_type *product_type;
    coda_product_definition *product_definition = NULL;

    product_class = coda_data_dictionary_get_product_class(product_class_name);
    if (product_class == NULL)
    {
        return -1;
    }
    product_type = coda_product_class_get_product_type(product_class, product_type_name);
    if (product_type == NULL)
    {
        return -1;
    }
    if (version < 0)
    {
        product_definition = coda_product_type_get_latest_product_definition(product_type);
    }
    else
    {
        product_definition = coda_product_type_get_product_definition_by_version(product_type, version);
    }
    if (product_definition == NULL)
    {
        return -1;
    }

    *definition = product_definition;
    return 0;
}

coda_detection_node *coda_data_dictionary_get_detection_tree(coda_format format)
{
    if (format == coda_format_ascii)
    {
        format = coda_format_binary;
    }
    return coda_global_data_dictionary->detection_tree[format];
}

int coda_data_dictionary_find_definition_for_product(coda_product *product, coda_product_definition **definition)
{
    coda_cursor cursor;

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }

    return coda_evaluate_detection_node(coda_data_dictionary_get_detection_tree(product->format), &cursor, definition);
}

int coda_data_dictionary_init(void)
{
    assert(coda_global_data_dictionary == NULL);

    coda_global_data_dictionary = coda_data_dictionary_new();
    if (coda_global_data_dictionary == NULL)
    {
        return -1;
    }

    return 0;
}

void coda_data_dictionary_done(void)
{
    assert(coda_global_data_dictionary != NULL);
    delete_data_dictionary(coda_global_data_dictionary);
    coda_global_data_dictionary = NULL;
}
