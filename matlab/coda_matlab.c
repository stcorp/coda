/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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

#include <stdlib.h>
#include <string.h>

#define MAX_FUNCNAME_LENGTH     50

/* set default values for: Handle, ConvertNumbersToDouble, FilterRecordFields, SwapDimensions, and Use64bitInteger */
coda_MatlabEnvironment coda_env = { NULL, 1, 1, 1, 0 };

const char *coda_matlab_options[] = {
    "ConvertNumbersToDouble",
    "FilterRecordFields",
    "PerformConversions",
    "SwapDimensions",
    "Use64bitInteger",
    "UseMMap",
    "UseSpecialTypes"
};

static int coda_matlab_initialised = 0;

static mxArray *coda_matlab_add_file_handle(coda_product *pf);
static void coda_matlab_remove_file_handle(const mxArray *mx_handle);
static coda_product *coda_matlab_get_product_file(const mxArray *mx_handle);
static void coda_matlab_cleanup(void);

static void coda_matlab_attributes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_class(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_clearall(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_close(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_description(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_eval(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_fetch(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_fieldavailable(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_fieldcount(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_fieldnames(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_getopt(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_open(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_open_as(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_product_class(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_product_type(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_product_version(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_setopt(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_size(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_time_to_string(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_unit(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
static void coda_matlab_version(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

void coda_matlab_coda_error(void)
{
    mexPrintf("ERROR : %s\n", coda_errno_to_string(coda_errno));
    mexErrMsgTxt("CODA Error");
}

static mxArray *coda_matlab_add_file_handle(coda_product *pf)
{
    coda_MatlabFileHandle **handle_ptr;
    coda_MatlabFileHandle *handle;
    int handle_id;

    handle_id = 1;
    handle_ptr = &coda_env.handle;
    while (*handle_ptr != NULL && (*handle_ptr)->handle_id == handle_id)
    {
        handle_ptr = &(*handle_ptr)->next;
        handle_id++;
    }

    handle = mxCalloc(1, sizeof(coda_MatlabFileHandle));
    mexMakeMemoryPersistent(handle);
    handle->handle_id = handle_id;
    handle->pf = pf;
    handle->next = *handle_ptr;
    *handle_ptr = handle;

    /* return new file handle as mxArray */
    return mxCreateDoubleScalar((double)handle_id);
}

static void coda_matlab_remove_file_handle(const mxArray *mx_handle)
{
    coda_MatlabFileHandle **handle_ptr;
    int handle_id;

    handle_id = (int)mxGetScalar(mx_handle);
    handle_ptr = &coda_env.handle;
    while (*handle_ptr != NULL)
    {
        if ((*handle_ptr)->handle_id == handle_id)
        {
            coda_MatlabFileHandle *handle;

            handle = *handle_ptr;
            *handle_ptr = handle->next;
            coda_close(handle->pf);
            mxFree(handle);
            return;
        }
        handle_ptr = &(*handle_ptr)->next;
    }

    mexErrMsgTxt("Not a valid file handle - no file associated with this file handle");
}

static coda_product *coda_matlab_get_product_file(const mxArray *mx_handle)
{
    coda_MatlabFileHandle *handle;
    int handle_id;

    handle_id = (int)mxGetScalar(mx_handle);
    handle = coda_env.handle;
    while (handle != NULL)
    {
        if (handle->handle_id == handle_id)
        {
            return handle->pf;
        }
        handle = handle->next;
    }

    mexErrMsgTxt("Not a valid file handle - no file associated with this file handle");

    return NULL;
}

static void coda_matlab_cleanup(void)
{
    /* close all open files */
    while (coda_env.handle != NULL)
    {
        coda_MatlabFileHandle *handle;

        handle = coda_env.handle;
        coda_env.handle = handle->next;
        coda_close(handle->pf);
        mxFree(handle);
    }

    /* destroy data dictionary */
    if (coda_matlab_initialised)
    {
        coda_done();
        coda_matlab_initialised = 0;
    }
}

static void coda_matlab_set_definition_path(void)
{
    if (getenv("CODA_DEFINITION") == NULL)
    {
        mxArray *mxpath;
        mxArray *arg;
        char *path;
        int path_length;

        arg = mxCreateString("coda_version");
        if (mexCallMATLAB(1, &mxpath, 1, &arg, "which") != 0)
        {
            mexErrMsgTxt("Could not retrieve module path");
        }
        mxDestroyArray(arg);

        path_length = (int)(mxGetN(mxpath) * mxGetM(mxpath) + 1);
        path = mxCalloc(path_length + 1, 1);
        if (mxGetString(mxpath, path, path_length + 1) != 0)
        {
            mexErrMsgTxt("Error copying string");
        }
        /* remove 'coda_version.m' from path */
        if (path_length > 14)
        {
            path[path_length - 14 - 1] = '\0';
        }
        mxDestroyArray(mxpath);
        coda_set_definition_path_conditional("coda_version.m", path, "../../../share/" PACKAGE "/definitions");
        mxFree(path);
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char funcname[MAX_FUNCNAME_LENGTH + 1];

    if (!coda_matlab_initialised)
    {
        coda_matlab_set_definition_path();

        if (coda_init() != 0)
        {
            coda_matlab_coda_error();
        }
        /* turn of boundary checking in libcoda for increased performance */
        coda_set_option_perform_boundary_checks(0);
        coda_matlab_initialised = 1;
        mexAtExit(&coda_matlab_cleanup);
    }

    /* check parameters */
    if (!(nrhs >= 1 && mxIsChar(prhs[0]) && mxGetM(prhs[0]) == 1 && mxGetN(prhs[0]) <= MAX_FUNCNAME_LENGTH))
    {
        mexErrMsgTxt("Incorrect invocation of CODA-MATLAB gateway function.");
    }

    if (mxGetString(prhs[0], funcname, MAX_FUNCNAME_LENGTH + 1) != 0)
    {
        mexErrMsgTxt("Error in CODA-MATLAB gateway function: Could not copy string.");
    }

    if (strcmp(funcname, "ATTRIBUTES") == 0)
    {
        coda_matlab_attributes(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "CLASS") == 0)
    {
        coda_matlab_class(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "CLEARALL") == 0)
    {
        coda_matlab_clearall(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "CLOSE") == 0)
    {
        coda_matlab_close(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "DESCRIPTION") == 0)
    {
        coda_matlab_description(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "EVAL") == 0)
    {
        coda_matlab_eval(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "FETCH") == 0)
    {
        coda_matlab_fetch(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "FIELDAVAILABLE") == 0)
    {
        coda_matlab_fieldavailable(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "FIELDCOUNT") == 0)
    {
        coda_matlab_fieldcount(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "FIELDNAMES") == 0)
    {
        coda_matlab_fieldnames(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "GETOPT") == 0)
    {
        coda_matlab_getopt(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "OPEN") == 0)
    {
        coda_matlab_open(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "OPEN_AS") == 0)
    {
        coda_matlab_open_as(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "PRODUCT_CLASS") == 0)
    {
        coda_matlab_product_class(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "PRODUCT_TYPE") == 0)
    {
        coda_matlab_product_type(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "PRODUCT_VERSION") == 0)
    {
        coda_matlab_product_version(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "SETOPT") == 0)
    {
        coda_matlab_setopt(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "SIZE") == 0)
    {
        coda_matlab_size(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "TIME_TO_STRING") == 0)
    {
        coda_matlab_time_to_string(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "UNIT") == 0)
    {
        coda_matlab_unit(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else if (strcmp(funcname, "VERSION") == 0)
    {
        coda_matlab_version(nlhs, plhs, nrhs - 1, &(prhs[1]));
    }
    else
    {
        mexErrMsgTxt("Error in CODA-MATLAB gateway function: Unknown function name.");
    }
}

static void coda_matlab_attributes(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    coda_cursor cursor;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_goto_attributes(&cursor) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = coda_matlab_read_data(&cursor);

    if (plhs[0] == NULL)
    {
        /* return empty array instead of NULL pointer */
        plhs[0] = mxCreateNumericArray(0, NULL, mxDOUBLE_CLASS, mxREAL);
    }
}

static void coda_matlab_class(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    coda_cursor cursor;
    coda_type_class type_class;
    coda_type *type;
    int is_array = 0;
    char *class = "";

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type(&cursor, &type) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_type_get_class(type, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }

    if (type_class == coda_array_class)
    {
        coda_type *base_type;

        /* return class for base type of array */

        if (coda_type_get_array_base_type(type, &base_type) != 0)
        {
            coda_matlab_coda_error();
        }
        type = base_type;
        if (coda_type_get_class(type, &type_class) != 0)
        {
            coda_matlab_coda_error();
        }
        if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
        {
            if (coda_type_get_special_base_type(type, &type) != 0)
            {
                coda_matlab_coda_error();
            }
            if (coda_type_get_class(type, &type_class) != 0)
            {
                coda_matlab_coda_error();
            }
        }
        is_array = 1;
    }

    switch (type_class)
    {
        case coda_array_class:
            class = "cell";
            break;
        case coda_record_class:
            class = "struct";
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(type, &read_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "int8";
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "uint8";
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "int16";
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "uint16";
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "int32";
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "uint32";
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "int64";
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "uint64";
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_env.option_convert_numbers_to_double)
                        {
                            class = "double";
                        }
                        else
                        {
                            class = "single";
                        }
                        break;
                    case coda_native_type_double:
                        class = "double";
                        break;
                    case coda_native_type_char:
                        class = "char";
                        break;
                    case coda_native_type_string:
                        if (is_array)
                        {
                            class = "cell";
                        }
                        else
                        {
                            class = "char";
                        }
                        break;
                    case coda_native_type_bytes:
                        class = "uint8";
                        break;
                    case coda_native_type_not_available:
                        mxAssert(0, "Cannot read data of this type");
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(type, &special_type) != 0)
                {
                    coda_matlab_coda_error();
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                    case coda_special_complex:
                        class = "double";
                        break;
                    case coda_special_no_data:
                        /* fetching an empty mx_data results in a zero length double array */
                        class = "double";
                        break;
                }
            }
            break;
    }

    plhs[0] = mxCreateString(class);
}

static void coda_matlab_clearall(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    (void)plhs; /* prevents 'unused parameter' warning */
    (void)prhs; /* prevents 'unused parameter' warning */

    /* check parameters */
    if (nlhs > 0)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 0)
    {
        mexErrMsgTxt("Function takes no arguments.");
    }

    coda_matlab_cleanup();
}

static void coda_matlab_close(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    (void)plhs; /* prevents 'unused parameter' warning */

    /* check parameters */
    if (nlhs > 0)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }
    if (!mxIsDouble(prhs[0]) || mxGetN(prhs[0]) != 1 || mxGetM(prhs[0]) != 1)
    {
        mexErrMsgTxt("Not a valid file handle");
    }

    coda_matlab_remove_file_handle(prhs[0]);
}

static void coda_matlab_description(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    coda_cursor cursor;
    coda_type *type;
    const char *description;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type(&cursor, &type) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_type_get_description(type, &description) != 0)
    {
        coda_matlab_coda_error();
    }
    if (description == NULL)
    {
        description = "";
    }
    plhs[0] = mxCreateString(description);
}

static void coda_matlab_eval(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_expression_type type;
    coda_cursor cursor;
    coda_cursor *expr_cursor = NULL;
    coda_expression *expr;
    char *exprstring;
    int buflen;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    if (!mxIsChar(prhs[0]))
    {
        mexErrMsgTxt("First argument should be a string.");
    }
    if (mxGetM(prhs[0]) != 1)
    {
        mexErrMsgTxt("First argument should be a row vector.");
    }

    buflen = (int)mxGetN(prhs[0]) + 1;
    exprstring = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(prhs[0], exprstring, buflen) != 0)
    {
        mexErrMsgTxt("Unable to copy the expression string.");
    }
    if (coda_expression_from_string(exprstring, &expr) != 0)
    {
        coda_matlab_coda_error();
    }
    mxFree(exprstring);
    if (coda_expression_get_type(expr, &type) != 0)
    {
        coda_expression_delete(expr);
        coda_matlab_coda_error();
    }

    if (nrhs > 1)
    {
        coda_product *pf;

        pf = coda_matlab_get_product_file(prhs[1]);
        coda_matlab_traverse_product(pf, nrhs - 2, &prhs[2], &cursor, NULL);
        expr_cursor = &cursor;
    }
    else if (!coda_expression_is_constant(expr))
    {
        coda_expression_delete(expr);
        mexErrMsgTxt("Product location is required if expression is not a constant expression");
    }

    switch (type)
    {
        case coda_expression_boolean:
            {
                int value;

                if (coda_expression_eval_bool(expr, expr_cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    coda_matlab_coda_error();
                }
                if (coda_env.option_convert_numbers_to_double)
                {
                    plhs[0] = mxCreateDoubleScalar(value);
                }
                else
                {
                    plhs[0] = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
                    ((int32_t *)mxGetData(plhs[0]))[0] = value;
                }
            }
            break;
        case coda_expression_integer:
            {
                int64_t value;

                if (coda_expression_eval_integer(expr, expr_cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    coda_matlab_coda_error();
                }
                if (coda_env.option_convert_numbers_to_double || !coda_env.option_use_64bit_integer)
                {
                    plhs[0] = mxCreateDoubleScalar((double)value);
                }
                else
                {
                    plhs[0] = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
                    ((int64_t *)mxGetData(plhs[0]))[0] = value;
                }
            }
            break;
        case coda_expression_float:
            {
                double value;

                if (coda_expression_eval_float(expr, expr_cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    coda_matlab_coda_error();
                }
                plhs[0] = mxCreateDoubleScalar(value);
            }
            break;
        case coda_expression_string:
            {
                char *value;
                long length;

                if (coda_expression_eval_string(expr, expr_cursor, &value, &length) != 0)
                {
                    coda_expression_delete(expr);
                    coda_matlab_coda_error();
                }
                plhs[0] = mxCreateString(value != NULL ? value : "");
            }
            break;
        case coda_expression_node:
            coda_expression_delete(expr);
            mexErrMsgTxt("Evaluation of void expressions not supported");
            break;
        case coda_expression_void:
            coda_expression_delete(expr);
            mexErrMsgTxt("Evaluation of void expressions not supported");
    }
}

static void coda_matlab_fetch(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    plhs[0] = coda_matlab_get_data(pf, nrhs - 1, &prhs[1]);
}

static void coda_matlab_fieldavailable(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_type_class type_class;
    coda_product *pf;
    coda_cursor cursor;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 2)
    {
        mexErrMsgTxt("Function needs at least two arguments.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    /* we move the datahandle to the record and parse the final fieldname argument ourselves */
    coda_matlab_traverse_product(pf, nrhs - 2, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type_class(&cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    if (type_class == coda_record_class)
    {
        int arg_num_dims;
        const mwSize *arg_dim;
        char *fieldname;
        mwSize length;
        long field_index;
        int available;

        arg_num_dims = (int)mxGetNumberOfDimensions(prhs[nrhs - 1]);
        arg_dim = mxGetDimensions(prhs[nrhs - 1]);
        if (mxGetClassID(prhs[nrhs - 1]) != mxCHAR_CLASS || !(arg_num_dims == 2 && arg_dim[0] == 1 && arg_dim[1] > 0))
        {
            mexErrMsgTxt("Error in paramater");
        }
        length = (arg_dim[0] * arg_dim[1] * sizeof(mxChar)) + 1;
        fieldname = mxCalloc(length, 1);
        if (mxGetString(prhs[nrhs - 1], fieldname, length) != 0)
        {
            mexErrMsgTxt("Error copying string");
        }
        if (coda_cursor_get_record_field_index_from_name(&cursor, fieldname, &field_index) != 0)
        {
            coda_matlab_coda_error();
        }
        mxFree(fieldname);
        if (coda_cursor_get_record_field_available_status(&cursor, field_index, &available) != 0)
        {
            coda_matlab_coda_error();
        }
        plhs[0] = mxCreateDoubleScalar(available);
    }
    else
    {
        mexErrMsgTxt("Not a record");
    }
}

static void coda_matlab_fieldcount(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_type_class type_class;
    coda_product *pf;
    coda_cursor cursor;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type_class(&cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    if (type_class == coda_record_class)
    {
        coda_type *record_type;
        long field_index;
        long num_fields;
        int mx_num_fields = 0;

        if (coda_cursor_get_type(&cursor, &record_type) != 0)
        {
            coda_matlab_coda_error();
        }
        if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
        {
            coda_matlab_coda_error();
        }
        for (field_index = 0; field_index < num_fields; field_index++)
        {
            int available;

            if (coda_cursor_get_record_field_available_status(&cursor, field_index, &available) != 0)
            {
                coda_matlab_coda_error();
            }
            if (available)
            {
                if (coda_env.option_filter_record_fields)
                {
                    int hidden;

                    if (coda_type_get_record_field_hidden_status(record_type, field_index, &hidden) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    if (!hidden)
                    {
                        mx_num_fields++;
                    }
                }
                else
                {
                    mx_num_fields++;
                }
            }
        }
        plhs[0] = mxCreateDoubleScalar(mx_num_fields);
    }
    else
    {
        mexErrMsgTxt("Not a record");
    }
}

static void coda_matlab_fieldnames(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_type_class type_class;
    coda_product *pf;
    coda_cursor cursor;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type_class(&cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    if (type_class == coda_record_class)
    {
        const char **field_name;
        int mx_num_fields = 0;
        coda_type *record_type;
        long field_index;
        long num_fields;

        if (coda_cursor_get_type(&cursor, &record_type) != 0)
        {
            coda_matlab_coda_error();
        }
        if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
        {
            coda_matlab_coda_error();
        }
        field_name = mxCalloc(num_fields, sizeof(*field_name));
        for (field_index = 0; field_index < num_fields; field_index++)
        {
            int available;

            if (coda_cursor_get_record_field_available_status(&cursor, field_index, &available) != 0)
            {
                coda_matlab_coda_error();
            }
            if (available)
            {
                if (coda_env.option_filter_record_fields)
                {
                    int hidden;

                    if (coda_type_get_record_field_hidden_status(record_type, field_index, &hidden) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    if (!hidden)
                    {
                        if (coda_type_get_record_field_name(record_type, field_index, &field_name[mx_num_fields]) != 0)
                        {
                            coda_matlab_coda_error();
                        }
                        mx_num_fields++;
                    }
                }
                else
                {
                    if (coda_type_get_record_field_name(record_type, field_index, &field_name[mx_num_fields]) != 0)
                    {
                        coda_matlab_coda_error();
                    }
                    mx_num_fields++;
                }
            }
        }
        plhs[0] = mxCreateCellMatrix(mx_num_fields, 1);
        for (field_index = 0; field_index < mx_num_fields; field_index++)
        {
            mxSetCell(plhs[0], field_index, mxCreateString(field_name[field_index]));
        }
        mxFree((char **)field_name);
    }
    else
    {
        mexErrMsgTxt("Not a record");
    }
}

static void coda_matlab_getopt(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs > 1)
    {
        mexErrMsgTxt("Too many input arguments.");
    }

    if (nrhs == 0)
    {
        /* Get full option list */

        /* Return struct with options */
        plhs[0] = mxCreateStructMatrix(1, 1, CODA_MATLAB_NUMBER_OF_OPTIONS, coda_matlab_options);
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_CONVERT_NUMBERS_TO_DOUBLE],
                   mxCreateDoubleScalar(coda_env.option_convert_numbers_to_double ? 1 : 0));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_FILTER_RECORD_FIELDS],
                   mxCreateDoubleScalar(coda_env.option_filter_record_fields ? 1 : 0));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_PERFORM_CONVERSIONS],
                   mxCreateDoubleScalar(coda_get_option_perform_conversions()));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_SWAP_DIMENSIONS],
                   mxCreateDoubleScalar(coda_env.option_swap_dimensions ? 1 : 0));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_USE_64BIT_INTEGER],
                   mxCreateDoubleScalar(coda_env.option_use_64bit_integer ? 1 : 0));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_USE_MMAP],
                   mxCreateDoubleScalar(coda_get_option_use_mmap()));
        mxSetField(plhs[0], 0, coda_matlab_options[CODA_MATLAB_OPTION_USE_SPECIAL_TYPES],
                   mxCreateDoubleScalar(!coda_get_option_bypass_special_types()));
    }
    else
    {
        mwSize length;
        char *name;
        const mwSize *prhs_dim;

        /* Get value of certain option */

        prhs_dim = mxGetDimensions(prhs[0]);
        if (!(mxGetClassID(prhs[0]) == mxCHAR_CLASS) && (mxGetNumberOfDimensions(prhs[0]) == 2) &&
            (prhs_dim[0] == 1) && (prhs_dim[1] > 0))
        {
            mexErrMsgTxt("Not a valid option name.");
        }
        length = (prhs_dim[0] * prhs_dim[1] * sizeof(mxChar)) + 1;
        name = mxCalloc(length, 1);
        if (mxGetString(prhs[0], name, length) != 0)
        {
            mexErrMsgTxt("Error copying string");
        }
        if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_CONVERT_NUMBERS_TO_DOUBLE]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_env.option_convert_numbers_to_double ? 1 : 0);
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_FILTER_RECORD_FIELDS]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_env.option_filter_record_fields ? 1 : 0);
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_PERFORM_CONVERSIONS]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_get_option_perform_conversions());
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_SWAP_DIMENSIONS]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_env.option_swap_dimensions ? 1 : 0);
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_64BIT_INTEGER]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_env.option_use_64bit_integer ? 1 : 0);
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_MMAP]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(coda_get_option_use_mmap());
        }
        else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_SPECIAL_TYPES]) == 0)
        {
            plhs[0] = mxCreateDoubleScalar(!coda_get_option_bypass_special_types());
        }
        else
        {
            mexErrMsgTxt("Unknown option");
        }
        mxFree(name);
    }
}

static void coda_matlab_open(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    char *filename;
    int buflen;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }
    if (!mxIsChar(prhs[0]))
    {
        mexErrMsgTxt("First argument should be a string.");
    }
    if (mxGetM(prhs[0]) != 1)
    {
        mexErrMsgTxt("First argument should be a row vector.");
    }

    buflen = (int)mxGetN(prhs[0]) + 1;
    filename = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(prhs[0], filename, buflen) != 0)
    {
        mexErrMsgTxt("Unable to copy the filename string.");
    }

    if (coda_open(filename, &pf) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = coda_matlab_add_file_handle(pf);

    mxFree(filename);
}

static void coda_matlab_open_as(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    char *filename;
    char *product_class;
    char *product_type;
    int version;
    int buflen;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 4)
    {
        mexErrMsgTxt("Function needs exactly four arguments.");
    }
    if (!mxIsChar(prhs[0]))
    {
        mexErrMsgTxt("First argument should be a string.");
    }
    if (mxGetM(prhs[0]) != 1)
    {
        mexErrMsgTxt("First argument should be a row vector.");
    }
    if (!mxIsChar(prhs[1]))
    {
        mexErrMsgTxt("Second argument should be a string.");
    }
    if (mxGetM(prhs[1]) != 1)
    {
        mexErrMsgTxt("Second argument should be a row vector.");
    }
    if (!mxIsChar(prhs[2]))
    {
        mexErrMsgTxt("Third argument should be a string.");
    }
    if (mxGetM(prhs[2]) != 1)
    {
        mexErrMsgTxt("Third argument should be a row vector.");
    }
    if (!mxIsNumeric(prhs[3]))
    {
        mexErrMsgTxt("Fourth argument should be a numerical value.");
    }

    buflen = (int)mxGetN(prhs[0]) + 1;
    filename = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(prhs[0], filename, buflen) != 0)
    {
        mexErrMsgTxt("Unable to copy the filename string.");
    }
    buflen = (int)mxGetN(prhs[1]) + 1;
    product_class = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(prhs[1], product_class, buflen) != 0)
    {
        mexErrMsgTxt("Unable to copy the product_class string.");
    }
    buflen = (int)mxGetN(prhs[2]) + 1;
    product_type = (char *)mxCalloc(buflen, sizeof(char));
    if (mxGetString(prhs[2], product_type, buflen) != 0)
    {
        mexErrMsgTxt("Unable to copy the product_type string.");
    }
    version = (int)mxGetScalar(prhs[3]);

    if (coda_open_as(filename, product_class, product_type, version, &pf) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = coda_matlab_add_file_handle(pf);

    mxFree(product_type);
    mxFree(product_class);
    mxFree(filename);
}

static void coda_matlab_product_class(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    const char *product_class;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    if (coda_get_product_class(pf, &product_class) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = mxCreateString(product_class);
}

static void coda_matlab_product_type(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    const char *product_type;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    if (coda_get_product_type(pf, &product_type) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = mxCreateString(product_type);
}

static void coda_matlab_product_version(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    int product_version;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    if (coda_get_product_version(pf, &product_version) != 0)
    {
        coda_matlab_coda_error();
    }

    plhs[0] = mxCreateDoubleScalar(product_version);
}

static void coda_matlab_setopt(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    const mwSize *prhs_dim;
    char *name;
    mwSize length;

    (void)plhs; /* prevents 'unused parameter' warning */

    /* check parameters */
    if (nlhs > 0)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 2)
    {
        mexErrMsgTxt("Function needs exactly two arguments.");
    }

    /* Set value of specified option */

    prhs_dim = mxGetDimensions(prhs[0]);
    if (!(mxGetClassID(prhs[0]) == mxCHAR_CLASS) && (mxGetNumberOfDimensions(prhs[0]) == 2) &&
        (prhs_dim[0] == 1) && (prhs_dim[1] > 0))
    {
        mexErrMsgTxt("Not a valid option name.");
    }
    length = (prhs_dim[0] * prhs_dim[1] * sizeof(mxChar)) + 1;
    name = mxCalloc(length, 1);
    if (mxGetString(prhs[0], name, length) != 0)
    {
        mexErrMsgTxt("Error copying string");
    }
    if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_CONVERT_NUMBERS_TO_DOUBLE]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        coda_env.option_convert_numbers_to_double = value;
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_FILTER_RECORD_FIELDS]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        coda_env.option_filter_record_fields = value;
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_PERFORM_CONVERSIONS]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        if (coda_set_option_perform_conversions(value) != 0)
        {
            coda_matlab_coda_error();
        }
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_SWAP_DIMENSIONS]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        coda_env.option_swap_dimensions = value;
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_64BIT_INTEGER]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        coda_env.option_use_64bit_integer = value;
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_MMAP]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        if (coda_set_option_use_mmap(value) != 0)
        {
            coda_matlab_coda_error();
        }
    }
    else if (strcmp(name, coda_matlab_options[CODA_MATLAB_OPTION_USE_SPECIAL_TYPES]) == 0)
    {
        int value = (int)mxGetScalar(prhs[1]);

        if (!(value == 0 || value == 1))
        {
            mexErrMsgTxt("Incorrect value for this option");
        }
        if (coda_set_option_bypass_special_types(!value) != 0)
        {
            coda_matlab_coda_error();
        }
    }
    else
    {
        mexErrMsgTxt("Unknown option");
    }
    mxFree(name);
}

static void coda_matlab_size(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    coda_cursor cursor;
    coda_type_class type_class;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type_class(&cursor, &type_class) != 0)
    {
        coda_matlab_coda_error();
    }
    if (type_class == coda_array_class)
    {
        int num_dims;
        long dim[CODA_MAX_NUM_DIMS];

        if (coda_cursor_get_array_dim(&cursor, &num_dims, dim) != 0)
        {
            coda_matlab_coda_error();
        }
        if (num_dims == 0)
        {
            plhs[0] = mxCreateDoubleScalar(1);
        }
        else
        {
            double *data;
            int i;

            plhs[0] = mxCreateNumericMatrix(1, num_dims, mxDOUBLE_CLASS, mxREAL);
            data = mxGetData(plhs[0]);
            for (i = 0; i < num_dims; i++)
            {
                if (coda_env.option_swap_dimensions)
                {
                    data[i] = (double)dim[i];
                }
                else
                {
                    data[num_dims - i - 1] = (double)dim[i];
                }
            }
        }
    }
    else
    {
        mexErrMsgTxt("Not an array");
    }
}

static void coda_matlab_time_to_string(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char str[27];
    int num_elements;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 1)
    {
        mexErrMsgTxt("Function needs exactly one argument.");
    }
    if (!mxIsDouble(prhs[0]))
    {
        mexErrMsgTxt("First argument should be a double.");
    }

    num_elements = (int)mxGetNumberOfElements(prhs[0]);
    if (num_elements == 1)
    {
        if (coda_time_double_to_string(mxGetScalar(prhs[0]), "yyyy-MM-dd HH:mm:ss.SSSSSS", str) != 0)
        {
            coda_matlab_coda_error();
        }
        plhs[0] = mxCreateString(str);
    }
    else
    {
        double *time_value;
        int num_dims;
        const mwSize *dim;
        int i;

        num_dims = (int)mxGetNumberOfDimensions(prhs[0]);
        dim = mxGetDimensions(prhs[0]);
        time_value = (double *)mxGetData(prhs[0]);

        plhs[0] = mxCreateCellArray(num_dims, dim);

        for (i = 0; i < num_elements; i++)
        {
            if (coda_time_double_to_string(time_value[i], "yyyy-MM-dd HH:mm:ss.SSSSSS", str) != 0)
            {
                coda_matlab_coda_error();
            }
            mxSetCell(plhs[0], i, mxCreateString(str));
        }
    }
}

static void coda_matlab_unit(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    coda_product *pf;
    coda_cursor cursor;
    coda_type *type;
    const char *unit;

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs < 1)
    {
        mexErrMsgTxt("Function needs at least one argument.");
    }

    pf = coda_matlab_get_product_file(prhs[0]);

    coda_matlab_traverse_product(pf, nrhs - 1, &prhs[1], &cursor, NULL);

    if (coda_cursor_get_type(&cursor, &type) != 0)
    {
        coda_matlab_coda_error();
    }
    if (coda_type_get_unit(type, &unit) != 0)
    {
        coda_matlab_coda_error();
    }
    if (unit == NULL)
    {
        unit = "";
    }
    plhs[0] = mxCreateString(unit);
}

static void coda_matlab_version(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    (void)prhs; /* prevents 'unused parameter' warning */

    /* check parameters */
    if (nlhs > 1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (nrhs != 0)
    {
        mexErrMsgTxt("Function takes no arguments.");
    }

    plhs[0] = mxCreateString(coda_get_libcoda_version());
}
