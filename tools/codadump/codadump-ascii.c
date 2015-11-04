/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

static int first_write_of_data = 1;

static void write_data(FILE *f, int depth, int array_depth, int record_depth);

static void write_index(FILE *f)
{
    int array_id;
    int i;

    array_id = 0;
    for (i = 0; i < traverse_info.current_depth; i++)
    {
        coda_type_class type_class;

        if (coda_type_get_class(traverse_info.type[i], &type_class) != 0)
        {
            handle_coda_error();
        }
        if (type_class == coda_array_class)
        {
            int j;

            for (j = 0; j < traverse_info.array_info[array_id].num_dims; j++)
            {
                fprintf(f, "%ld%s", (long)traverse_info.array_info[array_id].index[j], ascii_col_sep);
            }
            array_id++;
        }
    }
}

static void write_basic_data(FILE *f, int depth)
{
    coda_type_class type_class;

    if (show_index)
    {
        write_index(f);
    }

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
                    case coda_native_type_char:
                        {
                            char data;

                            if (coda_cursor_read_char(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            if (show_quotes)
                            {
                                fprintf(f, "'%c'", data);
                            }
                            else
                            {
                                fprintf(f, "%c", data);
                            }
                        }
                        break;
                    case coda_native_type_string:
                        {
                            long length;
                            char *data;

                            if (coda_cursor_get_string_length(&traverse_info.cursor, &length) != 0)
                            {
                                handle_coda_error();
                            }
                            data = (char *)malloc(length + 1);
                            if (data == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)length + 1, __FILE__, __LINE__);
                                handle_coda_error();
                            }
                            if (coda_cursor_read_string(&traverse_info.cursor, data, length + 1) != 0)
                            {
                                handle_coda_error();
                            }

                            if (show_quotes)
                            {
                                fprintf(f, "\"%s\"", data);
                            }
                            else
                            {
                                fprintf(f, "%s", data);
                            }

                            free(data);
                        }
                        break;
                    case coda_native_type_bytes:
                        {
                            int64_t bit_size;
                            int64_t byte_size;
                            uint8_t *data;
                            int i;

                            if (coda_cursor_get_bit_size(&traverse_info.cursor, &bit_size) != 0)
                            {
                                handle_coda_error();
                            }
                            byte_size = (bit_size >> 3) + (bit_size & 0x7 ? 1 : 0);
                            data = (uint8_t *)malloc((size_t)byte_size);
                            if (data == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)byte_size, __FILE__, __LINE__);
                                handle_coda_error();
                            }
                            if (coda_cursor_read_bits(&traverse_info.cursor, data, 0, bit_size) != 0)
                            {
                                handle_coda_error();
                            }

                            for (i = 0; i < byte_size; i++)
                            {
                                char c;

                                c = data[i];
                                switch (c)
                                {
                                    case '\a':
                                        fprintf(f, "\\a");
                                        break;
                                    case '\b':
                                        fprintf(f, "\\b");
                                        break;
                                    case '\t':
                                        fprintf(f, "\\t");
                                        break;
                                    case '\n':
                                        fprintf(f, "\\n");
                                        break;
                                    case '\v':
                                        fprintf(f, "\\v");
                                        break;
                                    case '\f':
                                        fprintf(f, "\\f");
                                        break;
                                    case '\r':
                                        fprintf(f, "\\r");
                                        break;
                                    case '\\':
                                        fprintf(f, "\\\\");
                                        break;
                                    default:
                                        if (c >= 32 && c <= 126)
                                        {
                                            fprintf(f, "%c", c);
                                        }
                                        else
                                        {
                                            fprintf(f, "\\%03o", (int)(unsigned char)c);
                                        }
                                }
                            }

                            free(data);
                        }
                        break;
                    case coda_native_type_int8:
                    case coda_native_type_int16:
                    case coda_native_type_int32:
                        {
                            int32_t data;

                            if (coda_cursor_read_int32(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            fprintf(f, "%ld", (long)data);
                        }
                        break;
                    case coda_native_type_uint8:
                    case coda_native_type_uint16:
                    case coda_native_type_uint32:
                        {
                            uint32_t data;

                            if (coda_cursor_read_uint32(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            fprintf(f, "%lu", (unsigned long)data);
                        }
                        break;
                    case coda_native_type_int64:
                        {
                            int64_t data;
                            char s[21];

                            if (coda_cursor_read_int64(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            coda_str64(data, s);
                            fprintf(f, "%s", s);
                        }
                        break;
                    case coda_native_type_uint64:
                        {
                            uint64_t data;
                            char s[21];

                            if (coda_cursor_read_uint64(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            coda_str64u(data, s);
                            fprintf(f, "%s", s);
                        }
                        break;
                    case coda_native_type_float:
                    case coda_native_type_double:
                        {
                            double data;

                            if (coda_cursor_read_double(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            if (read_type == coda_native_type_float)
                            {
                                fprintf(f, "%.7g", data);
                            }
                            else
                            {
                                fprintf(f, "%.16g", data);
                            }
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
                    case coda_special_no_data:
                        /* write nothing */
                        break;
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        {
                            double data;

                            if (coda_cursor_read_double(&traverse_info.cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }
                            if ((special_type == coda_special_time) && show_time_as_string)
                            {
                                char str[27];

                                if (coda_isNaN(data) || coda_isInf(data))
                                {
                                    strcpy(str, "                          ");
                                }
                                else
                                {
                                    if (coda_time_double_to_string(data, "yyyy-MM-dd HH:mm:ss.SSSSSS", str) != 0)
                                    {
                                        handle_coda_error();
                                    }
                                }
                                if (show_quotes)
                                {
                                    fprintf(f, "\"%s\"", str);
                                }
                                else
                                {
                                    fprintf(f, "%s", str);
                                }
                            }
                            else
                            {
                                fprintf(f, "%.16g", data);
                            }
                        }
                        break;
                    case coda_special_complex:
                        {
                            double data[2];

                            if (coda_cursor_read_complex_double_pair(&traverse_info.cursor, data) != 0)
                            {
                                handle_coda_error();
                            }

                            fprintf(f, "%g%s%g", data[0], ascii_col_sep, data[1]);
                        }
                        break;
                }
            }
            break;
        case coda_record_class:
        case coda_array_class:
            assert(0);
            exit(1);
    }
    fprintf(f, "\n");
}

static void write_data(FILE *f, int depth, int array_depth, int record_depth)
{
    coda_type_class type_class;

    if (coda_type_get_class(traverse_info.type[depth], &type_class) != 0)
    {
        handle_coda_error();
    }
    switch (type_class)
    {
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
                    array_info->index[i] = 0;
                }
                if (number_of_elements == 0)
                {
                    /* array is empty */
                    return;
                }

                /* traverse array */
                if (coda_cursor_goto_first_array_element(&traverse_info.cursor) != 0)
                {
                    handle_coda_error();
                }
                for (i = 0; i < number_of_elements; i++)
                {
                    /* write data for current array element */
                    write_data(f, depth + 1, array_depth + 1, record_depth);

                    if (i < number_of_elements - 1)
                    {
                        /* jump to next array element */
                        if (coda_cursor_goto_next_array_element(&traverse_info.cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        if (has_var_dim_sub_array && array_depth < traverse_info.num_arrays - 1)
                        {
                            traverse_info.array_info[array_depth + 1].global_index++;
                        }
                        if (show_index)
                        {
                            int k = array_info->num_dims - 1;

                            while (k >= 0)
                            {
                                array_info->index[k]++;
                                if (array_info->index[k] == local_dim[k])
                                {
                                    array_info->index[k--] = 0;
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                coda_cursor_goto_parent(&traverse_info.cursor);
            }
            break;
        case coda_record_class:
            {
                int available;

                if (coda_cursor_get_record_field_available_status(&traverse_info.cursor,
                                                                  traverse_info.parent_index[record_depth],
                                                                  &available) != 0)
                {
                    handle_coda_error();
                }
                /* if the field is not available just don't print it */
                if (available)
                {
                    if (coda_cursor_goto_record_field_by_index(&traverse_info.cursor,
                                                               traverse_info.parent_index[record_depth]) != 0)
                    {
                        handle_coda_error();
                    }
                    write_data(f, depth + 1, array_depth, record_depth + 1);
                    coda_cursor_goto_parent(&traverse_info.cursor);
                }
            }
            break;
        default:
            write_basic_data(f, depth);
            break;
    }
}

void export_data_element_to_ascii()
{
    if (first_write_of_data)
    {
        first_write_of_data = 0;
    }
    else
    {
        /* print data separator */
        fprintf(ascii_output, "\n");
    }

    if (show_label)
    {
        print_full_field_name(ascii_output, 2, 0);
        fprintf(ascii_output, "\n");
    }

    if (dim_info.num_dims > 0 && dim_info.filled_num_elements[dim_info.num_dims - 1] == 0)
    {
        /* no data */
        return;
    }

    write_data(ascii_output, 0, 0, 0);
}
