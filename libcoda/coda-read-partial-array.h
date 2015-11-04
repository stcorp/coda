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

#ifndef CODA_READ_PARTIAL_ARRAY_H
#define CODA_READ_PARTIAL_ARRAY_H

#ifndef CODA_READ_FUNC_TYPE_DEF
#define CODA_READ_FUNC_TYPE_DEF
typedef int (*read_function) (const coda_cursor *, void *);
#endif

static int read_partial_array(const coda_cursor *cursor, read_function read_basic_type_function, long offset,
                              long length, uint8_t *dst, int basic_type_size)
{
    coda_cursor array_cursor;
    int i;

    array_cursor = *cursor;

    if (length > 0)
    {
        if (coda_cursor_goto_array_element_by_index(&array_cursor, offset) != 0)
        {
            return -1;
        }
        for (i = 0; i < length; i++)
        {
            if ((*read_basic_type_function)(&array_cursor, &dst[i * basic_type_size]) != 0)
            {
                return -1;
            }
            if (i < length - 1)
            {
                if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

#endif
