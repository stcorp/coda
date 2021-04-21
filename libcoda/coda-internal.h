/*
 * Copyright (C) 2007-2021 S[&]T, The Netherlands.
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

#ifndef CODA_INTERNAL_H
#define CODA_INTERNAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef THREAD_LOCAL
#define THREAD_LOCAL
#endif

#include <stdarg.h>

#define CODA_INTERNAL

#include "coda.h"
#include "hashtable.h"

/* This defines the amount of items that will be allocated per block for an auto-growing array (using realloc) */
#define BLOCK_SIZE 16

#define bit_size_to_byte_size(x) (((x) >> 3) + ((((uint8_t)(x)) & 0x7) != 0))

/* Make sure this define is set to the last enum value of coda_format */
#define CODA_NUM_FORMATS (coda_format_sp3 + 1)

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
    coda_backend_hdf4,  /**< Backend that reads data via the HDF4 library */
    coda_backend_hdf5,  /**< Backend that reads data via the HDF5 library */
    coda_backend_cdf,   /**< Backend that reads data from CDF files */
    coda_backend_netcdf,      /**< Backend that reads data from netCDF 3.x files */
    coda_backend_grib   /**< Backend that reads data from GRIB files */
};
typedef enum coda_backend_enum coda_backend;

/* type 'base class' that describes the dynamic (i.e. instance specific) type information of a data element */
/* this is the type that is used within coda_product for the root type and within coda_cursor */
/* depending on the backend a coda_dynamic_type instance can also be a coda_type */
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
    int64_t mem_size;
    uint8_t *mem_ptr;
};

extern THREAD_LOCAL const char *libcoda_version;

extern THREAD_LOCAL int coda_errno;

extern THREAD_LOCAL int coda_option_bypass_special_types;
extern THREAD_LOCAL int coda_option_perform_boundary_checks;
extern THREAD_LOCAL int coda_option_perform_conversions;
extern THREAD_LOCAL int coda_option_read_all_definitions;
extern THREAD_LOCAL int coda_option_use_fast_size_expressions;
extern THREAD_LOCAL int coda_option_use_mmap;

#define coda_get_type_for_dynamic_type(dynamic_type) (((coda_dynamic_type *)dynamic_type)->backend < first_dynamic_backend_id ? (coda_type *)dynamic_type : ((coda_dynamic_type *)dynamic_type)->definition)

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
void coda_dynamic_type_delete(coda_dynamic_type *type);

LIBCODA_API int coda_type_get_record_field_index_from_name_n(const coda_type *type, const char *name, int name_length,
                                                             long *index);

int coda_cursor_compare(const coda_cursor *cursor1, const coda_cursor *cursor2);

int coda_expression_print_html(const coda_expression *expr, int (*print)(const char *, ...));
int coda_expression_print_xml(const coda_expression *expr, int (*print)(const char *, ...));

int coda_product_variable_get_size(coda_product *product, const char *name, long *size);
int coda_product_variable_get_pointer(coda_product *product, const char *name, long i, int64_t **ptr);

int coda_expression_eval_void(const coda_expression *expr, const coda_cursor *cursor);

int coda_format_from_string(const char *str, coda_format *format);
const char *coda_element_name_from_xml_name(const char *xml_name);
int coda_is_identifier(const char *name);
char *coda_identifier_from_name(const char *name, hashtable *hash_data);

int coda_dayofyear_to_month_day(int year, int day_of_year, int *month, int *day_of_month);
int coda_month_to_integer(const char month[3]);
int coda_leap_second_table_init(void);
void coda_leap_second_table_done(void);

#endif
