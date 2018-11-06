/*
 * Copyright (C) 2007-2018 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda.h"
#include "coda-tree.h"
#include "hashtable.h"

#define BLOCK_SIZE 16

/* internal CODA functions */
void coda_cursor_add_to_error_message(const coda_cursor *cursor);
int coda_cursor_print_path(const coda_cursor *cursor, int (*print) (const char *, ...));

const char *pre[] = { "< ", "> " };

int option_verbose;

struct
{
    int num_keys;
    const char **path;
    const char **key_expr;
    coda_tree_node *tree;
} array_key_info;

static void print_version()
{
    printf("codacheck version %s\n", libcoda_version);
    printf("Copyright (C) 2007-2018 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codacmp [-D definitionpath] [<options>] file1 file2\n");
    printf("        Compare contents of file1 and file2\n");
    printf("        Options:\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -p, --path <path>\n");
    printf("                    path (in the form of a CODA node expression) to the\n");
    printf("                    location in the product where the comparison should begin.\n");
    printf("                    This path should be available in both products. If this\n");
    printf("                    parameter is not provided the full products are compared.\n");
    printf("            -k, --key <path_to_array> <key_string_expr>\n");
    printf("                    for the given array in the product use the string\n");
    printf("                    expression as a unique key to line up the array elements in\n");
    printf("                    the two products. The array elements will then be compared\n");
    printf("                    as if it were record fields where the 'key' is used as the\n");
    printf("                    field name. This option can be provided multiple times (for\n");
    printf("                    different paths).\n");
    printf("            -V, --verbose\n");
    printf("                    show more information while performing the comparison\n");
    printf("\n");
    printf("    codacmp -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codacmp -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
    printf("    CODA will look for .codadef files using a definition path, which is a ':'\n");
    printf("    separated (';' on Windows) list of paths to .codadef files and/or to\n");
    printf("    directories containing .codadef files.\n");
    printf("    By default the definition path is set to a single directory relative to\n");
    printf("    the tool location. A different definition path can be set via the\n");
    printf("    CODA_DEFINITION environment variable or via the -D option.\n");
    printf("    (the -D option overrides the environment variable setting).\n");
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
    fprintf(stderr, "%sERROR: %s\n", pre[file_id - 1], coda_errno_to_string(coda_errno));
}

static void array_key_info_init(void)
{
    array_key_info.num_keys = 0;
    array_key_info.path = NULL;
    array_key_info.key_expr = NULL;
    array_key_info.tree = NULL;
}

static void array_key_info_done(void)
{
    if (array_key_info.path != NULL)
    {
        free(array_key_info.path);
    }
    if (array_key_info.key_expr != NULL)
    {
        free(array_key_info.key_expr);
    }
    if (array_key_info.tree != NULL)
    {
        coda_tree_node_delete(array_key_info.tree, NULL);
    }
}

static int array_key_info_add_key(const char *path, const char *key_expr)
{
    if (array_key_info.num_keys % BLOCK_SIZE == 0)
    {
        const char **new_path;
        const char **new_key_expr;

        new_path = realloc(array_key_info.path, (array_key_info.num_keys + BLOCK_SIZE) * sizeof(char *));
        if (new_path == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (array_key_info.num_keys + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        array_key_info.path = new_path;

        new_key_expr = realloc(array_key_info.key_expr, (array_key_info.num_keys + BLOCK_SIZE) * sizeof(char *));
        if (new_key_expr == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (array_key_info.num_keys + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        array_key_info.key_expr = new_key_expr;
    }
    array_key_info.path[array_key_info.num_keys] = path;
    array_key_info.key_expr[array_key_info.num_keys] = key_expr;
    array_key_info.num_keys++;

    return 0;
}

static int array_key_info_set_product(coda_product *product)
{
    coda_type *type;
    int i;

    if (array_key_info.num_keys == 0)
    {
        return 0;
    }

    if (coda_get_product_root_type(product, &type) != 0)
    {
        return -1;
    }
    array_key_info.tree = coda_tree_node_new(type);
    if (array_key_info.tree == NULL)
    {
        return -1;
    }

    for (i = 0; i < array_key_info.num_keys; i++)
    {
        if (coda_tree_node_add_item_for_path(array_key_info.tree, array_key_info.path[i],
                                             (void *)array_key_info.key_expr[i], 0) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int compare_data(coda_cursor *cursor1, coda_cursor *cursor2);

static int compare_arrays_as_records_sub(coda_cursor *cursor1, coda_cursor *cursor2, coda_expression *expr,
                                         long num_elements1, long num_elements2, char **keys1, char **keys2,
                                         hashtable *table1, hashtable *table2)
{
    int first_definition_mismatch;
    long index1;
    long index2;

    if (num_elements1 > 0)
    {
        if (coda_cursor_goto_first_array_element(cursor1) != 0)
        {
            print_error_with_cursor(cursor1, 1);
            return -1;
        }
        for (index1 = 0; index1 < num_elements1; index1++)
        {
            long length;

            if (coda_expression_eval_string(expr, cursor1, &keys1[index1], &length) != 0)
            {
                fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
                return -1;
            }
            if (hashtable_add_name(table1, keys1[index1]) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "multiple occurrence of array key '%s'", keys1[index1]);
                fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
                return -1;
            }
            if (index1 < num_elements1 - 1)
            {
                if (coda_cursor_goto_next_array_element(cursor1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
            }
        }
        coda_cursor_goto_parent(cursor1);
    }
    if (num_elements2 > 0)
    {
        if (coda_cursor_goto_first_array_element(cursor2) != 0)
        {
            print_error_with_cursor(cursor2, 2);
            return -1;
        }
        for (index2 = 0; index2 < num_elements2; index2++)
        {
            long length;

            if (coda_expression_eval_string(expr, cursor2, &keys2[index2], &length) != 0)
            {
                fprintf(stderr, "%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
                return -1;
            }
            if (hashtable_add_name(table2, keys2[index2]) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "multiple occurrence of array key '%s'", keys2[index2]);
                fprintf(stderr, "%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
                return -1;
            }
            if (index2 < num_elements2 - 1)
            {
                if (coda_cursor_goto_next_array_element(cursor2) != 0)
                {
                    print_error_with_cursor(cursor2, 2);
                    return -1;
                }
            }
        }
        coda_cursor_goto_parent(cursor2);
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
    }

    /* first perform structural comparison */
    first_definition_mismatch = 1;

    /* enumerate all elements of array #1 and try to find matching elements from array #2 */
    if (num_elements1 > 0)
    {
        for (index1 = 0; index1 < num_elements1; index1++)
        {
            if (hashtable_get_index_from_name(table2, keys1[index1]) < 0)
            {
                /* this field is not defined in record #2 */
                if (first_definition_mismatch)
                {
                    printf("array elements differ at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    first_definition_mismatch = 0;
                }
                if (option_verbose)
                {
                    printf("%scontains array element with key '%s'\n", pre[0], keys1[index1]);
                }
            }
        }
    }

    /* now enumerate all elements of record #2 and see which fields were not present in record #1 */
    if (num_elements2 > 0)
    {
        for (index2 = 0; index2 < num_elements2; index2++)
        {
            if (hashtable_get_index_from_name(table1, keys2[index2]) < 0)
            {
                /* this field is not defined in record #1 */
                if (first_definition_mismatch)
                {
                    printf("array elements differ at ");
                    coda_cursor_print_path(cursor1, printf);
                    printf("\n");
                    first_definition_mismatch = 0;
                }
                if (option_verbose)
                {
                    printf("%scontains array element with key '%s'\n", pre[1], keys2[index2]);
                }
            }
        }
    }

    /* perform content and availability comparison */
    if (num_elements1 > 0)
    {
        if (coda_cursor_goto_first_array_element(cursor1) != 0)
        {
            print_error_with_cursor(cursor1, 1);
            return -1;
        }
        for (index1 = 0; index1 < num_elements1; index1++)
        {
            index2 = hashtable_get_index_from_name(table2, keys1[index1]);
            if (index2 >= 0)
            {
                if (coda_cursor_goto_array_element_by_index(cursor2, index2) != 0)
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
            if (index1 < num_elements1 - 1)
            {
                if (coda_cursor_goto_next_array_element(cursor1) != 0)
                {
                    print_error_with_cursor(cursor1, 1);
                    return -1;
                }
            }
        }
        coda_cursor_goto_parent(cursor1);
    }

    return 0;
}

static int compare_arrays_as_records(coda_cursor *cursor1, coda_cursor *cursor2, const char *key_expr)
{
    coda_expression *expr = NULL;
    hashtable *table1 = NULL;
    hashtable *table2 = NULL;
    char **keys1 = NULL;
    char **keys2 = NULL;
    long num_elements1;
    long num_elements2;
    long i;
    int result;

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

    if (coda_expression_from_string(key_expr, &expr) != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        return -1;
    }

    keys1 = malloc(num_elements1 * sizeof(const char *));
    if (keys1 == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_elements1 * sizeof(const char *), __FILE__, __LINE__);
        print_error_with_cursor(cursor1, 1);
        coda_expression_delete(expr);
        return -1;
    }
    for (i = 0; i < num_elements1; i++)
    {
        keys1[i] = NULL;
    }
    keys2 = malloc(num_elements2 * sizeof(const char *));
    if (keys2 == NULL)
    {

        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       num_elements2 * sizeof(const char *), __FILE__, __LINE__);
        print_error_with_cursor(cursor2, 2);
        coda_expression_delete(expr);
        free(keys1);
        return -1;
    }
    for (i = 0; i < num_elements2; i++)
    {
        keys2[i] = NULL;
    }
    table1 = hashtable_new(1);
    if (table1 == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        print_error_with_cursor(cursor1, 1);
        coda_expression_delete(expr);
        free(keys1);
        free(keys2);
        return -1;
    }
    table2 = hashtable_new(1);
    if (table2 == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        print_error_with_cursor(cursor2, 2);
        coda_expression_delete(expr);
        hashtable_delete(table1);
        free(keys1);
        free(keys2);
        return -1;
    }

    result = compare_arrays_as_records_sub(cursor1, cursor2, expr, num_elements1, num_elements2, keys1, keys2, table1,
                                           table2);

    coda_expression_delete(expr);
    hashtable_delete(table1);
    hashtable_delete(table2);
    for (i = 0; i < num_elements1; i++)
    {
        if (keys1[i] != NULL)
        {
            coda_free(keys1[i]);
        }
    }
    free(keys1);
    for (i = 0; i < num_elements2; i++)
    {
        if (keys2[i] != NULL)
        {
            coda_free(keys2[i]);
        }
    }
    free(keys2);

    return result;
}

static int compare_arrays(coda_cursor *cursor1, coda_cursor *cursor2)
{
    const char *key_expr;
    long num_elements1;
    long num_elements2;
    long i;

    if (coda_tree_node_get_item_for_cursor(array_key_info.tree, cursor1, (void **)&key_expr) != 0)
    {
        print_error_with_cursor(cursor1, 1);
        return -1;
    }
    if (key_expr != NULL)
    {
        return compare_arrays_as_records(cursor1, cursor2, key_expr);
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

    return 0;
}

static int compare_records(coda_cursor *cursor1, coda_cursor *cursor2)
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

    return 0;
}

static int compare_numbers(coda_cursor *cursor1, coda_cursor *cursor2)
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

    return 0;
}

static int compare_strings(coda_cursor *cursor1, coda_cursor *cursor2)
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
    else if (length1 != length2 || memcmp(str1, str2, length1) != 0)
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

    return 0;
}

static int compare_bytes(coda_cursor *cursor1, coda_cursor *cursor2)
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

    return 0;
}

static int compare_attributes(coda_cursor *cursor1, coda_cursor *cursor2)
{
    int has_attributes1;
    int has_attributes2;

    if (coda_cursor_has_attributes(cursor1, &has_attributes1) != 0)
    {
        print_error_with_cursor(cursor1, 1);
        return -1;
    }
    if (coda_cursor_has_attributes(cursor2, &has_attributes2) != 0)
    {
        print_error_with_cursor(cursor2, 2);
        return -1;
    }
    if (has_attributes1 || has_attributes2)
    {
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
        if (compare_data(cursor1, cursor2) != 0)
        {
            return -1;
        }
        coda_cursor_goto_parent(cursor1);
        coda_cursor_goto_parent(cursor2);
    }

    return 0;
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
            if (compare_arrays(cursor1, cursor2) != 0)
            {
                return -1;
            }
            break;
        case coda_record_class:
            if (compare_records(cursor1, cursor2) != 0)
            {
                return -1;
            }
            break;
        case coda_integer_class:
        case coda_real_class:
            if (compare_numbers(cursor1, cursor2) != 0)
            {
                return -1;
            }
            break;
        case coda_text_class:
            if (compare_strings(cursor1, cursor2) != 0)
            {
                return -1;
            }
            break;
        case coda_raw_class:
            if (compare_bytes(cursor1, cursor2) != 0)
            {
                return -1;
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
    if (compare_attributes(cursor1, cursor2) != 0)
    {
        return -1;
    }

    return 0;
}

static int compare_files(char *filename1, char *filename2, char *starting_path)
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
        fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
        return -1;
    }

    if (array_key_info_set_product(pf1) != 0)
    {
        fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
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
        fprintf(stderr, "%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
        coda_close(pf1);
        return -1;
    }

    if (coda_cursor_set_product(&cursor1, pf1) != 0)
    {
        fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
        coda_close(pf1);
        coda_close(pf2);
        return -1;
    }

    if (coda_cursor_set_product(&cursor2, pf2) != 0)
    {
        fprintf(stderr, "%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
        coda_close(pf1);
        coda_close(pf2);
        return -1;
    }

    if (starting_path != NULL)
    {
        if (coda_cursor_goto(&cursor1, starting_path) != 0)
        {
            fprintf(stderr, "%sERROR: %s\n", pre[0], coda_errno_to_string(coda_errno));
            coda_close(pf1);
            coda_close(pf2);
            return -1;
        }

        if (coda_cursor_goto(&cursor2, starting_path) != 0)
        {
            fprintf(stderr, "%sERROR: %s\n", pre[1], coda_errno_to_string(coda_errno));
            coda_close(pf1);
            coda_close(pf2);
            return -1;
        }

    }

    result = compare_data(&cursor1, &cursor2);

    coda_close(pf1);
    coda_close(pf2);

    return result;
}

int main(int argc, char **argv)
{
    int perform_conversions;
    char *starting_path;
    int result;
    int i;

    option_verbose = 0;
    perform_conversions = 1;
    starting_path = NULL;

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

    array_key_info_init();

    i = 1;
    if (i + 1 < argc && strcmp(argv[i], "-D") == 0)
    {
        coda_set_definition_path(argv[i + 1]);
        i += 2;
    }
    else
    {
#ifdef WIN32
        const char *definition_path = "../definitions";
#else
        const char *definition_path = "../share/" PACKAGE "/definitions";
#endif
        if (coda_set_definition_path_conditional(argv[0], NULL, definition_path) != 0)
        {
            fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
            exit(1);
        }
    }

    while (i < argc)
    {
        if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            option_verbose = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            starting_path = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keys") == 0) && i + 2 < argc)
        {
            if (array_key_info_add_key(argv[i + 1], argv[i + 2]) != 0)
            {
                return -1;
            }
            i += 2;
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
        i++;
    }

    if (i != argc - 2)
    {
        /* we expect two filenames for the last two arguments */
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
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
    result = compare_files(argv[argc - 2], argv[argc - 1], starting_path);

    coda_done();

    array_key_info_done();

    if (result != 0)
    {
        exit(1);
    }

    return 0;
}
