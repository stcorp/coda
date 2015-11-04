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

int coda_ascbin_type_get_num_record_fields(const coda_type *type, long *num_fields)
{
    /* this also works for unions */
    *num_fields = ((coda_ascbin_record *)type)->num_fields;
    return 0;
}

int coda_ascbin_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;

    *index = hashtable_get_index_from_name(record->hash_data, name);
    if (*index >= 0)
    {
        return 0;
    }

    coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
    return -1;
}

int coda_ascbin_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *field_type = (coda_type *)record->field[index]->type;

    return 0;
}

int coda_ascbin_type_get_record_field_name(const coda_type *type, long index, const char **name)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *name = record->field[index]->name;

    return 0;
}

int coda_ascbin_type_get_record_field_real_name(const coda_type *type, long index, const char **real_name)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    if (record->field[index]->real_name != NULL)
    {
        *real_name = record->field[index]->real_name;
    }
    else
    {
        *real_name = record->field[index]->name;
    }

    return 0;
}

int coda_ascbin_type_get_record_field_hidden_status(const coda_type *type, long index, int *hidden)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *hidden = record->field[index]->hidden;

    return 0;
}

int coda_ascbin_type_get_record_field_available_status(const coda_type *type, long index, int *available)
{
    coda_ascbin_record *record;

    /* this also works for unions */
    record = (coda_ascbin_record *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    if (record->field[index]->available_expr != NULL)
    {
        *available = -1;
    }
    else
    {
        *available = 1;
    }

    return 0;
}

int coda_ascbin_type_get_record_union_status(const coda_type *type, int *is_union)
{
    *is_union = (((coda_ascbin_type *)type)->tag == tag_ascbin_union);
    return 0;
}

int coda_ascbin_type_get_array_num_dims(const coda_type *type, int *num_dims)
{
    *num_dims = ((coda_ascbin_array *)type)->num_dims;
    return 0;
}

int coda_ascbin_type_get_array_dim(const coda_type *type, int *num_dims, long dim[])
{
    coda_ascbin_array *array;
    int i;

    array = (coda_ascbin_array *)type;
    *num_dims = array->num_dims;
    for (i = 0; i < array->num_dims; i++)
    {
        dim[i] = array->dim[i];
    }

    return 0;
}

int coda_ascbin_type_get_array_base_type(const coda_type *type, coda_type **base_type)
{
    *base_type = (coda_type *)((coda_ascbin_array *)type)->base_type;
    return 0;
}
