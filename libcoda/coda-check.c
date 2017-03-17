/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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
#include "coda-type.h"
#include "coda-ascii-internal.h"
#include "coda-mem-internal.h"
#include "coda-read-bytes.h"
#include "coda-definition.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int check_definition(coda_cursor *cursor, coda_type **definition,
                            void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata)
{
    coda_type_class definition_type_class;
    coda_type_class type_class;

    if (*definition == NULL)
    {
        return 0;
    }

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        return -1;
    }
    if (coda_type_get_class(*definition, &definition_type_class) != 0)
    {
        return -1;
    }
    if (type_class != definition_type_class)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "type (%s) does not match definition (%s)",
                       coda_type_get_class_name(type_class), coda_type_get_class_name(definition_type_class));
        callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
        coda_errno = 0;

        /* no use to further compare sub-elements or attributes */
        *definition = NULL;
        return 0;
    }

    switch (type_class)
    {
        case coda_array_class:
            {
                coda_type_array *array = *(coda_type_array **)definition;
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;
                int i;

                if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    return -1;
                }

                if (num_dims != array->num_dims)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "number of dimensions (%d) does not match definition (%d)",
                                   num_dims, array->num_dims);
                    callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                    coda_errno = 0;

                    /* no use checking individual dimension sizes */
                    return 0;
                }
                for (i = 0; i < num_dims; i++)
                {
                    if (array->dim[i] >= 0)
                    {
                        if (dim[i] != array->dim[i])
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "size of dim[%d] (%ld) does not match definition (%ld)",
                                           i, dim[i], array->dim[i]);
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                    }
                    else if (array->dim_expr[i] != NULL)
                    {
                        int64_t size;

                        if (coda_expression_eval_integer(array->dim_expr[i], cursor, &size) != 0)
                        {
                            coda_add_error_message(" while evaluating definition expression for dimension %d", i);
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                        else if (dim[i] != size)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "size of dim[%d] (%ld) does not match definition (%ld)",
                                           i, dim[i], (long)size);
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                    }
                }
            }
            break;
        case coda_record_class:
            {
                coda_type_record *record = *(coda_type_record **)definition;
                coda_type *type;
                long num_fields;
                int i;

                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    return -1;
                }

                /* check whether each field in the definition is present */
                for (i = 0; i < record->num_fields; i++)
                {
                    int available_definition = 1;
                    int available = 1;
                    long index;

                    if (coda_cursor_get_record_field_index_from_name(cursor, record->field[i]->name, &index) != 0)
                    {
                        if (coda_errno != CODA_ERROR_INVALID_NAME)
                        {
                            return -1;
                        }
                        coda_errno = 0;
                        available = 0;
                    }
                    else
                    {
                        if (coda_cursor_get_record_field_available_status(cursor, index, &available) != 0)
                        {
                            return -1;
                        }
                    }

                    if (record->field[i]->optional)
                    {
                        if (record->field[i]->available_expr != NULL)
                        {
                            if (coda_expression_eval_bool(record->field[i]->available_expr, cursor,
                                                          &available_definition) != 0)
                            {
                                coda_add_error_message(" while evaluating definition expression for "
                                                       "availability of field '%s'", record->field[i]->name);
                                callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                                coda_errno = 0;
                            }
                        }
                        else
                        {
                            available_definition = -1;
                        }
                    }
                    if (available_definition != -1)
                    {
                        if (available != available_definition)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "field '%s' availability (%s) does not match definition "
                                           "(%s)", record->field[i]->name, available ? "available" : "unavailable",
                                           available_definition ? "available" : "unavailable");
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                    }
                    if (available && available_definition)
                    {
                        const char *real_name_definition;
                        const char *real_name;

                        if (coda_type_get_record_field_real_name(type, index, &real_name) != 0)
                        {
                            return -1;
                        }
                        real_name_definition = record->field[i]->real_name != NULL ? record->field[i]->real_name :
                            record->field[i]->name;
                        if (strcmp(real_name, real_name_definition) != 0)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "real name for field '%s' (%s) does not match "
                                           "definition (%s)", record->field[i]->name, real_name, real_name_definition);
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                    }
                }

                /* check whether product has fields that were not in the definition */
                if (coda_cursor_get_num_elements(cursor, &num_fields) != 0)
                {
                    return -1;
                }
                for (i = 0; i < num_fields; i++)
                {
                    const char *field_name;
                    long index;

                    if (coda_type_get_record_field_name(type, i, &field_name) != 0)
                    {
                        return -1;
                    }
                    if (coda_type_get_record_field_index_from_name(*definition, field_name, &index) != 0)
                    {
                        if (coda_errno == CODA_ERROR_INVALID_NAME)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "field '%s' availability (available) does not match "
                                           "definition (not allowed)", field_name);
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            coda_errno = 0;
                        }
                    }
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                {
                    return -1;
                }
                if (read_type != (*definition)->read_type)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "read type (%s) does not match definition (%s)",
                                   coda_type_get_native_type_name(read_type),
                                   coda_type_get_native_type_name((*definition)->read_type));
                    callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                    coda_errno = 0;
                }
            }
            break;
        case coda_text_class:
            break;
        case coda_raw_class:
            break;
        case coda_special_class:
            {
                coda_special_type special_type;
                coda_special_type definition_special_type;

                if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_special_type(*definition, &definition_special_type) != 0)
                {
                    return -1;
                }
                if (special_type != definition_special_type)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "special type (%s) does not match definition (%s)",
                                   coda_type_get_special_type_name(special_type),
                                   coda_type_get_special_type_name(definition_special_type));
                    callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                    coda_errno = 0;
                }
                /* don't compare base types */
                *definition = NULL;
            }
            break;
    }

    return 0;
}

static int check_data(coda_cursor *cursor, coda_type **definition, int read_check, int size_check, int64_t *bit_size,
                      void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata)
{
    coda_type_class type_class;
    int skip_mem_size_check = 0;
    int has_attributes;
    coda_type *type;

    /* even if size_check==0 we need bit_size!=NULL because for some cases sizes are still calculated */
    assert(bit_size != NULL);

    if (coda_cursor_get_type(cursor, &type) != 0)
    {
        return -1;
    }
    if (coda_type_get_class(type, &type_class) != 0)
    {
        return -1;
    }

    /* check definition */
    if (check_definition(cursor, definition, callbackfunc, userdata) != 0)
    {
        return -1;
    }

    /* check bit size */
    if (size_check)
    {
        switch (type_class)
        {
            case coda_array_class:
            case coda_record_class:
            case coda_special_class:
                /* start with size=0 and have traversal below add size of sub element(s) */
                *bit_size = 0;
                break;
            case coda_integer_class:
            case coda_real_class:
            case coda_text_class:
            case coda_raw_class:
                if (coda_cursor_get_bit_size(cursor, bit_size) != 0)
                {
                    return -1;
                }
                break;
        }
    }

    /* try to read data */
    if (read_check)
    {
        switch (type_class)
        {
            case coda_array_class:
            case coda_record_class:
                break;
            case coda_integer_class:
            case coda_real_class:
                {
                    double value;

                    if (coda_cursor_read_double(cursor, &value) != 0)
                    {
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT &&
                            coda_errno != CODA_ERROR_INVALID_DATETIME)
                        {
                            return -1;
                        }
                        callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                        /* just continue checking the remaining file */
                    }
                }
                break;
            case coda_text_class:
                {
                    char *data = NULL;
                    long string_length;
                    const char *fixed_value;
                    long fixed_value_length;

                    if (coda_cursor_get_string_length(cursor, &string_length) != 0)
                    {
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT &&
                            coda_errno != CODA_ERROR_INVALID_DATETIME)
                        {
                            return -1;
                        }
                        callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                        /* if we can't determine the string length, don't try to read the data */
                        skip_mem_size_check = 1;
                        break;
                    }
                    if (string_length < 0)
                    {
                        callbackfunc(cursor, "string length is negative", userdata);
                        /* if we can't determine a proper string length, don't try to read the data */
                        skip_mem_size_check = 1;
                        break;
                    }

                    if (coda_type_get_fixed_value(*definition != NULL ? *definition : type, &fixed_value,
                                                  &fixed_value_length) != 0)
                    {
                        return -1;
                    }

                    if (string_length > 0)
                    {
                        data = (char *)malloc(string_length + 1);
                        if (data == NULL)
                        {
                            coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                           "out of memory (could not allocate %lu bytes) (%s:%u)", string_length + 1,
                                           __FILE__, __LINE__);
                            return -1;
                        }
                        if (coda_cursor_read_string(cursor, data, string_length + 1) != 0)
                        {
                            free(data);
                            return -1;
                        }
                    }

                    if (fixed_value != NULL)
                    {
                        if (string_length != fixed_value_length)
                        {
                            callbackfunc(cursor, "string data does not match fixed value (length differs)", userdata);
                            /* we do not return -1; we can just continue checking the rest of the file */
                        }
                        else if (string_length > 0)
                        {
                            if (memcmp(data, fixed_value, fixed_value_length) != 0)
                            {
                                callbackfunc(cursor, "string data does not match fixed value", userdata);
                                /* we do not return -1; we can just continue checking the rest of the file */
                            }
                        }
                    }
                    if (((coda_type_text *)type)->special_text_type == ascii_text_line_separator)
                    {
                        switch (((coda_ascii_product *)cursor->product)->end_of_line)
                        {
                            case eol_lf:
                                if (string_length != 1 || data[0] != '\n')
                                {
                                    callbackfunc(cursor, "invalid end of line sequence (expected newline)", userdata);
                                }
                                break;
                            case eol_cr:
                                if (string_length != 1 || data[0] != '\r')
                                {
                                    callbackfunc(cursor, "invalid end of line sequence (expected carriage return)",
                                                 userdata);
                                }
                                break;
                            case eol_crlf:
                                if (string_length != 2 || data[0] != '\r' || data[1] != '\n')
                                {
                                    callbackfunc(cursor, "invalid end of line sequence (expected carriage return "
                                                 "followed by newline)", userdata);
                                }
                                break;
                            case eol_unknown:
                                assert(0);
                                exit(1);
                        }
                    }

                    if (data != NULL)
                    {
                        free(data);
                    }
                }
                break;
            case coda_raw_class:
                {
                    int64_t byte_size;
                    const char *fixed_value;
                    long fixed_value_length;

                    if (!size_check)
                    {
                        if (coda_cursor_get_bit_size(cursor, bit_size) != 0)
                        {
                            if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT &&
                                coda_errno != CODA_ERROR_INVALID_DATETIME)
                            {
                                return -1;
                            }
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            /* if we can't determine the bit size, don't try to read the data */
                            skip_mem_size_check = 1;
                            break;
                        }
                    }
                    if (*bit_size < 0)
                    {
                        callbackfunc(cursor, "bit size is negative", userdata);
                        /* if we can't determine a proper size, don't try to read the data */
                        skip_mem_size_check = 1;
                        break;
                    }
                    byte_size = (*bit_size >> 3) + (*bit_size & 0x7 ? 1 : 0);

                    if (coda_type_get_fixed_value(*definition != NULL ? *definition : type, &fixed_value,
                                                  &fixed_value_length) != 0)
                    {
                        return -1;
                    }
                    if (fixed_value != NULL)
                    {
                        if (byte_size != fixed_value_length)
                        {
                            callbackfunc(cursor, "data does not match fixed value (length differs)", userdata);
                            /* we do not return -1; we can just continue checking the rest of the file */
                        }
                        else if (fixed_value_length > 0)
                        {
                            uint8_t *data;

                            data = (uint8_t *)malloc((size_t)byte_size);
                            if (data == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)", (long)byte_size,
                                               __FILE__, __LINE__);
                                return -1;
                            }
                            if (coda_cursor_read_bits(cursor, data, 0, *bit_size) != 0)
                            {
                                free(data);
                                return -1;
                            }
                            if (memcmp(data, fixed_value, fixed_value_length) != 0)
                            {
                                callbackfunc(cursor, "data does not match fixed value (value differs)", userdata);
                                /* we do not return -1; we can just continue checking the rest of the file */
                            }
                            free(data);
                        }
                    }
                }
                break;
            case coda_special_class:
                {
                    coda_special_type special_type;

                    if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                    {
                        return -1;
                    }

                    if (special_type == coda_special_time)
                    {
                        double value;

                        /* try to read the time value as a double */
                        if (coda_cursor_read_double(cursor, &value) != 0)
                        {
                            if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT &&
                                coda_errno != CODA_ERROR_INVALID_DATETIME)
                            {
                                return -1;
                            }
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            /* just continue checking the remaining file */
                        }
                    }
                }
                break;
        }
    }

    /* check attributes */
    if (coda_cursor_has_attributes(cursor, &has_attributes) != 0)
    {
        return -1;
    }
    if (has_attributes)
    {
        coda_type *attributes_definition = NULL;
        int64_t attribute_size;

        if (*definition != NULL)
        {
            if (coda_type_get_attributes(*definition, &attributes_definition) != 0)
            {
                return -1;
            }
        }
        if (coda_cursor_goto_attributes(cursor) != 0)
        {
            return -1;
        }
        if (check_data(cursor, &attributes_definition, read_check, 0, &attribute_size, callbackfunc, userdata) != 0)
        {
            return -1;
        }
        coda_cursor_goto_parent(cursor);
        if (*definition != NULL && attributes_definition == NULL)
        {
            *definition = NULL;
        }
    }

    /* traverse sub-elements */
    if (*definition != NULL || read_check || size_check)
    {
        int64_t sub_bit_size;

        switch (type_class)
        {
            case coda_array_class:
                {
                    coda_type *base_definition = NULL;
                    long num_elements;
                    long i;

                    if (*definition != NULL)
                    {
                        if (coda_type_get_array_base_type(*definition, &base_definition) != 0)
                        {
                            return -1;
                        }
                    }
                    if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
                    {
                        return -1;
                    }
                    if (num_elements > 0)
                    {
                        if (coda_cursor_goto_first_array_element(cursor) != 0)
                        {
                            return -1;
                        }
                        for (i = 0; i < num_elements; i++)
                        {
                            if (check_data(cursor, &base_definition, read_check, size_check, &sub_bit_size,
                                           callbackfunc, userdata) != 0)
                            {
                                return -1;
                            }
                            if (size_check)
                            {
                                *bit_size += sub_bit_size;
                            }
                            if (i < num_elements - 1)
                            {
                                if (coda_cursor_goto_next_array_element(cursor) != 0)
                                {
                                    return -1;
                                }
                            }
                        }
                        coda_cursor_goto_parent(cursor);
                        if (*definition != NULL && base_definition == NULL)
                        {
                            *definition = NULL;
                        }
                    }
                }
                break;
            case coda_record_class:
                {
                    coda_cursor record_cursor = *cursor;
                    long num_elements;
                    long i;

                    if (coda_cursor_get_num_elements(&record_cursor, &num_elements) != 0)
                    {
                        return -1;
                    }
                    if (num_elements > 0)
                    {
                        if (coda_cursor_goto_first_record_field(cursor) != 0)
                        {
                            return -1;
                        }
                        for (i = 0; i < num_elements; i++)
                        {
                            coda_type *field_definition = NULL;
                            int available;

                            if (coda_cursor_get_record_field_available_status(&record_cursor, i, &available) != 0)
                            {
                                return -1;
                            }
                            if (available)
                            {
                                if (*definition != NULL)
                                {
                                    const char *field_name;
                                    long index;

                                    if (coda_type_get_record_field_name(type, i, &field_name) != 0)
                                    {
                                        return -1;
                                    }
                                    if (coda_type_get_record_field_index_from_name(*definition, field_name, &index) !=
                                        0)
                                    {
                                        /* allow field to not be available in definition -> field_definition=NULL */
                                        coda_errno = 0;
                                    }
                                    else if (coda_type_get_record_field_type(*definition, index, &field_definition) !=
                                             0)
                                    {
                                        return -1;
                                    }
                                }
                                if (check_data(cursor, &field_definition, read_check, size_check, &sub_bit_size,
                                               callbackfunc, userdata) != 0)
                                {
                                    return -1;
                                }
                                if (size_check)
                                {
                                    *bit_size += sub_bit_size;
                                }
                            }
                            if (i < num_elements - 1)
                            {
                                if (coda_cursor_goto_next_record_field(cursor) != 0)
                                {
                                    return -1;
                                }
                            }
                        }
                        coda_cursor_goto_parent(cursor);
                    }
                    if (size_check && ((coda_type_record *)type)->size_expr != NULL)
                    {
                        int64_t fast_size;
                        int prev_option_value = coda_get_option_use_fast_size_expressions();

                        coda_set_option_use_fast_size_expressions(1);
                        if (coda_cursor_get_bit_size(cursor, &fast_size) != 0)
                        {
                            callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                            skip_mem_size_check = 1;
                        }
                        else if (*bit_size != fast_size)
                        {
                            char error_message[256];
                            char s1[30];
                            char s2[30];

                            coda_str64((*bit_size) >> 3, s1);
                            if (*bit_size & 0x7)
                            {
                                sprintf(&s1[strlen(s1)], ":%d", (int)((*bit_size) & 0x7));
                            }
                            coda_str64(fast_size >> 3, s2);
                            if (fast_size & 0x7)
                            {
                                sprintf(&s2[strlen(s2)], ":%d", (int)(fast_size & 0x7));
                            }
                            sprintf(error_message, "invalid result for record size expression (actual record size %s "
                                    "does not match expression result %s)", s1, s2);
                            callbackfunc(cursor, error_message, userdata);
                        }
                        coda_set_option_use_fast_size_expressions(prev_option_value);
                    }
                }
                break;
            case coda_integer_class:
            case coda_real_class:
            case coda_text_class:
            case coda_raw_class:
                break;
            case coda_special_class:
                {
                    coda_special_type special_type;
                    coda_type *base_definition = NULL;

                    if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                    {
                        return -1;
                    }
                    assert(special_type != coda_special_no_data);

                    if (*definition != NULL)
                    {
                        if (coda_type_get_special_base_type(*definition, &base_definition) != 0)
                        {
                            return -1;
                        }
                    }
                    if (coda_cursor_use_base_type_of_special_type(cursor) != 0)
                    {
                        return -1;
                    }
                    if (check_data(cursor, &base_definition, read_check, size_check, bit_size, callbackfunc,
                                   userdata) != 0)
                    {
                        return -1;
                    }
                }
                break;
        }
    }

    /* additional size test for mem backend */
    if (size_check && !skip_mem_size_check && cursor->stack[cursor->n - 1].type->backend == coda_backend_memory)
    {
        if (((coda_mem_type *)cursor->stack[cursor->n - 1].type)->tag == tag_mem_data)
        {
            int64_t expected_byte_size = ((coda_mem_data *)cursor->stack[cursor->n - 1].type)->length;
            int64_t calculated_bit_size = 0;

            if (bit_size != NULL)
            {
                calculated_bit_size = *bit_size;
            }
            else if (coda_cursor_get_bit_size(cursor, &calculated_bit_size) != 0)
            {
                return -1;
            }
            if (cursor->product->format == coda_format_xml &&
                bit_size_to_byte_size(calculated_bit_size) < expected_byte_size)
            {
                long offset = (long)((coda_mem_data *)cursor->stack[cursor->n - 1].type)->offset;
                int64_t byte_size = expected_byte_size - (calculated_bit_size >> 3);
                char *data;
                long i;

                /* verify that trailing data consists of only whitespace */
                data = (char *)malloc((size_t)byte_size + 1);
                if (data == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                   "out of memory (could not allocate %lu bytes) (%s:%u)", (long)byte_size,
                                   __FILE__, __LINE__);
                    return -1;
                }
                if (read_bytes(cursor->product, offset + (calculated_bit_size >> 3), byte_size, (uint8_t *)data) != 0)
                {
                    free(data);
                    return -1;
                }
                data[byte_size] = '\0';
                for (i = 0; i < byte_size; i++)
                {
                    if (data[i] != ' ' && data[i] != '\t' && data[i] != '\n' && data[i] != '\r')
                    {
                        callbackfunc(cursor, "non-whitespace trailing data found for xml content", userdata);
                        break;
                    }
                }
                free(data);
            }
            else if (bit_size_to_byte_size(calculated_bit_size) != expected_byte_size)
            {
                char error_message[256];
                char s1[30];
                char s2[30];

                coda_str64(expected_byte_size, s1);
                coda_str64(calculated_bit_size >> 3, s2);
                if (calculated_bit_size & 0x7)
                {
                    sprintf(&s2[strlen(s2)], ":%d", (int)(calculated_bit_size & 0x7));
                }
                sprintf(error_message, "incorrect block size (actual size %s does not match calculated size %s)",
                        s1, s2);
                callbackfunc(cursor, error_message, userdata);
                /* just continue checking the remaining file */
            }
        }
    }

    return 0;
}

LIBCODA_API int coda_product_check(coda_product *product, int full_read_check,
                                   void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata)
{
    coda_type *definition = NULL;
    coda_cursor cursor;
    coda_format format;
    int64_t real_file_size = 0;
    int64_t calculated_file_size = 0;
    int size_check;

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }
    if (coda_get_product_format(product, &format) != 0)
    {
        return -1;
    }

    if (format != coda_format_ascii && format != coda_format_binary && format != coda_format_xml)
    {
        /* we only need to check against the format definition for self describing data formats */
        if (product->product_definition != NULL && product->product_definition->root_type != NULL)
        {
            definition = product->product_definition->root_type;
        }
    }

    size_check = (format == coda_format_ascii || format == coda_format_binary);

    if (size_check)
    {
        if (coda_get_product_file_size(product, &real_file_size) != 0)
        {
            return -1;
        }
        real_file_size <<= 3;   /* now in bits */
    }

    if (size_check && !full_read_check)
    {
        int prev_option_value;

        /* we explicitly disable the use of fast size expressions because we also want to verify the structural
         * integrity within each record. */
        prev_option_value = coda_get_option_use_fast_size_expressions();
        coda_set_option_use_fast_size_expressions(0);
        if (coda_cursor_get_bit_size(&cursor, &calculated_file_size) != 0)
        {
            coda_set_option_use_fast_size_expressions(prev_option_value);
            return -1;
        }
        coda_set_option_use_fast_size_expressions(prev_option_value);
    }
    else
    {
        if (check_data(&cursor, &definition, full_read_check, size_check, &calculated_file_size, callbackfunc,
                       userdata) != 0)
        {
            return -1;
        }
    }

    if (size_check && (real_file_size != calculated_file_size))
    {
        char error_message[256];
        char s1[21];
        char s2[21];

        coda_str64(real_file_size >> 3, s1);
        coda_str64(calculated_file_size >> 3, s2);
        if (calculated_file_size & 0x7)
        {
            sprintf(error_message, "incorrect file size (actual size %s does not match calculated file size %s:%d)", s1,
                    s2, (int)(calculated_file_size & 0x7));
        }
        else
        {
            sprintf(error_message, "incorrect file size (actual size %s does not match calculated file size %s)", s1,
                    s2);
        }
        callbackfunc(NULL, error_message, userdata);
    }

    return 0;
}
