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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int coda_xml_type_has_ascii_content(const coda_Type *type, int *has_ascii_content)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            *has_ascii_content = 1;
            break;
        case tag_xml_record:
        case tag_xml_text:
            *has_ascii_content = 1;
            break;
        case tag_xml_root:
        case tag_xml_attribute:
            *has_ascii_content = 1;
            break;
        case tag_xml_array:
        case tag_xml_attribute_record:
            *has_ascii_content = 0;
            break;
    }

    return 0;
}

int coda_xml_type_get_read_type(const coda_Type *type, coda_native_type *read_type)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_type_get_read_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, read_type);
        case tag_xml_text:
        case tag_xml_attribute:
            *read_type = coda_native_type_string;
            break;
        case tag_xml_root:
        case tag_xml_record:
        case tag_xml_array:
        case tag_xml_attribute_record:
            *read_type = coda_native_type_not_available;
            break;
    }

    return 0;
}

int coda_xml_type_get_string_length(const coda_Type *type, long *length)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascii_type_get_string_length((coda_Type *)((coda_xmlElement *)type)->ascii_type, length);
        case tag_xml_root:
        case tag_xml_record:
        case tag_xml_text:
        case tag_xml_array:
        case tag_xml_attribute:
        case tag_xml_attribute_record:
            *length = -1;
            break;
    }

    return 0;
}

int coda_xml_type_get_bit_size(const coda_Type *type, int64_t *bit_size)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascii_type_get_bit_size((coda_Type *)((coda_xmlElement *)type)->ascii_type, bit_size);
        default:
            *bit_size = -1;
            break;
    }

    return 0;
}

int coda_xml_type_get_unit(const coda_Type *type, const char **unit)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascii_type_get_unit((coda_Type *)((coda_xmlElement *)type)->ascii_type, unit);
        default:
            *unit = NULL;
            break;
    }

    return 0;
}

int coda_xml_type_get_fixed_value(const coda_Type *type, const char **fixed_value, long *length)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_xml_type_get_fixed_value((coda_Type *)((coda_xmlElement *)type)->ascii_type, fixed_value,
                                                 length);
        case tag_xml_attribute:
            *fixed_value = ((coda_xmlAttribute *)type)->fixed_value;
            if (*fixed_value != NULL)
            {
                *length = strlen(*fixed_value);
            }
            break;
        default:
            *fixed_value = NULL;
            break;
    }

    return 0;
}

int coda_xml_type_get_num_record_fields(const coda_Type *type, long *num_fields)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            *num_fields = 1;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_num_record_fields((coda_Type *)((coda_xmlElement *)type)->ascii_type,
                                                          num_fields);
        case tag_xml_record:
            *num_fields = ((coda_xmlElement *)type)->num_fields;
            break;
        case tag_xml_attribute_record:
            *num_fields = ((coda_xmlAttributeRecord *)type)->num_attributes;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index)
{
    hashtable *hash_data;

    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            if (strcasecmp(name, ((coda_xmlRoot *)type)->field->name) == 0)
            {
                *index = 0;
                return 0;
            }
            else
            {
                coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
                return -1;
            }
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_field_index_from_name((coda_Type *)((coda_xmlElement *)type)->ascii_type,
                                                                     name, index);
        case tag_xml_record:
            hash_data = ((coda_xmlElement *)type)->name_hash_data;
            break;
        case tag_xml_attribute_record:
            hash_data = ((coda_xmlAttributeRecord *)type)->name_hash_data;
            break;
        default:
            assert(0);
            exit(1);
    }

    *index = hashtable_get_index_from_name(hash_data, name);
    if (*index >= 0)
    {
        return 0;
    }

    coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
    return -1;
}

int coda_xml_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_xmlRoot *)type)->field->type;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_field_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, index,
                                                          field_type);
        case tag_xml_record:
            if (index < 0 || index >= ((coda_xmlElement *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlElement *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_xmlElement *)type)->field[index]->type;
            break;
        case tag_xml_attribute_record:
            if (index < 0 || index >= ((coda_xmlAttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlAttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *field_type = (coda_Type *)((coda_xmlAttributeRecord *)type)->attribute[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_record_field_name(const coda_Type *type, long index, const char **name)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_xmlRoot *)type)->field->name;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_field_name((coda_Type *)((coda_xmlElement *)type)->ascii_type, index,
                                                          name);
        case tag_xml_record:
            if (index < 0 || index >= ((coda_xmlElement *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlElement *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_xmlElement *)type)->field[index]->name;
            break;
        case tag_xml_attribute_record:
            if (index < 0 || index >= ((coda_xmlAttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlAttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *name = ((coda_xmlAttributeRecord *)type)->attribute[index]->attr_name;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_record_field_hidden_status(const coda_Type *type, long index, int *hidden)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            *hidden = 0;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_field_hidden_status((coda_Type *)((coda_xmlElement *)type)->ascii_type,
                                                                   index, hidden);
        case tag_xml_record:
            if (index < 0 || index >= ((coda_xmlElement *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlElement *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *hidden = ((coda_xmlElement *)type)->field[index]->hidden;
            break;
        case tag_xml_attribute_record:
            if (index < 0 || index >= ((coda_xmlAttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlAttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *hidden = 0;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_record_field_available_status(const coda_Type *type, long index, int *available)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_root:
            if (index != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,1) (%s:%u)", index,
                               __FILE__, __LINE__);
                return -1;
            }
            *available = 1;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_field_available_status((coda_Type *)
                                                                      ((coda_xmlElement *)type)->ascii_type, index,
                                                                      available);
        case tag_xml_record:
            if (index < 0 || index >= ((coda_xmlElement *)type)->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlElement *)type)->num_fields, __FILE__, __LINE__);
                return -1;
            }
            *available = ((coda_xmlElement *)type)->field[index]->optional ? -1 : 1;
            break;
        case tag_xml_attribute_record:
            if (index < 0 || index >= ((coda_xmlAttributeRecord *)type)->num_attributes)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%d) (%s:%u)", index,
                               ((coda_xmlAttributeRecord *)type)->num_attributes, __FILE__, __LINE__);
                return -1;
            }
            *available = ((coda_xmlAttributeRecord *)type)->attribute[index]->optional ? -1 : 1;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_record_union_status(const coda_Type *type, int *is_union)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_record_union_status((coda_Type *)((coda_xmlElement *)type)->ascii_type,
                                                            is_union);
        case tag_xml_record:
        case tag_xml_root:
        case tag_xml_attribute_record:
            *is_union = 0;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_array_num_dims(const coda_Type *type, int *num_dims)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_array:
            *num_dims = 1;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_array_num_dims((coda_Type *)((coda_xmlElement *)type)->ascii_type, num_dims);
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[])
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_array:
            *num_dims = 1;
            dim[0] = -1;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_array_dim((coda_Type *)((coda_xmlElement *)type)->ascii_type, num_dims, dim);
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_array_base_type(const coda_Type *type, coda_Type **base_type)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_array:
            *base_type = (coda_Type *)((coda_xmlArray *)type)->base_type;
            break;
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascbin_type_get_array_base_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, base_type);
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_special_type(const coda_Type *type, coda_special_type *special_type)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascii_type_get_special_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, special_type);
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_xml_type_get_special_base_type(const coda_Type *type, coda_Type **base_type)
{
    switch (((coda_xmlType *)type)->tag)
    {
        case tag_xml_ascii_type:
            /* the content is defined using an ascii definition */
            return coda_ascii_type_get_special_base_type((coda_Type *)((coda_xmlElement *)type)->ascii_type, base_type);
        default:
            assert(0);
            exit(1);
    }

    return 0;
}
