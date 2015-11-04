/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#ifndef CODA_GRIB_INTERNAL_H
#define CODA_GRIB_INTERNAL_H

#include "coda-grib.h"

typedef enum grib_type_tag_enum
{
    tag_grib_record,    /* coda_record_class */
    tag_grib_array,     /* coda_array_class */
    tag_grib_integer,   /* coda_integer_class */
    tag_grib_real,      /* coda_real_class */
    tag_grib_text,      /* coda_text_class */
    tag_grib_raw,       /* coda_raw_class */
    tag_grib_value_array,       /* coda_array_class */
    tag_grib_value      /* coda_real_class */
} grib_type_tag;


typedef struct coda_grib_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;
    coda_native_type read_type;
    int64_t bit_size;
/*  coda_expression *bit_size_expr;
    coda_record *attributes; */
} coda_grib_type;

typedef struct coda_grib_record_field_struct
{
    char *name;
    char *real_name;
    coda_grib_type *type;
    int hidden;
    uint8_t optional;
    coda_expression *available_expr;
} coda_grib_record_field;

typedef struct coda_grib_record_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;
    coda_native_type read_type;
    int64_t bit_size;

    hashtable *hash_data;
    long num_fields;
    coda_grib_record_field **field;
    int has_hidden_fields;
    int has_available_expr_fields;
} coda_grib_record;

typedef struct coda_grib_array_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;
    coda_native_type read_type;
    int64_t bit_size;

    coda_grib_type *base_type;
    long num_elements;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];        /* -1 means it's variable and the value needs to be retrieved from dim_expr */
    coda_expression *dim_expr[CODA_MAX_NUM_DIMS];
} coda_grib_array;

typedef struct coda_grib_basic_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;
    coda_native_type read_type;
    int64_t bit_size;
} coda_grib_basic_type;


typedef struct coda_grib_dynamic_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_type *definition;
} coda_grib_dynamic_type;

typedef struct coda_grib_dynamic_record_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_record *definition;

    coda_grib_dynamic_type **field_type;        /* if field_type[i] == NULL then field #i is not available */
} coda_grib_dynamic_record;

typedef struct coda_grib_dynamic_array_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_array *definition;

    long num_elements;
    coda_grib_dynamic_type **element_type;
} coda_grib_dynamic_array;

typedef struct coda_grib_dynamic_integer_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_basic_type *definition;

    int64_t value;
} coda_grib_dynamic_integer;

typedef struct coda_grib_dynamic_real_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_basic_type *definition;

    double value;
} coda_grib_dynamic_real;

typedef struct coda_grib_dynamic_text_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_basic_type *definition;

    char *text;
} coda_grib_dynamic_text;

typedef struct coda_grib_dynamic_raw_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_basic_type *definition;

    long length;
    uint8_t *data;
} coda_grib_dynamic_raw;

typedef struct coda_grib_dynamic_value_array_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    grib_type_tag tag;
    coda_grib_array *definition;

    long num_elements;
    coda_grib_dynamic_type *base_type;
    int64_t bit_offset;
    int element_bit_size;
    int16_t decimalScaleFactor;
    int16_t binaryScaleFactor;
    float referenceValue;
} coda_grib_dynamic_value_array;


typedef struct coda_grib_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;

    int use_mmap;       /* indicates whether the file was opened using mmap */
    int fd;     /* file handle when not using mmap */
#ifdef WIN32
    HANDLE file;
    HANDLE file_mapping;
#endif
    const uint8_t *mmap_ptr;

    int grib_version;
    long record_size;
} coda_grib_product;

/* static type interface (= structure definition) */
coda_grib_record_field *coda_grib_record_field_new(const char *name);
int coda_grib_record_field_set_type(coda_grib_record_field *field, coda_grib_type *type);
int coda_grib_record_field_set_hidden(coda_grib_record_field *field);
int coda_grib_record_field_set_optional(coda_grib_record_field *field);
int coda_grib_record_field_validate(coda_grib_record_field *field);

coda_grib_record *coda_grib_record_new(void);
coda_grib_record *coda_grib_empty_record(void);
int coda_grib_record_add_field(coda_grib_record *type, coda_grib_record_field *field);

coda_grib_array *coda_grib_array_new(void);
int coda_grib_array_set_base_type(coda_grib_array *type, coda_grib_type *base_type);
int coda_grib_array_add_fixed_dimension(coda_grib_array *type, long dim);
int coda_grib_array_add_variable_dimension(coda_grib_array *type, coda_expression *dim_expr);
int coda_grib_array_validate(coda_grib_array *type);

coda_grib_basic_type *coda_grib_basic_type_new(coda_type_class type_class);
int coda_grib_basic_type_set_bit_size(coda_grib_basic_type *type, int64_t bit_size);
int coda_grib_basic_type_set_read_type(coda_grib_basic_type *type, coda_native_type read_type);
int coda_grib_basic_type_validate(coda_grib_basic_type *type);

/* dynamic type interface (= actual contents of product) */
void coda_grib_release_dynamic_type(coda_grib_dynamic_type *type);

coda_grib_dynamic_record *coda_grib_dynamic_record_new(coda_grib_record *definition);
int coda_grib_dynamic_record_set_field(coda_grib_dynamic_record *type, const char *name,
                                       coda_grib_dynamic_type *field_type);
int coda_grib_dynamic_record_validate(coda_grib_dynamic_record *type);

coda_grib_dynamic_array *coda_grib_dynamic_array_new(coda_grib_array *definition);
int coda_grib_dynamic_array_add_element(coda_grib_dynamic_array *type, coda_grib_dynamic_type *element);
int coda_grib_dynamic_array_validate(coda_grib_dynamic_array *type);

coda_grib_dynamic_integer *coda_grib_dynamic_integer_new(coda_grib_basic_type *definition, int64_t value);
coda_grib_dynamic_real *coda_grib_dynamic_real_new(coda_grib_basic_type *definition, double value);
coda_grib_dynamic_text *coda_grib_dynamic_text_new(coda_grib_basic_type *definition, const char *text);
coda_grib_dynamic_raw *coda_grib_dynamic_raw_new(coda_grib_basic_type *definition, long length, const uint8_t *data);

coda_grib_dynamic_value_array *coda_grib_dynamic_value_array_new(coda_grib_array *definition, int num_elements,
                                                                 int64_t byte_offset, int element_bit_size,
                                                                 int16_t decimalScaleFactor, int16_t binaryScaleFactor,
                                                                 float referenceValue);

coda_grib_dynamic_record *coda_grib_empty_dynamic_record();

#endif
