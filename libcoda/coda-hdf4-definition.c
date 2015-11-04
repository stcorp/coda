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

#include "coda-internal.h"

#include <assert.h>

#include "coda-hdf4-internal.h"

/* Compatibility with versions before HDF4r2 */
#ifndef _HDF_SDSVAR
#define _HDF_SDSVAR            "SDSVar"
#endif
#ifndef _HDF_CRDVAR
#define _HDF_CRDVAR          "CoordVar"
#endif

#undef DEBUG

static coda_hdf4_attributes *empty_attributes_singleton = NULL;

static void delete_hdf4BasicType(coda_hdf4_basic_type *T)
{
    free(T);
}

static void delete_hdf4BasicTypeArray(coda_hdf4_basic_type_array *T)
{
    if (T->basic_type != NULL)
    {
        delete_hdf4BasicType(T->basic_type);
    }
    free(T);
}

static void delete_hdf4Attributes(coda_hdf4_attributes *T)
{
    int i;

    if (T->ann_id != NULL)
    {
        free(T->ann_id);
    }
    if (T->attribute != NULL)
    {
        for (i = 0; i < T->num_attributes; i++)
        {
            if (T->attribute[i] != NULL)
            {
                if (T->attribute[i]->tag == tag_hdf4_basic_type_array)
                {
                    delete_hdf4BasicTypeArray((coda_hdf4_basic_type_array *)T->attribute[i]);
                }
                else
                {
                    delete_hdf4BasicType((coda_hdf4_basic_type *)T->attribute[i]);
                }
            }
        }
        free(T->attribute);
    }
    if (T->attribute_name != NULL)
    {
        for (i = 0; i < T->num_attributes; i++)
        {
            if (T->attribute_name[i] != NULL)
            {
                free(T->attribute_name[i]);
            }
        }
        free(T->attribute_name);
    }
    hashtable_delete(T->hash_data);
    free(T);
}

static void delete_hdf4FileAttributes(coda_hdf4_file_attributes *T)
{
    int i;

    if (T->attribute != NULL)
    {
        for (i = 0; i < T->num_attributes; i++)
        {
            if (T->attribute[i] != NULL)
            {
                if (T->attribute[i]->tag == tag_hdf4_basic_type_array)
                {
                    delete_hdf4BasicTypeArray((coda_hdf4_basic_type_array *)T->attribute[i]);
                }
                else
                {
                    delete_hdf4BasicType((coda_hdf4_basic_type *)T->attribute[i]);
                }
            }
        }
        free(T->attribute);
    }
    if (T->attribute_name != NULL)
    {
        for (i = 0; i < T->num_attributes; i++)
        {
            if (T->attribute_name[i] != NULL)
            {
                free(T->attribute_name[i]);
            }
        }
        free(T->attribute_name);
    }
    hashtable_delete(T->hash_data);
    free(T);
}

static void delete_hdf4Root(coda_hdf4_root *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf4FileAttributes(T->attributes);
    }
    hashtable_delete(T->hash_data);
    if (T->entry_name != NULL)
    {
        int i;

        for (i = 0; i < T->num_entries; i++)
        {
            if (T->entry_name[i] != NULL)
            {
                free(T->entry_name[i]);
            }
        }
        free(T->entry_name);
    }
    if (T->entry != NULL)
    {
        free(T->entry);
    }
    free(T);
}

static void delete_hdf4GRImage(coda_hdf4_GRImage *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf4Attributes(T->attributes);
    }
    delete_hdf4BasicType(T->basic_type);
    GRendaccess(T->ri_id);
    free(T);
}

static void delete_hdf4SDS(coda_hdf4_SDS *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf4Attributes(T->attributes);
    }
    delete_hdf4BasicType(T->basic_type);
    SDendaccess(T->sds_id);
    free(T);
}

static void delete_hdf4VdataField(coda_hdf4_Vdata_field *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf4Attributes(T->attributes);
    }
    delete_hdf4BasicType(T->basic_type);
    free(T);
}

static void delete_hdf4Vdata(coda_hdf4_Vdata *T)
{
    int i;

    if (T->attributes != NULL)
    {
        delete_hdf4Attributes(T->attributes);
    }
    hashtable_delete(T->hash_data);
    for (i = 0; i < T->num_fields; i++)
    {
        if (T->field[i] != NULL)
        {
            delete_hdf4VdataField(T->field[i]);
        }
        if (T->field_name[i] != NULL)
        {
            free(T->field_name[i]);
        }
    }
    free(T->field_name);
    free(T->field);
    VSdetach(T->vdata_id);
    free(T);
}

static void delete_hdf4Vgroup(coda_hdf4_Vgroup *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf4Attributes(T->attributes);
    }
    hashtable_delete(T->hash_data);
    if (T->entry_name != NULL)
    {
        int i;

        for (i = 0; i < T->num_entries; i++)
        {
            if (T->entry_name[i] != NULL)
            {
                free(T->entry_name[i]);
            }
        }
        free(T->entry_name);
    }
    if (T->entry != NULL)
    {
        free(T->entry);
    }
    Vdetach(T->vgroup_id);
    free(T);
}

void coda_hdf4_release_type(coda_type *T)
{
    if (T != NULL)
    {
        switch (((coda_hdf4_type *)T)->tag)
        {
            case tag_hdf4_root:
                delete_hdf4Root((coda_hdf4_root *)T);
                break;
            case tag_hdf4_basic_type:
                delete_hdf4BasicType((coda_hdf4_basic_type *)T);
                break;
            case tag_hdf4_basic_type_array:
                delete_hdf4BasicTypeArray((coda_hdf4_basic_type_array *)T);
                break;
            case tag_hdf4_attributes:
                delete_hdf4Attributes((coda_hdf4_attributes *)T);
                break;
            case tag_hdf4_file_attributes:
                delete_hdf4FileAttributes((coda_hdf4_file_attributes *)T);
                break;
            case tag_hdf4_GRImage:
                delete_hdf4GRImage((coda_hdf4_GRImage *)T);
                break;
            case tag_hdf4_SDS:
                delete_hdf4SDS((coda_hdf4_SDS *)T);
                break;
            case tag_hdf4_Vdata:
                delete_hdf4Vdata((coda_hdf4_Vdata *)T);
                break;
            case tag_hdf4_Vdata_field:
                delete_hdf4VdataField((coda_hdf4_Vdata_field *)T);
                break;
            case tag_hdf4_Vgroup:
                delete_hdf4Vgroup((coda_hdf4_Vgroup *)T);
                break;
        }
    }
}

void coda_hdf4_release_dynamic_type(coda_dynamic_type *T)
{
    coda_hdf4_release_type((coda_type *)T);
}

static coda_hdf4_basic_type *new_hdf4BasicType(coda_format format, int32 data_type, double scale_factor,
                                               double add_offset)
{
    coda_hdf4_basic_type *T;

    T = (coda_hdf4_basic_type *)malloc(sizeof(coda_hdf4_basic_type));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_basic_type), __FILE__, __LINE__);
        return NULL;
    }

    T->retain_count = 0;
    T->format = format;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_basic_type;
    T->has_conversion = ((scale_factor != 1 || add_offset != 0) && data_type != DFNT_CHAR);
    T->add_offset = add_offset;
    T->scale_factor = scale_factor;

    switch (data_type)
    {
        case DFNT_CHAR:
            T->type_class = coda_text_class;
            T->read_type = coda_native_type_char;
            break;
        case DFNT_UCHAR:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_uint8;
            break;
        case DFNT_INT8:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_int8;
            break;
        case DFNT_UINT8:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_uint8;
            break;
        case DFNT_INT16:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_int16;
            break;
        case DFNT_UINT16:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_uint16;
            break;
        case DFNT_INT32:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_int32;
            break;
        case DFNT_UINT32:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_uint32;
            break;
        case DFNT_INT64:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_int64;
            break;
        case DFNT_UINT64:
            T->type_class = coda_integer_class;
            T->read_type = coda_native_type_uint64;
            break;
        case DFNT_FLOAT32:
            T->type_class = coda_real_class;
            T->read_type = coda_native_type_float;
            break;
        case DFNT_FLOAT64:
            T->type_class = coda_real_class;
            T->read_type = coda_native_type_double;
            break;
        default:
            coda_set_error(CODA_ERROR_PRODUCT, "unsupported HDF4 data type (%d)", data_type);
            free(T);
            return NULL;
    }

    return T;
}

static coda_hdf4_basic_type_array *new_hdf4BasicTypeArray(coda_format format, int32 data_type, int32 count,
                                                          double scale_factor, double add_offset)
{
    coda_hdf4_basic_type_array *T;

    T = (coda_hdf4_basic_type_array *)malloc(sizeof(coda_hdf4_basic_type_array));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_basic_type_array), __FILE__, __LINE__);
        return NULL;
    }

    T->retain_count = 0;
    T->format = format;
    T->type_class = coda_array_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_basic_type_array;
    T->count = count;
    T->basic_type = new_hdf4BasicType(format, data_type, scale_factor, add_offset);
    if (T->basic_type == NULL)
    {
        free(T);
        return NULL;
    }

    return T;
}

static coda_hdf4_attributes *new_hdf4AttributesForGRImage(coda_hdf4_product *product, int32 ri_id, int32 num_attributes)
{
    coda_hdf4_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_attributes;
    T->parent_tag = tag_hdf4_GRImage;
    T->parent_id = ri_id;
    T->field_index = -1;

    T->num_attributes = 0;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;
    T->ann_id = NULL;

    T->num_obj_attributes = num_attributes;
    T->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_RI, GRidtoref(ri_id));
    if (T->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    T->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_RI, GRidtoref(ri_id));
    if (T->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    T->num_attributes = T->num_obj_attributes + T->num_data_labels + T->num_data_descriptions;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4Attributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_obj_attributes; i++)
    {
        attr_index++;
        if (GRattrinfo(ri_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4Attributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
    }
    if (T->num_data_labels + T->num_data_descriptions > 0)
    {
        T->ann_id = malloc((T->num_data_labels + T->num_data_descriptions) * sizeof(int32));
        if (T->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_data_labels + T->num_data_descriptions) * sizeof(int32), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_RI, GRidtoref(ri_id), T->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_labels; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("label", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_RI, GRidtoref(ri_id), &T->ann_id[T->num_data_labels]) ==
                -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_descriptions; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("description", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[T->num_data_labels + i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
    }

    return T;
}

static coda_hdf4_attributes *new_hdf4AttributesForSDS(coda_hdf4_product *product, int32 sds_id, int32 num_attributes)
{
    coda_hdf4_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = product->format;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_attributes;
    T->parent_tag = tag_hdf4_SDS;
    T->parent_id = sds_id;
    T->field_index = -1;

    T->num_attributes = 0;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;
    T->ann_id = NULL;

    T->num_obj_attributes = num_attributes;
    if (product->is_hdf)
    {
        T->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_SD, (uint16)SDidtoref(sds_id));
        if (T->num_data_labels == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            free(T);
            return NULL;
        }
        T->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_SD, (uint16)SDidtoref(sds_id));
        if (T->num_data_descriptions == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            free(T);
            return NULL;
        }
    }
    else
    {
        T->num_data_labels = 0;
        T->num_data_descriptions = 0;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    T->num_attributes = T->num_obj_attributes + T->num_data_labels + T->num_data_descriptions;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4Attributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_obj_attributes; i++)
    {
        attr_index++;
        if (SDattrinfo(sds_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4Attributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(T->format, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(T->format, data_type, length, 1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
    }
    if (T->num_data_labels + T->num_data_descriptions > 0)
    {
        T->ann_id = malloc((T->num_data_labels + T->num_data_descriptions) * sizeof(int32));
        if (T->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_data_labels + T->num_data_descriptions) * sizeof(int32), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_SD, (uint16)SDidtoref(sds_id), T->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_labels; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("label", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(T->format, DFNT_CHAR, length, 1,
                                                                                        0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_SD, (uint16)SDidtoref(sds_id),
                          &T->ann_id[T->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_descriptions; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("description", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[T->num_data_labels + i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(T->format, DFNT_CHAR, length, 1,
                                                                                        0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
    }

    return T;
}

static coda_hdf4_attributes *new_hdf4AttributesForVdataField(int32 vdata_id, int32 index)
{
    coda_hdf4_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_attributes;
    T->parent_tag = tag_hdf4_Vdata_field;
    T->parent_id = vdata_id;
    T->field_index = index;

    T->num_attributes = 0;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;
    T->ann_id = NULL;

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    T->num_obj_attributes = VSfnattrs(vdata_id, index);
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    T->num_obj_attributes = 0;
#endif
    T->num_data_labels = 0;
    T->num_data_descriptions = 0;

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    T->num_attributes = T->num_obj_attributes;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4Attributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (VSattrinfo(vdata_id, index, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4Attributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
    }

    return T;
}

static coda_hdf4_attributes *new_hdf4AttributesForVdata(coda_hdf4_product *product, int32 vdata_id, int32 vdata_ref)
{
    coda_hdf4_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_attributes;
    T->parent_tag = tag_hdf4_Vdata;
    T->parent_id = vdata_id;
    T->field_index = _HDF_VDATA;

    T->num_attributes = 0;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;
    T->ann_id = NULL;

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    T->num_obj_attributes = VSfnattrs(vdata_id, _HDF_VDATA);
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    T->num_obj_attributes = 0;
#endif
    if (T->num_obj_attributes == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    T->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_VS, (uint16)vdata_ref);
    if (T->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    T->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_VS, (uint16)vdata_ref);
    if (T->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    T->num_attributes = T->num_obj_attributes + T->num_data_labels + T->num_data_descriptions;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4Attributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (VSattrinfo(vdata_id, _HDF_VDATA, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4Attributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
    }
    if (T->num_data_labels + T->num_data_descriptions > 0)
    {
        T->ann_id = malloc((T->num_data_labels + T->num_data_descriptions) * sizeof(int32));
        if (T->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_data_labels + T->num_data_descriptions) * sizeof(int32), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_VS, (uint16)vdata_ref, T->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_labels; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("label", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_VS, (uint16)vdata_ref, &T->ann_id[T->num_data_labels]) ==
                -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_descriptions; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("description", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[T->num_data_labels + i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
    }

    return T;
}

static coda_hdf4_attributes *new_hdf4AttributesForVgroup(coda_hdf4_product *product, int32 vgroup_id,
                                                         int32 num_attributes)
{
    coda_hdf4_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 data_type;
    int32 length;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_attributes;
    T->parent_tag = tag_hdf4_Vgroup;
    T->parent_id = vgroup_id;
    T->field_index = -1;

    T->num_attributes = 0;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;
    T->ann_id = NULL;

#ifdef ENABLE_HDF4_VDATA_ATTRIBUTES
    T->num_obj_attributes = num_attributes;
#else
    /* We need to disable Vdata/Vgroup attributes because of a problem in HDF 4.2r1 and earlier
     * that prevents us to read an attribute value more than once.
     */
    num_attributes = num_attributes;    /* prevent 'unused' warning */
    T->num_obj_attributes = 0;
#endif
    T->num_data_labels = ANnumann(product->an_id, AN_DATA_LABEL, DFTAG_VG, (uint16)VQueryref(vgroup_id));
    if (T->num_data_labels == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    T->num_data_descriptions = ANnumann(product->an_id, AN_DATA_DESC, DFTAG_VG, (uint16)VQueryref(vgroup_id));
    if (T->num_data_descriptions == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    /* from here on we use delete_hdf4Attributes to clean up after an error occurred */

    T->num_attributes = T->num_obj_attributes + T->num_data_labels + T->num_data_descriptions;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4Attributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_obj_attributes; i++)
    {
        int32 size;

        attr_index++;
        if (Vattrinfo(vgroup_id, i, hdf4_name, &data_type, &length, &size) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4Attributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4Attributes(T);
            return NULL;
        }
    }
    if (T->num_data_labels + T->num_data_descriptions > 0)
    {
        T->ann_id = malloc((T->num_data_labels + T->num_data_descriptions) * sizeof(int32));
        if (T->ann_id == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_data_labels + T->num_data_descriptions) * sizeof(int32), __FILE__, __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_LABEL, DFTAG_VG, (uint16)VQueryref(vgroup_id), T->ann_id) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_labels; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("label", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
        if (T->num_data_labels > 0)
        {
            if (ANannlist(product->an_id, AN_DATA_DESC, DFTAG_VG, (uint16)VQueryref(vgroup_id),
                          &T->ann_id[T->num_data_labels]) == -1)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                delete_hdf4Attributes(T);
                return NULL;
            }
            for (i = 0; i < T->num_data_descriptions; i++)
            {
                attr_index++;
                T->attribute_name[attr_index - 1] = coda_identifier_from_name("description", T->hash_data);
                if (T->attribute_name[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
                result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
                assert(result == 0);
                length = ANannlen(T->ann_id[T->num_data_labels + i]);
                T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR,
                                                                                        length, 1, 0);
                if (T->attribute[attr_index - 1] == NULL)
                {
                    delete_hdf4Attributes(T);
                    return NULL;
                }
            }
        }
    }

    return T;
}

static coda_hdf4_file_attributes *new_hdf4AttributesForRoot(coda_hdf4_product *product)
{
    coda_hdf4_file_attributes *T;
    char hdf4_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 num_data_labels;
    int32 num_data_descriptions;
    int32 data_type;
    int32 length;
    int32 ann_id;
    int attr_index;
    int result;
    int i;

    T = (coda_hdf4_file_attributes *)malloc(sizeof(coda_hdf4_file_attributes));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_file_attributes), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_file_attributes;
    T->parent_tag = tag_hdf4_root;

    T->num_sd_attributes = product->num_sd_file_attributes;
    T->num_gr_attributes = product->num_gr_file_attributes;
    if (product->is_hdf)
    {
        if (ANfileinfo(product->an_id, &(T->num_file_labels), &(T->num_file_descriptions), &num_data_labels,
                       &num_data_descriptions) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            free(T);
            return NULL;
        }
    }
    else
    {
        T->num_file_labels = 0;
        T->num_file_descriptions = 0;
    }

    T->num_attributes = T->num_gr_attributes + T->num_sd_attributes + T->num_file_labels + T->num_file_descriptions;
    T->attribute = NULL;
    T->attribute_name = NULL;
    T->hash_data = NULL;

    /* from here on we use delete_hdf4FileAttributes to clean up after an error occurred */

    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf4FileAttributes(T);
        return NULL;
    }
    if (T->num_attributes > 0)
    {
        T->attribute = malloc(T->num_attributes * sizeof(coda_hdf4_type *));
        if (T->attribute == NULL)
        {
            delete_hdf4FileAttributes(T);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(coda_hdf4_type *)), __FILE__, __LINE__);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute[i] = NULL;
        }
        T->attribute_name = malloc(T->num_attributes * sizeof(char *));
        if (T->attribute_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(T->num_attributes * sizeof(char *)), __FILE__, __LINE__);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        for (i = 0; i < T->num_attributes; i++)
        {
            T->attribute_name[i] = NULL;
        }
    }

    attr_index = 0;
    for (i = 0; i < T->num_gr_attributes; i++)
    {
        attr_index++;
        if (GRattrinfo(product->gr_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
    }
    for (i = 0; i < T->num_sd_attributes; i++)
    {
        attr_index++;
        if (SDattrinfo(product->sd_id, i, hdf4_name, &data_type, &length) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        T->attribute_name[attr_index - 1] = coda_identifier_from_name(hdf4_name, T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        if (length == 1)
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicType(coda_format_hdf4, data_type, 1, 0);
        }
        else
        {
            T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, data_type, length,
                                                                                    1, 0);
        }
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
    }
    for (i = 0; i < T->num_file_labels; i++)
    {
        attr_index++;
        T->attribute_name[attr_index - 1] = coda_identifier_from_name("label", T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        ann_id = ANselect(product->an_id, i, AN_FILE_LABEL);
        if (ann_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        length = ANannlen(ann_id);
        T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR, length, 1,
                                                                                0);
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        if (ANendaccess(ann_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
    }
    for (i = 0; i < T->num_file_descriptions; i++)
    {
        attr_index++;
        T->attribute_name[attr_index - 1] = coda_identifier_from_name("description", T->hash_data);
        if (T->attribute_name[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->attribute_name[attr_index - 1]);
        assert(result == 0);
        ann_id = ANselect(product->an_id, i, AN_FILE_DESC);
        if (ann_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        length = ANannlen(ann_id);
        T->attribute[attr_index - 1] = (coda_hdf4_type *)new_hdf4BasicTypeArray(coda_format_hdf4, DFNT_CHAR, length, 1,
                                                                                0);
        if (T->attribute[attr_index - 1] == NULL)
        {
            delete_hdf4FileAttributes(T);
            return NULL;
        }
        if (ANendaccess(ann_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            delete_hdf4FileAttributes(T);
            return NULL;
        }
    }

    return T;
}

static coda_hdf4_GRImage *new_hdf4GRImage(coda_hdf4_product *product, int32 index)
{
    coda_hdf4_GRImage *T;
    double scale_factor;
    double add_offset;
    int32 attr_index;

    T = (coda_hdf4_GRImage *)malloc(sizeof(coda_hdf4_GRImage));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_GRImage), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_array_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_GRImage;

    T->group_count = 0;

    T->index = index;
    T->ri_id = GRselect(product->gr_id, index);
    if (T->ri_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    /* set the interlace mode for reading to the fastest form */
    if (GRreqimageil(T->ri_id, MFGR_INTERLACE_PIXEL) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    T->ref = GRidtoref(T->ri_id);
    if (T->ref == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        GRendaccess(T->ri_id);
        free(T);
        return NULL;
    }
    if (GRgetiminfo(T->ri_id, T->gri_name, &T->ncomp, &T->data_type, &T->interlace_mode, T->dim_sizes,
                    &T->num_attributes) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        GRendaccess(T->ri_id);
        free(T);
        return NULL;
    }
    T->num_elements = T->dim_sizes[0] * T->dim_sizes[1] * T->ncomp;

    /* determine scale_factor and add_offset */
    scale_factor = 1;
    attr_index = GRfindattr(T->ri_id, "scale_factor");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;

        if (GRattrinfo(T->ri_id, attr_index, name, &data_type, &count) == 0)
        {
            if (data_type == DFNT_FLOAT64 && count == 1)
            {
                GRgetattr(T->ri_id, attr_index, &scale_factor);
            }
        }
    }
    add_offset = 0;
    attr_index = GRfindattr(T->ri_id, "add_offset");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;

        if (GRattrinfo(T->ri_id, attr_index, name, &data_type, &count) == 0)
        {
            if (data_type == DFNT_FLOAT64 && count == 1)
            {
                GRgetattr(T->ri_id, attr_index, &add_offset);
            }
        }
    }

    T->basic_type = new_hdf4BasicType(coda_format_hdf4, T->data_type, scale_factor, add_offset);
    if (T->basic_type == NULL)
    {
        GRendaccess(T->ri_id);
        free(T);
        return NULL;
    }

    T->attributes = new_hdf4AttributesForGRImage(product, T->ri_id, T->num_attributes);
    if (T->attributes == NULL)
    {
        delete_hdf4BasicType(T->basic_type);
        GRendaccess(T->ri_id);
        free(T);
        return NULL;
    }

#ifdef DEBUG
    printf("[GRImage] %s (ref=%d, data_type=%d, dim=(%d,%d), ncomp=%d)\n", T->gri_name, T->ref, T->data_type,
           T->dim_sizes[0], T->dim_sizes[1], T->ncomp);
    printf("   Attr: num_obj_attr=%d, num_data_labels=%d, num_data_descr=%d\n", T->attributes->num_obj_attributes,
           T->attributes->num_data_labels, T->attributes->num_data_descriptions);
#endif
    return T;
}

static coda_hdf4_SDS *new_hdf4SDS(coda_hdf4_product *product, int32 sds_index)
{
    coda_hdf4_SDS *T;
    double scale_factor;
    double add_offset;
    int32 attr_index;
    int i;

    T = (coda_hdf4_SDS *)malloc(sizeof(coda_hdf4_SDS));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_SDS), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = product->format;
    T->type_class = coda_array_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_SDS;

    T->group_count = 0;

    T->index = sds_index;
    T->sds_id = SDselect(product->sd_id, sds_index);
    if (T->sds_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    if (product->is_hdf)
    {
        T->ref = SDidtoref(T->sds_id);
        if (T->ref == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            SDendaccess(T->sds_id);
            free(T);
            return NULL;
        }
    }
    else
    {
        T->ref = -1;
    }

    if (SDgetinfo(T->sds_id, T->sds_name, &T->rank, T->dimsizes, &T->data_type, &T->num_attributes) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        SDendaccess(T->sds_id);
        free(T);
        return NULL;
    }
    assert(T->rank <= CODA_MAX_NUM_DIMS);
    T->num_elements = 1;
    for (i = 0; i < T->rank; i++)
    {
        T->num_elements *= T->dimsizes[i];
    }

    /* determine scale_factor and add_offset */
    scale_factor = 1;
    attr_index = SDfindattr(T->sds_id, "scale_factor");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;

        if (SDattrinfo(T->sds_id, attr_index, name, &data_type, &count) == 0)
        {
            if ((data_type == DFNT_FLOAT64 || data_type == DFNT_FLOAT32) && count == 1)
            {
                SDreadattr(T->sds_id, attr_index, &scale_factor);
                if (data_type == DFNT_FLOAT32)
                {
                    scale_factor = (double)*(float *)&scale_factor;
                }
            }
        }
    }
    add_offset = 0;
    attr_index = SDfindattr(T->sds_id, "add_offset");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;

        if (SDattrinfo(T->sds_id, attr_index, name, &data_type, &count) == 0)
        {
            if ((data_type == DFNT_FLOAT64 || data_type == DFNT_FLOAT32) && count == 1)
            {
                SDreadattr(T->sds_id, attr_index, &add_offset);
                if (data_type == DFNT_FLOAT32)
                {
                    add_offset = (double)*(float *)&add_offset;
                }
            }
        }
    }

    T->basic_type = new_hdf4BasicType(T->format, T->data_type, scale_factor, add_offset);
    if (T->basic_type == NULL)
    {
        SDendaccess(T->sds_id);
        free(T);
        return NULL;
    }

    T->attributes = new_hdf4AttributesForSDS(product, T->sds_id, T->num_attributes);
    if (T->attributes == NULL)
    {
        delete_hdf4BasicType(T->basic_type);
        SDendaccess(T->sds_id);
        free(T);
        return NULL;
    }

#ifdef DEBUG
    printf("[SDS] %s (ref=%d, rank=%d, data_type=%d)\n", T->sds_name, T->ref, T->rank, T->data_type);
    printf("   Attr: num_obj_attr=%d, num_data_labels=%d, num_data_descr=%d\n", T->attributes->num_obj_attributes,
           T->attributes->num_data_labels, T->attributes->num_data_descriptions);
#endif /* DEBUG */
    return T;
}

static coda_hdf4_Vdata_field *new_hdf4VdataField(int32 vdata_id, int32 field_index, int32 num_records)
{
    coda_hdf4_Vdata_field *T;
    const char *field_name;
    double scale_factor;
    double add_offset;
    int32 attr_index;

    T = malloc(sizeof(coda_hdf4_Vdata_field));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vdata_field), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_array_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_Vdata_field;

    field_name = VFfieldname(vdata_id, field_index);
    if (field_name == NULL)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    strncpy(T->field_name, field_name, MAX_HDF4_NAME_LENGTH);
    T->field_name[MAX_HDF4_NAME_LENGTH] = '\0';
    T->num_records = num_records;
    T->order = VFfieldorder(vdata_id, field_index);
    if (T->order == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    if (T->order > 1)
    {
        T->num_elements = T->num_records * T->order;
    }
    else
    {
        T->num_elements = T->num_records;
    }
    T->data_type = VFfieldtype(vdata_id, field_index);
    if (T->data_type == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    /* determine scale_factor and add_offset */
    scale_factor = 1;
    attr_index = VSfindattr(vdata_id, field_index, "scale_factor");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;
        int32 size;

        if (VSattrinfo(vdata_id, field_index, attr_index, name, &data_type, &count, &size) == 0)
        {
            if (data_type == DFNT_FLOAT64 && count == 1)
            {
                VSgetattr(vdata_id, field_index, attr_index, &scale_factor);
            }
        }
    }
    add_offset = 0;
    attr_index = VSfindattr(vdata_id, field_index, "add_offset");
    if (attr_index >= 0)
    {
        char name[MAX_HDF4_NAME_LENGTH];
        int32 data_type;
        int32 count;
        int32 size;

        if (VSattrinfo(vdata_id, field_index, attr_index, name, &data_type, &count, &size) == 0)
        {
            if (data_type == DFNT_FLOAT64 && count == 1)
            {
                VSgetattr(vdata_id, field_index, attr_index, &add_offset);
            }
        }
    }

    T->basic_type = new_hdf4BasicType(coda_format_hdf4, T->data_type, scale_factor, add_offset);
    if (T->basic_type == NULL)
    {
        free(T);
        return NULL;
    }

    T->attributes = new_hdf4AttributesForVdataField(vdata_id, field_index);
    if (T->attributes == NULL)
    {
        delete_hdf4BasicType(T->basic_type);
        free(T);
        return NULL;
    }

#ifdef DEBUG
    printf("[Vdata field] %s (order=%d, data_type=%d)\n", T->field_name, T->order, T->data_type);
    printf("   Attr: num_obj_attr=%d, num_data_labels=%d, num_data_descr=%d\n", T->attributes->num_obj_attributes,
           T->attributes->num_data_labels, T->attributes->num_data_descriptions);
#endif /* DEBUG */
    return T;
}

static coda_hdf4_Vdata *new_hdf4Vdata(coda_hdf4_product *product, int32 vdata_ref)
{
    coda_hdf4_Vdata *T;
    int i;

    T = malloc(sizeof(coda_hdf4_Vdata));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vdata), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_Vdata;

    T->group_count = 0;
    T->ref = vdata_ref;
    T->vdata_id = VSattach(product->file_id, vdata_ref, "r");
    if (T->vdata_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }
    if (VSgetname(T->vdata_id, T->vdata_name) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        VSdetach(T->vdata_id);
        free(T);
        return NULL;
    }
    if (VSgetclass(T->vdata_id, T->classname) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        VSdetach(T->vdata_id);
        free(T);
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
    T->hide = (strcasecmp(T->classname, RIGATTRNAME) == 0 || strcasecmp(T->classname, RIGATTRCLASS) == 0 ||
               strcasecmp(T->classname, _HDF_ATTRIBUTE) == 0 || strcasecmp(T->classname, _HDF_SDSVAR) == 0 ||
               strcasecmp(T->classname, _HDF_CRDVAR) == 0 || strcasecmp(T->classname, DIM_VALS) == 0 ||
               strcasecmp(T->classname, DIM_VALS01) == 0 || strcasecmp(T->classname, _HDF_CDF) == 0 ||
               strcasecmp(T->classname, DATA0) == 0 || strcasecmp(T->classname, ATTR_FIELD_NAME) == 0 ||
               strncmp(T->classname, "_HDF_CHK_TBL_", 13) == 0);

    T->num_fields = VFnfields(T->vdata_id);
    T->num_records = VSelts(T->vdata_id);
    T->field = malloc(T->num_fields * sizeof(coda_hdf4_Vdata_field *));
    if (T->field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(T->num_fields * sizeof(coda_hdf4_Vdata_field *)), __FILE__, __LINE__);
        VSdetach(T->vdata_id);
        free(T);
        return NULL;
    }
    T->field_name = malloc(T->num_fields * sizeof(char *));
    if (T->field_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(T->num_fields * sizeof(char *)), __FILE__, __LINE__);
        VSdetach(T->vdata_id);
        free(T->field);
        free(T);
        return NULL;
    }
    for (i = 0; i < T->num_fields; i++)
    {
        T->field[i] = NULL;
        T->field_name[i] = NULL;
    }
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        VSdetach(T->vdata_id);
        free(T->field_name);
        free(T->field);
        free(T);
        return NULL;
    }

    T->attributes = new_hdf4AttributesForVdata(product, T->vdata_id, T->ref);
    if (T->attributes == NULL)
    {
        hashtable_delete(T->hash_data);
        VSdetach(T->vdata_id);
        free(T->field_name);
        free(T->field);
        free(T);
        return NULL;
    }

    /* from here on we use delete_hdf4Vdata for the cleanup of T when an error condition occurs */

    for (i = 0; i < T->num_fields; i++)
    {
        int result;

        T->field[i] = new_hdf4VdataField(T->vdata_id, i, T->num_records);
        if (T->field[i] == NULL)
        {
            delete_hdf4Vdata(T);
            return NULL;
        }
        T->field_name[i] = coda_identifier_from_name(T->field[i]->field_name, T->hash_data);
        if (T->field_name[i] == NULL)
        {
            delete_hdf4Vdata(T);
            return NULL;
        }
        result = hashtable_add_name(T->hash_data, T->field_name[i]);
        assert(result == 0);
    }

#ifdef DEBUG
    printf("[Vdata] %s (ref=%d, class=%s, hide=%d, num_fields=%d, num_records=%d)\n", T->vdata_name, T->ref,
           T->classname, T->hide, T->num_fields, T->num_records);
    printf("   Attr: num_obj_attr=%d, num_data_labels=%d, num_data_descr=%d\n", T->attributes->num_obj_attributes,
           T->attributes->num_data_labels, T->attributes->num_data_descriptions);
#endif /* DEBUG */
    return T;
}

static coda_hdf4_Vgroup *new_hdf4Vgroup(coda_hdf4_product *product, int32 vgroup_ref)
{
    coda_hdf4_Vgroup *T;

    T = malloc(sizeof(coda_hdf4_Vgroup));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_Vgroup), __FILE__, __LINE__);
        return NULL;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_Vgroup;

    T->group_count = 0;
    T->ref = vgroup_ref;
    T->num_attributes = 0;
    T->num_entries = 0;
    T->entry = NULL;
    T->entry_name = NULL;

    T->vgroup_id = Vattach(product->file_id, vgroup_ref, "r");
    if (T->vgroup_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        free(T);
        return NULL;
    }

    if (Vinquire(T->vgroup_id, &(T->num_entries), T->vgroup_name) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        Vdetach(T->vgroup_id);
        free(T);
        return NULL;
    }

    if (Vgetclass(T->vgroup_id, T->classname) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        Vdetach(T->vgroup_id);
        free(T);
        return NULL;
    }
    T->version = Vgetversion(T->vgroup_id);
    if (T->version == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        Vdetach(T->vgroup_id);
        free(T);
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
    T->hide = (strcasecmp(T->classname, GR_NAME) == 0 || strcasecmp(T->classname, RI_NAME) == 0 ||
               strcasecmp(T->classname, _HDF_VARIABLE) == 0 || strcasecmp(T->classname, _HDF_DIMENSION) == 0 ||
               strcasecmp(T->classname, _HDF_UDIMENSION) == 0 || strcasecmp(T->classname, _HDF_CDF) == 0 ||
               strcasecmp(T->classname, DATA0) == 0 || strcasecmp(T->classname, ATTR_FIELD_NAME) == 0);

    /* The 'entry' array is initialized in init_hdf4Vgroups() */

    T->num_attributes = Vnattrs(T->vgroup_id);
    if (T->num_attributes < 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        Vdetach(T->vgroup_id);
        free(T);
        return NULL;
    }

    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        Vdetach(T->vgroup_id);
        free(T);
        return NULL;
    }

    T->attributes = new_hdf4AttributesForVgroup(product, T->vgroup_id, T->num_attributes);
    if (T->attributes == NULL)
    {
        hashtable_delete(T->hash_data);
        Vdetach(T->vgroup_id);
        free(T);
        return NULL;
    }

#ifdef DEBUG
    printf("[Vgroup] %s (ref=%d, class=%s, version=%d, hide=%d)\n", T->vgroup_name, T->ref, T->classname, T->version,
           T->hide);
    printf("   Attr: num_obj_attr=%d, num_data_labels=%d, num_data_descr=%d\n", T->attributes->num_obj_attributes,
           T->attributes->num_data_labels, T->attributes->num_data_descriptions);
#endif /* DEBUG */
    return T;
}

static int init_hdf4GRImages(coda_hdf4_product *product)
{
    if (GRfileinfo(product->gr_id, &(product->num_images), &(product->num_gr_file_attributes)) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        return -1;
    }
    if (product->num_images > 0)
    {
        int i;

        product->gri = malloc(product->num_images * sizeof(coda_hdf4_GRImage *));
        if (product->gri == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)product->num_images * sizeof(coda_hdf4_GRImage *), __FILE__, __LINE__);
            return -1;
        }
        for (i = 0; i < product->num_images; i++)
        {
            product->gri[i] = NULL;
        }
        for (i = 0; i < product->num_images; i++)
        {
            product->gri[i] = new_hdf4GRImage(product, i);
            if (product->gri[i] == NULL)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int init_hdf4SDSs(coda_hdf4_product *product)
{
    if (SDfileinfo(product->sd_id, &(product->num_sds), &(product->num_sd_file_attributes)) != 0)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        return -1;
    }
    if (product->num_sds > 0)
    {
        int i;

        product->sds = malloc(product->num_sds * sizeof(coda_hdf4_SDS *));
        if (product->sds == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)product->num_sds * sizeof(coda_hdf4_SDS *), __FILE__, __LINE__);
            return -1;
        }
        for (i = 0; i < product->num_sds; i++)
        {
            product->sds[i] = NULL;
        }
        for (i = 0; i < product->num_sds; i++)
        {
            product->sds[i] = new_hdf4SDS(product, i);
            if (product->sds[i] == NULL)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int init_hdf4Vdatas(coda_hdf4_product *product)
{
    int32 vdata_ref;

    vdata_ref = VSgetid(product->file_id, -1);
    while (vdata_ref != -1)
    {
        if (product->num_vdata % BLOCK_SIZE == 0)
        {
            coda_hdf4_Vdata **vdata;
            int i;

            vdata = realloc(product->vdata, (product->num_vdata + BLOCK_SIZE) * sizeof(coda_hdf4_Vdata *));
            if (vdata == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)(product->num_vdata + BLOCK_SIZE) * sizeof(coda_hdf4_Vdata *), __FILE__, __LINE__);
                return -1;
            }
            product->vdata = vdata;
            for (i = product->num_vdata; i < product->num_vdata + BLOCK_SIZE; i++)
            {
                product->vdata[i] = NULL;
            }
        }
        product->num_vdata++;
        product->vdata[product->num_vdata - 1] = new_hdf4Vdata(product, vdata_ref);
        if (product->vdata[product->num_vdata - 1] == NULL)
        {
            return -1;
        }
        vdata_ref = VSgetid(product->file_id, vdata_ref);
    }

    return 0;
}

static int init_hdf4Vgroups(coda_hdf4_product *product)
{
    int32 vgroup_ref;
    int result;
    int i;

    vgroup_ref = Vgetid(product->file_id, -1);
    while (vgroup_ref != -1)
    {
        if (product->num_vgroup % BLOCK_SIZE == 0)
        {
            coda_hdf4_Vgroup **vgroup;
            int i;

            vgroup = realloc(product->vgroup, (product->num_vgroup + BLOCK_SIZE) * sizeof(coda_hdf4_Vgroup *));
            if (vgroup == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)(product->num_vgroup + BLOCK_SIZE) * sizeof(coda_hdf4_Vgroup *), __FILE__,
                               __LINE__);
                return -1;
            }
            product->vgroup = vgroup;
            for (i = product->num_vgroup; i < product->num_vgroup + BLOCK_SIZE; i++)
            {
                product->vgroup[i] = NULL;
            }
        }
        product->num_vgroup++;
        /* This will not yet create the links to the entries of the Vgroup */
        product->vgroup[product->num_vgroup - 1] = new_hdf4Vgroup(product, vgroup_ref);
        if (product->vgroup[product->num_vgroup - 1] == NULL)
        {
            return -1;
        }
        vgroup_ref = Vgetid(product->file_id, vgroup_ref);
    }

    /* Now for each Vgroup create the links to its entries */
    for (i = 0; i < product->num_vgroup; i++)
    {
        coda_hdf4_Vgroup *T;

        T = product->vgroup[i];

        if (T->num_entries > 0 && !T->hide)
        {
            int32 *tags;
            int32 *refs;
            int32 num_entries;
            int j;

            T->entry = malloc(T->num_entries * sizeof(coda_hdf4_type *));
            if (T->entry == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)T->num_entries * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
                return -1;
            }
            T->entry_name = malloc(T->num_entries * sizeof(char *));
            if (T->entry_name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)T->num_entries * sizeof(char *), __FILE__, __LINE__);
                return -1;
            }
            for (j = 0; j < T->num_entries; j++)
            {
                T->entry_name[j] = NULL;
            }
            tags = malloc(T->num_entries * sizeof(int32));
            if (tags == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)T->num_entries * sizeof(int32), __FILE__, __LINE__);
                return -1;
            }
            refs = malloc(T->num_entries * sizeof(int32));
            if (refs == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)T->num_entries * sizeof(int32), __FILE__, __LINE__);
                free(tags);
                return -1;
            }

            result = Vgettagrefs(T->vgroup_id, tags, refs, T->num_entries);
            if (result != T->num_entries)
            {
                coda_set_error(CODA_ERROR_HDF4, NULL);
                free(refs);
                free(tags);
                return -1;
            }

            num_entries = T->num_entries;
            T->num_entries = 0;

            for (j = 0; j < num_entries; j++)
            {
                int32 index;
                int k;

                switch (tags[j])
                {
                    case DFTAG_RIG:
                    case DFTAG_RI:
                    case DFTAG_RI8:
                        index = GRreftoindex(product->gr_id, (uint16)refs[j]);
                        if (index != -1)
                        {
                            for (k = 0; k < product->num_images; k++)
                            {
                                if (product->gri[k]->index == index)
                                {
                                    product->gri[k]->group_count++;
                                    T->num_entries++;
                                    T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->gri[k];
                                    T->entry_name[T->num_entries - 1] =
                                        coda_identifier_from_name(product->gri[k]->gri_name, T->hash_data);
                                    if (T->entry_name[T->num_entries - 1] == NULL)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                                    assert(result == 0);
                                    break;
                                }
                            }
                            /* if k == product->num_images then the Vgroup links to a non-existent GRImage and
                             * we ignore the entry */
                        }
                        /* if index == -1 then the Vgroup links to a non-existent GRImage and we ignore the entry */
                        break;
                    case DFTAG_SD:
                    case DFTAG_SDG:
                    case DFTAG_NDG:
                        index = SDreftoindex(product->sd_id, refs[j]);
                        if (index != -1)
                        {
                            for (k = 0; k < product->num_sds; k++)
                            {
                                if (product->sds[k]->index == index)
                                {
                                    product->sds[k]->group_count++;
                                    T->num_entries++;
                                    T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->sds[k];
                                    T->entry_name[T->num_entries - 1] =
                                        coda_identifier_from_name(product->sds[k]->sds_name, T->hash_data);
                                    if (T->entry_name[T->num_entries - 1] == NULL)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                                    assert(result == 0);
                                    break;
                                }
                            }
                            /* if k == product->num_sds then the Vgroup links to a non-existent SDS and
                             * we ignore the entry */
                        }
                        /* if index == -1 then the Vgroup links to a non-existent SDS and we ignore the entry */
                        break;
                    case DFTAG_VH:
                    case DFTAG_VS:
                        for (k = 0; k < product->num_vdata; k++)
                        {
                            if (product->vdata[k]->ref == refs[j])
                            {
                                if (!product->vdata[k]->hide)
                                {
                                    product->vdata[k]->group_count++;
                                    T->num_entries++;
                                    T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->vdata[k];
                                    T->entry_name[T->num_entries - 1] =
                                        coda_identifier_from_name(product->vdata[k]->vdata_name, T->hash_data);
                                    if (T->entry_name[T->num_entries - 1] == NULL)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                                    assert(result == 0);
                                }
                                break;
                            }
                        }
                        /* if k == product->num_vdata then the Vgroup links to a non-existent Vdata and
                         * we ignore the entry */
                        break;
                    case DFTAG_VG:
                        for (k = 0; k < product->num_vgroup; k++)
                        {
                            if (product->vgroup[k]->ref == refs[j])
                            {
                                if (!product->vgroup[k]->hide)
                                {
                                    product->vgroup[k]->group_count++;
                                    T->num_entries++;
                                    T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->vgroup[k];
                                    T->entry_name[T->num_entries - 1] =
                                        coda_identifier_from_name(product->vgroup[k]->vgroup_name, T->hash_data);
                                    if (T->entry_name[T->num_entries - 1] == NULL)
                                    {
                                        free(refs);
                                        free(tags);
                                        return -1;
                                    }
                                    result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                                    assert(result == 0);
                                }
                                break;
                            }
                        }
                        /* if k == product->num_vgroup then the Vgroup links to a non-existent Vgroup and
                         * we ignore the entry */
                        break;
                    default:
                        /* The Vgroup contains an unsupported item and we ignore the entry */
                        break;
                }
            }
            free(refs);
            free(tags);
        }
    }

    return 0;
}

static int create_hdf4Root(coda_hdf4_product *product)
{
    coda_hdf4_root *T;
    int32 num_root_entries;
    int result;
    int i;

    T = (coda_hdf4_root *)malloc(sizeof(coda_hdf4_root));
    if (T == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf4_root), __FILE__, __LINE__);
        return -1;
    }
    T->retain_count = 0;
    T->format = coda_format_hdf4;
    T->type_class = coda_record_class;
    T->name = NULL;
    T->description = NULL;
    T->tag = tag_hdf4_root;

    T->num_entries = 0;
    T->entry = NULL;
    T->entry_name = NULL;
    T->attributes = NULL;
    T->hash_data = hashtable_new(0);
    if (T->hash_data == NULL)
    {
        free(T);
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    product->root_type = (coda_dynamic_type *)T;

    num_root_entries = 0;
    for (i = 0; i < product->num_vgroup; i++)
    {
        if (product->vgroup[i]->group_count == 0 && !product->vgroup[i]->hide)
        {
            num_root_entries++;
        }
    }
    for (i = 0; i < product->num_images; i++)
    {
        if (product->gri[i]->group_count == 0)
        {
            num_root_entries++;
        }
    }
    for (i = 0; i < product->num_sds; i++)
    {
        if (product->sds[i]->group_count == 0)
        {
            num_root_entries++;
        }
    }
    for (i = 0; i < product->num_vdata; i++)
    {
        if (product->vdata[i]->group_count == 0 && !product->vdata[i]->hide)
        {
            num_root_entries++;
        }
    }

    if (num_root_entries > 0)
    {
        T->entry = malloc(num_root_entries * sizeof(coda_hdf4_type *));
        if (T->entry == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)T->num_entries * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            return -1;
        }
        T->entry_name = malloc(num_root_entries * sizeof(coda_hdf4_type *));
        if (T->entry_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)T->num_entries * sizeof(coda_hdf4_type *), __FILE__, __LINE__);
            return -1;
        }

        /* We add the entries to the root group in the same order as the hdfview application does */

        for (i = 0; i < product->num_vgroup; i++)
        {
            if (product->vgroup[i]->group_count == 0 && !product->vgroup[i]->hide)
            {
                product->vgroup[i]->group_count++;
                T->num_entries++;
                T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->vgroup[i];
                T->entry_name[T->num_entries - 1] =
                    coda_identifier_from_name(product->vgroup[i]->vgroup_name, T->hash_data);
                if (T->entry_name[T->num_entries - 1] == NULL)
                {
                    return -1;
                }
                result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                assert(result == 0);
            }
        }
        for (i = 0; i < product->num_images; i++)
        {
            if (product->gri[i]->group_count == 0)
            {
                product->gri[i]->group_count++;
                T->num_entries++;
                T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->gri[i];
                T->entry_name[T->num_entries - 1] = coda_identifier_from_name(product->gri[i]->gri_name, T->hash_data);
                if (T->entry_name[T->num_entries - 1] == NULL)
                {
                    return -1;
                }
                result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                assert(result == 0);
            }
        }
        for (i = 0; i < product->num_sds; i++)
        {
            if (product->sds[i]->group_count == 0)
            {
                product->sds[i]->group_count++;
                T->num_entries++;
                T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->sds[i];
                T->entry_name[T->num_entries - 1] = coda_identifier_from_name(product->sds[i]->sds_name, T->hash_data);
                if (T->entry_name[T->num_entries - 1] == NULL)
                {
                    return -1;
                }
                result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                assert(result == 0);
            }
        }
        for (i = 0; i < product->num_vdata; i++)
        {
            if (product->vdata[i]->group_count == 0 && !product->vdata[i]->hide)
            {
                product->vdata[i]->group_count++;
                T->num_entries++;
                T->entry[T->num_entries - 1] = (coda_hdf4_type *)product->vdata[i];
                T->entry_name[T->num_entries - 1] =
                    coda_identifier_from_name(product->vdata[i]->vdata_name, T->hash_data);
                if (T->entry_name[T->num_entries - 1] == NULL)
                {
                    return -1;
                }
                result = hashtable_add_name(T->hash_data, T->entry_name[T->num_entries - 1]);
                assert(result == 0);
            }
        }
    }

    T->attributes = new_hdf4AttributesForRoot(product);
    if (T->attributes == NULL)
    {
        return -1;
    }
#ifdef DEBUG
    printf("Root Attr: num_gr_attr=%d, num_sd_attr=%d, num_file_labels=%d, num_file_descr=%d\n",
           T->attributes->num_gr_attributes, T->attributes->num_sd_attributes,
           T->attributes->num_file_labels, T->attributes->num_file_descriptions);
#endif

    return 0;
}

coda_hdf4_attributes *coda_hdf4_empty_attributes()
{
    if (empty_attributes_singleton == NULL)
    {
        coda_hdf4_attributes *T;

        T = (coda_hdf4_attributes *)malloc(sizeof(coda_hdf4_attributes));
        if (T == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)sizeof(coda_hdf4_attributes), __FILE__, __LINE__);
            return NULL;
        }
        T->retain_count = 0;
        T->format = coda_format_hdf4;
        T->type_class = coda_record_class;
        T->name = NULL;
        T->description = NULL;
        T->tag = tag_hdf4_attributes;
        T->parent_tag = -1;     /* just an invalid value. This field shouldn't be accessed for an empty attributes record */

        T->num_attributes = 0;
        T->num_obj_attributes = 0;
        T->num_data_descriptions = 0;
        T->num_data_labels = 0;
        T->attribute = NULL;
        T->attribute_name = NULL;
        T->ann_id = NULL;

        T->hash_data = hashtable_new(0);
        if (T->hash_data == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                           __LINE__);
            delete_hdf4Attributes(T);
            return NULL;
        }

        empty_attributes_singleton = T;
    }

    return empty_attributes_singleton;
}

void coda_hdf4_done(void)
{
    if (empty_attributes_singleton != NULL)
    {
        delete_hdf4Attributes(empty_attributes_singleton);
        empty_attributes_singleton = NULL;
    }
}

int coda_hdf4_close(coda_product *product)
{
    coda_hdf4_product *product_file = (coda_hdf4_product *)product;
    int i;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    if (product_file->root_type != NULL)
    {
        delete_hdf4Root((coda_hdf4_root *)product_file->root_type);
    }

    if (product_file->vgroup != NULL)
    {
        for (i = 0; i < product_file->num_vgroup; i++)
        {
            if (product_file->vgroup[i] != NULL)
            {
                delete_hdf4Vgroup(product_file->vgroup[i]);
            }
        }
        free(product_file->vgroup);
    }
    if (product_file->vdata != NULL)
    {
        for (i = 0; i < product_file->num_vdata; i++)
        {
            if (product_file->vdata[i] != NULL)
            {
                delete_hdf4Vdata(product_file->vdata[i]);
            }
        }
        free(product_file->vdata);
    }
    if (product_file->sds != NULL)
    {
        for (i = 0; i < product_file->num_sds; i++)
        {
            if (product_file->sds[i] != NULL)
            {
                delete_hdf4SDS(product_file->sds[i]);
            }
        }
        free(product_file->sds);
    }
    if (product_file->gri != NULL)
    {
        for (i = 0; i < product_file->num_images; i++)
        {
            if (product_file->gri[i] != NULL)
            {
                delete_hdf4GRImage(product_file->gri[i]);
            }
        }
        free(product_file->gri);
    }

    if (product_file->sd_id != -1)
    {
        SDend(product_file->sd_id);
    }
    if (product_file->is_hdf)
    {
        if (product_file->gr_id != -1)
        {
            GRend(product_file->gr_id);
        }
        if (product_file->an_id != -1)
        {
            ANend(product_file->an_id);
        }
        if (product_file->file_id != -1)
        {
            Vend(product_file->file_id);
            Hclose(product_file->file_id);
        }
    }

    free(product_file);

    return 0;
}

int coda_hdf4_open(const char *filename, int64_t file_size, coda_format format, coda_product **product)
{
    coda_hdf4_product *product_file;

    product_file = (coda_hdf4_product *)malloc(sizeof(coda_hdf4_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_hdf4_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = format;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->is_hdf = 0;
    product_file->file_id = -1;
    product_file->gr_id = -1;
    product_file->sd_id = -1;
    product_file->an_id = -1;
    product_file->num_gr_file_attributes = 0;
    product_file->num_sd_file_attributes = 0;
    product_file->num_sds = 0;
    product_file->sds = NULL;
    product_file->num_images = 0;
    product_file->gri = NULL;
    product_file->num_vgroup = 0;
    product_file->vgroup = NULL;
    product_file->num_vdata = 0;
    product_file->vdata = NULL;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }

    product_file->is_hdf = Hishdf(product_file->filename);      /* is this a real HDF4 file or a (net)CDF file */
    if (product_file->is_hdf)
    {
        product_file->file_id = Hopen(product_file->filename, DFACC_READ, 0);
        if (product_file->file_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            return -1;
        }
        if (Vstart(product_file->file_id) != 0)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        product_file->gr_id = GRstart(product_file->file_id);
        if (product_file->gr_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        product_file->an_id = ANstart(product_file->file_id);
        if (product_file->an_id == -1)
        {
            coda_set_error(CODA_ERROR_HDF4, NULL);
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
    }
    product_file->sd_id = SDstart(product_file->filename, DFACC_READ);
    if (product_file->sd_id == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }
    product_file->root_type = NULL;

    if (init_hdf4SDSs(product_file) != 0)
    {
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }
    if (product_file->is_hdf)
    {
        if (init_hdf4GRImages(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        if (init_hdf4Vdatas(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
        /* initialization of Vgroup entries should happen last, so we can build the structural tree */
        if (init_hdf4Vgroups(product_file) != 0)
        {
            coda_hdf4_close((coda_product *)product_file);
            return -1;
        }
    }

    if (create_hdf4Root(product_file) != 0)
    {
        coda_hdf4_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_hdf4_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type, coda_type **type)
{
    *type = (coda_type *)dynamic_type;
    return 0;
}

void coda_hdf4_add_error_message(void)
{
    int error = HEvalue(1);

    if (error != 0)
    {
        coda_add_error_message(HEstring(error));
    }
}
