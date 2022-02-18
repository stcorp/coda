/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
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

#ifndef CODA_READ_PARTIAL_ARRAY_H
#define CODA_READ_PARTIAL_ARRAY_H

#include "coda.h"

#ifndef CODA_READ_FUNC_TYPE_DEF
#define CODA_READ_FUNC_TYPE_DEF
typedef int (*read_function)(const coda_cursor *, void *);
#endif

static int read_partial_array(const coda_cursor *cursor, read_function read_basic_type_function, long offset,
                              long length, uint8_t *dst, int basic_type_size)
{
    coda_cursor array_cursor;
    int i;

    array_cursor = *cursor;

    if (length > 0)
    {
        if (coda_cursor_goto_array_element_by_index(&array_cursor, offset) != 0)
        {
            return -1;
        }
        for (i = 0; i < length; i++)
        {
            if ((*read_basic_type_function)(&array_cursor, &dst[i * basic_type_size]) != 0)
            {
                return -1;
            }
            if (i < length - 1)
            {
                if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

#endif
