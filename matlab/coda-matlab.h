/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

#ifndef CODA_MATLAB_H
#define CODA_MATLAB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mex.h"
#include "coda.h"

/* coda-matlab environment declarations */
typedef struct coda_MatlabFileHandle_struct
{
    int handle_id;
    coda_product *pf;
    struct coda_MatlabFileHandle_struct *next;
} coda_MatlabFileHandle;

typedef struct coda_MatlabEnvironment_struct
{
    coda_MatlabFileHandle *handle;
    int option_convert_numbers_to_double;
    int option_filter_record_fields;
    int option_swap_dimensions; /* whether to swap the dimensions of the _data_ */
    int option_use_64bit_integer;
} coda_MatlabEnvironment;

typedef struct coda_MatlabCursorInfo_struct
{
    int intermediate_cursor_flag;
    int argument_index; /* Index of argument containing the vector */
    int variable_index[CODA_MAX_NUM_DIMS];      /* Index vector containing variable indices */
    int num_variable_indices;   /* Number of indices in vector */
} coda_MatlabCursorInfo;

#define CODA_MATLAB_OPTION_CONVERT_NUMBERS_TO_DOUBLE   0
#define CODA_MATLAB_OPTION_FILTER_RECORD_FIELDS        1
#define CODA_MATLAB_OPTION_PERFORM_CONVERSIONS         2
#define CODA_MATLAB_OPTION_SWAP_DIMENSIONS             3
#define CODA_MATLAB_OPTION_USE_64BIT_INTEGER           4
#define CODA_MATLAB_OPTION_USE_MMAP                    5
#define CODA_MATLAB_OPTION_USE_SPECIAL_TYPES           6
#define CODA_MATLAB_NUMBER_OF_OPTIONS                  7
extern const char *coda_matlab_options[];

extern coda_MatlabEnvironment coda_env;

/* coda-traverse functions */
void coda_matlab_traverse_data(int nrhs, const mxArray *prhs[], coda_cursor *cursor, coda_MatlabCursorInfo *info);
void coda_matlab_traverse_product(coda_product *pf, int nrhs, const mxArray *prhs[], coda_cursor *cursor,
                                  coda_MatlabCursorInfo *info);

/* coda-getdata functions */
mxArray *coda_matlab_get_data(coda_product *pf, int nrhs, const mxArray *prhs[]);
mxArray *coda_matlab_read_data(coda_cursor *cursor);

/* coda-matlab functions */
void coda_matlab_coda_error(void);

#endif
