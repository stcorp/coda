/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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

#ifndef PDSDUMP_FILTER_H
#define PDSDUMP_FILTER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "coda.h"

typedef struct codadump_filter
{
    char *fieldname;

    struct codadump_filter *subfilter;
    struct codadump_filter *next;
} codadump_filter;

codadump_filter *codadump_filter_create(const char *filter_expr);
void codadump_filter_remove(codadump_filter **filter);
const char *codadump_filter_get_fieldname(codadump_filter *filter);
codadump_filter *codadump_filter_get_subfilter(codadump_filter *filter);
codadump_filter *codadump_filter_get_next_filter(codadump_filter *filter);

#endif
