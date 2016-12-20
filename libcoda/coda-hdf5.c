/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

int coda_hdf5_reopen(coda_product **product)
{
    coda_hdf5_product *product_file;
    int result;

    product_file = (coda_hdf5_product *)malloc(sizeof(coda_hdf5_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_hdf5_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_hdf5;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;
    product_file->file_id = -1;
    product_file->num_objects = 0;
    product_file->object = NULL;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_hdf5_close((coda_product *)product_file);
        coda_close(*product);
        return -1;
    }

    coda_close(*product);

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
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
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
    (void)client_data;

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
