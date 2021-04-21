/*
 * Copyright (C) 2007-2021 S[&]T, The Netherlands.
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
