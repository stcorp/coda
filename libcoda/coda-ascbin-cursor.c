/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-bin.h"
#include "coda-definition.h"
#include "coda-ascbin-definition.h"

/* replace current type if necessary: use 'no-data' type for missing fields */
static int eval_current_type(coda_Cursor *cursor)
{
    /* check whether this is a missing field */
    if (cursor->n > 1 && (cursor->stack[cursor->n - 2].type->format == coda_format_ascii ||
                          cursor->stack[cursor->n - 2].type->format == coda_format_binary))
    {
        switch (((coda_ascbinType *)cursor->stack[cursor->n - 2].type)->tag)
        {
            case tag_ascbin_record:
                {
                    coda_ascbinField *field;
                    long index;

                    index = cursor->stack[cursor->n - 1].index;
                    /* if index == -1 we are pointing to the attributes of the record */
                    if (index != -1)
                    {
                        field = ((coda_ascbinRecord *)cursor->stack[cursor->n - 2].type)->field[index];
                        if (field->available_expr != NULL)
                        {
                            coda_Cursor record_cursor;
                            int available;

                            record_cursor = *cursor;
                            record_cursor.n--;
                            if (coda_expr_eval_bool(field->available_expr, &record_cursor, &available) != 0)
                            {
                                return -1;
                            }
                            if (!available)
                            {
                                cursor->stack[cursor->n - 1].type = coda_bin_no_data();
                                return 0;
                            }
                        }
                    }
                }
                break;
            case tag_ascbin_union:
                {
                    coda_Cursor union_cursor;
                    long available_index;
                    long index;

                    union_cursor = *cursor;
                    union_cursor.n--;

                    index = cursor->stack[cursor->n - 1].index;
                    /* if index == -1 we are pointing to the attributes of the union */
                    if (index != -1)
                    {
                        if (coda_ascbin_cursor_get_available_union_field_index(&union_cursor, &available_index) != 0)
                        {
                            return -1;
                        }
                        if (index != available_index)
                        {
                            cursor->stack[cursor->n - 1].type = coda_bin_no_data();
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
    if (coda_option_bypass_special_types && cursor->stack[cursor->n - 1].type->type_class == coda_special_class)
    {
        if (coda_cursor_use_base_type_of_special_type(cursor) != 0)
        {
            return -1;
        }
    }

    return 0;
}

/* cursor should point to record for this function */
static int get_relative_field_bit_offset_by_index(const coda_Cursor *cursor, long field_index, int64_t *rel_bit_offset)
{
    coda_ascbinField *field;
    coda_ascbinRecord *record;
    coda_Cursor field_cursor;
    int64_t prev_bit_offset;
    long index;
    long i;

    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 1].type;
    field = record->field[field_index];

    if (field->bit_offset >= 0)
    {
        /* use static offset */
        *rel_bit_offset = field->bit_offset;
        return 0;
    }

    if (field->bit_offset_expr != NULL)
    {
        /* determine offset using expr */
        return coda_expr_eval_integer(field->bit_offset_expr, cursor, rel_bit_offset);
    }

    assert(field_index != 0);   /* the first field should either have a fixed bit offset or a bit_offset_expr */

    /* previous methods did not work, determine offset by:
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

        field_cursor.stack[field_cursor.n - 1].type = (coda_DynamicType *)record->field[i]->type;
        field_cursor.stack[field_cursor.n - 1].index = i;
        if (eval_current_type(&field_cursor) != 0)
        {
            return -1;
        }
        if (coda_cursor_get_bit_size(&field_cursor, &bit_size) != 0)
        {
            return -1;
        }
        prev_bit_offset += bit_size;
        field_cursor.stack[field_cursor.n - 1].bit_offset += bit_size;
    }
    *rel_bit_offset = prev_bit_offset;

    return 0;
}

/* cursor should point to record field for this function */
static int get_next_relative_field_bit_offset(const coda_Cursor *cursor, int64_t *rel_bit_offset,
                                              int64_t *current_field_size)
{
    coda_ascbinField *field;
    coda_ascbinRecord *record;
    int64_t prev_bit_offset;
    int64_t bit_size;
    int field_index;

    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 2].type;
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

    if (field->bit_offset_expr != NULL)
    {
        coda_Cursor record_cursor;

        /* determine offset using expr */
        record_cursor = *cursor;
        record_cursor.n--;
        if (current_field_size != NULL)
        {
            *current_field_size = -1;   /* not calculated */
        }
        return coda_expr_eval_integer(field->bit_offset_expr, &record_cursor, rel_bit_offset);
    }

    /* previous methods did not work, determine offset by using bit_offset of current field and adding its bit size */
    prev_bit_offset = cursor->stack[cursor->n - 1].bit_offset - cursor->stack[cursor->n - 2].bit_offset;
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

int coda_ascbin_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    cursor->pf = pf;
    cursor->n = 1;
    cursor->stack[0].type = pf->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = 0;

    return eval_current_type(cursor);
}

int coda_ascbin_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    coda_ascbinRecord *record;
    int64_t rel_bit_offset;

    if (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag == tag_ascbin_union)
    {
        /* also allow union traversal with record functions */
        return coda_ascbin_cursor_goto_union_field_by_index(cursor, index);
    }

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag == tag_ascbin_record);
    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 1].type;

    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    if (get_relative_field_bit_offset_by_index(cursor, index, &rel_bit_offset) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)record->field[index]->type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset + rel_bit_offset;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    coda_ascbinRecord *record;
    int64_t rel_bit_offset;
    long index;

    if (((coda_ascbinType *)cursor->stack[cursor->n - 2].type)->tag == tag_ascbin_union)
    {
        /* also allow union traversal with record functions */
        return coda_ascbin_cursor_goto_next_union_field(cursor);
    }

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 2].type)->tag == tag_ascbin_record);
    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 2].type;

    index = cursor->stack[cursor->n - 1].index + 1;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    if (get_next_relative_field_bit_offset(cursor, &rel_bit_offset, NULL) != 0)
    {
        return -1;
    }

    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)record->field[index]->type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset + rel_bit_offset;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_available_union_field(coda_Cursor *cursor)
{
    long index;

    if (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag != tag_ascbin_union)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a union");
        return -1;
    }

    if (coda_ascbin_cursor_get_available_union_field_index(cursor, &index) != 0)
    {
        return -1;
    }
    return coda_ascbin_cursor_goto_union_field_by_index(cursor, index);
}

int coda_ascbin_cursor_goto_union_field_by_index(coda_Cursor *cursor, long index)
{
    coda_ascbinUnion *dd_union;

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag == tag_ascbin_union);
    dd_union = (coda_ascbinUnion *)cursor->stack[cursor->n - 1].type;

    if (index < 0 || index >= dd_union->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       dd_union->num_fields, __FILE__, __LINE__);
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)dd_union->field[index]->type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = cursor->stack[cursor->n - 2].bit_offset;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_next_union_field(coda_Cursor *cursor)
{
    coda_ascbinUnion *dd_union;
    long index;

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 2].type)->tag == tag_ascbin_union);
    dd_union = (coda_ascbinUnion *)cursor->stack[cursor->n - 2].type;

    index = cursor->stack[cursor->n - 1].index + 1;
    if (index < 0 || index >= dd_union->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       dd_union->num_fields, __FILE__, __LINE__);
        return -1;
    }

    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)dd_union->field[index]->type;
    cursor->stack[cursor->n - 1].index = index;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    coda_ascbinArray *array;
    long offset_elements;
    long i;

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag = tag_ascbin_array);
    array = (coda_ascbinArray *)cursor->stack[cursor->n - 1].type;

    /* check the number of dimensions */
    if (num_subs != array->num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d) (%s:%u)",
                       num_subs, array->num_dims, __FILE__, __LINE__);
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

            if (coda_expr_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
            {
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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           subs[i], dim, __FILE__, __LINE__);
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

            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)array->base_type;
            cursor->stack[cursor->n - 1].index = i;
            if (eval_current_type(cursor) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
            {
                cursor->n--;
                return -1;
            }
            cursor->stack[cursor->n - 1].bit_offset += bit_size;
        }
    }
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)array->base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    coda_ascbinArray *array;
    long i;

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag = tag_ascbin_array);
    array = (coda_ascbinArray *)cursor->stack[cursor->n - 1].type;

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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
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

            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)array->base_type;
            cursor->stack[cursor->n - 1].index = i;
            if (eval_current_type(cursor) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
            {
                cursor->n--;
                return -1;
            }
            cursor->stack[cursor->n - 1].bit_offset += bit_size;
        }
    }
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)array->base_type;
    cursor->stack[cursor->n - 1].index = index;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_next_array_element(coda_Cursor *cursor)
{
    coda_ascbinArray *array;
    int64_t bit_size;
    long index;

    assert(((coda_ascbinType *)cursor->stack[cursor->n - 2].type)->tag = tag_ascbin_array);
    array = (coda_ascbinArray *)cursor->stack[cursor->n - 2].type;

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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           index, num_elements, __FILE__, __LINE__);
            return -1;
        }
    }

    if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
    {
        return -1;
    }
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)array->base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset += bit_size;
    if (eval_current_type(cursor) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_ascbin_cursor_goto_attributes(coda_Cursor *cursor)
{
    cursor->n++;
    cursor->stack[cursor->n - 1].type = coda_ascbin_empty_record();
    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* virtual types do not have a bit offset */

    return eval_current_type(cursor);
}

int coda_ascbin_cursor_get_bit_size(const coda_Cursor *cursor, int64_t *bit_size)
{
    if (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->bit_size >= 0)
    {
        *bit_size = ((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->bit_size;
    }
    else
    {
        switch (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag)
        {
            case tag_ascbin_record:
                {
                    coda_ascbinRecord *record;

                    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 1].type;
                    if (coda_option_use_fast_size_expressions && record->fast_size_expr != NULL)
                    {
                        if (coda_expr_eval_integer(record->fast_size_expr, cursor, bit_size) != 0)
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
                            coda_Cursor field_cursor;
                            long i;

                            field_cursor = *cursor;
                            if (coda_ascbin_cursor_goto_record_field_by_index(&field_cursor, 0) != 0)
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
                                    if (get_next_relative_field_bit_offset
                                        (&field_cursor, &rel_bit_offset, &field_bit_size) != 0)
                                    {
                                        return -1;
                                    }
                                }
                                if (field_bit_size == -1)
                                {
                                    if (coda_cursor_get_bit_size(&field_cursor, &field_bit_size) != 0)
                                    {
                                        return -1;
                                    }
                                }
                                record_bit_size += field_bit_size;
                                if (i < record->num_fields - 1)
                                {
                                    field_cursor.stack[field_cursor.n - 1].type =
                                        (coda_DynamicType *)record->field[i + 1]->type;
                                    field_cursor.stack[field_cursor.n - 1].index = i + 1;
                                    field_cursor.stack[field_cursor.n - 1].bit_offset =
                                        cursor->stack[cursor->n - 1].bit_offset + rel_bit_offset;
                                    if (eval_current_type(&field_cursor) != 0)
                                    {
                                        return -1;
                                    }
                                }
                            }
                        }
                        *bit_size = record_bit_size;
                    }
                }
                break;
            case tag_ascbin_union:
                {
                    coda_ascbinUnion *dd_union;

                    dd_union = (coda_ascbinUnion *)cursor->stack[cursor->n - 1].type;
                    if (coda_option_use_fast_size_expressions && dd_union->fast_size_expr != NULL)
                    {
                        if (coda_expr_eval_integer(dd_union->fast_size_expr, cursor, bit_size) != 0)
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        coda_Cursor field_cursor;

                        field_cursor = *cursor;
                        if (coda_ascbin_cursor_goto_available_union_field(&field_cursor) != 0)
                        {
                            return -1;
                        }
                        if (coda_cursor_get_bit_size(&field_cursor, bit_size) != 0)
                        {
                            return -1;
                        }
                    }
                }
                break;
            case tag_ascbin_array:
                {
                    coda_ascbinArray *array;
                    long num_elements;

                    array = (coda_ascbinArray *)cursor->stack[cursor->n - 1].type;

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
                        coda_Cursor array_cursor;
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

                            array_cursor.stack[array_cursor.n - 1].type = (coda_DynamicType *)array->base_type;
                            array_cursor.stack[array_cursor.n - 1].index = i;
                            if (eval_current_type(&array_cursor) != 0)
                            {
                                return -1;
                            }
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
        }
    }

    return 0;
}

int coda_ascbin_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    switch (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascbin_record:
        case tag_ascbin_union:
            *num_elements = ((coda_ascbinRecord *)cursor->stack[cursor->n - 1].type)->num_fields;
            break;
        case tag_ascbin_array:
            {
                coda_ascbinArray *array;

                array = (coda_ascbinArray *)cursor->stack[cursor->n - 1].type;
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

                            if (coda_expr_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
                            {
                                return -1;
                            }
                            if (var_dim < 0)
                            {
                                coda_set_error(CODA_ERROR_PRODUCT, "product error detected in %s (invalid array size - "
                                               "calculated array size = %ld)", cursor->pf->filename, (long)var_dim);
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
    }

    return 0;
}

int coda_ascbin_cursor_get_record_field_available_status(const coda_Cursor *cursor, long index, int *available)
{
    coda_ascbinRecord *record;

    assert(cursor->stack[cursor->n - 1].type->type_class == coda_record_class);
    record = (coda_ascbinRecord *)cursor->stack[cursor->n - 1].type;

    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    if (record->tag == tag_ascbin_union)
    {
        long available_index;

        if (coda_ascbin_cursor_get_available_union_field_index(cursor, &available_index) != 0)
        {
            return -1;
        }
        *available = (index == available_index);
    }
    else if (record->field[index]->available_expr != NULL)
    {
        if (coda_expr_eval_bool(record->field[index]->available_expr, cursor, available) != 0)
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

int coda_ascbin_cursor_get_available_union_field_index(const coda_Cursor *cursor, long *index)
{
    coda_ascbinUnion *dd_union;
    coda_Cursor union_cursor;
    int64_t index64;

    if (((coda_ascbinType *)cursor->stack[cursor->n - 1].type)->tag != tag_ascbin_union)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a union");
        return -1;
    }
    dd_union = (coda_ascbinUnion *)cursor->stack[cursor->n - 1].type;
    assert(dd_union->num_fields > 0);

    /* find field using the type of the first union field to evaluate the expression */
    union_cursor = *cursor;
    union_cursor.n++;
    union_cursor.stack[union_cursor.n - 1].type = (coda_DynamicType *)dd_union->field[0]->type;
    union_cursor.stack[union_cursor.n - 1].index = -1;
    union_cursor.stack[union_cursor.n - 1].bit_offset = union_cursor.stack[union_cursor.n - 2].bit_offset;
    if (coda_expr_eval_integer(dd_union->field_expr, &union_cursor, &index64) != 0)
    {
        return -1;
    }
    if (index64 < 0 || index64 >= dd_union->num_fields)
    {
        coda_set_error(CODA_ERROR_PRODUCT,
                       "possible product error detected in %s (invalid result (%ld) from union field expression - num "
                       "fields = %ld - byte:bit offset = %ld:%ld)", cursor->pf->filename, (long)index64,
                       dd_union->num_fields, (long)(cursor->stack[cursor->n - 1].bit_offset >> 3),
                       (long)(cursor->stack[cursor->n - 1].bit_offset & 0x7));
        return -1;
    }
    *index = (int)index64;

    return 0;
}

int coda_ascbin_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    coda_ascbinArray *array;
    int i;

    array = (coda_ascbinArray *)cursor->stack[cursor->n - 1].type;
    *num_dims = array->num_dims;
    for (i = 0; i < array->num_dims; i++)
    {
        if (array->dim[i] == -1)
        {
            int64_t var_dim;

            if (coda_expr_eval_integer(array->dim_expr[i], cursor, &var_dim) != 0)
            {
                return -1;
            }
            if (var_dim < 0)
            {
                coda_set_error(CODA_ERROR_PRODUCT, "product error detected in %s (invalid array size (%ld))",
                               cursor->pf->filename, (long)var_dim);
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

/** @} */
