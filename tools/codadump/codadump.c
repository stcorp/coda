/*
 * Copyright (C) 2007-2017 S[&]T, The Netherlands.
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

run_mode_t run_mode;
dim_info_t dim_info;
traverse_info_t traverse_info;

#ifdef HAVE_HDF4
hdf4_info_t hdf4_info;
#endif
char *ascii_col_sep;
FILE *ascii_output;
char *output_file_name;
char *starting_path;
int verbosity;
int calc_dim;
int max_depth = -1;
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
    printf("Copyright (C) 2007-2017 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codadump [-D definitionpath] list [<list options>] <product file>\n");
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
    printf("    codadump [-D definitionpath] ascii [<ascii options>] <product file>\n");
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
    printf("    codadump [-D definitionpath] hdf4 [<hdf4 options>] <product file>\n");
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
    printf("    codadump [-D definitionpath] json [<json options>] <product file>\n");
    printf("        Write the contents of a product file to a JSON file\n");
    printf("        JSON options:\n");
    printf("            -a, --attributes\n");
    printf("                    include attributes - items with attributes will be\n");
    printf("                    encapsulated in a {'attr':<attr>,'data':<data>} object\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("            -p, --path <path>\n");
    printf("                    path (in the form of a CODA node expression) to the\n");
    printf("                    location in the product where the operation should begin\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
    printf("    codadump [-D definitionpath] yaml [<json options>] <product file>\n");
    printf("        Write the contents of a product file to a YAML file\n");
    printf("        YAML options:\n");
    printf("            -a, --attributes\n");
    printf("                    include attributes - items with attributes will be\n");
    printf("                    encapsulated in a an associative array with keys 'attr'\n");
    printf("                    and 'data'\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("            -p, --path <path>\n");
    printf("                    path (in the form of a CODA node expression) to the\n");
    printf("                    location in the product where the operation should begin\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
    printf("    codadump [-D definitionpath] debug [<debug options>] <product file>\n");
    printf("        Show the contents of a product file in sequential order for debug\n");
    printf("        purposes. No conversions are applied and (if applicable) for each\n");
    printf("        data element the file offset is given\n");
    printf("        Debug options:\n");
    printf("            -d, --disable_fast_size_expressions\n");
    printf("                    do not use fast-size expressions\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("            -p, --path <path>\n");
    printf("                    path (in the form of a CODA node expression) to the\n");
    printf("                    location in the product where the operation should begin\n");
    printf("            --max_depth <depth>\n");
    printf("                    only traverse arrays/records this deep for printing items\n");
    printf("                    (the max depth is relative to any path provided by -p)\n");
    printf("            --open_as <product class> <product type> <version>\n");
    printf("                    force opening the product using the given product class,\n");
    printf("                    product type, and format version\n");
    printf("\n");
    printf("    codadump -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codadump -v, --version\n");
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

    for (i = 0; i < argc; i++)
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

    for (i = 0; i < argc; i++)
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

    for (i = 0; i < argc; i++)
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

static void handle_json_run_mode(int argc, char *argv[])
{
    int use_special_types;
    int perform_conversions;
    int include_attributes;
    int i;

    traverse_info.file_name = NULL;
    output_file_name = NULL;
    starting_path = NULL;
    ascii_output = stdout;
    use_special_types = 1;
    perform_conversions = 1;
    include_attributes = 0;

    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--attributes") == 0)
        {
            include_attributes = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            starting_path = argv[i + 1];
            i++;
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
    coda_set_option_perform_conversions(perform_conversions);
    if (output_file_name != NULL)
    {
        ascii_output = fopen(output_file_name, "w");
        if (ascii_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    print_json_data(include_attributes);

    if (output_file_name != NULL)
    {
        fclose(ascii_output);
    }
    coda_done();
}

static void handle_yaml_run_mode(int argc, char *argv[])
{
    int use_special_types;
    int perform_conversions;
    int include_attributes;
    int i;

    traverse_info.file_name = NULL;
    output_file_name = NULL;
    starting_path = NULL;
    ascii_output = stdout;
    use_special_types = 1;
    perform_conversions = 1;
    include_attributes = 0;

    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--attributes") == 0)
        {
            include_attributes = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            starting_path = argv[i + 1];
            i++;
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
    coda_set_option_perform_conversions(perform_conversions);
    if (output_file_name != NULL)
    {
        ascii_output = fopen(output_file_name, "w");
        if (ascii_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    print_yaml_data(include_attributes);

    if (output_file_name != NULL)
    {
        fclose(ascii_output);
    }
    coda_done();
}

static void handle_debug_run_mode(int argc, char *argv[])
{
    const char *product_class = NULL;
    const char *product_type = NULL;
    int format_version = 0;
    int use_fast_size_expressions;
    int i;

    traverse_info.file_name = NULL;
    output_file_name = NULL;
    starting_path = NULL;
    ascii_output = stdout;
    use_fast_size_expressions = 1;

    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_fast_size_expressions") == 0)
        {
            use_fast_size_expressions = 0;
        }
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            output_file_name = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--path") == 0) && i + 1 < argc &&
                 argv[i + 1][0] != '-')
        {
            starting_path = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "--max_depth") == 0) && i + 1 < argc && argv[i + 1][0] != '-')
        {
            max_depth = atoi(argv[i + 1]);      /* we just use '0' if the conversion to int fails */
            i++;
        }
        else if (strcmp(argv[i], "--open_as") == 0 && i + 3 < argc && argv[i + 1][0] != '-' && argv[i + 2][0] != '-' &&
                 argv[i + 3][0] != '-')
        {
            product_class = argv[i + 1];
            product_type = argv[i + 2];
            format_version = atoi(argv[i + 3]); /* we just use '0' if the conversion to int fails */
            i += 3;
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
    coda_set_option_perform_conversions(0);
    coda_set_option_use_fast_size_expressions(use_fast_size_expressions);
    if (output_file_name != NULL)
    {
        ascii_output = fopen(output_file_name, "w");
        if (ascii_output == NULL)
        {
            fprintf(stderr, "ERROR: could not create output file \"%s\"\n", output_file_name);
            exit(1);
        }
    }

    print_debug_data(product_class, product_type, format_version);

    if (output_file_name != NULL)
    {
        fclose(ascii_output);
    }
    coda_done();
}

int main(int argc, char *argv[])
{
    int i;

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

    if (strcmp(argv[i], "list") == 0)
    {
        run_mode = RUN_MODE_LIST;
        i++;
        handle_list_run_mode(argc - i, &argv[i]);
    }
    else if (strcmp(argv[i], "ascii") == 0)
    {
        run_mode = RUN_MODE_ASCII;
        i++;
        handle_ascii_run_mode(argc - i, &argv[i]);
    }
#ifdef HAVE_HDF4
    else if (strcmp(argv[i], "hdf4") == 0)
    {
        run_mode = RUN_MODE_HDF4;
        i++;
        handle_hdf4_run_mode(argc - i, &argv[i]);
    }
#endif
    else if (strcmp(argv[i], "json") == 0)
    {
        run_mode = RUN_MODE_JSON;
        i++;
        handle_json_run_mode(argc - i, &argv[i]);
    }
    else if (strcmp(argv[i], "yaml") == 0)
    {
        run_mode = RUN_MODE_YAML;
        i++;
        handle_yaml_run_mode(argc - i, &argv[i]);
    }
    else if (strcmp(argv[i], "debug") == 0)
    {
        run_mode = RUN_MODE_DEBUG;
        i++;
        handle_debug_run_mode(argc - i, &argv[i]);
    }
    else
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    return 0;
}
