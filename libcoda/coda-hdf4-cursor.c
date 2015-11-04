/*
 * Copyright (C) 2007-2008 S&T, The Netherlands.
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

static int read_array_without_conversion(const coda_Cursor *cursor, void *dst);

static int get_native_type_size(coda_native_type type)
{
    switch (type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_char:
            return 1;
        case coda_native_type_int16:
        case coda_native_type_uint16:
            return 2;
        case coda_native_type_int32:
        case coda_native_type_uint32:
        case coda_native_type_float:
            return 4;
        case coda_native_type_int64:
        case coda_native_type_uint64:
        case coda_native_type_double:
            return 8;
        default:
            assert(0);
            exit(1);
    }
}

int coda_hdf4_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    cursor->pf = pf;
    cursor->n = 1;
    cursor->stack[0].type = pf->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for HDF4 backend */
    return 0;
}

int coda_hdf4_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *field_type;

    if (coda_hdf4_type_get_record_field_type((coda_Type *)cursor->stack[cursor->n - 1].type, index, &field_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)field_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    cursor->n--;
    if (coda_hdf4_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_hdf4_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    coda_Type *base_type;
    long offset_elements;
    int num_dims;
    long dim[MAX_HDF4_VAR_DIMS];
    int i;

    if (coda_hdf4_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, &num_dims, dim) != 0)
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

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = offset_elements;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    coda_Type *base_type;

    /* check the range for index */
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;

        if (coda_hdf4_cursor_get_num_elements(cursor, &num_elements) != 0)
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

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_DynamicType *)base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_next_array_element(coda_Cursor *cursor)
{
    if (coda_option_perform_boundary_checks)
    {
        long num_elements;
        long index;

        index = cursor->stack[cursor->n - 1].index + 1;

        cursor->n--;
        if (coda_hdf4_cursor_get_num_elements(cursor, &num_elements) != 0)
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

int coda_hdf4_cursor_goto_attributes(coda_Cursor *cursor)
{
    coda_hdf4Type *type;

    type = (coda_hdf4Type *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_hdf4_root:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4Root *)type)->attributes;
            break;
        case tag_hdf4_basic_type:
        case tag_hdf4_basic_type_array:
        case tag_hdf4_attributes:
        case tag_hdf4_file_attributes:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)coda_hdf4_empty_attributes();
            break;
        case tag_hdf4_GRImage:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4GRImage *)type)->attributes;
            break;
        case tag_hdf4_SDS:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4SDS *)type)->attributes;
            break;
        case tag_hdf4_Vdata:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4Vdata *)type)->attributes;
            break;
        case tag_hdf4_Vdata_field:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4VdataField *)type)->attributes;
            break;
        case tag_hdf4_Vgroup:
            cursor->stack[cursor->n - 1].type = (coda_DynamicType *)((coda_hdf4Vgroup *)type)->attributes;
            break;
        default:
            assert(0);
            exit(1);
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    coda_hdf4Type *type;

    type = (coda_hdf4Type *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_hdf4_root:
            *num_elements = ((coda_hdf4Root *)type)->num_entries;
            break;
        case tag_hdf4_basic_type:
            *num_elements = 1;
            break;
        case tag_hdf4_basic_type_array:
            *num_elements = ((coda_hdf4BasicTypeArray *)type)->count;
            break;
        case tag_hdf4_attributes:
            *num_elements = ((coda_hdf4Attributes *)type)->num_attributes;
            break;
        case tag_hdf4_file_attributes:
            *num_elements = ((coda_hdf4FileAttributes *)type)->num_attributes;
            break;
        case tag_hdf4_GRImage:
            *num_elements = ((coda_hdf4GRImage *)type)->num_elements;
            break;
        case tag_hdf4_SDS:
            *num_elements = ((coda_hdf4SDS *)type)->num_elements;
            break;
        case tag_hdf4_Vdata:
            *num_elements = ((coda_hdf4Vdata *)type)->num_fields;
            break;
        case tag_hdf4_Vdata_field:
            *num_elements = ((coda_hdf4VdataField *)type)->num_elements;
            break;
        case tag_hdf4_Vgroup:
            *num_elements = ((coda_hdf4Vgroup *)type)->num_entries;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_cursor_get_string_length(const coda_Cursor *cursor, long *length)
{
    return coda_hdf4_type_get_string_length((coda_Type *)cursor->stack[cursor->n - 1].type, length);
}

int coda_hdf4_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    return coda_hdf4_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, num_dims, dim);
}

static int coda_hdf4_read_attribute_sub(int32 tag, int32 attr_id, int32 attr_index, int32 field_index, int32 length,
                                        void *buffer)
{
    int result;

    result = 0;
    switch (tag)
    {
        case DFTAG_RI: /* GRImage attribute */
            result = GRgetattr(attr_id, attr_index, buffer);
            break;
        case DFTAG_SD: /* SDS attribute */
            result = SDreadattr(attr_id, attr_index, buffer);
            break;
        case DFTAG_VS: /* Vdata attribute */
            result = VSgetattr(attr_id, field_index, attr_index, buffer);
            break;
        case DFTAG_VG: /* Vgroup attribute */
            result = Vgetattr(attr_id, attr_index, buffer);
            break;
        case DFTAG_DIL:        /* data label annotation */
        case DFTAG_FID:        /* file label annotation */
            {
                char *label;

                /* labels receive a terminating zero from the HDF4 lib, so we need to read it using a larger buffer */
                label = malloc(length + 1);
                if (label == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)(length + 1), __FILE__, __LINE__);
                    return -1;
                }
                result = ANreadann(attr_id, label, length + 1);
                memcpy(buffer, label, length);
                free(label);
            }
            break;
        case DFTAG_DIA:        /* data description annotation */
        case DFTAG_FD: /* file description annotation */
            result = ANreadann(attr_id, buffer, length);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (result == -1)
    {
        coda_set_error(CODA_ERROR_HDF4, NULL);
        return -1;
    }

    return 0;
}

static int coda_hdf4_read_attribute(const coda_Cursor *cursor, void *dst)
{
    int32 count;
    long index;

    index = cursor->stack[cursor->n - 1].index;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag == tag_hdf4_basic_type_array)
    {
        count = ((coda_hdf4BasicTypeArray *)cursor->stack[cursor->n - 1].type)->count;
    }
    else
    {
        count = 1;
    }

    assert(cursor->n >= 2);
    switch (((coda_hdf4Type *)cursor->stack[cursor->n - 2].type)->tag)
    {
        case tag_hdf4_attributes:
            {
                coda_hdf4Attributes *type;

                type = (coda_hdf4Attributes *)cursor->stack[cursor->n - 2].type;
                if (cursor->stack[cursor->n - 1].index < type->num_obj_attributes)
                {
                    int32 tag = -1;

                    switch (type->parent_tag)
                    {
                        case tag_hdf4_GRImage:
                            tag = DFTAG_RI;
                            break;
                        case tag_hdf4_SDS:
                            tag = DFTAG_SD;
                            break;
                        case tag_hdf4_Vdata_field:
                        case tag_hdf4_Vdata:
                            tag = DFTAG_VS;
                            break;
                        case tag_hdf4_Vgroup:
                            tag = DFTAG_VG;
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    if (coda_hdf4_read_attribute_sub(tag, type->parent_id, index, type->field_index, count, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_obj_attributes + type->num_data_labels)
                {
                    if (coda_hdf4_read_attribute_sub(DFTAG_DIL, type->ann_id[index - type->num_obj_attributes], index -
                                                     type->num_obj_attributes, type->field_index, count, dst) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (coda_hdf4_read_attribute_sub(DFTAG_DIA, type->ann_id[index - type->num_obj_attributes],
                                                     index - type->num_obj_attributes - type->num_data_labels,
                                                     type->field_index, count, dst) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case tag_hdf4_file_attributes:
            {
                coda_hdf4FileAttributes *type;

                type = (coda_hdf4FileAttributes *)cursor->stack[cursor->n - 2].type;
                if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes)
                {
                    if (coda_hdf4_read_attribute_sub(DFTAG_RI, ((coda_hdf4ProductFile *)cursor->pf)->gr_id, index, -1,
                                                     count, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes + type->num_sd_attributes)
                {
                    if (coda_hdf4_read_attribute_sub(DFTAG_SD, ((coda_hdf4ProductFile *)cursor->pf)->sd_id,
                                                     index - type->num_gr_attributes, -1, count, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes + type->num_sd_attributes +
                         type->num_file_labels)
                {
                    int32 ann_id;

                    ann_id = ANselect(((coda_hdf4ProductFile *)cursor->pf)->an_id, index - type->num_gr_attributes -
                                      type->num_sd_attributes, AN_FILE_LABEL);
                    if (ann_id == -1)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                    if (coda_hdf4_read_attribute_sub(DFTAG_FID, ann_id, index - type->num_gr_attributes -
                                                     type->num_sd_attributes, -1, count, dst) != 0)
                    {
                        return -1;
                    }
                    if (ANendaccess(ann_id) != 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                }
                else
                {
                    int32 ann_id;

                    ann_id = ANselect(((coda_hdf4ProductFile *)cursor->pf)->an_id, index - type->num_gr_attributes -
                                      type->num_sd_attributes - type->num_file_labels, AN_FILE_DESC);
                    if (ann_id == -1)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                    if (coda_hdf4_read_attribute_sub(DFTAG_FD, ann_id, index - type->num_gr_attributes -
                                                     type->num_sd_attributes - type->num_file_labels, -1, count,
                                                     dst) != 0)
                    {
                        return -1;
                    }
                    if (ANendaccess(ann_id) != 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int coda_hdf4_read_basic_type(const coda_Cursor *cursor, void *dst)
{
    int32 start[MAX_HDF4_VAR_DIMS];
    int32 stride[MAX_HDF4_VAR_DIMS];
    int32 edge[MAX_HDF4_VAR_DIMS];
    long index;
    long i;

    index = cursor->stack[cursor->n - 1].index;

    assert(cursor->n > 1);
    switch (((coda_hdf4Type *)cursor->stack[cursor->n - 2].type)->tag)
    {
        case tag_hdf4_basic_type_array:
            {
                coda_Cursor array_cursor;
                char *buffer;
                int native_type_size;
                long num_elements;

                /* we first read the whole array and then return only the requested element */

                array_cursor = *cursor;
                array_cursor.n--;

                if (coda_cursor_get_num_elements(&array_cursor, &num_elements) != 0)
                {
                    return -1;
                }
                assert(index < num_elements);
                native_type_size = get_native_type_size(((coda_hdf4BasicType *)
                                                         cursor->stack[cursor->n - 1].type)->read_type);
                buffer = malloc(num_elements * native_type_size);
                if (buffer == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)(num_elements * native_type_size), __FILE__, __LINE__);
                    return -1;
                }

                if (read_array_without_conversion(&array_cursor, buffer) != 0)
                {
                    free(buffer);
                    return -1;
                }

                memcpy(dst, &buffer[index * native_type_size], native_type_size);
                free(buffer);
            }
            break;
        case tag_hdf4_attributes:
        case tag_hdf4_file_attributes:
            if (coda_hdf4_read_attribute(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case tag_hdf4_GRImage:
            {
                coda_hdf4GRImage *type;

                stride[0] = 1;
                stride[1] = 1;
                edge[0] = 1;
                edge[1] = 1;
                type = (coda_hdf4GRImage *)cursor->stack[cursor->n - 2].type;
                if (type->ncomp != 1)
                {
                    uint8 *buffer;
                    int component_size;
                    int component_index;

                    component_size = get_native_type_size(type->basic_type->read_type);

                    /* HDF4 does not allow reading a single component of a GRImage, so we have to first read all
                     * components and then return only the data item that was requested */
                    buffer = malloc(component_size * type->ncomp);
                    if (buffer == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       (long)(component_size * type->ncomp), __FILE__, __LINE__);
                        return -1;
                    }
                    component_index = index % type->ncomp;
                    index /= type->ncomp;
                    /* For GRImage data the first dimension is the fastest running */
                    start[0] = index % type->dim_sizes[0];
                    start[1] = index / type->dim_sizes[0];
                    if (GRreadimage(type->ri_id, start, stride, edge, buffer) != 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        free(buffer);
                        return -1;
                    }
                    memcpy(dst, &buffer[component_index * component_size], component_size);
                    free(buffer);
                }
                else
                {
                    /* For GRImage data the first dimension is the fastest running */
                    start[0] = index % type->dim_sizes[0];
                    start[1] = index / type->dim_sizes[0];
                    if (GRreadimage(type->ri_id, start, stride, edge, dst) != 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                }
            }
            break;
        case tag_hdf4_SDS:
            {
                coda_hdf4SDS *type;

                type = (coda_hdf4SDS *)cursor->stack[cursor->n - 2].type;
                for (i = type->rank - 1; i >= 0; i--)
                {
                    start[i] = index % type->dimsizes[i];
                    index /= type->dimsizes[i];
                    stride[i] = 1;
                    edge[i] = 1;
                }
                if (SDreaddata(type->sds_id, start, stride, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_Vdata_field:
            {
                coda_hdf4Vdata *type;
                coda_hdf4VdataField *field_type;
                int record_pos;
                int order_pos;

                assert(cursor->n > 2);
                type = (coda_hdf4Vdata *)cursor->stack[cursor->n - 3].type;
                field_type = (coda_hdf4VdataField *)cursor->stack[cursor->n - 2].type;
                order_pos = index % field_type->order;
                record_pos = index / field_type->order;
                if (VSseek(type->vdata_id, record_pos) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSsetfields(type->vdata_id, field_type->name) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (field_type->order > 1)
                {
                    /* HDF4 does not allow reading part of a vdata field, so we have to first read the full field and
                     * then return only the data item that was requested */
                    uint8 *buffer;
                    int element_size;
                    int size;

                    size = VSsizeof(type->vdata_id, field_type->name);
                    if (size < 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                    buffer = malloc(size);
                    if (buffer == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       (long)size, __FILE__, __LINE__);
                        return -1;
                    }
                    if (VSread(type->vdata_id, buffer, 1, FULL_INTERLACE) < 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        free(buffer);
                        return -1;
                    }
                    /* the size of a field element is the field size divided by the order of the field */
                    element_size = size / field_type->order;
                    memcpy(dst, &buffer[order_pos * element_size], element_size);
                    free(buffer);
                }
                else
                {
                    if (VSread(type->vdata_id, (uint8 *)dst, 1, FULL_INTERLACE) < 0)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int read_array_without_conversion(const coda_Cursor *cursor, void *dst)
{
    int32 start[MAX_HDF4_VAR_DIMS];
    int32 stride[MAX_HDF4_VAR_DIMS];
    int32 edge[MAX_HDF4_VAR_DIMS];
    long i;

    switch (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_hdf4_basic_type_array:
            if (coda_hdf4_read_attribute(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case tag_hdf4_GRImage:
            {
                coda_hdf4GRImage *type;

                type = (coda_hdf4GRImage *)cursor->stack[cursor->n - 1].type;
                start[0] = 0;
                start[1] = 0;
                stride[0] = 1;
                stride[1] = 1;
                edge[0] = type->dim_sizes[0];
                edge[1] = type->dim_sizes[1];
                if (GRreadimage(type->ri_id, start, stride, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_SDS:
            {
                coda_hdf4SDS *type;

                type = (coda_hdf4SDS *)cursor->stack[cursor->n - 1].type;
                for (i = 0; i < type->rank; i++)
                {
                    start[i] = 0;
                    stride[i] = 1;
                    edge[i] = type->dimsizes[i];
                }
                if (SDreaddata(type->sds_id, start, stride, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_Vdata_field:
            {
                coda_hdf4Vdata *type;
                coda_hdf4VdataField *field_type;

                assert(cursor->n > 1);
                type = (coda_hdf4Vdata *)cursor->stack[cursor->n - 2].type;
                field_type = (coda_hdf4VdataField *)cursor->stack[cursor->n - 1].type;
                if (VSseek(type->vdata_id, 0) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSsetfields(type->vdata_id, field_type->name) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSread(type->vdata_id, (uint8 *)dst, type->num_records, FULL_INTERLACE) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int coda_hdf4_read_array(const coda_Cursor *cursor, void *dst, coda_native_type from_type,
                                coda_native_type to_type, coda_array_ordering array_ordering)
{
    char *buffer;
    long num_elements;
    int element_to_size = 0;
    long to_size;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long i;

    if (coda_hdf4_cursor_get_num_elements(cursor, &num_elements) != 0)
    {
        return -1;
    }
    if (num_elements <= 0)
    {
        /* no data to be read */
        return 0;
    }
    if (coda_hdf4_type_get_array_dim((coda_Type *)cursor->stack[cursor->n - 1].type, &num_dims, dim) != 0)
    {
        return -1;
    }

    if (from_type == to_type && (num_dims <= 1 || array_ordering == coda_array_ordering_c))
    {
        /* no conversion needed */
        return read_array_without_conversion(cursor, dst);
    }

    element_to_size = get_native_type_size(to_type);

    to_size = element_to_size * num_elements;
    buffer = malloc(to_size);
    if (buffer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)", (long)to_size,
                       __FILE__, __LINE__);
        return -1;
    }

    if (read_array_without_conversion(cursor, buffer) != 0)
    {
        free(buffer);
        return -1;
    }

    if (num_dims <= 1 || array_ordering == coda_array_ordering_c)
    {
        /* requested array ordering is the same as that of the source */

        for (i = 0; i < num_elements; i++)
        {
            switch (to_type)
            {
                case coda_native_type_int16:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((int16_t *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((int16_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_uint16:
                    switch (from_type)
                    {
                        case coda_native_type_uint8:
                            ((uint16_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_int32:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((int32_t *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((int32_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((int32_t *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((int32_t *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_uint32:
                    switch (from_type)
                    {
                        case coda_native_type_uint8:
                            ((uint32_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((uint32_t *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_int64:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((int64_t *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((int64_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((int64_t *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((int64_t *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_int32:
                            ((int64_t *)dst)[i] = ((int32_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((int64_t *)dst)[i] = ((uint32_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_uint64:
                    switch (from_type)
                    {
                        case coda_native_type_uint8:
                            ((uint64_t *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((uint64_t *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((uint64_t *)dst)[i] = ((uint32_t *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_float:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((float *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((float *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((float *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((float *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_int32:
                            ((float *)dst)[i] = (float)((int32_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((float *)dst)[i] = (float)((uint32_t *)buffer)[i];
                            break;
                        case coda_native_type_int64:
                            ((float *)dst)[i] = (float)((int64_t *)buffer)[i];
                            break;
                        case coda_native_type_uint64:
                            ((float *)dst)[i] = (float)(int64_t)((uint64_t *)buffer)[i];
                            break;
                        case coda_native_type_double:
                            ((float *)dst)[i] = (float)((double *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                case coda_native_type_double:
                    switch (from_type)
                    {
                        case coda_native_type_int8:
                            ((double *)dst)[i] = ((int8_t *)buffer)[i];
                            break;
                        case coda_native_type_uint8:
                            ((double *)dst)[i] = ((uint8_t *)buffer)[i];
                            break;
                        case coda_native_type_int16:
                            ((double *)dst)[i] = ((int16_t *)buffer)[i];
                            break;
                        case coda_native_type_uint16:
                            ((double *)dst)[i] = ((uint16_t *)buffer)[i];
                            break;
                        case coda_native_type_int32:
                            ((double *)dst)[i] = ((int32_t *)buffer)[i];
                            break;
                        case coda_native_type_uint32:
                            ((double *)dst)[i] = ((uint32_t *)buffer)[i];
                            break;
                        case coda_native_type_int64:
                            ((double *)dst)[i] = (double)((int64_t *)buffer)[i];
                            break;
                        case coda_native_type_uint64:
                            ((double *)dst)[i] = (double)(int64_t)((uint64_t *)buffer)[i];
                            break;
                        case coda_native_type_float:
                            ((double *)dst)[i] = ((float *)buffer)[i];
                            break;
                        default:
                            assert(0);
                            exit(1);
                    }
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* requested array ordering differs from that of the source */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        assert(num_elements == incr[num_dims]);

        c_index = 0;
        fortran_index = 0;
        for (;;)
        {
            do
            {
                switch (to_type)
                {
                    case coda_native_type_int8:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((int8_t *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_uint8:
                        switch (from_type)
                        {
                            case coda_native_type_uint8:
                                ((uint8_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_int16:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((int16_t *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((int16_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((int16_t *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_uint16:
                        switch (from_type)
                        {
                            case coda_native_type_uint8:
                                ((uint16_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((uint16_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_int32:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((int32_t *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((int32_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((int32_t *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((int32_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((int32_t *)dst)[fortran_index] = ((int32_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_uint32:
                        switch (from_type)
                        {
                            case coda_native_type_uint8:
                                ((uint32_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((uint32_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((uint32_t *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_int64:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((int64_t *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((int64_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((int64_t *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((int64_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((int64_t *)dst)[fortran_index] = ((int32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((int64_t *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int64:
                                ((int64_t *)dst)[fortran_index] = ((int64_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_uint64:
                        switch (from_type)
                        {
                            case coda_native_type_uint8:
                                ((uint64_t *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((uint64_t *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((uint64_t *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint64:
                                ((uint64_t *)dst)[fortran_index] = ((uint64_t *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_float:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((float *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((float *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((float *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((float *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((float *)dst)[fortran_index] = (float)((int32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((float *)dst)[fortran_index] = (float)((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int64:
                                ((float *)dst)[fortran_index] = (float)((int64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint64:
                                ((float *)dst)[fortran_index] = (float)(int64_t)((uint64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_float:
                                ((float *)dst)[fortran_index] = ((float *)buffer)[c_index];
                                break;
                            case coda_native_type_double:
                                ((float *)dst)[fortran_index] = (float)((double *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_double:
                        switch (from_type)
                        {
                            case coda_native_type_int8:
                                ((double *)dst)[fortran_index] = ((int8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint8:
                                ((double *)dst)[fortran_index] = ((uint8_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int16:
                                ((double *)dst)[fortran_index] = ((int16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint16:
                                ((double *)dst)[fortran_index] = ((uint16_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int32:
                                ((double *)dst)[fortran_index] = ((int32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint32:
                                ((double *)dst)[fortran_index] = ((uint32_t *)buffer)[c_index];
                                break;
                            case coda_native_type_int64:
                                ((double *)dst)[fortran_index] = (double)((int64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_uint64:
                                ((double *)dst)[fortran_index] = (double)(int64_t)((uint64_t *)buffer)[c_index];
                                break;
                            case coda_native_type_float:
                                ((double *)dst)[fortran_index] = ((float *)buffer)[c_index];
                                break;
                            case coda_native_type_double:
                                ((double *)dst)[fortran_index] = ((double *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    case coda_native_type_char:
                        switch (from_type)
                        {
                            case coda_native_type_char:
                                ((char *)dst)[fortran_index] = ((char *)buffer)[c_index];
                                break;
                            default:
                                assert(0);
                                exit(1);
                        }
                        break;
                    default:
                        assert(0);
                        exit(1);
                }

                c_index++;
                fortran_index += increment;

            } while (fortran_index < num_elements);

            if (c_index == num_elements)
            {
                break;
            }

            fortran_index += incr[num_dims - 2] - incr[num_dims];
            i = num_dims - 3;
            while (i >= 0 && fortran_index >= incr[i + 2])
            {
                fortran_index += incr[i] - incr[i + 2];
                i--;
            }
        }
    }

    free(buffer);
    return 0;
}

int coda_hdf4_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int8 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int8_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint8 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint8_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int16 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int16_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int16_t)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int16_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint16_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint16_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int32 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint32 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint32_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint32_t)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint32_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int64 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (coda_option_perform_conversions && type->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint64 data type");
        return -1;
    }
    switch (type->read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }

    return 0;
}

int coda_hdf4_cursor_read_float(const coda_Cursor *cursor, float *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)(int64_t)value;
            }
            break;
        case coda_native_type_float:
            {
                float value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_double:
            {
                double value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }
    if (coda_option_perform_conversions && type->has_conversion)
    {
        *dst = (float)(*dst * type->scale_factor + type->add_offset);
    }

    return 0;
}

int coda_hdf4_cursor_read_double(const coda_Cursor *cursor, double *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    switch (type->read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)(int64_t)value;
            }
            break;
        case coda_native_type_float:
            {
                float value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_double:
            {
                double value;

                if (coda_hdf4_read_basic_type(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                           coda_type_get_native_type_name(type->read_type));
            return -1;
    }
    if (coda_option_perform_conversions && type->has_conversion)
    {
        *dst = *dst * type->scale_factor + type->add_offset;
    }

    return 0;
}

int coda_hdf4_cursor_read_char(const coda_Cursor *cursor, char *dst)
{
    coda_hdf4BasicType *type;

    if (((coda_hdf4Type *)cursor->stack[cursor->n - 1].type)->tag != tag_hdf4_basic_type)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
        return -1;
    }

    type = (coda_hdf4BasicType *)cursor->stack[cursor->n - 1].type;
    if (type->read_type != coda_native_type_char)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                       coda_type_get_native_type_name(type->read_type));
        return -1;
    }

    return coda_hdf4_read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size)
{
    /* HDF4 only supports single characters as basic type, so the string length is always 1 */
    if (dst_size > 1)
    {
        if (coda_hdf4_cursor_read_char(cursor, dst) != 0)
        {
            return -1;
        }
        dst[1] = '\0';
    }
    else if (dst_size == 1)
    {
        dst[0] = '\0';
    }

    return 0;
}

int coda_hdf4_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int8 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_int8, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint8 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_uint8, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int16 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_int16, array_ordering);
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_int16, array_ordering);
        case coda_native_type_int16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int16, coda_native_type_int16, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_uint16, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_uint16, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int32 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_int32, array_ordering);
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_int32, array_ordering);
        case coda_native_type_int16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int16, coda_native_type_int32, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_int32, array_ordering);
        case coda_native_type_int32:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int32, coda_native_type_int32, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint32 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_uint32, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_uint32, array_ordering);
        case coda_native_type_uint32:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_uint32, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int64 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_int64, array_ordering);
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_int64, array_ordering);
        case coda_native_type_int16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int16, coda_native_type_int64, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_int64, array_ordering);
        case coda_native_type_int32:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int32, coda_native_type_int64, array_ordering);
        case coda_native_type_uint32:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_int64, array_ordering);
        case coda_native_type_int64:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_int64, coda_native_type_int64, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint64 data type");
        return -1;
    }
    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_uint8:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_uint64, array_ordering);
        case coda_native_type_uint16:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_uint64, array_ordering);
        case coda_native_type_uint32:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_uint64, array_ordering);
        case coda_native_type_uint64:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_uint64, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}

int coda_hdf4_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;
    int result;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_uint8:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_int16:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int16, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_uint16:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_int32:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int32, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_uint32:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_int64:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int64, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_uint64:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_float:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_float, coda_native_type_float, array_ordering);
            break;
        case coda_native_type_double:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_double, coda_native_type_float, array_ordering);
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                           coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
            return -1;
    }
    if (result != 0)
    {
        return -1;
    }

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        double add_offset;
        double scale_factor;
        long num_elements;
        long i;

        add_offset = ((coda_hdf4BasicType *)base_type)->add_offset;
        scale_factor = ((coda_hdf4BasicType *)base_type)->scale_factor;
        if (coda_hdf4_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }

        for (i = 0; i < num_elements; i++)
        {
            ((float *)dst)[i] = (float)(scale_factor * ((float *)dst)[i] + add_offset);
        }
    }

    return 0;
}

int coda_hdf4_cursor_read_double_array(const coda_Cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;
    int result;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_int8:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int8, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_uint8:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_uint8, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_int16:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int16, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_uint16:
            result =
                coda_hdf4_read_array(cursor, dst, coda_native_type_uint16, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_int32:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int32, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_uint32:
            result =
                coda_hdf4_read_array(cursor, dst, coda_native_type_uint32, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_int64:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_int64, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_uint64:
            result =
                coda_hdf4_read_array(cursor, dst, coda_native_type_uint64, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_float:
            result = coda_hdf4_read_array(cursor, dst, coda_native_type_float, coda_native_type_double, array_ordering);
            break;
        case coda_native_type_double:
            result =
                coda_hdf4_read_array(cursor, dst, coda_native_type_double, coda_native_type_double, array_ordering);
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                           coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
            return -1;
    }
    if (result != 0)
    {
        return -1;
    }

    if (coda_option_perform_conversions && ((coda_hdf4BasicType *)base_type)->has_conversion)
    {
        double add_offset;
        double scale_factor;
        long num_elements;
        long i;

        add_offset = ((coda_hdf4BasicType *)base_type)->add_offset;
        scale_factor = ((coda_hdf4BasicType *)base_type)->scale_factor;
        if (coda_hdf4_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }

        for (i = 0; i < num_elements; i++)
        {
            ((double *)dst)[i] = scale_factor * ((double *)dst)[i] + add_offset;
        }
    }

    return 0;
}

int coda_hdf4_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_Type *base_type;

    if (coda_hdf4_type_get_array_base_type((coda_Type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }
    assert(((coda_hdf4Type *)base_type)->tag == tag_hdf4_basic_type);

    switch (((coda_hdf4BasicType *)base_type)->read_type)
    {
        case coda_native_type_char:
            return coda_hdf4_read_array(cursor, dst, coda_native_type_char, coda_native_type_char, array_ordering);
        default:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                   coda_type_get_native_type_name(((coda_hdf4BasicType *)base_type)->read_type));
    return -1;
}
