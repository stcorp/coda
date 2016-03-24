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

#include "codadump-filter.h"
#include "codadump.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void get_filter_item(const char *filter_expr, codadump_filter **filter, char **tail)
{
    int n = 0;

    assert(filter_expr != NULL);
    assert(filter != NULL);
    assert(tail != NULL);

    /* strip leading spaces */
    while (filter_expr[0] == ' ' || filter_expr[0] == '\t' || filter_expr[0] == '\n')
    {
        filter_expr = &filter_expr[1];
    }

    /* find end of fieldname */
    while (filter_expr[n] != '\0' && filter_expr[n] != '.' && filter_expr[n] != ';' && filter_expr[n] != ',')
    {
        n++;
    }

    if (n > 0)
    {
        int p = n;

        /* create and initialize new filter item */
        *filter = (codadump_filter *)malloc(sizeof(codadump_filter));
        if (*filter == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           sizeof(codadump_filter), __FILE__, __LINE__);
            handle_coda_error();
        }
        (*filter)->fieldname = (char *)malloc(n + 1);
        if ((*filter)->fieldname == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %u bytes) (%s:%u)",
                           n + 1, __FILE__, __LINE__);
            handle_coda_error();
        }
        strncpy((*filter)->fieldname, filter_expr, n);
        (*filter)->subfilter = NULL;
        (*filter)->next = NULL;

        /* strip trailing spaces of the fieldname */
        while ((*filter)->fieldname[n - 1] == ' ' || (*filter)->fieldname[n - 1] == '\t' ||
               (*filter)->fieldname[n - 1] == '\n')
        {
            n--;
        }
        (*filter)->fieldname[n] = '\0';

        if (filter_expr[p] == '.')
        {
            get_filter_item(&(filter_expr[p + 1]), &(*filter)->subfilter, tail);
            if ((*filter)->subfilter == NULL)
            {
                /* the subfilter was incorrect, so we remove the complete filter item */
                codadump_filter_remove(filter);
            }
        }
        else if (filter_expr[p] == ';' || filter_expr[p] == ',')
        {
            *tail = (char *)&(filter_expr[p + 1]);
        }
        else
        {
            *tail = (char *)&(filter_expr[p]);
        }
    }
    else
    {
        *filter = NULL;
    }
}

static void add_filter(codadump_filter **filter, codadump_filter *new_filter)
{
    assert(filter != NULL);

    if (new_filter == NULL)
    {
        return;
    }

    if (*filter == NULL)
    {
        *filter = new_filter;
        return;
    }

    if (strcmp((*filter)->fieldname, new_filter->fieldname) == 0)
    {
        if ((*filter)->subfilter != NULL)
        {
            if (new_filter->subfilter != NULL)
            {
                add_filter(&(*filter)->subfilter, new_filter->subfilter);
                new_filter->subfilter = NULL;
            }
            else
            {
                codadump_filter_remove(&(*filter)->subfilter);
            }
        }
        codadump_filter_remove(&new_filter);
        return;
    }
    else if ((*filter)->next != NULL)
    {
        add_filter(&(*filter)->next, new_filter);
    }
    else
    {
        (*filter)->next = new_filter;
    }
}

static void get_filter(const char *filter_expr, codadump_filter **filter)
{
    codadump_filter *new_filter = NULL;
    char *expr = (char *)filter_expr;

    if (filter_expr == NULL)
    {
        return;
    }

    while (expr[0] != '\0')
    {
        get_filter_item(expr, &new_filter, &expr);
        if (new_filter == NULL)
        {
            /* the filter item was incorrect, so we remove the whole filter and terminate the parsing */
            codadump_filter_remove(filter);
            return;
        }
        add_filter(filter, new_filter);
        new_filter = NULL;
    }
}


codadump_filter *codadump_filter_create(const char *filter_expr)
{
    codadump_filter *filter = NULL;

    get_filter(filter_expr, &filter);
    return filter;
}

void codadump_filter_remove(codadump_filter **filter)
{
    assert(filter != NULL);

    if (*filter != NULL)
    {
        if ((*filter)->next != NULL)
        {
            codadump_filter_remove(&(*filter)->next);
        }
        if ((*filter)->subfilter != NULL)
        {
            codadump_filter_remove(&(*filter)->subfilter);
        }
        if ((*filter)->fieldname != NULL)
        {
            free((*filter)->fieldname);
            (*filter)->fieldname = NULL;
        }
        free(*filter);
        *filter = NULL;
    }
}

const char *codadump_filter_get_fieldname(codadump_filter *filter)
{
    assert(filter != NULL);

    return filter->fieldname;
}

codadump_filter *codadump_filter_get_subfilter(codadump_filter *filter)
{
    assert(filter != NULL);

    return filter->subfilter;
}

codadump_filter *codadump_filter_get_next_filter(codadump_filter *filter)
{
    assert(filter != NULL);

    return filter->next;
}
