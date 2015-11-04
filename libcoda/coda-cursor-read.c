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

#include "coda-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-bin.h"
#include "coda-mem.h"
#include "coda-xml.h"
#include "coda-netcdf.h"
#include "coda-grib.h"
#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif
#include "coda-type.h"

/** \file */

typedef int (*read_function) (const coda_cursor *, void *);

static int get_read_type(const coda_cursor *cursor, coda_native_type *read_type)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    if ((type->type_class == coda_integer_class || type->type_class == coda_real_class) &&
        coda_option_perform_conversions && ((coda_type_number *)type)->conversion != NULL)
    {
        *read_type = coda_native_type_double;
    }
    else
    {
        *read_type = type->read_type;
    }

    return 0;
}

static int get_unconverted_read_type(const coda_cursor *cursor, coda_native_type *read_type,
                                     coda_conversion **conversion)
{
    coda_type *type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);

    *read_type = type->read_type;

    if ((type->type_class == coda_integer_class || type->type_class == coda_real_class) &&
        coda_option_perform_conversions)
    {
        *conversion = ((coda_type_number *)type)->conversion;
    }
    else
    {
        *conversion = NULL;
    }

    return 0;
}

static int get_array_element_read_type(coda_type *type, coda_native_type *read_type)
{
    coda_type *base_type = ((coda_type_array *)type)->base_type;

    if ((base_type->type_class == coda_integer_class || base_type->type_class == coda_real_class) &&
        coda_option_perform_conversions && ((coda_type_number *)base_type)->conversion != NULL)
    {
        *read_type = coda_native_type_double;
    }
    else
    {
        *read_type = base_type->read_type;
    }

    return 0;
}

static int get_array_element_unconverted_read_type(coda_type *type, coda_native_type *read_type,
                                                   coda_conversion **conversion)
{
    coda_type *base_type = ((coda_type_array *)type)->base_type;

    *read_type = base_type->read_type;

    if ((base_type->type_class == coda_integer_class || base_type->type_class == coda_real_class) &&
        coda_option_perform_conversions)
    {
        *conversion = ((coda_type_number *)base_type)->conversion;
    }
    else
    {
        *conversion = NULL;
    }

    return 0;
}

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

static int read_array(const coda_cursor *cursor, read_function read_basic_type_function, uint8_t *dst,
                      int basic_type_size, coda_array_ordering array_ordering)
{
    coda_cursor array_cursor;
    long dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    long num_elements;
    int i;

    if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    array_cursor = *cursor;
    if (num_dims <= 1 || array_ordering != coda_array_ordering_fortran)
    {
        /* C-style array ordering */

        num_elements = 1;
        for (i = 0; i < num_dims; i++)
        {
            num_elements *= dim[i];
        }

        if (num_elements > 0)
        {
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (i = 0; i < num_elements; i++)
            {
                if ((*read_basic_type_function)(&array_cursor, &dst[i * basic_type_size]) != 0)
                {
                    return -1;
                }
                if (i < num_elements - 1)
                {
                    if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* Fortran-style array ordering */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        num_elements = incr[num_dims];

        if (num_elements > 0)
        {
            c_index = 0;
            fortran_index = 0;
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (;;)
            {
                do
                {
                    if ((*read_basic_type_function)(&array_cursor, &dst[fortran_index * basic_type_size]) != 0)
                    {
                        return -1;
                    }
                    c_index++;
                    if (c_index < num_elements)
                    {
                        if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                        {
                            return -1;
                        }
                    }
                    fortran_index += increment;
                } while (fortran_index < num_elements);

                if (c_index == num_elements)
                {
                    break;
                }

                fortran_index += incr[num_dims - 2] - incr[num_dims];
                i = num_dims - 3;
                while (i >= 0 && fortran_index >= incr[i + 2])
                {
                    fortran_index += incr[i] - incr[i + 2];
                    i--;
                }
            }
        }
    }

    return 0;
}

static int read_split_array(const coda_cursor *cursor, read_function read_basic_type_function, uint8_t *dst_1,
                            uint8_t *dst_2, int basic_type_size, coda_array_ordering array_ordering)
{
    coda_cursor array_cursor;
    uint8_t buffer[16];
    long dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    long num_elements;
    int i;

    if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    array_cursor = *cursor;
    if (num_dims <= 1 || array_ordering != coda_array_ordering_fortran)
    {
        /* C-style array ordering */

        num_elements = 1;
        for (i = 0; i < num_dims; i++)
        {
            num_elements *= dim[i];
        }

        if (num_elements > 0)
        {
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (i = 0; i < num_elements; i++)
            {
                (*read_basic_type_function)(&array_cursor, buffer);
                memcpy(&dst_1[i * basic_type_size], &buffer[0], basic_type_size);
                memcpy(&dst_2[i * basic_type_size], &buffer[basic_type_size], basic_type_size);
                if (i < num_elements - 1)
                {
                    if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* Fortran-style array ordering */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        num_elements = incr[num_dims];

        if (num_elements > 0)
        {
            c_index = 0;
            fortran_index = 0;
            if (coda_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (;;)
            {
                do
                {
                    (*read_basic_type_function)(&array_cursor, buffer);
                    memcpy(&dst_1[fortran_index * basic_type_size], &buffer[0], basic_type_size);
                    memcpy(&dst_2[fortran_index * basic_type_size], &buffer[basic_type_size], basic_type_size);
                    c_index++;
                    if (c_index < num_elements)
                    {
                        if (coda_cursor_goto_next_array_element(&array_cursor) != 0)
                        {
                            return -1;
                        }
                    }
                    fortran_index += increment;
                } while (fortran_index < num_elements);

                if (c_index == num_elements)
                {
                    break;
                }

                fortran_index += incr[num_dims - 2] - incr[num_dims];
                i = num_dims - 3;
                while (i >= 0 && fortran_index >= incr[i + 2])
                {
                    fortran_index += incr[i] - incr[i + 2];
                    i--;
                }
            }
        }
    }

    return 0;
}

static int read_int8(const coda_cursor *cursor, int8_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_int8(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_int8(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_int8(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_int8(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_int8(cursor, dst);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_uint8(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_uint8(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_uint8(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_uint8(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_int16(const coda_cursor *cursor, int16_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_int16(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_int16(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_int16(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_int16(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_int16(cursor, dst);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_uint16(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_uint16(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_uint16(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_uint16(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_int32(const coda_cursor *cursor, int32_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_int32(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_int32(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_int32(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_int32(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_int32(cursor, dst);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_uint32(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_uint32(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_uint32(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_uint32(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_int64(const coda_cursor *cursor, int64_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_int64(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_int64(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_int64(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_int64(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_uint64(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_uint64(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_uint64(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_uint64(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_float(const coda_cursor *cursor, float *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_float(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_float(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_float(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_float(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_float(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_float(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_float(cursor, dst);
        case coda_backend_grib:
            return coda_grib_cursor_read_float(cursor, dst);
    }

    assert(0);
    exit(1);
}

static int read_double(const coda_cursor *cursor, double *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_double(cursor, dst, -1);
        case coda_backend_binary:
            return coda_bin_cursor_read_double(cursor, dst);
        case coda_backend_memory:
            return coda_mem_cursor_read_double(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_double(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_double(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_double(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_double(cursor, dst);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_char(const coda_cursor *cursor, char *dst)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_char(cursor, dst, -1);
        case coda_backend_binary:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
            return -1;
        case coda_backend_memory:
            return coda_mem_cursor_read_char(cursor, dst);
        case coda_backend_xml:
            return coda_xml_cursor_read_char(cursor, dst);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_char(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_char(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_char(cursor, dst);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}

static int read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_string(cursor, dst, dst_size, -1);
        case coda_backend_binary:
            break;
        case coda_backend_memory:
            return coda_mem_cursor_read_string(cursor, dst, dst_size);
        case coda_backend_xml:
            return coda_xml_cursor_read_string(cursor, dst, dst_size);
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_string(cursor, dst, dst_size);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_string(cursor, dst, dst_size);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            return coda_netcdf_cursor_read_string(cursor, dst, dst_size);
        case coda_backend_grib:
            break;
    }

    assert(0);
    exit(1);
}


static int read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_int8_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_int8, (uint8_t *)dst, sizeof(int8_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_int8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_int8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_int8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_int8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_int8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(int8_t));
    }

    return 0;
}

static int read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_uint8_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_uint8, (uint8_t *)dst, sizeof(uint8_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_uint8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_uint8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_uint8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_uint8_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(uint8_t));
    }

    return 0;
}

static int read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_int16_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_int16, (uint8_t *)dst, sizeof(int16_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_int16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_int16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_int16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_int16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_int16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(int16_t));
    }

    return 0;
}

static int read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_uint16_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_uint16, (uint8_t *)dst, sizeof(uint16_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_uint16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_uint16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_uint16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_uint16_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(uint16_t));
    }

    return 0;
}

static int read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_int32_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_int32, (uint8_t *)dst, sizeof(int32_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_int32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_int32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_int32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_int32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_int32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(int32_t));
    }

    return 0;
}

static int read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_uint32_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_uint32, (uint8_t *)dst, sizeof(uint32_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_uint32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_uint32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_uint32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_uint32_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(uint32_t));
    }

    return 0;
}

static int read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_int64_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_int64, (uint8_t *)dst, sizeof(int64_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_int64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_int64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_int64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_int64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(int64_t));
    }

    return 0;
}

static int read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_uint64_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_uint64, (uint8_t *)dst, sizeof(uint64_t), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_uint64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_uint64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_uint64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_uint64_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(uint64_t));
    }

    return 0;
}

static int read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_float_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_float, (uint8_t *)dst, sizeof(float), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            if (coda_grib_cursor_read_float_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(float));
    }

    return 0;
}

static int read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_double_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_double, (uint8_t *)dst, sizeof(double), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_double_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_double_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_double_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_double_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_double_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(double));
    }

    return 0;
}

static int read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            if (coda_ascii_cursor_read_char_array(cursor, dst, -1) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_binary:
            return read_array(cursor, (read_function)&read_char, (uint8_t *)dst, sizeof(char), array_ordering);
        case coda_backend_memory:
            if (coda_mem_cursor_read_char_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_xml:
            if (coda_xml_cursor_read_char_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_cursor_read_char_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_cursor_read_char_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_backend_netcdf:
            if (coda_netcdf_cursor_read_char_array(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_backend_grib:
            assert(0);
            exit(1);
    }

    if (array_ordering != coda_array_ordering_c)
    {
        return transpose_array(cursor, dst, sizeof(char));
    }

    return 0;
}

/** \addtogroup coda_cursor
 * @{
 */

/** Retrieve data as type \c int8 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int8(const coda_cursor *cursor, int8_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c uint8 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            if (read_uint8(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c int16 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int16(const coda_cursor *cursor, int16_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (read_int8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int16_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int16_t)value;
            }
            break;
        case coda_native_type_int16:
            if (read_int16(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c uint16 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint16_t)value;
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c int32 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int32(const coda_cursor *cursor, int32_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (read_int8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (read_int16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int32_t)value;
            }
            break;
        case coda_native_type_int32:
            if (read_int32(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c uint32 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint32_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint32_t)value;
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c int64 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int64(const coda_cursor *cursor, int64_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (read_int8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (read_int16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (read_int32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (read_uint32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (int64_t)value;
            }
            break;
        case coda_native_type_int64:
            if (read_int64(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c uint64 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 * - \c uint64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (read_uint32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (uint64_t)value;
            }
            break;
        case coda_native_type_uint64:
            if (read_uint64(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c float from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_float(const coda_cursor *cursor, float *dst)
{
    coda_native_type read_type;
    coda_conversion *conversion;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_unconverted_read_type(cursor, &read_type, &conversion) != 0)
    {
        return -1;
    }
    if (conversion != NULL)
    {
        double value;

        /* let the conversion be performed by coda_cursor_read_double() and cast the result */
        if (coda_cursor_read_double(cursor, &value) != 0)
        {
            return -1;
        }
        *dst = (float)value;
        return 0;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (read_int8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (read_int16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (read_int32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (read_uint32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (read_int64(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (read_uint64(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)(int64_t)value;
            }
            break;
        case coda_native_type_float:
            if (read_float(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        case coda_native_type_double:
            {
                double value;

                if (read_double(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (float)value;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve data as type \c double from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_double(const coda_cursor *cursor, double *dst)
{
    coda_native_type read_type;
    coda_conversion *conversion;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_unconverted_read_type(cursor, &read_type, &conversion) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            {
                int8_t value;

                if (read_int8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint8:
            {
                uint8_t value;

                if (read_uint8(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int16:
            {
                int16_t value;

                if (read_int16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint16:
            {
                uint16_t value;

                if (read_uint16(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int32:
            {
                int32_t value;

                if (read_int32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint32:
            {
                uint32_t value;

                if (read_uint32(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_int64:
            {
                int64_t value;

                if (read_int64(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t value;

                if (read_uint64(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)(int64_t)value;
            }
            break;
        case coda_native_type_float:
            {
                float value;

                if (read_float(cursor, &value) != 0)
                {
                    return -1;
                }
                *dst = (double)value;
            }
            break;
        case coda_native_type_double:
            if (read_double(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }
    if (conversion != NULL)
    {
        if (*dst == conversion->invalid_value)
        {
            *dst = coda_NaN();
        }
        else
        {
            *dst = (*dst * conversion->numerator) / conversion->denominator + conversion->add_offset;
        }
    }

    return 0;
}

/** Retrieve data as type \c char from the product file. The value is stored in \a dst.
 * The cursor must point to data with read type \c char to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_char(const coda_cursor *cursor, char *dst)
{
    coda_native_type read_type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_read_type(cursor, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_char:
            if (read_char(cursor, dst) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve text data as a 0 terminated string. The value is stored in \a dst.
 * You will be able to read data as a string if the cursor points to data with type class #coda_text_class or if
 * it points to ASCII data (see coda_cursor_has_ascii_content()). For other cases the function will return an error.
 * The function will fill at most \a dst_size bytes in the dst memory space. The last character that is put in \a dst
 * will always be a zero termination character, which means that at most \a dst_size - 1 characters of text data will
 * be read.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param dst_size The maximum number of bytes to write in \a dst.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size)
{
    int has_ascii_content;

    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_size <= 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_size (%ld) argument is <= 0 (%s:%u)", dst_size, __FILE__,
                       __LINE__);
        return -1;
    }

    if (coda_cursor_has_ascii_content(cursor, &has_ascii_content) != 0)
    {
        return -1;
    }
    if (!has_ascii_content)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to text (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    return read_string(cursor, dst, dst_size);
}

/** Read a specified amount of bits. The data is stored in \a dst.
 * This function will work independent of the type of data at the current cursor position, but it will not work on
 * ASCII, XML, HDF4, and HDF5 data.
 * The function will read a \a bit_length amount of bits starting from the sum of the cursor offset position and the
 * amount of bits given by the \a bit_offset parameter. If \a bit_length is not a rounded amount of bytes the data will
 * be put in the first \a bit_length/8 + 1 bytes and will be right adjusted (i.e. padding bits with value 0 will be put
 * in the most significant bits of the first byte of \a dst).
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param bit_offset The offset relative to the current cursor position from where the bits should be read.
 * \param bit_length The number of bits to read.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_length < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_length argument is negative (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_length == 0)
    {
        return 0;
    }

    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_backend_binary:
            return coda_bin_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_backend_memory:
            return coda_mem_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_backend_xml:
            return coda_xml_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_backend_hdf4:
        case coda_backend_hdf5:
        case coda_backend_netcdf:
        case coda_backend_grib:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bits data type");
    return -1;
}

/** Read a specified amount of bytes. The data is stored in \a dst.
 * This function will work independent of the type of data at the current cursor position, but it will not work on HDF4
 * and HDF5 files. For XML files it will only work if the cursor points to a single element (i.e. you will get an error
 * when the cursor points to an XML element array or an attribute record).
 * The function will read a \a length amount of bytes starting from the sum of the cursor offset position and the
 * amount of bytes given by the \a offset parameter.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param offset The offset relative to the current cursor position from where the bytes should be read.
 * \param length The number of bytes to read.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (offset < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "offset argument is negative (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (length < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length argument is negative (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (length == 0)
    {
        return 0;
    }

    switch (cursor->stack[cursor->n - 1].type->backend)
    {
        case coda_backend_ascii:
            return coda_ascii_cursor_read_bytes(cursor, dst, offset, length);
        case coda_backend_binary:
            return coda_bin_cursor_read_bytes(cursor, dst, offset, length);
        case coda_backend_memory:
            return coda_mem_cursor_read_bytes(cursor, dst, offset, length);
        case coda_backend_xml:
            return coda_xml_cursor_read_bytes(cursor, dst, offset, length);
        case coda_backend_grib:
        case coda_backend_hdf4:
        case coda_backend_hdf5:
        case coda_backend_netcdf:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bytes data type");
    return -1;
}

/** Retrieve a data array as type \c int8 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c uint8 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst,
                                             coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c int16 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst,
                                             coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, (int8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int16_t)((int8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int16_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_int16:
            if (read_int16_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c uint16 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst,
                                              coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint16_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c int32 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst,
                                             coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, (int8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int32_t)((int8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int32_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_int16:
            if (read_int16_array(cursor, (int16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int32_t)((int16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int32_t)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_int32:
            if (read_int32_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c uint32 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst,
                                              coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint32_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint32_t)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c int64 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst,
                                             coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, (int8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((int8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_int16:
            if (read_int16_array(cursor, (int16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((int16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_int32:
            if (read_int32_array(cursor, (int32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((int32_t *)dst)[i];
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32_array(cursor, (uint32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (int64_t)((uint32_t *)dst)[i];
            }
            break;
        case coda_native_type_int64:
            if (read_int64_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c uint64 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 * - \c uint64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst,
                                              coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint64_t)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint64_t)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32_array(cursor, (uint32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (uint64_t)((uint32_t *)dst)[i];
            }
            break;
        case coda_native_type_uint64:
            if (read_uint64_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c float from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_conversion *conversion;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_unconverted_read_type(type, &read_type, &conversion) != 0)
    {
        return -1;
    }
    if (conversion != NULL)
    {
        double *array;

        /* let the conversion be performed by coda_cursor_read_double_array() and cast the result */
        if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }
        array = malloc(num_elements * sizeof(double));
        if (array == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)num_elements * sizeof(double), __FILE__, __LINE__);
            return -1;
        }
        if (coda_cursor_read_double_array(cursor, array, array_ordering) != 0)
        {
            free(array);
            return -1;
        }
        for (i = num_elements - 1; i >= 0; i--)
        {
            dst[i] = (float)array[i];
        }
        free(array);
        return 0;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, (int8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((int8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_int16:
            if (read_int16_array(cursor, (int16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((int16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_int32:
            if (read_int32_array(cursor, (int32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((int32_t *)dst)[i];
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32_array(cursor, (uint32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (float)((uint32_t *)dst)[i];
            }
            break;
        case coda_native_type_int64:
            {
                int64_t *array;

                if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
                {
                    return -1;
                }
                array = malloc(num_elements * sizeof(int64_t));
                if (array == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)num_elements * sizeof(int64_t), __FILE__, __LINE__);
                    return -1;
                }
                if (read_int64_array(cursor, array, array_ordering) != 0)
                {
                    free(array);
                    return -1;
                }
                for (i = num_elements - 1; i >= 0; i--)
                {
                    dst[i] = (float)array[i];
                }
                free(array);
            }
            break;
        case coda_native_type_uint64:
            {
                uint64_t *array;

                if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
                {
                    return -1;
                }
                array = malloc(num_elements * sizeof(uint64_t));
                if (array == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)num_elements * sizeof(int64_t), __FILE__, __LINE__);
                    return -1;
                }
                if (read_uint64_array(cursor, array, array_ordering) != 0)
                {
                    free(array);
                    return -1;
                }
                for (i = num_elements - 1; i >= 0; i--)
                {
                    dst[i] = (float)array[i];
                }
                free(array);
            }
            break;
        case coda_native_type_float:
            if (read_float_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        case coda_native_type_double:
            {
                double *array;

                if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
                {
                    return -1;
                }
                array = malloc(num_elements * sizeof(double));
                if (array == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)num_elements * sizeof(double), __FILE__, __LINE__);
                    return -1;
                }
                if (read_double_array(cursor, array, array_ordering) != 0)
                {
                    free(array);
                    return -1;
                }
                for (i = num_elements - 1; i >= 0; i--)
                {
                    dst[i] = (float)array[i];
                }
                free(array);
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve a data array as type \c double from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_double_array(const coda_cursor *cursor, double *dst,
                                              coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_conversion *conversion;
    coda_type *type;
    long num_elements;
    long i;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_unconverted_read_type(type, &read_type, &conversion) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_int8:
            if (read_int8_array(cursor, (int8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((int8_t *)dst)[i];
            }
            break;
        case coda_native_type_uint8:
            if (read_uint8_array(cursor, (uint8_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((uint8_t *)dst)[i];
            }
            break;
        case coda_native_type_int16:
            if (read_int16_array(cursor, (int16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((int16_t *)dst)[i];
            }
            break;
        case coda_native_type_uint16:
            if (read_uint16_array(cursor, (uint16_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((uint16_t *)dst)[i];
            }
            break;
        case coda_native_type_int32:
            if (read_int32_array(cursor, (int32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((int32_t *)dst)[i];
            }
            break;
        case coda_native_type_uint32:
            if (read_uint32_array(cursor, (uint32_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((uint32_t *)dst)[i];
            }
            break;
        case coda_native_type_int64:
            if (read_int64_array(cursor, (int64_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((int64_t *)dst)[i];
            }
            break;
        case coda_native_type_uint64:
            if (read_uint64_array(cursor, (uint64_t *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((uint64_t *)dst)[i];
            }
            break;
        case coda_native_type_float:
            if (read_float_array(cursor, (float *)dst, array_ordering) != 0)
            {
                return -1;
            }
            if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
            {
                return -1;
            }
            for (i = num_elements - 1; i >= 0; i--)
            {
                dst[i] = (double)((float *)dst)[i];
            }
            break;
        case coda_native_type_double:
            if (read_double_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }
    if (conversion != NULL)
    {
        if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
        {
            return -1;
        }
        for (i = 0; i < num_elements; i++)
        {
            if (dst[i] == conversion->invalid_value)
            {
                dst[i] = coda_NaN();
            }
            else
            {
                dst[i] = (dst[i] * conversion->numerator) / conversion->denominator + conversion->add_offset;
            }
        }
    }
    return 0;
}

/** Retrieve a data array as type \c char from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has read type \c char to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    coda_native_type read_type;
    coda_type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (get_array_element_read_type(type, &read_type) != 0)
    {
        return -1;
    }
    switch (read_type)
    {
        case coda_native_type_char:
            if (read_char_array(cursor, dst, array_ordering) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a char data type",
                           coda_type_get_native_type_name(read_type));
            return -1;
    }

    return 0;
}

/** Retrieve complex data as type \c double from the product file.
 * The real and imaginary values are stored consecutively in \a dst.
 * The cursor must point to data with special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_pair(const coda_cursor *cursor, double *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->backend != coda_backend_binary)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
        return -1;
    }
    return coda_bin_cursor_read_double_pair(cursor, dst);
}

/** Retrieve an array of complex data as type \c double from the product file.
 * All complex array elements are stored consecutively in \a dst (for each element the real and imaginary values are 
 * stored next to each other).
 * The cursor must point to data with a base type that has special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_pairs_array(const coda_cursor *cursor, double *dst,
                                                            coda_array_ordering array_ordering)
{
    coda_type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->backend != coda_backend_binary)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
        return -1;
    }
    return read_array(cursor, (read_function)&coda_bin_cursor_read_double_pair, (uint8_t *)dst,
                      2 * sizeof(double), array_ordering);
}

/** Retrieve complex data as type \c double from the product file.
 * The real and imaginary values are stored in \a dst_re and \a dst_im.
 * The cursor must point to data with special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst_re Pointer to the variable where the real value that was read from the product will be stored.
 * \param dst_im Pointer to the variable where the imaginary value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_split(const coda_cursor *cursor, double *dst_re, double *dst_im)
{
    double dst[2];

    if (coda_cursor_read_complex_double_pair(cursor, dst) != 0)
    {
        return -1;
    }
    *dst_re = dst[0];
    *dst_im = dst[1];

    return 0;
}

/** Retrieve an array of complex data as type \c double from the product file.
 * All real array elements are stored in \a dst_re and all imaginary array elements in \a dsr_im.
 * The cursor must point to data with a base type that has special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst_re Pointer to the variable where the real values read from the product will be stored.
 * \param dst_im Pointer to the variable where the imaginary values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst_im and \a dst_re: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_split_array(const coda_cursor *cursor, double *dst_re, double *dst_im,
                                                            coda_array_ordering array_ordering)
{
    coda_type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_re == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_re argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_im == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_im argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    type = coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type);
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(type->type_class), __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->backend != coda_backend_binary)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
        return -1;
    }
    return read_split_array(cursor, (read_function)&coda_bin_cursor_read_double_pair, (uint8_t *)dst_re,
                            (uint8_t *)dst_im, sizeof(double), array_ordering);
}

/** @} */
