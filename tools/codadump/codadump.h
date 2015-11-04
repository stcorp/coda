/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#ifndef PDSDUMP_H
#define PDSDUMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_HDF4
#include "mfhdf.h"
#endif

#include "coda.h"
#include "codadump-filter.h"

#define MAX_NUM_DIMS CODA_MAX_NUM_DIMS
#define MAX_DIM_NAME (3 + 3 + CODA_CURSOR_MAXDEPTH * 4 + 3 + 1)

typedef enum run_mode
{
    RUN_MODE_LIST,
    RUN_MODE_ASCII,
    RUN_MODE_HDF4,
    RUN_MODE_DEBUG
} run_mode_t;

extern run_mode_t run_mode;
extern char *ascii_col_sep;
extern FILE *ascii_output;
extern char *output_file_name;
extern int verbosity;
extern int calc_dim;
extern int show_dim_vals;
extern int show_index;
extern int show_label;
extern int show_quotes;
extern int show_time_as_string;
extern int show_type;
extern int show_unit;
extern int show_description;

/* this structure contains the dimension information for a single product variable
 *(i.e. the combined information of all arrays that are a parent to the variable)
 */
typedef struct dim_info
{
    int32_t num_dims;   /* total number of dimensions */
    int32_t dim[MAX_NUM_DIMS];  /* (maximum) dimensions */
    int32_t min_dim[MAX_NUM_DIMS];      /* minimum dimension (only set if the dimension is variable sized) */
    int is_var_size;    /* is any of the dimensions variable sized? */
    int is_var_size_dim[MAX_NUM_DIMS];  /* is a specific dimension variable sized? */
    int last_var_size_dim;      /* last variable sized dimension (-1 if there is no variable sized dimension) */
    int32_t *var_dim[MAX_NUM_DIMS];     /* array pointing to lists of all posible values for variable dimensions */
    int32_t var_dim_num_dims[MAX_NUM_DIMS];     /* allocated size of var_dim[i] is num_elements[var_dim_num_dims[i]] */
    int64_t array_size[MAX_NUM_DIMS];   /* amount of elements needed for all subdimensions (product of max dim sizes, dim_id = k..n) */
    int64_t num_elements[MAX_NUM_DIMS]; /* product of max dim sizes, dim_id = 1..k */
    int64_t filled_num_elements[MAX_NUM_DIMS];  /* total number of elements that are really in the product (dim_id = 1..k) (i.e. by counting instead of multiplying maximum dimensions) */
} dim_info_t;

extern dim_info_t dim_info;

typedef struct array_info
{
    int32_t dim_id;     /* offset of dimensions for this array in dim_info struct */
    int32_t num_dims;   /* number of dimensions of this pds array */
    int32_t dim[MAX_NUM_DIMS];  /* fixed diminsions as returned by data dictionary */
    int32_t num_elements;       /* product of max dim sizes for this array */

    int32_t global_index;       /* cumulative index within all elements of this array */
    int32_t index[MAX_NUM_DIMS];        /* current index in array */
} array_info_t;

typedef struct traverse_info
{
    char *file_name;
    coda_product *pf;

    /* properties regarding position in product */
    coda_type *type[CODA_CURSOR_MAXDEPTH];      /* data type */
    int current_depth;  /* current depth in product */
    coda_cursor cursor; /* cursor positioned at root point */

    /* filter properties */
    codadump_filter *filter[CODA_CURSOR_MAXDEPTH + 1];  /* applicable filter at each cursor depth */
    int filter_depth;

    /* array properties */
    array_info_t array_info[CODA_CURSOR_MAXDEPTH];      /* info for each parent that is an array */
    int num_arrays;     /* number of parents that are arrays */

    /* record properties */
    int field_available_status[CODA_CURSOR_MAXDEPTH];   /* -1 if dynamically available, otherwise 1 */
    int parent_index[CODA_CURSOR_MAXDEPTH];     /* field indices for parents that are records */
    const char *field_name[CODA_CURSOR_MAXDEPTH];       /* field names for parents that are records */
    int num_records;    /* number of parents that are records */
} traverse_info_t;

extern traverse_info_t traverse_info;

#ifdef HAVE_HDF4
typedef struct hdf4_info
{
    int32 hdf_vfile_id;
    int32 hdf_file_id;

    int32 sds_id;
    int32 vgroup_id[CODA_CURSOR_MAXDEPTH];
    int32 vgroup_depth;

    int32 hdf_type;
    int32 sizeof_hdf_type;

    char dim_name[MAX_NUM_DIMS][MAX_DIM_NAME];

    /* properties for writing field data */
    int32 start[MAX_NUM_DIMS];
    int32 edges[MAX_NUM_DIMS];
    int offset;
    unsigned char *data;
} hdf4_info_t;

extern hdf4_info_t hdf4_info;
#endif

/* codadump.c functions */
void handle_coda_error();

/* codadump-ascii.c functions */
void export_data_element_to_ascii();

/* codadump-dim.c functions */
void print_all_distinct_dims(int dim_id);
void dim_info_init();
void dim_info_done();
void dim_enter_array();
void dim_leave_array();
void clear_array_info();
int dim_record_field_available();


/* codadump-hdf4.c functions */
#ifdef HAVE_HDF4
void hdf4_info_init();
void hdf4_info_done();
void hdf4_enter_record();
void hdf4_leave_record();
void hdf4_enter_array();
void hdf4_leave_array();
void export_data_element_to_hdf4();
#endif

/* codadump-debug.c functions */
void print_debug_data();

/* codadump-traverse.c functions */
void print_full_field_name(FILE *f, int print_dims, int compound_as_array);
void traverse_info_init();
void traverse_info_done();
void traverse_product();

#endif
