/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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

#include "coda-netcdf-internal.h"

#ifndef WORDS_BIGENDIAN
static void swap2(void *value)
{
    union
    {
        uint8_t as_bytes[2];
        int16_t as_int16;
    } v1, v2;

    v1.as_int16 = *(int16_t *)value;

    v2.as_bytes[0] = v1.as_bytes[1];
    v2.as_bytes[1] = v1.as_bytes[0];

    *(int16_t *)value = v2.as_int16;
}

static void swap4(void *value)
{
    union
    {
        uint8_t as_bytes[4];
        int32_t as_int32;
    } v1, v2;

    v1.as_int32 = *(int32_t *)value;

    v2.as_bytes[0] = v1.as_bytes[3];
    v2.as_bytes[1] = v1.as_bytes[2];
    v2.as_bytes[2] = v1.as_bytes[1];
    v2.as_bytes[3] = v1.as_bytes[0];

    *(int32_t *)value = v2.as_int32;
}

static void swap8(void *value)
{
    union
    {
        uint8_t as_bytes[8];
        int64_t as_int64;
    } v1, v2;

    v1.as_int64 = *(int64_t *)value;

    v2.as_bytes[0] = v1.as_bytes[7];
    v2.as_bytes[1] = v1.as_bytes[6];
    v2.as_bytes[2] = v1.as_bytes[5];
    v2.as_bytes[3] = v1.as_bytes[4];
    v2.as_bytes[4] = v1.as_bytes[3];
    v2.as_bytes[5] = v1.as_bytes[2];
    v2.as_bytes[6] = v1.as_bytes[1];
    v2.as_bytes[7] = v1.as_bytes[0];

    *(int64_t *)value = v2.as_int64;
}
#endif

static coda_netcdfAttributeRecord *empty_attributes_singleton = NULL;

coda_netcdfAttributeRecord *coda_netcdf_empty_attribute_record()
{
    if (empty_attributes_singleton == NULL)
    {
        empty_attributes_singleton = coda_netcdf_attribute_record_new();
    }

    return empty_attributes_singleton;
}

void coda_netcdf_release_type(coda_netcdfType *T);

void coda_netcdf_done()
{
    if (empty_attributes_singleton != NULL)
    {
        coda_netcdf_release_type((coda_netcdfType *)empty_attributes_singleton);
        empty_attributes_singleton = NULL;
    }
}

static int read_dim_array(coda_netcdfProductFile *pf, int32_t num_records, int32_t *num_dims, int32_t **dim_length,
                          int *appendable_dim)
{
    int32_t tag;
    long i;

    if (read(pf->fd, &tag, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&tag);
#endif

    if (read(pf->fd, num_dims, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(num_dims);
#endif

    if (tag == 0)
    {
        if (*num_dims != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty dim_array) "
                           "for file %s", pf->filename);
            return -1;
        }
        return 0;
    }

    if (tag != 10)      /* NC_DIMENSION */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_DIMENSION tag) for file %s",
                       pf->filename);
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
        if (read(pf->fd, &string_length, 4) < 0)
        {
            free(*dim_length);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&string_length);
#endif
        /* skip chars + padding */
        if ((string_length & 3) != 0)
        {
            string_length += 4 - (string_length & 3);
        }
        if (lseek(pf->fd, string_length, SEEK_CUR) < 0)
        {
            free(*dim_length);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
        /* dim_length */
        if (read(pf->fd, &(*dim_length)[i], 4) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&(*dim_length)[i]);
#endif
        if ((*dim_length)[i] == 0)
        {
            /* appendable dimension */
            (*dim_length)[i] = num_records;
            *appendable_dim = i;
        }
    }

    return 0;
}

static int read_att_array(coda_netcdfProductFile *pf, coda_netcdfAttributeRecord **attributes, double *scale_factor,
                          double *add_offset, double *fill_value)
{
    int32_t tag;
    int32_t num_att;
    long i;

    if (read(pf->fd, &tag, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&tag);
#endif

    if (read(pf->fd, &num_att, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&num_att);
#endif

    if (tag == 0)
    {
        *attributes = NULL;
        if (num_att != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty att_array) "
                           "for file %s", pf->filename);
            return -1;
        }
        return 0;
    }

    if (tag != 12)      /* NC_ATTRIBUTE */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_ATTRIBUTE tag) for file %s",
                       pf->filename);
        return -1;
    }

    *attributes = coda_netcdf_attribute_record_new();
    if (*attributes == NULL)
    {
        return -1;
    }

    for (i = 0; i < num_att; i++)
    {
        coda_netcdfBasicType *basic_type;
        int32_t string_length;
        int64_t offset;
        int32_t nc_type;
        int32_t nelems;
        long value_length;
        char *name;

        /* nelems */
        if (read(pf->fd, &string_length, 4) < 0)
        {
            coda_netcdf_release_type((coda_netcdfType *)*attributes);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&string_length);
#endif
        /* chars */
        name = malloc(string_length + 1);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(string_length + 1), __FILE__, __LINE__);
            return -1;
        }
        name[string_length] = '\0';
        if (read(pf->fd, name, string_length) < 0)
        {
            free(name);
            coda_netcdf_release_type((coda_netcdfType *)*attributes);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
        /* chars padding */
        if ((string_length & 3) != 0)
        {
            if (lseek(pf->fd, 4 - (string_length & 3), SEEK_CUR) < 0)
            {
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)attributes);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
        }
        /* nc_type */
        if (read(pf->fd, &nc_type, 4) < 0)
        {
            free(name);
            coda_netcdf_release_type((coda_netcdfType *)*attributes);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&nc_type);
#endif
        /* nelems */
        if (read(pf->fd, &nelems, 4) < 0)
        {
            free(name);
            coda_netcdf_release_type((coda_netcdfType *)*attributes);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&nelems);
#endif
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
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid netcdf type (%d)) for file %s",
                               (int)nc_type, pf->filename);
                return -1;
        }
        if ((value_length & 3) != 0)
        {
            value_length += 4 - (value_length & 3);
        }
        offset = lseek(pf->fd, value_length, SEEK_CUR);
        if (offset < 0)
        {
            free(name);
            coda_netcdf_release_type((coda_netcdfType *)*attributes);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
        offset -= value_length;

        if (nelems == 1 && ((strcmp(name, "scale_factor") == 0 && scale_factor != NULL) ||
                            (strcmp(name, "add_offset") == 0 && add_offset != NULL)))
        {
            if (nc_type == 5)
            {
                float value;

                if (lseek(pf->fd, offset, SEEK_SET) < 0)
                {
                    free(name);
                    coda_netcdf_release_type((coda_netcdfType *)*attributes);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename,
                                   strerror(errno));
                    return -1;
                }
                if (read(pf->fd, &value, 4) < 0)
                {
                    free(name);
                    coda_netcdf_release_type((coda_netcdfType *)*attributes);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename,
                                   strerror(errno));
                    return -1;
                }
#ifndef WORDS_BIGENDIAN
                swap4(&value);
#endif
                if (name[0] == 's')
                {
                    *scale_factor = value;
                }
                else
                {
                    *add_offset = value;
                }
            }
            else if (nc_type == 6)
            {
                double value;

                if (lseek(pf->fd, offset, SEEK_SET) < 0)
                {
                    free(name);
                    coda_netcdf_release_type((coda_netcdfType *)*attributes);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename,
                                   strerror(errno));
                    return -1;
                }
                if (read(pf->fd, &value, 8) < 0)
                {
                    free(name);
                    coda_netcdf_release_type((coda_netcdfType *)*attributes);
                    coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename,
                                   strerror(errno));
                    return -1;
                }
#ifndef WORDS_BIGENDIAN
                swap8(&value);
#endif
                if (name[0] == 's')
                {
                    *scale_factor = value;
                }
                else
                {
                    *add_offset = value;
                }
            }
        }
        /* note that missing_value has preference over _FillValue */
        /* We therefore don't modify fill_value for _FillValue if fill_value was already set */
        if (nelems == 1 && nc_type != 2 && fill_value != NULL &&
            (strcmp(name, "missing_value") == 0 || (strcmp(name, "_FillValue") == 0 && coda_isNaN(*fill_value))))
        {
            uint8_t value[8];

            if (lseek(pf->fd, offset, SEEK_SET) < 0)
            {
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
            if (read(pf->fd, value, value_length) < 0)
            {
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
            switch (nc_type)
            {
                case 1:
                    *fill_value = (double)value[0];
                    break;
                case 3:
#ifndef WORDS_BIGENDIAN
                    swap2(value);
#endif
                    *fill_value = (double)(*(int16_t *)value);
                    break;
                case 4:
#ifndef WORDS_BIGENDIAN
                    swap4(value);
#endif
                    *fill_value = (double)(*(int32_t *)value);
                    break;
                case 5:
#ifndef WORDS_BIGENDIAN
                    swap4(value);
#endif
                    *fill_value = (double)(*(float *)value);
                    break;
                case 6:
#ifndef WORDS_BIGENDIAN
                    swap8(value);
#endif
                    *fill_value = *(double *)value;
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }

        if (nc_type == 2)       /* treat char arrays as strings */
        {
            basic_type = coda_netcdf_basic_type_new(nc_type, offset, 0, nelems);
        }
        else
        {
            basic_type = coda_netcdf_basic_type_new(nc_type, offset, 0, 1);
        }
        if (basic_type == NULL)
        {
            free(name);
            coda_netcdf_release_type((coda_netcdfType *)attributes);
            return -1;
        }

        if (nc_type == 2 || nelems == 1)
        {
            if (coda_netcdf_attribute_record_add_attribute(*attributes, name, (coda_netcdfType *)basic_type) != 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)basic_type);
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                return -1;
            }
        }
        else
        {
            coda_netcdfArray *array;
            long size = nelems;

            array = coda_netcdf_array_new(1, &size, basic_type);
            if (array == NULL)
            {
                coda_netcdf_release_type((coda_netcdfType *)basic_type);
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                return -1;
            }
            if (coda_netcdf_attribute_record_add_attribute(*attributes, name, (coda_netcdfType *)array) != 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)array);
                free(name);
                coda_netcdf_release_type((coda_netcdfType *)*attributes);
                return -1;
            }
        }

        free(name);
    }

    return 0;
}

static int read_var_array(coda_netcdfProductFile *pf, int32_t num_dim_lengths, int32_t *dim_length, int appendable_dim,
                          coda_netcdfRoot *root)
{
    int32_t tag;
    int32_t num_var;
    long i;

    if (read(pf->fd, &tag, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&tag);
#endif

    if (read(pf->fd, &num_var, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&num_var);
#endif

    if (tag == 0)
    {
        if (num_var != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for nelems for empty var_array) "
                           "for file %s", pf->filename);
            return -1;
        }
        return 0;
    }

    if (tag != 11)      /* NC_VARIABLE */
    {
        coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid value for NC_VARIABLE tag) for file %s",
                       pf->filename);
        return -1;
    }

    for (i = 0; i < num_var; i++)
    {
        coda_netcdfBasicType *basic_type;
        coda_netcdfAttributeRecord *attributes;
        long dim[CODA_MAX_NUM_DIMS];
        double scale_factor = 1.0;
        double add_offset = 0.0;
        double fill_value = coda_NaN();
        int32_t string_length;
        int64_t offset;
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
        if (read(pf->fd, &string_length, 4) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&string_length);
#endif
        /* chars */
        name = malloc(string_length + 1);
        if (name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)(string_length + 1), __FILE__, __LINE__);
            return -1;
        }
        name[string_length] = '\0';
        if (read(pf->fd, name, string_length) < 0)
        {
            free(name);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
        /* chars padding */
        if ((string_length & 3) != 0)
        {
            if (lseek(pf->fd, 4 - (string_length & 3), SEEK_CUR) < 0)
            {
                free(name);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
        }
        /* nelems */
        if (read(pf->fd, &nelems, 4) < 0)
        {
            free(name);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&nelems);
#endif
        num_dims = 0;
        for (j = 0; j < nelems; j++)
        {
            /* dimid */
            if (read(pf->fd, &dim_id, 4) < 0)
            {
                free(name);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap4(&dim_id);
#endif
            if (dim_id < 0 || dim_id >= num_dim_lengths)
            {
                free(name);
                coda_set_error(CODA_ERROR_PRODUCT, "invalid netCDF file (invalid dimid for variable %s) for file "
                               "%s", name, pf->filename);
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

        /* vatt_array */
        if (read_att_array(pf, &attributes, &scale_factor, &add_offset, &fill_value) != 0)
        {
            free(name);
            return -1;
        }

        /* nc_type */
        if (read(pf->fd, &nc_type, 4) < 0)
        {
            coda_netcdf_release_type((coda_netcdfType *)attributes);
            free(name);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&nc_type);
#endif
        /* vsize */
        if (read(pf->fd, &vsize, 4) < 0)
        {
            coda_netcdf_release_type((coda_netcdfType *)attributes);
            free(name);
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap4(&vsize);
#endif
        if (record_var)
        {
            pf->record_size += vsize;
        }

        /* offset */
        if (pf->netcdf_version == 1)
        {
            int32_t offset32;

            if (read(pf->fd, &offset32, 4) < 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)attributes);
                free(name);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap4(&offset32);
#endif
            offset = (int64_t)offset32;
        }
        else
        {
            if (read(pf->fd, &offset, 8) < 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)attributes);
                free(name);
                coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", pf->filename, strerror(errno));
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap8(&offset);
#endif
        }
        if (last_dim_set)
        {
            if (nc_type == 2 && !(num_dims == 0 && record_var))
            {
                /* we treat the last dimension of a char array as a string */
                /* except if it is a one dimensional char array were the first dimension is the appendable dimension */
                basic_type = coda_netcdf_basic_type_new(nc_type, offset, record_var, last_dim_length);
            }
            else
            {
                basic_type = coda_netcdf_basic_type_new(nc_type, offset, record_var, 1);
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
            basic_type = coda_netcdf_basic_type_new(nc_type, offset, 0, 1);
        }
        if (basic_type == NULL)
        {
            coda_netcdf_release_type((coda_netcdfType *)attributes);
            free(name);
            return -1;
        }
        if (scale_factor != 1.0 || add_offset != 0.0)
        {
            basic_type->has_conversion = 1;
            basic_type->scale_factor = scale_factor;
            basic_type->add_offset = add_offset;
        }
        if (!coda_isNaN(fill_value))
        {
            basic_type->has_fill_value = 1;
            basic_type->fill_value = fill_value;
        }
        if (attributes != NULL)
        {
            if (coda_netcdf_basic_type_add_attributes(basic_type, attributes) != 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)attributes);
                free(name);
                return -1;
            }
        }
        if (num_dims == 0)
        {
            if (coda_netcdf_root_add_variable(root, name, (coda_netcdfType *)basic_type) != 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)basic_type);
                free(name);
                return -1;
            }
        }
        else
        {
            coda_netcdfArray *array;

            array = coda_netcdf_array_new(num_dims, dim, basic_type);
            if (array == NULL)
            {
                coda_netcdf_release_type((coda_netcdfType *)basic_type);
                free(name);
                return -1;
            }
            if (coda_netcdf_root_add_variable(root, name, (coda_netcdfType *)array) != 0)
            {
                coda_netcdf_release_type((coda_netcdfType *)basic_type);
                free(name);
                return -1;
            }
        }

        free(name);
    }

    return 0;
}

int coda_netcdf_open(const char *filename, int64_t file_size, coda_ProductFile **pf)
{
    coda_netcdfProductFile *product_file;
    coda_netcdfAttributeRecord *attributes;
    coda_netcdfRoot *root;
    char magic[4];
    int open_flags;
    int32_t num_records;
    int32_t num_dims;
    int32_t *dim_length = NULL;
    int appendable_dim = -1;

    product_file = (coda_netcdfProductFile *)malloc(sizeof(coda_netcdfProductFile));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_netcdfProductFile), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_netcdf;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->use_mmap = 0;
    product_file->fd = -1;
    product_file->mmap_ptr = NULL;
#ifdef WIN32
    product_file->file_mapping = INVALID_HANDLE_VALUE;
    product_file->file = INVALID_HANDLE_VALUE;
#endif
    product_file->netcdf_version = 1;
    product_file->record_size = 0;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    product_file->fd = open(filename, open_flags);
    if (product_file->fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", filename, strerror(errno));
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    /* create root type */
    root = coda_netcdf_root_new();
    if (root == NULL)
    {
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }
    product_file->root_type = (coda_DynamicType *)root;

    /* magic */
    if (read(product_file->fd, magic, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                       strerror(errno));
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }
    assert(magic[0] == 'C' && magic[1] == 'D' && magic[2] == 'F');
    product_file->netcdf_version = magic[3];
    if (product_file->netcdf_version != 1 && product_file->netcdf_version != 2)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "not a supported format version (%d) of the netCDF format for "
                       "file %s", product_file->netcdf_version, product_file->filename);
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    /* numrecs */
    if (read(product_file->fd, &num_records, 4) < 0)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                       strerror(errno));
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap4(&num_records);
#endif

    /* dim_array */
    if (read_dim_array(product_file, num_records, &num_dims, &dim_length, &appendable_dim) != 0)
    {
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    /* gatt_array */
    if (read_att_array(product_file, &attributes, NULL, NULL, NULL) != 0)
    {
        if (dim_length != NULL)
        {
            free(dim_length);
        }
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    if (coda_netcdf_root_add_attributes(root, attributes) != 0)
    {
        if (dim_length != NULL)
        {
            free(dim_length);
        }
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    /* var_array */
    if (read_var_array(product_file, num_dims, dim_length, appendable_dim, root) != 0)
    {
        if (dim_length != NULL)
        {
            free(dim_length);
        }
        coda_netcdf_close((coda_ProductFile *)product_file);
        return -1;
    }

    if (dim_length != NULL)
    {
        free(dim_length);
    }

    if (coda_option_use_mmap)
    {
        /* Perform an mmap() of the file, filling the following fields:
         *   pf->use_mmap = 1
         *   pf->file         (windows only )
         *   pf->file_mapping (windows only )
         *   pf->mmap_ptr     (windows, *nix)
         */
        product_file->use_mmap = 1;
#ifdef WIN32
        close(product_file->fd);
        product_file->file = CreateFile(product_file->filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, NULL);
        if (product_file->file == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                coda_set_error(CODA_ERROR_FILE_NOT_FOUND, "could not find %s", product_file->filename);
            }
            else
            {
                LPVOID lpMsgBuf;

                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
                {
                    /* Set error without additional information */
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s", product_file->filename);
                }
                else
                {
                    coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename,
                                   (LPCTSTR) lpMsgBuf);
                    LocalFree(lpMsgBuf);
                }
            }
            coda_netcdf_close((coda_ProductFile *)product_file);
            return -1;  /* indicate failure */
        }

        /* Try to do file mapping */
        product_file->file_mapping = CreateFileMapping(product_file->file, NULL, PAGE_READONLY, 0,
                                                       (int32_t)product_file->file_size, 0);
        if (product_file->file_mapping == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product_file->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_netcdf_close((coda_ProductFile *)product_file);
            return -1;
        }

        product_file->mmap_ptr = (uint8_t *)MapViewOfFile(product_file->file_mapping, FILE_MAP_READ, 0, 0, 0);
        if (product_file->mmap_ptr == NULL)
        {
            LPVOID lpMsgBuf;

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
            {
                /* Set error without additional information */
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory", product_file->filename);
            }
            else
            {
                coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                               (LPCTSTR) lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
            coda_netcdf_close((coda_ProductFile *)product_file);
            return -1;
        }
#else
        product_file->mmap_ptr = (uint8_t *)mmap(0, product_file->file_size, PROT_READ, MAP_SHARED, product_file->fd,
                                                 0);
        if (product_file->mmap_ptr == (uint8_t *)MAP_FAILED)
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not map file %s into memory (%s)", product_file->filename,
                           strerror(errno));
            product_file->mmap_ptr = NULL;
            close(product_file->fd);
            coda_netcdf_close((coda_ProductFile *)product_file);
            return -1;
        }

        /* close file descriptor (the file handle is not needed anymore) */
        close(product_file->fd);
#endif
    }

    *pf = (coda_ProductFile *)product_file;

    return 0;
}

int coda_netcdf_close(coda_ProductFile *pf)
{
    coda_netcdfProductFile *product_file = (coda_netcdfProductFile *)pf;

    if (product_file->use_mmap)
    {
#ifdef WIN32
        if (product_file->mmap_ptr != NULL)
        {
            UnmapViewOfFile(product_file->mmap_ptr);
        }
        if (product_file->file_mapping != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product_file->file_mapping);
        }
        if (product_file->file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(product_file->file);
        }
#else
        if (product_file->mmap_ptr != NULL)
        {
            munmap((void *)product_file->mmap_ptr, product_file->file_size);
        }
#endif
    }
    else
    {
        if (product_file->fd >= 0)
        {
            close(product_file->fd);
        }
    }

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }

    free(product_file);

    return 0;
}

int coda_netcdf_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type)
{
    *type = (coda_Type *)dynamic_type;
    return 0;
}
