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

#ifndef CODA_BIN_INTERNAL_H
#define CODA_BIN_INTERNAL_H

#include "coda-bin.h"

struct coda_bin_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    const uint8_t *mem_ptr;

    /* 'bin' product specific fields */
    int use_mmap;       /* indicates whether to use mem_ptr (or the file descriptor 'fd') */
    int fd;     /* file handle when not using mem_ptr */
#ifdef WIN32
    HANDLE file;
    HANDLE file_mapping;
#endif
};
typedef struct coda_bin_product_struct coda_bin_product;

int coda_bin_product_open(coda_bin_product *product);
int coda_bin_product_close(coda_bin_product *product);

#endif
