/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

/* When auto-growing coda_product.mem_ptr (using realloc) this will be a multiple of DATA_BLOCK_SIZE  */
#define DATA_BLOCK_SIZE 4096

typedef enum mem_type_tag_enum
{
    tag_mem_record,
    tag_mem_array,
    tag_mem_data,
    tag_mem_special
} mem_type_tag;

typedef struct coda_mem_type_struct
{
    coda_backend backend;
    coda_type *definition;
    mem_type_tag tag;
    coda_dynamic_type *attributes;
} coda_mem_type;

typedef struct coda_mem_record_struct
{
    coda_backend backend;
    coda_type_record *definition;
    mem_type_tag tag;
    coda_dynamic_type *attributes;
    long num_fields;
    coda_dynamic_type **field_type;     /* if field_type[i] == NULL then field #i is not available */
} coda_mem_record;

typedef struct coda_mem_array_struct
{
    coda_backend backend;
    coda_type_array *definition;
    mem_type_tag tag;
    coda_dynamic_type *attributes;
    long num_elements;
    coda_dynamic_type **element;
} coda_mem_array;

typedef struct coda_mem_data_struct
{
    coda_backend backend;
    coda_type *definition;
    mem_type_tag tag;
    coda_dynamic_type *attributes;
    long length;        /* byte length of data block in coda_product.mem_ptr */
    int64_t offset;     /* byte offset within coda_product.mem_ptr */
} coda_mem_data;

typedef struct coda_mem_special_struct
{
    coda_backend backend;
    coda_type_special *definition;
    mem_type_tag tag;
    coda_dynamic_type *attributes;
    coda_dynamic_type *base_type;
} coda_mem_special;

int coda_mem_type_update(coda_dynamic_type **type, coda_type *definition);

int coda_mem_type_add_attribute(coda_mem_type *type, const char *real_name, coda_dynamic_type *attribute_type,
                                int update_definition);
int coda_mem_type_set_attributes(coda_mem_type *type, coda_dynamic_type *attributes, int update_definition);

coda_mem_record *coda_mem_record_new(coda_type_record *definition, coda_dynamic_type *attributes);
int coda_mem_record_add_field(coda_mem_record *type, const char *name, coda_dynamic_type *field_type,
                              int update_definition);
int coda_mem_record_validate(coda_mem_record *type);

coda_mem_array *coda_mem_array_new(coda_type_array *definition, coda_dynamic_type *attributes);

/* use coda_mem_array_add_element() if array definition has dynamic length */
int coda_mem_array_add_element(coda_mem_array *type, coda_dynamic_type *element);

/* use coda_mem_array_set_element() if array definition has static length */
int coda_mem_array_set_element(coda_mem_array *type, long index, coda_dynamic_type *element);
int coda_mem_array_validate(coda_mem_array *type);

coda_mem_data *coda_mem_data_new(coda_type *definition, coda_dynamic_type *attributes, coda_product *product,
                                 long length, const uint8_t *data);
coda_mem_data *coda_mem_int8_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                 int8_t value);
coda_mem_data *coda_mem_uint8_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  uint8_t value);
coda_mem_data *coda_mem_int16_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int16_t value);
coda_mem_data *coda_mem_uint16_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint16_t value);
coda_mem_data *coda_mem_int32_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int32_t value);
coda_mem_data *coda_mem_uint32_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint32_t value);
coda_mem_data *coda_mem_int64_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  int64_t value);
coda_mem_data *coda_mem_uint64_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   uint64_t value);
coda_mem_data *coda_mem_float_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                  float value);
coda_mem_data *coda_mem_double_new(coda_type_number *definition, coda_dynamic_type *attributes, coda_product *product,
                                   double value);
coda_mem_data *coda_mem_char_new(coda_type_text *definition, coda_dynamic_type *attributes, coda_product *product,
                                 char value);
coda_mem_data *coda_mem_string_new(coda_type_text *definition, coda_dynamic_type *attributes, coda_product *product,
                                   const char *str);
coda_mem_data *coda_mem_raw_new(coda_type_raw *definition, coda_dynamic_type *attributes, coda_product *product,
                                long length, const uint8_t *data);

coda_mem_special *coda_mem_time_new(coda_type_special *definition, coda_dynamic_type *attributes,
                                    coda_dynamic_type *base_type);
coda_mem_special *coda_mem_no_data_new(coda_format format);

#endif
