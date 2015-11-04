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

#ifndef CODA_READ_ARRAY_H
#define CODA_READ_ARRAY_H

#ifndef CODA_READ_FUNC_TYPE_DEF
#define CODA_READ_FUNC_TYPE_DEF
typedef int (*read_function) (const coda_cursor *, void *);
#endif

static int read_array(const coda_cursor *cursor, read_function read_basic_type_function, uint8_t *dst,
                      int basic_type_size, coda_array_ordering array_ordering)
{
    coda_cursor array_cursor;
    long dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    long num_elements;
    int i;

    if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    array_cursor = *cursor;
    if (num_dims <= 1 || array_ordering != coda_array_ordering_fortran)
    {
        /* C-style array ordering */

        num_elements = 1;
        for (i = 0; i < num_dims; i++)
        {
            num_elements *= dim[i];
        }

        if (num_elements > 0)
        {
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (i = 0; i < num_elements; i++)
            {
                if ((*read_basic_type_function)(&array_cursor, &dst[i * basic_type_size]) != 0)
                {
                    return -1;
                }
                if (i < num_elements - 1)
                {
                    if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* Fortran-style array ordering */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        num_elements = incr[num_dims];

        if (num_elements > 0)
        {
            c_index = 0;
            fortran_index = 0;
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (;;)
            {
                do
                {
                    if ((*read_basic_type_function)(&array_cursor, &dst[fortran_index * basic_type_size]) != 0)
                    {
                        return -1;
                    }
                    c_index++;
                    if (c_index < num_elements)
                    {
                        if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                        {
                            return -1;
                        }
                    }
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
    }

    return 0;
}

#endif
