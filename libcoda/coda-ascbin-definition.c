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

#include "coda-ascbin-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "coda-definition.h"
#include "coda-ascbin-definition.h"

static coda_ascbin_record *empty_record_singleton = NULL;

void coda_conversion_delete(coda_conversion *conversion)
{
    if (conversion->unit != NULL)
    {
        free(conversion->unit);
    }
    free(conversion);
}

coda_conversion *coda_conversion_new(double numerator, double denominator)
{
    coda_conversion *conversion;

    if (denominator == 0.0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "denominator may not be 0 for conversion in definition");
        return NULL;
    }
    conversion = (coda_conversion *)malloc(sizeof(coda_conversion));
    if (conversion == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_conversion), __FILE__, __LINE__);
        return NULL;
    }
    conversion->unit = NULL;
    conversion->numerator = numerator;
    conversion->denominator = denominator;

    return conversion;
}

int coda_conversion_set_unit(coda_conversion *conversion, const char *unit)
{
    char *new_unit = NULL;

    if (conversion->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "conversion already has a unit");
        return -1;
    }
    if (unit != NULL)
    {
        new_unit = strdup(unit);
        if (new_unit == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    conversion->unit = new_unit;

    return 0;
}

void coda_ascbin_field_delete(coda_ascbin_field *field)
{
    if (field->name != NULL)
    {
        free(field->name);
    }
    if (field->real_name != NULL)
    {
        free(field->real_name);
    }
    if (field->type != NULL)
    {
        coda_release_type((coda_type *)field->type);
    }
    if (field->available_expr != NULL)
    {
        coda_expression_delete(field->available_expr);
    }
    if (field->bit_offset_expr != NULL)
    {
        coda_expression_delete(field->bit_offset_expr);
    }
    free(field);
}

void coda_ascbin_record_delete(coda_ascbin_record *record)
{
    if (record->name != NULL)
    {
        free(record->name);
    }
    if (record->description != NULL)
    {
        free(record->description);
    }
    if (record->fast_size_expr != NULL)
    {
        coda_expression_delete(record->fast_size_expr);
    }
    if (record->hash_data != NULL)
    {
        hashtable_delete(record->hash_data);
    }
    if (record->num_fields > 0)
    {
        int i;

        for (i = 0; i < record->num_fields; i++)
        {
            coda_ascbin_field_delete(record->field[i]);
        }
        free(record->field);
    }
    free(record);
}

void coda_ascbin_union_delete(coda_ascbin_union *dd_union)
{
    if (dd_union->field_expr != NULL)
    {
        coda_expression_delete(dd_union->field_expr);
    }
    coda_ascbin_record_delete((coda_ascbin_record *)dd_union);
}

void coda_ascbin_array_delete(coda_ascbin_array *array)
{
    int i;

    if (array->name != NULL)
    {
        free(array->name);
    }
    if (array->description != NULL)
    {
        free(array->description);
    }
    if (array->base_type != NULL)
    {
        coda_release_type((coda_type *)array->base_type);
    }
    if (array->dim != NULL)
    {
        free(array->dim);
    }
    if (array->dim_expr != NULL)
    {
        for (i = 0; i < array->num_dims; i++)
        {
            if (array->dim_expr[i] != NULL)
            {
                coda_expression_delete(array->dim_expr[i]);
            }
        }
        free(array->dim_expr);
    }
    free(array);
}

coda_ascbin_field *coda_ascbin_field_new(const char *name, const char *real_name)
{
    coda_ascbin_field *field;

    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field name '%s' is not a valid identifier for field definition",
                       name);
        return NULL;
    }

    field = (coda_ascbin_field *)malloc(sizeof(coda_ascbin_field));
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_field), __FILE__, __LINE__);
        return NULL;
    }
    field->name = NULL;
    field->real_name = NULL;
    field->type = NULL;
    field->hidden = 0;
    field->available_expr = NULL;
    field->bit_offset_expr = NULL;

    field->name = strdup(name);
    if (field->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_ascbin_field_delete(field);
        return NULL;
    }
    if (real_name != NULL)
    {
        field->real_name = strdup(real_name);
        if (field->real_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            coda_ascbin_field_delete(field);
            return NULL;
        }
    }

    return field;
}

int coda_ascbin_field_set_type(coda_ascbin_field *field, coda_ascbin_type *type)
{
    assert(type != NULL);
    if (field->type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a type");
        return -1;
    }
    field->type = type;
    type->retain_count++;
    return 0;
}

int coda_ascbin_field_set_hidden(coda_ascbin_field *field)
{
    field->hidden = 1;
    return 0;
}

int coda_ascbin_field_set_available_expression(coda_ascbin_field *field, coda_expression *available_expr)
{
    if (field->available_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has an available expression");
        return -1;
    }
    field->available_expr = available_expr;
    return 0;
}

int coda_ascbin_field_set_bit_offset_expression(coda_ascbin_field *field, coda_expression *bit_offset_expr)
{
    if (field->bit_offset_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a bit offset expression");
        return -1;
    }
    field->bit_offset_expr = bit_offset_expr;
    return 0;
}

int coda_ascbin_field_validate(coda_ascbin_field *field)
{
    if (field->type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing type for field definition");
        return -1;
    }
    return 0;
}

coda_dynamic_type *coda_ascbin_empty_record(void)
{
    if (empty_record_singleton == NULL)
    {
        empty_record_singleton = coda_ascbin_record_new(coda_format_binary);
        assert(empty_record_singleton != NULL);
    }

    return (coda_dynamic_type *)empty_record_singleton;
}


coda_ascbin_record *coda_ascbin_record_new(coda_format format)
{
    coda_ascbin_record *record;

    if (format != coda_format_ascii && format != coda_format_binary)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid format for ascii/binary record");
        return NULL;
    }

    record = (coda_ascbin_record *)malloc(sizeof(coda_ascbin_record));
    if (record == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_record), __FILE__, __LINE__);
        return NULL;
    }
    record->retain_count = 0;
    record->format = format;
    record->type_class = coda_record_class;
    record->name = NULL;
    record->description = NULL;
    record->tag = tag_ascbin_record;
    record->bit_size = 0;
    record->hash_data = NULL;
    record->fast_size_expr = NULL;
    record->num_fields = 0;
    record->field = NULL;
    record->has_hidden_fields = 0;
    record->has_available_expr_fields = 0;

    record->hash_data = hashtable_new(0);
    if (record->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        coda_ascbin_record_delete(record);
        return NULL;
    }

    return record;
}

int coda_ascbin_record_set_fast_size_expression(coda_ascbin_record *record, coda_expression *fast_size_expr)
{
    if (record->fast_size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "record already has a fast size expression");
        return -1;
    }
    record->fast_size_expr = fast_size_expr;
    return 0;
}

int coda_ascbin_record_add_field(coda_ascbin_record *record, coda_ascbin_field *field)
{
    if (record->num_fields % BLOCK_SIZE == 0)
    {
        coda_ascbin_field **new_field;

        new_field = realloc(record->field, (record->num_fields + BLOCK_SIZE) * sizeof(coda_ascbin_field *));
        if (new_field == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (record->num_fields + BLOCK_SIZE) * sizeof(coda_ascbin_field *), __FILE__, __LINE__);
            return -1;
        }
        record->field = new_field;
    }
    if (hashtable_add_name(record->hash_data, field->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate field with name %s for record definition", field->name);
        return -1;
    }
    record->num_fields++;
    record->field[record->num_fields - 1] = field;

    /* set bit_offset */
    if (field->bit_offset_expr != NULL)
    {
        field->bit_offset = -1;
    }
    else if (record->num_fields == 1)
    {
        field->bit_offset = 0;
    }
    else if (record->field[record->num_fields - 2]->bit_offset >= 0 &&
             record->field[record->num_fields - 2]->type->bit_size >= 0 &&
             record->field[record->num_fields - 2]->available_expr == NULL)
    {
        field->bit_offset = record->field[record->num_fields - 2]->bit_offset +
            record->field[record->num_fields - 2]->type->bit_size;
    }
    else
    {
        field->bit_offset = -1;
    }

    /* update bit_size */
    if (record->bit_size >= 0)
    {
        if (field->type->bit_size >= 0 && record->field[record->num_fields - 1]->available_expr == NULL)
        {
            record->bit_size += field->type->bit_size;
        }
        else
        {
            record->bit_size = -1;
        }
    }

    return 0;
}

coda_ascbin_union *coda_ascbin_union_new(coda_format format)
{
    coda_ascbin_union *dd_union;

    if (format != coda_format_ascii && format != coda_format_binary)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid format for ascii/binary union");
        return NULL;
    }

    dd_union = (coda_ascbin_union *)malloc(sizeof(coda_ascbin_union));
    if (dd_union == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_union), __FILE__, __LINE__);
        return NULL;
    }
    dd_union->retain_count = 0;
    dd_union->format = format;
    dd_union->type_class = coda_record_class;
    dd_union->name = NULL;
    dd_union->description = NULL;
    dd_union->tag = tag_ascbin_union;
    dd_union->bit_size = 0;
    dd_union->fast_size_expr = NULL;
    dd_union->hash_data = NULL;
    dd_union->num_fields = 0;
    dd_union->field = NULL;
    dd_union->has_hidden_fields = 0;
    dd_union->has_available_expr_fields = 1;
    dd_union->field_expr = NULL;

    dd_union->hash_data = hashtable_new(0);
    if (dd_union->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        free(dd_union);
        return NULL;
    }

    return dd_union;
}

int coda_ascbin_union_set_fast_size_expression(coda_ascbin_union *dd_union, coda_expression *fast_size_expr)
{
    if (dd_union->fast_size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "union already has a fast size expression");
        return -1;
    }
    dd_union->fast_size_expr = fast_size_expr;
    return 0;
}

int coda_ascbin_union_set_field_expression(coda_ascbin_union *dd_union, coda_expression *field_expr)
{
    assert(field_expr != NULL);
    if (dd_union->field_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "union already has a field expression");
        return -1;
    }
    dd_union->field_expr = field_expr;
    return 0;
}

int coda_ascbin_union_add_field(coda_ascbin_union *dd_union, coda_ascbin_field *field)
{
    if (dd_union->num_fields % BLOCK_SIZE == 0)
    {
        coda_ascbin_field **new_field;

        new_field = realloc(dd_union->field, (dd_union->num_fields + BLOCK_SIZE) * sizeof(coda_ascbin_field *));
        if (new_field == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (dd_union->num_fields + BLOCK_SIZE) * sizeof(coda_ascbin_field *), __FILE__, __LINE__);
            return -1;
        }
        dd_union->field = new_field;
    }
    if (hashtable_add_name(dd_union->hash_data, field->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate field with name %s for union definition", field->name);
        return -1;
    }
    dd_union->num_fields++;
    dd_union->field[dd_union->num_fields - 1] = field;

    /* update bit_size */
    if (dd_union->num_fields == 1)
    {
        dd_union->bit_size = field->type->bit_size;
    }
    else if (field->type->bit_size != dd_union->bit_size)
    {
        dd_union->bit_size = -1;
    }

    return 0;
}

int coda_ascbin_union_validate(coda_ascbin_union *dd_union)
{
    if (dd_union->field_expr == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing field expression for union definition");
        return -1;
    }
    if (dd_union->num_fields == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "union definition has no fields");
        return -1;
    }
    return 0;
}

coda_ascbin_array *coda_ascbin_array_new(coda_format format)
{
    coda_ascbin_array *array;

    if (format != coda_format_ascii && format != coda_format_binary)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid format for ascii/binary array");
        return NULL;
    }

    array = (coda_ascbin_array *)malloc(sizeof(coda_ascbin_array));
    if (array == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_array), __FILE__, __LINE__);
        return NULL;
    }
    array->retain_count = 0;
    array->format = format;
    array->type_class = coda_array_class;
    array->name = NULL;
    array->description = NULL;
    array->tag = tag_ascbin_array;
    array->bit_size = -1;
    array->base_type = NULL;
    array->num_elements = 0;
    array->num_dims = 0;
    array->dim = NULL;
    array->dim_expr = NULL;

    return array;
}

int coda_ascbin_array_set_base_type(coda_ascbin_array *array, coda_ascbin_type *base_type)
{
    if (array->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "array already has a base type");
        return -1;
    }
    array->base_type = base_type;
    base_type->retain_count++;

    /* update bit_size */
    if (array->num_elements != -1)
    {
        if (base_type->bit_size == -1)
        {
            array->bit_size = -1;       /* determine dynamically at run-time */
        }
        else
        {
            array->bit_size = array->num_elements * base_type->bit_size;
        }
    }

    return 0;
}

static int array_add_dimension(coda_ascbin_array *array, long dim, coda_expression *dim_expr)
{
    if (array->num_dims == CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "maximum number of dimensions (%d) exceeded for array definition",
                       CODA_MAX_NUM_DIMS);
        return -1;
    }
    if (array->num_dims == 0)
    {
        array->dim = (long *)malloc(sizeof(long));
        if (array->dim == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           sizeof(long), __FILE__, __LINE__);
            return -1;
        }
        array->dim_expr = (coda_expression **)malloc(sizeof(coda_expression *));
        if (array->dim_expr == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           sizeof(long), __FILE__, __LINE__);
            return -1;
        }
    }
    else
    {
        long *new_dim;
        coda_expression **new_dim_expr;

        new_dim = (long *)realloc(array->dim, (array->num_dims + 1) * sizeof(long));
        if (new_dim == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (array->num_dims + 1) * sizeof(long), __FILE__, __LINE__);
            return -1;
        }
        array->dim = new_dim;
        new_dim_expr = (coda_expression **)realloc(array->dim_expr, (array->num_dims + 1) * sizeof(coda_expression *));
        if (new_dim_expr == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (array->num_dims + 1) * sizeof(coda_expression *), __FILE__, __LINE__);
            return -1;
        }
        array->dim_expr = new_dim_expr;
    }
    array->dim[array->num_dims] = dim;
    array->dim_expr[array->num_dims] = dim_expr;
    array->num_dims++;

    /* update num_elements */
    if (array->num_elements != -1)
    {
        if (dim_expr != NULL)
        {
            array->num_elements = -1;
        }
        else if (array->num_dims == 1)
        {
            array->num_elements = dim;
        }
        else
        {
            array->num_elements *= dim;
        }

        /* update bit_size */
        if (array->num_elements == -1)
        {
            array->bit_size = -1;
        }
        else if (array->base_type != NULL)
        {
            if (array->base_type->bit_size == -1)
            {
                array->bit_size = -1;   /* determine dynamically at run-time */
            }
            else
            {
                array->bit_size = array->num_elements * array->base_type->bit_size;
            }
        }
    }

    return 0;
}

int coda_ascbin_array_add_fixed_dimension(coda_ascbin_array *array, long dim)
{
    return array_add_dimension(array, dim, NULL);
}

int coda_ascbin_array_add_variable_dimension(coda_ascbin_array *array, coda_expression *dim_expr)
{
    return array_add_dimension(array, -1, dim_expr);
}

int coda_ascbin_array_validate(coda_ascbin_array *array)
{
    if (array->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing base type for array definition");
        return -1;
    }
    if (array->num_dims == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "number of dimensions is 0 for array definition");
        return -1;
    }
    return 0;
}

static void delete_detection_node(coda_ascbin_detection_node *node)
{
    int i;

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

static coda_ascbin_detection_node *detection_node_new(void)
{
    coda_ascbin_detection_node *node;

    node = malloc(sizeof(coda_ascbin_detection_node));
    if (node == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascbin_detection_node), __FILE__, __LINE__);
        return NULL;
    }
    node->entry = NULL;
    node->rule = NULL;
    node->num_subnodes = 0;
    node->subnode = NULL;

    return node;
}

static int detection_node_add_node(coda_ascbin_detection_node *node, coda_ascbin_detection_node *new_node)
{
    coda_detection_rule_entry *new_entry;
    int i;

    if (node->num_subnodes % BLOCK_SIZE == 0)
    {
        coda_ascbin_detection_node **new_subnode;

        new_subnode = realloc(node->subnode, (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_ascbin_detection_node *));
        if (new_subnode == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (node->num_subnodes + BLOCK_SIZE) * sizeof(coda_ascbin_detection_node *), __FILE__,
                           __LINE__);
            return -1;
        }
        node->subnode = new_subnode;
    }

    new_entry = new_node->entry;
    node->num_subnodes++;
    for (i = node->num_subnodes - 1; i > 0; i--)
    {
        coda_detection_rule_entry *entry;

        /* check if subnode[i - 1] should be after new_node */
        entry = node->subnode[i - 1]->entry;
        if (new_entry->use_filename && !entry->use_filename)
        {
            /* filename checks go after path/data checks */
            node->subnode[i] = new_node;
            break;
        }
        if (entry->use_filename && !new_entry->use_filename)
        {
            /* filename checks go after path/data checks */
            node->subnode[i] = node->subnode[i - 1];
        }
        else
        {
            if (new_entry->value_length != 0)
            {
                if (entry->value_length != 0)
                {
                    if (entry->offset == -1)
                    {
                        /* value checks with offset go after value checks without offset */
                        node->subnode[i] = new_node;
                        break;
                    }
                    else
                    {
                        /* value checks with shorter values go after value checks with longer values */
                        if (entry->value_length < new_entry->value_length)
                        {
                            node->subnode[i] = node->subnode[i - 1];
                        }
                        else
                        {
                            node->subnode[i] = new_node;
                            break;
                        }
                    }
                }
                else
                {
                    /* size checks go after value checks */
                    node->subnode[i] = node->subnode[i - 1];
                }
            }
            else
            {
                if (entry->value_length != 0)
                {
                    /* size checks go after value checks */
                    node->subnode[i] = new_node;
                    break;
                }
                else
                {
                    /* size checks can go in any order */
                    node->subnode[i] = new_node;
                    break;
                }
            }
        }
    }
    if (i == 0)
    {
        node->subnode[0] = new_node;
    }

    return 0;
}

static coda_ascbin_detection_node *get_node_for_entry(coda_ascbin_detection_node *node,
                                                      coda_detection_rule_entry *entry)
{
    coda_ascbin_detection_node *new_node;
    int i;

    for (i = 0; i < node->num_subnodes; i++)
    {
        coda_detection_rule_entry *current_entry;

        /* check if entries are equal */
        current_entry = node->subnode[i]->entry;
        if (entry->use_filename == current_entry->use_filename && entry->offset == current_entry->offset &&
            entry->value_length == current_entry->value_length)
        {
            if (entry->value_length > 0)
            {
                if (memcmp(entry->value, current_entry->value, entry->value_length) == 0)
                {
                    /* same entry -> use this subnode */
                    return node->subnode[i];
                }
            }
            else
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
    new_node->entry = entry;
    if (detection_node_add_node(node, new_node) != 0)
    {
        delete_detection_node(new_node);
        return NULL;
    }

    return new_node;
}

void coda_ascbin_detection_tree_delete(void *detection_tree)
{
    delete_detection_node((coda_ascbin_detection_node *)detection_tree);
}

int coda_ascbin_detection_tree_add_rule(void *detection_tree, coda_detection_rule *detection_rule)
{
    coda_ascbin_detection_node *node;
    int i;

    if (detection_rule->num_entries == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule for '%s' should have at least one entry",
                       detection_rule->product_definition->name);
        return -1;
    }
    for (i = 0; i < detection_rule->num_entries; i++)
    {
        if (detection_rule->entry[i]->path != NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule %d for '%s' can not be based on paths", i,
                           detection_rule->product_definition->name);
            return -1;
        }
        if (detection_rule->entry[i]->offset == -1 && detection_rule->entry[0]->value_length == 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "detection rule %d for '%s' has an empty entry", i,
                           detection_rule->product_definition->name);
            return -1;
        }
    }

    node = *(coda_ascbin_detection_node **)detection_tree;
    if (node == NULL)
    {
        node = detection_node_new();
        if (node == NULL)
        {
            return -1;
        }
        *(coda_ascbin_detection_node **)detection_tree = node;
    }
    for (i = 0; i < detection_rule->num_entries; i++)
    {
        node = get_node_for_entry(node, detection_rule->entry[i]);
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

void coda_ascbin_done(void)
{
    if (empty_record_singleton != NULL)
    {
        coda_ascbin_record_delete(empty_record_singleton);
        empty_record_singleton = NULL;
    }
}
