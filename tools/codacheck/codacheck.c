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

/* internal CODA functions */
int coda_cursor_print_path(const coda_cursor *cursor, int (*print) (const char *, ...));
int coda_product_check(coda_product *product, int full_read_check,
                       void (*callbackfunc) (coda_cursor *, const char *, void *), void *userdata);

int option_verbose;
int option_quick;
int option_require_definition;
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
    printf("            -d, --definition\n");
    printf("                    require products to have a definition in a codadef file,\n");
    printf("                    return an error and abort verification otherwise\n");
    printf("                    (affects products using formats such as xml/netcdf/hdf)\n");
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

static void print_error(coda_cursor *cursor, const char *error, void *userdata)
{
    userdata = userdata;

    printf("  ERROR: %s", error);
    if (cursor != NULL)
    {
        printf(" at ");
        coda_cursor_print_path(cursor, printf);
    }
    printf("\n");
    found_errors = 1;
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

    if (option_require_definition && (product_class == NULL || product_class == NULL))
    {
        printf("  ERROR: could not determine product type\n\n");
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
        found_errors = 1;
        return;
    }

    if (coda_product_check(product, !option_quick, print_error, NULL) != 0)
    {
        printf("  ERROR: %s\n\n", coda_errno_to_string(coda_errno));
        found_errors = 1;
        coda_close(product);
        return;
    }

    if (coda_close(product) != 0)
    {
        printf("  ERROR: %s\n", coda_errno_to_string(coda_errno));
        found_errors = 1;
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
    option_require_definition = 0;

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
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--definition") == 0)
        {
            option_require_definition = 1;
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
