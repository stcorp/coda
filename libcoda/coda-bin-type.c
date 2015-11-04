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

#include "coda-bin-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int coda_bin_type_get_read_type(const coda_Type *type, coda_native_type *read_type)
{
    switch (((coda_binType *)type)->tag)
    {
        case tag_bin_integer:
        case tag_bin_float:
            if (coda_option_perform_conversions && ((coda_binNumber *)type)->conversion != NULL)
            {
                *read_type = coda_native_type_double;
            }
            else
            {
                *read_type = ((coda_binNumber *)type)->read_type;
            }
            break;
        case tag_bin_record:
        case tag_bin_union:
        case tag_bin_array:
        case tag_bin_raw:
        case tag_bin_no_data:
            *read_type = coda_native_type_bytes;
            break;
        case tag_bin_vsf_integer:
        case tag_bin_time:
            *read_type = coda_native_type_double;
            break;
        case tag_bin_complex:
            *read_type = coda_native_type_not_available;
            break;
    }

    return 0;
}

int coda_bin_type_get_bit_size(const coda_Type *type, int64_t *bit_size)
{
    *bit_size = ((coda_binType *)type)->bit_size;
    return 0;
}

int coda_bin_type_get_unit(const coda_Type *type, const char **unit)
{
    switch (((coda_binType *)type)->tag)
    {
        case tag_bin_integer:
        case tag_bin_float:
            {
                if (coda_option_perform_conversions)
                {
                    coda_Conversion *conversion = ((coda_binNumber *)type)->conversion;

                    if (conversion != NULL)
                    {
                        *unit = conversion->unit;
                        return 0;
                    }
                }
                *unit = ((coda_binNumber *)type)->unit;
            }
            break;
        case tag_bin_vsf_integer:
            *unit = ((coda_binVSFInteger *)type)->unit;
            break;
        case tag_bin_time:
            *unit = "s since 2000-01-01";
            break;
        case tag_bin_complex:
            /* use unit of element type */
            return coda_bin_type_get_unit((coda_Type *)
                                          ((coda_ascbinRecord *)((coda_binComplex *)type)->base_type)->field[0]->type,
                                          unit);
        default:
            /* no number -> no unit */
            *unit = NULL;
            break;
    }

    return 0;
}

int coda_bin_type_get_fixed_value(const coda_Type *type, const char **fixed_value, long *length)
{
    switch (((coda_binType *)type)->tag)
    {
        case tag_bin_raw:
            *fixed_value = ((coda_binRaw *)type)->fixed_value;
            if (length != NULL)
            {
                *length = ((*fixed_value == NULL) ? 0 : ((coda_binRaw *)type)->fixed_value_length);
            }
            break;
        default:
            /* no fixed value */
            *fixed_value = NULL;
            break;
    }

    return 0;
}

int coda_bin_type_get_special_type(const coda_Type *type, coda_special_type *special_type)
{
    switch (((coda_binType *)type)->tag)
    {
        case tag_bin_no_data:
            *special_type = coda_special_no_data;
            break;
        case tag_bin_vsf_integer:
            *special_type = coda_special_vsf_integer;
            break;
        case tag_bin_time:
            *special_type = coda_special_time;
            break;
        case tag_bin_complex:
            *special_type = coda_special_complex;
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_bin_type_get_special_base_type(const coda_Type *type, coda_Type **base_type)
{
    *base_type = (coda_Type *)((coda_binSpecialType *)type)->base_type;
    return 0;
}
