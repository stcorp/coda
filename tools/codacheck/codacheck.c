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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "coda.h"

/* internal CODA function */
void coda_cursor_add_to_error_message(const coda_cursor *cursor);

int option_verbose;
int option_quick;
int found_errors;

static void print_version()
{
    printf("codacheck version %s\n", libcoda_version);
    printf("Copyright (C) 2007-2012 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codacheck [<options>] <files>\n");
    printf("        Provide a basic sanity check on product files supported by CODA\n");
    printf("        Options:\n");
    printf("            -q, --quick\n");
    printf("                    only perform a quick check of the product\n");
    printf("                    (do not traverse the full product)\n");
    printf("            -V, --verbose\n");
    printf("                    show more information while performing the check\n");
    printf("\n");
    printf("        If you pass a '-' for the <files> section then the list of files will\n");
    printf("        be read from stdin.\n");
    printf("\n");
    printf("    codacheck -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codacheck -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
}

static void print_error_with_cursor(coda_cursor *cursor)
{
    coda_cursor_add_to_error_message(cursor);
    printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
    found_errors = 1;
}

static int quick_size_check(coda_product *product)
{
    coda_cursor cursor;
    int64_t real_file_size;
    int64_t calculated_file_size;
    int prev_option_value;

    if (coda_get_product_file_size(product, &real_file_size) != 0)
    {
        return -1;
    }
    real_file_size <<= 3;       /* now in bits */

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }

    /* we explicitly disable the use of fast size expressions because we
     * also want to verify the structural integrity within each record.
     */
    prev_option_value = coda_get_option_use_fast_size_expressions();
    coda_set_option_use_fast_size_expressions(0);
    if (coda_cursor_get_bit_size(&cursor, &calculated_file_size) != 0)
    {
        print_error_with_cursor(&cursor);
    }
    coda_set_option_use_fast_size_expressions(prev_option_value);

    if (real_file_size != calculated_file_size)
    {
        char s1[21];
        char s2[21];

        coda_str64(real_file_size >> 3, s1);
        coda_str64(calculated_file_size >> 3, s2);
        if (calculated_file_size & 0x7)
        {
            printf("  ERROR: incorrect file size (actual size: %s, calculated: %s:%d)\n", s1, s2,
                   (int)(calculated_file_size & 0x7));
        }
        else
        {
            printf("  ERROR: incorrect file size (actual size: %s, calculated: %s)\n", s1, s2);
        }
        found_errors = 1;
    }

    return 0;
}

static int check_data(coda_cursor *cursor, int64_t *bit_size)
{
    coda_type_class type_class;
    int has_attributes;

    if (coda_cursor_get_type_class(cursor, &type_class) != 0)
    {
        print_error_with_cursor(cursor);
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
                    print_error_with_cursor(cursor);
                    return -1;
                }
                if (num_elements > 0)
                {
                    if (coda_cursor_goto_first_array_element(cursor) != 0)
                    {
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        if (bit_size != NULL)
                        {
                            int64_t element_size;

                            if (check_data(cursor, &element_size) != 0)
                            {
                                return -1;
                            }
                            *bit_size += element_size;
                        }
                        else
                        {
                            if (check_data(cursor, NULL) != 0)
                            {
                                return -1;
                            }
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_array_element(cursor) != 0)
                            {
                                print_error_with_cursor(cursor);
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
                    print_error_with_cursor(cursor);
                    return -1;
                }
                if (num_elements > 0)
                {
                    if (coda_cursor_goto_first_record_field(cursor) != 0)
                    {
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                    for (i = 0; i < num_elements; i++)
                    {
                        if (bit_size != NULL)
                        {
                            int64_t element_size;

                            if (check_data(cursor, &element_size) != 0)
                            {
                                return -1;
                            }
                            *bit_size += element_size;
                        }
                        else
                        {
                            if (check_data(cursor, NULL) != 0)
                            {
                                return -1;
                            }
                        }
                        if (i < num_elements - 1)
                        {
                            if (coda_cursor_goto_next_record_field(cursor) != 0)
                            {
                                print_error_with_cursor(cursor);
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
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                }
                if (coda_cursor_read_double(cursor, &value) != 0)
                {
                    print_error_with_cursor(cursor);
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                    {
                        return -1;
                    }
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
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                }
                if (coda_cursor_get_string_length(cursor, &string_length) != 0)
                {
                    print_error_with_cursor(cursor);
                    if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                    {
                        return -1;
                    }
                    /* if we can't determine the string length, don't try to read the data */
                    break;
                }
                if (string_length < 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "string length is negative");
                    print_error_with_cursor(cursor);
                    /* if we can't determine a proper string length, don't try to read the data */
                    break;
                }

                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    print_error_with_cursor(cursor);
                    return -1;
                }
                if (coda_type_get_fixed_value(type, &fixed_value, &fixed_value_length) != 0)
                {
                    print_error_with_cursor(cursor);
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
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                    if (coda_cursor_read_string(cursor, data, string_length + 1) != 0)
                    {
                        print_error_with_cursor(cursor);
                        free(data);
                        return -1;
                    }
                }

                if (fixed_value != NULL)
                {
                    if (string_length != fixed_value_length)
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "string data does not match fixed value (length differs)");
                        print_error_with_cursor(cursor);
                        /* we do not return -1; we can just continue checking the rest of the file */
                    }
                    else if (string_length > 0)
                    {
                        if (memcmp(data, fixed_value, fixed_value_length) != 0)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "string data does not match fixed value");
                            print_error_with_cursor(cursor);
                            /* we do not return -1; we can just continue checking the rest of the file */
                        }
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
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                    local_bit_size = *bit_size;
                }
                else
                {
                    if (coda_cursor_get_bit_size(cursor, &local_bit_size) != 0)
                    {
                        print_error_with_cursor(cursor);
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            return -1;
                        }
                        /* if we can't determine the bit size, don't try to read the data */
                        break;
                    }
                }
                if (local_bit_size < 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT, "bit size is negative");
                    print_error_with_cursor(cursor);
                    /* if we can't determine a proper size, don't try to read the data */
                    break;
                }
                byte_size = (local_bit_size >> 3) + (local_bit_size & 0x7 ? 1 : 0);

                if (coda_cursor_get_type(cursor, &type) != 0)
                {
                    print_error_with_cursor(cursor);
                    return -1;
                }
                if (coda_type_get_fixed_value(type, &fixed_value, &fixed_value_length) != 0)
                {
                    print_error_with_cursor(cursor);
                    return -1;
                }
                if (fixed_value != NULL)
                {
                    if (byte_size != fixed_value_length)
                    {
                        coda_set_error(CODA_ERROR_PRODUCT, "data does not match fixed value (length differs)");
                        print_error_with_cursor(cursor);
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
                            print_error_with_cursor(cursor);
                            return -1;
                        }
                        if (coda_cursor_read_bits(cursor, data, 0, local_bit_size) != 0)
                        {
                            print_error_with_cursor(cursor);
                            free(data);
                            return -1;
                        }
                        if (memcmp(data, fixed_value, fixed_value_length) != 0)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "data does not match fixed value");
                            print_error_with_cursor(cursor);
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
                    print_error_with_cursor(cursor);
                    return -1;
                }

                if (special_type == coda_special_time)
                {
                    double value;

                    /* try to read the time value as a double */
                    if (coda_cursor_read_double(cursor, &value) != 0)
                    {
                        print_error_with_cursor(cursor);
                        if (coda_errno != CODA_ERROR_PRODUCT && coda_errno != CODA_ERROR_INVALID_FORMAT)
                        {
                            return -1;
                        }
                        /* just continue checking the remaining file */
                    }
                }

                if (special_type != coda_special_no_data)
                {
                    if (coda_cursor_use_base_type_of_special_type(cursor) != 0)
                    {
                        print_error_with_cursor(cursor);
                        return -1;
                    }
                    if (check_data(cursor, bit_size) != 0)
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
        print_error_with_cursor(cursor);
        return -1;
    }
    if (has_attributes)
    {
        if (coda_cursor_goto_attributes(cursor) != 0)
        {
            print_error_with_cursor(cursor);
            return -1;
        }
        if (check_data(cursor, NULL) != 0)
        {
            return -1;
        }
        coda_cursor_goto_parent(cursor);
    }

    return 0;
}

static int size_and_read_check(coda_product *product)
{
    coda_cursor cursor;
    int64_t real_file_size;
    int64_t calculated_file_size;

    if (coda_get_product_file_size(product, &real_file_size) != 0)
    {
        return -1;
    }
    real_file_size <<= 3;       /* now in bits */

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }

    if (check_data(&cursor, &calculated_file_size) != 0)
    {
        /* The error is already printed, so return success */
        return 0;
    }

    if (real_file_size != calculated_file_size)
    {
        char s1[21];
        char s2[21];

        coda_str64(real_file_size >> 3, s1);
        coda_str64(calculated_file_size >> 3, s2);
        if (calculated_file_size & 0x7)
        {
            printf("  ERROR: incorrect file size (actual size: %s, calculated: %s:%d)\n", s1, s2,
                   (int)(calculated_file_size & 0x7));
        }
        else
        {
            printf("  ERROR: incorrect file size (actual size: %s, calculated: %s)\n", s1, s2);
        }
        found_errors = 1;
    }

    return 0;
}

static int read_check(coda_product *product)
{
    coda_cursor cursor;

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        return -1;
    }

    check_data(&cursor, NULL);

    /* The error was already printed if there was an error, so just return success */
    return 0;
}

static void check_file(char *filename)
{
    coda_product *product;
    coda_format format;
    int64_t file_size;
    const char *product_class;
    const char *product_type;
    int version;
    int result;

    printf("%s\n", filename);

    if (coda_recognize_file(filename, &file_size, &format, &product_class, &product_type, &version) != 0)
    {
        printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
        coda_set_error(CODA_SUCCESS, NULL);
        found_errors = 1;
        return;
    }

    if (option_verbose)
    {
        printf("  product format: %s", coda_type_get_format_name(format));
        if (product_class != NULL && product_type != NULL)
        {
            printf(" %s/%s v%d", product_class, product_type, version);
        }
        printf("\n");
    }

    result = coda_open(filename, &product);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(filename, &product);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
        coda_set_error(CODA_SUCCESS, NULL);
        found_errors = 1;
        return;
    }

    switch (format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            if (option_quick)
            {
                if (quick_size_check(product) != 0)
                {
                    printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
                    coda_close(product);
                    return;
                }
            }
            else
            {
                if (size_and_read_check(product) != 0)
                {
                    printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
                    coda_close(product);
                    return;
                }
            }
            break;
        default:
            if (!option_quick)
            {
                if (read_check(product) != 0)
                {
                    printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
                    coda_close(product);
                    return;
                }
            }
            break;
    }

    if (coda_close(product) != 0)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        return;
    }

    printf("\n");
}

int main(int argc, char *argv[])
{
#ifdef WIN32
    const char *definition_path = "../definitions";
#else
    const char *definition_path = "../share/" PACKAGE "/definitions";
#endif
    int option_stdin;
    int i;

    option_stdin = 0;
    option_verbose = 0;
    option_quick = 0;

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
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quick") == 0)
        {
            option_quick = 1;
        }
        else if (strcmp(argv[i], "-") == 0 && i == argc - 1)
        {
            option_stdin = 1;
            break;
        }
        else if (argv[i][0] != '-')
        {
            /* assume all arguments from here on are files */
            break;
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
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

    /* We disable conversions since this speeds up the check of reading all numerical data */
    coda_set_option_perform_conversions(0);

    if (option_stdin)
    {
        char filename[1000];
        char c;

        do
        {
            int k;

            k = 0;
            for (;;)
            {
                c = getchar();
                if (c == '\r')
                {
                    char c2;

                    c2 = getchar();
                    /* test for '\r\n' combination */
                    if (c2 != '\n')
                    {
                        /* if the second char is not '\n' put it back in the read buffer */
                        ungetc(c2, stdin);
                    }
                }
                if (c == EOF || c == '\n' || c == '\r')
                {
                    filename[k] = '\0';
                    break;
                }
                filename[k] = c;
                k++;
                assert(k < 1000);
            }
            if (k > 0)
            {
                check_file(filename);
                fflush(NULL);
            }
        } while (c != EOF);
    }
    else
    {
        int k;

        for (k = i; k < argc; k++)
        {
            check_file(argv[k]);
            fflush(NULL);
        }
    }

    coda_done();

    if (found_errors)
    {
        exit(1);
    }

    return 0;
}
