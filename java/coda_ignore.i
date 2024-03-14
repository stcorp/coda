/*
 * Copyright (C) 2007-2024 S[&]T, The Netherlands.
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

%ignore CODA_H;
%ignore HAVE_INTTYPES_H;
%ignore HAVE_STDINT_H;
%ignore HAVE_SYS_TYPES_H;
%ignore CODA_SUCCESS;
%ignore CODA_ERROR_OUT_OF_MEMORY;
%ignore CODA_ERROR_HDF4;
%ignore CODA_ERROR_NO_HDF4_SUPPORT;
%ignore CODA_ERROR_HDF5;
%ignore CODA_ERROR_NO_HDF5_SUPPORT;
%ignore CODA_ERROR_XML;
%ignore CODA_ERROR_FILE_NOT_FOUND;
%ignore CODA_ERROR_FILE_OPEN;
%ignore CODA_ERROR_FILE_READ;
%ignore CODA_ERROR_FILE_WRITE;
%ignore CODA_ERROR_INVALID_ARGUMENT;
%ignore CODA_ERROR_INVALID_INDEX;
%ignore CODA_ERROR_INVALID_NAME;
%ignore CODA_ERROR_INVALID_FORMAT;
%ignore CODA_ERROR_INVALID_DATETIME;
%ignore CODA_ERROR_INVALID_TYPE;
%ignore CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH;
%ignore CODA_ERROR_ARRAY_OUT_OF_BOUNDS;
%ignore CODA_ERROR_NO_PARENT;
%ignore CODA_ERROR_UNSUPPORTED_PRODUCT;
%ignore CODA_ERROR_PRODUCT;
%ignore CODA_ERROR_OUT_OF_BOUNDS_READ;
%ignore CODA_ERROR_DATA_DEFINITION;
%ignore CODA_ERROR_EXPRESSION;
%ignore CODA_PRIVATE_FIELD(name);
%ignore CODA_CURSOR_MAXDEPTH;

%ignore coda_free;
%ignore coda_str64;
%ignore coda_str64u;
%ignore coda_strfl;

%ignore coda_cursor_print_path;
%ignore coda_expression_print;
