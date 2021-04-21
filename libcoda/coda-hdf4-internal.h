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

#ifndef CODA_HDF4_INTERNAL_H
#define CODA_HDF4_INTERNAL_H

#include "coda-hdf4.h"
#include "coda-mem-internal.h"

#include "hdf.h"
#include "mfhdf.h"

#define MAX_HDF4_NAME_LENGTH 256
#define MAX_HDF4_VAR_DIMS 32

typedef enum hdf4_type_tag_enum
{
    tag_hdf4_basic_type,        /* coda_integer_class, coda_real_class, coda_text_class */
    tag_hdf4_basic_type_array,  /* coda_array_class */
    tag_hdf4_string,    /* coda_text_class (= attribute containing array of chars) */
    tag_hdf4_attributes,        /* coda_record_class */
    tag_hdf4_file_attributes,   /* coda_record_class */
    tag_hdf4_GRImage,   /* coda_array_class */
    tag_hdf4_SDS,       /* coda_array_class */
    tag_hdf4_Vdata,     /* coda_record_class */
    tag_hdf4_Vdata_field,       /* coda_array_class */
    tag_hdf4_Vgroup     /* coda_record_class */
} hdf4_type_tag;

/* Inheritance tree:
 * coda_dynamic_type
 * \ -- coda_hdf4_type
 *      \ -- coda_hdf4_basic_type_array
 *       |-- coda_hdf4_attributes
 *       |-- coda_hdf4_file_attributes
 *       |-- coda_hdf4_GRImage
 *       |-- coda_hdf4_SDS
 *       |-- coda_hdf4_Vdata
 *       |-- coda_hdf4_Vdata_field
 *       |-- coda_hdf4_Vgroup
 */

typedef struct coda_hdf4_type_struct
{
    coda_backend backend;
    coda_type *definition;
    hdf4_type_tag tag;
} coda_hdf4_type;

/* We only use this type for attribute data.
 * Although other types, such as GRImage, and Vdata objects also have a properties such as 'ncomp' and 'order'
 * that might be used to create an array of basic types, the 'ncomp' and 'order' for these types can be more
 * naturally implemented as additional diminsions to the parent type (which is an array).
 * We therefore only use the basic_type_array if the parent compound type is a record (which is only when the parent
 * type is a tag_hdf4_attributes or tag_hdf4_file_attributes type).
 */
typedef struct coda_hdf4_basic_type_array_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf4_type_tag tag;
    coda_hdf4_type *basic_type;
} coda_hdf4_basic_type_array;

typedef struct coda_hdf4_attributes_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf4_type_tag tag;
    hdf4_type_tag parent_tag;
    int32 parent_id;
    int32 field_index;  /* only for Vdata */
    coda_hdf4_type **attribute; /* basic types for each of the attributes */
    int32 num_obj_attributes;
    int32 num_data_labels;
    int32 num_data_descriptions;
    int32 *ann_id;
} coda_hdf4_attributes;

typedef struct coda_hdf4_file_attributes_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf4_type_tag tag;
    coda_hdf4_type **attribute; /* basic types for each of the attributes */
    int32 num_gr_attributes;
    int32 num_sd_attributes;
    int32 num_file_labels;
    int32 num_file_descriptions;
} coda_hdf4_file_attributes;

typedef struct coda_hdf4_GRImage_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 ri_id;
    int32 index;
    char gri_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 ncomp;
    int32 data_type;
    int32 interlace_mode;
    int32 dim_sizes[2];
    coda_hdf4_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_GRImage;

typedef struct coda_hdf4_SDS_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 sds_id;
    int32 index;
    char sds_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 rank;
    int32 dimsizes[MAX_HDF4_VAR_DIMS];
    int32 data_type;
    coda_hdf4_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_SDS;

typedef struct coda_hdf4_Vdata_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 vdata_id;
    int32 hide;
    char vdata_name[MAX_HDF4_NAME_LENGTH + 1];
    char classname[MAX_HDF4_NAME_LENGTH + 1];
    struct coda_hdf4_Vdata_field_struct **field;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vdata;

typedef struct coda_hdf4_Vdata_field_struct
{
    coda_backend backend;
    coda_type_array *definition;
    hdf4_type_tag tag;
    char field_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 num_records;
    int32 order;
    int num_elements;
    int32 data_type;
    coda_hdf4_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vdata_field;

typedef struct coda_hdf4_Vgroup_struct
{
    coda_backend backend;
    coda_type_record *definition;
    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 vgroup_id;
    int32 hide;
    char vgroup_name[MAX_HDF4_NAME_LENGTH + 1];
    char classname[MAX_HDF4_NAME_LENGTH + 1];
    int32 version;
    struct coda_hdf4_type_struct **entry;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vgroup;

struct coda_hdf4_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_mem_record *root_type;
    const coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;
    int64_t mem_size;
    uint8_t *mem_ptr;

    /* 'hdf4' product specific fields */
    int32 is_hdf;       /* is it a real HDF4 file or are we accessing a (net)CDF file */
    int32 file_id;
    int32 gr_id;
    int32 sd_id;
    int32 an_id;

    int32 num_sd_file_attributes;
    int32 num_gr_file_attributes;

    int32 num_sds;
    coda_hdf4_SDS **sds;

    int32 num_images;
    coda_hdf4_GRImage **gri;

    int32 num_vgroup;
    coda_hdf4_Vgroup **vgroup;

    int32 num_vdata;
    coda_hdf4_Vdata **vdata;
};
typedef struct coda_hdf4_product_struct coda_hdf4_product;

coda_hdf4_GRImage *coda_hdf4_GRImage_new(coda_hdf4_product *product, int32 index);
coda_hdf4_SDS *coda_hdf4_SDS_new(coda_hdf4_product *product, int32 sds_index);
coda_hdf4_Vdata *coda_hdf4_Vdata_new(coda_hdf4_product *product, int32 vdata_ref);
coda_hdf4_Vgroup *coda_hdf4_Vgroup_new(coda_hdf4_product *product, int32 vgroup_ref);
int coda_hdf4_create_root(coda_hdf4_product *product);

#endif
