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

#ifndef CODA_BIN_INTERNAL_H
#define CODA_BIN_INTERNAL_H

#include "coda-ascbin-internal.h"
#include "coda-bin.h"
#include "coda-bin-definition.h"
#include "coda-definition.h"

typedef enum bin_type_tag_enum
{
    tag_bin_record,     /* coda_record_class */
    tag_bin_union,      /* coda_record_class */
    tag_bin_array,      /* coda_array_class */
    tag_bin_integer,    /* coda_integer_class */
    tag_bin_float,      /* coda_real_class */
    tag_bin_raw,        /* coda_raw_class */
    tag_bin_no_data,    /* coda_special_class */
    tag_bin_vsf_integer,        /* coda_special_class */
    tag_bin_time,       /* coda_special_class */
    tag_bin_complex     /* coda_special_class */
} bin_type_tag;

enum coda_bin_time_type_enum
{
    binary_envisat_datetime,            /**< Record with 3 fields: days since 1 Jan 2000 (int32), seconds since that day
                                             (uint32), microseconds since that second (uint32). */
    binary_gome_datetime,               /**< Record with 2 fields: days since 1 Jan 1950 (int32), milliseconds since
                                             that day (uint32). */
    binary_eps_datetime,                /**< Record with 2 fields: days since 1 Jan 2000 (uint16), milliseconds since
                                             that day (uint32). */
    binary_eps_datetime_long            /**< Record with 3 fields: days since 1 Jan 2000 (uint16), milliseconds since
                                             that day (uint32), microseconds since that millisecond (uint16). */
};
typedef enum coda_bin_time_type_enum coda_bin_time_type;

struct coda_bin_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* -1 means it's variable and needs to be calculated */
};

struct coda_bin_number_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;
    char *unit;
    coda_native_type read_type;
    coda_conversion *conversion;
};
typedef struct coda_bin_number_struct coda_bin_number;

struct coda_bin_special_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */
    coda_bin_type *base_type;
};
typedef struct coda_bin_special_type_struct coda_bin_special_type;

struct coda_binRecord_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;
    coda_expression *fast_size_expr;
    hashtable *hash_data;
    long num_fields;
    coda_ascbin_field **field;
    int has_hidden_fields;
    int has_available_expr_fields;
};

struct coda_binUnion_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;
    coda_expression *fast_size_expr;
    hashtable *hash_data;
    long num_fields;
    coda_ascbin_field **field;
    int has_hidden_fields;
    int has_available_expr_fields;
    coda_expression *field_expr;        /* returns index in range [0..num_fields) */
};

struct coda_binArray_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;
    coda_bin_type *base_type;
    long num_elements;
    int num_dims;
    long *dim;  /* -1 means it's variable and the value needs to be retrieved from dim_expr */
    coda_expression **dim_expr;
};

struct coda_bin_integer_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* anywhere from 1 to 64 bits. -1 means it's variable -> use bit_size_expr */
    char *unit;
    coda_native_type read_type;
    coda_conversion *conversion;
    coda_endianness endianness;
    coda_expression *bit_size_expr;
};

struct coda_bin_float_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* either 32 or 64 */
    char *unit;
    coda_native_type read_type;
    coda_conversion *conversion;
    coda_endianness endianness;
};

struct coda_bin_raw_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;
    coda_expression *bit_size_expr;
    long fixed_value_length;
    char *fixed_value;
};

struct coda_bin_no_data_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type (which is always 0) */
    coda_bin_type *base_type;
};
typedef struct coda_bin_no_data_struct coda_bin_no_data;

struct coda_bin_vsf_integer_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */
    coda_bin_type *base_type;
    char *unit;
};

struct coda_bin_time_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */
    coda_bin_type *base_type;
    coda_bin_time_type time_type;
};

struct coda_bin_complex_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    bin_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */
    coda_bin_type *base_type;
};

#endif
