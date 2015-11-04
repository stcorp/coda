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

#include "codadump.h"

run_mode_t run_mode;
dim_info_t dim_info;
traverse_info_t traverse_info;

#ifdef HAVE_HDF4
hdf4_info_t hdf4_info;
#endif
char *ascii_col_sep;
FILE *ascii_output;
char *output_file_name;
int verbosity;
int calc_dim;
int show_dim_vals;
int show_index;
int show_label;
int show_quotes;
int show_time_as_string;
int show_type;
int show_unit;
int show_description;

static void print_version()
{
    printf("codadump version %s\n", libcoda_version);
    printf("Copyright (C) 2007-2012 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codadump list [<list options>] <product file>\n");
    printf("        List the contents of a product file\n");
    printf("        List options:\n");
    printf("            -c, --calc_dim\n");
    printf("                    calculate dimensions of all arrays\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -f, --filter '<filter expression>'\n");
    printf("                    restrict the output to data that matches the filter\n");
    printf("            -t, --type\n");
    printf("                    show basic data type\n");
    printf("            -u, --unit\n");
    printf("                    show unit information\n");
    printf("            --description\n");
    printf("                    show description information\n");
    printf("            --dim_values\n");
    printf("                    show all possible values for a variable sized dim\n");
    printf("                    (implies --calc_dim)\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
    printf("    codadump ascii [<ascii options>] <product file>\n");
    printf("        Show the contents of a product file in ascii format\n");
    printf("        List options:\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -f, --filter '<filter expression>'\n");
    printf("                    restrict the output to data that matches the filter\n");
    printf("            -i, --index\n");
    printf("                    print the array index for each array element\n");
    printf("            -l, --label\n");
    printf("                    print the full name and array dims for each data block\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("            -q, --quote_strings\n");
    printf("                    put \"\" around string data and '' around character data\n");
    printf("            -s, --column_separator '<separator string>'\n");
    printf("                    use the given string as column separator (default: ' ')\n");
    printf("            -t, --time_as_string\n");
    printf("                    print time as a string (instead of a floating point value)\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
#ifdef HAVE_HDF4
    printf("    codadump hdf4 [<hdf4 options>] <product file>\n");
    printf("        Convert a product file to a HDF4 file\n");
    printf("        HDF4 options:\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -f '<filter expression>', --filter '<filter expression>'\n");
    printf("                    restrict the output to data that matches the filter\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("            -s, --silent\n");
    printf("                    run in silent mode\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
#endif
    printf("    codadump debug [<debug options>] <product file>\n");
    printf("        Show the contents of a product file in sequential order for debug\n");
    printf("        purposes. No conversions are applied and (if applicable) for each\n");
    printf("        data element the file offset is given\n");
    printf("        Debug options:\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("\n");
    printf("    codadump -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codadump -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
}

void handle_coda_error()
{
    fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
    fflush(stderr);
    exit(1);
}

static void handle_list_run_mode(int argc, char *argv[])
{
    int use_special_types;
    int perform_conversions;
    int i;

    traverse_info.file_name = NULL;
    traverse_info.filter[0] = NULL;
    verbosity = 1;
    calc_dim = 0;
    use_special_types = 1;
    perform_conversions = 1;
    show_dim_vals = 0;
    show_type = 0;
    show_unit = 0;
    show_description = 0;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--calc_dim") == 0)
        {
            calc_dim = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            traverse_info.filter[0] = codadump_filter_create(argv[i + 1]);
            if (traverse_info.filter[0] == NULL)
            {
                fprintf(stderr, "ERROR: incorrect filter or empty filter\n");
                print_help();
                exit(1);
            }
            i++;
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0)
        {
            show_type = 1;
        }
        else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--unit") == 0)
        {
            show_unit = 1;
        }
        else if (strcmp(argv[i], "--description") == 0)
        {
            show_description = 1;
        }
        else if (strcmp(argv[i], "--dim_values") == 0)
        {
            calc_dim = 1;
            show_dim_vals = 1;
        }
        else if (strcmp(argv[i], "--no_special_types") == 0)
        {
            use_special_types = 0;
        }
        else if (i == argc - 1 && argv[i][0] != '-')
        {
            traverse_info.file_name = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
    }

    if (traverse_info.file_name == NULL || traverse_info.file_name[0] == '\0')
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    coda_set_option_bypass_special_types(!use_special_types);
    coda_set_option_perform_boundary_checks(0);
    coda_set_option_perform_conversions(perform_conversions);
    traverse_info_init();
    dim_info_init();

    traverse_product();

    dim_info_done();
    traverse_info_done();
    coda_done();
}

static void handle_ascii_run_mode(int argc, char *argv[])
{
    int use_special_types;
    int perform_conversions;
    int i;

    traverse_info.file_name = NULL;
    traverse_info.filter[0] = NULL;
    output_file_name = NULL;
    ascii_col_sep = " ";
    ascii_output = stdout;
    verbosity = 1;
    calc_dim = 1;
    use_special_types = 1;
    perform_conversions = 1;
    show_index = 0;
    show_label = 0;
    show_quotes = 0;
    show_time_as_string = 0;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            traverse_info.filter[0] = codadump_filter_create(argv[i + 1]);
            if (traverse_info.filter[0] == NULL)
            {
                fprintf(stderr, "ERROR: incorrect filter or empty filter\n");
                print_help();
                exit(1);
            }
            i++;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--index") == 0)
        {
            show_index = 1;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--label") == 0)
        {
            show_label = 1;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quote_strings") == 0)
        {
            show_quotes = 1;
        }
        else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--column_separator") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            ascii_col_sep = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--time_as_string") == 0)
        {
            show_time_as_string = 1;
        }
        else if (strcmp(argv[i], "--no_special_types") == 0)
        {
            use_special_types = 0;
        }
        else if (i == argc - 1 && argv[i][0] != '-')
        {
            traverse_info.file_name = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
    }

    if (traverse_info.file_name == NULL || traverse_info.file_name[0] == '\0')
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    coda_set_option_bypass_special_types(!use_special_types);
    coda_set_option_perform_boundary_checks(0);
    coda_set_option_perform_conversions(perform_conversions);
    traverse_info_init();
    dim_info_init();
    if (output_file_name != NULL)
    {
        ascii_output = fopen(output_file_name, "w");
        if (ascii_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    traverse_product();

    if (output_file_name != NULL)
    {
        fclose(ascii_output);
    }
    dim_info_done();
    traverse_info_done();
    coda_done();
}

#ifdef HAVE_HDF4
static void handle_hdf4_run_mode(int argc, char *argv[])
{
    int own_output_file_name = 0;
    int use_special_types;
    int perform_conversions;
    int i;

    traverse_info.file_name = NULL;
    traverse_info.filter[0] = NULL;
    output_file_name = NULL;
    verbosity = 1;
    calc_dim = 1;
    use_special_types = 1;
    perform_conversions = 1;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filter") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            traverse_info.filter[0] = codadump_filter_create(argv[i + 1]);
            if (traverse_info.filter[0] == NULL)
            {
                fprintf(stderr, "ERROR: incorrect filter or empty filter\n");
                print_help();
                exit(1);
            }
            i++;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) &&
                 i + 1 < argc && argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0)
        {
            verbosity = 0;
        }
        else if (strcmp(argv[i], "--no_special_types") == 0)
        {
            use_special_types = 0;
        }
        else if (i == argc - 1 && argv[i][0] != '-')
        {
            traverse_info.file_name = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
    }

    if (traverse_info.file_name == NULL)
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if ((traverse_info.file_name != NULL) && (output_file_name == NULL))
    {
        own_output_file_name = 1;
        output_file_name = malloc(strlen(traverse_info.file_name) + 5);
        if (output_file_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           strlen(traverse_info.file_name) + 5, __FILE__, __LINE__);
            handle_coda_error();
        }
        sprintf(output_file_name, "%s.hdf", traverse_info.file_name);
    }

    if ((traverse_info.file_name[0] == '\0') || (output_file_name[0] == '\0'))
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    coda_set_option_bypass_special_types(!use_special_types);
    coda_set_option_perform_boundary_checks(0);
    coda_set_option_perform_conversions(perform_conversions);
    traverse_info_init();
    dim_info_init();
    hdf4_info_init();

    traverse_product();

    hdf4_info_done();
    dim_info_done();
    traverse_info_done();
    coda_done();

    if (own_output_file_name)
    {
        free(output_file_name);
    }
}
#endif

static void handle_debug_run_mode(int argc, char *argv[])
{
    int i;

    traverse_info.file_name = NULL;
    output_file_name = NULL;
    ascii_output = stdout;

    for (i = 1; i < argc; i++)
    {
        if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc && argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if (i == argc - 1 && argv[i][0] != '-')
        {
            traverse_info.file_name = argv[i];
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
    }

    if (traverse_info.file_name == NULL || traverse_info.file_name[0] == '\0')
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    coda_set_option_perform_boundary_checks(0);
    coda_set_option_perform_conversions(0);
    if (output_file_name != NULL)
    {
        ascii_output = fopen(output_file_name, "w");
        if (ascii_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    print_debug_data();

    if (output_file_name != NULL)
    {
        fclose(ascii_output);
    }
    coda_done();
}

int main(int argc, char *argv[])
{
#ifdef WIN32
    const char *definition_path = "../definitions";
#else
    const char *definition_path = "../share/" PACKAGE "/definitions";
#endif
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

    if (coda_set_definition_path_conditional(argv[0], NULL, definition_path) != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (strcmp(argv[1], "list") == 0)
    {
        run_mode = RUN_MODE_LIST;
        handle_list_run_mode(argc - 1, &argv[1]);
    }
    else if (strcmp(argv[1], "ascii") == 0)
    {
        run_mode = RUN_MODE_ASCII;
        handle_ascii_run_mode(argc - 1, &argv[1]);
    }
#ifdef HAVE_HDF4
    else if (strcmp(argv[1], "hdf4") == 0)
    {
        run_mode = RUN_MODE_HDF4;
        handle_hdf4_run_mode(argc - 1, &argv[1]);
    }
#endif
    else if (strcmp(argv[1], "debug") == 0)
    {
        run_mode = RUN_MODE_DEBUG;
        handle_debug_run_mode(argc - 1, &argv[1]);
    }
    else
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    return 0;
}
