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

#ifndef CODA_HDF4_INTERNAL_H
#define CODA_HDF4_INTERNAL_H

#include "coda-hdf4.h"

#include "hdf.h"
#include "mfhdf.h"

#define MAX_HDF4_NAME_LENGTH 64
#define MAX_HDF4_VAR_DIMS 32

typedef enum hdf4_type_tag_enum
{
    tag_hdf4_root,      /* coda_record_class */
    tag_hdf4_basic_type,        /* coda_integer_class, coda_real_class, coda_text_class */
    tag_hdf4_basic_type_array,  /* coda_array_class */
    tag_hdf4_attributes,        /* coda_record_class */
    tag_hdf4_file_attributes,   /* coda_record_class */
    tag_hdf4_GRImage,   /* coda_array_class */
    tag_hdf4_SDS,       /* coda_array_class */
    tag_hdf4_Vdata,     /* coda_record_class */
    tag_hdf4_Vdata_field,       /* coda_array_class */
    tag_hdf4_Vgroup     /* coda_record_class */
} hdf4_type_tag;

/* Inheritance tree:
 * coda_type
 * \ -- coda_hdf4_type
 *      \ -- coda_hdf4_root
 *       |-- coda_hdf4_basic_type
 *       |-- coda_hdf4_basic_type_array
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
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
} coda_hdf4_type;

typedef struct coda_hdf4_root_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    int32 num_entries;
    struct coda_hdf4_type_struct **entry;
    char **entry_name;
    hashtable *hash_data;
    struct coda_hdf4_file_attributes_struct *attributes;
} coda_hdf4_root;

typedef struct coda_hdf4_basic_type_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    coda_native_type read_type;
    int has_conversion;
    double add_offset;
    double scale_factor;
} coda_hdf4_basic_type;

/* We only use this type for attribute data.
 * Although other types, such as GRImage, and Vdata objects also have a properties such as 'ncomp' and 'order'
 * that might be used to create an array of basic types, the 'ncomp' and 'order' for these types can be more
 * naturally implemented as additional diminsions to the parent type (which is an array).
 * We therefore only use the basic_type_array if the parent compound type is a record (which is only when the parent
 * type is a tag_hdf4_attributes or tag_hdf4_file_attributes type).
 */
typedef struct coda_hdf4_basic_type_array_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    int32 count;        /* number of basic types */
    coda_hdf4_basic_type *basic_type;
} coda_hdf4_basic_type_array;

typedef struct coda_hdf4_attributes_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    hdf4_type_tag parent_tag;
    int32 parent_id;
    int32 field_index;  /* only for Vdata */
    /* total number of attributes = num_obj_attributes + num_data_labels + num_data_descriptions */
    int32 num_attributes;
    coda_hdf4_type **attribute; /* basic types for each of the attributes */
    char **attribute_name;
    hashtable *hash_data;

    int32 num_obj_attributes;
    int32 num_data_labels;
    int32 num_data_descriptions;
    int32 *ann_id;
} coda_hdf4_attributes;

typedef struct coda_hdf4_file_attributes_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    hdf4_type_tag parent_tag;
    /* total number of attributes = num_gr_attributes + num_sd_attributes + num_file_labels + num_file_descriptions */
    int32 num_attributes;
    coda_hdf4_type **attribute; /* basic types for each of the attributes */
    char **attribute_name;
    hashtable *hash_data;

    int32 num_gr_attributes;
    int32 num_sd_attributes;
    int32 num_file_labels;
    int32 num_file_descriptions;
} coda_hdf4_file_attributes;

typedef struct coda_hdf4_GRImage_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

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
    int num_elements;
    int32 num_attributes;
    coda_hdf4_basic_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_GRImage;

typedef struct coda_hdf4_SDS_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 sds_id;
    int32 index;
    char sds_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 rank;
    int32 dimsizes[MAX_HDF4_VAR_DIMS];
    int num_elements;
    int32 data_type;
    int32 num_attributes;
    coda_hdf4_basic_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_SDS;

typedef struct coda_hdf4_Vdata_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 vdata_id;
    int32 hide;
    char vdata_name[MAX_HDF4_NAME_LENGTH + 1];
    char classname[MAX_HDF4_NAME_LENGTH + 1];
    int32 num_fields;
    int32 num_records;
    struct coda_hdf4_Vdata_field_struct **field;
    char **field_name;
    hashtable *hash_data;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vdata;

typedef struct coda_hdf4_Vdata_field_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    char field_name[MAX_HDF4_NAME_LENGTH + 1];
    int32 num_records;
    int32 order;
    int num_elements;
    int32 data_type;
    coda_hdf4_basic_type *basic_type;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vdata_field;

typedef struct coda_hdf4_Vgroup_struct
{
    int retain_count;
    coda_format format;
    coda_type_class type_class;
    char *name;
    char *description;

    hdf4_type_tag tag;
    int32 group_count;  /* number of groups this item belongs to */
    int32 ref;
    int32 vgroup_id;
    int32 hide;
    char vgroup_name[MAX_HDF4_NAME_LENGTH + 1];
    char classname[MAX_HDF4_NAME_LENGTH + 1];
    int32 version;
    int32 num_attributes;
    int32 num_entries;
    struct coda_hdf4_type_struct **entry;
    char **entry_name;
    hashtable *hash_data;
    coda_hdf4_attributes *attributes;
} coda_hdf4_Vgroup;

coda_hdf4_attributes *coda_hdf4_empty_attributes();

struct coda_hdf4_product_struct
{
    /* general fields (shared between all supported product types) */
    char *filename;
    int64_t file_size;
    coda_format format;
    coda_dynamic_type *root_type;
    coda_product_definition *product_definition;
    long *product_variable_size;
    int64_t **product_variable;


    int32 is_hdf;       /* is it a real HDF4 file or are we accessing a (net)CDF file */
    int32 file_id;
    int32 gr_id;
    int32 sd_id;
    int32 an_id;

    int32 num_sd_file_attributes;
    int32 num_gr_file_attributes;

    int32 num_sds;
    struct coda_hdf4_SDS_struct **sds;

    int32 num_images;
    struct coda_hdf4_GRImage_struct **gri;

    int32 num_vgroup;
    struct coda_hdf4_Vgroup_struct **vgroup;

    int32 num_vdata;
    struct coda_hdf4_Vdata_struct **vdata;
};
typedef struct coda_hdf4_product_struct coda_hdf4_product;

#endif
