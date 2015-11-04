/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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

#ifndef CODA_INTERNAL_H
#define CODA_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>

#define CODA_INTERNAL

#include "coda.h"
#include "hashtable.h"

/* This defines the amount of items that will be allocated per block for an auto-growing array (using realloc) */
#define BLOCK_SIZE 16

#define bit_size_to_byte_size(x) (((x) >> 3) + ((((uint8_t)(x)) & 0x7) != 0))

enum coda_endianness_enum
{
    coda_big_endian,    /**< Most significant byte comes first. */
    coda_little_endian  /**< Least significant byte comes first. */
};
typedef enum coda_endianness_enum coda_endianness;

#define first_dynamic_backend_id 100
enum coda_backend_enum
{
    /* first all backends for which the dynamic types equal the static types */
    coda_backend_ascii = coda_format_ascii, /**< Backend that reads ascii data from a file */
    coda_backend_binary = coda_format_binary,   /**< Backend that reads binary data from a file */

    /* then all backends that have separate dynamic types */
    coda_backend_memory = first_dynamic_backend_id,   /**< Backend that feeds data from memory */
    coda_backend_xml,   /**< Backend that reads data from an XML file */
    coda_backend_hdf4,  /**< Backend that reads data via the HDF4 library */
    coda_backend_hdf5,  /**< Backend that reads data via the HDF5 library */
    coda_backend_cdf,   /**< Backend that reads data from CDF files */
    coda_backend_netcdf,      /**< Backend that reads data from netCDF 3.x files */
    coda_backend_grib   /**< Backend that reads data from GRIB files */
};
typedef enum coda_backend_enum coda_backend;

/* type 'base class' that describes the dynamic (i.e. instance specific) type information of a data element */
/* this is the type that is used within coda_product for the root type and within coda_cursor */
/* depending on the backend a coda_dynamic_type instance can also be a coda_type (or vice versa) */
struct coda_dynamic_type_struct
{
    coda_backend backend;
    coda_type *definition;
};
typedef struct coda_dynamic_type_struct coda_dynamic_type;

typedef struct coda_product_definition_struct coda_product_definition;

struct coda_product_struct
{
    /* general fields (shared between all supported product types) */

    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
#if CODA_USE_QIAP
    void *qiap_info;
#endif
};

extern int coda_option_bypass_special_types;
extern int coda_option_perform_boundary_checks;
extern int coda_option_perform_conversions;
extern int coda_option_read_all_definitions;
extern int coda_option_use_fast_size_expressions;
extern int coda_option_use_mmap;

void coda_add_error_message(const char *message, ...);
void coda_set_error_message(const char *message, ...);
void coda_add_error_message_vargs(const char *message, va_list ap);
void coda_set_error_message_vargs(const char *message, va_list ap);
void coda_cursor_add_to_error_message(const coda_cursor *cursor);

int coda_data_dictionary_init(void);
void coda_data_dictionary_done(void);
int coda_read_definitions(const char *path);
int coda_read_product_definition(coda_product_definition *product_definition);

coda_dynamic_type *coda_no_data_singleton(coda_format format);
coda_dynamic_type *coda_mem_empty_record(coda_format format);
coda_type *coda_get_type_for_dynamic_type(coda_dynamic_type *dynamic_type);
void coda_dynamic_type_delete(coda_dynamic_type *type);
int coda_dynamic_type_update(coda_dynamic_type **type, coda_type **definition);

LIBCODA_API int coda_type_get_record_field_index_from_name_n(const coda_type *type, const char *name, int name_length,
                                                             long *index);

int coda_cursor_print_path(const coda_cursor *cursor, int (*print) (const char *, ...));

int coda_product_variable_get_size(coda_product *product, const char *name, long *size);
int coda_product_variable_get_pointer(coda_product *product, const char *name, long i, int64_t **ptr);

int coda_expression_eval_void(const coda_expression *expr, const coda_cursor *cursor);

const char *coda_element_name_from_xml_name(const char *xml_name);
int coda_is_identifier(const char *name);
char *coda_identifier_from_name(const char *name, hashtable *hash_data);
char *coda_short_identifier_from_name(const char *name, hashtable *hash_data, int maxlength);

int coda_dayofyear_to_month_day(int year, int day_of_year, int *month, int *day_of_month);
int coda_month_to_integer(const char month[3]);
int coda_string_to_time_with_format(const char *format, const char *str, double *datetime);
int coda_time_to_string_with_format(const char *format, double datetime, char *str);
int coda_leap_second_table_init(void);
void coda_leap_second_table_done(void);

#endif
