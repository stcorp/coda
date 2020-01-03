/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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

#include "codadump.h"

static void traverse_data();

static void print_array_dim(FILE *f, int array_id)
{
    array_info_t *array_info;
    int i;

    array_info = &traverse_info.array_info[array_id];

    for (i = 0; i < array_info->num_dims; i++)
    {
        if (i > 0)
        {
            fprintf(f, ",");
        }
        if (calc_dim)
        {
            if (dim_info.is_var_size_dim[array_info->dim_id + i])
            {
                fprintf(f, "%ld-%ld", (long)dim_info.min_dim[array_info->dim_id + i],
                        (long)dim_info.dim[array_info->dim_id + i]);
            }
            else
            {
                fprintf(f, "%ld", (long)dim_info.dim[array_info->dim_id + i]);
            }
        }
        else
        {
            if (array_info->dim[i] == -1)
            {
                fprintf(f, "?");
            }
            else
            {
                fprintf(f, "%ld", (long)array_info->dim[i]);
            }
        }
    }
}

void print_full_field_name(FILE *f, int print_dims, int compound_as_array)
{
    int i;

    if (print_dims == 1)
    {
        int record_id = 0;
        int array_id = 0;

        for (i = 0; i < traverse_info.current_depth; i++)
        {
            coda_type_class type_class;

            if (coda_type_get_class(traverse_info.type[i], &type_class) != 0)
            {
                handle_coda_error();
            }
            switch (type_class)
            {
                case coda_record_class:
                    fprintf(f, "/%s", traverse_info.field_name[record_id]);
                    record_id++;
                    break;
                case coda_array_class:
                    if (i == 0)
                    {
                        fprintf(f, "/");
                    }
                    if (traverse_info.array_info[array_id].num_dims > 0)
                    {
                        fprintf(f, "[");
                        print_array_dim(f, array_id);
                        fprintf(f, "]");
                    }
                    array_id++;
                    break;
                default:
                    break;
            }
        }
        if (compound_as_array && array_id < traverse_info.num_arrays)
        {
            fprintf(f, "[");
            print_array_dim(f, array_id);
            fprintf(f, "]");
        }
    }
    else
    {
        for (i = 0; i < traverse_info.num_records; i++)
        {
            if (i > 0)
            {
                fprintf(f, ".");
            }
            fprintf(f, "%s", traverse_info.field_name[i]);
        }

        if (print_dims == 2)
        {
            int array_id = 0;

            for (i = 0; i < traverse_info.current_depth; i++)
            {
                coda_type_class type_class;

                if (coda_type_get_class(traverse_info.type[i], &type_class) != 0)
                {
                    handle_coda_error();
                }
                if (type_class == coda_array_class)
                {
                    if (traverse_info.array_info[array_id].num_dims > 0)
                    {
                        if (traverse_info.array_info[array_id].dim_id == 0)
                        {
                            fprintf(f, " [");
                        }
                        else
                        {
                            fprintf(f, ",");
                        }
                        print_array_dim(f, array_id);
                        array_id++;
                    }
                }
            }
            if (compound_as_array && array_id < traverse_info.num_arrays)
            {
                if (traverse_info.array_info[array_id].dim_id == 0)
                {
                    fprintf(f, " [");
                }
                else
                {
                    fprintf(f, ",");
                }
                print_array_dim(f, array_id);
                array_id++;
            }
            if (array_id > 0)
            {
                fprintf(f, "]");
            }
        }
    }
}

void traverse_info_init()
{
    traverse_info.pf = NULL;
    traverse_info.current_depth = 0;
    traverse_info.num_arrays = 0;
    traverse_info.num_records = 0;
}

void traverse_info_done()
{
    if (traverse_info.pf != NULL)
    {
        coda_close(traverse_info.pf);
    }
    if (traverse_info.filter[0] != NULL)
    {
        codadump_filter_remove(&traverse_info.filter[0]);
    }
}

static void handle_data_element()
{
    if (run_mode == RUN_MODE_LIST)
    {
        print_full_field_name(stdout, 1, 0);
        if (show_type)
        {
            coda_type_class type_class;

            if (coda_type_get_class(traverse_info.type[traverse_info.current_depth], &type_class) != 0)
            {
                handle_coda_error();
            }
            if (type_class == coda_special_class)
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(traverse_info.type[traverse_info.current_depth], &special_type) != 0)
                {
                    handle_coda_error();
                }
                printf(" %s", coda_type_get_special_type_name(special_type));
            }
            else
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(traverse_info.type[traverse_info.current_depth], &read_type) != 0)
                {
                    handle_coda_error();
                }
                printf(" %s", coda_type_get_native_type_name(read_type));
                if (read_type == coda_native_type_string || read_type == coda_native_type_bytes)
                {
                    printf("(");
                    assert(traverse_info.num_arrays > 0);
                    print_array_dim(stdout, traverse_info.num_arrays - 1);
                    printf(")");
                }
            }
        }
        if (show_unit)
        {
            const char *unit;

            if (coda_type_get_unit(traverse_info.type[traverse_info.current_depth], &unit) != 0)
            {
                handle_coda_error();
            }
            if (unit != NULL && unit[0] != '\0')
            {
                printf(" [%s]", unit);
            }
        }
        if (show_description)
        {
            const char *description;

            if (coda_type_get_description(traverse_info.type[traverse_info.current_depth], &description) != 0)
            {
                handle_coda_error();
            }
            if (description != NULL && description[0] != '\0')
            {
                printf(" \"%s\"", description);
            }
        }
        printf("\n");
        if (show_dim_vals)
        {
            int i;

            for (i = 0; i < dim_info.num_dims; i++)
            {
                print_all_distinct_dims(i);
            }
        }
    }
    else if (run_mode == RUN_MODE_ASCII)
    {
        export_data_element_to_ascii();
    }
#ifdef HAVE_HDF4
    else if (run_mode == RUN_MODE_HDF4)
    {
        export_data_element_to_hdf4();
    }
#endif
}

/* If the user explicitly asks for traversal of a hidden record field,
 * this function will be called with traverse_hidden set to 1.
 */
static void traverse_record(int index, int traverse_hidden)
{
    int hidden;

    traverse_info.parent_index[traverse_info.num_records - 1] = index;

    if (coda_type_get_record_field_hidden_status(traverse_info.type[traverse_info.current_depth - 1], index, &hidden)
        != 0)
    {
        handle_coda_error();
    }
    if (hidden && !traverse_hidden)
    {
        /* skip this field */
        return;
    }

    if (calc_dim)
    {
        int available;

        /* we do not traverse records that are globally not available
         * (i.e. not available for every element of our parent array(s))
         */
        if (coda_type_get_record_field_available_status(traverse_info.type[traverse_info.current_depth - 1], index,
                                                        &available) != 0)
        {
            handle_coda_error();
        }
        if (available == -1)
        {
            /* traverse all occurrences of this field to check whether at least one is available */
            if (!dim_record_field_available())
            {
                return;
            }
        }
        traverse_info.field_available_status[traverse_info.current_depth - 1] = available;
    }

    if (coda_type_get_record_field_name(traverse_info.type[traverse_info.current_depth - 1], index,
                                        &traverse_info.field_name[traverse_info.num_records - 1]) != 0)
    {
        handle_coda_error();
    }
    if (coda_type_get_record_field_type(traverse_info.type[traverse_info.current_depth - 1], index,
                                        &traverse_info.type[traverse_info.current_depth]) != 0)
    {
        handle_coda_error();
    }

    traverse_data();
}

static void traverse_data()
{
    coda_type_class type_class;

    if (coda_type_get_class(traverse_info.type[traverse_info.current_depth], &type_class) != 0)
    {
        handle_coda_error();
    }
    switch (type_class)
    {
        case coda_record_class:
            {
                long num_fields;
                long index;

                if (traverse_info.current_depth >= CODA_CURSOR_MAXDEPTH - 1)
                {
                    /* don't traverse further, since we can't navigate to it with a CODA cursor */
                    break;
                }

#ifdef HAVE_HDF4
                if (run_mode == RUN_MODE_HDF4)
                {
                    hdf4_enter_record();
                }
#endif
                traverse_info.num_records++;
                if (coda_type_get_num_record_fields(traverse_info.type[traverse_info.current_depth], &num_fields) != 0)
                {
                    handle_coda_error();
                }
                traverse_info.current_depth++;
                if (traverse_info.filter[traverse_info.filter_depth] != NULL)
                {
                    codadump_filter *filter;

                    filter = traverse_info.filter[traverse_info.filter_depth];
                    while (traverse_info.filter[traverse_info.filter_depth] != NULL)
                    {
                        const char *name;

                        name = codadump_filter_get_fieldname(traverse_info.filter[traverse_info.filter_depth]);
                        assert(name != NULL);
                        if (coda_type_get_record_field_index_from_name
                            (traverse_info.type[traverse_info.current_depth - 1], name, &index) != 0)
                        {
                            if (coda_errno == CODA_ERROR_INVALID_NAME)
                            {
                                fprintf(stderr, "ERROR: incorrect filter - incorrect fieldname (%s)\n", name);
                                exit(1);
                            }
                            handle_coda_error();
                        }
                        traverse_info.filter[traverse_info.filter_depth + 1] =
                            codadump_filter_get_subfilter(traverse_info.filter[traverse_info.filter_depth]);
                        traverse_info.filter_depth++;
                        traverse_record(index, 1);
                        traverse_info.filter_depth--;
                        traverse_info.filter[traverse_info.filter_depth] =
                            codadump_filter_get_next_filter(traverse_info.filter[traverse_info.filter_depth]);
                    }
                    traverse_info.filter[traverse_info.filter_depth] = filter;
                }
                else
                {
                    for (index = 0; index < num_fields; index++)
                    {
                        traverse_record(index, 0);
                    }
                }
                traverse_info.current_depth--;
                traverse_info.num_records--;
#ifdef HAVE_HDF4
                if (run_mode == RUN_MODE_HDF4)
                {
                    hdf4_leave_record();
                }
#endif
            }
            break;
        case coda_array_class:
            {
                if (traverse_info.current_depth >= CODA_CURSOR_MAXDEPTH - 1)
                {
                    /* don't traverse further, since we can't navigate to it with a CODA cursor */
                    break;
                }

                dim_enter_array();
#ifdef HAVE_HDF4
                if (run_mode == RUN_MODE_HDF4)
                {
                    hdf4_enter_array();
                }
#endif
                traverse_info.num_arrays++;
                traverse_info.current_depth++;
                if (coda_type_get_array_base_type(traverse_info.type[traverse_info.current_depth - 1],
                                                  &traverse_info.type[traverse_info.current_depth]) != 0)
                {
                    handle_coda_error();
                }
                traverse_data();
                traverse_info.current_depth--;
                traverse_info.num_arrays--;
#ifdef HAVE_HDF4
                if (run_mode == RUN_MODE_HDF4)
                {
                    hdf4_leave_array();
                }
#endif
                dim_leave_array();
            }
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
                    case coda_native_type_string:
                    case coda_native_type_bytes:
                        dim_enter_array();
#ifdef HAVE_HDF4
                        if (run_mode == RUN_MODE_HDF4)
                        {
                            hdf4_enter_array();
                        }
#endif
                        traverse_info.num_arrays++;
                        handle_data_element();
                        traverse_info.num_arrays--;
#ifdef HAVE_HDF4
                        if (run_mode == RUN_MODE_HDF4)
                        {
                            hdf4_leave_array();
                        }
#endif
                        dim_leave_array();
                        break;
                    default:
                        handle_data_element();
                        break;
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_get_option_bypass_special_types())
                {
                    /* use the base type for all special types */
                    if (coda_type_get_special_base_type(traverse_info.type[traverse_info.current_depth],
                                                        &traverse_info.type[traverse_info.current_depth]) != 0)
                    {
                        handle_coda_error();
                    }
                    traverse_data();
                    return;
                }
                if (coda_type_get_special_type(traverse_info.type[traverse_info.current_depth], &special_type) != 0)
                {
                    handle_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_no_data:
                        /* ignore */
                        return;
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        handle_data_element();
                        break;
                    case coda_special_complex:
                        dim_enter_array();
#ifdef HAVE_HDF4
                        if (run_mode == RUN_MODE_HDF4)
                        {
                            hdf4_enter_array();
                        }
#endif
                        traverse_info.num_arrays++;
                        handle_data_element();
                        traverse_info.num_arrays--;
#ifdef HAVE_HDF4
                        if (run_mode == RUN_MODE_HDF4)
                        {
                            hdf4_leave_array();
                        }
#endif
                        dim_leave_array();
                        break;
                }
            }
            break;
    }
}

void traverse_product()
{
    int result;

    result = coda_open(traverse_info.file_name, &traverse_info.pf);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(traverse_info.file_name, &traverse_info.pf);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        handle_coda_error();
    }
    if (coda_cursor_set_product(&traverse_info.cursor, traverse_info.pf) != 0)
    {
        handle_coda_error();
    }
    if (coda_cursor_get_type(&traverse_info.cursor, &traverse_info.type[traverse_info.current_depth]) != 0)
    {
        handle_coda_error();
    }
    traverse_data();

    coda_close(traverse_info.pf);
    traverse_info.pf = NULL;
}
