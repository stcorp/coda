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

#ifndef CODA_HDF5_INTERNAL_H
#define CODA_HDF5_INTERNAL_H

#include "coda-hdf5.h"
#include "coda-type.h"

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
    tag_hdf5_attribute, /* coda_array_class */
    tag_hdf5_attribute_record,  /* coda_record_class */
    tag_hdf5_group,     /* coda_record_class */
    tag_hdf5_dataset    /* coda_array_class */
} hdf5_type_tag;

/* Inheritance tree:
 * coda_dynamic_type
 * \ -- coda_hdf5_type
 *      \ -- coda_hdf5_data_type
 *           \ -- coda_hdf5_basic_data_type
 *            |-- coda_hdf5_compound_data_type
 *       |-- coda_hdf5_attribute
 *       |-- coda_hdf5_attribute_record
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

typedef struct coda_hdf5_attribute_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf5_type_tag tag;
    hid_t attribute_id;
    hid_t dataspace_id;
    int ndims;
    hsize_t dims[CODA_MAX_NUM_DIMS];
    coda_hdf5_data_type *base_type;
} coda_hdf5_attribute;

typedef struct coda_hdf5_attribute_record_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf5_type_tag tag;
    hid_t obj_id;       /* id of object to which the attributes are attached */
    coda_hdf5_attribute **attribute;
} coda_hdf5_attribute_record;

typedef struct coda_hdf5_group_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf5_type_tag tag;
    unsigned long fileno[2];
    unsigned long objno[2];
    hid_t group_id;
    coda_hdf5_object **object;
    coda_hdf5_attribute_record *attributes;
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
    coda_hdf5_attribute_record *attributes;
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
#if CODA_USE_QIAP
    void *qiap_info;
#endif

    hid_t file_id;
    hsize_t num_objects;
    struct coda_hdf5_object_struct **object;
};
typedef struct coda_hdf5_product_struct coda_hdf5_product;

int coda_hdf5_create_tree(coda_hdf5_product *product, hid_t loc_id, const char *path, coda_hdf5_object **object);

#endif
