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

#include "coda-internal.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif

#define MAX_ERROR_INFO_LENGTH	4096

static THREAD_LOCAL char coda_error_message_buffer[MAX_ERROR_INFO_LENGTH + 1];

/** \defgroup coda_error CODA Error
 * With a few exceptions almost all CODA functions return an integer that indicate whether the function was able to
 * perform its operations successfully. The return value will be 0 on success and -1 otherwise. In case you get a -1
 * you can look at the global variable #coda_errno for a precise error code. Each error code and its meaning is
 * described in this section. You will also be able to retrieve a character string with an error description via
 * the coda_errno_to_string() function. This function will return either the default error message for the error
 * code, or a custom error message. A custom error message will only be returned if the error code you pass to
 * coda_errno_to_string() is equal to the last error that occurred and if this last error was set with a custom error
 * message. The CODA error state can be set with the coda_set_error() function.<br>
 */

/** \addtogroup coda_error
 * @{
 */

/** \name Error values
 * \ingroup coda_error
 * \note Error values in the range -900..-999 are reserved for use by layers built on top of the CODA library.
 * @{
 */

/** \def CODA_SUCCESS
 * Success (no error).
 */
/** \def CODA_ERROR_OUT_OF_MEMORY
 * Out of memory.
 */
/** \def CODA_ERROR_HDF4
 * An error occurred in the HDF4 library.
 */
/** \def CODA_ERROR_NO_HDF4_SUPPORT
 * No HDF4 support built into CODA.
 */
/** \def CODA_ERROR_HDF5
 * An error occurred in the HDF5 library.
 */
/** \def CODA_ERROR_NO_HDF5_SUPPORT
 * No HDF5 support built into CODA.
 */
/** \def CODA_ERROR_XML
 * An error occurred while parsing an XML data block.
 */

/** \def CODA_ERROR_FILE_NOT_FOUND
 * File not found.
 */
/** \def CODA_ERROR_FILE_OPEN
 * Could not open file.
 */
/** \def CODA_ERROR_FILE_READ
 * Could not read data from file.
 */
/** \def CODA_ERROR_FILE_WRITE
 * Could not write data to file.
 */

/** \def CODA_ERROR_INVALID_ARGUMENT
 * Invalid argument.
 */
/** \def CODA_ERROR_INVALID_INDEX
 * Invalid index argument.
 */
/** \def CODA_ERROR_INVALID_NAME
 * Invalid name argument.
 */
/** \def CODA_ERROR_INVALID_FORMAT
 * Invalid format in argument.
 */
/** \def CODA_ERROR_INVALID_DATETIME
 * Invalid date/time argument.
 */
/** \def CODA_ERROR_INVALID_TYPE
 * Invalid type.
 */
/** \def CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH
 * Incorrect number of dimensions argument.
 */
/** \def CODA_ERROR_ARRAY_OUT_OF_BOUNDS
 * Array index out of bounds.
 */
/** \def CODA_ERROR_NO_PARENT
 * Cursor has no parent.
 */

/** \def CODA_ERROR_UNSUPPORTED_PRODUCT
 * Unsupported product file. This means that either the product format is not supported or that it was not possible to
 * determine the product type and version of the file.
 */

/** \def CODA_ERROR_PRODUCT
 * There was an error detected in the product.
 */
/** \def CODA_ERROR_OUT_OF_BOUNDS_READ
 * Trying to read outside the element boundary. This happens if there was a read beyond the end of the product or a 
 * read outside the range of an enclosing element such as an XML element.
 * This error usually means that either the product or its definition in CODA contains an error.
 */

/** \def CODA_ERROR_DATA_DEFINITION
 * There was an error detected in the CODA Data Definitions.
 */
/** \def CODA_ERROR_EXPRESSION
 * There was an error detected while parsing or evaluating an expression.
 */

/** @} */

/** Variable that contains the error type.
 * If no error has occurred the variable contains #CODA_SUCCESS (0).
 * \hideinitializer
 */
THREAD_LOCAL int coda_errno = CODA_SUCCESS;


LIBCODA_API int *coda_get_errno(void)
{
    return &coda_errno;
}

void coda_add_error_message_vargs(const char *message, va_list ap)
{
    char message_buffer[MAX_ERROR_INFO_LENGTH + 1];
    int current_length;

    if (message == NULL)
    {
        return;
    }

    current_length = (int)strlen(coda_error_message_buffer);
    if (current_length >= MAX_ERROR_INFO_LENGTH)
    {
        return;
    }
    if (current_length == 0)
    {
        /* populate the message buffer with the default error message */
        strcpy(message_buffer, coda_errno_to_string(coda_errno));
        current_length = (int)strlen(message_buffer);
    }
    /* write to local buffer in order to allow using the result of coda_errno_to_string inside the va_list */
    vsnprintf(message_buffer, MAX_ERROR_INFO_LENGTH - current_length, message, ap);
    message_buffer[MAX_ERROR_INFO_LENGTH - current_length] = '\0';
    strcat(coda_error_message_buffer, message_buffer);
}

void coda_add_error_message(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    coda_add_error_message_vargs(message, ap);
    va_end(ap);
}

void coda_set_error_message_vargs(const char *message, va_list ap)
{
    if (message == NULL)
    {
        coda_error_message_buffer[0] = '\0';
    }
    else
    {
        char message_buffer[MAX_ERROR_INFO_LENGTH + 1];

        /* write to local buffer first in order to allow using the result of coda_errno_to_string inside the va_list */
        vsnprintf(message_buffer, MAX_ERROR_INFO_LENGTH, message, ap);
        message_buffer[MAX_ERROR_INFO_LENGTH] = '\0';
        strcpy(coda_error_message_buffer, message_buffer);
    }
}

void coda_set_error_message(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    coda_set_error_message_vargs(message, ap);
    va_end(ap);
}

static int add_error_message(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    coda_add_error_message_vargs(message, ap);
    va_end(ap);

    return 0;
}

void coda_cursor_add_to_error_message(const coda_cursor *cursor)
{
    coda_add_error_message(" at ");
    coda_cursor_print_path(cursor, add_error_message);
}

/** Set the error value and optionally set a custom error message.
 * If \a message is NULL then the default error message for the error number will be used.
 * \param err Value of #coda_errno.
 * \param message Optional error message using printf() format.
 */
LIBCODA_API void coda_set_error(int err, const char *message, ...)
{
    va_list ap;

    coda_errno = err;

    va_start(ap, message);
    coda_set_error_message_vargs(message, ap);
    va_end(ap);

#ifdef HAVE_HDF4
    if (err == CODA_ERROR_HDF4 && message == NULL)
    {
        coda_hdf4_add_error_message();
    }
#endif
#ifdef HAVE_HDF5
    if (err == CODA_ERROR_HDF5 && message == NULL)
    {
        coda_hdf5_add_error_message();
    }
#endif
}

/** Returns a string with the description of the CODA error.
 * If \a err equals the current CODA error status then this function will return the error message that was last set using
 * coda_set_error(). If the error message argument to coda_set_error() was NULL or if \a err does not equal the current
 * CODA error status then the default error message for \a err will be returned.
 * \param err Value of #coda_errno.
 * \return String with a description of the CODA error.
 */
LIBCODA_API const char *coda_errno_to_string(int err)
{
    if (err == coda_errno && coda_error_message_buffer[0] != '\0')
    {
        /* return the custom error message for the current CODA error */
        return coda_error_message_buffer;
    }
    else
    {
        switch (err)
        {
            case CODA_SUCCESS:
                return "success (no error)";
            case CODA_ERROR_OUT_OF_MEMORY:
                return "out of memory";
            case CODA_ERROR_HDF4:
                return "HDF4 error";
            case CODA_ERROR_NO_HDF4_SUPPORT:
                return "HDF4 is not supported (this version of CODA was not built with HDF4 support)";
            case CODA_ERROR_HDF5:
                return "HDF5 error";
            case CODA_ERROR_NO_HDF5_SUPPORT:
                return "HDF5 is not supported (this version of CODA was not built with HDF5 support)";
            case CODA_ERROR_XML:
                return "unknown error while parsing XML data";

            case CODA_ERROR_FILE_NOT_FOUND:
                return "file not found";
            case CODA_ERROR_FILE_OPEN:
                return "could not open file";
            case CODA_ERROR_FILE_READ:
                return "could not read data from file";
            case CODA_ERROR_FILE_WRITE:
                return "could not write data to file";

            case CODA_ERROR_INVALID_ARGUMENT:
                return "invalid argument";
            case CODA_ERROR_INVALID_INDEX:
                return "invalid index argument";
            case CODA_ERROR_INVALID_NAME:
                return "invalid name argument";
            case CODA_ERROR_INVALID_FORMAT:
                return "invalid format in argument";
            case CODA_ERROR_INVALID_DATETIME:
                return "invalid date/time argument";
            case CODA_ERROR_INVALID_TYPE:
                return "invalid type";
            case CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH:
                return "incorrect number of dimensions argument";
            case CODA_ERROR_ARRAY_OUT_OF_BOUNDS:
                return "array index out of bounds";
            case CODA_ERROR_NO_PARENT:
                return "cursor has no parent";

            case CODA_ERROR_UNSUPPORTED_PRODUCT:
                return "unsupported product file";

            case CODA_ERROR_PRODUCT:
                return "product error detected";
            case CODA_ERROR_OUT_OF_BOUNDS_READ:
                return "trying to read outside the element boundary";

            case CODA_ERROR_DATA_DEFINITION:
                return "error in data definitions detected";
            case CODA_ERROR_EXPRESSION:
                return "error detected while parsing/evaluating expression";

            default:
                if (err == coda_errno)
                {
                    return coda_error_message_buffer;
                }
                else
                {
                    return "";
                }
        }
    }
}

/** @} */
