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

#include "coda-xml-internal.h"
#include "coda-bin.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int64_t get_bit_offset(coda_DynamicType *type)
{
    switch (((coda_xmlDynamicType *)type)->tag)
    {
        case tag_xml_root_dynamic:
            return 0;
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
            return ((coda_xmlElementDynamicType *)type)->inner_bit_offset;
        default:
            return -1;
    }
}

int coda_xml_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    cursor->pf = pf;
    cursor->n = 1;
    cursor->stack[0].type = pf->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = get_bit_offset(pf->root_type);
    return 0;
}

int coda_xml_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    coda_xmlDynamicType *type;
    coda_DynamicType *field_type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_root_dynamic:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlRootDynamicType *)type)->element;
            break;
        case tag_xml_ascii_type_dynamic:
            {
                int result;
                int depth;

                /* use the ascii base type */
                depth = cursor->n - 1;
                cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                result = coda_ascbin_cursor_goto_record_field_by_index(cursor, index);
                cursor->stack[depth].type = (coda_DynamicType *)type;
                return result;
            }
        case tag_xml_record_dynamic:
            if (index < 0 || index >= ((coda_xmlElementDynamicType *)type)->num_elements)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               ((coda_xmlElementDynamicType *)type)->num_elements, __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->element[index];
            break;
        case tag_xml_attribute_record_dynamic:
            if (index < 0 || index >= ((coda_xmlAttributeRecordDynamicType *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               ((coda_xmlAttributeRecordDynamicType *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlAttributeRecordDynamicType *)type)->attribute[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].index = index;
    if (field_type == NULL)
    {
        cursor->stack[cursor->n - 1].bit_offset = -1;
        cursor->stack[cursor->n - 1].type = coda_bin_no_data();
    }
    else
    {
        cursor->stack[cursor->n - 1].bit_offset = get_bit_offset(field_type);
        cursor->stack[cursor->n - 1].type = field_type;
    }

    return 0;
}

int coda_xml_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    coda_xmlDynamicType *record_type;

    record_type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 2].type;
    if (record_type->tag == tag_xml_ascii_type_dynamic)
    {
        int depth;
        int result;

        /* use the ascii base type */
        depth = cursor->n - 2;
        cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)record_type)->type->ascii_type;
        result = coda_ascbin_cursor_goto_next_record_field(cursor);
        cursor->stack[depth].type = (coda_DynamicType *)record_type;
        return result;

    }

    cursor->n--;
    if (coda_xml_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_xml_cursor_goto_available_union_field(coda_Cursor *cursor)
{
    coda_xmlDynamicType *type;
    int result;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        int depth;

        /* use the ascii base type */
        depth = cursor->n - 1;
        cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        result = coda_ascbin_cursor_goto_available_union_field(cursor);
        cursor->stack[depth].type = (coda_DynamicType *)type;
        return result;
    }

    assert(0);
    exit(1);
}

int coda_xml_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    coda_xmlDynamicType *type;
    int result;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        int depth;

        /* use the ascii base type */
        depth = cursor->n - 1;
        cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        result = coda_ascbin_cursor_goto_array_element(cursor, num_subs, subs);
        cursor->stack[depth].type = (coda_DynamicType *)type;
        return result;
    }

    /* check the number of dimensions */
    if (num_subs != 1)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (1) (%s:%u)", num_subs,
                       __FILE__, __LINE__);
        return -1;
    }

    return coda_xml_cursor_goto_array_element_by_index(cursor, subs[0]);
}

int coda_xml_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    coda_xmlDynamicType *type;
    coda_DynamicType *element_type;
    int result;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        int depth;

        /* use the ascii base type */
        depth = cursor->n - 1;
        cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        result = coda_ascbin_cursor_goto_array_element_by_index(cursor, index);
        cursor->stack[depth].type = (coda_DynamicType *)type;
        return result;
    }

    assert(type->tag == tag_xml_array_dynamic);

    /* check the index */
    if (index < 0 || index >= ((coda_xmlArrayDynamicType *)type)->num_elements)
    {
        coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                       index, ((coda_xmlArrayDynamicType *)type)->num_elements, __FILE__, __LINE__);
        return -1;
    }

    element_type = (coda_DynamicType *)((coda_xmlArrayDynamicType *)type)->element[index];

    cursor->n++;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = get_bit_offset(element_type);
    cursor->stack[cursor->n - 1].type = element_type;

    return 0;
}

int coda_xml_cursor_goto_next_array_element(coda_Cursor *cursor)
{
    coda_xmlDynamicType *array_type;
    int result;

    array_type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 2].type;
    if (array_type->tag == tag_xml_ascii_type_dynamic)
    {
        int depth;

        /* use the ascii base type */
        depth = cursor->n - 2;
        cursor->stack[depth].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)array_type)->type->ascii_type;
        result = coda_ascbin_cursor_goto_next_array_element(cursor);
        cursor->stack[depth].type = (coda_DynamicType *)array_type;
        return result;

    }

    cursor->n--;
    if (coda_xml_cursor_goto_array_element_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }

    return 0;
}

int coda_xml_cursor_goto_attributes(coda_Cursor *cursor)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_xml_array_dynamic:
        case tag_xml_attribute_dynamic:
        case tag_xml_attribute_record_dynamic:
        case tag_xml_root_dynamic:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_xml_empty_dynamic_attribute_record();
            break;
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->attributes;
            break;
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for attributes */

    return 0;
}

int coda_xml_cursor_use_base_type_of_special_type(coda_Cursor *cursor)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;

    assert(type->tag == tag_xml_ascii_type_dynamic);

    /* use the ascii base type */
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
    return coda_ascii_cursor_use_base_type_of_special_type(cursor);
}

int coda_xml_cursor_get_string_length(const coda_Cursor *cursor, long *length)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascii_cursor_get_string_length(&dd_cursor, length,
                                                           ((coda_xmlElementDynamicType *)type)->inner_bit_size);
            }
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
            *length = (long)(((coda_xmlElementDynamicType *)type)->inner_bit_size >> 3);
            break;
        case tag_xml_root_dynamic:
            *length = (long)(cursor->pf->file_size >> 3);
            break;
        case tag_xml_attribute_dynamic:
            *length = strlen(((coda_xmlAttributeDynamicType *)type)->value);
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_cursor_get_bit_size(const coda_Cursor *cursor, int64_t *bit_size)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_root_dynamic:
            *bit_size = cursor->pf->file_size;
            break;
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
            *bit_size = ((coda_xmlElementDynamicType *)type)->inner_bit_size;
            break;
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascii_cursor_get_bit_size(&dd_cursor, bit_size,
                                                      ((coda_xmlElementDynamicType *)type)->inner_bit_size);
            }
            break;
        case tag_xml_attribute_dynamic:
            *bit_size = strlen(((coda_xmlAttributeDynamicType *)cursor->stack[cursor->n - 1].type)->value) * 8;
            break;
        case tag_xml_array_dynamic:
        case tag_xml_attribute_record_dynamic:
            *bit_size = -1;
            break;
    }

    return 0;
}

int coda_xml_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_root_dynamic:
            *num_elements = 1;
            break;
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascii_cursor_get_num_elements(&dd_cursor, num_elements);
            }
        case tag_xml_record_dynamic:
            *num_elements = ((coda_xmlElementDynamicType *)type)->num_elements;
            break;
        case tag_xml_text_dynamic:
            *num_elements = 1;
            break;
        case tag_xml_array_dynamic:
            *num_elements = ((coda_xmlArrayDynamicType *)type)->num_elements;
            break;
        case tag_xml_attribute_record_dynamic:
            *num_elements = ((coda_xmlAttributeRecordDynamicType *)type)->num_attributes;
            break;
        case tag_xml_attribute_dynamic:
            *num_elements = 1;
            break;
    }

    return 0;
}

int coda_xml_cursor_get_record_field_available_status(const coda_Cursor *cursor, long index, int *available)
{
    coda_xmlDynamicType *type;
    coda_DynamicType *field_type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_root_dynamic:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlRootDynamicType *)type)->element;
            break;
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascbin_cursor_get_record_field_available_status(&dd_cursor, index, available);
            }
        case tag_xml_record_dynamic:
            if (index < 0 || index >= ((coda_xmlElementDynamicType *)type)->num_elements)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               ((coda_xmlElementDynamicType *)type)->num_elements, __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->element[index];
            break;
        case tag_xml_attribute_record_dynamic:
            if (index < 0 || index >= ((coda_xmlAttributeRecordDynamicType *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                               ((coda_xmlAttributeRecordDynamicType *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            field_type = (coda_DynamicType *)((coda_xmlAttributeRecordDynamicType *)type)->attribute[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    *available = (field_type != NULL);
    return 0;
}

int coda_xml_cursor_get_available_union_field_index(const coda_Cursor *cursor, long *index)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascbin_cursor_get_available_union_field_index(&dd_cursor, index);
    }

    assert(0);
    exit(1);
}

int coda_xml_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_array_dynamic:
            *num_dims = 1;
            dim[0] = ((coda_xmlArrayDynamicType *)cursor->stack[cursor->n - 1].type)->num_elements;
            break;
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascbin_cursor_get_array_dim(&dd_cursor, num_dims, dim);
            }
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int8(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 data type");
    return -1;
}

int coda_xml_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint8(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 data type");
    return -1;
}

int coda_xml_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int16(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 data type");
    return -1;
}

int coda_xml_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint16(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 data type");
    return -1;
}

int coda_xml_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int32(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 data type");
    return -1;
}

int coda_xml_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint32(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 data type");
    return -1;
}

int coda_xml_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int64(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 data type");
    return -1;
}

int coda_xml_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint64(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 data type");
    return -1;
}

int coda_xml_cursor_read_float(const coda_Cursor *cursor, float *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_float(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float data type");
    return -1;
}

int coda_xml_cursor_read_double(const coda_Cursor *cursor, double *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_double(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
    return -1;
}

int coda_xml_cursor_read_char(const coda_Cursor *cursor, char *dst)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_char(&dd_cursor, dst, ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
    return -1;
}

int coda_xml_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size)
{
    coda_xmlDynamicType *type;
    long read_size;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_xml_ascii_type_dynamic:
            {
                coda_Cursor dd_cursor;

                /* use the ascii base type */
                dd_cursor = *cursor;
                dd_cursor.stack[cursor->n - 1].type =
                    (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
                return coda_ascii_cursor_read_string(&dd_cursor, dst, dst_size,
                                                     ((coda_xmlElementDynamicType *)type)->inner_bit_size);
            }
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
            read_size = (long)(((coda_xmlElementDynamicType *)type)->inner_bit_size >> 3);
            if (read_size + 1 > dst_size)
            {
                read_size = dst_size - 1;
            }
            if (read_size > 0)
            {
                if (coda_ascii_cursor_read_bytes(cursor, (uint8_t *)dst, 0, read_size) != 0)
                {
                    return -1;
                }
                dst[read_size] = '\0';
            }
            else
            {
                dst[0] = '\0';
            }
            break;
        case tag_xml_root_dynamic:
            read_size = (long)(cursor->pf->file_size >> 3);
            if (read_size + 1 > dst_size)
            {
                read_size = dst_size - 1;
            }
            if (read_size > 0)
            {
                if (coda_ascii_cursor_read_bytes(cursor, (uint8_t *)dst, 0, read_size) != 0)
                {
                    return -1;
                }
                dst[read_size] = '\0';
            }
            else
            {
                dst[0] = '\0';
            }
            break;
        case tag_xml_attribute_dynamic:
            read_size = strlen(((coda_xmlAttributeDynamicType *)type)->value);
            if (read_size + 1 > dst_size)
            {
                read_size = dst_size - 1;
            }
            if (read_size > 0)
            {
                memcpy(dst, ((coda_xmlAttributeDynamicType *)type)->value, read_size);
                dst[read_size] = '\0';
            }
            else
            {
                dst[0] = '\0';
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a string data type");
            return -1;
    }

    return 0;
}


int coda_xml_cursor_read_bytes(const coda_Cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    switch (((coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
        case tag_xml_root_dynamic:
            return coda_ascii_cursor_read_bytes(cursor, dst, offset, length);
        case tag_xml_attribute_dynamic:
            {
                coda_xmlAttributeDynamicType *type;
                long attribute_size;

                type = (coda_xmlAttributeDynamicType *)cursor->stack[cursor->n - 1].type;
                attribute_size = strlen(type->value);
                if (offset > attribute_size || offset + length > attribute_size)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, NULL);
                    return -1;
                }
                if (length > 0)
                {
                    memcpy(dst, &type->value[offset], (long)length);
                }
            }
            break;
        case tag_xml_array_dynamic:
        case tag_xml_attribute_record_dynamic:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bits/bytes data type");
            return -1;
    }

    return 0;
}

int coda_xml_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int8_array(&dd_cursor, dst, array_ordering,
                                                 ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 array data type");
    return -1;
}

int coda_xml_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint8_array(&dd_cursor, dst, array_ordering,
                                                  ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 array data type");
    return -1;
}

int coda_xml_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int16_array(&dd_cursor, dst, array_ordering,
                                                  ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 array data type");
    return -1;
}

int coda_xml_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint16_array(&dd_cursor, dst, array_ordering,
                                                   ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 array data type");
    return -1;
}

int coda_xml_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int32_array(&dd_cursor, dst, array_ordering,
                                                  ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 array data type");
    return -1;
}

int coda_xml_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint32_array(&dd_cursor, dst, array_ordering,
                                                   ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 array data type");
    return -1;
}

int coda_xml_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_int64_array(&dd_cursor, dst, array_ordering,
                                                  ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 array data type");
    return -1;
}

int coda_xml_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_uint64_array(&dd_cursor, dst, array_ordering,
                                                   ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 array data type");
    return -1;
}

int coda_xml_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_float_array(&dd_cursor, dst, array_ordering,
                                                  ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float array data type");
    return -1;
}

int coda_xml_cursor_read_double_array(const coda_Cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_double_array(&dd_cursor, dst, array_ordering,
                                                   ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double array data type");
    return -1;
}

int coda_xml_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_xmlDynamicType *type;

    type = (coda_xmlDynamicType *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_xml_ascii_type_dynamic)
    {
        coda_Cursor dd_cursor;

        /* use the ascii base type */
        dd_cursor = *cursor;
        dd_cursor.stack[cursor->n - 1].type =
            (coda_DynamicType *)((coda_xmlElementDynamicType *)type)->type->ascii_type;
        return coda_ascii_cursor_read_char_array(&dd_cursor, dst, array_ordering,
                                                 ((coda_xmlElementDynamicType *)type)->inner_bit_size);
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char array data type");
    return -1;
}
