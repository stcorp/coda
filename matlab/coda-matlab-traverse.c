/*
 * Copyright (C) 2007-2018 S[&]T, The Netherlands.
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

#include "coda-matlab.h"

#include <string.h>

/*
 * Depending on the contents of arg, arg_type is set to
 *   0 : index -> index[] is set to the value in arg and length is set to the number of dimensions
 *   1 : name -> *name is set to the value in arg and length is set to the length of the string
 *  -1 : invalid -> nothing is changed.
 *     arg is considered invalid if it doesn't contain a valid string or
 *     if it doesn't contain a value array of indices.
 *
 *     The user should pass index as an integer array of size = CODA_MAX_N_DIMS.
 *
 *     If *name != NULL then *name contains a string that was allocated with
 *     mxCalloc and the caller of this function should free it with mxFree.
 */
static void coda_matlab_parse_arg(const mxArray *arg, int *arg_type, long *index, char **name, int *length)
{
    int arg_num_dims;
    const mwSize *arg_dim;

    mxAssert(arg != NULL, "Arguments array pointer is zero");
    mxAssert(arg_type != NULL, "Pointer to 'type' argument is zero");
    mxAssert(index != NULL, "Pointer to 'index' argument is zero");
    mxAssert(name != NULL, "Pointer to 'name' argument is zero");
    mxAssert(length != NULL, "Pointer to 'lenght' argument is zero");

    *arg_type = -1;

    arg_num_dims = (int)mxGetNumberOfDimensions(arg);
    arg_dim = mxGetDimensions(arg);

    if (mxGetClassID(arg) == mxCHAR_CLASS)
    {
        /* arg contains string */

        if (arg_num_dims == 2 && arg_dim[0] == 1 && arg_dim[1] > 0)
        {
            *length = (int)(arg_dim[0] * arg_dim[1] * sizeof(mxChar)) + 1;
            *name = mxCalloc(*length, 1);
            if (mxGetString(arg, *name, *length) != 0)
            {
                mexErrMsgTxt("Error copying string");
            }

            *arg_type = 1;
        }
    }
    else if (arg_num_dims == 2 && arg_dim[0] == 1 && arg_dim[1] > 0 && arg_dim[1] <= CODA_MAX_NUM_DIMS)
    {
        void *data;
        mwSize i;

        /* arg contains value array */

        data = mxGetData(arg);

        for (i = 0; i < arg_dim[1]; i++)
        {
            switch (mxGetClassID(arg))
            {
                case mxDOUBLE_CLASS:
                    index[i] = (int)((double *)data)[i];
                    break;
                case mxINT32_CLASS:
                    index[i] = (int)((int32_t *)data)[i];
                    break;
                default:
                    mexErrMsgTxt("index parameter not of type double or int32");
                    break;
            }
        }

        *length = (int)arg_dim[1];
        *arg_type = 0;
    }
}

void coda_matlab_traverse_data(int nrhs, const mxArray *prhs[], coda_cursor *cursor, coda_MatlabCursorInfo *info)
{
    int arg_idx;
    long index[CODA_MAX_NUM_DIMS];

    if (info != NULL)
    {
        info->intermediate_cursor_flag = 0;     /* Final cursor unless otherwise specified */
    }

    for (arg_idx = 0; arg_idx < nrhs; arg_idx++)
    {
        char *name = NULL;
        int arg_type;
        int length;

        coda_matlab_parse_arg(prhs[arg_idx], &arg_type, index, &name, &length);

        if (arg_type == 0)
        {
            coda_type_class type_class;
            long local_index[CODA_MAX_NUM_DIMS];
            int i;

            if (coda_cursor_get_type_class(cursor, &type_class) != 0)
            {
                coda_matlab_coda_error();
            }

            if (type_class != coda_array_class)
            {
                mexErrMsgTxt("Error in paramater");
            }

            for (i = 0; i < length; i++)
            {
                if (index[i] != -1)
                {
                    index[i]--;
                }
                else if (info != NULL)
                {
                    info->intermediate_cursor_flag = 1;
                }
            }

            if ((info != NULL) && (info->intermediate_cursor_flag))
            {
                info->argument_index = arg_idx;
                info->num_variable_indices = length;
                for (i = 0; i < length; i++)
                {
                    info->variable_index[i] = index[i];
                }
                return; /* Return intermediate cursor */
            }

            if (length == 1 && index[0] == 0)
            {
                coda_type *type;
                int num_dims;

                /* convert to zero dimensional index if needed */
                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    coda_matlab_coda_error();
                }
                if (coda_type_get_array_num_dims(type, &num_dims) != 0)
                {
                    coda_matlab_coda_error();
                }
                if (num_dims == 0)
                {
                    length = 0;
                }
            }

            for (i = 0; i < length; i++)
            {
                local_index[i] = coda_env.option_swap_dimensions ? index[i] : index[length - i + 1];
            }
            if (coda_cursor_goto_array_element(cursor, length, local_index) != 0)
            {
                if (coda_errno == CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH)
                {
                    mexPrintf("ERROR: array dimensions mismatch\n");
                    mexErrMsgTxt("Error in parameter");
                }
                if (coda_errno == CODA_ERROR_ARRAY_OUT_OF_BOUNDS)
                {
                    mexPrintf("ERROR: array index out of bounds\n");
                    mexErrMsgTxt("Error in parameter");
                }
                coda_matlab_coda_error();
            }

        }
        else if (arg_type == 1)
        {
            if (coda_cursor_goto(cursor, name) != 0)
            {
                coda_matlab_coda_error();
            }
        }
        else
        {
            mexErrMsgTxt("Error in paramater");
        }

        if (name != NULL)
        {
            mxFree(name);
        }
    }
}

void coda_matlab_traverse_product(coda_product *pf, int nrhs, const mxArray *prhs[], coda_cursor *cursor,
                                  coda_MatlabCursorInfo *info)
{
    mxAssert(pf != NULL, "Productfile pointer is zero");
    mxAssert(cursor != NULL, "Coda Cursor pointer is zero");

    if (coda_cursor_set_product(cursor, pf) != 0)
    {
        coda_matlab_coda_error();
    }

    coda_matlab_traverse_data(nrhs, prhs, cursor, info);
}
