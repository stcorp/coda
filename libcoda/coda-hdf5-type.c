/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include <assert.h>
#include <stdlib.h>

#include "coda-hdf5-internal.h"

void coda_hdf5_type_delete(coda_dynamic_type *type)
{
    long i;

    assert(type != NULL);
    assert(type->backend == coda_backend_hdf5);

    switch (((coda_hdf5_type *)type)->tag)
    {
        case tag_hdf5_basic_datatype:
            H5Tclose(((coda_hdf5_basic_data_type *)type)->datatype_id);
            break;
        case tag_hdf5_compound_datatype:
            {
                coda_hdf5_compound_data_type *compound_type = (coda_hdf5_compound_data_type *)type;

                if (compound_type->member_type != NULL)
                {
                    for (i = 0; i < compound_type->definition->num_fields; i++)
                    {
                        if (compound_type->member_type[i] >= 0)
                        {
                            H5Tclose(compound_type->member_type[i]);
                        }
                    }
                    free(compound_type->member_type);
                }
                if (compound_type->member != NULL)
                {
                    for (i = 0; i < compound_type->definition->num_fields; i++)
                    {
                        if (compound_type->member[i] != NULL)
                        {
                            coda_dynamic_type_delete((coda_dynamic_type *)compound_type->member[i]);
                        }
                    }
                    free(compound_type->member);
                }
                H5Tclose(compound_type->datatype_id);
            }
            break;
        case tag_hdf5_group:
            if (((coda_hdf5_group *)type)->attributes != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf5_group *)type)->attributes);
            }
            if (((coda_hdf5_group *)type)->object != NULL)
            {
                for (i = 0; i < ((coda_hdf5_group *)type)->definition->num_fields; i++)
                {
                    if (((coda_hdf5_group *)type)->object[i] != NULL)
                    {
                        coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf5_group *)type)->object[i]);
                    }
                }
                free(((coda_hdf5_group *)type)->object);
            }
            H5Gclose(((coda_hdf5_group *)type)->group_id);
            break;
        case tag_hdf5_dataset:
            if (((coda_hdf5_dataset *)type)->attributes != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf5_dataset *)type)->attributes);
            }
            if (((coda_hdf5_dataset *)type)->base_type != NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)((coda_hdf5_dataset *)type)->base_type);
            }
            H5Sclose(((coda_hdf5_dataset *)type)->dataspace_id);
            H5Dclose(((coda_hdf5_dataset *)type)->dataset_id);
            break;
    }
    if (type->definition != NULL)
    {
        coda_type_release(type->definition);
    }
    free(type);
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
    basic_type->backend = coda_backend_hdf5;
    basic_type->definition = NULL;
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
                coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                return -1;
            }
            /* pass through and determine native type from super type of enumeration */
        case H5T_INTEGER:
            {
                int sign = 0;
                int result = 0;

                basic_type->definition = (coda_type *)coda_type_number_new(coda_format_hdf5, coda_integer_class);
                if (basic_type->definition == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                    if (H5Tget_class(basic_type->datatype_id) == H5T_ENUM)
                    {
                        H5Tclose(datatype_id);
                    }
                    return -1;
                }
                switch (H5Tget_sign(datatype_id))
                {
                    case H5T_SGN_NONE:
                        /* unsigned type */
                        sign = 0;
                        break;
                    case H5T_SGN_ERROR:
                        coda_set_error(CODA_ERROR_HDF5, NULL);
                        coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
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
                        if (sign)
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_int8);
                        }
                        else
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_uint8);
                        }
                        break;
                    case 2:
                        if (sign)
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_int16);
                        }
                        else
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_uint16);
                        }
                        break;
                    case 3:
                    case 4:
                        if (sign)
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_int32);
                        }
                        else
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_uint32);
                        }
                        break;
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        if (sign)
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_int64);
                        }
                        else
                        {
                            result = coda_type_set_read_type(basic_type->definition, coda_native_type_uint64);
                        }
                        break;
                    default:
                        /* the integer type is larger than what CODA can support */
                        coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
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
                if (result != 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                    return -1;
                }
            }
            break;
        case H5T_FLOAT:
            {
                hid_t native_type;
                int result = 0;

                basic_type->definition = (coda_type *)coda_type_number_new(coda_format_hdf5, coda_real_class);
                if (basic_type->definition == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                    return -1;
                }
                native_type = H5Tget_native_type(datatype_id, H5T_DIR_ASCEND);
                if (native_type < 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                    return -1;
                }
                if (H5Tequal(native_type, H5T_NATIVE_FLOAT))
                {
                    result = coda_type_set_read_type(basic_type->definition, coda_native_type_float);
                }
                else if (H5Tequal(native_type, H5T_NATIVE_DOUBLE))
                {
                    result = coda_type_set_read_type(basic_type->definition, coda_native_type_double);
                }
                else
                {
                    /* unsupported floating point type */
                    result = 1;
                }
                H5Tclose(native_type);
                if (result != 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                    return -1;
                }
            }
            break;
        case H5T_STRING:
            basic_type->definition = (coda_type *)coda_type_text_new(coda_format_hdf5);
            if (basic_type->definition == NULL)
            {
                coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                return -1;
            }
            basic_type->is_variable_string = H5Tis_variable_str(datatype_id);
            if (basic_type->is_variable_string && !allow_vlen_data)
            {
                /* we don't support variable strings in this context */
                coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
                return 1;
            }
            break;
        default:
            /* unsupported basic data type */
            coda_hdf5_type_delete((coda_dynamic_type *)basic_type);
            return 1;
    }

    *type = (coda_hdf5_data_type *)basic_type;

    return 0;
}

static int new_hdf5CompoundDataType(hid_t datatype_id, coda_hdf5_data_type **type)
{
    coda_hdf5_compound_data_type *compound_type;
    long num_members;
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
    compound_type->backend = coda_backend_hdf5;
    compound_type->definition = NULL;
    compound_type->tag = tag_hdf5_compound_datatype;
    compound_type->datatype_id = datatype_id;
    compound_type->member = NULL;
    compound_type->member_type = NULL;

    compound_type->definition = coda_type_record_new(coda_format_hdf5);
    if (compound_type->definition == NULL)
    {
        coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
        return -1;
    }
    num_members = H5Tget_nmembers(compound_type->datatype_id);
    if (num_members < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
        return -1;
    }
    compound_type->member = malloc(num_members * sizeof(coda_hdf5_data_type *));
    if (compound_type->member == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_members * sizeof(coda_hdf5_data_type *), __FILE__, __LINE__);
        coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
        return -1;
    }
    for (i = 0; i < num_members; i++)
    {
        compound_type->member[i] = NULL;
    }
    compound_type->member_type = malloc(num_members * sizeof(hid_t));
    if (compound_type->member_type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_members * sizeof(hid_t), __FILE__, __LINE__);
        coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
        return -1;
    }
    for (i = 0; i < num_members; i++)
    {
        compound_type->member_type[i] = -1;
    }

    /* initialize members */
    index = 0;
    for (i = 0; i < num_members; i++)
    {
        hid_t member_id;
        char *name;

        member_id = H5Tget_member_type(datatype_id, i);
        if (member_id < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
            return -1;
        }

        result = new_hdf5BasicDataType(member_id, &compound_type->member[index], 0);
        if (result < 0)
        {
            /* member_id is already closed by new_hdf5BasicDataType() */
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
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
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
            return -1;
        }
        if (coda_type_record_create_field(compound_type->definition, name, compound_type->member[index]->definition) !=
            0)
        {
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
            free(name);
            return -1;
        }

        /* create a type for each member that allows us to read this single member only */
        compound_type->member_type[index] = H5Tcreate(H5T_COMPOUND, H5Tget_size(member_id));
        if (compound_type->member_type[index] < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
            free(name);
            return -1;
        }
        if (H5Tinsert(compound_type->member_type[index], name, 0, member_id) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_hdf5_type_delete((coda_dynamic_type *)compound_type);
            free(name);
            return -1;
        }
        free(name);

        /* increase number of accepted attributes */
        index++;
    }

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
            H5Tclose(datatype_id);
            /* we do not support these data types */
            break;
    }

    return 1;
}

static int new_hdf5AttributeDefinition(hid_t attr_id, coda_type **type)
{
    coda_type *definition = NULL;
    hid_t dataspace_id;
    hid_t datatype_id;
    hid_t enum_datatype_id = -1;
    hsize_t dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    int i;

    dataspace_id = H5Aget_space(attr_id);
    if (dataspace_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }
    if (!H5Sis_simple(dataspace_id))
    {
        /* we don't support complex dataspaces */
        H5Sclose(dataspace_id);
        return 1;
    }
    num_dims = H5Sget_simple_extent_ndims(dataspace_id);
    if (num_dims < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        H5Sclose(dataspace_id);
        return -1;
    }
    if (num_dims > CODA_MAX_NUM_DIMS)
    {
        /* we don't support arrays with more dimensions than CODA can handle */
        H5Sclose(dataspace_id);
        return 1;
    }
    if (num_dims > 0)
    {
        if (H5Sget_simple_extent_dims(dataspace_id, dim, NULL) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            H5Sclose(dataspace_id);
            return -1;
        }
    }
    H5Sclose(dataspace_id);

    datatype_id = H5Aget_type(attr_id);
    switch (H5Tget_class(datatype_id))
    {
        case H5T_ENUM:
            enum_datatype_id = datatype_id;
            datatype_id = H5Tget_super(enum_datatype_id);
            H5Tclose(enum_datatype_id);
            if (datatype_id < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                return -1;
            }
            /* pass through and determine native type from super type of enumeration */
        case H5T_INTEGER:
            {
                int sign = 0;
                int result = 0;

                definition = (coda_type *)coda_type_number_new(coda_format_hdf5, coda_integer_class);
                if (definition == NULL)
                {
                    H5Tclose(datatype_id);
                    return -1;
                }
                switch (H5Tget_sign(datatype_id))
                {
                    case H5T_SGN_NONE:
                        /* unsigned type */
                        sign = 0;
                        break;
                    case H5T_SGN_ERROR:
                        coda_set_error(CODA_ERROR_HDF5, NULL);
                        coda_type_release(definition);
                        H5Tclose(datatype_id);
                        return -1;
                    default:
                        /* signed type */
                        sign = 1;
                        break;
                }
                switch (H5Tget_size(datatype_id))
                {
                    case 1:
                        if (sign)
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_int8);
                        }
                        else
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_uint8);
                        }
                        if (result == 0)
                        {
                            result = coda_type_set_byte_size(definition, 1);
                        }
                        break;
                    case 2:
                        if (sign)
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_int16);
                        }
                        else
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_uint16);
                        }
                        if (result == 0)
                        {
                            result = coda_type_set_byte_size(definition, 2);
                        }
                        break;
                    case 3:
                    case 4:
                        if (sign)
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_int32);
                        }
                        else
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_uint32);
                        }
                        if (result == 0)
                        {
                            result = coda_type_set_byte_size(definition, 4);
                        }
                        break;
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                        if (sign)
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_int64);
                        }
                        else
                        {
                            result = coda_type_set_read_type(definition, coda_native_type_uint64);
                        }
                        if (result == 0)
                        {
                            result = coda_type_set_byte_size(definition, 8);
                        }
                        break;
                    default:
                        /* the integer type is larger than what CODA can support */
                        coda_type_release(definition);
                        H5Tclose(datatype_id);
                        return 1;
                }
                H5Tclose(datatype_id);
                if (result != 0)
                {
                    coda_type_release(definition);
                    return -1;
                }
#ifndef WORDS_BIGENDIAN
                if (coda_type_number_set_endianness((coda_type_number *)definition, coda_little_endian) != 0)
                {
                    coda_type_release(definition);
                    return -1;
                }
#endif
            }
            break;
        case H5T_FLOAT:
            {
                hid_t native_type;
                int result = 0;

                definition = (coda_type *)coda_type_number_new(coda_format_hdf5, coda_real_class);
                if (definition == NULL)
                {
                    H5Tclose(datatype_id);
                    return -1;
                }
                native_type = H5Tget_native_type(datatype_id, H5T_DIR_ASCEND);
                if (native_type < 0)
                {
                    coda_type_release(definition);
                    H5Tclose(datatype_id);
                    return -1;
                }
                if (H5Tequal(native_type, H5T_NATIVE_FLOAT))
                {
                    result = coda_type_set_read_type(definition, coda_native_type_float);
                    if (result == 0)
                    {
                        result = coda_type_set_byte_size(definition, 4);
                    }
                }
                else if (H5Tequal(native_type, H5T_NATIVE_DOUBLE))
                {
                    result = coda_type_set_read_type(definition, coda_native_type_double);
                    if (result == 0)
                    {
                        result = coda_type_set_byte_size(definition, 8);
                    }
                }
                else
                {
                    /* unsupported floating point type */
                    result = 1;
                }
                H5Tclose(native_type);
                H5Tclose(datatype_id);
                if (result != 0)
                {
                    coda_type_release(definition);
                    return result;
                }
#ifndef WORDS_BIGENDIAN
                if (coda_type_number_set_endianness((coda_type_number *)definition, coda_little_endian) != 0)
                {
                    coda_type_release(definition);
                    return -1;
                }
#endif
            }
            break;
        case H5T_STRING:
            definition = (coda_type *)coda_type_text_new(coda_format_hdf5);
            if (definition == NULL)
            {
                H5Tclose(datatype_id);
                return -1;
            }
            break;
        default:
            /* unsupported basic data type */
            H5Tclose(datatype_id);
            return 1;
    }

    if (num_dims > 0)
    {
        coda_type_array *array = NULL;

        array = coda_type_array_new(coda_format_hdf5);
        if (array == NULL)
        {
            coda_type_release(definition);
            return -1;
        }

        if (coda_type_array_set_base_type(array, definition) != 0)
        {
            coda_type_release(definition);
            coda_type_release((coda_type *)array);
            return -1;
        }
        coda_type_release(definition);
        for (i = 0; i < num_dims; i++)
        {
            if (coda_type_array_add_fixed_dimension(array, (long)dim[i]) != 0)
            {
                coda_type_release((coda_type *)array);
                return -1;
            }
        }
        definition = (coda_type *)array;
    }

    *type = definition;

    return 0;
}

static int new_hdf5Attribute(coda_product *product, hid_t attr_id, coda_dynamic_type **type)
{
    coda_type *definition;
    hid_t datatype_id;
    uint8_t *buffer = NULL;
    int is_variable_string;
    long num_elements;
    long size = -1;
    int result;
    int k;

    result = new_hdf5AttributeDefinition(attr_id, &definition);
    if (result != 0)
    {
        return result;
    }

    datatype_id = H5Aget_type(attr_id);
    size = H5Tget_size(datatype_id);
    is_variable_string = H5Tis_variable_str(datatype_id);

    num_elements = 1;
    if (definition->type_class == coda_array_class)
    {
        num_elements = ((coda_type_array *)definition)->num_elements;
    }

    buffer = malloc((size_t)num_elements * size);
    if (buffer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_elements * size, __FILE__, __LINE__);
        coda_type_release(definition);
        H5Tclose(datatype_id);
        return -1;
    }

    if (H5Aread(attr_id, datatype_id, buffer) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        coda_type_release(definition);
        H5Tclose(datatype_id);
        free(buffer);
        return -1;
    }
    H5Tclose(datatype_id);

    if (definition->type_class == coda_array_class)
    {
        coda_mem_array *array;
        coda_type *base_type = ((coda_type_array *)definition)->base_type;
        long i;

        array = coda_mem_array_new((coda_type_array *)definition, NULL);
        coda_type_release(definition);
        if (array == NULL)
        {
            if (is_variable_string)
            {
                for (k = 0; k < num_elements; k++)
                {
                    free(((char **)buffer)[k]);
                }
            }
            free(buffer);
            return -1;
        }

        for (i = 0; i < num_elements; i++)
        {
            coda_mem_data *element;

            if (base_type->read_type == coda_native_type_string && is_variable_string)
            {
                element = coda_mem_string_new((coda_type_text *)base_type, NULL, product, ((char **)buffer)[i]);
            }
            else
            {
                element = coda_mem_data_new(base_type, NULL, product, size, &(((uint8_t *)buffer)[i * size]));
            }
            if (element == NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                if (is_variable_string)
                {
                    for (k = 0; k < num_elements; k++)
                    {
                        free(((char **)buffer)[k]);
                    }
                }
                free(buffer);
                return -1;
            }
            if (coda_mem_array_set_element(array, i, (coda_dynamic_type *)element) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)element);
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                if (is_variable_string)
                {
                    for (k = 0; k < num_elements; k++)
                    {
                        free(((char **)buffer)[k]);
                    }
                }
                free(buffer);
                return -1;
            }
        }

        *type = (coda_dynamic_type *)array;
    }
    else
    {
        coda_mem_data *element;

        if (definition->read_type == coda_native_type_string && is_variable_string)
        {
            element = coda_mem_string_new((coda_type_text *)definition, NULL, product, *((char **)buffer));
        }
        else
        {
            element = coda_mem_data_new(definition, NULL, product, size, buffer);
        }
        coda_type_release(definition);
        if (element == NULL)
        {
            if (is_variable_string)
            {
                for (k = 0; k < num_elements; k++)
                {
                    free(((char **)buffer)[k]);
                }
            }
            free(buffer);
            return -1;
        }

        *type = (coda_dynamic_type *)element;
    }

    if (is_variable_string)
    {
        for (k = 0; k < num_elements; k++)
        {
            free(((char **)buffer)[k]);
        }
    }
    free(buffer);

    return 0;
}

static coda_mem_record *new_hdf5AttributeRecord(coda_product *product, hid_t obj_id)
{
    coda_type_record *definition;
    coda_mem_record *attrs;
    long num_attributes;
    long i;
    int result;

    definition = coda_type_record_new(coda_format_hdf5);
    if (definition == NULL)
    {
        return NULL;
    }
    attrs = coda_mem_record_new(definition, NULL);
    coda_type_release((coda_type *)definition);
    if (attrs == NULL)
    {
        return NULL;
    }

    num_attributes = H5Aget_num_attrs(obj_id);
    if (num_attributes < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        coda_dynamic_type_delete((coda_dynamic_type *)attrs);
        return NULL;
    }

    /* initialize attributes */
    for (i = 0; i < num_attributes; i++)
    {
        coda_dynamic_type *attribute = NULL;
        hid_t attr_id;
        char *name;
        int length;

        attr_id = H5Aopen_idx(obj_id, i);
        if (attr_id < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            return NULL;
        }

        length = H5Aget_name(attr_id, 0, NULL);
        if (length < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            H5Aclose(attr_id);
            return NULL;
        }
        if (length == 0)
        {
            /* we ignore attributes that have no name */
            H5Aclose(attr_id);
            continue;
        }
        name = malloc(length + 1);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)length + 1, __FILE__, __LINE__);
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            H5Aclose(attr_id);
            return NULL;
        }
        if (H5Aget_name(attr_id, length + 1, name) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            free(name);
            H5Aclose(attr_id);
            return NULL;
        }

        result = new_hdf5Attribute(product, attr_id, &attribute);
        H5Aclose(attr_id);
        if (result < 0)
        {
            /* attr_id is closed by new_hdf5Attributes() */
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            free(name);
            return NULL;
        }
        if (result == 1)
        {
            /* unsupported basic type -> ignore this attribute */
            free(name);
            continue;
        }

        if (coda_mem_record_add_field(attrs, name, attribute, 1) != 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attribute);
            coda_dynamic_type_delete((coda_dynamic_type *)attrs);
            free(name);
            return NULL;
        }

        free(name);
    }

    return attrs;
}

/* returns: -1 = error, 0 = ok, 1 = ignore object ('type' is not set) */
int coda_hdf5_create_tree(coda_hdf5_product *product, hid_t loc_id, const char *path, coda_hdf5_object **object)
{
    H5G_stat_t statbuf;
    hsize_t num_objects = 0;
    herr_t status;
    long i;
    int result;

    if (H5Gget_objinfo(loc_id, path, 0, &statbuf) < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        return -1;
    }

    /* check if the object does not already exist */
    for (i = 0; i < (long)product->num_objects; i++)
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
                group->backend = coda_backend_hdf5;
                group->definition = NULL;
                group->tag = tag_hdf5_group;
                group->object = NULL;
                group->attributes = NULL;
                group->group_id = H5Gopen(loc_id, path);
                if (group->group_id < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }

                group->definition = coda_type_record_new(coda_format_hdf5);
                if (group->definition == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }
                status = H5Gget_num_objs(group->group_id, &num_objects);
                if (status < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }

                group->object = malloc((size_t)num_objects * sizeof(coda_hdf5_object *));
                if (group->object == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)num_objects * sizeof(coda_hdf5_object *), __FILE__, __LINE__);
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }
                for (i = 0; i < (long)num_objects; i++)
                {
                    group->object[i] = NULL;
                }

                group->attributes = new_hdf5AttributeRecord((coda_product *)product, group->group_id);
                if (group->attributes == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }
                if (coda_type_set_attributes((coda_type *)group->definition, group->attributes->definition) != 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)group);
                    return -1;
                }

                *object = (coda_hdf5_object *)group;
            }
            break;
        case H5G_DATASET:
            {
                coda_hdf5_dataset *dataset;
                hsize_t dim[CODA_MAX_NUM_DIMS];
                int num_dims;

                dataset = (coda_hdf5_dataset *)malloc(sizeof(coda_hdf5_dataset));
                if (dataset == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)sizeof(coda_hdf5_dataset), __FILE__, __LINE__);
                    return -1;
                }
                dataset->backend = coda_backend_hdf5;
                dataset->definition = NULL;
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
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return 1;
                }

                dataset->definition = coda_type_array_new(coda_format_hdf5);
                if (dataset->definition == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }
                num_dims = H5Sget_simple_extent_ndims(dataset->dataspace_id);
                if (num_dims < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }
                if (num_dims > CODA_MAX_NUM_DIMS)
                {
                    /* we don't support arrays with more dimensions than CODA can handle */
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return 1;
                }
                if (H5Sget_simple_extent_dims(dataset->dataspace_id, dim, NULL) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF5, NULL);
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }
                for (i = 0; i < num_dims; i++)
                {
                    if (coda_type_array_add_fixed_dimension(dataset->definition, (long)dim[i]) != 0)
                    {
                        coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                        return -1;
                    }
                }

                result = new_hdf5DataType(H5Dget_type(dataset->dataset_id), &dataset->base_type, 1);
                if (result < 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }
                if (result == 1)
                {
                    /* unsupported basic type -> ignore this dataset */
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return 1;
                }
                if (coda_type_array_set_base_type(dataset->definition, dataset->base_type->definition) != 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }

                dataset->attributes = new_hdf5AttributeRecord((coda_product *)product, dataset->dataset_id);
                if (dataset->attributes == NULL)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
                    return -1;
                }
                if (coda_type_set_attributes((coda_type *)dataset->definition, dataset->attributes->definition) != 0)
                {
                    coda_hdf5_type_delete((coda_dynamic_type *)dataset);
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
        for (i = (long)product->num_objects; i < (long)product->num_objects + BLOCK_SIZE; i++)
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
        for (i = 0; i < (long)num_objects; i++)
        {
            char *name;
            int length;

            length = H5Gget_objname_by_idx(group->group_id, i, NULL, 0);
            if (length < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
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
                return -1;
            }
            if (H5Gget_objname_by_idx(group->group_id, i, name, length + 1) < 0)
            {
                coda_set_error(CODA_ERROR_HDF5, NULL);
                free(name);
                return -1;
            }

            result = coda_hdf5_create_tree(product, group->group_id, name, &group->object[index]);
            if (result == -1)
            {
                free(name);
                return -1;
            }
            if (result == 1)
            {
                /* skip this object */
                free(name);
                continue;
            }
            if (coda_type_record_create_field(group->definition, name, group->object[index]->definition) != 0)
            {
                free(name);
                return -1;
            }
            free(name);

            /* increase number of unignored objects */
            index++;
        }
    }

    return 0;
}
