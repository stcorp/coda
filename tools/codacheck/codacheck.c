/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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
#include <time.h>

#include "coda.h"

/* internal CODA functions */
int coda_cursor_print_path(const coda_cursor *cursor, int (*print)(const char *, ...));
int coda_product_check(coda_product *product, int full_read_check,
                       void (*callbackfunc)(coda_cursor *, const char *, void *), void *userdata);

int option_verbose;
int option_quick;
int option_require_definition;
int found_errors;

static void print_version(void)
{
    printf("codacheck version %s\n", libcoda_version);
    printf("Copyright (C) 2007-2023 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help(void)
{
    printf("Usage:\n");
    printf("    codacheck [-D definitionpath] [<options>] <files>\n");
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
    printf("            --no-mmap\n");
    printf("                    disable the use of mmap when opening files\n");
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
    printf("    CODA will look for .codadef files using a definition path, which is a ':'\n");
    printf("    separated (';' on Windows) list of paths to .codadef files and/or to\n");
    printf("    directories containing .codadef files.\n");
    printf("    By default the definition path is set to a single directory relative to\n");
    printf("    the tool location. A different definition path can be set via the\n");
    printf("    CODA_DEFINITION environment variable or via the -D option.\n");
    printf("    (the -D option overrides the environment variable setting).\n");
    printf("\n");
}

static void print_error(coda_cursor *cursor, const char *error, void *userdata)
{
    (void)userdata;     /* prevent unused warning */

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

    if (option_require_definition && (product_class == NULL || product_type == NULL))
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
    int option_stdin;
    int option_use_mmap;
    int i;

    option_stdin = 0;
    option_verbose = 0;
    option_quick = 0;
    option_use_mmap = 1;
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

    i = 1;
    if (i + 1 < argc && strcmp(argv[i], "-D") == 0)
    {
        coda_set_definition_path(argv[i + 1]);
        i += 2;
    }
    else
    {
        const char *definition_path = "../share/" PACKAGE "/definitions";

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
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quick") == 0)
        {
            option_quick = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--definition") == 0)
        {
            option_require_definition = 1;
        }
        else if (strcmp(argv[i], "--no-mmap") == 0)
        {
            option_use_mmap = 0;
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
        i++;
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

    /* Set mmap based on the chosen option */
    coda_set_option_use_mmap(option_use_mmap);

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
        while (i < argc)
        {
            check_file(argv[i]);
            fflush(NULL);
            i++;
        }
    }

    coda_done();

    if (found_errors)
    {
        exit(1);
    }

    return 0;
}
