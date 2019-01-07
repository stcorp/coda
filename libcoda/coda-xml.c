/*
 * Copyright (C) 2007-2019 S[&]T, The Netherlands.
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

#include "coda-xml-internal.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-definition.h"

int coda_xml_reopen(coda_product **product)
{
    coda_xml_product *product_file;

    product_file = (coda_xml_product *)malloc(sizeof(coda_xml_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xml_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_xml;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;
    product_file->raw_product = *product;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_close((coda_product *)product_file);
        return -1;
    }

    if (coda_xml_parse(product_file) != 0)
    {
        coda_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_xml_reopen_with_definition(coda_product **product, const coda_product_definition *definition)
{
    coda_xml_product *product_file = *(coda_xml_product **)product;

    assert(definition != NULL);
    assert(product_file->format == coda_format_xml);
    assert(definition->format == coda_format_xml);

    coda_dynamic_type_delete(product_file->root_type);
    product_file->root_type = NULL;
    product_file->mem_size = 0;
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
        product_file->mem_ptr = NULL;
    }
    product_file->product_definition = definition;

    if (coda_xml_parse(product_file) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_xml_close(coda_product *product)
{
    coda_xml_product *product_file = (coda_xml_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete(product_file->root_type);
    }
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
    }
    if (product_file->raw_product != NULL)
    {
        coda_bin_close((coda_product *)product_file->raw_product);
    }

    free(product_file);

    return 0;
}

int coda_xml_cursor_set_product(coda_cursor *cursor, coda_product *product)
{
    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product->root_type;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = 0;
    return 0;
}
