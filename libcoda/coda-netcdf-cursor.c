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

#include "coda-internal.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-netcdf-internal.h"

#ifndef WORDS_BIGENDIAN
static void swap2(void *value)
{
    union
    {
        uint8_t as_bytes[2];
        int16_t as_int16;
    } v1, v2;

    v1.as_int16 = *(int16_t *)value;

    v2.as_bytes[0] = v1.as_bytes[1];
    v2.as_bytes[1] = v1.as_bytes[0];

    *(int16_t *)value = v2.as_int16;
}

static void swap4(void *value)
{
    union
    {
        uint8_t as_bytes[4];
        int32_t as_int32;
    } v1, v2;

    v1.as_int32 = *(int32_t *)value;

    v2.as_bytes[0] = v1.as_bytes[3];
    v2.as_bytes[1] = v1.as_bytes[2];
    v2.as_bytes[2] = v1.as_bytes[1];
    v2.as_bytes[3] = v1.as_bytes[0];

    *(int32_t *)value = v2.as_int32;
}

static void swap8(void *value)
{
    union
    {
        uint8_t as_bytes[8];
        int64_t as_int64;
    } v1, v2;

    v1.as_int64 = *(int64_t *)value;

    v2.as_bytes[0] = v1.as_bytes[7];
    v2.as_bytes[1] = v1.as_bytes[6];
    v2.as_bytes[2] = v1.as_bytes[5];
    v2.as_bytes[3] = v1.as_bytes[4];
    v2.as_bytes[4] = v1.as_bytes[3];
    v2.as_bytes[5] = v1.as_bytes[2];
    v2.as_bytes[6] = v1.as_bytes[1];
    v2.as_bytes[7] = v1.as_bytes[0];

    *(int64_t *)value = v2.as_int64;
}
#endif

static int read_bytes(coda_netcdfProductFile *product_file, int64_t byte_offset, int64_t length, void *dst)
{
    if (((uint64_t)byte_offset + length) > ((uint64_t)product_file->file_size))
    {
        coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
        return -1;
    }
    if (product_file->use_mmap)
    {
        memcpy(dst, product_file->mmap_ptr + byte_offset, (size_t)length);
    }
    else
    {
#if HAVE_PREAD
        if (pread(product_file->fd, dst, (size_t)length, (off_t) byte_offset) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#else
        if (lseek(product_file->fd, (off_t) byte_offset, SEEK_SET) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %f in file %s (%s)",
                           (double)byte_offset, product_file->filename, strerror(errno));
            return -1;
        }
        if (read(product_file->fd, dst, (size_t)length) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#endif
    }

    return 0;
}

int coda_netcdf_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    cursor->pf = pf;
    cursor->n = 1;
    cursor->stack[0].type = pf->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for netCDF backend */

    return 0;
}

int coda_netcdf_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *field_type;

    if (coda_netcdf_type_get_record_field_type((coda_Type *)cursor->stack[cursor->n - 1].type, index, &field_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)field_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */
    return 0;
}

int coda_netcdf_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    cursor->n--;
    if (coda_netcdf_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_netcdf_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    coda_Type *base_type;
    long offset_elements;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long i;

    if (coda_netcdf_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, &num_dims, dim) != 0)
    {
        return -1;
    }

    /* check the number of dimensions */
    if (num_subs != num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d) (%s:%u)", num_subs,
                       num_dims, __FILE__, __LINE__);
        return -1;
    }

    /* check the dimensions... */
    offset_elements = 0;
    for (i = 0; i < num_dims; i++)
    {
        if (subs[i] < 0 || subs[i] >= dim[i])
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld) (%s:%u)",
                           subs[i], dim[i], __FILE__, __LINE__);
            return -1;
        }
        if (i > 0)
        {
            offset_elements *= dim[i];
        }
        offset_elements += subs[i];
    }

    if (coda_netcdf_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_netcdf_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *base_type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        if (coda_netcdf_cursor_get_num_elements(cursor, &num_elements) != 0)
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

    if (coda_netcdf_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_netcdf_cursor_goto_next_array_element(coda_Cursor *cursor)
{
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;
        long index;

        index = cursor->stack[cursor->n - 1].index + 1;

        cursor->n--;
        if (coda_netcdf_cursor_get_num_elements(cursor, &num_elements) != 0)
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

    cursor->stack[cursor->n - 1].index++;

    return 0;
}

int coda_netcdf_cursor_goto_attributes(coda_Cursor *cursor)
{
    coda_netcdfType *type;

    type = (coda_netcdfType *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_netcdf_array:
        case tag_netcdf_attribute_record:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_netcdf_empty_attribute_record();
            break;
        case tag_netcdf_root:
            if (((coda_netcdfRoot *)type)->attributes != NULL)
            {
                cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_netcdfRoot *)type)->attributes;
            }
            else
            {
                cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_netcdf_empty_attribute_record();
            }
            break;
        case tag_netcdf_basic_type:
            if (((coda_netcdfBasicType *)type)->attributes != NULL)
            {
                cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_netcdfBasicType *)type)->attributes;
            }
            else
            {
                cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_netcdf_empty_attribute_record();
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for netCDF backend */

    return 0;
}

int coda_netcdf_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    coda_netcdfType *type;

    type = (coda_netcdfType *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_netcdf_root:
            *num_elements = (long)((coda_netcdfRoot *)type)->num_variables;
            break;
        case tag_netcdf_array:
            *num_elements = (long)((coda_netcdfArray *)type)->num_elements;
            break;
        case tag_netcdf_basic_type:
            *num_elements = 1;
            break;
        case tag_netcdf_attribute_record:
            *num_elements = ((coda_netcdfAttributeRecord *)type)->num_attributes;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_netcdf_cursor_get_string_length(const coda_Cursor *cursor, long *length)
{
    return coda_netcdf_type_get_string_length((coda_Type *)cursor->stack[cursor->n - 1].type, length);
}

int coda_netcdf_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    return coda_netcdf_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, num_dims, dim);
}

static int read_array(const coda_Cursor *cursor, void *dst)
{
    coda_netcdfArray *type;
    coda_netcdfProductFile *pf;
    long block_size;
    long num_blocks;
    long i;

    type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    pf = (coda_netcdfProductFile *)cursor->pf;

    block_size = type->base_type->byte_size * type->num_elements;
    num_blocks = 1;
    if (type->base_type->record_var)
    {
        num_blocks = type->dim[0];
        block_size /= num_blocks;
    }

    for (i = 0; i < num_blocks; i++)
    {
        if (read_bytes(pf, type->base_type->offset + i * pf->record_size, block_size,
                       &((uint8_t *)dst)[i * block_size]) != 0)
        {
            return -1;
        }
    }

#ifndef WORDS_BIGENDIAN
    switch (type->base_type->byte_size)
    {
        case 1:
            /* no endianness conversion needed */
            break;
        case 2:
            for (i = 0; i < type->num_elements; i++)
            {
                swap2(&((int16_t *)dst)[i]);
            }
            break;
        case 4:
            for (i = 0; i < type->num_elements; i++)
            {
                swap4(&((int32_t *)dst)[i]);
            }
            break;
        case 8:
            for (i = 0; i < type->num_elements; i++)
            {
                swap8(&((int64_t *)dst)[i]);
            }
            break;
        default:
            assert(0);
            exit(1);
    }
#endif

    return 0;
}

static int read_basic_type(const coda_Cursor *cursor, void *dst, long size_boundary)
{
    coda_netcdfBasicType *type;
    coda_netcdfProductFile *pf;
    int64_t offset;

    type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;
    pf = (coda_netcdfProductFile *)cursor->pf;
    offset = type->offset;

    if (cursor->stack[cursor->n - 2].type->type_class == coda_array_class)
    {
        if (type->record_var)
        {
            coda_netcdfArray *array = (coda_netcdfArray *)cursor->stack[cursor->n - 2].type;
            long num_sub_elements;
            long record_index;

            num_sub_elements = array->num_elements / array->dim[0];
            record_index = cursor->stack[cursor->n - 1].index / num_sub_elements;
            /* jump to record */
            offset += record_index * pf->record_size;
            /* jump to sub element in record */
            offset += (cursor->stack[cursor->n - 1].index - record_index * num_sub_elements) * type->byte_size;
        }
        else
        {
            offset += cursor->stack[cursor->n - 1].index * type->byte_size;
        }
    }

    if (size_boundary >= 0 && type->byte_size > size_boundary)
    {
        if (read_bytes(pf, offset, size_boundary, dst) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (read_bytes(pf, offset, type->byte_size, dst) != 0)
        {
            return -1;
        }
    }

#ifndef WORDS_BIGENDIAN
    if (type->type_class == coda_integer_class || type->type_class == coda_real_class)
    {
        switch (type->byte_size)
        {
            case 1:
                /* no endianness conversion needed */
                break;
            case 2:
                swap2(dst);
                break;
            case 4:
                swap4(dst);
                break;
            case 8:
                swap8(dst);
                break;
            default:
                assert(0);
                exit(1);
        }
    }
#endif

    return 0;
}

int coda_netcdf_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int8 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
            return read_basic_type(cursor, dst, -1);

        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint8 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            return read_basic_type(cursor, dst, -1);

        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int16 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int16_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int16_t)(*(uint8_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            if (type->read_type == coda_native_type_uint8)
            {
                *dst = (uint16_t)(*(uint8_t *)dst);
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int32 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int32_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int32_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (int32_t)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (int32_t)(*(uint16_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint32 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_uint8:
                    *dst = (uint32_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (uint32_t)(*(uint16_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int64 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_int8:
                    *dst = (int64_t)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (int64_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (int64_t)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (int64_t)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (int64_t)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (int64_t)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint64 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_uint8:
                    *dst = (uint64_t)(*(uint8_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (uint64_t)(*(uint16_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (uint64_t)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_float(const coda_Cursor *cursor, float *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_float:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_int8:
                    *dst = (float)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (float)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (float)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (float)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (float)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (float)(*(uint32_t *)dst);
                    break;
                default:
                    break;
            }
            if (coda_option_perform_conversions && type->has_conversion)
            {
                *dst = (float)(*dst * type->scale_factor + type->add_offset);
            }
            return 0;
        case coda_native_type_int64:
        case coda_native_type_uint64:
            {
                uint64_t buffer;

                if (read_basic_type(cursor, &buffer, -1) != 0)
                {
                    return -1;
                }
                if (type->read_type == coda_native_type_int64)
                {
                    *dst = (float)(*(int64_t *)&buffer);
                }
                else
                {
                    *dst = (float)(int64_t)(*(uint64_t *)&buffer);
                }
                if (coda_option_perform_conversions && type->has_conversion)
                {
                    *dst = (float)(*dst * type->scale_factor + type->add_offset);
                }
            }
            return 0;
        case coda_native_type_double:
            {
                double buffer;

                if (read_basic_type(cursor, &buffer, -1) != 0)
                {
                    return -1;
                }
                if (coda_option_perform_conversions && type->has_conversion)
                {
                    *dst = (float)(buffer * type->scale_factor + type->add_offset);
                }
                else
                {
                    *dst = (float)buffer;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_double(const coda_Cursor *cursor, double *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    switch (type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_float:
        case coda_native_type_double:
            if (read_basic_type(cursor, dst, -1) != 0)
            {
                return -1;
            }
            switch (type->read_type)
            {
                case coda_native_type_int8:
                    *dst = (double)(*(int8_t *)dst);
                    break;
                case coda_native_type_uint8:
                    *dst = (double)(*(uint8_t *)dst);
                    break;
                case coda_native_type_int16:
                    *dst = (double)(*(int16_t *)dst);
                    break;
                case coda_native_type_uint16:
                    *dst = (double)(*(uint16_t *)dst);
                    break;
                case coda_native_type_int32:
                    *dst = (double)(*(int32_t *)dst);
                    break;
                case coda_native_type_uint32:
                    *dst = (double)(*(uint32_t *)dst);
                    break;
                case coda_native_type_int64:
                    *dst = (double)(*(int64_t *)dst);
                    break;
                case coda_native_type_uint64:
                    *dst = (double)(int64_t)(*(uint64_t *)dst);
                    break;
                case coda_native_type_float:
                    *dst = (double)(*(float *)dst);
                    break;
                default:
                    break;
            }
            if (coda_option_perform_conversions && type->has_conversion)
            {
                *dst = *dst * type->scale_factor + type->add_offset;
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_char(const coda_Cursor *cursor, char *dst)
{
    coda_netcdfBasicType *type = (coda_netcdfBasicType *)cursor->stack[cursor->n - 1].type;

    switch (type->read_type)
    {
        case coda_native_type_char:
            return read_basic_type(cursor, dst, -1);

        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                   coda_type_get_native_type_name(type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size)
{
    if (read_basic_type(cursor, dst, dst_size) != 0)
    {
        return -1;
    }
    dst[dst_size - 1] = '\0';
    return 0;
}

int coda_netcdf_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int8 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint8 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int16 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int16_t *)dst)[i] = (int16_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int16_t *)dst)[i] = (int16_t)((uint8_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 2) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint16_t *)dst)[i] = (uint16_t)((uint8_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 2) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int32 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int32_t *)dst)[i] = (int32_t)((uint16_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 4) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint32 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint32_t *)dst)[i] = (uint32_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint32_t *)dst)[i] = (uint32_t)((uint16_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 4) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int64 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((int64_t *)dst)[i] = (int64_t)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    if (coda_option_perform_conversions && type->base_type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint64 data type");
        return -1;
    }
    switch (type->base_type->read_type)
    {
        case coda_native_type_uint8:
        case coda_native_type_uint16:
        case coda_native_type_uint32:
        case coda_native_type_uint64:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((uint64_t *)dst)[i] = (uint64_t)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 8) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_float:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((float *)dst)[i] = (float)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((float *)dst)[i] = (float)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((float *)dst)[i] = (float)((uint32_t *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 8) != 0)
                {
                    return -1;
                }
            }
            if (coda_option_perform_conversions && type->base_type->has_conversion)
            {
                double add_offset;
                double scale_factor;

                add_offset = type->base_type->add_offset;
                scale_factor = type->base_type->scale_factor;
                for (i = 0; i < num_elements; i++)
                {
                    ((float *)dst)[i] = (float)(scale_factor * ((float *)dst)[i] + add_offset);
                }
            }
            return 0;
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_double:
            {
                void *buffer;

                /* we need to read data with 8 byte element size, while a float has only 4 bytes */
                /* we therefore read the data first into a buffer that can fit the full array of 8 byte elements */
                buffer = malloc(num_elements * 8);
                if (buffer == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   num_elements * 8, __FILE__, __LINE__);
                    return -1;
                }
                if (read_array(cursor, buffer) != 0)
                {
                    free(buffer);
                    return -1;
                }
                switch (type->base_type->read_type)
                {
                    case coda_native_type_int64:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)((int64_t *)buffer)[i];
                        }
                        break;
                    case coda_native_type_uint64:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)(int64_t)((uint64_t *)buffer)[i];
                        }
                        break;
                    case coda_native_type_double:
                        for (i = 0; i < num_elements; i++)
                        {
                            ((float *)dst)[i] = (float)((double *)buffer)[i];
                        }
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
                free(buffer);
                if (array_ordering != coda_array_ordering_c)
                {
                    if (coda_array_transpose(dst, type->num_dims, type->dim, 4) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_option_perform_conversions && type->base_type->has_conversion)
                {
                    double add_offset;
                    double scale_factor;

                    add_offset = type->base_type->add_offset;
                    scale_factor = type->base_type->scale_factor;
                    for (i = 0; i < num_elements; i++)
                    {
                        ((float *)dst)[i] = (float)(scale_factor * ((float *)dst)[i] + add_offset);
                    }
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_double_array(const coda_Cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;
    long num_elements = type->num_elements;
    long i;

    switch (type->base_type->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_float:
        case coda_native_type_double:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            switch (type->base_type->read_type)
            {
                case coda_native_type_int8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint8:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint8_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint16:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint16_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((int32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint32:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((uint32_t *)dst)[i];
                    }
                    break;
                case coda_native_type_int64:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((double *)dst)[i] = (double)((int64_t *)dst)[i];
                    }
                    break;
                case coda_native_type_uint64:
                    for (i = 0; i < num_elements; i++)
                    {
                        ((double *)dst)[i] = (double)(int64_t)((uint64_t *)dst)[i];
                    }
                    break;
                case coda_native_type_float:
                    for (i = num_elements - 1; i >= 0; i--)
                    {
                        ((double *)dst)[i] = (double)((float *)dst)[i];
                    }
                    break;
                default:
                    break;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 8) != 0)
                {
                    return -1;
                }
            }
            if (coda_option_perform_conversions && type->base_type->has_conversion)
            {
                double add_offset;
                double scale_factor;

                add_offset = type->base_type->add_offset;
                scale_factor = type->base_type->scale_factor;
                if (type->base_type->has_fill_value)
                {
                    double fill_value;

                    fill_value = type->base_type->fill_value;
                    for (i = 0; i < num_elements; i++)
                    {
                        if (((double *)dst)[i] == fill_value)
                        {
                            ((double *)dst)[i] = coda_NaN();
                        }
                        else
                        {
                            ((double *)dst)[i] = scale_factor * ((double *)dst)[i] + add_offset;
                        }
                    }
                }
                else
                {
                    for (i = 0; i < num_elements; i++)
                    {
                        ((double *)dst)[i] = scale_factor * ((double *)dst)[i] + add_offset;
                    }
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}

int coda_netcdf_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_netcdfArray *type = (coda_netcdfArray *)cursor->stack[cursor->n - 1].type;

    switch (type->base_type->read_type)
    {
        case coda_native_type_char:
            if (read_array(cursor, dst) != 0)
            {
                return -1;
            }
            if (array_ordering != coda_array_ordering_c)
            {
                if (coda_array_transpose(dst, type->num_dims, type->dim, 1) != 0)
                {
                    return -1;
                }
            }
            return 0;
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                   coda_type_get_native_type_name(type->base_type->read_type));
    return -1;
}
