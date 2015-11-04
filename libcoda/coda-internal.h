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

enum coda_endianness_enum
{
    coda_big_endian,    /**< Most significant byte comes first. */
    coda_little_endian  /**< Least significant byte comes first. */
};
typedef enum coda_endianness_enum coda_endianness;

/* type 'base class' that describes the dynamic (i.e. instance specific) type information of a data element */
/* this is the type that is used within coda_ProductFile for the root type and within coda_Cursor */
/* depending on the backend a coda_DynamicType instance can also be a coda_Type (or vice versa) */
struct coda_DynamicType_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
};
typedef struct coda_DynamicType_struct coda_DynamicType;

/* type 'base class' that describes the definition of a data element */
/* this is the type that is used throughout the CODA Types module */
struct coda_Type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;
};

typedef struct coda_ProductDefinition_struct coda_ProductDefinition;

struct coda_ProductFile_struct
{
    /* general fields (shared between all supported product types) */

    char *filename;
    int64_t file_size;
    coda_format format;
    coda_DynamicType *root_type;
    coda_ProductDefinition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
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

void coda_release_type(coda_Type *type);
void coda_release_dynamic_type(coda_DynamicType *type);

int coda_data_dictionary_init(void);
void coda_data_dictionary_done(void);
int coda_read_definitions(const char *path);
int coda_read_product_definition(coda_ProductDefinition *product_definition);

int coda_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type);

int coda_product_variable_get_size(coda_ProductFile *pf, const char *name, long *size);
int coda_product_variable_get_pointer(coda_ProductFile *pf, const char *name, long i, int64_t **ptr);

const char *coda_element_name_from_xml_name(const char *xml_name);
int coda_is_identifier(const char *name);
char *coda_identifier_from_name(const char *name, hashtable *hash_data);
char *coda_short_identifier_from_name(const char *name, hashtable *hash_data, int maxlength);

int coda_array_transpose(void *array, int num_dims, const long dim[], int element_size);

int coda_dayofyear_to_month_day(int year, int day_of_year, int *month, int *day_of_month);
int coda_month_to_integer(const char month[3]);

#endif
