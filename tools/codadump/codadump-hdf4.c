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

#include "codadump.h"

#ifdef HAVE_HDF4

#define MAX_BLOCK_SIZE (4*1024*1024)
#define MIN_SDS_FILL_EFFICIENCY 0.15
#define IGNORE_FILL_EFFICIENCY_SIZE (1*1024*1024)

static void write_data(int depth, int array_depth, int record_depth);

static void handle_hdf4_error()
{
    fprintf(stderr, "ERROR: HDF error\n");
    HEprint(stderr, 0);
    fflush(stderr);
    exit(1);
}

void hdf4_info_init()
{
    hdf4_info.hdf_vfile_id = Hopen(output_file_name, DFACC_CREATE, 0);
    if (hdf4_info.hdf_vfile_id == -1)
    {
        fprintf(stderr, "ERROR: Could not create HDF4 file \"%s\"\n", output_file_name);
        exit(1);
    }
    if (Vstart(hdf4_info.hdf_vfile_id) == -1)
    {
        fprintf(stderr, "ERROR: Could not initialize HDF4 Vdata/Vgroup interface\n");
        exit(1);
    }
    hdf4_info.hdf_file_id = SDstart(output_file_name, DFACC_WRITE);
    if (hdf4_info.hdf_file_id == -1)
    {
        fprintf(stderr, "ERROR: Could not initialize HDF4 SD interface\n");
        exit(1);
    }
    hdf4_info.vgroup_depth = 0;
}

void hdf4_info_done()
{
    SDend(hdf4_info.hdf_file_id);
    Vend(hdf4_info.hdf_vfile_id);
    Hclose(hdf4_info.hdf_vfile_id);
}

static const char *hdf_type_name(int32 type)
{
    switch (type)
    {
        case DFNT_CHAR:
            return "char";
        case DFNT_UCHAR:
            return "uchar";
        case DFNT_INT8:
            return "int8";
        case DFNT_UINT8:
            return "uint8";
        case DFNT_INT16:
            return "int16";
        case DFNT_UINT16:
            return "uint16";
        case DFNT_INT32:
            return "int32";
        case DFNT_UINT32:
            return "uint32";
        case DFNT_FLOAT32:
            return "float";
        case DFNT_FLOAT64:
            return "double";
        default:
            return "unknown";
    }
}

static int32 dd_type_to_hdf_type(coda_Type *type)
{
    coda_type_class type_class;

    if (coda_type_get_class(type, &type_class) != 0)
    {
        handle_coda_error();
    }

    switch (type_class)
    {
        case coda_record_class:
        case coda_array_class:
            assert(0);
            exit(1);
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(type, &read_type) != 0)
                {
                    handle_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_not_available:
                        /* can not be exported */
                        break;
                    case coda_native_type_int8:
                        return DFNT_INT8;
                    case coda_native_type_uint8:
                        return DFNT_UINT8;
                    case coda_native_type_int16:
                        return DFNT_INT16;
                    case coda_native_type_uint16:
                        return DFNT_UINT16;
                    case coda_native_type_int32:
                        return DFNT_INT32;
                    case coda_native_type_uint32:
                        return DFNT_UINT32;
                    case coda_native_type_int64:
                    case coda_native_type_uint64:
                        /* DFNT_INT64 and DFNT_UINT64 are unfortunately not yet supported in HDF4 */
                        return DFNT_FLOAT64;
                    case coda_native_type_float:
                        return DFNT_FLOAT32;
                    case coda_native_type_double:
                        return DFNT_FLOAT64;
                    case coda_native_type_char:
                    case coda_native_type_string:
                    case coda_native_type_bytes:
                        return DFNT_CHAR;
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(type, &special_type) != 0)
                {
                    handle_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_no_data:
                        assert(0);
                        exit(1);
                    case coda_special_vsf_integer:
                    case coda_special_time:
                    case coda_special_complex:
                        return DFNT_FLOAT64;
                }
            }
            break;
    }

    return -1;
}

static int32 sizeof_hdf_type(int32 type)
{
    switch (type)
    {
        case DFNT_INT8:
        case DFNT_UINT8:
        case DFNT_CHAR:
            return 1;
        case DFNT_INT16:
        case DFNT_UINT16:
            return 2;
        case DFNT_INT32:
        case DFNT_UINT32:
            return 4;
        case DFNT_FLOAT32:
            return 4;
        case DFNT_FLOAT64:
            return 8;
        default:
            return -1;
    }
}

static void *hdf_fill_value(int32 type)
{
    static char char_fill_value = ' ';
    static int8_t int8_fill_value = (int8_t)0x80;
    static uint8_t uint8_fill_value = 0xFF;
    static int16_t int16_fill_value = (int16_t)0x8000;
    static uint16_t uint16_fill_value = 0xFFFF;
    static int32_t int32_fill_value = 0x80000000;
    static uint32_t uint32_fill_value = 0xFFFFFFFF;
    static float float_fill_value;
    static double double_fill_value;

    switch (type)
    {
        case DFNT_UINT8:
            return &uint8_fill_value;
        case DFNT_INT8:
            return &int8_fill_value;
        case DFNT_CHAR:
            return &char_fill_value;
        case DFNT_UINT16:
            return &uint16_fill_value;
        case DFNT_INT16:
            return &int16_fill_value;
        case DFNT_UINT32:
            return &uint32_fill_value;
        case DFNT_INT32:
            return &int32_fill_value;
        case DFNT_FLOAT32:
            float_fill_value = (float)sqrt(-1);
            return &float_fill_value;
        case DFNT_FLOAT64:
            double_fill_value = sqrt(-1);
            return &double_fill_value;
        default:
            return NULL;
    }
}

void hdf4_enter_record()
{
    const char *description;

    if (traverse_info.num_records > 0)
    {
        hdf4_info.vgroup_id[hdf4_info.vgroup_depth] = Vattach(hdf4_info.hdf_vfile_id, -1, "w");
        if (hdf4_info.vgroup_id[hdf4_info.vgroup_depth] == -1)
        {
            handle_hdf4_error();
        }
        if (Vsetname(hdf4_info.vgroup_id[hdf4_info.vgroup_depth],
                     traverse_info.field_name[traverse_info.num_records - 1]) != 0)
        {
            handle_hdf4_error();
        }
        if (hdf4_info.vgroup_depth > 0)
        {
            Vinsert(hdf4_info.vgroup_id[hdf4_info.vgroup_depth - 1], hdf4_info.vgroup_id[hdf4_info.vgroup_depth]);
        }
        if (coda_type_get_description(traverse_info.type[traverse_info.current_depth], &description) != 0)
        {
            handle_coda_error();
        }
        if ((description != NULL) && (description[0] != '\0'))
        {
            if (Vsetattr(hdf4_info.vgroup_id[hdf4_info.vgroup_depth], "description", DFNT_CHAR,
                         strlen(description), description) != 0)
            {
                handle_hdf4_error();
            }
        }
        hdf4_info.vgroup_depth++;
    }
}

void hdf4_leave_record()
{
    if (traverse_info.num_records > 0)
    {
        hdf4_info.vgroup_depth--;
        if (Vdetach(hdf4_info.vgroup_id[hdf4_info.vgroup_depth]) != 0)
        {
            handle_hdf4_error();
        }
    }
}

void hdf4_enter_array()
{
    int dim_id;
    int num_dims;
    int i;

    num_dims = traverse_info.array_info[traverse_info.num_arrays].num_dims;
    dim_id = traverse_info.array_info[traverse_info.num_arrays].dim_id;

    for (i = 0; i < num_dims; i++)
    {
        int j;

        sprintf(hdf4_info.dim_name[dim_id + i], "DIM%03d", traverse_info.parent_index[0]);
        for (j = 1; j < traverse_info.num_records; j++)
        {
            sprintf(&hdf4_info.dim_name[dim_id + i][2 + j * 4], ":%03d", traverse_info.parent_index[j]);
        }
        sprintf(&hdf4_info.dim_name[dim_id + i][2 + traverse_info.num_records * 4], "-%02d", dim_id + i);
    }
}

void hdf4_leave_array()
{
}

static void create_hdf_data_block(int dim_id)
{
    int64_t data_size;

    data_size = dim_info.array_size[dim_id] * hdf4_info.sizeof_hdf_type;
    /* allocate memory */
    assert(data_size > 0);
    hdf4_info.data = malloc((size_t)data_size);
    if (hdf4_info.data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)data_size, __FILE__, __LINE__);
        handle_coda_error();
    }
}

static void destroy_hdf_data_block()
{
    assert(hdf4_info.data != NULL);
    free(hdf4_info.data);
    hdf4_info.data = NULL;
}

static void set_dim_names(int num_dims)
{
    int32 dimid;

    if (num_dims > 0)
    {
        int i;

        for (i = 0; i < num_dims; i++)
        {
            dimid = SDgetdimid(hdf4_info.sds_id, i);
            if (SDsetdimname(dimid, hdf4_info.dim_name[i]) == -1)
            {
                handle_hdf4_error();
            }
        }
    }
    else
    {
        dimid = SDgetdimid(hdf4_info.sds_id, 0);
        if (SDsetdimname(dimid, "SINGLE_ELEMENT_DIM") == -1)
        {
            handle_hdf4_error();
        }
    }
}

static void write_dims()
{
    int dim_id;

    for (dim_id = 0; dim_id < dim_info.num_dims; dim_id++)
    {
        if (dim_info.is_var_size_dim[dim_id])
        {
            char *sds_name;
            int32 start[MAX_NUM_DIMS];

            sds_name = malloc(strlen(traverse_info.field_name[traverse_info.num_records - 1]) + 10);
            if (sds_name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               strlen(traverse_info.field_name[traverse_info.num_records - 1]) + 10, __FILE__,
                               __LINE__);
                handle_coda_error();
            }
            sprintf(sds_name, "%s_dims{%d}", traverse_info.field_name[traverse_info.num_records - 1], dim_id + 1);
            hdf4_info.sds_id = SDcreate(hdf4_info.hdf_file_id, sds_name, DFNT_INT32, dim_info.var_dim_num_dims[dim_id],
                                        (int32 *)dim_info.dim);
            if (hdf4_info.sds_id == -1)
            {
                handle_hdf4_error();
            }
            set_dim_names(dim_info.var_dim_num_dims[dim_id]);
            SDsetfillvalue(hdf4_info.sds_id, hdf_fill_value(DFNT_INT32));

            Vaddtagref(hdf4_info.vgroup_id[hdf4_info.vgroup_depth - 1], DFTAG_NDG, SDidtoref(hdf4_info.sds_id));

            memset(start, 0, MAX_NUM_DIMS * sizeof(int32));
            if (SDwritedata(hdf4_info.sds_id, start, NULL, (int32 *)dim_info.dim, (void *)dim_info.var_dim[dim_id]) !=
                0)
            {
                handle_hdf4_error();
            }
            SDendaccess(hdf4_info.sds_id);
        }
    }
}

static void read_data(int depth, int array_depth)
{
    coda_type_class type_class;
    int result = 0;

    if (coda_type_get_class(traverse_info.type[depth], &type_class) != 0)
    {
        handle_coda_error();
    }
    switch (type_class)
    {
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(traverse_info.type[depth], &read_type) != 0)
                {
                    handle_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        result = coda_cursor_read_int8(&traverse_info.cursor,
                                                       (int8_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_uint8:
                        result = coda_cursor_read_uint8(&traverse_info.cursor,
                                                        (uint8_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_int16:
                        result = coda_cursor_read_int16(&traverse_info.cursor,
                                                        (int16_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_uint16:
                        result = coda_cursor_read_uint16(&traverse_info.cursor,
                                                         (uint16_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_int32:
                        result = coda_cursor_read_int32(&traverse_info.cursor,
                                                        (int32_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_uint32:
                        result = coda_cursor_read_uint32(&traverse_info.cursor,
                                                         (uint32_t *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_float:
                        result = coda_cursor_read_float(&traverse_info.cursor,
                                                        (float *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_int64:
                    case coda_native_type_uint64:
                    case coda_native_type_double:
                        result = coda_cursor_read_double(&traverse_info.cursor,
                                                         (double *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_char:
                        result = coda_cursor_read_char(&traverse_info.cursor,
                                                       (char *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_native_type_string:
                        {
                            char *str;
                            int length;

                            length = dim_info.dim[traverse_info.array_info[array_depth].dim_id];
                            str = malloc(length + 1);
                            if (str == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) "
                                               "(%s:%u)", (long)length, __FILE__, __LINE__);
                                handle_coda_error();
                            }
                            result = coda_cursor_read_string(&traverse_info.cursor, str, length + 1);
                            if (result == 0)
                            {
                                memcpy(&hdf4_info.data[hdf4_info.offset], str, length);
                            }
                            free(str);
                        }
                        break;
                    case coda_native_type_bytes:
                        {
                            int64_t bit_size;

                            if (coda_cursor_get_bit_size(&traverse_info.cursor, &bit_size) != 0)
                            {
                                handle_coda_error();
                            }
                            result = coda_cursor_read_bits(&traverse_info.cursor,
                                                           (uint8_t *)&hdf4_info.data[hdf4_info.offset], 0, bit_size);
                        }
                        break;
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(traverse_info.type[depth], &special_type) != 0)
                {
                    handle_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        result = coda_cursor_read_double(&traverse_info.cursor,
                                                         (double *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_special_complex:
                        result = coda_cursor_read_complex_double_pair(&traverse_info.cursor,
                                                                      (double *)&hdf4_info.data[hdf4_info.offset]);
                        break;
                    case coda_special_no_data:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_record_class:
        case coda_array_class:
            assert(0);
            exit(1);
    }
    if (result != 0)
    {
        handle_coda_error();
    }
    if (array_depth < traverse_info.num_arrays)
    {
        /* increase offset for compound basic types (complex, string, raw) */
        hdf4_info.offset += dim_info.dim[traverse_info.array_info[array_depth].dim_id] * hdf4_info.sizeof_hdf_type;
    }
    else
    {
        /* increase offset for singular basic types */
        hdf4_info.offset += hdf4_info.sizeof_hdf_type;
    }
}

static void read_array_data(int depth, int array_depth, int record_depth)
{
    coda_type_class type_class;

    if (coda_type_get_class(traverse_info.type[depth], &type_class) != 0)
    {
        handle_coda_error();
    }
    if (type_class == coda_array_class)
    {
        array_info_t *array_info;
        int number_of_elements;
        int traverse_array;
        int result = 0;
        int dim_id;
        int i;

        array_info = &traverse_info.array_info[array_depth];
        dim_id = array_info->dim_id;
        if (dim_info.is_var_size_dim[dim_id])
        {
            int i;

            number_of_elements = dim_info.var_dim[dim_id][array_info->global_index];
            for (i = 1; i < array_info->num_dims; i++)
            {
                number_of_elements *= array_info->dim[i];
            }
        }
        else
        {
            number_of_elements = array_info->num_elements;
        }
        assert(number_of_elements != 0);

        traverse_array = 0;

        if (coda_type_get_class(traverse_info.type[depth + 1], &type_class) != 0)
        {
            handle_coda_error();
        }
        /* look at array base type */
        switch (type_class)
        {
            case coda_record_class:
            case coda_array_class:
                traverse_array = 1;
                break;
            case coda_integer_class:
            case coda_real_class:
            case coda_text_class:
            case coda_raw_class:
                {
                    coda_native_type read_type;

                    if (coda_type_get_read_type(traverse_info.type[traverse_info.current_depth], &read_type) != 0)
                    {
                        handle_coda_error();
                    }
                    switch (read_type)
                    {
                        case coda_native_type_int8:
                            result = coda_cursor_read_int8_array(&traverse_info.cursor,
                                                                 (int8_t *)&hdf4_info.data[hdf4_info.offset],
                                                                 coda_array_ordering_c);
                            break;
                        case coda_native_type_uint8:
                            result = coda_cursor_read_uint8_array(&traverse_info.cursor,
                                                                  (uint8_t *)&hdf4_info.data[hdf4_info.offset],
                                                                  coda_array_ordering_c);
                            break;
                        case coda_native_type_int16:
                            result = coda_cursor_read_int16_array(&traverse_info.cursor,
                                                                  (int16_t *)&hdf4_info.data[hdf4_info.offset],
                                                                  coda_array_ordering_c);
                            break;
                        case coda_native_type_uint16:
                            result = coda_cursor_read_uint16_array(&traverse_info.cursor,
                                                                   (uint16_t *)&hdf4_info.data[hdf4_info.offset],
                                                                   coda_array_ordering_c);
                            break;
                        case coda_native_type_int32:
                            result = coda_cursor_read_int32_array(&traverse_info.cursor,
                                                                  (int32_t *)&hdf4_info.data[hdf4_info.offset],
                                                                  coda_array_ordering_c);
                            break;
                        case coda_native_type_uint32:
                            result = coda_cursor_read_uint32_array(&traverse_info.cursor,
                                                                   (uint32_t *)&hdf4_info.data[hdf4_info.offset],
                                                                   coda_array_ordering_c);
                            break;
                        case coda_native_type_float:
                            result = coda_cursor_read_float_array(&traverse_info.cursor,
                                                                  (float *)&hdf4_info.data[hdf4_info.offset],
                                                                  coda_array_ordering_c);
                            break;
                        case coda_native_type_int64:
                        case coda_native_type_uint64:
                        case coda_native_type_double:
                            result = coda_cursor_read_double_array(&traverse_info.cursor,
                                                                   (double *)&hdf4_info.data[hdf4_info.offset],
                                                                   coda_array_ordering_c);
                            break;
                        case coda_native_type_char:
                            result = coda_cursor_read_char_array(&traverse_info.cursor,
                                                                 (char *)&hdf4_info.data[hdf4_info.offset],
                                                                 coda_array_ordering_c);
                            break;
                        case coda_native_type_string:
                        case coda_native_type_bytes:
                            traverse_array = 1;
                            break;
                        case coda_native_type_not_available:
                            assert(0);
                            exit(1);
                    }
                }
                break;
            case coda_special_class:
                {
                    coda_special_type special_type;

                    if (coda_type_get_special_type(traverse_info.type[traverse_info.current_depth], &special_type) != 0)
                    {
                        handle_coda_error();
                    }
                    switch (special_type)
                    {
                        case coda_special_vsf_integer:
                        case coda_special_time:
                            result = coda_cursor_read_double_array(&traverse_info.cursor,
                                                                   (double *)&hdf4_info.data[hdf4_info.offset],
                                                                   coda_array_ordering_c);
                            break;
                        case coda_special_complex:
                            result = coda_cursor_read_complex_double_pairs_array(&traverse_info.cursor,
                                                                                 (double *)&hdf4_info.data[hdf4_info.
                                                                                                           offset],
                                                                                 coda_array_ordering_c);
                            break;
                        case coda_special_no_data:
                            assert(0);
                            exit(1);
                    }
                }
                break;
        }
        if (result != 0)
        {
            handle_coda_error();
        }
        if (traverse_array)
        {
            if (coda_cursor_goto_first_array_element(&traverse_info.cursor) != 0)
            {
                handle_coda_error();
            }
            for (i = 0; i < number_of_elements; i++)
            {
                /* read data for current array element */
                read_array_data(depth + 1, array_depth + 1, record_depth);

                if (i < number_of_elements - 1)
                {
                    if (coda_cursor_goto_next_array_element(&traverse_info.cursor) != 0)
                    {
                        handle_coda_error();
                    }
                }
            }
            coda_cursor_goto_parent(&traverse_info.cursor);
        }
        else
        {
            hdf4_info.offset += number_of_elements * hdf4_info.sizeof_hdf_type;
        }
    }
    else if (type_class == coda_record_class)
    {
        int available;

        if (coda_cursor_get_record_field_available_status(&traverse_info.cursor,
                                                          traverse_info.parent_index[record_depth], &available) != 0)
        {
            handle_coda_error();
        }
        if (available)
        {
            if (coda_cursor_goto_record_field_by_index(&traverse_info.cursor,
                                                       traverse_info.parent_index[record_depth]) != 0)
            {
                handle_coda_error();
            }
            read_array_data(depth + 1, array_depth, record_depth + 1);
            coda_cursor_goto_parent(&traverse_info.cursor);
        }
        else
        {
            void *fill_value;
            int num_elements = 1;
            int i;

            /* The record field is not available, so we have to fill the array block with fill values ourselves */
            fill_value = hdf_fill_value(hdf4_info.hdf_type);
            if (array_depth < traverse_info.num_arrays)
            {
                num_elements = (int)dim_info.array_size[traverse_info.array_info[array_depth].dim_id];
            }
            for (i = 0; i < num_elements; i++)
            {
                memcpy(&hdf4_info.data[hdf4_info.offset], fill_value, hdf4_info.sizeof_hdf_type);
                hdf4_info.offset += hdf4_info.sizeof_hdf_type;
            }
        }
    }
    else
    {
        read_data(depth, array_depth);
    }
}

static void write_data(int depth, int array_depth, int record_depth)
{
    coda_type_class type_class;

    if (coda_type_get_class(traverse_info.type[depth], &type_class) != 0)
    {
        handle_coda_error();
    }
    switch (type_class)
    {
        case coda_record_class:
            {
                int available;

                if (coda_cursor_get_record_field_available_status(&traverse_info.cursor,
                                                                  traverse_info.parent_index[record_depth],
                                                                  &available) != 0)
                {
                    handle_coda_error();
                }
                /* if the field is not available, don't write any data (HDF will fill the block with fill values) */
                if (available)
                {
                    if (coda_cursor_goto_record_field_by_index(&traverse_info.cursor,
                                                               traverse_info.parent_index[record_depth]) != 0)
                    {
                        handle_coda_error();
                    }
                    write_data(depth + 1, array_depth, record_depth + 1);
                    coda_cursor_goto_parent(&traverse_info.cursor);
                }
            }
            break;
        case coda_array_class:
            {
                array_info_t *array_info;
                int number_of_elements;
                int has_var_dim_sub_array;
                int local_dim[MAX_NUM_DIMS];
                int dim_id;
                int i;

                array_info = &traverse_info.array_info[array_depth];
                dim_id = array_info->dim_id;

                if (array_depth == 0)
                {
                    array_info->global_index = 0;
                }

                has_var_dim_sub_array = (dim_info.last_var_size_dim >= dim_id + array_info->num_dims);
                if (has_var_dim_sub_array && array_depth < traverse_info.num_arrays - 1)
                {
                    /* Set the index for the var_dim list(s) for the next array */
                    traverse_info.array_info[array_depth + 1].global_index =
                        array_info->global_index * array_info->num_elements;
                }

                /* calculate local dimensions and number of array elements */
                number_of_elements = 1;
                for (i = 0; i < array_info->num_dims; i++)
                {
                    if (dim_info.is_var_size_dim[dim_id + i])
                    {
                        local_dim[i] = dim_info.var_dim[dim_id + i][array_info->global_index];
                    }
                    else
                    {
                        local_dim[i] = dim_info.dim[dim_id + i];
                    }
                    number_of_elements *= local_dim[i];
                }
                if (number_of_elements == 0)
                {
                    /* array is empty */
                    return;
                }

                /* Only use a data buffer if the block size is small enough and the last variable sized dim is the
                 * first dim of the current array or lies before that dim (i.e. we need to be able to write the buffer
                 * as one contiguous block)
                 */
                if (dim_info.array_size[dim_id] <= MAX_BLOCK_SIZE && dim_info.last_var_size_dim <= dim_id)
                {
                    /* create data buffer */
                    create_hdf_data_block(dim_id);
                    hdf4_info.offset = 0;

                    /* read array */
                    read_array_data(depth, array_depth, record_depth);

                    /* write data buffer */
                    for (i = dim_id; i < dim_info.num_dims; i++)
                    {
                        hdf4_info.start[i] = 0;
                        hdf4_info.edges[i] = dim_info.dim[i];
                    }
                    if (dim_info.last_var_size_dim == dim_id)
                    {
                        /* if first dim is variable sized then use local_dim (= var_dim) value */
                        hdf4_info.edges[dim_id] = local_dim[0];
                    }
                    if (SDwritedata(hdf4_info.sds_id, hdf4_info.start, NULL, hdf4_info.edges, hdf4_info.data) != 0)
                    {
                        handle_hdf4_error();
                    }

                    /* destroy data buffer */
                    destroy_hdf_data_block();
                }
                else
                {
                    for (i = 0; i < array_info->num_dims; i++)
                    {
                        hdf4_info.start[dim_id + i] = 0;
                        hdf4_info.edges[dim_id + i] = 1;
                    }
                    /* traverse array */
                    if (coda_cursor_goto_first_array_element(&traverse_info.cursor) != 0)
                    {
                        handle_coda_error();
                    }
                    for (i = 0; i < number_of_elements; i++)
                    {
                        /* write data for current array element */
                        write_data(depth + 1, array_depth + 1, record_depth);

                        if (i < number_of_elements - 1)
                        {
                            int k;

                            /* increase current position */
                            k = array_info->num_dims - 1;
                            hdf4_info.start[dim_id + k]++;
                            while (hdf4_info.start[dim_id + k] == local_dim[k])
                            {
                                hdf4_info.start[dim_id + k] = 0;
                                k--;
                                hdf4_info.start[dim_id + k]++;
                            }
                            /* jump to next array element */
                            if (coda_cursor_goto_next_array_element(&traverse_info.cursor) != 0)
                            {
                                handle_coda_error();
                            }
                            if (has_var_dim_sub_array && array_depth < traverse_info.num_arrays - 1)
                            {
                                traverse_info.array_info[array_depth + 1].global_index++;
                            }
                        }
                    }
                    coda_cursor_goto_parent(&traverse_info.cursor);
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(traverse_info.type[depth], &read_type) != 0)
                {
                    handle_coda_error();
                }

                switch (read_type)
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
                    case coda_native_type_char:
                        {
                            unsigned char data[8];      /* buffer for maximum basic data type size */

                            hdf4_info.data = data;
                            hdf4_info.offset = 0;
                            read_data(depth, array_depth);
                            if (SDwritedata(hdf4_info.sds_id, hdf4_info.start, NULL, hdf4_info.edges, data) != 0)
                            {
                                handle_hdf4_error();
                            }
                            hdf4_info.data = NULL;
                        }
                        break;
                    case coda_native_type_string:
                    case coda_native_type_bytes:
                        {
                            int dim_id;

                            dim_id = traverse_info.array_info[array_depth].dim_id;
                            create_hdf_data_block(dim_id);
                            hdf4_info.offset = 0;
                            hdf4_info.start[dim_id] = 0;
                            hdf4_info.edges[dim_id] = dim_info.dim[dim_id];
                            read_data(depth, array_depth);
                            if (SDwritedata(hdf4_info.sds_id, hdf4_info.start, NULL, hdf4_info.edges,
                                            hdf4_info.data) != 0)
                            {
                                handle_hdf4_error();
                            }
                            destroy_hdf_data_block(dim_id);
                        }
                        break;
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(traverse_info.type[depth], &special_type) != 0)
                {
                    handle_coda_error();
                }

                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        {
                            unsigned char data[8];      /* buffer for double data type */

                            hdf4_info.data = data;
                            hdf4_info.offset = 0;
                            read_data(depth, array_depth);
                            if (SDwritedata(hdf4_info.sds_id, hdf4_info.start, NULL, hdf4_info.edges, data) != 0)
                            {
                                handle_hdf4_error();
                            }
                            hdf4_info.data = NULL;
                        }
                        break;
                    case coda_special_complex:
                        {
                            unsigned char data[16];     /* buffer for double compound data type */

                            hdf4_info.data = data;
                            hdf4_info.offset = 0;
                            hdf4_info.start[traverse_info.array_info[array_depth].dim_id] = 0;
                            hdf4_info.edges[traverse_info.array_info[array_depth].dim_id] = 2;
                            read_data(depth, array_depth);
                            if (SDwritedata(hdf4_info.sds_id, hdf4_info.start, NULL, hdf4_info.edges, data) != 0)
                            {
                                handle_hdf4_error();
                            }
                            hdf4_info.data = NULL;
                        }
                        break;
                    case coda_special_no_data:
                        assert(0);
                        exit(1);
                }
            }
            break;
    }
}

void export_data_element_to_hdf4()
{
    int64_t filled_size;
    int64_t size;
    int has_dyn_available_fields;
    int i;

    /* determine whether we have any dynamically available fields as parents */
    has_dyn_available_fields = 0;
    for (i = 0; i < traverse_info.current_depth; i++)
    {
        coda_type_class type_class;

        coda_type_get_class(traverse_info.type[i], &type_class);
        if (type_class == coda_record_class && traverse_info.field_available_status[i] == -1)
        {
            has_dyn_available_fields = 1;
            break;
        }
    }

    /* initialize HDF properties of dataset */
    hdf4_info.hdf_type = dd_type_to_hdf_type(traverse_info.type[traverse_info.current_depth]);
    hdf4_info.sizeof_hdf_type = sizeof_hdf_type(hdf4_info.hdf_type);
    assert(hdf4_info.hdf_type != -1);
    if (dim_info.num_dims > 0)
    {
        filled_size = dim_info.filled_num_elements[dim_info.num_dims - 1] * hdf4_info.sizeof_hdf_type;
        size = dim_info.num_elements[dim_info.num_dims - 1] * hdf4_info.sizeof_hdf_type;
    }
    else
    {
        filled_size = hdf4_info.sizeof_hdf_type;
        size = hdf4_info.sizeof_hdf_type;
    }

    if (verbosity > 0)
    {
        coda_native_type read_type;
        char s[21];

        if (coda_type_get_read_type(traverse_info.type[traverse_info.current_depth], &read_type) != 0)
        {
            handle_coda_error();
        }
        print_full_field_name(stdout, 2, 1);
        printf(" '%s'->'%s'", coda_type_get_native_type_name(read_type), hdf_type_name(hdf4_info.hdf_type));
        coda_str64(size, s);
        if (filled_size != size)
        {
            char s2[21];

            coda_str64(filled_size, s2);
            printf(" (%s/%s bytes)", s2, s);
        }
        else
        {
            printf(" (%s bytes)", s);
        }
        printf("\n");
    }

    if (filled_size == 0)
    {
        fprintf(stderr, "WARNING: field \"");
        print_full_field_name(stderr, 0, 0);
        fprintf(stderr, "\" ignored because it contains no elements.\n");
        return;
    }

    if ((filled_size / (double)size < MIN_SDS_FILL_EFFICIENCY) && (size - filled_size > IGNORE_FILL_EFFICIENCY_SIZE))
    {
        fprintf(stderr, "WARNING: field \"");
        print_full_field_name(stderr, 0, 0);
        fprintf(stderr, "\" ignored because HDF data set will be too sparse (%.0f%%)\n", (100.0 * filled_size) / size);
        return;
    }
    /* create HDF SDS */
    if (dim_info.num_dims > 0)
    {
        hdf4_info.sds_id = SDcreate(hdf4_info.hdf_file_id, traverse_info.field_name[traverse_info.num_records - 1],
                                    hdf4_info.hdf_type, dim_info.num_dims, (int32 *)dim_info.dim);
    }
    else
    {
        int32 fixed_dim[1] = { 1 };

        hdf4_info.sds_id = SDcreate(hdf4_info.hdf_file_id, traverse_info.field_name[traverse_info.num_records - 1],
                                    hdf4_info.hdf_type, 1, fixed_dim);
        hdf4_info.start[0] = 0;
        hdf4_info.edges[0] = 1;
    }
    if (hdf4_info.sds_id == -1)
    {
        handle_hdf4_error();
    }

    /* Set dimension names */
    set_dim_names(dim_info.num_dims);

    if (!dim_info.is_var_size && !has_dyn_available_fields)
    {
        SDsetfillmode(hdf4_info.sds_id, SD_NOFILL);
    }
    if (SDsetfillvalue(hdf4_info.sds_id, hdf_fill_value(hdf4_info.hdf_type)) != 0)
    {
        handle_hdf4_error();
    }

    if (hdf4_info.vgroup_depth > 0)
    {
        if (Vaddtagref(hdf4_info.vgroup_id[hdf4_info.vgroup_depth - 1], DFTAG_NDG, SDidtoref(hdf4_info.sds_id)) == -1)
        {
            handle_hdf4_error();
        }
    }

    /* write data to HDF file */
    hdf4_info.offset = 0;
    write_data(0, 0, 0);

    /* set description and unit attributes */
    {
        const char *description;
        const char *unit;

        if (coda_type_get_description(traverse_info.type[traverse_info.current_depth], &description) != 0)
        {
            handle_coda_error();
        }
        if ((description != NULL) && (description[0] != '\0'))
        {
            if (SDsetattr(hdf4_info.sds_id, "description", DFNT_CHAR, strlen(description), description) != 0)
            {
                handle_hdf4_error();
            }
        }

        if (coda_type_get_unit(traverse_info.type[traverse_info.current_depth], &unit) != 0)
        {
            handle_coda_error();
        }
        if ((unit != NULL) && (unit[0] != '\0'))
        {
            if (SDsetattr(hdf4_info.sds_id, "unit", DFNT_CHAR, strlen(unit), unit) != 0)
            {
                handle_hdf4_error();
            }
        }
    }

    if (SDendaccess(hdf4_info.sds_id) != 0)
    {
        handle_hdf4_error();
    }

    /* write dimension data set if the data set has variable sized dimensions */
    if (dim_info.is_var_size)
    {
        write_dims();
    }
}

#endif
