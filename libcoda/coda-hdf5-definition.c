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
#include <stdlib.h>
#include <string.h>

#include "coda-hdf5-internal.h"

static coda_hdf5_attribute_record *empty_attributes_singleton = NULL;


void coda_hdf5_release_type(coda_type *T);

static void delete_hdf5BasicDataType(coda_hdf5_basic_data_type *T)
{
    H5Tclose(T->datatype_id);
    free(T);
}

static void delete_hdf5CompoundDataType(coda_hdf5_compound_data_type *T)
{
    int i;

    hashtable_delete(T->hash_data);
    if (T->member_type != NULL)
    {
        for (i = 0; i < T->num_members; i++)
        {
            if (T->member_type[i] >= 0)
            {
                H5Tclose(T->member_type[i]);
            }
        }
        free(T->member_type);
    }
    if (T->member_name != NULL)
    {
        for (i = 0; i < T->num_members; i++)
        {
            if (T->member_name[i] != NULL)
            {
                free(T->member_name[i]);
            }
        }
        free(T->member_name);
    }
    if (T->member != NULL)
    {
        for (i = 0; i < T->num_members; i++)
        {
            if (T->member[i] != NULL)
            {
                coda_hdf5_release_type((coda_type *)T->member[i]);
            }
        }
        free(T->member);
    }
    H5Tclose(T->datatype_id);
    free(T);
}

static void delete_hdf5Attribute(coda_hdf5_attribute *T)
{
    if (T->base_type != NULL)
    {
        coda_hdf5_release_type((coda_type *)T->base_type);
    }
    H5Sclose(T->dataspace_id);
    H5Aclose(T->attribute_id);
    free(T);
}

static void delete_hdf5AttributeRecord(coda_hdf5_attribute_record *T)
{
    int i;

    hashtable_delete(T->hash_data);
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
    if (T->attribute != NULL)
    {
        for (i = 0; i < T->num_attributes; i++)
        {
            if (T->attribute[i] != NULL)
            {
                delete_hdf5Attribute(T->attribute[i]);
            }
        }
        free(T->attribute);
    }
    free(T);
}

static void delete_hdf5Group(coda_hdf5_group *T)
{
    hsize_t i;

    if (T->attributes != NULL)
    {
        delete_hdf5AttributeRecord(T->attributes);
    }
    hashtable_delete(T->hash_data);
    if (T->object_name != NULL)
    {
        for (i = 0; i < T->num_objects; i++)
        {
            if (T->object_name[i] != NULL)
            {
                free(T->object_name[i]);
            }
        }
        free(T->object_name);
    }
    if (T->object != NULL)
    {
        for (i = 0; i < T->num_objects; i++)
        {
            if (T->object[i] != NULL)
            {
                coda_hdf5_release_type((coda_type *)T->object[i]);
            }
        }
        free(T->object);
    }
    H5Gclose(T->group_id);
    free(T);
}

static void delete_hdf5DataSet(coda_hdf5_dataset *T)
{
    if (T->attributes != NULL)
    {
        delete_hdf5AttributeRecord(T->attributes);
    }
    if (T->base_type != NULL)
    {
        coda_hdf5_release_type((coda_type *)T->base_type);
    }
    H5Sclose(T->dataspace_id);
    H5Dclose(T->dataset_id);
    free(T);
}

void coda_hdf5_release_type(coda_type *T)
{
    if (T != NULL)
    {
        switch (((coda_hdf5_type *)T)->tag)
        {
            case tag_hdf5_basic_datatype:
                delete_hdf5BasicDataType((coda_hdf5_basic_data_type *)T);
                break;
            case tag_hdf5_compound_datatype:
                delete_hdf5CompoundDataType((coda_hdf5_compound_data_type *)T);
                break;
            case tag_hdf5_attribute:
                delete_hdf5Attribute((coda_hdf5_attribute *)T);
                break;
            case tag_hdf5_attribute_record:
                delete_hdf5AttributeRecord((coda_hdf5_attribute_record *)T);
                break;
            case tag_hdf5_group:
                delete_hdf5Group((coda_hdf5_group *)T);
                break;
            case tag_hdf5_dataset:
                delete_hdf5DataSet((coda_hdf5_dataset *)T);
                break;
            default:
                assert(0);
                exit(1);
        }
    }
}

void coda_hdf5_release_dynamic_type(coda_dynamic_type *T)
{
    coda_hdf5_release_type((coda_type *)T);
}

static int new_hdf5DataType(hid_t datatype_id, coda_hdf5_data_type **type, int allow_vlen_data);

static int new_hdf5BasicDataType(hid_t datatype_id, coda_hdf5_data_type **type, int allow_vlen_data)
{
    coda_hdf5_basic_data_type *basic_type;

    basic_type = malloc(sizeof(coda_hdf5_basic_data_type));
    if (basic_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf5_basic_data_type), __FILE__, __LINE__);
        H5Tclose(datatype_id);
        return -1;
    }
    basic_type->retain_count = 0;
    basic_type->format = coda_format_hdf5;
    basic_type->name = NULL;
    basic_type->description = NULL;
    basic_type->tag = tag_hdf5_basic_datatype;
    basic_type->datatype_id = datatype_id;
    basic_type->is_variable_string = 0;
    switch (H5Tget_class(basic_type->datatype_id))
    {
        case H5T_ENUM:
            datatype_id = H5Tget_super(datatype_id);
            if (datatype_id < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                delete_hdf5BasicDataType(basic_type);
                return -1;
            }
            /* pass through and determine native type from super type of enumeration */
        case H5T_INTEGER:
            {
                int sign = 0;

                basic_type->type_class = coda_integer_class;
                switch (H5Tget_sign(datatype_id))
                {
                    case H5T_SGN_NONE:
                        /* unsigned type */
                        sign = 0;
                        break;
                    case H5T_SGN_ERROR:
                        coda_set_error(CODA_ERROR_HDF5, NULL);
                        delete_hdf5BasicDataType(basic_type);
                        if (H5Tget_class(basic_type->datatype_id) == H5T_ENUM)
                        {
                            H5Tclose(datatype_id);
                        }
                        return -1;
                    default:
                        /* signed type */
                        sign = 1;
                        break;
                }
                switch (H5Tget_size(datatype_id))
                {
                    case 1:
                        basic_type->read_type = (sign ? coda_native_type_int8 : coda_native_type_uint8);
                        break;
                    case 2:
                        basic_type->read_type = (sign ? coda_native_type_int16 : coda_native_type_uint16);
                        break;
                    case 3:
                    case 4:
                        basic_type->read_type = (sign ? coda_native_type_int32 : coda_native_type_uint32);
                        break;
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        basic_type->read_type = (sign ? coda_native_type_int64 : coda_native_type_uint64);
                        break;
                    default:
                        /* the integer type is larger than what CODA can support */
                        delete_hdf5BasicDataType(basic_type);
                        if (H5Tget_class(basic_type->datatype_id) == H5T_ENUM)
                        {
                            H5Tclose(datatype_id);
                        }
                        return 1;
                }
                if (H5Tget_class(basic_type->datatype_id) == H5T_ENUM)
                {
                    H5Tclose(datatype_id);
                }
            }
            break;
        case H5T_FLOAT:
            {
                hid_t native_type;

                basic_type->type_class = coda_real_class;
                native_type = H5Tget_native_type(datatype_id, H5T_DIR_ASCEND);
                if (native_type < 0)
                {
                    delete_hdf5BasicDataType(basic_type);
                    return -1;
                }
                if (H5Tequal(native_type, H5T_NATIVE_FLOAT))
                {
                    basic_type->read_type = coda_native_type_float;
                }
                else if (H5Tequal(native_type, H5T_NATIVE_DOUBLE))
                {
                    basic_type->read_type = coda_native_type_double;
                }
                else
                {
                    /* unsupported floating point type */
                    delete_hdf5BasicDataType(basic_type);
                    H5Tclose(native_type);
                    return 1;
                }
                H5Tclose(native_type);
            }
            break;
        case H5T_STRING:
            basic_type->type_class = coda_text_class;
            basic_type->read_type = coda_native_type_string;
            basic_type->is_variable_string = H5Tis_variable_str(datatype_id);
            if (basic_type->is_variable_string && !allow_vlen_data)
            {
                /* we don't support variable strings in this context */
                delete_hdf5BasicDataType(basic_type);
                return 1;
            }
            break;
        default:
            /* unsupported basic data type */
            delete_hdf5BasicDataType(basic_type);
            return 1;
    }

    *type = (coda_hdf5_data_type *)basic_type;

    return 0;
}

static int new_hdf5CompoundDataType(hid_t datatype_id, coda_hdf5_data_type **type)
{
    coda_hdf5_compound_data_type *compound_type;
    int result;
    int index;
    int i;

    compound_type = malloc(sizeof(coda_hdf5_compound_data_type));
    if (compound_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf5_compound_data_type), __FILE__, __LINE__);
        H5Tclose(datatype_id);
        return -1;
    }
    compound_type->retain_count = 0;
    compound_type->format = coda_format_hdf5;
    compound_type->type_class = coda_record_class;
    compound_type->name = NULL;
    compound_type->description = NULL;
    compound_type->tag = tag_hdf5_compound_datatype;
    compound_type->datatype_id = datatype_id;
    compound_type->num_members = 0;
    compound_type->member = NULL;
    compound_type->member_name = NULL;
    compound_type->member_type = NULL;
    compound_type->hash_data = hashtable_new(0);
    if (compound_type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf5CompoundDataType(compound_type);
        return -1;
    }
    compound_type->num_members = H5Tget_nmembers(compound_type->datatype_id);
    if (compound_type->num_members < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        delete_hdf5CompoundDataType(compound_type);
        return -1;
    }
    compound_type->member = malloc(compound_type->num_members * sizeof(coda_hdf5_data_type *));
    if (compound_type->member == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)compound_type->num_members * sizeof(coda_hdf5_data_type *), __FILE__, __LINE__);
        delete_hdf5CompoundDataType(compound_type);
        return -1;
    }
    for (i = 0; i < compound_type->num_members; i++)
    {
        compound_type->member[i] = NULL;
    }
    compound_type->member_name = malloc(compound_type->num_members * sizeof(char *));
    if (compound_type->member_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)compound_type->num_members * sizeof(char *), __FILE__, __LINE__);
        delete_hdf5CompoundDataType(compound_type);
        return -1;
    }
    for (i = 0; i < compound_type->num_members; i++)
    {
        compound_type->member_name[i] = NULL;
    }
    compound_type->member_type = malloc(compound_type->num_members * sizeof(hid_t));
    if (compound_type->member_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)compound_type->num_members * sizeof(hid_t), __FILE__, __LINE__);
        delete_hdf5CompoundDataType(compound_type);
        return -1;
    }
    for (i = 0; i < compound_type->num_members; i++)
    {
        compound_type->member_type[i] = -1;
    }

    /* initialize members */
    index = 0;
    for (i = 0; i < compound_type->num_members; i++)
    {
        hid_t member_id;
        char *name;

        member_id = H5Tget_member_type(datatype_id, i);
        if (member_id < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5CompoundDataType(compound_type);
            return -1;
        }

        result = new_hdf5BasicDataType(member_id, &compound_type->member[index], 0);
        if (result < 0)
        {
            /* member_id is already closed by new_hdf5BasicDataType() */
            delete_hdf5CompoundDataType(compound_type);
            return -1;
        }
        if (result == 1)
        {
            /* unsupported data type -> ignore this compound member */
            /* member_id is already closed by new_hdf5BasicDataType() */
            continue;
        }

        name = H5Tget_member_name(datatype_id, i);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5CompoundDataType(compound_type);
            return -1;
        }
        compound_type->member_name[index] = coda_identifier_from_name(name, compound_type->hash_data);
        if (compound_type->member_name[index] == NULL)
        {
            delete_hdf5CompoundDataType(compound_type);
            free(name);
            return -1;
        }
        result = hashtable_add_name(compound_type->hash_data, compound_type->member_name[index]);
        assert(result == 0);

        /* create a type for each member that allows us to read this single member only */
        compound_type->member_type[index] = H5Tcreate(H5T_COMPOUND, H5Tget_size(member_id));
        if (compound_type->member_type[index] < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5CompoundDataType(compound_type);
            free(name);
            return -1;
        }
        if (H5Tinsert(compound_type->member_type[index], name, 0, member_id) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5CompoundDataType(compound_type);
            free(name);
            return -1;
        }
        free(name);

        /* increase number of accepted attributes */
        index++;
    }

    /* update num_members with the number of members that were accepted */
    compound_type->num_members = index;

    *type = (coda_hdf5_data_type *)compound_type;

    return 0;
}

static int new_hdf5DataType(hid_t datatype_id, coda_hdf5_data_type **type, int allow_vlen_data)
{
    if (datatype_id < 0)
    {
        /* we perform the result checking of the type fetching function here */
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }

    switch (H5Tget_class(datatype_id))
    {
        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_STRING:
        case H5T_ENUM:
            return new_hdf5BasicDataType(datatype_id, type, allow_vlen_data);
        case H5T_COMPOUND:
            return new_hdf5CompoundDataType(datatype_id, type);
        case H5T_TIME:
        case H5T_BITFIELD:
        case H5T_OPAQUE:
        case H5T_REFERENCE:
        case H5T_ARRAY:
        case H5T_VLEN:
        case H5T_NO_CLASS:
        case H5T_NCLASSES:
            /* we do not support these data types */
            break;
    }

    return 1;
}

static int new_hdf5Attribute(hid_t attr_id, coda_hdf5_attribute **type)
{
    coda_hdf5_attribute *attr;
    int result;

    attr = malloc(sizeof(coda_hdf5_attribute));
    if (attr == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf5_attribute), __FILE__, __LINE__);
        H5Aclose(attr_id);
        return -1;
    }
    attr->retain_count = 0;
    attr->format = coda_format_hdf5;
    attr->type_class = coda_array_class;
    attr->name = NULL;
    attr->description = NULL;
    attr->tag = tag_hdf5_attribute;
    attr->base_type = NULL;
    attr->attribute_id = attr_id;
    attr->dataspace_id = H5Aget_space(attr_id);
    if (attr->dataspace_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        H5Aclose(attr_id);
        free(attr);
        return -1;
    }
    if (!H5Sis_simple(attr->dataspace_id))
    {
        /* we don't support complex dataspaces */
        delete_hdf5Attribute(attr);
        return 1;
    }
    attr->ndims = H5Sget_simple_extent_ndims(attr->dataspace_id);
    if (attr->ndims < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        delete_hdf5Attribute(attr);
        return -1;
    }
    if (attr->ndims > CODA_MAX_NUM_DIMS)
    {
        /* we don't support arrays with more dimensions than CODA can handle */
        delete_hdf5Attribute(attr);
        return 1;
    }
    if (H5Sget_simple_extent_dims(attr->dataspace_id, attr->dims, NULL) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        delete_hdf5Attribute(attr);
        return -1;
    }
    attr->num_elements = H5Sget_simple_extent_npoints(attr->dataspace_id);
    if (attr->num_elements == 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        delete_hdf5Attribute(attr);
        return -1;
    }
    result = new_hdf5DataType(H5Aget_type(attr->attribute_id), &attr->base_type, 0);
    if (result < 0)
    {
        delete_hdf5Attribute(attr);
        return -1;
    }
    if (result == 1)
    {
        /* unsupported basic type -> ignore this dataset */
        delete_hdf5Attribute(attr);
        return 1;
    }

    *type = attr;

    return 0;
}

static coda_hdf5_attribute_record *new_hdf5AttributeRecord(hid_t obj_id)
{
    coda_hdf5_attribute_record *attrs;
    int result;
    int index;
    int i;

    attrs = malloc(sizeof(coda_hdf5_attribute_record));
    if (attrs == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_hdf5_attribute_record), __FILE__, __LINE__);
        return NULL;
    }
    attrs->retain_count = 0;
    attrs->format = coda_format_hdf5;
    attrs->type_class = coda_record_class;
    attrs->name = NULL;
    attrs->description = NULL;
    attrs->tag = tag_hdf5_attribute_record;
    attrs->obj_id = obj_id;
    attrs->num_attributes = 0;
    attrs->attribute = NULL;
    attrs->attribute_name = NULL;
    attrs->hash_data = hashtable_new(0);
    if (attrs->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                       __LINE__);
        delete_hdf5AttributeRecord(attrs);
        return NULL;
    }

    attrs->num_attributes = H5Aget_num_attrs(obj_id);
    if (attrs->num_attributes < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        delete_hdf5AttributeRecord(attrs);
        return NULL;
    }
    attrs->attribute = malloc(attrs->num_attributes * sizeof(coda_hdf5_attribute *));
    if (attrs->attribute == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)attrs->num_attributes * sizeof(coda_hdf5_attribute *), __FILE__, __LINE__);
        delete_hdf5AttributeRecord(attrs);
        return NULL;
    }
    for (i = 0; i < attrs->num_attributes; i++)
    {
        attrs->attribute[i] = NULL;
    }
    attrs->attribute_name = malloc(attrs->num_attributes * sizeof(char *));
    if (attrs->attribute_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)attrs->num_attributes * sizeof(char *), __FILE__, __LINE__);
        delete_hdf5AttributeRecord(attrs);
        return NULL;
    }
    for (i = 0; i < attrs->num_attributes; i++)
    {
        attrs->attribute_name[i] = NULL;
    }

    /* initialize attributes */
    index = 0;
    for (i = 0; i < attrs->num_attributes; i++)
    {
        hid_t attr_id;
        char *name;
        int length;

        attr_id = H5Aopen_idx(obj_id, i);
        if (attr_id < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }

        result = new_hdf5Attribute(attr_id, &attrs->attribute[index]);
        if (result < 0)
        {
            /* attr_id is closed by new_hdf5Attributes() */
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }
        if (result == 1)
        {
            /* unsupported basic type -> ignore this attribute */
            continue;
        }
        length = H5Aget_name(attr_id, 0, NULL);
        if (length < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }
        if (length == 0)
        {
            /* we ignore attributes that have no name */
            delete_hdf5Attribute(attrs->attribute[index]);
            attrs->attribute[index] = NULL;
            continue;
        }

        name = malloc(length + 1);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)length + 1, __FILE__, __LINE__);
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }
        if (H5Aget_name(attr_id, length + 1, name) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            free(name);
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }
        attrs->attribute_name[index] = coda_identifier_from_name(name, attrs->hash_data);
        free(name);
        if (attrs->attribute_name[index] == NULL)
        {
            delete_hdf5AttributeRecord(attrs);
            return NULL;
        }
        result = hashtable_add_name(attrs->hash_data, attrs->attribute_name[index]);
        assert(result == 0);

        /* increase number of unignored attributes */
        index++;
    }

    /* update num_attributes with the number of attributes that are not ignored */
    attrs->num_attributes = index;

    return attrs;
}

/* returns: -1 = error, 0 = ok, 1 = ignore object ('type' is not set) */
static int create_tree(coda_hdf5_product *product, hid_t loc_id, const char *path, coda_hdf5_object **object)
{
    H5G_stat_t statbuf;
    herr_t status;
    int result;
    hsize_t i;

    if (H5Gget_objinfo(loc_id, path, 0, &statbuf) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }

    /* check if the object does not already exist */
    for (i = 0; i < product->num_objects; i++)
    {
        coda_hdf5_object *obj;

        obj = product->object[i];
        if (obj->fileno[0] == statbuf.fileno[0] && obj->fileno[1] == statbuf.fileno[1] &&
            obj->objno[0] == statbuf.objno[0] && obj->objno[1] == statbuf.objno[1])
        {
            /* we only allow one instance of an object in the tree, all other instances will be ignored */
            return 1;
        }
    }
    switch (statbuf.type)
    {
        case H5G_GROUP:
            {
                coda_hdf5_group *group;

                group = (coda_hdf5_group *)malloc(sizeof(coda_hdf5_group));
                if (group == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)sizeof(coda_hdf5_group), __FILE__, __LINE__);
                    return -1;
                }
                group->retain_count = 0;
                group->format = coda_format_hdf5;
                group->type_class = coda_record_class;
                group->name = NULL;
                group->description = NULL;
                group->tag = tag_hdf5_group;
                group->num_objects = 0;
                group->object = NULL;
                group->object_name = NULL;
                group->hash_data = NULL;
                group->attributes = NULL;
                group->group_id = H5Gopen(loc_id, path);
                if (group->group_id < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    delete_hdf5Group(group);
                    return -1;
                }
                group->hash_data = hashtable_new(0);
                if (group->hash_data == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)",
                                   __FILE__, __LINE__);
                    delete_hdf5Group(group);
                    return -1;
                }

                status = H5Gget_num_objs(group->group_id, &group->num_objects);
                if (status < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    delete_hdf5Group(group);
                    return -1;
                }

                group->object = malloc((size_t)group->num_objects * sizeof(coda_hdf5_object *));
                if (group->object == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)group->num_objects * sizeof(coda_hdf5_object *), __FILE__, __LINE__);
                    delete_hdf5Group(group);
                    return -1;
                }
                for (i = 0; i < group->num_objects; i++)
                {
                    group->object[i] = NULL;
                }
                group->object_name = malloc((size_t)group->num_objects * sizeof(char *));
                if (group->object_name == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)group->num_objects * sizeof(char *), __FILE__, __LINE__);
                    delete_hdf5Group(group);
                    return -1;
                }
                for (i = 0; i < group->num_objects; i++)
                {
                    group->object_name[i] = NULL;
                }

                group->attributes = new_hdf5AttributeRecord(group->group_id);
                if (group->attributes == NULL)
                {
                    delete_hdf5Group(group);
                    return -1;
                }

                *object = (coda_hdf5_object *)group;
            }
            break;
        case H5G_DATASET:
            {
                coda_hdf5_dataset *dataset;

                dataset = (coda_hdf5_dataset *)malloc(sizeof(coda_hdf5_dataset));
                if (dataset == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)sizeof(coda_hdf5_dataset), __FILE__, __LINE__);
                    return -1;
                }
                dataset->retain_count = 0;
                dataset->format = coda_format_hdf5;
                dataset->type_class = coda_array_class;
                dataset->name = NULL;
                dataset->description = NULL;
                dataset->tag = tag_hdf5_dataset;
                dataset->base_type = NULL;
                dataset->attributes = NULL;

                dataset->dataset_id = H5Dopen(loc_id, path);
                if (dataset->dataset_id < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    free(dataset);
                    return -1;
                }
                dataset->dataspace_id = H5Dget_space(dataset->dataset_id);
                if (dataset->dataspace_id < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    H5Dclose(dataset->dataset_id);
                    free(dataset);
                    return -1;
                }
                if (!H5Sis_simple(dataset->dataspace_id))
                {
                    /* we don't support complex dataspaces */
                    delete_hdf5DataSet(dataset);
                    return 1;
                }
                dataset->ndims = H5Sget_simple_extent_ndims(dataset->dataspace_id);
                if (dataset->ndims < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    delete_hdf5DataSet(dataset);
                    return -1;
                }
                if (dataset->ndims > CODA_MAX_NUM_DIMS)
                {
                    /* we don't support arrays with more dimensions than CODA can handle */
                    delete_hdf5DataSet(dataset);
                    return 1;
                }
                if (H5Sget_simple_extent_dims(dataset->dataspace_id, dataset->dims, NULL) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    delete_hdf5DataSet(dataset);
                    return -1;
                }
                dataset->num_elements = H5Sget_simple_extent_npoints(dataset->dataspace_id);
                if (dataset->num_elements == 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    delete_hdf5DataSet(dataset);
                    return -1;
                }
                result = new_hdf5DataType(H5Dget_type(dataset->dataset_id), &dataset->base_type, 1);
                if (result < 0)
                {
                    delete_hdf5DataSet(dataset);
                    return -1;
                }
                if (result == 1)
                {
                    /* unsupported basic type -> ignore this dataset */
                    delete_hdf5DataSet(dataset);
                    return 1;
                }

                dataset->attributes = new_hdf5AttributeRecord(dataset->dataset_id);
                if (dataset->attributes == NULL)
                {
                    delete_hdf5DataSet(dataset);
                    return -1;
                }

                *object = (coda_hdf5_object *)dataset;
            }
            break;
        case H5G_LINK:
        case H5G_TYPE:
        default:
            /* we don't support softlink, datatype, or unknown objects */
            return 1;
    }

    (*object)->fileno[0] = statbuf.fileno[0];
    (*object)->fileno[1] = statbuf.fileno[1];
    (*object)->objno[0] = statbuf.objno[0];
    (*object)->objno[1] = statbuf.objno[1];

    /* add object to the list of hdf5 objects */
    if (product->num_objects % BLOCK_SIZE == 0)
    {
        coda_hdf5_object **objects;

        objects = realloc(product->object, (size_t)(product->num_objects + BLOCK_SIZE) * sizeof(coda_hdf5_object *));
        if (objects == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(product->num_objects + BLOCK_SIZE) * sizeof(coda_hdf5_object *), __FILE__, __LINE__);
            return -1;
        }
        product->object = objects;
        for (i = product->num_objects; i < product->num_objects + BLOCK_SIZE; i++)
        {
            product->object[i] = NULL;
        }
    }
    product->num_objects++;
    product->object[product->num_objects - 1] = *object;

    if (statbuf.type == H5G_GROUP)
    {
        coda_hdf5_group *group;
        hsize_t index;

        group = (coda_hdf5_group *)*object;

        /* initialize group members */
        index = 0;
        for (i = 0; i < group->num_objects; i++)
        {
            char *name;
            int length;

            length = H5Gget_objname_by_idx(group->group_id, i, NULL, 0);
            if (length < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                delete_hdf5Group(group);
                return -1;
            }
            if (length == 0)
            {
                /* we ignore objects that can not be referenced using a path with names */
                continue;
            }

            name = malloc(length + 1);
            if (name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)length + 1, __FILE__, __LINE__);
                delete_hdf5Group(group);
                return -1;
            }
            if (H5Gget_objname_by_idx(group->group_id, i, name, length + 1) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                free(name);
                delete_hdf5Group(group);
                return -1;
            }

            result = create_tree(product, group->group_id, name, &group->object[index]);
            if (result == -1)
            {
                free(name);
                delete_hdf5Group(group);
                return -1;
            }
            if (result == 1)
            {
                /* skip this object */
                free(name);
                continue;
            }
            group->object_name[index] = coda_identifier_from_name(name, group->hash_data);
            free(name);
            if (group->object_name[index] == NULL)
            {
                delete_hdf5Group(group);
                return -1;
            }

            result = hashtable_add_name(group->hash_data, group->object_name[index]);
            assert(result == 0);

            /* increase number of unignored objects */
            index++;
        }
        /* update num_objects with the number of objects that are not ignored */
        group->num_objects = index;
    }

    return 0;
}

coda_hdf5_attribute_record *coda_hdf5_empty_attribute_record()
{
    if (empty_attributes_singleton == NULL)
    {
        coda_hdf5_attribute_record *T;

        T = (coda_hdf5_attribute_record *)malloc(sizeof(coda_hdf5_attribute_record));
        if (T == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)sizeof(coda_hdf5_attribute_record), __FILE__, __LINE__);
            return NULL;
        }
        T->retain_count = 0;
        T->format = coda_format_hdf5;
        T->type_class = coda_record_class;
        T->name = NULL;
        T->description = NULL;
        T->tag = tag_hdf5_attribute_record;
        T->obj_id = -1;
        T->num_attributes = 0;
        T->attribute = NULL;
        T->attribute_name = NULL;
        T->hash_data = hashtable_new(0);
        if (T->hash_data == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashdata) (%s:%u)", __FILE__,
                           __LINE__);
            delete_hdf5AttributeRecord(T);
            return NULL;
        }

        empty_attributes_singleton = T;
    }

    return empty_attributes_singleton;
}

int coda_hdf5_init(void)
{
    /* Don't let HDF5 print error messages to the console */
    H5Eset_auto(NULL, NULL);
    return 0;
}

void coda_hdf5_done()
{
    if (empty_attributes_singleton != NULL)
    {
        delete_hdf5AttributeRecord(empty_attributes_singleton);
        empty_attributes_singleton = NULL;
    }
}

int coda_hdf5_open(const char *filename, int64_t file_size, coda_product **product)
{
    coda_hdf5_product *product_file;
    int result;

    product_file = (coda_hdf5_product *)malloc(sizeof(coda_hdf5_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_hdf5_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_hdf5;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->file_id = -1;
    product_file->num_objects = 0;
    product_file->object = NULL;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }

    product_file->file_id = H5Fopen(product_file->filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (product_file->file_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }

    result = create_tree(product_file, product_file->file_id, ".", (coda_hdf5_object **)&product_file->root_type);
    if (result == -1)
    {
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }
    /* the root type is a vgroup and it should not be possible to ignore the root vgroup */
    assert(result != 1);

    *product = (coda_product *)product_file;

    return 0;
}

int coda_hdf5_close(coda_product *product)
{
    coda_hdf5_product *product_file = (coda_hdf5_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    if (product_file->root_type != NULL)
    {
        coda_hdf5_release_type((coda_type *)product_file->root_type);
    }
    if (product_file->object != NULL)
    {
        free(product_file->object);
    }
    if (product_file->file_id >= 0)
    {
        if (H5Fclose(product_file->file_id) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }
    }

    free(product_file);

    return 0;
}

int coda_hdf5_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type, coda_type **type)
{
    *type = (coda_type *)dynamic_type;
    return 0;
}

static herr_t add_error_message(int n, H5E_error_t *err_desc, void *client_data)
{
    client_data = client_data;

    if (n == 0)
    {
        /* we only display the deepest error in the stack */
        coda_add_error_message("%s(): %s (major=\"%s\", minor=\"%s\") (%s:%u)", err_desc->func_name, err_desc->desc,
                               H5Eget_major(err_desc->maj_num), H5Eget_minor(err_desc->min_num), err_desc->file_name,
                               err_desc->line);
    }

    return 0;
}

void coda_hdf5_add_error_message(void)
{
    H5Ewalk(H5E_WALK_UPWARD, add_error_message, NULL);
}
