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

#include "coda-ascbin.h"
#include "coda-definition.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* cursor should point to record for this function */
static int get_relative_field_bit_offset_by_index(const coda_cursor *cursor, long field_index, int64_t *rel_bit_offset)
{
    coda_type_record_field *field;
    coda_type_record *record;
    coda_cursor field_cursor;
    int64_t prev_bit_offset;
    long index;
    long i;

    record = (coda_type_record *)cursor->stack[cursor->n - 1].type;
    field = record->field[field_index];

    if (field->bit_offset >= 0)
    {
        /* use static offset */
        *rel_bit_offset = field->bit_offset;
        return 0;
    }

    if (field->bit_offset_expr != NULL)
    {
        if (field->available_expr != NULL)
        {
            int available;

            if (coda_expression_eval_bool(field->available_expr, cursor, &available) != 0)
            {
                coda_add_error_message(" for available expression");
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
            /* don't evaluate offset expression if field is not available! */
            if (!available)
            {
                if (field_index == 0)
                {
                    /* Just set to 0. With a proper format definition you should actually never have this case. */
                    *rel_bit_offset = 0;
                    return 0;
                }
                else
                {
                    /* the size of this field is zero, so just use the offset of the previous field */
                    return get_relative_field_bit_offset_by_index(cursor, field_index - 1, rel_bit_offset);
                }
            }
        }
        /* determine offset using expr */
        if (coda_expression_eval_integer(field->bit_offset_expr, cursor, rel_bit_offset) != 0)
        {
            coda_add_error_message(" for offset expression");
            coda_cursor_add_to_error_message(cursor);
            return -1;
        }
        return 0;
    }

    assert(field_index != 0);   /* the first field should either have a fixed bit offset or a bit_offset_expr */

    /* determine offset by:
     *  - finding previous field with a fixed bit_offset or a bit_offset_expr
     *  - calculating the bit offset for that field
     *  - adding bit sizes of fields after that offset until we get to our field
     */
    index = field_index - 1;
    while (record->field[index]->bit_offset == -1 && record->field[index]->bit_offset_expr == NULL)
    {
        index--;
        assert(index >= 0);
    }
    if (get_relative_field_bit_offset_by_index(cursor, index, &prev_bit_offset) != 0)
    {
        return -1;
    }
    field_cursor = *cursor;
    field_cursor.n++;
    field_cursor.stack[field_cursor.n - 1].bit_offset = cursor->stack[cursor->n - 1].bit_offset + prev_bit_offset;
    for (i = index; i < field_index; i++)
    {
        int64_t bit_size;
        int available = 1;

        if (record->field[i]->available_expr != NULL)
        {
            if (coda_expression_eval_bool(record->field[i]->available_expr, cursor, &available) != 0)
            {
                coda_add_error_message(" for available expression");
                return -1;
            }
        }
        if (available)
        {
            field_cursor.stack[field_cursor.n - 1].type = (coda_dynamic_type *)record->field[i]->type;
            field_cursor.stack[field_cursor.n - 1].index = i;
            if (coda_cursor_get_bit_size(&field_cursor, &bit_size) != 0)
            {
                return -1;
            }
            prev_bit_offset += bit_size;
            field_cursor.stack[field_cursor.n - 1].bit_offset += bit_size;
        }
    }
    *rel_bit_offset = prev_bit_offset;

    return 0;
}

/* cursor should point to record field for this function */
static int get_next_relative_field_bit_offset(const coda_cursor *cursor, int64_t *rel_bit_offset,
                                              int64_t *current_field_size)
{
    coda_type_record_field *field;
    coda_type_record *record;
    int64_t prev_bit_offset;
    int64_t bit_size;
    int field_index;

    record = (coda_type_record *)cursor->stack[cursor->n - 2].type;
    field_index = cursor->stack[cursor->n - 1].index + 1;
    assert(field_index < record->num_fields);
    field = record->field[field_index];

    if (field->bit_offset >= 0)
    {
        /* use static offset */
        *rel_bit_offset = field->bit_offset;
        if (current_field_size != NULL)
        {
            *current_field_size = -1;   /* not calculated */
        }
        return 0;
    }

    prev_bit_offset = cursor->stack[cursor->n - 1].bit_offset - cursor->stack[cursor->n - 2].bit_offset;

    if (field->bit_offset_expr != NULL)
    {
        coda_cursor record_cursor;

        record_cursor = *cursor;
        record_cursor.n--;
        if (current_field_size != NULL)
        {
            *current_field_size = -1;   /* not calculated */
        }

        if (field->available_expr != NULL)
        {
            int available;

            if (coda_expression_eval_bool(field->available_expr, &record_cursor, &available) != 0)
            {
                coda_add_error_message(" for available expression");
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
            /* don't evaluate offset expression if field is not available! */
            if (!available)
            {
                /* the size of this field is zero, so just use the offset of the previous field */
                *rel_bit_offset = prev_bit_offset;
                return 0;
            }
        }
        /* determine offset using expr */
        if (coda_expression_eval_integer(field->bit_offset_expr, &record_cursor, rel_bit_offset) != 0)
        {
            coda_add_error_message(" for offset expression");
            coda_cursor_add_to_error_message(cursor);
            return -1;
        }
        return 0;
    }

    /* determine offset by using bit_offset of current field and adding its bit size */
    if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
    {
        return -1;
    }
    *rel_bit_offset = prev_bit_offset + bit_size;
    if (current_field_size != NULL)
    {
        *current_field_size = bit_size;
    }

    return 0;
}

int coda_ascbin_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    assert(product->root_type != NULL);
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = 0;

    return 0;
}

int coda_ascbin_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_type_record *record;
    int64_t bit_offset;
    int available = 1;

    record = (coda_type_record *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                       record->num_fields);
        return -1;
    }

    bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    if (record->union_field_expr != NULL)
    {
        long available_index;

        if (coda_cursor_get_available_union_field_index(cursor, &available_index) != 0)
        {
            return -1;
        }
        if (index != available_index)
        {
            available = 0;
        }
    }
    else
    {
        int64_t rel_bit_offset;

        if (get_relative_field_bit_offset_by_index(cursor, index, &rel_bit_offset) != 0)
        {
            return -1;
        }
        bit_offset += rel_bit_offset;
        if (record->field[index]->available_expr != NULL)
        {
            if (coda_expression_eval_bool(record->field[index]->available_expr, cursor, &available) != 0)
            {
                coda_add_error_message(" for available expression");
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
        }
    }

    cursor->n++;
    if (available)
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)record->field[index]->type;
    }
    else
    {
        cursor->stack[cursor->n - 1].type = coda_no_data_singleton(record->format);
    }
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = bit_offset;

    return 0;
}

int coda_ascbin_cursor_goto_next_record_field(coda_cursor *cursor)
{
    coda_type_record *record;
    int64_t bit_offset;
    int available = 1;
    long index;

    record = (coda_type_record *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 2].type);
    index = cursor->stack[cursor->n - 1].index + 1;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                       record->num_fields);
        return -1;
    }

    bit_offset = cursor->stack[cursor->n - 2].bit_offset;
    if (record->union_field_expr != NULL)
    {
        coda_cursor record_cursor = *cursor;
        long available_index;

        record_cursor.n--;
        if (coda_cursor_get_available_union_field_index(&record_cursor, &available_index) != 0)
        {
            return -1;
        }
        if (index != available_index)
        {
            available = 0;
        }
    }
    else
    {
        int64_t rel_bit_offset;

        if (get_next_relative_field_bit_offset(cursor, &rel_bit_offset, NULL) != 0)
        {
            return -1;
        }
        bit_offset += rel_bit_offset;
        if (record->field[index]->available_expr != NULL)
        {
            coda_cursor record_cursor = *cursor;

            record_cursor.n--;
            if (coda_expression_eval_bool(record->field[index]->available_expr, &record_cursor, &available) != 0)
            {
                coda_add_error_message(" for available expression");
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
        }
    }
    if (available)
    {
        cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)record->field[index]->type;
    }
    else
    {
        cursor->stack[cursor->n - 1].type = coda_no_data_singleton(record->format);
    }
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = bit_offset;

    return 0;
}

int coda_ascbin_cursor_goto_available_union_field(coda_cursor *cursor)
{
    coda_type_record *record;
    long index;

    record = (coda_type_record *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (record->union_field_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a union");
        return -1;
    }

    if (coda_ascbin_cursor_get_available_union_field_index(cursor, &index) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)record->field[index]->type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset;

    return 0;
}

int coda_ascbin_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_type_array *array;
    long offset_elements;
    long i;

    array = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    /* check the number of dimensions */
    if (num_subs != array->num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH, "number of dimensions argument (%d) does not match rank of "
                       " array (%d)", num_subs, array->num_dims);
        return -1;
    }

    /* check the dimensions... */
    offset_elements = 0;
    for (i = 0; i < array->num_dims; i++)
    {
        long dim;

        if (array->dim[i] == -1)
        {
            int64_t var_dim;

            if (coda_expression_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
            {
                coda_add_error_message(" for dim[%d] expression", i);
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
            dim = (long)var_dim;
        }
        else
        {
            dim = array->dim[i];
        }

        if (subs[i] < 0 || subs[i] >= dim)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", subs[i],
                           dim);
            return -1;
        }
        if (i > 0)
        {
            offset_elements *= dim;
        }
        offset_elements += subs[i];
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset;

    if (array->base_type->bit_size >= 0)
    {
        /* if the array base type is simple, do a calculated index calculation. */
        cursor->stack[cursor->n - 1].bit_offset += offset_elements * array->base_type->bit_size;
    }
    else        /* not a simple base type, so walk the elements. */
    {
        for (i = 0; i < offset_elements; i++)
        {
            int64_t bit_size;

            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)array->base_type;
            cursor->stack[cursor->n - 1].index = i;
            if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
            {
                cursor->n--;
                return -1;
            }
            cursor->stack[cursor->n - 1].bit_offset += bit_size;
        }
    }
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)array->base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;

    return 0;
}

int coda_ascbin_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_type_array *array;
    long i;

    array = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }
        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset;

    if (array->base_type->bit_size >= 0)
    {
        /* if the array base type is simple, do a calculated index calculation. */
        cursor->stack[cursor->n - 1].bit_offset += index * array->base_type->bit_size;
    }
    else        /* not a simple base type, so walk the elements. */
    {
        for (i = 0; i < index; i++)
        {
            int64_t bit_size;

            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)array->base_type;
            cursor->stack[cursor->n - 1].index = i;
            if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
            {
                cursor->n--;
                return -1;
            }
            cursor->stack[cursor->n - 1].bit_offset += bit_size;
        }
    }
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)array->base_type;
    cursor->stack[cursor->n - 1].index = index;

    return 0;
}

int coda_ascbin_cursor_goto_next_array_element(coda_cursor *cursor)
{
    coda_type_array *array;
    int64_t bit_size;
    long index;

    array = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 2].type);
    index = cursor->stack[cursor->n - 1].index + 1;

    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        cursor->n--;
        if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            cursor->n++;
            return -1;
        }
        cursor->n++;

        if (index < 0 || index >= num_elements)
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
    {
        return -1;
    }
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)array->base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset += bit_size;

    return 0;
}

int coda_ascbin_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_format format = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type)->format;

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)coda_type_empty_record(format);
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* virtual types do not have a bit offset */

    return 0;
}

int coda_ascbin_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    coda_type_special *type = (coda_type_special *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)type->base_type;

    return 0;
}

int coda_ascbin_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (type->bit_size >= 0)
    {
        *bit_size = type->bit_size;
    }
    else
    {
        switch (type->type_class)
        {
            case coda_record_class:
                {
                    coda_type_record *record = (coda_type_record *)type;

                    if (coda_option_use_fast_size_expressions && record->size_expr != NULL)
                    {
                        if (coda_expression_eval_integer(record->size_expr, cursor, bit_size) != 0)
                        {
                            coda_add_error_message(" for size expression");
                            coda_cursor_add_to_error_message(cursor);
                            return -1;
                        }
                        if (record->bit_size == -8)
                        {
                            /* convert 'byte size' to 'bit size' */
                            *bit_size *= 8;
                        }
                        if (*bit_size < 0)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "calculated size is negative (%ld bits)",
                                           (long)*bit_size);
                            coda_cursor_add_to_error_message(cursor);
                            return -1;
                        }
                    }
                    else if (record->union_field_expr != NULL)
                    {
                        coda_cursor field_cursor;

                        field_cursor = *cursor;
                        if (coda_cursor_goto_available_union_field(&field_cursor) != 0)
                        {
                            return -1;
                        }
                        if (coda_cursor_get_bit_size(&field_cursor, bit_size) != 0)
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        int64_t record_bit_size;

                        record_bit_size = 0;
                        if (record->num_fields > 0)
                        {
                            coda_cursor field_cursor;
                            long i;

                            field_cursor = *cursor;
                            if (coda_cursor_goto_first_record_field(&field_cursor) != 0)
                            {
                                return -1;
                            }
                            for (i = 0; i < record->num_fields; i++)
                            {
                                int64_t rel_bit_offset;
                                int64_t field_bit_size;

                                field_bit_size = -1;
                                if (i < record->num_fields - 1)
                                {
                                    if (get_next_relative_field_bit_offset(&field_cursor, &rel_bit_offset,
                                                                           &field_bit_size) != 0)
                                    {
                                        return -1;
                                    }
                                }
                                if (field_bit_size < 0)
                                {
                                    if (coda_cursor_get_bit_size(&field_cursor, &field_bit_size) != 0)
                                    {
                                        return -1;
                                    }
                                }
                                record_bit_size += field_bit_size;
                                if (i < record->num_fields - 1)
                                {
                                    int available = 1;

                                    if (record->field[i + 1]->available_expr != NULL)
                                    {
                                        if (coda_expression_eval_bool(record->field[i + 1]->available_expr, cursor,
                                                                      &available) != 0)
                                        {
                                            return -1;
                                        }
                                    }
                                    if (available)
                                    {
                                        field_cursor.stack[field_cursor.n - 1].type =
                                            (coda_dynamic_type *)record->field[i + 1]->type;
                                    }
                                    else
                                    {
                                        field_cursor.stack[field_cursor.n - 1].type =
                                            coda_no_data_singleton(record->format);
                                    }
                                    field_cursor.stack[field_cursor.n - 1].index = i + 1;
                                    field_cursor.stack[field_cursor.n - 1].bit_offset =
                                        cursor->stack[cursor->n - 1].bit_offset + rel_bit_offset;
                                }
                            }
                        }
                        *bit_size = record_bit_size;
                    }
                }
                break;
            case coda_array_class:
                {
                    coda_type_array *array = (coda_type_array *)type;
                    long num_elements;

                    /* get the number of elements in array */
                    if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
                    {
                        return -1;
                    }

                    if (num_elements == 0)
                    {
                        /* empty array */
                        *bit_size = 0;
                    }
                    else if (array->base_type->bit_size >= 0)
                    {
                        /* the basetype has a constant size. */
                        *bit_size = num_elements * array->base_type->bit_size;
                    }
                    else
                    {
                        coda_cursor array_cursor;
                        int64_t array_bit_size;
                        long i;

                        /* sum the sizes of the elements 'manually' */
                        array_bit_size = 0;
                        array_cursor = *cursor;
                        array_cursor.n++;
                        array_cursor.stack[array_cursor.n - 1].bit_offset =
                            array_cursor.stack[array_cursor.n - 2].bit_offset;
                        for (i = 0; i < num_elements; i++)
                        {
                            int64_t element_bit_size;

                            array_cursor.stack[array_cursor.n - 1].type = (coda_dynamic_type *)array->base_type;
                            array_cursor.stack[array_cursor.n - 1].index = i;
                            if (coda_cursor_get_bit_size(&array_cursor, &element_bit_size) != 0)
                            {
                                return -1;
                            }
                            array_bit_size += element_bit_size;
                            array_cursor.stack[array_cursor.n - 1].bit_offset += element_bit_size;
                        }
                        *bit_size = array_bit_size;
                    }
                }
                break;
            default:
                assert(0);
                exit(1);
        }
    }

    return 0;
}

int coda_ascbin_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    switch (type->type_class)
    {
        case coda_record_class:
            *num_elements = ((coda_type_record *)type)->num_fields;
            break;
        case coda_array_class:
            {
                coda_type_array *array = (coda_type_array *)type;

                if (array->num_elements != -1)
                {
                    *num_elements = array->num_elements;
                }
                else
                {
                    long n;
                    int i;

                    /* count the number of elements in array */

                    n = 1;
                    for (i = 0; i < array->num_dims; i++)
                    {
                        if (array->dim[i] == -1)
                        {
                            int64_t var_dim;

                            if (coda_expression_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
                            {
                                coda_add_error_message(" for dim[%d] expression", i);
                                coda_cursor_add_to_error_message(cursor);
                                return -1;
                            }
                            if (var_dim < 0)
                            {
                                char s[21];

                                coda_str64(var_dim, s);
                                coda_set_error(CODA_ERROR_PRODUCT, "product error detected (invalid array size - "
                                               "calculated array size = %s)", s);
                                coda_cursor_add_to_error_message(cursor);
                                return -1;
                            }
                            n *= (long)var_dim;
                        }
                        else
                        {
                            n *= array->dim[i];
                        }
                    }
                    *num_elements = n;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_ascbin_cursor_get_record_field_available_status(const coda_cursor *cursor, long index, int *available)
{
    coda_type_record *record = (coda_type_record *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                       record->num_fields);
        return -1;
    }

    if (record->union_field_expr != NULL)
    {
        long available_index;

        if (coda_cursor_get_available_union_field_index(cursor, &available_index) != 0)
        {
            return -1;
        }
        *available = (index == available_index);
    }
    else if (record->field[index]->available_expr != NULL)
    {
        if (coda_expression_eval_bool(record->field[index]->available_expr, cursor, available) != 0)
        {
            return -1;
        }
    }
    else
    {
        *available = 1;
    }

    return 0;
}

int coda_ascbin_cursor_get_available_union_field_index(const coda_cursor *cursor, long *index)
{
    coda_type_record *record = (coda_type_record *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    coda_cursor union_cursor;
    int64_t index64;

    if (record->union_field_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a union");
        return -1;
    }
    assert(record->num_fields > 0);

    /* find field using the type of the first union field to evaluate the expression */
    union_cursor = *cursor;
    union_cursor.n++;
    union_cursor.stack[union_cursor.n - 1].type = (coda_dynamic_type *)record->field[0]->type;
    union_cursor.stack[union_cursor.n - 1].index = -1;
    union_cursor.stack[union_cursor.n - 1].bit_offset = union_cursor.stack[union_cursor.n - 2].bit_offset;
    if (coda_expression_eval_integer(record->union_field_expr, &union_cursor, &index64) != 0)
    {
        coda_add_error_message(" for union field expression");
        coda_cursor_add_to_error_message(cursor);
        return -1;
    }
    if (index64 < 0 || index64 >= record->num_fields)
    {
        char s1[21];
        char s2[21];

        coda_str64(index64, s1);
        coda_str64((cursor->stack[cursor->n - 1].bit_offset >> 3), s2);
        coda_set_error(CODA_ERROR_PRODUCT,
                       "possible product error detected (invalid result (%s) from union field expression - "
                       "num fields = %ld - byte:bit offset = %s:%d)", s1, record->num_fields, s2,
                       (int)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
        coda_cursor_add_to_error_message(cursor);
        return -1;
    }
    *index = (long)index64;

    return 0;
}

int coda_ascbin_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    coda_type_array *array = (coda_type_array *)coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    int i;

    *num_dims = array->num_dims;
    for (i = 0; i < array->num_dims; i++)
    {
        if (array->dim[i] == -1)
        {
            int64_t var_dim;

            if (coda_expression_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
            {
                coda_add_error_message(" for dim[%d] expression", i);
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
            if (var_dim < 0)
            {
                char s[21];

                coda_str64(var_dim, s);
                coda_set_error(CODA_ERROR_PRODUCT, "product error detected (invalid array size (%s))", s);
                coda_cursor_add_to_error_message(cursor);
                return -1;
            }
            dim[i] = (long)var_dim;
        }
        else
        {
            dim[i] = array->dim[i];
        }
    }

    return 0;
}
