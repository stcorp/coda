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

#ifndef CODA_NETCDF_INTERNAL_H
#define CODA_NETCDF_INTERNAL_H

#include "coda-netcdf.h"

typedef enum netcdf_type_tag_enum
{
    tag_netcdf_root,    /* coda_record_class */
    tag_netcdf_array,   /* coda_array_class */
    tag_netcdf_basic_type,      /* coda_integer_class, coda_real_class, coda_text_class */
    tag_netcdf_attribute_record /* coda_record_class */
} netcdf_type_tag;

typedef struct coda_netcdfType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    netcdf_type_tag tag;
} coda_netcdfType;

typedef struct coda_netcdfRoot_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    netcdf_type_tag tag;

    int num_variables;
    coda_netcdfType **variable;
    char **variable_name;
    char **variable_real_name;
    hashtable *hash_data;

    struct coda_netcdfAttributeRecord_struct *attributes;
} coda_netcdfRoot;

typedef struct coda_netcdfArray_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    netcdf_type_tag tag;

    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];
    long num_elements;
    struct coda_netcdfBasicType_struct *base_type;
} coda_netcdfArray;

typedef struct coda_netcdfBasicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    netcdf_type_tag tag;
    coda_native_type read_type;
    int64_t offset;
    int record_var;
    int byte_size;
    int has_conversion;
    double add_offset;
    double scale_factor;
    int has_fill_value;
    double fill_value;

    struct coda_netcdfAttributeRecord_struct *attributes;
} coda_netcdfBasicType;

typedef struct coda_netcdfAttributeRecord_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    netcdf_type_tag tag;

    int num_attributes;
    coda_netcdfType **attribute;
    char **attribute_name;
    char **attribute_real_name;
    hashtable *hash_data;
} coda_netcdfAttributeRecord;

typedef struct coda_netcdfProductFile_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_DynamicType *root_type;
    coda_ProductDefinition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;

    int use_mmap;       /* indicates whether the file was opened using mmap */
    int fd;     /* file handle when not using mmap */
#ifdef WIN32
    HANDLE file;
    HANDLE file_mapping;
#endif
    const uint8_t *mmap_ptr;

    int netcdf_version;
    long record_size;
} coda_netcdfProductFile;

coda_netcdfRoot *coda_netcdf_root_new(void);
int coda_netcdf_root_add_variable(coda_netcdfRoot *root, const char *name, coda_netcdfType *variable);
int coda_netcdf_root_add_attributes(coda_netcdfRoot *root, coda_netcdfAttributeRecord *attributes);

coda_netcdfArray *coda_netcdf_array_new(int num_dims, long dim[CODA_MAX_NUM_DIMS], coda_netcdfBasicType *base_type);

coda_netcdfBasicType *coda_netcdf_basic_type_new(int nc_type, int64_t offset, int record_var, int length);
int coda_netcdf_basic_type_add_attributes(coda_netcdfBasicType *type, coda_netcdfAttributeRecord *attributes);

coda_netcdfAttributeRecord *coda_netcdf_attribute_record_new(void);
int coda_netcdf_attribute_record_add_attribute(coda_netcdfAttributeRecord *type, const char *name,
                                               coda_netcdfType *attribute);

coda_netcdfAttributeRecord *coda_netcdf_empty_attribute_record();

#endif
