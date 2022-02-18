/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
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

#include "codadump.h"

#include <stdarg.h>

static int INDENT = 0;

static int show_attributes = 0;

static void indent(void)
{
    int i;

    assert(INDENT >= 0);
    for (i = INDENT; i > 0; i--)
    {
        fprintf(ascii_output, "  ");
    }
}

static int ff_printf(const char *templ, ...)
{
    int result;
    va_list ap;

    va_start(ap, templ);
    result = vfprintf(ascii_output, templ, ap);
    va_end(ap);

    return result;
}

static int fi_printf(const char *templ, ...)
{
    int result;
    va_list ap;

    indent();

    va_start(ap, templ);
    result = vfprintf(ascii_output, templ, ap);
    va_end(ap);

    return result;
}

static void print_escaped(const char *data, long length)
{
    long i;

    for (i = 0; i < length; i++)
    {
        char c;

        c = data[i];
        switch (c)
        {
            case '\b':
                ff_printf("\\b");
                break;
            case '\f':
                ff_printf("\\f");
                break;
            case '\n':
                ff_printf("\\n");
                break;
            case '\r':
                ff_printf("\\r");
                break;
            case '\t':
                ff_printf("\\t");
                break;
            case '"':
                ff_printf("\\\"");
                break;
            case '\\':
                ff_printf("\\\\");
                break;
            default:
                if (c >= 32 && c <= 126)
                {
                    ff_printf("%c", c);
                }
                else
                {
                    ff_printf("\\u%02x", (int)(unsigned char)c);
                }
        }
    }
}

static void print_data(coda_cursor *cursor, int compound_newline)
{
    coda_type_class type_class;
    int has_attributes = 0;

    if (show_attributes)
    {
        if (coda_cursor_has_attributes(cursor, &has_attributes) != 0)
        {
            handle_coda_error();
        }
        if (has_attributes)
        {
            if (compound_newline)
            {
                ff_printf("\n");
                indent();
            }
            ff_printf("attr: ");
            if (coda_cursor_goto_attributes(cursor) != 0)
            {
                handle_coda_error();
            }
            INDENT++;
            print_data(cursor, 1);
            INDENT--;
            coda_cursor_goto_parent(cursor);
            fi_printf("data: ");
            INDENT++;
        }
    }

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        handle_coda_error();
    }

    switch (type_class)
    {
        case coda_record_class:
            {
                long num_fields;

                if (coda_cursor_get_num_elements(cursor, &num_fields) != 0)
                {
                    handle_coda_error();
                }
                if (num_fields > 0)
                {
                    coda_type *record_type;
                    int is_union;
                    long i;

                    if (compound_newline)
                    {
                        ff_printf("\n");
                    }
                    if (coda_cursor_get_type(cursor, &record_type) != 0)
                    {
                        handle_coda_error();
                    }

                    if (coda_type_get_record_union_status(record_type, &is_union) != 0)
                    {
                        handle_coda_error();
                    }
                    if (is_union)
                    {
                        const char *field_name;

                        if (coda_cursor_get_available_union_field_index(cursor, &i) != 0)
                        {
                            handle_coda_error();
                        }
                        if (coda_type_get_record_field_name(record_type, i, &field_name) != 0)
                        {
                            handle_coda_error();
                        }
                        if (coda_cursor_goto_record_field_by_index(cursor, i) != 0)
                        {
                            handle_coda_error();
                        }
                        if (compound_newline)
                        {
                            indent();
                        }
                        ff_printf("%s: ", field_name);
                        INDENT++;
                        print_data(cursor, 1);
                        INDENT--;
                        coda_cursor_goto_parent(cursor);
                    }
                    else
                    {
                        int first_field = 1;

                        if (coda_cursor_goto_first_record_field(cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        for (i = 0; i < num_fields; i++)
                        {
                            const char *field_name;
                            int hidden;

                            if (coda_type_get_record_field_hidden_status(record_type, i, &hidden) != 0)
                            {
                                handle_coda_error();
                            }
                            if (!hidden)
                            {
                                if (coda_type_get_record_field_name(record_type, i, &field_name) != 0)
                                {
                                    handle_coda_error();
                                }
                                if (compound_newline || !first_field)
                                {
                                    indent();
                                }
                                if (first_field)
                                {
                                    first_field = 0;
                                }
                                ff_printf("%s: ", field_name);
                                INDENT++;
                                print_data(cursor, 1);
                                INDENT--;
                            }
                            if (i < num_fields - 1)
                            {
                                if (coda_cursor_goto_next_record_field(cursor) != 0)
                                {
                                    handle_coda_error();
                                }
                            }
                        }
                        coda_cursor_goto_parent(cursor);
                    }
                }
                else
                {
                    ff_printf("{}\n");
                }
            }
            break;
        case coda_array_class:
            {
                long dim[CODA_MAX_NUM_DIMS];
                int num_dims;
                long num_elements;

                if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
                {
                    handle_coda_error();
                }
                if (num_dims >= 0)
                {
                    int i;

                    num_elements = 1;
                    for (i = 0; i < num_dims; i++)
                    {
                        num_elements *= dim[i];
                    }
                    if (num_elements > 0)
                    {
                        ff_printf("\n");
                        if (coda_cursor_goto_first_array_element(cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        for (i = 0; i < num_elements; i++)
                        {
                            fi_printf("- ");
                            INDENT++;
                            print_data(cursor, 0);
                            INDENT--;
                            if (i < num_elements - 1)
                            {
                                if (coda_cursor_goto_next_array_element(cursor) != 0)
                                {
                                    handle_coda_error();
                                }
                            }
                        }
                        coda_cursor_goto_parent(cursor);
                    }
                    else
                    {
                        ff_printf("[]\n");
                    }
                }
                else
                {
                    ff_printf("[]\n");
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                {
                    handle_coda_error();
                }
                switch (read_type)
                {
                    case coda_native_type_char:
                        {
                            char data;

                            if (coda_cursor_read_char(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("\"");
                            print_escaped(&data, 1);
                            ff_printf("\"\n");
                        }
                        break;
                    case coda_native_type_string:
                        {
                            long length;
                            char *data;

                            if (coda_cursor_get_string_length(cursor, &length) != 0)
                            {
                                handle_coda_error();
                            }
                            data = (char *)malloc(length + 1);
                            if (data == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)length + 1, __FILE__, __LINE__);
                                handle_coda_error();
                            }
                            if (coda_cursor_read_string(cursor, data, length + 1) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("\"");
                            print_escaped(data, length);
                            ff_printf("\"\n", length);

                            free(data);
                        }
                        break;
                    case coda_native_type_bytes:
                        {
                            int64_t bit_size;
                            int64_t byte_size;
                            uint8_t *data;

                            if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
                            {
                                handle_coda_error();
                            }
                            byte_size = (bit_size >> 3) + (bit_size & 0x7 ? 1 : 0);
                            data = (uint8_t *)malloc((size_t)byte_size);
                            if (data == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)byte_size, __FILE__, __LINE__);
                                handle_coda_error();
                            }
                            if (coda_cursor_read_bits(cursor, data, 0, bit_size) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("\"");
                            print_escaped((char *)data, (long)byte_size);
                            ff_printf("\"\n");

                            free(data);
                        }
                        break;
                    case coda_native_type_int8:
                    case coda_native_type_int16:
                    case coda_native_type_int32:
                        {
                            int32_t data;

                            if (coda_cursor_read_int32(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("%ld\n", (long)data);
                        }
                        break;
                    case coda_native_type_uint8:
                    case coda_native_type_uint16:
                    case coda_native_type_uint32:
                        {
                            uint32_t data;

                            if (coda_cursor_read_uint32(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("%lu\n", (unsigned long)data);
                        }
                        break;
                    case coda_native_type_int64:
                        {
                            int64_t data;
                            char s[21];

                            if (coda_cursor_read_int64(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            coda_str64(data, s);
                            ff_printf("%s\n", s);
                        }
                        break;
                    case coda_native_type_uint64:
                        {
                            uint64_t data;
                            char s[21];

                            if (coda_cursor_read_uint64(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            coda_str64u(data, s);
                            ff_printf("%s\n", s);
                        }
                        break;
                    case coda_native_type_float:
                    case coda_native_type_double:
                        {
                            double data;

                            if (coda_cursor_read_double(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            if (read_type == coda_native_type_float)
                            {
                                ff_printf("%.7g\n", data);
                            }
                            else
                            {
                                ff_printf("%.16g\n", data);
                            }
                        }
                        break;
                    case coda_native_type_not_available:
                        ff_printf("null\n");
                        break;
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(cursor, &special_type) != 0)
                {
                    handle_coda_error();
                }

                switch (special_type)
                {
                    case coda_special_no_data:
                        ff_printf("null\n");
                        break;
                    case coda_special_vsf_integer:
                        {
                            double data;

                            if (coda_cursor_read_double(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("%.16g\n", data);
                        }
                        break;
                    case coda_special_time:
                        {
                            double data;
                            char str[27];

                            if (coda_cursor_read_double(cursor, &data) != 0)
                            {
                                handle_coda_error();
                            }
                            if (coda_isNaN(data) || coda_isInf(data))
                            {
                                ff_printf("%.16g\n", data);
                            }
                            else
                            {
                                if (coda_time_double_to_string(data, "yyyy-MM-dd'T'HH:mm:ss.SSSSSS", str) != 0)
                                {
                                    ff_printf("\"{--invalid time value--}\"");
                                }
                                else
                                {
                                    ff_printf("%s\n", str);
                                }
                            }
                        }
                        break;
                    case coda_special_complex:
                        {
                            double re, im;

                            if (coda_cursor_read_complex_double_split(cursor, &re, &im) != 0)
                            {
                                handle_coda_error();
                            }

                            ff_printf("%g + %gi\n", re, im);
                        }
                        break;
                }
            }
            break;
    }

    if (has_attributes)
    {
        INDENT--;
    }
}

void print_yaml_data(int include_attributes)
{
    coda_product *pf;
    coda_cursor cursor;
    int result;

    show_attributes = include_attributes;

    result = coda_open(traverse_info.file_name, &pf);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(traverse_info.file_name, &pf);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        handle_coda_error();
    }

    if (coda_cursor_set_product(&cursor, pf) != 0)
    {
        handle_coda_error();
    }
    if (starting_path != NULL)
    {
        result = coda_cursor_goto(&cursor, starting_path);
        if (result != 0)
        {
            handle_coda_error();
        }
    }

    coda_set_option_perform_boundary_checks(0);
    print_data(&cursor, 0);

    coda_close(pf);
}
