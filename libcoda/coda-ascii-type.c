/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#include "coda-ascii-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int coda_ascii_type_get_read_type(const coda_type *type, coda_native_type *read_type)
{
    switch (((coda_ascii_type *)type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            if (coda_option_perform_conversions && ((coda_ascii_number *)type)->conversion != NULL)
            {
                *read_type = coda_native_type_double;
            }
            else
            {
                *read_type = ((coda_ascii_number *)type)->read_type;
            }
            break;
        case tag_ascii_text:
            *read_type = ((coda_ascii_text *)type)->read_type;
            break;
        case tag_ascii_line_separator:
        case tag_ascii_line:
        case tag_ascii_white_space:
            *read_type = coda_native_type_string;
            break;
        case tag_ascii_record:
        case tag_ascii_union:
        case tag_ascii_array:
            *read_type = coda_native_type_bytes;
            break;
        case tag_ascii_time:
            *read_type = coda_native_type_double;
            break;
    }

    return 0;
}

int coda_ascii_type_get_string_length(const coda_type *type, long *length)
{
    switch (((coda_ascii_type *)type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
        case tag_ascii_text:
        case tag_ascii_line_separator:
        case tag_ascii_line:
        case tag_ascii_white_space:
        case tag_ascii_time:
            {
                int64_t bit_size;

                if (coda_ascii_type_get_bit_size(type, &bit_size) != 0)
                {
                    return -1;
                }
                *length = (bit_size == -1 ? -1 : (long)(bit_size >> 3));
            }
            break;
        case tag_ascii_array:
        case tag_ascii_record:
        case tag_ascii_union:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_ascii_type_get_bit_size(const coda_type *type, int64_t *bit_size)
{
    *bit_size = ((coda_ascii_type *)type)->bit_size;
    return 0;
}

int coda_ascii_type_get_unit(const coda_type *type, const char **unit)
{
    switch (((coda_ascii_type *)type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                if (coda_option_perform_conversions)
                {
                    coda_conversion *conversion = ((coda_ascii_number *)type)->conversion;

                    if (conversion != NULL)
                    {
                        *unit = conversion->unit;
                        return 0;
                    }
                }
                *unit = ((coda_ascii_number *)type)->unit;
            }
            break;
        case tag_ascii_time:
            *unit = "s since 2000-01-01";
            break;
        default:
            /* no number -> no unit */
            *unit = NULL;
            break;
    }

    return 0;
}

int coda_ascii_type_get_fixed_value(const coda_type *type, const char **fixed_value, long *length)
{
    switch (((coda_ascii_type *)type)->tag)
    {
        case tag_ascii_text:
            *fixed_value = ((coda_ascii_text *)type)->fixed_value;
            if (length != NULL)
            {
                *length = ((*fixed_value == NULL) ? 0 : strlen(*fixed_value));
            }
            break;
        default:
            /* no fixed value */
            *fixed_value = NULL;
            break;
    }

    return 0;
}

int coda_ascii_type_get_special_type(const coda_type *type, coda_special_type *special_type)
{
    switch (((coda_ascii_type *)type)->tag)
    {
        case tag_ascii_time:
            *special_type = coda_special_time;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_ascii_type_get_special_base_type(const coda_type *type, coda_type **base_type)
{
    *base_type = (coda_type *)((coda_ascii_special_type *)type)->base_type;
    return 0;
}
