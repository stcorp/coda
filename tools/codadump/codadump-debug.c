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

#include "codadump.h"

#include <stdarg.h>

static int INDENT = 0;

static int print_offsets = 1;

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
            case '\a':
                ff_printf("\\a");
                break;
            case '\b':
                ff_printf("\\b");
                break;
            case '\t':
                ff_printf("\\t");
                break;
            case '\n':
                ff_printf("\\n");
                break;
            case '\v':
                ff_printf("\\v");
                break;
            case '\f':
                ff_printf("\\f");
                break;
            case '\r':
                ff_printf("\\r");
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
                    ff_printf("\\%03o", (int)(unsigned char)c);
                }
        }
    }
}

static void print_data(coda_Cursor *cursor)
{
    coda_type_class type_class;
    long num_attributes;

    /* print attributes */
    if (coda_cursor_goto_attributes(cursor) != 0)
    {
        handle_coda_error();
    }
    if (coda_cursor_get_num_elements(cursor, &num_attributes) != 0)
    {
        handle_coda_error();
    }
    if (num_attributes > 0)
    {
        fi_printf("{attributes}\n");
        INDENT++;
        print_data(cursor);
        INDENT--;
    }
    coda_cursor_goto_parent(cursor);

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
                    coda_Type *record_type;
                    int is_union;
                    long i;

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
                        fi_printf("[%s]", field_name);
                        if (print_offsets)
                        {
                            int64_t offset;

                            if (coda_cursor_get_file_bit_offset(cursor, &offset) != 0)
                            {
                                handle_coda_error();
                            }
                            if (offset >= 0)
                            {
                                char s[21];

                                coda_str64(offset >> 3, s);
                                ff_printf(":%s", s);
                                if ((offset & 0x7) != 0)
                                {
                                    ff_printf(":%d", (int)(offset & 0x7));
                                }
                            }
                        }
                        ff_printf("\n");
                        INDENT++;
                        print_data(cursor);
                        INDENT--;
                        coda_cursor_goto_parent(cursor);
                    }
                    else
                    {
                        if (coda_cursor_goto_first_record_field(cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        for (i = 0; i < num_fields; i++)
                        {
                            const char *field_name;

                            if (coda_type_get_record_field_name(record_type, i, &field_name) != 0)
                            {
                                handle_coda_error();
                            }
                            fi_printf("[%s]", field_name);
                            if (print_offsets)
                            {
                                int64_t offset;

                                if (coda_cursor_get_file_bit_offset(cursor, &offset) != 0)
                                {
                                    handle_coda_error();
                                }
                                if (offset >= 0)
                                {
                                    char s[21];

                                    coda_str64(offset >> 3, s);
                                    ff_printf(":%s", s);
                                    if ((offset & 0x7) != 0)
                                    {
                                        ff_printf(":%d", (int)(offset & 0x7));
                                    }
                                }
                            }
                            ff_printf("\n");
                            INDENT++;
                            print_data(cursor);
                            INDENT--;
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
                    int index[CODA_MAX_NUM_DIMS];
                    int i;

                    num_elements = 1;
                    for (i = 0; i < num_dims; i++)
                    {
                        num_elements *= dim[i];
                        index[i] = 0;
                    }
                    if (num_elements > 0)
                    {
                        if (coda_cursor_goto_first_array_element(cursor) != 0)
                        {
                            handle_coda_error();
                        }
                        for (i = 0; i < num_elements; i++)
                        {
                            int k;

                            fi_printf("(");
                            for (k = 0; k < num_dims; k++)
                            {
                                ff_printf("%d", index[k]);
                                if (k < num_dims - 1)
                                {
                                    ff_printf(",");
                                }
                            }
                            ff_printf(")");
                            if (print_offsets)
                            {
                                int64_t offset;

                                if (coda_cursor_get_file_bit_offset(cursor, &offset) != 0)
                                {
                                    handle_coda_error();
                                }
                                if (offset >= 0)
                                {
                                    char s[21];

                                    coda_str64(offset >> 3, s);
                                    ff_printf(":%s", s);
                                    if ((offset & 0x7) != 0)
                                    {
                                        ff_printf(":%d", (int)(offset & 0x7));
                                    }
                                }
                            }
                            ff_printf("\n");
                            INDENT++;
                            print_data(cursor);
                            INDENT--;

                            k = num_dims - 1;
                            while (k >= 0)
                            {
                                index[k]++;
                                if (index[k] == dim[k])
                                {
                                    index[k--] = 0;
                                }
                                else
                                {
                                    break;
                                }
                            }
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
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
        case coda_raw_class:
            {
                int has_ascii_content;

                if (coda_cursor_has_ascii_content(cursor, &has_ascii_content) != 0)
                {
                    handle_coda_error();
                }
                if (has_ascii_content)
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

                    fi_printf("\"");
                    print_escaped(data, length);
                    ff_printf("\" (length=%ld)\n", length);
                    free(data);
                }
                else
                {
                    coda_native_type read_type;

                    if (coda_cursor_get_read_type(cursor, &read_type) != 0)
                    {
                        handle_coda_error();
                    }
                    switch (read_type)
                    {
                        case coda_native_type_bytes:
                            {
                                int64_t bit_size;
                                int64_t byte_size;
                                uint8_t *data;
                                char s[21];

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

                                fi_printf("\"");
                                print_escaped((char *)data, (long)byte_size);
                                ff_printf("\" (size=");
                                coda_str64(bit_size >> 3, s);
                                ff_printf("%s", s);
                                if ((bit_size & 0x7) != 0)
                                {
                                    ff_printf(":%d", (int)(bit_size & 0x7));
                                }
                                ff_printf(")\n");

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

                                fi_printf("%ld\n", (long)data);
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

                                fi_printf("%lu\n", (unsigned long)data);
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
                                fi_printf("%s\n", s);
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
                                fi_printf("%s\n", s);
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

                                fi_printf("%g\n", data);
                            }
                            break;
                        case coda_native_type_char:
                        case coda_native_type_string:
                        case coda_native_type_not_available:
                            assert(0);
                            exit(1);
                    }
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

                fi_printf("<%s>\n", coda_type_get_special_type_name(special_type));
                if (coda_cursor_use_base_type_of_special_type(cursor) != 0)
                {
                    handle_coda_error();
                }
                INDENT++;
                print_data(cursor);
                INDENT--;
            }
            break;
    }
}

void print_debug_data()
{
    coda_ProductFile *pf;
    coda_Cursor cursor;
    coda_format format;
    int result;

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

    if (coda_get_product_format(pf, &format) != 0)
    {
        handle_coda_error();
    }
    print_offsets = (format == coda_format_ascii || format == coda_format_binary || format == coda_format_xml);

    if (coda_cursor_set_product(&cursor, pf) != 0)
    {
        handle_coda_error();
    }

    print_data(&cursor);

    coda_close(pf);
}
