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

#ifndef CODA_TRANSPOSE_ARRAY_H
#define CODA_TRANSPOSE_ARRAY_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int transpose_array(const coda_cursor *cursor, void *array, int element_size)
{
    long dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    long num_elements;
    long multiplier[CODA_MAX_NUM_DIMS + 1];
    long rsub[CODA_MAX_NUM_DIMS + 1];   /* reversed index in multi dim array */
    long rdim[CODA_MAX_NUM_DIMS + 1];   /* reversed order of dim[] */
    long index = 0;
    long i;
    uint8_t *src;
    uint8_t *dst;

    if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    if (num_dims <= 1)
    {
        /* nothing to do */
        return 0;
    }

    src = (uint8_t *)array;

    num_elements = 1;
    for (i = 0; i < num_dims; i++)
    {
        num_elements *= dim[i];
        rsub[i] = 0;
        rdim[i] = dim[num_dims - 1 - i];
    }
    if (num_elements <= 1)
    {
        /* nothing to do */
        return 0;
    }

    multiplier[num_dims] = 1;
    rdim[num_dims] = 1;
    for (i = num_dims; i > 0; i--)
    {
        multiplier[i - 1] = multiplier[i] * rdim[i];
    }
    rdim[num_dims] = 0;
    rsub[num_dims] = 0;

    dst = (uint8_t *)malloc(num_elements * element_size);
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_elements * element_size, __FILE__, __LINE__);
        return -1;
    }

    switch (element_size)
    {
        case 1:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                dst[index] = src[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 2:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint16_t *)dst)[index] = ((uint16_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 4:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint32_t *)dst)[index] = ((uint32_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        case 8:
            for (i = 0; i < num_elements; i++)
            {
                int j = 0;

                ((uint64_t *)dst)[index] = ((uint64_t *)src)[i];
                index += multiplier[j];
                rsub[j]++;
                while (rsub[j] == rdim[j])
                {
                    rsub[j] = 0;
                    index -= multiplier[j] * rdim[j];
                    j++;
                    index += multiplier[j];
                    rsub[j]++;
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    memcpy(array, dst, num_elements * element_size);

    free(dst);

    return 0;
}

#endif
