/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

#ifndef CODA_HDF5_INTERNAL_H
#define CODA_HDF5_INTERNAL_H

#include "coda-hdf5.h"
#include "coda-type.h"
#include "coda-mem-internal.h"

/* HDF5 creates its own versions of uint32_t, int64_t, and uint64_t as typedefs */
/* We therefore disable our #define entries for these types if we have them */
#undef int64_t
#undef uint32_t
#undef uint64_t

#include "hdf5.h"

typedef enum hdf5_type_tag_enum
{
    tag_hdf5_basic_datatype,    /* coda_integer_class, coda_real_class, coda_text_class */
    tag_hdf5_compound_datatype, /* coda_record_class */
    tag_hdf5_group,     /* coda_record_class */
    tag_hdf5_dataset    /* coda_array_class */
} hdf5_type_tag;

/* Inheritance tree:
 * coda_dynamic_type
 * \ -- coda_hdf5_type
 *      \ -- coda_hdf5_data_type
 *           \ -- coda_hdf5_basic_data_type
 *            |-- coda_hdf5_compound_data_type
 *       |-- coda_hdf5_object
 *           \ -- coda_hdf5_group
 *            |-- coda_hdf5_dataset
 */

typedef struct coda_hdf5_type_struct
{
    coda_backend backend;
    coda_type *definition;
    hdf5_type_tag tag;
} coda_hdf5_type;

typedef struct coda_hdf5_object_struct
{
    coda_backend backend;
    coda_type *definition;
    hdf5_type_tag tag;
    unsigned long fileno[2];
    unsigned long objno[2];
} coda_hdf5_object;

typedef struct coda_hdf5_data_type_struct
{
    coda_backend backend;
    coda_type *definition;
    hdf5_type_tag tag;
    hid_t datatype_id;
} coda_hdf5_data_type;

typedef struct coda_hdf5_basic_data_type_struct
{
    coda_backend backend;
    coda_type *definition;
    hdf5_type_tag tag;
    hid_t datatype_id;
    int is_variable_string;
} coda_hdf5_basic_data_type;

typedef struct coda_hdf5_compound_data_type_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf5_type_tag tag;
    hid_t datatype_id;
    coda_hdf5_data_type **member;
    hid_t *member_type;
} coda_hdf5_compound_data_type;

typedef struct coda_hdf5_group_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf5_type_tag tag;
    unsigned long fileno[2];
    unsigned long objno[2];
    hid_t group_id;
    coda_hdf5_object **object;
    coda_mem_record *attributes;
} coda_hdf5_group;

typedef struct coda_hdf5_dataset_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf5_type_tag tag;
    unsigned long fileno[2];
    unsigned long objno[2];
    hid_t dataset_id;
    hid_t dataspace_id;
    coda_hdf5_data_type *base_type;
    coda_mem_record *attributes;
} coda_hdf5_dataset;

struct coda_hdf5_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_hdf5_object *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    uint8_t *mem_ptr;

    /* 'hdf5' product specific fields */
    hid_t file_id;
    hsize_t num_objects;
    struct coda_hdf5_object_struct **object;
};
typedef struct coda_hdf5_product_struct coda_hdf5_product;

int coda_hdf5_create_tree(coda_hdf5_product *product, hid_t loc_id, const char *path, coda_hdf5_object **object);
int coda_hdf5_basic_type_set_conversion(coda_hdf5_data_type *type, coda_conversion *conversion);

#endif
