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

#ifndef CODA_ASCII_INTERNAL_H
#define CODA_ASCII_INTERNAL_H

#include "coda-ascbin-internal.h"
#include "coda-ascii.h"
#include "coda-expr.h"
#include "coda-ascii-definition.h"
#include "coda-definition.h"

typedef enum ascii_type_tag_enum
{
    tag_ascii_record,   /* coda_record_class */
    tag_ascii_union,    /* coda_record_class */
    tag_ascii_array,    /* coda_array_class */
    tag_ascii_integer,  /* coda_integer_class */
    tag_ascii_float,    /* coda_real_class */
    tag_ascii_text,     /* coda_text_class */
    tag_ascii_line_separator,   /* coda_text_class */
    tag_ascii_line,     /* coda_text_class */
    tag_ascii_white_space,      /* coda_text_class */
    tag_ascii_time      /* coda_special_class */
} ascii_type_tag;

enum coda_ascii_time_type_enum
{
    ascii_envisat_datetime,             /**< Fixed length ASCII string with the following format:
                                             "DD-MMM-YYYY hh:mm:ss.uuuuuu" */
    ascii_gome_datetime,                /**< Fixed length ASCII string with the following format:
                                             "DD-MMM-YYYY hh:mm:ss.uuu" */
    ascii_eps_datetime,                 /**< Fixed length ASCII string with the following format: "YYYYMMDDHHMMSSZ"
                                             (with exception "xxxxxxxxxxxxxxZ") */
    ascii_eps_datetime_long,            /**< Fixed length ASCII string with the following format: "YYYYMMDDHHMMSSmmmZ"
                                             (with exception "xxxxxxxxxxxxxxxxxZ") */
    ascii_ccsds_datetime_ymd1,          /**< Fixed length ASCII string with the following format: "YYYY-MM-DDThh:mm:ss"
                                             */
    ascii_ccsds_datetime_ymd1_with_ref, /**< Fixed length ASCII string with the following format:
                                             "RRR=YYYY-MM-DDThh:mm:ss". "RRR" can be any of "UT1", "UTC", "TAI", or
                                             "GPS" */
    ascii_ccsds_datetime_ymd2,          /**< Fixed length ASCII string with the following format:
                                             "YYYY-MM-DDThh:mm:ss.uuuuuu" */
    ascii_ccsds_datetime_ymd2_with_ref, /**< Fixed length ASCII string with the following format:
                                             "RRR=YYYY-MM-DDThh:mm:ss.uuuuuu". "RRR" can be any of "UT1", "UTC", "TAI",
                                             or "GPS" */
    ascii_ccsds_datetime_utc1,          /**< Fixed length ASCII string with the following format: "YYYY-DDDThh:mm:ss" */
    ascii_ccsds_datetime_utc2           /**< Fixed length ASCII string with the following format:
                                             "YYYY-DDDThh:mm:ss.uuuuuu" */
};
typedef enum coda_ascii_time_type_enum coda_ascii_time_type;

struct coda_asciiMapping_struct
{
    int length;
    char *str;
};
typedef struct coda_asciiMapping_struct coda_asciiMapping;

struct coda_asciiIntegerMapping_struct
{
    int length;
    char *str;
    int64_t value;
};

struct coda_asciiFloatMapping_struct
{
    int length;
    char *str;
    double value;
};

struct coda_asciiMappings_struct
{
    int64_t default_bit_size;   /* bit_size if none of the mappings apply */
    int num_mappings;
    coda_asciiMapping **mapping;
};
typedef struct coda_asciiMappings_struct coda_asciiMappings;


struct coda_asciiType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* -1 means it's variable and needs to be calculated */
};

struct coda_asciiMappingsType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* -1 means it's variable and needs to be calculated */
    coda_asciiMappings *mappings;
};
typedef struct coda_asciiMappingsType_struct coda_asciiMappingsType;

struct coda_asciiNumber_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;
    coda_asciiMappings *mappings;
    char *unit;
    coda_native_type read_type;
    coda_Conversion *conversion;
};
typedef struct coda_asciiNumber_struct coda_asciiNumber;

struct coda_asciiSpecialType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */
    coda_asciiType *base_type;
};
typedef struct coda_asciiSpecialType_struct coda_asciiSpecialType;

struct coda_asciiInteger_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* if -1 than no fixed length, size determined by reading */
    coda_asciiMappings *mappings;
    char *unit;
    coda_native_type read_type;
    coda_Conversion *conversion;
};

struct coda_asciiFloat_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* if -1 than no fixed length, size determined by reading */
    coda_asciiMappings *mappings;
    char *unit;
    coda_native_type read_type;
    coda_Conversion *conversion;
};

struct coda_asciiText_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;
    coda_asciiMappings *mappings;
    coda_native_type read_type;
    coda_Expr *byte_size_expr;
    char *fixed_value;
};

struct coda_asciiLineSeparator_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* always -1 */
};

struct coda_asciiLine_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* always -1 */

    int include_eol;
};

struct coda_asciiWhiteSpace_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* always -1 */
};

struct coda_asciiTime_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    ascii_type_tag tag;
    int64_t bit_size;   /* same as bit_size of base_type */

    coda_asciiType *base_type;
    coda_ascii_time_type time_type;
};

int coda_ascii_init_asciilines(coda_ProductFile *pf);

#endif
