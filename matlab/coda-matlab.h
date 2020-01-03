/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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
