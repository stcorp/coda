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

#include "codadump.h"

void dim_info_init()
{
    dim_info.num_dims = 0;
    dim_info.is_var_size = 0;
    dim_info.last_var_size_dim = -1;
}

void dim_info_done()
{
}

void print_all_distinct_dims(int dim_id)
{
    int *dims;
    int i;

    assert(dim_id < dim_info.num_dims);

    if (!dim_info.is_var_size_dim[dim_id])
    {
        return;
    }

    dims = (int *)malloc((dim_info.dim[dim_id] + 1) * sizeof(int));
    if (dims == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (dim_info.dim[dim_id] + 1) * sizeof(int), __FILE__, __LINE__);
        handle_coda_error();
    }
    memset(dims, 0, (dim_info.dim[dim_id] + 1) * sizeof(int));

    for (i = 0; i < dim_info.num_elements[dim_info.var_dim_num_dims[dim_id] - 1]; i++)
    {
        if (dim_info.var_dim[dim_id][i] >= 0)
        {
            dims[dim_info.var_dim[dim_id][i]]++;
        }
    }
    printf("  dim{%d}=(", dim_id + 1);
    for (i = dim_info.min_dim[dim_id]; i < dim_info.dim[dim_id]; i++)
    {
        if (dims[i] > 0)
        {
            printf("%d,", i);
        }
    }
    printf("%d)", dim_info.dim[dim_id]);
    printf(", num=(");
    for (i = dim_info.min_dim[dim_id]; i < dim_info.dim[dim_id]; i++)
    {
        if (dims[i] > 0)
        {
            printf("%d,", dims[i]);
        }
    }
    printf("%d)", dims[dim_info.dim[dim_id]]);
    printf("\n");

    free(dims);
}

static void get_all_dims_for_array(int depth, int array_depth, int record_depth)
{
    coda_type_class type_class;

    if (coda_cursor_get_type_class(&traverse_info.cursor, &type_class) != 0)
    {
        handle_coda_error();
    }

    switch (type_class)
    {
        case coda_array_class:
            {
                array_info_t *array_info;
                int dim_id;

                array_info = &traverse_info.array_info[array_depth];
                dim_id = array_info->dim_id;

                if (array_depth == traverse_info.num_arrays)
                {
                    long var_dim[MAX_NUM_DIMS];
                    int num_dims;
                    int i;

                    if (coda_cursor_get_array_dim(&traverse_info.cursor, &num_dims, var_dim) != 0)
                    {
                        handle_coda_error();
                    }
                    assert(num_dims == array_info->num_dims);
                    for (i = 0; i < array_info->num_dims; i++)
                    {
                        if (array_info->dim[i] == -1)   /* variable sized dimension? */
                        {
                            dim_info.var_dim[dim_id + i][array_info->global_index] = var_dim[i];
                            if (dim_info.dim[dim_id + i] == -1)
                            {
                                dim_info.dim[dim_id + i] = var_dim[i];
                                dim_info.min_dim[dim_id + i] = var_dim[i];
                            }
                            else
                            {
                                if (dim_info.dim[dim_id + i] < var_dim[i])
                                {
                                    dim_info.dim[dim_id + i] = var_dim[i];
                                }
                                if (dim_info.min_dim[dim_id + i] > var_dim[i])
                                {
                                    dim_info.min_dim[dim_id + i] = var_dim[i];
                                }
                            }
                        }
                    }
                }
                else
                {
                    /* traverse array */
                    int number_of_elements;
                    int i;

                    if (array_depth == 0)
                    {
                        array_info->global_index = 0;
                    }
                    traverse_info.array_info[array_depth + 1].global_index =
                        array_info->global_index * array_info->num_elements;

                    number_of_elements = 1;
                    for (i = dim_id; i < dim_id + array_info->num_dims; i++)
                    {
                        if (dim_info.is_var_size_dim[i])
                        {
                            number_of_elements *= dim_info.var_dim[i][array_info->global_index];
                        }
                        else
                        {
                            number_of_elements *= dim_info.dim[i];
                        }
                    }
                    if (number_of_elements > 0)
                    {
                        int i;

                        if (coda_cursor_goto_first_array_element(&traverse_info.cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        for (i = 0; i < number_of_elements; i++)
                        {
                            get_all_dims_for_array(depth + 1, array_depth + 1, record_depth);
                            if (i < number_of_elements - 1)
                            {
                                if (coda_cursor_goto_next_array_element(&traverse_info.cursor) != 0)
                                {
                                    handle_coda_error();
                                }
                                traverse_info.array_info[array_depth + 1].global_index++;
                            }
                        }
                        coda_cursor_goto_parent(&traverse_info.cursor);
                    }
                }
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
                if (available)
                {
                    if (coda_cursor_goto_record_field_by_index(&traverse_info.cursor,
                                                               traverse_info.parent_index[record_depth]) != 0)
                    {
                        handle_coda_error();
                    }
                    get_all_dims_for_array(depth + 1, array_depth, record_depth + 1);
                    coda_cursor_goto_parent(&traverse_info.cursor);
                }
                else
                {
                    array_info_t *array_info;
                    int dim_id;
                    int i;

                    /* field is not available -> set array dimensions to 0 */

                    array_info = &traverse_info.array_info[traverse_info.num_arrays];
                    dim_id = array_info->dim_id;
                    for (i = 0; i < array_info->num_dims; i++)
                    {
                        if (array_info->dim[i] == -1)   /* variable sized dimension? */
                        {
                            dim_info.var_dim[dim_id + i][array_info->global_index] = 0;
                            if (dim_info.dim[dim_id + i] == -1)
                            {
                                dim_info.dim[dim_id + i] = 0;
                            }
                            dim_info.min_dim[dim_id + i] = 0;
                        }
                    }
                }
            }
            break;
        case coda_text_class:
        case coda_raw_class:
            {
                array_info_t *array_info;
                int dim_id;
                int64_t size;

                assert(array_depth == traverse_info.num_arrays);

                if (type_class == coda_text_class)
                {
                    long length;

                    if (coda_cursor_get_string_length(&traverse_info.cursor, &length) != 0)
                    {
                        handle_coda_error();
                    }
                    size = length;
                }
                else
                {
                    if (coda_cursor_get_byte_size(&traverse_info.cursor, &size) != 0)
                    {
                        handle_coda_error();
                    }
                }

                array_info = &traverse_info.array_info[array_depth];
                dim_id = array_info->dim_id;
                dim_info.var_dim[dim_id][array_info->global_index] = (int)size;
                if (dim_info.dim[dim_id] == -1)
                {
                    dim_info.dim[dim_id] = (int)size;
                    dim_info.min_dim[dim_id] = (int)size;
                }
                else
                {
                    if (dim_info.dim[dim_id] < size)
                    {
                        dim_info.dim[dim_id] = (int)size;
                    }
                    if (dim_info.min_dim[dim_id] > size)
                    {
                        dim_info.min_dim[dim_id] = (int)size;
                    }
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }
}

void dim_enter_array()
{
    coda_type_class type_class;
    array_info_t *array_info;
    int64_t array_count;        /* maximum possible number of these arrays in the product */
    int64_t filled_array_count; /* real count of number of these arrays in the product */
    int dd_var_size;    /* is the array variable sized according to the data dictionary */
    int is_var_size;    /* is the array really variable sized */
    int dim_id;
    int i;

    array_info = &traverse_info.array_info[traverse_info.num_arrays];
    if (coda_type_get_class(traverse_info.type[traverse_info.current_depth], &type_class) != 0)
    {
        handle_coda_error();
    }

    switch (type_class)
    {
        case coda_array_class:
            {
                long dim[MAX_NUM_DIMS];
                int num_dims;

                if (coda_type_get_array_dim(traverse_info.type[traverse_info.current_depth], &num_dims, dim) != 0)
                {
                    handle_coda_error();
                }
                array_info->num_dims = (int32_t)num_dims;
                for (i = 0; i < num_dims; i++)
                {
                    array_info->dim[i] = (int32_t)dim[i];
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
                    case coda_special_complex:
                        array_info->num_dims = 1;
                        array_info->dim[0] = 2;
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_text_class:
            {
                long length;

                if (coda_type_get_string_length(traverse_info.type[traverse_info.current_depth], &length) != 0)
                {
                    handle_coda_error();
                }
                array_info->num_dims = 1;
                array_info->dim[0] = length;
            }
            break;
        case coda_raw_class:
            {
                int64_t size;

                if (coda_type_get_bit_size(traverse_info.type[traverse_info.current_depth], &size) != 0)
                {
                    handle_coda_error();
                }
                array_info->num_dims = 1;
                if (size >= 0)
                {
                    array_info->dim[0] = (int)((size >> 3) + ((size & 0x7) != 0 ? 1 : 0));
                }
                else
                {
                    array_info->dim[0] = -1;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    if (!calc_dim)
    {
        /* we only update the array_info struct and further ignore the dim_info struct */
        array_info->dim_id = -1;
        array_info->num_elements = 0;
        array_info->global_index = 0;
        return;
    }

    dim_id = dim_info.num_dims;
    array_info->dim_id = dim_id;

    assert(dim_info.num_dims + array_info->num_dims <= MAX_NUM_DIMS);
    dim_info.num_dims += array_info->num_dims;

    if (dim_id > 0)
    {
        array_count = dim_info.num_elements[dim_id - 1];
        filled_array_count = dim_info.filled_num_elements[dim_id - 1];
    }
    else
    {
        array_count = 1;
        filled_array_count = 1;
    }

    dd_var_size = 0;
    is_var_size = 0;
    if (filled_array_count > 0)
    {
        for (i = 0; i < array_info->num_dims; i++)
        {
            dim_info.dim[dim_id + i] = array_info->dim[i];
            dim_info.is_var_size_dim[dim_id + i] = 0;
            if (array_info->dim[i] == -1)
            {
                dd_var_size = 1;
            }
        }
    }
    else
    {
        /* There are no arrays of this kind in the product so set all dimensions to 0 */
        for (i = 0; i < array_info->num_dims; i++)
        {
            dim_info.dim[dim_id + i] = 0;
            dim_info.is_var_size_dim[dim_id + i] = 0;
        }
    }

    /* find out whether the dimensions of this array are really variable sized */
    if (dd_var_size)
    {
        /* retrieve all dimensions for this kind of array */
        for (i = 0; i < array_info->num_dims; i++)
        {
            if (array_info->dim[i] == -1)       /* variable sized dimension? */
            {
                int j;

                /* allocate array to store all dimensions */
                dim_info.var_dim_num_dims[dim_id + i] = dim_id;
                dim_info.var_dim[dim_id + i] = (int32_t *)malloc((size_t)array_count * sizeof(int32_t));
                if (dim_info.var_dim[dim_id + i] == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)array_count * sizeof(int32_t), __FILE__, __LINE__);
                    handle_coda_error();
                }
                for (j = 0; j < array_count; j++)
                {
                    dim_info.var_dim[dim_id + i][j] = -1;
                }
            }
            else
            {
                dim_info.var_dim_num_dims[dim_id + i] = 0;
                dim_info.var_dim[dim_id + i] = NULL;
            }
        }
        array_info->global_index = 0;
        get_all_dims_for_array(0, 0, 0);

        /* check whether array is really variable sized (and clear var_dim if not) */
        for (i = 0; i < array_info->num_dims; i++)
        {
            if (array_info->dim[i] == -1)
            {
                if (dim_info.dim[dim_id + i] != dim_info.min_dim[dim_id + i])
                {
                    dim_info.is_var_size_dim[dim_id + i] = 1;
                    is_var_size = 1;
                }
                else
                {
                    dim_info.is_var_size_dim[dim_id + i] = 0;
                    free(dim_info.var_dim[dim_id + i]);
                    dim_info.var_dim[dim_id + i] = NULL;
                    dim_info.var_dim_num_dims[dim_id + i] = 0;
                }
            }
        }
    }

    /* update is_var_size and last_var_size_dim */
    dim_info.is_var_size = 0;
    dim_info.last_var_size_dim = -1;
    for (i = 0; i < dim_info.num_dims; i++)
    {
        if (dim_info.is_var_size_dim[i])
        {
            dim_info.is_var_size = 1;
            dim_info.last_var_size_dim = i;
        }
    }

    /* determine amount of elements */
    array_info->num_elements = 1;
    for (i = 0; i < array_info->num_dims; i++)
    {
        array_info->num_elements *= dim_info.dim[dim_id + i];
        if (i == 0)
        {
            dim_info.num_elements[dim_id] = array_count * dim_info.dim[dim_id];
        }
        else
        {
            dim_info.num_elements[dim_id + i] = dim_info.num_elements[dim_id + i - 1] * dim_info.dim[dim_id + i];
        }
    }

    /* determine filled amount of elements */
    if (is_var_size)
    {
        int j;

        for (i = 0; i < array_info->num_dims; i++)
        {
            dim_info.filled_num_elements[dim_id + i] = 0;
        }
        for (j = 0; j < array_count; j++)
        {
            int num_elements = 1;

            for (i = 0; i < array_info->num_dims; i++)
            {
                if (dim_info.is_var_size_dim[dim_id + i])
                {
                    num_elements *= dim_info.var_dim[dim_id + i][j];
                }
                else
                {
                    num_elements *= dim_info.dim[dim_id + i];
                }
                dim_info.filled_num_elements[dim_id + i] += num_elements;
            }
        }
    }
    else
    {
        dim_info.filled_num_elements[dim_id] = filled_array_count * dim_info.dim[dim_id];
        for (i = 1; i < array_info->num_dims; i++)
        {
            dim_info.filled_num_elements[dim_id + i] =
                dim_info.filled_num_elements[dim_id + i - 1] * dim_info.dim[dim_id + i];
        }
    }

    /* determine (sub)array size */
    if (dim_info.filled_num_elements[dim_id + array_info->num_dims - 1] > 0)
    {
        dim_info.array_size[dim_info.num_dims - 1] = dim_info.dim[dim_info.num_dims - 1];
        for (i = array_info->num_dims - 2; i >= 0; i--)
        {
            dim_info.array_size[dim_id + i] = dim_info.dim[dim_id + i] * dim_info.array_size[dim_id + i + 1];
        }
        for (i = dim_id - 1; i >= 0; i--)
        {
            dim_info.array_size[i] *= dim_info.array_size[dim_id];
        }
    }
}

void dim_leave_array()
{
    array_info_t *array_info;
    int dim_id;
    int i;

    assert(traverse_info.num_arrays >= 0);

    array_info = &traverse_info.array_info[traverse_info.num_arrays];
    dim_id = array_info->dim_id;

    dim_info.num_dims -= array_info->num_dims;

    /* clear var_dim arrays */
    for (i = 0; i < array_info->num_dims; i++)
    {
        if (dim_info.is_var_size_dim[dim_id + i])
        {
            free(dim_info.var_dim[dim_id + i]);
        }
    }

    /* update is_var_size and last_var_size_dim */
    dim_info.is_var_size = 0;
    dim_info.last_var_size_dim = -1;
    for (i = 0; i < dim_info.num_dims; i++)
    {
        if (dim_info.is_var_size_dim[i])
        {
            dim_info.is_var_size = 1;
            dim_info.last_var_size_dim = i;
        }
    }

    /* update (sub)array size */
    if (dim_info.filled_num_elements[dim_id + array_info->num_dims - 1] > 0)
    {
        for (i = dim_id - 1; i >= 0; i--)
        {
            dim_info.array_size[i] /= dim_info.array_size[dim_id];
        }
    }
}

static int get_record_field_available_status(int depth, int array_depth, int record_depth)
{
    coda_type_class type_class;

    if (coda_cursor_get_type_class(&traverse_info.cursor, &type_class) != 0)
    {
        handle_coda_error();
    }

    switch (type_class)
    {
        case coda_array_class:
            {
                array_info_t *array_info;
                int number_of_elements;
                int dim_id;
                int i;

                array_info = &traverse_info.array_info[array_depth];
                dim_id = array_info->dim_id;

                assert(array_depth < traverse_info.num_arrays);

                /* traverse array */

                if (array_depth == 0)
                {
                    array_info->global_index = 0;
                }
                if (array_depth < traverse_info.num_arrays)
                {
                    traverse_info.array_info[array_depth + 1].global_index =
                        array_info->global_index * array_info->num_elements;
                }

                number_of_elements = 1;
                for (i = dim_id; i < dim_id + array_info->num_dims; i++)
                {
                    if (dim_info.is_var_size_dim[i])
                    {
                        number_of_elements *= dim_info.var_dim[i][array_info->global_index];
                    }
                    else
                    {
                        number_of_elements *= dim_info.dim[i];
                    }
                }
                if (number_of_elements > 0)
                {
                    int i;

                    if (coda_cursor_goto_first_array_element(&traverse_info.cursor) != 0)
                    {
                        handle_coda_error();
                    }
                    for (i = 0; i < number_of_elements; i++)
                    {
                        if (get_record_field_available_status(depth + 1, array_depth + 1, record_depth))
                        {
                            /* available */
                            coda_cursor_goto_parent(&traverse_info.cursor);
                            return 1;
                        }
                        if (i < number_of_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(&traverse_info.cursor) != 0)
                            {
                                handle_coda_error();
                            }
                            traverse_info.array_info[array_depth + 1].global_index++;
                        }
                    }
                    coda_cursor_goto_parent(&traverse_info.cursor);
                }
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
                if (available)
                {
                    if (record_depth == traverse_info.num_records - 1)
                    {
                        /* available */
                        return 1;
                    }
                    else
                    {
                        if (coda_cursor_goto_record_field_by_index(&traverse_info.cursor,
                                                                   traverse_info.parent_index[record_depth]) != 0)
                        {
                            handle_coda_error();
                        }
                        available = get_record_field_available_status(depth + 1, array_depth, record_depth + 1);
                        coda_cursor_goto_parent(&traverse_info.cursor);
                        if (available)
                        {
                            /* available */
                            return 1;
                        }
                    }
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    /* not available */
    return 0;
}

int dim_record_field_available()
{
    /* find out whether there is at least one occurence where this field is available */
    return get_record_field_available_status(0, 0, 0);
}
