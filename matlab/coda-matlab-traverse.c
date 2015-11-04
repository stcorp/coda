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
    const int *arg_dim;

    mxAssert(arg != NULL, "Arguments array pointer is zero");
    mxAssert(arg_type != NULL, "Pointer to 'type' argument is zero");
    mxAssert(index != NULL, "Pointer to 'index' argument is zero");
    mxAssert(name != NULL, "Pointer to 'name' argument is zero");
    mxAssert(length != NULL, "Pointer to 'lenght' argument is zero");

    *arg_type = -1;

    arg_num_dims = mxGetNumberOfDimensions(arg);
    arg_dim = mxGetDimensions(arg);

    if (mxGetClassID(arg) == mxCHAR_CLASS)
    {
        /* arg contains string */

        if (arg_num_dims == 2 && arg_dim[0] == 1 && arg_dim[1] > 0)
        {
            *length = (arg_dim[0] * arg_dim[1] * sizeof(mxChar)) + 1;
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
        int i;

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

        *length = arg_dim[1];
        *arg_type = 0;
    }
}

void coda_matlab_traverse_data(int nrhs, const mxArray *prhs[], coda_Cursor *cursor, coda_MatlabCursorInfo *info)
{
    int arg_idx;
    long index[CODA_MAX_NUM_DIMS];

    if (info != NULL)
    {
        info->intermediate_cursor_flag = 0;     /* Final cursor unless otherwise specified */
    }

    for (arg_idx = 0; arg_idx < nrhs; arg_idx++)
    {
        coda_type_class type_class;
        char *name;

        name = NULL;

        if (coda_cursor_get_type_class(cursor, &type_class) != 0)
        {
            coda_matlab_coda_error();
        }
        switch (type_class)
        {
            case coda_array_class:
                {
                    int arg_type;
                    int length;
                    int i;

                    coda_matlab_parse_arg(prhs[arg_idx], &arg_type, index, &name, &length);

                    if (arg_type == 1)
                    {
                        /* name */
                        mexPrintf("ERROR: \"%s\" not valid as array index\n", name);
                        mexErrMsgTxt("Error in parameter");
                    }
                    if (arg_type != 0)
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
                        coda_Type *type;
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

                    if (coda_cursor_goto_array_element(cursor, length, index) != 0)
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
                break;
            case coda_record_class:
                {
                    int available_status;
                    int field_index;
                    int arg_type;
                    int length;

                    coda_matlab_parse_arg(prhs[arg_idx], &arg_type, index, &name, &length);

                    if (arg_type != 1)
                    {
                        mexErrMsgTxt("Error in paramater");
                    }
                    if (coda_cursor_get_record_field_index_from_name(cursor, name, index) != 0)
                    {
                        mexPrintf("ERROR: field \"%s\" not found\n", name);
                        mexErrMsgTxt("Error in parameter");
                    }
                    field_index = index[0];
                    if (coda_cursor_get_record_field_available_status(cursor, field_index, &available_status) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    if (available_status == 0)
                    {
                        mexPrintf("ERROR: field \"%s\" not available\n", name);
                        mexErrMsgTxt("Error in parameter");
                    }
                    if (coda_cursor_goto_record_field_by_index(cursor, field_index) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                }
                break;
            default:
                mexErrMsgTxt("Error in paramater");
                break;
        }

        if (name != NULL)
        {
            mxFree(name);
        }
    }
}

void coda_matlab_traverse_product(coda_ProductFile *pf, int nrhs, const mxArray *prhs[], coda_Cursor *cursor,
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
