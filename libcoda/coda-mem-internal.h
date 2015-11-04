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

#ifndef CODA_MEM_INTERNAL_H
#define CODA_MEM_INTERNAL_H

#include "coda-mem.h"
#include "coda-type.h"

typedef struct coda_mem_type_struct
{
    coda_backend backend;
    coda_type *definition;
    coda_dynamic_type *attributes;
} coda_mem_type;

typedef struct coda_mem_record_struct
{
    coda_backend backend;
    coda_type_record *definition;
    coda_dynamic_type *attributes;
    long num_fields;
    coda_dynamic_type **field_type;     /* if field_type[i] == NULL then field #i is not available */
} coda_mem_record;

typedef struct coda_mem_array_struct
{
    coda_backend backend;
    coda_type_array *definition;
    coda_dynamic_type *attributes;
    long num_elements;
    coda_dynamic_type **element;
} coda_mem_array;

typedef struct coda_mem_integer_struct
{
    coda_backend backend;
    coda_type_number *definition;
    coda_dynamic_type *attributes;
    int64_t value;
} coda_mem_integer;

typedef struct coda_mem_real_struct
{
    coda_backend backend;
    coda_type_number *definition;
    coda_dynamic_type *attributes;
    double value;
} coda_mem_real;

typedef struct coda_mem_text_struct
{
    coda_backend backend;
    coda_type_text *definition;
    coda_dynamic_type *attributes;
    char *text;
} coda_mem_text;

typedef struct coda_mem_raw_struct
{
    coda_backend backend;
    coda_type_raw *definition;
    coda_dynamic_type *attributes;
    long length;
    uint8_t *data;
} coda_mem_raw;

typedef struct coda_mem_special_struct
{
    coda_backend backend;
    coda_type_special *definition;
    coda_dynamic_type *attributes;
    coda_dynamic_type *base_type;
} coda_mem_special;

typedef struct coda_mem_time_struct
{
    coda_backend backend;
    coda_type_special *definition;
    coda_dynamic_type *attributes;
    coda_dynamic_type *base_type;
    double value;
} coda_mem_time;


int coda_mem_type_add_attribute(coda_mem_type *type, const char *real_name, coda_dynamic_type *attribute_type,
                                int update_definition);
int coda_mem_type_set_attributes(coda_mem_type *type, coda_dynamic_type *attributes, int update_definition);

coda_mem_record *coda_mem_record_new(coda_type_record *definition);
int coda_mem_record_add_field(coda_mem_record *type, const char *name, coda_dynamic_type *field_type,
                              int update_definition);
int coda_mem_record_validate(coda_mem_record *type);

coda_mem_array *coda_mem_array_new(coda_type_array *definition);

/* use if array definition has dynamic length */
int coda_mem_array_add_element(coda_mem_array *type, coda_dynamic_type *element);

/* use if array definition has static length */
int coda_mem_array_set_element(coda_mem_array *type, long index, coda_dynamic_type *element);
int coda_mem_array_validate(coda_mem_array *type);

coda_mem_integer *coda_mem_integer_new(coda_type_number *definition, int64_t value);
coda_mem_real *coda_mem_real_new(coda_type_number *definition, double value);
coda_mem_text *coda_mem_char_new(coda_type_text *definition, char value);
coda_mem_text *coda_mem_text_new(coda_type_text *definition, const char *text);
coda_mem_raw *coda_mem_raw_new(coda_type_raw *definition, long length, const uint8_t *data);
coda_mem_time *coda_mem_time_new(coda_type_special *definition, double value, coda_dynamic_type *base_type);

coda_mem_special *coda_mem_no_data_new(coda_format format);

#endif
