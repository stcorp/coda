/*
 * Copyright (C) 2007-2019 S[&]T, The Netherlands.
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

#include "coda-hdf4-internal.h"

#include <assert.h>

/* Compatibility with versions before HDF4r2 */
#ifndef _HDF_SDSVAR
#define _HDF_SDSVAR            "SDSVar"
#endif
#ifndef _HDF_CRDVAR
#define _HDF_CRDVAR          "CoordVar"
#endif

void coda_hdf4_type_delete(coda_dynamic_type *type)
{
    long i;

    assert(type != NULL);
    assert(type->backend == coda_backend_hdf4);

    switch (((coda_hdf4_type *)type)->tag)
    {
        case tag_hdf4_basic_type:
            break;
        case tag_hdf4_basic_type_array:
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_basic_type_array *)type)->basic_type);
            break;
        case tag_hdf4_string:
            break;
        case tag_hdf4_attributes:
            if (((coda_hdf4_attributes *)type)->attribute != NULL)
            {
                for (i = 0; i < ((coda_hdf4_attributes *)type)->definition->num_fields; i++)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_attributes *)type)->attribute[i]);
                }
                free(((coda_hdf4_attributes *)type)->attribute);
            }
            if (((coda_hdf4_attributes *)type)->ann_id != NULL)
            {
                free(((coda_hdf4_attributes *)type)->ann_id);
            }
            break;
        case tag_hdf4_file_attributes:
            if (((coda_hdf4_file_attributes *)type)->attribute != NULL)
            {
                for (i = 0; i < ((coda_hdf4_file_attributes *)type)->definition->num_fields; i++)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_file_attributes *)type)->attribute[i]);
                }
                free(((coda_hdf4_file_attributes *)type)->attribute);
            }
            break;
        case tag_hdf4_GRImage:
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_GRImage *)type)->basic_type);
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_GRImage *)type)->attributes);
            if (((coda_hdf4_GRImage *)type)->ri_id != -1)
            {
                GRendaccess(((coda_hdf4_GRImage *)type)->ri_id);
            }
            break;
        case tag_hdf4_SDS:
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_SDS *)type)->basic_type);
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_SDS *)type)->attributes);
            if (((coda_hdf4_SDS *)type)->sds_id != -1)
            {
                SDendaccess(((coda_hdf4_SDS *)type)->sds_id);
            }
            break;
        case tag_hdf4_Vdata:
            if (((coda_hdf4_Vdata *)type)->field != NULL)
            {
                for (i = 0; i < ((coda_hdf4_Vdata *)type)->definition->num_fields; i++)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_Vdata *)type)->field[i]);
                }
                free(((coda_hdf4_Vdata *)type)->field);
            }
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_Vdata *)type)->attributes);
            if (((coda_hdf4_Vdata *)type)->vdata_id != -1)
            {
                VSdetach(((coda_hdf4_Vdata *)type)->vdata_id);
            }
            break;
        case tag_hdf4_Vdata_field:
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_Vdata_field *)type)->basic_type);
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_Vdata_field *)type)->attributes);
            break;
        case tag_hdf4_Vgroup:
            if (((coda_hdf4_Vgroup *)type)->entry != NULL)
            {
                free(((coda_hdf4_Vgroup *)type)->entry);
            }
            coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf4_Vgroup *)type)->attributes);
            if (((coda_hdf4_Vgroup *)type)->vgroup_id != -1)
            {
                Vdetach(((coda_hdf4_Vgroup *)type)->vgroup_id);
            }
            break;
    }
    if (type->definition != NULL)
    {
        coda_type_release(type->definition);
    }
    free(type);
}

/* will leave 'value' unmodified if attribute value can not be read as a double value */
static int get_attribute_value(const coda_hdf4_product *product, coda_hdf4_attributes *attributes, const char *name,
                               double *value)
{
    coda_hdf4_type *attribute;
    coda_cursor cursor;
    long index;

    index = hashtable_get_index_from_name(attributes->definition->real_name_hash_data, name);
    if (index < 0)
    {
        return 0;
    }
    attribute = attributes->attribute[index];
    if (attribute->definition->type_class != coda_integer_class || attribute->definition->type_class != coda_real_class)
    {
        return 0;
    }
    if (coda_cursor_set_product(&cursor, (coda_product *)product) != 0)
    {
        return -1;
    }
    cursor.stack[0].type = (coda_dynamic_type *)attributes;
    cursor.stack[1].type = (coda_dynamic_type *)attribute;
    cursor.stack[1].index = index;
    cursor.n = 2;
    return coda_cursor_read_double(&cursor, value);
}

static int get_conversion_from_attributes(const coda_hdf4_product *product, coda_hdf4_attributes *attributes,
                                          coda_conversion **conversion)
{
    double scale_factor = 1.0;
    double add_offset = 0.0;
    double invalid_value = coda_NaN();

    if (get_attribute_value(product, attributes, "scale_factor", &scale_factor) != 0)
    {
        return -1;
    }
    if (get_attribute_value(product, attributes, "add_offset", &add_offset) != 0)
    {
        return -1;
    }
    if (get_attribute_value(product, attributes, "_FillValue", &invalid_value) != 0)
    {
        return -1;
    }
    if (scale_factor == 1.0 && add_offset == 0.0 && coda_isNaN(invalid_value))
    {
        *conversion = NULL;
        return 0;
    }
    *conversion = coda_conversion_new(scale_factor, 1.0, add_offset, invalid_value);
    if (*conversion == NULL)
    {
        return -1;
    }
    return 0;
}

static coda_hdf4_type *basic_type_new(int32 data_type, coda_conversion *conversion)
{
    coda_hdf4_type *type;
    int result = 0;

    type = (coda_hdf4_type *)malloc(sizeof(coda_hdf4_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_type), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_basic_type;

    if (data_type == DFNT_CHAR)
    {
        type->definition = (coda_type *)coda_type_text_new(coda_format_hdf4);
    }
    else if (data_type == DFNT_FLOAT32 || data_type == DFNT_FLOAT64)
    {
        type->definition = (coda_type *)coda_type_number_new(coda_format_hdf4, coda_real_class);
    }
    else
    {
        type->definition = (coda_type *)coda_type_number_new(coda_format_hdf4, coda_integer_class);
    }
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    switch (data_type)
    {
        case DFNT_CHAR:
            result = coda_type_set_read_type(type->definition, coda_native_type_char);
            break;
        case DFNT_INT8:
            result = coda_type_set_read_type(type->definition, coda_native_type_int8);
            break;
        case DFNT_UCHAR:
        case DFNT_UINT8:
            result = coda_type_set_read_type(type->definition, coda_native_type_uint8);
            break;
        case DFNT_INT16:
            result = coda_type_set_read_type(type->definition, coda_native_type_int16);
            break;
        case DFNT_UINT16:
            result = coda_type_set_read_type(type->definition, coda_native_type_uint16);
            break;
        case DFNT_INT32:
            result = coda_type_set_read_type(type->definition, coda_native_type_int32);
            break;
        case DFNT_UINT32:
            result = coda_type_set_read_type(type->definition, coda_native_type_uint32);
            break;
        case DFNT_INT64:
            result = coda_type_set_read_type(type->definition, coda_native_type_int64);
            break;
        case DFNT_UINT64:
            result = coda_type_set_read_type(type->definition, coda_native_type_uint64);
            break;
        case DFNT_FLOAT32:
            result = coda_type_set_read_type(type->definition, coda_native_type_float);
            break;
        case DFNT_FLOAT64:
            result = coda_type_set_read_type(type->definition, coda_native_type_double);
            break;
        default:
            coda_set_error(CODA_ERROR_PRODUCT, "unsupported HDF4 data type (%d)", data_type);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
    }
    if (result != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (conversion != NULL)
    {
        if (coda_type_number_set_conversion((coda_type_number *)type, conversion) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    return type;
}

static coda_hdf4_basic_type_array *basic_type_array_new(int32 data_type, int32 count, coda_conversion *conversion)
{
    coda_hdf4_basic_type_array *type;

    type = (coda_hdf4_basic_type_array *)malloc(sizeof(coda_hdf4_basic_type_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_basic_type_array), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_basic_type_array;
    type->basic_type = NULL;

    type->definition = coda_type_array_new(coda_format_hdf4);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_array_add_fixed_dimension(type->definition, count) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->basic_type = basic_type_new(data_type, conversion);
    if (type->basic_type == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_array_set_base_type(type->definition, type->basic_type->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

static coda_hdf4_type *string_new(int32 count)
{
    coda_hdf4_type *type;

    type = (coda_hdf4_type *)malloc(sizeof(coda_hdf4_type));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_type), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_string;

    type->definition = (coda_type *)coda_type_text_new(coda_format_hdf4);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_byte_size(type->definition, count) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

static coda_hdf4_attributes *attributes_for_GRImage(coda_hdf4_product *product, int32 ri_id, int32 num_attributes)
{
    coda_hdf4_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_attributes;
    type->parent_tag = tag_hdf4_GRImage;
    type->parent_id = ri_id;
    type->field_index = -1;
    type->attribute = NULL;
    type->ann_id = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->num_obj_attributes = num_attributes;
    type->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_RI, GRidtoref(ri_id));
    if (type->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_RI, GRidtoref(ri_id));
    if (type->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    tot_num_attributes = type->num_obj_attributes + type->num_data_labels + type->num_data_descriptions;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_obj_attributes; i++)
    {
        attr_index++;
        if (GRattrinfo(ri_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    if (type->num_data_labels + type->num_data_descriptions > 0)
    {
        type->ann_id = malloc((type->num_data_labels + type->num_data_descriptions) * sizeof(int32));
        if (type->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(type->num_data_labels + type->num_data_descriptions) * sizeof(int32), __FILE__,
                           __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (type->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_RI, GRidtoref(ri_id), type->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_labels; i++)
            {
                attr_index++;
                type->attribute[attr_index - 1] = string_new(ANannlen(type->ann_id[i]));
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "label",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
        if (type->num_data_descriptions > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_RI, GRidtoref(ri_id),
                          &type->ann_id[type->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_descriptions; i++)
            {
                attr_index++;
                length = ANannlen(type->ann_id[type->num_data_labels + i]);
                type->attribute[attr_index - 1] = string_new(length);
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "description",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
    }

    return type;
}

static coda_hdf4_attributes *attributes_for_SDS(coda_hdf4_product *product, int32 sds_id, int32 num_attributes)
{
    coda_hdf4_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_attributes;
    type->parent_tag = tag_hdf4_SDS;
    type->parent_id = sds_id;
    type->field_index = -1;
    type->attribute = NULL;
    type->num_obj_attributes = num_attributes;
    type->num_data_labels = 0;
    type->num_data_descriptions = 0;
    type->ann_id = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (product->is_hdf)
    {
        type->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_SD, (uint16)SDidtoref(sds_id));
        if (type->num_data_labels == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_SD, (uint16)SDidtoref(sds_id));
        if (type->num_data_descriptions == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    tot_num_attributes = type->num_obj_attributes + type->num_data_labels + type->num_data_descriptions;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_obj_attributes; i++)
    {
        attr_index++;
        if (SDattrinfo(sds_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (type->attribute[attr_index - 1] == NULL)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    if (type->num_data_labels + type->num_data_descriptions > 0)
    {
        type->ann_id = malloc((type->num_data_labels + type->num_data_descriptions) * sizeof(int32));
        if (type->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(type->num_data_labels + type->num_data_descriptions) * sizeof(int32), __FILE__,
                           __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (type->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_SD, (uint16)SDidtoref(sds_id), type->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_labels; i++)
            {
                attr_index++;
                type->attribute[attr_index - 1] = string_new(ANannlen(type->ann_id[i]));
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "label",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
        if (type->num_data_descriptions > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_SD, (uint16)SDidtoref(sds_id),
                          &type->ann_id[type->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_descriptions; i++)
            {
                attr_index++;
                length = ANannlen(type->ann_id[type->num_data_labels + i]);
                type->attribute[attr_index - 1] = string_new(length);
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "description",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
    }

    return type;
}

static coda_hdf4_attributes *attributes_for_Vdata_field(int32 vdata_id, int32 index)
{
    coda_hdf4_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_attributes;
    type->parent_tag = tag_hdf4_Vdata_field;
    type->parent_id = vdata_id;
    type->field_index = index;
    type->attribute = NULL;
    type->ann_id = NULL;

    type->definition = coda_type_record_new(coda_format_hdf4);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    type->num_obj_attributes = VSfnattrs(vdata_id, index);
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    type->num_obj_attributes = 0;
#endif
    type->num_data_labels = 0;
    type->num_data_descriptions = 0;

    tot_num_attributes = type->num_obj_attributes;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (VSattrinfo(vdata_id, index, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    return type;
}

static coda_hdf4_attributes *attributes_for_Vdata(coda_hdf4_product *product, int32 vdata_id, int32 vdata_ref)
{
    coda_hdf4_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_attributes;
    type->parent_tag = tag_hdf4_Vdata;
    type->parent_id = vdata_id;
    type->field_index = _HDF_VDATA;
    type->attribute = NULL;
    type->ann_id = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    type->num_obj_attributes = VSfnattrs(vdata_id, _HDF_VDATA);
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    type->num_obj_attributes = 0;
#endif
    if (type->num_obj_attributes == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_VS, (uint16)vdata_ref);
    if (type->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_VS, (uint16)vdata_ref);
    if (type->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    tot_num_attributes = type->num_obj_attributes + type->num_data_labels + type->num_data_descriptions;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (VSattrinfo(vdata_id, _HDF_VDATA, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    if (type->num_data_labels + type->num_data_descriptions > 0)
    {
        type->ann_id = malloc((type->num_data_labels + type->num_data_descriptions) * sizeof(int32));
        if (type->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(type->num_data_labels + type->num_data_descriptions) * sizeof(int32), __FILE__,
                           __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (type->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_VS, (uint16)vdata_ref, type->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_labels; i++)
            {
                attr_index++;
                type->attribute[attr_index - 1] = string_new(ANannlen(type->ann_id[i]));
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "label",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
        if (type->num_data_descriptions > 0)
        {
            if (ANannlist
                (product->an_id, AN_DATA_DESC, DFTAG_VS, (uint16)vdata_ref, &type->ann_id[type->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_descriptions; i++)
            {
                attr_index++;
                length = ANannlen(type->ann_id[type->num_data_labels + i]);
                type->attribute[attr_index - 1] = string_new(length);
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "description",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
    }

    return type;
}

static coda_hdf4_attributes *attributes_for_Vgroup(coda_hdf4_product *product, int32 vgroup_id)
{
    coda_hdf4_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_attributes;
    type->parent_tag = tag_hdf4_Vgroup;
    type->parent_id = vgroup_id;
    type->field_index = -1;
    type->attribute = NULL;
    type->ann_id = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    type->num_obj_attributes = Vnattrs(vgroup_id);
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    type->num_obj_attributes = 0;
#endif
    if (type->num_obj_attributes < 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_VG, (uint16)VQueryref(vgroup_id));
    if (type->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_VG, (uint16)VQueryref(vgroup_id));
    if (type->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    tot_num_attributes = type->num_obj_attributes + type->num_data_labels + type->num_data_descriptions;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (Vattrinfo(vgroup_id, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    if (type->num_data_labels + type->num_data_descriptions > 0)
    {
        type->ann_id = malloc((type->num_data_labels + type->num_data_descriptions) * sizeof(int32));
        if (type->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(type->num_data_labels + type->num_data_descriptions) * sizeof(int32), __FILE__,
                           __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (type->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_VG, (uint16)VQueryref(vgroup_id), type->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_labels; i++)
            {
                attr_index++;
                type->attribute[attr_index - 1] = string_new(ANannlen(type->ann_id[i]));
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "label",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
        if (type->num_data_descriptions > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_VG, (uint16)VQueryref(vgroup_id),
                          &type->ann_id[type->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                coda_hdf4_type_delete((coda_dynamic_type *)type);
                return NULL;
            }
            for (i = 0; i < type->num_data_descriptions; i++)
            {
                attr_index++;
                length = ANannlen(type->ann_id[type->num_data_labels + i]);
                type->attribute[attr_index - 1] = string_new(length);
                if (type->attribute[attr_index - 1] == NULL)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
                if (coda_type_record_create_field(type->definition, "description",
                                                  type->attribute[attr_index - 1]->definition) != 0)
                {
                    coda_hdf4_type_delete((coda_dynamic_type *)type);
                    return NULL;
                }
            }
        }
    }

    return type;
}

static coda_hdf4_file_attributes *attributes_for_root(coda_hdf4_product *product)
{
    coda_hdf4_file_attributes *type;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 num_data_labels;
    int32 num_data_descriptions;
    int32 data_type;
    int32 length;
    int32 ann_id;
    long tot_num_attributes;
    long attr_index;
    long i;

    type = (coda_hdf4_file_attributes *)malloc(sizeof(coda_hdf4_file_attributes));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_file_attributes), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_file_attributes;
    type->attribute = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->num_gr_attributes = product->num_gr_file_attributes;
    type->num_sd_attributes = product->num_sd_file_attributes;
    if (product->is_hdf)
    {
        if (ANfileinfo(product->an_id, &(type->num_file_labels), &(type->num_file_descriptions), &num_data_labels,
                       &num_data_descriptions) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    else
    {
        type->num_file_labels = 0;
        type->num_file_descriptions = 0;
    }

    tot_num_attributes = type->num_gr_attributes + type->num_sd_attributes + type->num_file_labels +
        type->num_file_descriptions;
    if (tot_num_attributes > 0)
    {
        type->attribute = malloc(tot_num_attributes * sizeof(coda_hdf4_type *));
        if (type->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           tot_num_attributes * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        for (i = 0; i < tot_num_attributes; i++)
        {
            type->attribute[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < type->num_gr_attributes; i++)
    {
        attr_index++;
        if (GRattrinfo(product->gr_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    for (i = 0; i < type->num_sd_attributes; i++)
    {
        attr_index++;
        if (SDattrinfo(product->sd_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (length == 1)
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_new(data_type, NULL);
        }
        else if (data_type == DFNT_CHAR)
        {
            type->attribute[attr_index - 1] = string_new(length);
        }
        else
        {
            type->attribute[attr_index - 1] = (coda_hdf4_type *)basic_type_array_new(data_type, length, NULL);
        }
        if (coda_type_record_create_field(type->definition, hdf4_name, type->attribute[attr_index - 1]->definition) !=
            0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    for (i = 0; i < type->num_file_labels; i++)
    {
        attr_index++;
        ann_id = ANselect(product->an_id, i, AN_FILE_LABEL);
        if (ann_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->attribute[attr_index - 1] = string_new(ANannlen(ann_id));
        if (type->attribute[attr_index - 1] == NULL)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (ANendaccess(ann_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (coda_type_record_create_field(type->definition, "label", type->attribute[attr_index - 1]->definition) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    for (i = 0; i < type->num_file_descriptions; i++)
    {
        attr_index++;
        ann_id = ANselect(product->an_id, i, AN_FILE_DESC);
        if (ann_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        type->attribute[attr_index - 1] = string_new(ANannlen(ann_id));
        if (type->attribute[attr_index - 1] == NULL)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (ANendaccess(ann_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (coda_type_record_create_field(type->definition, "description",
                                          type->attribute[attr_index - 1]->definition) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    return type;
}

coda_hdf4_GRImage *coda_hdf4_GRImage_new(coda_hdf4_product *product, int32 index)
{
    coda_conversion *conversion = NULL;
    coda_hdf4_GRImage *type;
    int32 num_attributes;

    type = (coda_hdf4_GRImage *)malloc(sizeof(coda_hdf4_GRImage));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_GRImage), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_GRImage;
    type->group_count = 0;
    type->ref = -1;
    type->ri_id = -1;
    type->index = index;
    type->basic_type = NULL;
    type->attributes = NULL;

    type->definition = coda_type_array_new(coda_format_hdf4);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->ri_id = GRselect(product->gr_id, index);
    if (type->ri_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* set the interlace mode for reading to the fastest form */
    if (GRreqimageil(type->ri_id, MFGR_INTERLACE_PIXEL) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->ref = GRidtoref(type->ri_id);
    if (type->ref == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (GRgetiminfo(type->ri_id, type->gri_name, &type->ncomp, &type->data_type, &type->interlace_mode, type->dim_sizes,
                    &num_attributes) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    /* The C interface to GRImage data uses fortran array ordering, so we swap the dimensions */
    if (coda_type_array_add_fixed_dimension(type->definition, type->dim_sizes[1]) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_array_add_fixed_dimension(type->definition, type->dim_sizes[0]) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (type->ncomp != 1)
    {
        if (coda_type_array_add_fixed_dimension(type->definition, type->ncomp) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    type->attributes = attributes_for_GRImage(product, type->ri_id, num_attributes);
    if (type->attributes == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_attributes((coda_type *)type->definition, type->attributes->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (get_conversion_from_attributes(product, type->attributes, &conversion) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->basic_type = basic_type_new(type->data_type, conversion);
    if (type->basic_type == NULL)
    {
        coda_conversion_delete(conversion);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (coda_type_array_set_base_type(type->definition, type->basic_type->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_hdf4_SDS *coda_hdf4_SDS_new(coda_hdf4_product *product, int32 sds_index)
{
    coda_conversion *conversion = NULL;
    coda_hdf4_SDS *type;
    int32 num_attributes;
    long i;

    type = (coda_hdf4_SDS *)malloc(sizeof(coda_hdf4_SDS));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_SDS), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_SDS;
    type->group_count = 0;
    type->ref = -1;
    type->sds_id = -1;
    type->index = sds_index;
    type->basic_type = NULL;
    type->attributes = NULL;

    type->definition = coda_type_array_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->sds_id = SDselect(product->sd_id, sds_index);
    if (type->sds_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (product->is_hdf)
    {
        type->ref = SDidtoref(type->sds_id);
        if (type->ref == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    if (SDgetinfo(type->sds_id, type->sds_name, &type->rank, type->dimsizes, &type->data_type, &num_attributes) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    assert(type->rank <= CODA_MAX_NUM_DIMS);
    for (i = 0; i < type->rank; i++)
    {
        if (coda_type_array_add_fixed_dimension(type->definition, type->dimsizes[i]) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    type->attributes = attributes_for_SDS(product, type->sds_id, num_attributes);
    if (type->attributes == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_attributes((coda_type *)type->definition, type->attributes->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (get_conversion_from_attributes(product, type->attributes, &conversion) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->basic_type = basic_type_new(type->data_type, conversion);
    if (type->basic_type == NULL)
    {
        coda_conversion_delete(conversion);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (coda_type_array_set_base_type(type->definition, type->basic_type->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

static coda_hdf4_Vdata_field *Vdata_field_new(coda_hdf4_product *product, int32 vdata_id, int32 field_index,
                                              int32 num_records)
{
    coda_conversion *conversion = NULL;
    coda_hdf4_Vdata_field *type;
    const char *field_name;

    type = malloc(sizeof(coda_hdf4_Vdata_field));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vdata_field), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_Vdata_field;
    type->basic_type = NULL;
    type->attributes = NULL;

    type->definition = coda_type_array_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    field_name = VFfieldname(vdata_id, field_index);
    if (field_name == NULL)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    strncpy(type->field_name, field_name, MAX_HDF4_NAME_LENGTH);
    type->field_name[MAX_HDF4_NAME_LENGTH] = '\0';
    type->num_records = num_records;
    if (coda_type_array_add_fixed_dimension(type->definition, type->num_records) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->order = VFfieldorder(vdata_id, field_index);
    if (type->order == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (type->order > 1)
    {
        if (coda_type_array_add_fixed_dimension(type->definition, type->order) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }
    type->data_type = VFfieldtype(vdata_id, field_index);
    if (type->data_type == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->attributes = attributes_for_Vdata_field(vdata_id, field_index);
    if (type->attributes == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_attributes((coda_type *)type->definition, type->attributes->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (get_conversion_from_attributes(product, type->attributes, &conversion) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->basic_type = basic_type_new(type->data_type, conversion);
    if (type->basic_type == NULL)
    {
        coda_conversion_delete(conversion);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (coda_type_array_set_base_type(type->definition, type->basic_type->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

coda_hdf4_Vdata *coda_hdf4_Vdata_new(coda_hdf4_product *product, int32 vdata_ref)
{
    coda_hdf4_Vdata *type;
    int32 num_records;
    long num_fields;
    long i;

    type = (coda_hdf4_Vdata *)malloc(sizeof(coda_hdf4_Vdata));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vdata), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_Vdata;
    type->group_count = 0;
    type->ref = vdata_ref;
    type->vdata_id = -1;
    type->field = NULL;
    type->attributes = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->vdata_id = VSattach(product->file_id, vdata_ref, "r");
    if (type->vdata_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (VSgetname(type->vdata_id, type->vdata_name) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (VSgetclass(type->vdata_id, type->classname) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* Do not show Vdata with reserved classnames:
     *  RIGATTRNAME     "RIATTR0.0N"    - name of a Vdata containing an attribute
     *  RIGATTRCLASS    "RIATTR0.0C"    - class of a Vdata containing an attribute
     *  _HDF_ATTRIBUTE  "Attr0.0"       - class of a Vdata containing SD/Vdata/Vgroup interface attribute
     *  _HDF_SDSVAR     "SDSVar"        - class of a Vdata indicating its group is an SDS variable
     *  _HDF_CRDVAR     "CoorVar"       - class of a Vdata indicating its group is a coordinate variable
     *  DIM_VALS        "DimVal0.0"     - class of a Vdata containing an SD dimension size and fake values
     *  DIM_VALS01      "DimVal0.1"     - class of a Vdata containing an SD dimension size
     *  _HDF_CDF        "CDF0.0"
     *  DATA0           "Data0.0"
     *  ATTR_FIELD_NAME "VALUES"
     *                  "_HDF_CHK_TBL_" - this class name prefix is reserved by the Chunking interface
     */
    type->hide = (strcasecmp(type->classname, RIGATTRNAME) == 0 || strcasecmp(type->classname, RIGATTRCLASS) == 0 ||
                  strcasecmp(type->classname, _HDF_ATTRIBUTE) == 0 || strcasecmp(type->classname, _HDF_SDSVAR) == 0 ||
                  strcasecmp(type->classname, _HDF_CRDVAR) == 0 || strcasecmp(type->classname, DIM_VALS) == 0 ||
                  strcasecmp(type->classname, DIM_VALS01) == 0 || strcasecmp(type->classname, _HDF_CDF) == 0 ||
                  strcasecmp(type->classname, DATA0) == 0 || strcasecmp(type->classname, ATTR_FIELD_NAME) == 0 ||
                  strncmp(type->classname, "_HDF_CHK_TBL_", 13) == 0);

    num_fields = VFnfields(type->vdata_id);
    num_records = VSelts(type->vdata_id);
    type->field = malloc(num_fields * sizeof(coda_hdf4_Vdata_field *));
    if (type->field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_fields * sizeof(coda_hdf4_Vdata_field *), __FILE__, __LINE__);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    for (i = 0; i < num_fields; i++)
    {
        type->field[i] = NULL;
    }

    type->attributes = attributes_for_Vdata(product, type->vdata_id, type->ref);
    if (type->attributes == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_attributes((coda_type *)type->definition, type->attributes->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* from here on we use delete_hdf4Vdata for the cleanup of T when an error condition occurs */

    for (i = 0; i < num_fields; i++)
    {
        type->field[i] = Vdata_field_new(product, type->vdata_id, i, num_records);
        if (type->field[i] == NULL)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
        if (coda_type_record_create_field(type->definition, type->field[i]->field_name,
                                          (coda_type *)type->field[i]->definition) != 0)
        {
            coda_hdf4_type_delete((coda_dynamic_type *)type);
            return NULL;
        }
    }

    return type;
}

coda_hdf4_Vgroup *coda_hdf4_Vgroup_new(coda_hdf4_product *product, int32 vgroup_ref)
{
    coda_hdf4_Vgroup *type;
    int32 num_entries;

    type = (coda_hdf4_Vgroup *)malloc(sizeof(coda_hdf4_Vgroup));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vgroup), __FILE__, __LINE__);
        return NULL;
    }
    type->backend = coda_backend_hdf4;
    type->definition = NULL;
    type->tag = tag_hdf4_Vgroup;
    type->group_count = 0;
    type->ref = vgroup_ref;
    type->vgroup_id = -1;
    type->entry = NULL;
    type->attributes = NULL;

    type->definition = coda_type_record_new(product->format);
    if (type->definition == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    type->vgroup_id = Vattach(product->file_id, vgroup_ref, "r");
    if (type->vgroup_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (Vinquire(type->vgroup_id, &num_entries, type->vgroup_name) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    if (Vgetclass(type->vgroup_id, type->classname) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    type->version = Vgetversion(type->vgroup_id);
    if (type->version == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    /* Do not show Vgroups with reserved classnames:
     *  GR_NAME         "RIG0.0"        - name of the Vgroup containing all the images
     *  RI_NAME         "RI0.0"         - name of a Vgroup containing information about one image
     *  _HDF_VARIABLE   "Var0.0"        - class of a Vgroup representing an SD NDG
     *  _HDF_DIMENSION  "Dim0.0"        - class of a Vgroup representing an SD dimension
     *  _HDF_UDIMENSION "UDim0.0"       - class of a Vgroup representing an SD UNLIMITED dimension
     *  _HDF_CDF        "CDF0.0"
     *  DATA0           "Data0.0"
     *  ATTR_FIELD_NAME "VALUES"
     */
    type->hide = (strcasecmp(type->classname, GR_NAME) == 0 || strcasecmp(type->classname, RI_NAME) == 0 ||
                  strcasecmp(type->classname, _HDF_VARIABLE) == 0 || strcasecmp(type->classname, _HDF_DIMENSION) == 0 ||
                  strcasecmp(type->classname, _HDF_UDIMENSION) == 0 || strcasecmp(type->classname, _HDF_CDF) == 0 ||
                  strcasecmp(type->classname, DATA0) == 0 || strcasecmp(type->classname, ATTR_FIELD_NAME) == 0);

    /* The 'entry' array is initialized in init_hdf4Vgroups() */

    type->attributes = attributes_for_Vgroup(product, type->vgroup_id);
    if (type->attributes == NULL)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }
    if (coda_type_set_attributes((coda_type *)type->definition, type->attributes->definition) != 0)
    {
        coda_hdf4_type_delete((coda_dynamic_type *)type);
        return NULL;
    }

    return type;
}

int coda_hdf4_create_root(coda_hdf4_product *product)
{
    coda_dynamic_type *attributes;
    coda_type_record *root_definition;
    int i;

    root_definition = coda_type_record_new(coda_format_hdf4);
    if (root_definition == NULL)
    {
        return -1;
    }
    product->root_type = coda_mem_record_new(root_definition, NULL);
    if (product->root_type == NULL)
    {
        coda_type_release((coda_type *)root_definition);
        return -1;
    }
    coda_type_release((coda_type *)root_definition);

    /* We add the entries to the root group in the same order as the hdfview application does */
    for (i = 0; i < product->num_vgroup; i++)
    {
        if (product->vgroup[i]->group_count == 0 && !product->vgroup[i]->hide)
        {
            if (coda_mem_record_add_field(product->root_type, product->vgroup[i]->vgroup_name,
                                          (coda_dynamic_type *)product->vgroup[i], 1) != 0)
            {
                return -1;
            }
        }
    }
    for (i = 0; i < product->num_images; i++)
    {
        if (product->gri[i]->group_count == 0)
        {
            if (coda_mem_record_add_field(product->root_type, product->gri[i]->gri_name,
                                          (coda_dynamic_type *)product->gri[i], 1) != 0)
            {
                return -1;
            }
        }
    }
    for (i = 0; i < product->num_sds; i++)
    {
        if (product->sds[i]->group_count == 0)
        {
            if (coda_mem_record_add_field(product->root_type, product->sds[i]->sds_name,
                                          (coda_dynamic_type *)product->sds[i], 1) != 0)
            {
                return -1;
            }
        }
    }
    for (i = 0; i < product->num_vdata; i++)
    {
        if (product->vdata[i]->group_count == 0 && !product->vdata[i]->hide)
        {
            if (coda_mem_record_add_field(product->root_type, product->vdata[i]->vdata_name,
                                          (coda_dynamic_type *)product->vdata[i], 1) != 0)
            {
                return -1;
            }
        }
    }

    attributes = (coda_dynamic_type *)attributes_for_root(product);
    if (attributes == NULL)
    {
        return -1;
    }
    if (coda_mem_type_set_attributes((coda_mem_type *)product->root_type, attributes, 1) != 0)
    {
        coda_dynamic_type_delete(attributes);
        return -1;
    }

    return 0;
}
