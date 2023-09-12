/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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

#include "coda-mem-internal.h"

#include <stdio.h>
#include <assert.h>

static THREAD_LOCAL coda_mem_record *empty_record_singleton[] =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define num_empty_record_singletons ((int)(sizeof(empty_record_singleton)/sizeof(empty_record_singleton[0])))

static THREAD_LOCAL coda_mem_special *no_data_singleton[] =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

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
