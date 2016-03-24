/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#include "coda-mem-internal.h"

#include <stdio.h>
#include <assert.h>

static coda_mem_record *empty_record_singleton[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define num_empty_record_singletons ((int)(sizeof(empty_record_singleton)/sizeof(empty_record_singleton[0])))

static coda_mem_special *no_data_singleton[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define num_no_data_singletons ((int)(sizeof(no_data_singleton)/sizeof(no_data_singleton[0])))

coda_dynamic_type *coda_mem_empty_record(coda_format format)
{
    assert(format < num_empty_record_singletons);
    if (empty_record_singleton[format] == NULL)
    {
        empty_record_singleton[format] = coda_mem_record_new(coda_type_empty_record(format), NULL);
        assert(empty_record_singleton[format] != NULL);
    }

    return (coda_dynamic_type *)empty_record_singleton[format];
}

coda_dynamic_type *coda_no_data_singleton(coda_format format)
{
    assert(format < num_no_data_singletons);
    if (no_data_singleton[format] == NULL)
    {
        no_data_singleton[format] = coda_mem_no_data_new(format);
        assert(no_data_singleton[format] != NULL);
    }

    return (coda_dynamic_type *)no_data_singleton[format];
}

void coda_mem_done(void)
{
    int i;

    for (i = 0; i < num_empty_record_singletons; i++)
    {
        if (empty_record_singleton[i] != NULL)
        {
            coda_mem_type_delete((coda_dynamic_type *)empty_record_singleton[i]);
        }
        empty_record_singleton[i] = NULL;
    }
    for (i = 0; i < num_no_data_singletons; i++)
    {
        if (no_data_singleton[i] != NULL)
        {
            coda_mem_type_delete((coda_dynamic_type *)no_data_singleton[i]);
        }
        no_data_singleton[i] = NULL;
    }
}
