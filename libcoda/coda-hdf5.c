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

#include "coda-hdf5-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int coda_hdf5_init(void)
{
    /* Don't let HDF5 print error messages to the console */
    H5Eset_auto(NULL, NULL);
    return 0;
}

int coda_hdf5_open(const char *filename, int64_t file_size, const coda_product_definition *definition,
                   coda_product **product)
{
    coda_hdf5_product *product_file;
    int result;

    product_file = (coda_hdf5_product *)malloc(sizeof(coda_hdf5_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_hdf5_product), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_hdf5;
    product_file->root_type = NULL;
    product_file->product_definition = definition;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
#if CODA_USE_QIAP
    product_file->qiap_info = NULL;
#endif
    product_file->file_id = -1;
    product_file->num_objects = 0;
    product_file->object = NULL;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }

    product_file->file_id = H5Fopen(product_file->filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (product_file->file_id < 0)
    {
        coda_set_error(CODA_ERROR_HDF5, NULL);
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }

    result = coda_hdf5_create_tree(product_file, product_file->file_id, ".", &product_file->root_type);
    if (result == -1)
    {
        coda_hdf5_close((coda_product *)product_file);
        return -1;
    }
    /* the root type is a vgroup and it should not be possible to ignore the root vgroup */
    assert(result != 1);

    *product = (coda_product *)product_file;

    return 0;
}

int coda_hdf5_close(coda_product *product)
{
    coda_hdf5_product *product_file = (coda_hdf5_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)product_file->root_type);
    }
    if (product_file->object != NULL)
    {
        free(product_file->object);
    }
    if (product_file->file_id >= 0)
    {
        if (H5Fclose(product_file->file_id) < 0)
        {
            coda_set_error(CODA_ERROR_HDF5, NULL);
            return -1;
        }
    }

    free(product_file);

    return 0;
}

static herr_t add_error_message(int n, H5E_error_t *err_desc, void *client_data)
{
    client_data = client_data;

    if (n == 0)
    {
        /* we only display the deepest error in the stack */
        coda_add_error_message("[HDF5] %s(): %s (major=\"%s\", minor=\"%s\") (%s:%u)", err_desc->func_name,
                               err_desc->desc, H5Eget_major(err_desc->maj_num), H5Eget_minor(err_desc->min_num),
                               err_desc->file_name, err_desc->line);
    }

    return 0;
}

void coda_hdf5_add_error_message(void)
{
    H5Ewalk(H5E_WALK_UPWARD, add_error_message, NULL);
}
