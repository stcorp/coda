/*
 * Copyright (C) 2007-2012 S[&]T, The Netherlands.
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

#include "coda-internal.h"
#include "coda-type.h"
#include "coda-ascbin-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int check_data(coda_cursor *cursor, int64_t *bit_size,
                      void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata)
{
    coda_type_class type_class;
    int has_attributes;

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        return -1;
    }
    switch (type_class)
    {
        case coda_array_class:
            {
                long num_elements;
                long i;

                if (bit_size != NULL)
                {
                    *bit_size = 0;
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
                        if (bit_size != NULL)
                        {
                            int64_t element_size;

                            if (check_data(cursor, &element_size, callbackfunc, userdata) != 0)
                            {
                                return -1;
                            }
                            *bit_size += element_size;
                        }
                        else
                        {
                            if (check_data(cursor, NULL, callbackfunc, userdata) != 0)
                            {
                                return -1;
                            }
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
                }
            }
            break;
        case coda_record_class:
            {
                long num_elements;
                long i;

                if (bit_size != NULL)
                {
                    *bit_size = 0;
                }
                if (coda_cursor_get_num_elements(cursor, &num_elements) != 0)
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
                        if (bit_size != NULL)
                        {
                            int64_t element_size;

                            if (check_data(cursor, &element_size, callbackfunc, userdata) != 0)
                            {
                                return -1;
                            }
                            *bit_size += element_size;
                        }
                        else
                        {
                            if (check_data(cursor, NULL, callbackfunc, userdata) != 0)
                            {
                                return -1;
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
            }
            break;
        case coda_integer_class:
        case coda_real_class:
            {
                double value;

                if (bit_size != NULL)
                {
                    if (coda_cursor_get_bit_size(cursor, bit_size) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_cursor_read_double(cursor, &value) != 0)
                {
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
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
                coda_type *type;
                char *data = NULL;
                long string_length;
                const char *fixed_value;
                long fixed_value_length;

                if (bit_size != NULL)
                {
                    if (coda_cursor_get_bit_size(cursor, bit_size) != 0)
                    {
                        return -1;
                    }
                }
                if (coda_cursor_get_string_length(cursor, &string_length) != 0)
                {
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                    {
                        return -1;
                    }
                    callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                    /* if we can't determine the string length, don't try to read the data */
                    break;
                }
                if (string_length < 0)
                {
                    callbackfunc(cursor, "string length is negative", userdata);
                    /* if we can't determine a proper string length, don't try to read the data */
                    break;
                }

                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_fixed_value(type, &fixed_value, &fixed_value_length) != 0)
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
                    switch (((coda_ascbin_product *)cursor->product)->end_of_line)
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
                                callbackfunc(cursor, "invalid end of line sequence (expected carriage return followed "
                                             "by newline)", userdata);
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
                int64_t local_bit_size;
                int64_t byte_size;
                coda_type *type;
                const char *fixed_value;
                long fixed_value_length;

                if (bit_size != NULL)
                {
                    if (coda_cursor_get_bit_size(cursor, bit_size) != 0)
                    {
                        return -1;
                    }
                    local_bit_size = *bit_size;
                }
                else
                {
                    if (coda_cursor_get_bit_size(cursor, &local_bit_size) != 0)
                    {
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            return -1;
                        }
                        callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                        /* if we can't determine the bit size, don't try to read the data */
                        break;
                    }
                }
                if (local_bit_size < 0)
                {
                    callbackfunc(cursor, "bit size is negative", userdata);
                    /* if we can't determine a proper size, don't try to read the data */
                    break;
                }
                byte_size = (local_bit_size >> 3) + (local_bit_size & 0x7 ? 1 : 0);

                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_fixed_value(type, &fixed_value, &fixed_value_length) != 0)
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
                        if (coda_cursor_read_bits(cursor, data, 0, local_bit_size) != 0)
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
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            return -1;
                        }
                        callbackfunc(cursor, coda_errno_to_string(coda_errno), userdata);
                        /* just continue checking the remaining file */
                    }
                }

                if (special_type != coda_special_no_data)
                {
                    if (coda_cursor_use_base_type_of_special_type(cursor) != 0)
                    {
                        return -1;
                    }
                    if (check_data(cursor, bit_size, callbackfunc, userdata) != 0)
                    {
                        return -1;
                    }
                }
                else if (bit_size != NULL)
                {
                    *bit_size = 0;
                }

            }
            break;
    }

    if (coda_cursor_has_attributes(cursor, &has_attributes) != 0)
    {
        return -1;
    }
    if (has_attributes)
    {
        if (coda_cursor_goto_attributes(cursor) != 0)
        {
            return -1;
        }
        if (check_data(cursor, NULL, callbackfunc, userdata) != 0)
        {
            return -1;
        }
        coda_cursor_goto_parent(cursor);
    }

    return 0;
}

LIBCODA_API int coda_product_check(coda_product *product, int full_read_check,
                                   void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata)
{
    coda_cursor cursor;
    coda_format format;
    int result = 0;

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }
    if (coda_get_product_format(product, &format) != 0)
    {
        return -1;
    }

    if (format == coda_format_ascii || format == coda_format_binary)
    {
        int64_t real_file_size;
        int64_t calculated_file_size;

        if (coda_get_product_file_size(product, &real_file_size) != 0)
        {
            return -1;
        }
        real_file_size <<= 3;   /* now in bits */

        if (full_read_check)
        {
            result = check_data(&cursor, &calculated_file_size, callbackfunc, userdata);
        }
        else
        {
            int prev_option_value;

            /* we explicitly disable the use of fast size expressions because we also want to verify the structural
             * integrity within each record. */
            prev_option_value = coda_get_option_use_fast_size_expressions();
            coda_set_option_use_fast_size_expressions(0);
            result = coda_cursor_get_bit_size(&cursor, &calculated_file_size);
            coda_set_option_use_fast_size_expressions(prev_option_value);
        }

        if (result == 0 && real_file_size != calculated_file_size)
        {
            char msg[256];
            char s1[21];
            char s2[21];

            coda_str64(real_file_size >> 3, s1);
            coda_str64(calculated_file_size >> 3, s2);
            if (calculated_file_size & 0x7)
            {
                sprintf(msg, "incorrect file size (actual size %s does not match calculated file size %s:%d)", s1, s2,
                        (int)(calculated_file_size & 0x7));
            }
            else
            {
                sprintf(msg, "incorrect file size (actual size %s does not match calculated file size %s)", s1, s2);
            }
            callbackfunc(NULL, msg, userdata);
        }
    }
    else
    {
        if (full_read_check)
        {
            result = check_data(&cursor, NULL, callbackfunc, userdata);
        }
    }

    return result;
}
