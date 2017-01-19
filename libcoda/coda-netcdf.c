/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

#include "coda-netcdf-internal.h"
#include "coda-mem-internal.h"
#include "coda-bin-internal.h"
#include "coda-read-bytes.h"
#ifndef WORDS_BIGENDIAN
#include "coda-swap2.h"
#include "coda-swap4.h"
#include "coda-swap8.h"
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int read_dim_array(coda_netcdf_product *product, int64_t *offset, int32_t num_records, int32_t *num_dims,
                          int32_t **dim_length, int *appendable_dim)
{
    int32_t tag;
    long i;

    if (read_bytes(product->raw_product, *offset, 4, &tag) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&tag);
#endif
    *offset += 4;

    if (read_bytes(product->raw_product, *offset, 4, num_dims) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(num_dims);
#endif
    *offset += 4;

    if (tag == 0)
    {
        if (*num_dims != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty dim_array)");
            return -1;
        }
        return 0;
    }

    if (tag != 10)      /* NC_DIMENSION */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_DIMENSION tag)");
        return -1;
    }

    *dim_length = malloc((*num_dims) * sizeof(int32_t));
    if (*dim_length == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (*num_dims) * sizeof(int32_t), __FILE__, __LINE__);
        return -1;
    }

    for (i = 0; i < *num_dims; i++)
    {
        int32_t string_length;

        /* nelems */
        if (read_bytes(product->raw_product, *offset, 4, &string_length) < 0)
        {
            free(*dim_length);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&string_length);
#endif
        *offset += 4;
        /* skip chars + padding */
        *offset += string_length;
        if ((string_length & 3) != 0)
        {
            *offset += 4 - (string_length & 3);
        }
        /* dim_length */
        if (read_bytes(product->raw_product, *offset, 4, &(*dim_length)[i]) < 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&(*dim_length)[i]);
#endif
        *offset += 4;
        if ((*dim_length)[i] == 0)
        {
            /* appendable dimension */
            (*dim_length)[i] = num_records;
            *appendable_dim = i;
        }
    }

    return 0;
}

static int read_att_array(coda_netcdf_product *product, int64_t *offset, coda_mem_record **attributes,
                          coda_conversion *conversion)
{
    coda_type_record *attributes_definition;
    int32_t tag;
    int32_t num_att;
    long i;

    if (read_bytes(product->raw_product, *offset, 4, &tag) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&tag);
#endif
    *offset += 4;

    if (read_bytes(product->raw_product, *offset, 4, &num_att) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&num_att);
#endif
    *offset += 4;

    if (tag == 0)
    {
        *attributes = NULL;
        if (num_att != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty att_array)");
            return -1;
        }
        return 0;
    }

    if (tag != 12)      /* NC_ATTRIBUTE */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_ATTRIBUTE tag)");
        return -1;
    }

    attributes_definition = coda_type_record_new(coda_format_netcdf);
    if (attributes_definition == NULL)
    {
        return -1;
    }
    *attributes = coda_mem_record_new(attributes_definition, NULL);
    coda_type_release((coda_type *)attributes_definition);
    if (*attributes == NULL)
    {
        return -1;
    }

    for (i = 0; i < num_att; i++)
    {
        coda_netcdf_basic_type *basic_type;
        int32_t string_length;
        int32_t nc_type;
        int32_t nelems;
        long value_length;
        char *name;

        /* nelems */
        if (read_bytes(product->raw_product, *offset, 4, &string_length) < 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&string_length);
#endif
        *offset += 4;
        /* chars */
        name = malloc(string_length + 1);
        if (name == NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(string_length + 1), __FILE__, __LINE__);
            return -1;
        }
        name[string_length] = '\0';
        if (read_bytes(product->raw_product, *offset, string_length, name) < 0)
        {
            free(name);
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            return -1;
        }
        *offset += string_length;
        /* chars padding */
        if ((string_length & 3) != 0)
        {
            *offset += 4 - (string_length & 3);
        }
        /* nc_type */
        if (read_bytes(product->raw_product, *offset, 4, &nc_type) < 0)
        {
            free(name);
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&nc_type);
#endif
        *offset += 4;
        /* nelems */
        if (read_bytes(product->raw_product, *offset, 4, &nelems) < 0)
        {
            free(name);
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&nelems);
#endif
        *offset += 4;
        value_length = nelems;
        switch (nc_type)
        {
            case 1:
            case 2:
                break;
            case 3:
                value_length *= 2;
                break;
            case 4:
            case 5:
                value_length *= 4;
                break;
            case 6:
                value_length *= 8;
                break;
            default:
                free(name);
                coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid netcdf type (%d))", (int)nc_type);
                return -1;
        }

        if (conversion != NULL)
        {
            if (nelems == 1 && (strcmp(name, "scale_factor") == 0 || strcmp(name, "add_offset") == 0))
            {
                if (nc_type == 5)
                {
                    float value;

                    if (read_bytes(product->raw_product, *offset, 4, &value) < 0)
                    {
                        free(name);
                        coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                        return -1;
                    }
#ifndef WORDS_BIGENDIAN
                    swap_float(&value);
#endif
                    if (name[0] == 's')
                    {
                        conversion->numerator = value;
                    }
                    else
                    {
                        conversion->add_offset = value;
                    }
                }
                else if (nc_type == 6)
                {
                    double value;

                    if (read_bytes(product->raw_product, *offset, 8, &value) < 0)
                    {
                        free(name);
                        coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                        return -1;
                    }
#ifndef WORDS_BIGENDIAN
                    swap_double(&value);
#endif
                    if (name[0] == 's')
                    {
                        conversion->numerator = value;
                    }
                    else
                    {
                        conversion->add_offset = value;
                    }
                }
            }
            /* note that missing_value has preference over _FillValue */
            /* We therefore don't modify fill_value for _FillValue if fill_value was already set */
            if (nelems == 1 && nc_type != 2 && (strcmp(name, "missing_value") == 0 ||
                                                (strcmp(name, "_FillValue") == 0 &&
                                                 coda_isNaN(conversion->invalid_value))))
            {
                union
                {
                    int8_t as_int8[8];
                    int16_t as_int16[4];
                    int32_t as_int32[2];
                    int64_t as_int64[1];
                    float as_float[2];
                    double as_double[1];
                } value;

                if (read_bytes(product->raw_product, *offset, value_length, value.as_int8) < 0)
                {
                    free(name);
                    coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                    return -1;
                }
                switch (nc_type)
                {
                    case 1:
                        conversion->invalid_value = (double)value.as_int8[0];
                        break;
                    case 3:
#ifndef WORDS_BIGENDIAN
                        swap_int16(value.as_int16);
#endif
                        conversion->invalid_value = (double)value.as_int16[0];
                        break;
                    case 4:
#ifndef WORDS_BIGENDIAN
                        swap_int32(value.as_int32);
#endif
                        conversion->invalid_value = (double)value.as_int32[0];
                        break;
                    case 5:
#ifndef WORDS_BIGENDIAN
                        swap_float(value.as_float);
#endif
                        conversion->invalid_value = (double)value.as_float[0];
                        break;
                    case 6:
#ifndef WORDS_BIGENDIAN
                        swap_double(value.as_double);
#endif
                        conversion->invalid_value = value.as_double[0];
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
            }
        }

        if (nc_type == 2)       /* treat char arrays as strings */
        {
            basic_type = coda_netcdf_basic_type_new(nc_type, *offset, 0, nelems);
        }
        else
        {
            basic_type = coda_netcdf_basic_type_new(nc_type, *offset, 0, 1);
        }
        if (basic_type == NULL)
        {
            free(name);
            coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
            return -1;
        }
        *offset += value_length;
        if ((value_length & 3) != 0)
        {
            *offset += 4 - (value_length & 3);
        }

        if (nc_type == 2 || nelems == 1)
        {
            if (coda_mem_record_add_field(*attributes, name, (coda_dynamic_type *)basic_type, 1) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)basic_type);
                free(name);
                coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                return -1;
            }
        }
        else
        {
            coda_netcdf_array *array;
            long size = nelems;

            array = coda_netcdf_array_new(1, &size, basic_type);
            if (array == NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)basic_type);
                free(name);
                coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                return -1;
            }
            if (coda_mem_record_add_field(*attributes, name, (coda_dynamic_type *)array, 1) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                free(name);
                coda_dynamic_type_delete((coda_dynamic_type *)*attributes);
                return -1;
            }
        }
        free(name);
    }

    return 0;
}

static int read_var_array(coda_netcdf_product *product, int64_t *offset, int32_t num_dim_lengths, int32_t *dim_length,
                          int appendable_dim, coda_mem_record *root)
{
    int32_t tag;
    int32_t num_var;
    long i;

    if (read_bytes(product->raw_product, *offset, 4, &tag) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&tag);
#endif
    *offset += 4;

    if (read_bytes(product->raw_product, *offset, 4, &num_var) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&num_var);
#endif
    *offset += 4;

    if (tag == 0)
    {
        if (num_var != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty var_array)");
            return -1;
        }
        return 0;
    }

    if (tag != 11)      /* NC_VARIABLE */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_VARIABLE tag)");
        return -1;
    }

    for (i = 0; i < num_var; i++)
    {
        coda_netcdf_basic_type *basic_type;
        coda_mem_record *attributes = NULL;
        coda_conversion *conversion;
        long dim[CODA_MAX_NUM_DIMS];
        int64_t var_offset;
        int32_t string_length;
        int32_t nc_type;
        int32_t nelems;
        int32_t vsize;
        int32_t dim_id;
        char *name;
        long last_dim_length = 0;
        int last_dim_set = 0;
        int record_var = 0;
        int num_dims;
        long j;

        /* nelems */
        if (read_bytes(product->raw_product, *offset, 4, &string_length) < 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&string_length);
#endif
        *offset += 4;
        /* chars */
        name = malloc(string_length + 1);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(string_length + 1), __FILE__, __LINE__);
            return -1;
        }
        name[string_length] = '\0';
        if (read_bytes(product->raw_product, *offset, string_length, name) < 0)
        {
            free(name);
            return -1;
        }
        *offset += string_length;
        /* chars padding */
        if ((string_length & 3) != 0)
        {
            *offset += 4 - (string_length & 3);
        }
        /* nelems */
        if (read_bytes(product->raw_product, *offset, 4, &nelems) < 0)
        {
            free(name);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&nelems);
#endif
        *offset += 4;
        num_dims = 0;
        for (j = 0; j < nelems; j++)
        {
            /* dimid */
            if (read_bytes(product->raw_product, *offset, 4, &dim_id) < 0)
            {
                free(name);
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap_int32(&dim_id);
#endif
            *offset += 4;
            if (dim_id < 0 || dim_id >= num_dim_lengths)
            {
                free(name);
                coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid dimid for variable %s)", name);
                return -1;
            }
            if (j == nelems - 1)
            {
                last_dim_length = dim_length[dim_id];
                last_dim_set = 1;
            }
            else
            {
                if (j < CODA_MAX_NUM_DIMS)
                {
                    dim[j] = dim_length[dim_id];
                    num_dims++;
                }
                else
                {
                    dim[CODA_MAX_NUM_DIMS - 1] *= dim_length[dim_id];
                }
            }
            if (j == 0)
            {
                record_var = (dim_id == appendable_dim);
            }
        }

        conversion = coda_conversion_new(1.0, 1.0, 0.0, coda_NaN());
        if (conversion == NULL)
        {
            free(name);
            return -1;
        }
        /* vatt_array */
        if (read_att_array(product, offset, &attributes, conversion) != 0)
        {
            coda_conversion_delete(conversion);
            free(name);
            return -1;
        }

        /* nc_type */
        if (read_bytes(product->raw_product, *offset, 4, &nc_type) < 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            coda_conversion_delete(conversion);
            free(name);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&nc_type);
#endif
        *offset += 4;

        /* check if we need to use a conversion */
        if (conversion->numerator == 1.0 && conversion->add_offset == 0.0)
        {
            /* don't create conversions for non-floating type data if we only have an 'invalid_value' attribute */
            if ((nc_type != 5 && nc_type != 6) || coda_isNaN(conversion->invalid_value))
            {
                coda_conversion_delete(conversion);
                conversion = NULL;
            }
        }

        /* vsize */
        if (read_bytes(product->raw_product, *offset, 4, &vsize) < 0)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            coda_conversion_delete(conversion);
            free(name);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&vsize);
#endif
        *offset += 4;
        if (record_var)
        {
            product->record_size += vsize;
        }

        /* offset */
        if (product->netcdf_version == 1)
        {
            int32_t offset32;

            if (read_bytes(product->raw_product, *offset, 4, &offset32) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                coda_conversion_delete(conversion);
                free(name);
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap_int32(&offset32);
#endif
            var_offset = (int64_t)offset32;
            *offset += 4;
        }
        else
        {
            if (read_bytes(product->raw_product, *offset, 8, &var_offset) < 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                coda_conversion_delete(conversion);
                free(name);
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap_int64(&var_offset);
#endif
            *offset += 8;
        }
        if (last_dim_set)
        {
            if (nc_type == 2 && !(num_dims == 0 && record_var))
            {
                /* we treat the last dimension of a char array as a string */
                /* except if it is a one dimensional char array were the first dimension is the appendable dimension */
                basic_type = coda_netcdf_basic_type_new(nc_type, var_offset, record_var, last_dim_length);
            }
            else
            {
                basic_type = coda_netcdf_basic_type_new(nc_type, var_offset, record_var, 1);
                if (num_dims < CODA_MAX_NUM_DIMS)
                {
                    dim[num_dims] = last_dim_length;
                    num_dims++;
                }
                else
                {
                    dim[CODA_MAX_NUM_DIMS - 1] *= last_dim_length;
                }
            }
        }
        else
        {
            /* we only get here if the variable is a real scalar (num_dims == 0) */
            basic_type = coda_netcdf_basic_type_new(nc_type, var_offset, 0, 1);
        }
        if (basic_type == NULL)
        {
            coda_dynamic_type_delete((coda_dynamic_type *)attributes);
            coda_conversion_delete(conversion);
            free(name);
            return -1;
        }
        if (conversion != NULL)
        {
            if (coda_netcdf_basic_type_set_conversion(basic_type, conversion) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                coda_conversion_delete(conversion);
                free(name);
                return -1;
            }
        }
        if (num_dims == 0)
        {
            if (attributes != NULL)
            {
                if (coda_netcdf_basic_type_set_attributes(basic_type, attributes) != 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)basic_type);
                    coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                    free(name);
                    return -1;
                }
            }
            if (coda_mem_record_add_field(root, name, (coda_dynamic_type *)basic_type, 1) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)basic_type);
                free(name);
                return -1;
            }
        }
        else
        {
            coda_netcdf_array *array;

            array = coda_netcdf_array_new(num_dims, dim, basic_type);
            if (array == NULL)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)basic_type);
                coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                free(name);
                return -1;
            }
            if (attributes != NULL)
            {
                if (coda_netcdf_array_set_attributes(array, attributes) != 0)
                {
                    coda_dynamic_type_delete((coda_dynamic_type *)array);
                    coda_dynamic_type_delete((coda_dynamic_type *)attributes);
                    free(name);
                    return -1;
                }
            }
            if (coda_mem_record_add_field(root, name, (coda_dynamic_type *)array, 1) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                free(name);
                return -1;
            }
        }
        free(name);
    }

    return 0;
}

int coda_netcdf_reopen(coda_product **product)
{
    coda_netcdf_product *product_file;
    coda_type_record *root_definition;
    coda_mem_record *attributes;
    coda_mem_record *root;
    char magic[4];
    int32_t num_records;
    int32_t num_dims;
    int32_t *dim_length = NULL;
    int64_t offset = 0;
    int appendable_dim = -1;

    product_file = (coda_netcdf_product *)malloc(sizeof(coda_netcdf_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_netcdf_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_netcdf;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;

    product_file->raw_product = *product;
    product_file->netcdf_version = 1;
    product_file->record_size = 0;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }

    /* create root type */
    root_definition = coda_type_record_new(coda_format_netcdf);
    if (root_definition == NULL)
    {
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
    root = coda_mem_record_new(root_definition, NULL);
    coda_type_release((coda_type *)root_definition);
    if (root == NULL)
    {
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
    product_file->root_type = (coda_dynamic_type *)root;

    /* magic */
    if (read_bytes(product_file->raw_product, offset, 4, magic) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
    assert(magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F');
    product_file->netcdf_version = magic[3];
    if (product_file->netcdf_version != 1 && product_file->netcdf_version != 2)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "not a supported format version (%d) of the netCDF format",
                       product_file->netcdf_version);
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
    offset += 4;

    /* numrecs */
    if (read_bytes(product_file->raw_product, offset, 4, &num_records) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file (%s)", strerror(errno));
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&num_records);
#endif
    offset += 4;

    /* dim_array */
    if (read_dim_array(product_file, &offset, num_records, &num_dims, &dim_length, &appendable_dim) != 0)
    {
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }

    /* gatt_array */
    if (read_att_array(product_file, &offset, &attributes, NULL) != 0)
    {
        if (dim_length != NULL)
        {
            free(dim_length);
        }
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }
    if (attributes != NULL)
    {
        if (coda_mem_type_set_attributes((coda_mem_type *)root, (coda_dynamic_type *)attributes, 1) != 0)
        {
            if (dim_length != NULL)
            {
                free(dim_length);
            }
            coda_netcdf_close((coda_product *)product_file);
            return -1;
        }
    }

    /* var_array */
    if (read_var_array(product_file, &offset, num_dims, dim_length, appendable_dim, root) != 0)
    {
        if (dim_length != NULL)
        {
            free(dim_length);
        }
        coda_netcdf_close((coda_product *)product_file);
        return -1;
    }

    if (dim_length != NULL)
    {
        free(dim_length);
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_netcdf_close(coda_product *product)
{
    coda_netcdf_product *product_file = (coda_netcdf_product *)product;

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
