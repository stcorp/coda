/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda.h"

/* internal CODA functions */
void coda_cursor_add_to_error_message(const coda_cursor *cursor);
int coda_cursor_print_path(const coda_cursor *cursor, int (*print) (const char *, ...));

const char *pre[] = { "< ", "> " };

int option_verbose;

static void print_version()
{
    printf("codacheck version %s\n", libcoda_version);
    printf("Copyright (C) 2007-2011 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codacmp [<options>] file1 file2\n");
    printf("        Compare contents of file1 and file2\n");
    printf("        Options:\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -V, --verbose\n");
    printf("                    show more information while performing the comparison\n");
    printf("\n");
    printf("    codacmp -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codacmp -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
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
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\v':
                printf("\\v");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\\':
                printf("\\\\");
                break;
            default:
                if (c >= 32 && c <= 126)
                {
                    printf("%c", c);
                }
                else
                {
                    printf("\\%03o", (int)(unsigned char)c);
                }
        }
    }
}

static void print_error_with_cursor(coda_cursor *cursor, int file_id)
{
    coda_cursor_add_to_error_message(cursor);
    printf("%sERROR: %s\n", pre[file_id - 1], coda_errno_to_string(coda_errno));
}

static int compare_data(coda_cursor *cursor1, coda_cursor *cursor2)
{
    coda_type_class type_class1;
    coda_type_class type_class2;

    if (coda_cursor_get_type_class(cursor1, &type_class1) != 0)
    {
        print_error_with_cursor(cursor1, 1);
        return -1;
    }
    if (coda_cursor_get_type_class(cursor2, &type_class2) != 0)
    {
        print_error_with_cursor(cursor2, 2);
        return -1;
    }

    if (type_class1 != type_class2)
    {
        printf("type differs at ");
        coda_cursor_print_path(cursor1, printf);
        printf("\n");
        if (option_verbose)
        {
            printf("%s%s\n", pre[0], coda_type_get_class_name(type_class1));
            printf("%s%s\n", pre[1], coda_type_get_class_name(type_class2));
        }
        return 0;
    }

    switch (type_class1)
    {
        case coda_array_class:
            {
                long num_elements1;
                long num_elements2;
                long i;

                if (coda_cursor_get_num_elements(cursor1, &num_elements1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_num_elements(cursor2, &num_elements2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                if (num_elements1 != num_elements2)
                {
                    printf("number of array elements differs at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    if (option_verbose)
                    {
                        printf("%s%ld\n", pre[0], num_elements1);
                        printf("%s%ld\n", pre[1], num_elements2);
                    }
                    return 0;
                }
                if (num_elements1 > 0)
                {
                    if (coda_cursor_goto_first_array_element(cursor1) != 0)
                    {
                        print_error_with_cursor(cursor1, 1);
                        return -1;
                    }
                    if (coda_cursor_goto_first_array_element(cursor2) != 0)
                    {
                        print_error_with_cursor(cursor2, 1);
                        return -1;
                    }
                    for (i = 0; i < num_elements1; i++)
                    {
                        if (compare_data(cursor1, cursor2) != 0)
                        {
                            return -1;
                        }
                        if (i < num_elements1 - 1)
                        {
                            if (coda_cursor_goto_next_array_element(cursor1) != 0)
                            {
                                print_error_with_cursor(cursor1, 1);
                                return -1;
                            }
                            if (coda_cursor_goto_next_array_element(cursor2) != 0)
                            {
                                print_error_with_cursor(cursor2, 2);
                                return -1;
                            }
                        }
                    }
                    coda_cursor_goto_parent(cursor1);
                    coda_cursor_goto_parent(cursor2);
                }
            }
            break;
        case coda_record_class:
            {
                coda_type *record_type1;
                coda_type *record_type2;
                int first_definition_mismatch;
                long num_elements1;
                long num_elements2;
                long index1;
                long index2;

                if (coda_cursor_get_type(cursor1, &record_type1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_type(cursor2, &record_type2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                if (coda_cursor_get_num_elements(cursor1, &num_elements1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_num_elements(cursor2, &num_elements2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }

                /* first perform structural comparison */
                first_definition_mismatch = 1;

                /* enumerate all elements of record #1 and try to find matching fields from record #2 */
                if (num_elements1 > 0)
                {
                    for (index1 = 0; index1 < num_elements1; index1++)
                    {
                        const char *field_name;

                        if (coda_type_get_record_field_name(record_type1, index1, &field_name) != 0)
                        {
                            print_error_with_cursor(cursor1, 1);
                            return -1;
                        }
                        if (coda_type_get_record_field_index_from_name(record_type2, field_name, &index2) != 0)
                        {
                            /* this field is not defined in record #2 */
                            if (first_definition_mismatch)
                            {
                                printf("definition differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                first_definition_mismatch = 0;
                            }
                            if (option_verbose)
                            {
                                printf("%scontains '%s'\n", pre[0], field_name);
                            }
                        }
                    }
                }

                /* now enumerate all elements of record #2 and see which fields were not present in record #1 */
                if (num_elements2 > 0)
                {
                    for (index2 = 0; index2 < num_elements2; index2++)
                    {
                        const char *field_name;

                        if (coda_type_get_record_field_name(record_type2, index2, &field_name) != 0)
                        {
                            print_error_with_cursor(cursor2, 2);
                            return -1;
                        }
                        if (coda_type_get_record_field_index_from_name(record_type1, field_name, &index1) != 0)
                        {
                            /* this field is not defined in record #1 */
                            if (first_definition_mismatch)
                            {
                                printf("definition differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                first_definition_mismatch = 0;
                            }
                            if (option_verbose)
                            {
                                printf("%scontains '%s'\n", pre[1], field_name);
                            }
                        }
                    }
                }

                /* perform content and availability comparison */
                if (num_elements1 > 0)
                {
                    coda_cursor record_cursor1;

                    record_cursor1 = *cursor1;
                    if (coda_cursor_goto_first_record_field(cursor1) != 0)
                    {
                        print_error_with_cursor(cursor1, 1);
                        return -1;
                    }
                    for (index1 = 0; index1 < num_elements1; index1++)
                    {
                        const char *field_name;
                        int available1;

                        if (coda_cursor_get_record_field_available_status(&record_cursor1, index1, &available1) != 0)
                        {
                            print_error_with_cursor(&record_cursor1, 1);
                            return -1;
                        }
                        if (coda_type_get_record_field_name(record_type1, index1, &field_name) != 0)
                        {
                            print_error_with_cursor(cursor1, 1);
                            return -1;
                        }
                        if (coda_type_get_record_field_index_from_name(record_type2, field_name, &index2) == 0)
                        {
                            int available2;

                            /* field is defined for both records */
                            if (coda_cursor_get_record_field_available_status(cursor2, index2, &available2) != 0)
                            {
                                print_error_with_cursor(cursor2, 2);
                                return -1;
                            }
                            if (available1)
                            {
                                if (available2)
                                {
                                    if (coda_cursor_goto_record_field_by_index(cursor2, index2) != 0)
                                    {
                                        print_error_with_cursor(cursor2, 2);
                                        return -1;
                                    }
                                    if (compare_data(cursor1, cursor2) != 0)
                                    {
                                        return -1;
                                    }
                                    coda_cursor_goto_parent(cursor2);
                                }
                                else
                                {
                                    /* this field is only available in record #1 */
                                    printf("availability differs at ");
                                    coda_cursor_print_path(cursor1, printf);
                                    printf("\n");
                                    if (option_verbose)
                                    {
                                        printf("%savailable\n", pre[0]);
                                        printf("%snot available\n", pre[1]);
                                    }
                                }
                            }
                            else if (available2)
                            {
                                /* this field is only available in record #2 */
                                printf("availability differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                if (option_verbose)
                                {
                                    printf("%snot available\n", pre[0]);
                                    printf("%savailable\n", pre[1]);
                                }
                            }
                        }
                        if (index1 < num_elements1 - 1)
                        {
                            if (coda_cursor_goto_next_record_field(cursor1) != 0)
                            {
                                print_error_with_cursor(cursor1, 1);
                                return -1;
                            }
                        }
                    }
                    coda_cursor_goto_parent(cursor1);
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
            {
                coda_native_type read_type1;
                coda_native_type read_type2;

                if (coda_cursor_get_read_type(cursor1, &read_type1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_read_type(cursor2, &read_type2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                if (read_type1 != read_type2)
                {
                    printf("native type differs at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    if (option_verbose)
                    {
                        printf("%s%s\n", pre[0], coda_type_get_native_type_name(read_type1));
                        printf("%s%s\n", pre[1], coda_type_get_native_type_name(read_type2));
                    }
                    return 0;
                }

                switch (read_type1)
                {
                    case coda_native_type_int8:
                    case coda_native_type_int16:
                    case coda_native_type_int32:
                    case coda_native_type_int64:
                        {
                            int64_t value1;
                            int64_t value2;

                            if (coda_cursor_read_int64(cursor1, &value1) != 0)
                            {
                                print_error_with_cursor(cursor1, 1);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (coda_cursor_read_int64(cursor2, &value2) != 0)
                            {
                                print_error_with_cursor(cursor2, 2);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (value1 != value2)
                            {
                                printf("value differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                if (option_verbose)
                                {
                                    char s[21];

                                    coda_str64(value1, s);
                                    printf("%s%s\n", pre[0], s);
                                    coda_str64(value2, s);
                                    printf("%s%s\n", pre[1], s);
                                }
                                return 0;
                            }
                        }
                        break;
                    case coda_native_type_uint8:
                    case coda_native_type_uint16:
                    case coda_native_type_uint32:
                    case coda_native_type_uint64:
                        {
                            uint64_t value1;
                            uint64_t value2;

                            if (coda_cursor_read_uint64(cursor1, &value1) != 0)
                            {
                                print_error_with_cursor(cursor1, 1);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (coda_cursor_read_uint64(cursor2, &value2) != 0)
                            {
                                print_error_with_cursor(cursor2, 2);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (value1 != value2)
                            {
                                printf("value differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                if (option_verbose)
                                {
                                    char s[21];

                                    coda_str64u(value1, s);
                                    printf("%s%s\n", pre[0], s);
                                    coda_str64u(value2, s);
                                    printf("%s%s\n", pre[1], s);
                                }
                                return 0;
                            }
                        }
                        break;
                    case coda_native_type_float:
                    case coda_native_type_double:
                        {
                            double value1;
                            double value2;

                            if (coda_cursor_read_double(cursor1, &value1) != 0)
                            {
                                print_error_with_cursor(cursor1, 1);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (coda_cursor_read_double(cursor2, &value2) != 0)
                            {
                                print_error_with_cursor(cursor2, 2);
                                if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                                {
                                    return -1;
                                }
                            }
                            else if (value1 != value2 && !(coda_isNaN(value1) && coda_isNaN(value2)))
                            {
                                printf("value differs at ");
                                coda_cursor_print_path(cursor1, printf);
                                printf("\n");
                                if (option_verbose)
                                {
                                    printf("%s%.15g\n", pre[0], value1);
                                    printf("%s%.15g\n", pre[1], value2);
                                }
                                return 0;
                            }
                        }
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_text_class:
            {
                char *str1;
                char *str2;
                long length1;
                long length2;

                if (coda_cursor_get_string_length(cursor1, &length1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_string_length(cursor2, &length2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                str1 = (char *)malloc(length1 + 1);
                if (str1 == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   length1, __FILE__, __LINE__);
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                str2 = (char *)malloc(length2 + 1);
                if (str2 == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   length2, __FILE__, __LINE__);
                    print_error_with_cursor(cursor2, 2);
                    free(str1);
                    return -1;
                }
                if (coda_cursor_read_string(cursor1, str1, length1 + 1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                    {
                        free(str1);
                        free(str2);
                        return -1;
                    }
                }
                else if (coda_cursor_read_string(cursor2, str2, length2 + 1) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                    {
                        free(str1);
                        free(str2);
                        return -1;
                    }
                }
                else if (memcmp(str1, str2, length1) != 0)
                {
                    printf("string value differs at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    if (option_verbose)
                    {
                        printf("%s%s\n", pre[0], str1);
                        printf("%s%s\n", pre[1], str2);
                    }
                }
                free(str1);
                free(str2);
            }
            break;
        case coda_raw_class:
            {
                int64_t byte_size1;
                int64_t byte_size2;
                int64_t bit_size1;
                int64_t bit_size2;

                if (coda_cursor_get_bit_size(cursor1, &bit_size1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                byte_size1 = (bit_size1 >> 3) + ((bit_size1 & 0x7) != 0 ? 1 : 0);
                if (coda_cursor_get_bit_size(cursor2, &bit_size2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                byte_size2 = (bit_size2 >> 3) + ((bit_size2 & 0x7) != 0 ? 1 : 0);
                if (bit_size1 != bit_size2)
                {
                    printf("data size differs at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    if (option_verbose)
                    {
                        char s[21];

                        coda_str64(bit_size1, s);
                        printf("%s%s bits\n", pre[0], s);
                        coda_str64(bit_size2, s);
                        printf("%s%s bits\n", pre[1], s);
                    }
                    return 0;
                }
                if (bit_size1 > 0)
                {
                    uint8_t *value1;
                    uint8_t *value2;

                    value1 = (uint8_t *)malloc((size_t)byte_size1);
                    if (value1 == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       (long)byte_size1, __FILE__, __LINE__);
                        print_error_with_cursor(cursor1, 1);
                        return -1;
                    }
                    value2 = (uint8_t *)malloc((size_t)byte_size2);
                    if (value2 == NULL)
                    {
                        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                       (long)byte_size2, __FILE__, __LINE__);
                        print_error_with_cursor(cursor2, 2);
                        free(value1);
                        return -1;
                    }
                    if (coda_cursor_read_bits(cursor1, value1, 0, bit_size1) != 0)
                    {
                        print_error_with_cursor(cursor1, 1);
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            free(value1);
                            free(value2);
                            return -1;
                        }
                    }
                    else if (coda_cursor_read_bits(cursor2, value2, 0, bit_size2) != 0)
                    {
                        print_error_with_cursor(cursor2, 2);
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            free(value1);
                            free(value2);
                            return -1;
                        }
                    }
                    else if (memcmp(value1, value2, (size_t)byte_size1) != 0)
                    {
                        printf("data differs at ");
                        coda_cursor_print_path(cursor1, printf);
                        printf("\n");
                        if (option_verbose && byte_size1 <= 256)
                        {
                            printf("%s", pre[0]);
                            print_escaped((char *)value1, (long)byte_size1);
                            printf("\n");
                            printf("%s", pre[1]);
                            print_escaped((char *)value2, (long)byte_size2);
                            printf("\n");
                        }
                    }
                    free(value1);
                    free(value2);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type1;
                coda_special_type special_type2;

                if (coda_cursor_get_special_type(cursor1, &special_type1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_get_special_type(cursor2, &special_type2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                if (special_type1 != special_type2)
                {
                    printf("special type differs at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    if (option_verbose)
                    {
                        printf("%s%s\n", pre[0], coda_type_get_special_type_name(special_type1));
                        printf("%s%s\n", pre[1], coda_type_get_special_type_name(special_type2));
                    }
                    return 0;
                }
                if (coda_cursor_use_base_type_of_special_type(cursor1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
                if (coda_cursor_use_base_type_of_special_type(cursor2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
                if (compare_data(cursor1, cursor2) != 0)
                {
                    return -1;
                }
            }
            break;
    }

    /* check attributes */
    {
        long num_elements1;
        long num_elements2;

        if (coda_cursor_goto_attributes(cursor1) != 0)
        {
            print_error_with_cursor(cursor1, 1);
            return -1;
        }
        if (coda_cursor_goto_attributes(cursor2) != 0)
        {
            print_error_with_cursor(cursor2, 2);
            return -1;
        }
        if (coda_cursor_get_num_elements(cursor1, &num_elements1) != 0)
        {
            print_error_with_cursor(cursor1, 1);
            return -1;
        }
        if (coda_cursor_get_num_elements(cursor2, &num_elements2) != 0)
        {
            print_error_with_cursor(cursor2, 2);
            return -1;
        }
        if (num_elements1 > 0 || num_elements2 > 0)
        {
            if (compare_data(cursor1, cursor2) != 0)
            {
                return -1;
            }
        }
        coda_cursor_goto_parent(cursor1);
        coda_cursor_goto_parent(cursor2);
    }

    return 0;
}

static int compare_files(char *filename1, char *filename2)
{
    coda_product *pf1;
    coda_product *pf2;
    coda_cursor cursor1;
    coda_cursor cursor2;
    int result;

    result = coda_open(filename1, &pf1);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(filename1, &pf1);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        printf("%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
        return -1;
    }

    result = coda_open(filename2, &pf2);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        coda_set_option_use_mmap(0);
        result = coda_open(filename2, &pf2);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        printf("%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
        coda_close(pf1);
        return -1;
    }

    if (coda_cursor_set_product(&cursor1, pf1) != 0)
    {
        coda_close(pf1);
        coda_close(pf2);
        return -1;
    }

    if (coda_cursor_set_product(&cursor2, pf2) != 0)
    {
        coda_close(pf1);
        coda_close(pf2);
        return -1;
    }

    result = compare_data(&cursor1, &cursor2);

    coda_close(pf1);
    coda_close(pf2);

    return result;
}

int main(int argc, char **argv)
{
#ifdef WIN32
    const char *definition_path = "../definitions";
#else
    const char *definition_path = "../share/" PACKAGE "/definitions";
#endif
    int perform_conversions;
    int result;
    int i;

    option_verbose = 0;
    perform_conversions = 1;

    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_help();
        exit(0);
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
    {
        print_version();
        exit(0);
    }

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            option_verbose = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if (argv[i][0] != '-')
        {
            /* assume all arguments from here on are files */
            break;
        }
        else
        {
            fprintf(stderr, "ERROR: Incorrect arguments\n");
            print_help();
            exit(1);
        }
    }

    if (i != argc - 2)
    {
        /* we expect two filenames for the last two arguments */
        fprintf(stderr, "ERROR: Incorrect arguments\n");
        print_help();
        exit(1);
    }

    if (coda_set_definition_path_conditional(argv[0], NULL, definition_path) != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    /* The codacheck program should never navigate beyond the array bounds.
     * We therefore disable to boundary check option to increase performance.
     * Mind that this option does not influence the out-of-bounds check that CODA performs to ensure
     * that a read is performed using a byte offset/size that is within the limits of the total file size.
     */
    coda_set_option_perform_boundary_checks(0);

    coda_set_option_perform_conversions(perform_conversions);

    /* compare files */
    result = compare_files(argv[argc - 2], argv[argc - 1]);

    coda_done();

    if (result != 0)
    {
        exit(1);
    }

    return 0;
}
