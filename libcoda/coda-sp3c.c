/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#include "coda-sp3c.h"
#include "coda-ascbin.h"
#include "coda-mem-internal.h"

#include <stdlib.h>
#include <string.h>

static int read_file(const char *filename, coda_dynamic_type **root)
{
    coda_type_record *definition;
    FILE *f;

    f = fopen(filename, "r");
    if (f == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", filename);
        return -1;
    }

    fclose(f);

    definition = coda_type_record_new(coda_format_sp3c);
    if (definition == NULL)
    {
        return -1;
    }
    *root = (coda_dynamic_type *)coda_mem_record_new(definition);
    if (*root == NULL)
    {
        coda_type_release((coda_type *)definition);
        return -1;
    }

    return 0;
}

int coda_sp3c_open(const char *filename, int64_t file_size, coda_product **product)
{
    coda_product *product_file;
    coda_product_definition *product_definition;

    /* We currently use the ascii detection tree to assign product class/type to SP3-c files */
    if (coda_ascbin_recognize_file(filename, file_size, &product_definition) != 0)
    {
        return -1;
    }

    product_file = (coda_product *)malloc(sizeof(coda_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_sp3c;
    product_file->root_type = NULL;
    product_file->product_definition = product_definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_sp3c_close(product_file);
        return -1;
    }

    /* create root type */
    if (read_file(filename, &product_file->root_type) != 0)
    {
        coda_sp3c_close(product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_sp3c_close(coda_product *product)
{
    if (product->root_type != NULL)
    {
        coda_dynamic_type_delete(product->root_type);
    }

    if (product->filename != NULL)
    {
        free(product->filename);
    }

    free(product);

    return 0;
}

int coda_sp3c_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = -1;   /* not applicable for memory backend */

    return 0;
}
