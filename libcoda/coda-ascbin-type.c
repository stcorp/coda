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

#include "coda-bin-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int coda_ascbin_type_get_num_record_fields(const coda_Type *type, long *num_fields)
{
    /* this also works for unions */
    *num_fields = ((coda_ascbinRecord *)type)->num_fields;
    return 0;
}

int coda_ascbin_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index)
{
    coda_ascbinRecord *record;

    /* this also works for unions */
    record = (coda_ascbinRecord *)type;

    *index = hashtable_get_index_from_name(record->hash_data, name);
    if (*index >= 0)
    {
        return 0;
    }

    coda_set_error(CODA_ERROR_INVALID_NAME, NULL);
    return -1;
}

int coda_ascbin_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type)
{
    coda_ascbinRecord *record;

    /* this also works for unions */
    record = (coda_ascbinRecord *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *field_type = (coda_Type *)record->field[index]->type;

    return 0;
}

int coda_ascbin_type_get_record_field_name(const coda_Type *type, long index, const char **name)
{
    coda_ascbinRecord *record;

    /* this also works for unions */
    record = (coda_ascbinRecord *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *name = record->field[index]->name;

    return 0;
}

int coda_ascbin_type_get_record_field_hidden_status(const coda_Type *type, long index, int *hidden)
{
    coda_ascbinRecord *record;

    /* this also works for unions */
    record = (coda_ascbinRecord *)type;
    if (index < 0 || index >= record->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       record->num_fields, __FILE__, __LINE__);
        return -1;
    }

    *hidden = record->field[index]->hidden;

    return 0;
}

int coda_ascbin_type_get_record_field_available_status(const coda_Type *type, long index, int *available)
{
    coda_ascbinRecord *record;

    /* this also works for unions */
    record = (coda_ascbinRecord *)type;
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

int coda_ascbin_type_get_record_union_status(const coda_Type *type, int *is_union)
{
    *is_union = (((coda_ascbinType *)type)->tag == tag_ascbin_union);
    return 0;
}

int coda_ascbin_type_get_array_num_dims(const coda_Type *type, int *num_dims)
{
    *num_dims = ((coda_ascbinArray *)type)->num_dims;
    return 0;
}

int coda_ascbin_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[])
{
    coda_ascbinArray *array;
    int i;

    array = (coda_ascbinArray *)type;
    *num_dims = array->num_dims;
    for (i = 0; i < array->num_dims; i++)
    {
        dim[i] = array->dim[i];
    }

    return 0;
}

int coda_ascbin_type_get_array_base_type(const coda_Type *type, coda_Type **base_type)
{
    *base_type = (coda_Type *)((coda_ascbinArray *)type)->base_type;
    return 0;
}
