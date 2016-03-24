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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-internal.h"

char *ascii_col_sep;
int show_type;
int show_unit;
int show_format;
int show_description;
int show_quotes;
int show_hidden;
int show_expressions;
int show_parent_types;
int show_attributes;
int use_special_types;

void generate_html(const char *prefixdir);
void generate_list(const char *product_class, const char *product_type, int version);
void generate_xmlschema(const char *output_file_name, const char *product_class, const char *product_type, int version);
void generate_definition(const char *output_file_name, const char *file_name);
void generate_detection_tree(coda_format format);

static void print_version()
{
    printf("codadd %s\n", libcoda_version);
    printf("Copyright (C) 2007-2016 S[&]T, The Netherlands.\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codadd [-D definitionpath]\n");
    printf("        Try to read all product format definitions and report any problems\n");
    printf("\n");
    printf("    codadd [-D definitionpath] doc <directory>\n");
    printf("        Generate HTML product format documentation in the specified directory\n");
    printf("\n");
    printf("    codadd [-D definitionpath] list [<list options>]\n");
    printf("                               [<product class> [<product type> [<version>]]]\n");
    printf("        Gives an overview of available product format definitions\n");
    printf("        When all of product class, product type, and format version are provided\n");
    printf("        an overview of the product content for the specified product format\n");
    printf("        definition is given\n");
    printf("        List options:\n");
    printf("            -e, --expr\n");
    printf("                    show expressions for dynamic array sizes\n");
    printf("            -q, --quote_strings\n");
    printf("                    put \"\" around string data and '' around character data\n");
    printf("            -s, --column_separator '<separator string>'\n");
    printf("                    use the given string as column separator (default: ' ')\n");
    printf("            -t, --type\n");
    printf("                    show basic data type\n");
    printf("            -u, --unit\n");
    printf("                    show unit information\n");
    printf("            --description\n");
    printf("                    show description information\n");
    printf("            --hidden\n");
    printf("                    show record fields with 'hidden' property\n");
    printf("            --parent-types\n");
    printf("                    show additional lines for records and arrays\n");
    printf("            --attributes\n");
    printf("                    show additional lines for attributes\n");
    printf("            --no_special_types\n");
    printf("                    bypass special data types from the CODA format definition -\n");
    printf("                    data with a special type is treated using its non-special\n");
    printf("                    base type\n");
    printf("\n");
    printf("    codadd [-D definitionpath] xmlschema [<xmlschema options>]\n");
    printf("                               <product class> <product type> <version>\n");
    printf("        Create an XML Schema file for a single product definition\n");
    printf("        Note that this will only work if the product class/type/version points\n");
    printf("        to a product definition for an XML file\n");
    printf("        XML Schema options:\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("\n");
    printf("    codadd [-D definitionpath] definition [<definition options>] <product file>\n");
    printf("        Create a CODA definition XML file with the format definition of a\n");
    printf("        product. The XML file is a standalone definition file similar to those\n");
    printf("        used within .codadef files.\n");
    printf("        Definition options:\n");
    printf("            -o, --output <filename>\n");
    printf("                    write output to specified file\n");
    printf("\n");
    printf("    codadd [-D definitionpath] dtree <format>\n");
    printf("        Shows the product recognition detection tree for the given file format.\n");
    printf("        Note that ascii and binary formatted products use the same detection\n");
    printf("        tree.\n");
    printf("\n");
    printf("    codadd -h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codadd -v, --version\n");
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

int main(int argc, char *argv[])
{
    int i = 1;

    ascii_col_sep = " ";
    show_type = 0;
    show_unit = 0;
    show_format = 0;
    show_description = 0;
    show_quotes = 0;
    show_hidden = 0;
    show_expressions = 0;
    show_parent_types = 0;
    show_attributes = 0;
    use_special_types = 1;

    if (argc > 1)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_help();
            exit(0);
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
        {
            print_version();
            exit(0);
        }
    }

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

    coda_option_read_all_definitions = 1;
    if (coda_init() != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (i == argc)
    {
        coda_done();
        exit(0);
    }

    coda_set_option_perform_conversions(0);

    if (strcmp(argv[i], "doc") == 0)
    {
        i++;
        if (i != argc - 1)
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
        generate_html(argv[i]);
    }
    else if (strcmp(argv[i], "list") == 0)
    {
        const char *product_class = NULL;
        const char *product_type = NULL;
        int version = -1;

        i++;
        while (i < argc)
        {
            if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--expr") == 0)
            {
                show_expressions = 1;
            }
            else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quote_strings") == 0)
            {
                show_quotes = 1;
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
            else if (strcmp(argv[i], "--hidden") == 0)
            {
                show_hidden = 1;
            }
            else if (strcmp(argv[i], "--parent-types") == 0)
            {
                show_parent_types = 1;
            }
            else if (strcmp(argv[i], "--attributes") == 0)
            {
                show_attributes = 1;
            }
            else if (strcmp(argv[i], "--no_special_types") == 0)
            {
                use_special_types = 0;
            }
            else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--column_separator") == 0) &&
                     i + 1 < argc && argv[i + 1][0] != '-')
            {
                i++;
                ascii_col_sep = argv[i];
            }
            else if (argv[i][0] != '-')
            {
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

        if (i < argc)
        {
            product_class = argv[i];
            i++;
            if (i < argc)
            {
                product_type = argv[i];
                i++;
                if (i < argc)
                {
                    if (sscanf(argv[i], "%d", &version) != 1)
                    {
                        fprintf(stderr, "ERROR: invalid product version argument\n");
                        print_help();
                        exit(1);
                    }
                    i++;
                    if (i < argc)
                    {
                        fprintf(stderr, "ERROR: invalid arguments\n");
                        print_help();
                        exit(1);
                    }
                }
            }
        }
        generate_list(product_class, product_type, version);
    }
    else if (strcmp(argv[i], "xmlschema") == 0)
    {
        const char *output_file_name = NULL;
        const char *product_class = NULL;
        const char *product_type = NULL;
        int version = -1;

        i++;
        while (i < argc)
        {
            if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) &&
                i + 1 < argc && argv[i + 1][0] != '-')
            {
                i++;
                output_file_name = argv[i];
            }
            else if (argv[i][0] != '-')
            {
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

        if (i != argc - 3)
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
        product_class = argv[i];
        i++;
        product_type = argv[i];
        i++;
        if (sscanf(argv[i], "%d", &version) != 1)
        {
            fprintf(stderr, "ERROR: invalid product version argument\n");
            print_help();
            exit(1);
        }
        generate_xmlschema(output_file_name, product_class, product_type, version);
    }
    else if (strcmp(argv[i], "dtree") == 0)
    {
        coda_format format;

        i++;
        if (i != argc - 1)
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
        if (coda_format_from_string(argv[i], &format) != 0)
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
        generate_detection_tree(format);
    }
    else if (strcmp(argv[i], "definition") == 0)
    {
        const char *output_file_name = NULL;

        i++;
        while (i < argc)
        {
            if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) &&
                i + 1 < argc && argv[i + 1][0] != '-')
            {
                i++;
                output_file_name = argv[i];
            }
            else if (argv[i][0] != '-')
            {
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

        if (i != argc - 1)
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
        generate_definition(output_file_name, argv[i]);
    }
    else
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }

    coda_done();

    return 0;
}
