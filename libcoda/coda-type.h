/*
 * Copyright (C) 2007-2024 S[&]T, The Netherlands.
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

#ifndef CODA_TYPE_H
#define CODA_TYPE_H

#include "coda-internal.h"

/* the maximum string length that can be used to represent an integer or floating point number */
/* note that this includes strings with time information (which also map to floating point numbers) */
#define MAX_ASCII_NUMBER_LENGTH 64

enum coda_ascii_special_text_type_enum
{
    ascii_text_default,
    ascii_text_line_separator,
    ascii_text_line_with_eol,
    ascii_text_line_without_eol,
    ascii_text_whitespace
};
typedef enum coda_ascii_special_text_type_enum coda_ascii_special_text_type;

typedef struct coda_conversion
{
    /* value = (value * numerator) / denominator + add_offset */
    double numerator;
    double denominator;
    double add_offset;
    /* if (value == invalid_value) { value = coda_NaN(); } (will happen before applying scaling/offset) */
    double invalid_value;
    char *unit;
} coda_conversion;

struct coda_ascii_mapping_struct
{
    int length;
    char *str;
};
typedef struct coda_ascii_mapping_struct coda_ascii_mapping;

typedef struct coda_ascii_integer_mapping_struct
{
    int length;
    char *str;
    int64_t value;
} coda_ascii_integer_mapping;

typedef struct coda_ascii_float_mapping_struct
{
    int length;
    char *str;
    double value;
} coda_ascii_float_mapping;

struct coda_ascii_mappings_struct
{
    int64_t default_bit_size;   /* bit_size if none of the mappings apply */
    int num_mappings;
    coda_ascii_mapping **mapping;
};
typedef struct coda_ascii_mappings_struct coda_ascii_mappings;

typedef struct coda_type_record_struct coda_type_record;

struct coda_type_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;   /* -1: dynamic calculated; -8: treat size_expr as byte_size_expr instead of bit_size_expr */
    coda_expression *size_expr;
    coda_type_record *attributes;
};

/* NOTE: record fields are not coda_type types! so don't cast them to coda_type. */
typedef struct coda_type_record_field_struct
{
    char *name;
    char *real_name;
    coda_type *type;
    int hidden;
    uint8_t optional;
    coda_expression *available_expr;
    int64_t bit_offset;
    coda_expression *bit_offset_expr;
} coda_type_record_field;

struct coda_type_record_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;
    coda_expression *size_expr;
    coda_type_record *attributes;

    hashtable *hash_data;
    hashtable *real_name_hash_data;
    long num_fields;
    coda_type_record_field **field;
    int has_hidden_fields;
    int has_optional_fields;
    int is_union;
    coda_expression *union_field_expr;  /* returns index in range [0..num_fields) if record is a union */
};

typedef struct coda_type_array_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;
    coda_expression *size_expr;
    coda_type_record *attributes;

    coda_type *base_type;
    long num_elements;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];        /* -1 means it's variable and the value needs to be retrieved from dim_expr */
    coda_expression *dim_expr[CODA_MAX_NUM_DIMS];
} coda_type_array;

typedef struct coda_type_number_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;   /* anywhere from 1 to 64 bits. -1 means it's variable -> use bit_size_expr */
    coda_expression *size_expr;
    coda_type_record *attributes;

    char *unit;
    coda_endianness endianness;
    coda_conversion *conversion;
    coda_ascii_mappings *mappings;
} coda_type_number;

typedef struct coda_type_text_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;
    coda_expression *size_expr;
    coda_type_record *attributes;

    char *fixed_value;
    coda_ascii_special_text_type special_text_type;
} coda_type_text;

typedef struct coda_type_raw_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;
    coda_expression *size_expr;
    coda_type_record *attributes;

    long fixed_value_length;
    char *fixed_value;
} coda_type_raw;

typedef struct coda_type_special_struct
{
    coda_format format;
    int retain_count;
    coda_type_class type_class;
    coda_native_type read_type;
    char *name;
    char *description;
    int64_t bit_size;
    coda_expression *size_expr;
    coda_type_record *attributes;

    coda_special_type special_type;
    coda_type *base_type;
    char *unit;
    coda_expression *value_expr;
} coda_type_special;

coda_conversion *coda_conversion_new(double numerator, double denominator, double add_offset, double invalid_value);
int coda_conversion_set_unit(coda_conversion *conversion, const char *unit);
void coda_conversion_delete(coda_conversion *conversion);

coda_ascii_integer_mapping *coda_ascii_integer_mapping_new(const char *str, int64_t value);
void coda_ascii_integer_mapping_delete(coda_ascii_integer_mapping *mapping);

coda_ascii_float_mapping *coda_ascii_float_mapping_new(const char *str, double value);
void coda_ascii_float_mapping_delete(coda_ascii_float_mapping *mapping);

void coda_type_record_field_delete(coda_type_record_field *field);
void coda_type_release(coda_type *type);

int coda_type_set_read_type(coda_type *type, coda_native_type read_type);
int coda_type_set_name(coda_type *type, const char *name);
int coda_type_set_description(coda_type *type, const char *description);
int coda_type_set_bit_size(coda_type *type, int64_t bit_size);
int coda_type_set_byte_size(coda_type *type, int64_t byte_size);
int coda_type_set_bit_size_expression(coda_type *type, coda_expression *bit_size_expr);
int coda_type_set_byte_size_expression(coda_type *type, coda_expression *byte_size_expr);
int coda_type_add_attribute(coda_type *type, coda_type_record_field *attribute);
int coda_type_set_attributes(coda_type *type, coda_type_record *attributes);

coda_type_record_field *coda_type_record_field_new(const char *name);
int coda_type_record_field_set_real_name(coda_type_record_field *field, const char *real_name);
int coda_type_record_field_set_type(coda_type_record_field *field, coda_type *type);
int coda_type_record_field_set_hidden(coda_type_record_field *field);
int coda_type_record_field_set_optional(coda_type_record_field *field);
int coda_type_record_field_set_available_expression(coda_type_record_field *field, coda_expression *available_expr);
int coda_type_record_field_set_bit_offset_expression(coda_type_record_field *field, coda_expression *bit_offset_expr);
int coda_type_record_field_validate(const coda_type_record_field *field);
int coda_type_record_field_get_type(const coda_type_record_field *field, coda_type **type);

coda_type_record *coda_type_record_new(coda_format format);
coda_type_record *coda_type_union_new(coda_format format);
coda_type_record *coda_type_empty_record(coda_format format);
int coda_type_record_add_field(coda_type_record *type, coda_type_record_field *field_type);
int coda_type_record_insert_field(coda_type_record *type, long index, coda_type_record_field *field);
int coda_type_record_create_field(coda_type_record *type, const char *real_name, coda_type *field_type);
int coda_type_union_set_field_expression(coda_type_record *type, coda_expression *field_expr);
int coda_type_record_validate(const coda_type_record *type);
char *coda_type_record_get_unique_field_name(const coda_type_record *type, const char *name);

coda_type_array *coda_type_array_new(coda_format format);
int coda_type_array_set_base_type(coda_type_array *type, coda_type *base_type);
int coda_type_array_add_fixed_dimension(coda_type_array *type, long dim);
int coda_type_array_add_variable_dimension(coda_type_array *type, coda_expression *dim_expr);
int coda_type_array_validate(const coda_type_array *type);

coda_type_number *coda_type_number_new(coda_format format, coda_type_class type_class);
int coda_type_number_set_unit(coda_type_number *type, const char *unit);
int coda_type_number_set_endianness(coda_type_number *type, coda_endianness endianness);
int coda_type_number_set_conversion(coda_type_number *type, coda_conversion *conversion);
int coda_type_number_add_ascii_float_mapping(coda_type_number *type, coda_ascii_float_mapping *mapping);
int coda_type_number_add_ascii_integer_mapping(coda_type_number *type, coda_ascii_integer_mapping *mapping);
int coda_type_number_validate(const coda_type_number *type);

coda_type_text *coda_type_text_new(coda_format format);
int coda_type_text_set_fixed_value(coda_type_text *type, const char *fixed_value);
int coda_type_text_set_special_text_type(coda_type_text *type, coda_ascii_special_text_type special_text_type);
int coda_type_text_validate(const coda_type_text *type);

coda_type_raw *coda_type_raw_new(coda_format format);
int coda_type_raw_set_fixed_value(coda_type_raw *type, long length, const char *fixed_value);
int coda_type_raw_validate(const coda_type_raw *type);

coda_type_raw *coda_type_raw_file_singleton(void);

coda_type_special *coda_type_no_data_singleton(coda_format format);

coda_type_special *coda_type_vsf_integer_new(coda_format format);
int coda_type_vsf_integer_set_type(coda_type_special *type, coda_type *base_type);
int coda_type_vsf_integer_set_scale_factor(coda_type_special *type, coda_type *scale_factor);
int coda_type_vsf_integer_set_unit(coda_type_special *type, const char *unit);
int coda_type_vsf_integer_validate(coda_type_special *type);

coda_type_special *coda_type_time_new(coda_format format, coda_expression *value_expr);
int coda_type_time_add_ascii_float_mapping(coda_type_special *type, coda_ascii_float_mapping *mapping);
int coda_type_time_set_base_type(coda_type_special *type, coda_type *base_type);
int coda_type_time_validate(coda_type_special *type);

coda_type_special *coda_type_complex_new(coda_format format);
int coda_type_complex_set_type(coda_type_special *type, coda_type *element_type);
int coda_type_complex_validate(coda_type_special *type);

void coda_type_done(void);

#endif
