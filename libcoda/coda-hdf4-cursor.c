/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

static int get_array_base_type(const coda_hdf4_type *type, coda_hdf4_type **base_type)
{
    switch (type->tag)
    {
        case tag_hdf4_basic_type_array:
            *base_type = ((coda_hdf4_basic_type_array *)type)->basic_type;
            break;
        case tag_hdf4_GRImage:
            *base_type = ((coda_hdf4_GRImage *)type)->basic_type;
            break;
        case tag_hdf4_SDS:
            *base_type = ((coda_hdf4_SDS *)type)->basic_type;
            break;
        case tag_hdf4_Vdata_field:
            *base_type = ((coda_hdf4_Vdata_field *)type)->basic_type;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_hdf4_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for HDF4 backend */
    return 0;
}

int coda_hdf4_cursor_goto_record_field_by_index(coda_cursor *cursor, long index)
{
    coda_hdf4_type *record_type;
    coda_hdf4_type *field_type;

    record_type = (coda_hdf4_type *)cursor->stack[cursor->n - 1].type;
    switch (record_type->tag)
    {
        case tag_hdf4_attributes:
            if (index < 0 || index >= ((coda_hdf4_attributes *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf4_attributes *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = ((coda_hdf4_attributes *)record_type)->attribute[index];
            break;
        case tag_hdf4_file_attributes:
            if (index < 0 || index >= ((coda_hdf4_file_attributes *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf4_file_attributes *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = ((coda_hdf4_file_attributes *)record_type)->attribute[index];
            break;
        case tag_hdf4_Vdata:
            if (index < 0 || index >= ((coda_hdf4_Vdata *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf4_Vdata *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = (coda_hdf4_type *)((coda_hdf4_Vdata *)record_type)->field[index];
            break;
        case tag_hdf4_Vgroup:
            if (index < 0 || index >= ((coda_hdf4_Vgroup *)record_type)->definition->num_fields)
            {
                coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld)", index,
                               ((coda_hdf4_Vgroup *)record_type)->definition->num_fields);
                return -1;
            }
            field_type = ((coda_hdf4_Vgroup *)record_type)->entry[index];
            break;
        default:
            assert(0);
            exit(1);
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)field_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_next_record_field(coda_cursor *cursor)
{
    cursor->n--;
    if (coda_hdf4_cursor_goto_record_field_by_index(cursor, cursor->stack[cursor->n].index + 1) != 0)
    {
        cursor->n++;
        return -1;
    }
    return 0;
}

int coda_hdf4_cursor_goto_array_element(coda_cursor *cursor, int num_subs, const long subs[])
{
    coda_hdf4_type *base_type;
    long index;
    int num_dims;
    long dim[MAX_HDF4_VAR_DIMS];
    int i;

    if (coda_hdf4_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    /* check the number of dimensions */
    if (num_subs != num_dims)
    {
        coda_set_error(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH,
                       "number of dimensions argument (%d) does not match rank of array (%d)", num_subs, num_dims);
        return -1;
    }

    /* check the dimensions... */
    index = 0;
    for (i = 0; i < num_dims; i++)
    {
        if (subs[i] < 0 || subs[i] >= dim[i])
        {
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", subs[i],
                           dim[i]);
            return -1;
        }
        if (i > 0)
        {
            index *= dim[i];
        }
        index += subs[i];
    }

    if (get_array_base_type((coda_hdf4_type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_array_element_by_index(coda_cursor *cursor, long index)
{
    coda_hdf4_type *base_type;

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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    if (get_array_base_type((coda_hdf4_type *)cursor->stack[cursor->n - 1].type, &base_type) != 0)
    {
        return -1;
    }

    cursor->n++;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)base_type;
    cursor->stack[cursor->n - 1].index = index;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_goto_next_array_element(coda_cursor *cursor)
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
            coda_set_error(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, "array index (%ld) exceeds array range [0:%ld)", index,
                           num_elements);
            return -1;
        }
    }

    cursor->stack[cursor->n - 1].index++;

    return 0;
}

int coda_hdf4_cursor_goto_attributes(coda_cursor *cursor)
{
    coda_hdf4_type *type;

    type = (coda_hdf4_type *)cursor->stack[cursor->n - 1].type;
    cursor->n++;
    switch (type->tag)
    {
        case tag_hdf4_GRImage:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf4_GRImage *)type)->attributes;
            break;
        case tag_hdf4_SDS:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf4_SDS *)type)->attributes;
            break;
        case tag_hdf4_Vdata:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf4_Vdata *)type)->attributes;
            break;
        case tag_hdf4_Vdata_field:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf4_Vdata_field *)type)->attributes;
            break;
        case tag_hdf4_Vgroup:
            cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)((coda_hdf4_Vgroup *)type)->attributes;
            break;
        default:
            cursor->stack[cursor->n - 1].type = coda_mem_empty_record(coda_format_hdf4);
            break;
    }

    /* we use the special index value '-1' to indicate that we are pointing to the attributes of the parent */
    cursor->stack[cursor->n - 1].index = -1;
    cursor->stack[cursor->n - 1].bit_offset = -1;       /* not applicable for HDF4 backend */

    return 0;
}

int coda_hdf4_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    coda_hdf4_type *type;

    type = (coda_hdf4_type *)cursor->stack[cursor->n - 1].type;
    switch (type->tag)
    {
        case tag_hdf4_basic_type:
            *num_elements = 1;
            break;
        case tag_hdf4_basic_type_array:
            *num_elements = ((coda_hdf4_basic_type_array *)type)->definition->num_elements;
            break;
        case tag_hdf4_string:
            *num_elements = 1;
            break;
        case tag_hdf4_attributes:
            *num_elements = ((coda_hdf4_attributes *)type)->definition->num_fields;
            break;
        case tag_hdf4_file_attributes:
            *num_elements = ((coda_hdf4_file_attributes *)type)->definition->num_fields;
            break;
        case tag_hdf4_GRImage:
            *num_elements = ((coda_hdf4_GRImage *)type)->definition->num_elements;
            break;
        case tag_hdf4_SDS:
            *num_elements = ((coda_hdf4_SDS *)type)->definition->num_elements;
            break;
        case tag_hdf4_Vdata:
            *num_elements = ((coda_hdf4_Vdata *)type)->definition->num_fields;
            break;
        case tag_hdf4_Vdata_field:
            *num_elements = ((coda_hdf4_Vdata_field *)type)->definition->num_elements;
            break;
        case tag_hdf4_Vgroup:
            *num_elements = ((coda_hdf4_Vgroup *)type)->definition->num_fields;
            break;
    }

    return 0;
}

int coda_hdf4_cursor_get_string_length(const coda_cursor *cursor, long *length)
{
    coda_hdf4_type *type;

    type = (coda_hdf4_type *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_hdf4_string)
    {
        return coda_type_get_string_length(type->definition, length);
    }

    *length = 1;

    return 0;
}

int coda_hdf4_cursor_get_array_dim(const coda_cursor *cursor, int *num_dims, long dim[])
{
    coda_hdf4_type *type;
    int i;

    type = (coda_hdf4_type *)cursor->stack[cursor->n - 1].type;
    switch (((coda_hdf4_type *)type)->tag)
    {
        case tag_hdf4_basic_type_array:
            *num_dims = 1;
            dim[0] = ((coda_hdf4_basic_type_array *)type)->definition->num_elements;
            break;
        case tag_hdf4_GRImage:
            /* The C interface to GRImage data uses fortran array ordering, so we swap the dimensions */
            dim[0] = ((coda_hdf4_GRImage *)type)->dim_sizes[1];
            dim[1] = ((coda_hdf4_GRImage *)type)->dim_sizes[0];
            if (((coda_hdf4_GRImage *)type)->ncomp != 1)
            {
                *num_dims = 3;
                dim[2] = ((coda_hdf4_GRImage *)type)->ncomp;
            }
            else
            {
                *num_dims = 2;
            }
            break;
        case tag_hdf4_SDS:
            *num_dims = ((coda_hdf4_SDS *)type)->rank;
            for (i = 0; i < ((coda_hdf4_SDS *)type)->rank; i++)
            {
                dim[i] = ((coda_hdf4_SDS *)type)->dimsizes[i];
            }
            break;
        case tag_hdf4_Vdata_field:
            if (((coda_hdf4_Vdata_field *)type)->order > 1)
            {
                *num_dims = 2;
                dim[1] = ((coda_hdf4_Vdata_field *)type)->order;
            }
            else
            {
                *num_dims = 1;
            }
            dim[0] = ((coda_hdf4_Vdata_field *)type)->num_records;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int read_attribute_sub(int32 tag, int32 attr_id, int32 attr_index, int32 field_index, int32 length, void *buffer)
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
            /* labels receive a terminating zero from the HDF4 lib, so we need to read it using one byte extra */
            /* the buffer should have already been made large enough for this by coda_hdf4_cursor_read_string */
            result = ANreadann(attr_id, buffer, length + 1);
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

static int read_attribute(const coda_cursor *cursor, void *dst, long length)
{
    long index;

    index = cursor->stack[cursor->n - 1].index;

    assert(cursor->n >= 2);
    switch (((coda_hdf4_type *)cursor->stack[cursor->n - 2].type)->tag)
    {
        case tag_hdf4_attributes:
            {
                coda_hdf4_attributes *type;

                type = (coda_hdf4_attributes *)cursor->stack[cursor->n - 2].type;
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
                    if (read_attribute_sub(tag, type->parent_id, index, type->field_index, length, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_obj_attributes + type->num_data_labels)
                {
                    if (read_attribute_sub(DFTAG_DIL, type->ann_id[index - type->num_obj_attributes],
                                           index - type->num_obj_attributes, type->field_index, length, dst) != 0)
                    {
                        return -1;
                    }
                }
                else
                {
                    if (read_attribute_sub(DFTAG_DIA, type->ann_id[index - type->num_obj_attributes],
                                           index - type->num_obj_attributes - type->num_data_labels,
                                           type->field_index, length, dst) != 0)
                    {
                        return -1;
                    }
                }
            }
            break;
        case tag_hdf4_file_attributes:
            {
                coda_hdf4_file_attributes *type;

                type = (coda_hdf4_file_attributes *)cursor->stack[cursor->n - 2].type;
                if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes)
                {
                    if (read_attribute_sub(DFTAG_RI, ((coda_hdf4_product *)cursor->product)->gr_id, index, -1,
                                           length, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes + type->num_sd_attributes)
                {
                    if (read_attribute_sub(DFTAG_SD, ((coda_hdf4_product *)cursor->product)->sd_id,
                                           index - type->num_gr_attributes, -1, length, dst) != 0)
                    {
                        return -1;
                    }
                }
                else if (cursor->stack[cursor->n - 1].index < type->num_gr_attributes + type->num_sd_attributes +
                         type->num_file_labels)
                {
                    int32 ann_id;

                    ann_id = ANselect(((coda_hdf4_product *)cursor->product)->an_id, index - type->num_gr_attributes -
                                      type->num_sd_attributes, AN_FILE_LABEL);
                    if (ann_id == -1)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                    if (read_attribute_sub(DFTAG_FID, ann_id, index - type->num_gr_attributes -
                                           type->num_sd_attributes, -1, length, dst) != 0)
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

                    ann_id = ANselect(((coda_hdf4_product *)cursor->product)->an_id, index - type->num_gr_attributes -
                                      type->num_sd_attributes - type->num_file_labels, AN_FILE_DESC);
                    if (ann_id == -1)
                    {
                        coda_set_error(CODA_ERROR_HDF4, NULL);
                        return -1;
                    }
                    if (read_attribute_sub(DFTAG_FD, ann_id, index - type->num_gr_attributes -
                                           type->num_sd_attributes - type->num_file_labels, -1, length, dst) != 0)
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

static int read_array(const coda_cursor *cursor, void *dst)
{
    int32 start[MAX_HDF4_VAR_DIMS];
    int32 stride[MAX_HDF4_VAR_DIMS];
    int32 edge[MAX_HDF4_VAR_DIMS];
    long num_elements;
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

    switch (((coda_hdf4_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_hdf4_basic_type_array:
            if (read_attribute(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case tag_hdf4_GRImage:
            {
                coda_hdf4_GRImage *type;

                type = (coda_hdf4_GRImage *)cursor->stack[cursor->n - 1].type;
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
                coda_hdf4_SDS *type;

                type = (coda_hdf4_SDS *)cursor->stack[cursor->n - 1].type;
                if (type->rank == 0)
                {
                    start[0] = 0;
                    edge[0] = 1;
                }
                else
                {
                    for (i = 0; i < type->rank; i++)
                    {
                        start[i] = 0;
                        edge[i] = type->dimsizes[i];
                    }
                }
                if (SDreaddata(type->sds_id, start, NULL, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_Vdata_field:
            {
                coda_hdf4_Vdata *type;
                coda_hdf4_Vdata_field *field_type;

                assert(cursor->n > 1);
                type = (coda_hdf4_Vdata *)cursor->stack[cursor->n - 2].type;
                field_type = (coda_hdf4_Vdata_field *)cursor->stack[cursor->n - 1].type;
                if (VSseek(type->vdata_id, 0) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSsetfields(type->vdata_id, field_type->field_name) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSread(type->vdata_id, (uint8 *)dst, field_type->num_records, FULL_INTERLACE) < 0)
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

static int read_partial_array(const coda_cursor *cursor, long offset, long length, void *dst)
{
    int32 start[MAX_HDF4_VAR_DIMS];
    int32 stride[MAX_HDF4_VAR_DIMS];
    int32 edge[MAX_HDF4_VAR_DIMS];
    long i;

    switch (((coda_hdf4_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_hdf4_basic_type_array:
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading is not supported for HDF4 attributes");
            return -1;
        case tag_hdf4_GRImage:
            {
                coda_hdf4_GRImage *type;

                type = (coda_hdf4_GRImage *)cursor->stack[cursor->n - 1].type;
                stride[0] = 1;
                stride[1] = 1;
                if (length < type->dim_sizes[1])
                {
                    start[0] = offset / type->dim_sizes[1];
                    start[1] = offset % type->dim_sizes[1];
                    edge[0] = 1;
                    edge[1] = length;
                    if (start[1] + edge[1] > type->dim_sizes[1])
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 GRImage requires "
                                       "offset (%ld) and length (%ld) to represent a hyperslab (range [%ld,%ld] "
                                       "exceeds length of dimension #1 (%ld)))", offset, length, (long)start[1],
                                       (long)start[1] + edge[1] - 1, (long)type->dim_sizes[1]);
                        return -1;
                    }
                }
                else
                {
                    if (length % type->dim_sizes[1] != 0)
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 GRImage requires "
                                       "length (%ld) to be a multiple of the subdimension size (%ld)", length,
                                       (long)type->dim_sizes[1]);
                        return -1;
                    }
                    if (offset % type->dim_sizes[1] != 0)
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 GRImage requires "
                                       " offset (%ld) to be a multiple of the subdimension size (%ld)", offset,
                                       (long)type->dim_sizes[1]);
                        return -1;
                    }
                    start[0] = offset / type->dim_sizes[1];
                    start[1] = 0;
                    edge[0] = length / type->dim_sizes[1];
                    edge[1] = type->dim_sizes[1];
                    if (start[0] + edge[0] > type->dim_sizes[0])
                    {
                        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 GRImage requires "
                                       "offset (%ld) and length (%ld) to represent a hyperslab (range [%ld,%ld] "
                                       "exceeds length of dimension #0 (%ld)))", offset, length, (long)start[0],
                                       (long)start[0] + edge[0] - 1, (long)type->dim_sizes[0]);
                        return -1;
                    }
                }
                if (GRreadimage(type->ri_id, start, stride, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_SDS:
            {
                coda_hdf4_SDS *type;
                long block_size = 1;

                type = (coda_hdf4_SDS *)cursor->stack[cursor->n - 1].type;

                /* determine hyperslab start/edge */
                if (type->rank == 0)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading not allowed for zero "
                                   "dimensional HDF4 SDS");
                    return -1;
                }
                for (i = type->rank - 1; i >= 0; i--)
                {
                    if (length <= block_size * type->dimsizes[i])
                    {
                        if (length % block_size != 0)
                        {
                            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 SDS requires "
                                           "length (%ld) to be a multiple of the subdimension size (%ld)", length,
                                           block_size);
                            return -1;
                        }
                        start[i] = (offset / block_size) % type->dimsizes[i];
                        edge[i] = length / block_size;
                        break;
                    }
                    start[i] = 0;
                    edge[i] = type->dimsizes[i];
                    block_size *= type->dimsizes[i];
                }
                if (offset % block_size != 0)
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 SDS requires offset "
                                   "(%ld) to be a multiple of the subdimension size (%ld)", offset, block_size);
                    return -1;
                }
                if (start[i] + edge[i] > type->dimsizes[i])
                {
                    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading for HDF4 SDS requires offset "
                                   "(%ld) and length (%ld) to represent a hyperslab (range [%ld,%ld] exceeds length "
                                   "of dimension #%ld (%ld)))", offset, length, (long)start[i],
                                   (long)start[i] + edge[i] - 1, (long)i, (long)type->dimsizes[i]);
                    return -1;
                }
                while (i > 0)
                {
                    block_size *= type->dimsizes[i];
                    i--;
                    start[i] = (offset / block_size) % type->dimsizes[i];
                    edge[i] = 1;
                }

                if (SDreaddata(type->sds_id, start, NULL, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_Vdata_field:
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "partial array reading is not supported for HDF4 Vdata");
            return -1;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

static int read_basic_type(const coda_cursor *cursor, void *dst)
{
    int32 start[MAX_HDF4_VAR_DIMS];
    int32 stride[MAX_HDF4_VAR_DIMS];
    int32 edge[MAX_HDF4_VAR_DIMS];
    long index;
    long i;

    index = cursor->stack[cursor->n - 1].index;

    assert(cursor->n > 1);
    switch (((coda_hdf4_type *)cursor->stack[cursor->n - 2].type)->tag)
    {
        case tag_hdf4_basic_type_array:
            {
                coda_cursor array_cursor;
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
                native_type_size =
                    get_native_type_size(((coda_hdf4_type *)cursor->stack[cursor->n - 1].type)->definition->read_type);
                buffer = malloc(num_elements * native_type_size);
                if (buffer == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)(num_elements * native_type_size), __FILE__, __LINE__);
                    return -1;
                }

                if (read_attribute(&array_cursor, buffer, -1) != 0)
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
            if (read_attribute(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case tag_hdf4_GRImage:
            {
                coda_hdf4_GRImage *type;

                stride[0] = 1;
                stride[1] = 1;
                edge[0] = 1;
                edge[1] = 1;
                type = (coda_hdf4_GRImage *)cursor->stack[cursor->n - 2].type;
                if (type->ncomp != 1)
                {
                    uint8 *buffer;
                    int component_size;
                    int component_index;

                    component_size = get_native_type_size(type->basic_type->definition->read_type);

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
                coda_hdf4_SDS *type;

                type = (coda_hdf4_SDS *)cursor->stack[cursor->n - 2].type;
                if (type->rank == 0)
                {
                    start[0] = 0;
                    edge[1] = 1;
                }
                else
                {
                    for (i = type->rank - 1; i >= 0; i--)
                    {
                        start[i] = index % type->dimsizes[i];
                        index /= type->dimsizes[i];
                        edge[i] = 1;
                    }
                }
                if (SDreaddata(type->sds_id, start, NULL, edge, dst) != 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
            }
            break;
        case tag_hdf4_Vdata_field:
            {
                coda_hdf4_Vdata *type;
                coda_hdf4_Vdata_field *field_type;
                int record_pos;
                int order_pos;

                assert(cursor->n > 2);
                type = (coda_hdf4_Vdata *)cursor->stack[cursor->n - 3].type;
                field_type = (coda_hdf4_Vdata_field *)cursor->stack[cursor->n - 2].type;
                order_pos = index % field_type->order;
                record_pos = index / field_type->order;
                if (VSseek(type->vdata_id, record_pos) < 0)
                {
                    coda_set_error(CODA_ERROR_HDF4, NULL);
                    return -1;
                }
                if (VSsetfields(type->vdata_id, field_type->field_name) != 0)
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

                    size = VSsizeof(type->vdata_id, field_type->field_name);
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

int coda_hdf4_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    return read_basic_type(cursor, dst);
}

int coda_hdf4_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    if (((coda_hdf4_type *)cursor->stack[cursor->n - 1].type)->tag == tag_hdf4_string)
    {
        long length;

        if (coda_hdf4_cursor_get_string_length(cursor, &length) != 0)
        {
            return -1;
        }
        if (dst_size >= length + 1)
        {
            /* we can directly read into the destination buffer */
            if (read_attribute(cursor, dst, length) != 0)
            {
                return -1;
            }
            dst[length] = '\0';
        }
        else
        {
            char *buffer;

            /* we first read the whole string and then return only the requested part */

            buffer = malloc(length + 1);        /* add 1, because the AN interface depend on this additional byte */
            if (buffer == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               length + 1, __FILE__, __LINE__);
                return -1;
            }

            if (read_attribute(cursor, buffer, length) != 0)
            {
                free(buffer);
                return -1;
            }

            memcpy(dst, buffer, dst_size - 1);
            free(buffer);
            dst[dst_size - 1] = '\0';
        }
    }
    else
    {
        /* basic type should be a single character, so the string length is always 1 */
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
    }

    return 0;
}

int coda_hdf4_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_float_array(const coda_cursor *cursor, float *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_double_array(const coda_cursor *cursor, double *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_char_array(const coda_cursor *cursor, char *dst)
{
    return read_array(cursor, dst);
}

int coda_hdf4_cursor_read_int8_partial_array(const coda_cursor *cursor, long offset, long length, int8_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_uint8_partial_array(const coda_cursor *cursor, long offset, long length, uint8_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_int16_partial_array(const coda_cursor *cursor, long offset, long length, int16_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_uint16_partial_array(const coda_cursor *cursor, long offset, long length, uint16_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_int32_partial_array(const coda_cursor *cursor, long offset, long length, int32_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_uint32_partial_array(const coda_cursor *cursor, long offset, long length, uint32_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_int64_partial_array(const coda_cursor *cursor, long offset, long length, int64_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_uint64_partial_array(const coda_cursor *cursor, long offset, long length, uint64_t *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_float_partial_array(const coda_cursor *cursor, long offset, long length, float *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_double_partial_array(const coda_cursor *cursor, long offset, long length, double *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}

int coda_hdf4_cursor_read_char_partial_array(const coda_cursor *cursor, long offset, long length, char *dst)
{
    return read_partial_array(cursor, offset, length, dst);
}
