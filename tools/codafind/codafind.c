/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

static int verbosity;

static void print_version()
{
    printf("codafind %s\n", libcoda_version);
    printf("Copyright (C) 2007-2016 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codafind [-D definitionpath] [<options>] <files|directories>\n");
    printf("        Match a filter on a series of files and/or recursively on all contents\n");
    printf("        of directories\n");
    printf("\n");
    printf("        Options:\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -f, --filter '<filter expression>'\n");
    printf("                    restrict the output to data that matches the filter\n");
    printf("                    if no filter is provided codafind will find all files that\n");
    printf("                    can be opened with CODA\n");
    printf("            -V, --verbose\n");
    printf("                    show the match result for each file\n");
    printf("\n");
    printf("    codafind -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codafind -v, --version\n");
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

int callback(const char *filepath, coda_filefilter_status status, const char *error, void *userdata)
{
    (void)userdata;     /* prevent unused warning */

    if (status == coda_ffs_error)
    {
        fprintf(stderr, "%s: %s\n", filepath, error);
    }
    else if (status == coda_ffs_could_not_access_directory)
    {
        fprintf(stderr, "%s: unable to access directory\n", filepath);
    }
    else if (status == coda_ffs_could_not_open_file)
    {
        fprintf(stderr, "%s: could not open file (%s)\n", filepath, error);
    }
    else if (verbosity)
    {
        printf("%s -> ", filepath);
        switch (status)
        {
            case coda_ffs_error:
            case coda_ffs_could_not_access_directory:
            case coda_ffs_could_not_open_file:
                /* this case is already handled above */
                assert(0);
                break;
            case coda_ffs_unsupported_file:
                printf("unsupported product\n");
                break;
            case coda_ffs_no_match:
                printf("no match\n");
                break;
            case coda_ffs_match:
                printf("match\n");
                break;
        }
    }
    else
    {
        if (status == coda_ffs_match)
        {
            printf("%s\n", filepath);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char *filter = NULL;
    int perform_conversions;
    int i;

    verbosity = 0;
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
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            filter = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--verbosity") == 0)
        {
            verbosity = 1;
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

    if (i == argc)
    {
        /* no files or directories provided */
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    coda_set_option_perform_conversions(perform_conversions);

    if (coda_match_filefilter(filter, argc - i, (const char **)&argv[i], &callback, NULL) != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    coda_done();

    return 0;
}
