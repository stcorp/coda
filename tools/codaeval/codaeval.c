/*
* Copyright (C) 2007-2009 S&T, The Netherlands.
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda.h"

/* Expression to be evaluated. */
static coda_expression *eval_expr = NULL;
static coda_expression_type expr_type;

/* Node expression to identify the node on which to evaluate the expression. */
static coda_expression *node_expr = NULL;

static void generate_escaped_string(const char *str, int length)
{
    int i = 0;

    if (length == 0 || str == NULL)
    {
        return;
    }

    if (length < 0)
    {
        length = strlen(str);
    }

    while (i < length)
    {
        switch (str[i])
        {
            case '\033':       /* windows does not recognize '\e' */
                printf("\\e");
                break;
            case '\a':
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\v':
                printf("\\v");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '"':
                printf("\\\"");
                break;
            default:
                if (!isprint(str[i]))
                {
                    printf("\\%03o", (int)(unsigned char)str[i]);
                }
                else
                {
                    printf("%c", str[i]);
                }
                break;
        }
        i++;
    }
}

static void print_version()
{
    printf("codaeval %s\n", libcoda_version);
    printf("Copyright (C) 2007-2012 S[&]T, The Netherlands\n");
    printf("\n");
}

static void print_help()
{
    printf("Usage:\n");
    printf("    codaeval [<options>] expression [<files|directories>]\n");
    printf("        Evaluate a CODA expression on a series of files and/or recursively on\n");
    printf("        all contents of directories\n");
    printf("        If no files or directories are provided then codaeval should be a\n");
    printf("        'constant' expression (i.e. it may not contain node expressions or\n");
    printf("        functions that rely on product content)\n");
    printf("\n");
    printf("        Options:\n");
    printf("            -c, --check\n");
    printf("                    only check the syntax of the expression, without evaluating\n");
    printf("                    it; any remaining options (including files) will be ignored\n");
    printf("            -d, --disable_conversions\n");
    printf("                    do not perform unit/value conversions\n");
    printf("            -p '<path>'\n");
    printf("                    a path (in the form of a CODA node expression) to the\n");
    printf("                    location in the product where the expression should be\n");
    printf("                    evaluated\n");
    printf("                    if no path is provided the expression will be evaluated\n");
    printf("                    at the root of the product\n");
    printf("\n");
    printf("    A description of the syntax of CODA expression language can be found in the\n");
    printf("    CODA documentation\n");
    printf("\n");
    printf("    codaeval h, --help\n");
    printf("        Show help (this text)\n");
    printf("\n");
    printf("    codaeval -v, --version\n");
    printf("        Print the version number of CODA and exit\n");
    printf("\n");
}


static int eval_expression(coda_cursor *cursor)
{
    switch (expr_type)
    {
        case coda_expression_boolean:  /* boolean */
            {
                int value;

                if (coda_expression_eval_bool(eval_expr, cursor, &value))
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot evaluate boolean expression (%s)",
                                   coda_errno_to_string(coda_errno));
                    return -1;
                }
                printf("%s\n", (value ? "true" : "false"));
            }
            break;
        case coda_expression_integer:  /* integer */
            {
                int64_t value;
                char s[21];

                if (coda_expression_eval_integer(eval_expr, cursor, &value))
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot evaluate integer expression (%s)",
                                   coda_errno_to_string(coda_errno));
                    return -1;
                }
                coda_str64(value, s);
                printf("%s\n", s);
            }
            break;
        case coda_expression_float:    /* floating point */
            {
                double value;

                if (coda_expression_eval_float(eval_expr, cursor, &value))
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot evaluate floating point expression (%s)",
                                   coda_errno_to_string(coda_errno));
                    return -1;
                }
                printf("%f\n", value);
            }
            break;
        case coda_expression_string:   /* string */
            {
                char *value = NULL;
                long length;

                if (coda_expression_eval_string(eval_expr, cursor, &value, &length))
                {
                    coda_set_error(CODA_ERROR_EXPRESSION, "cannot evaluate string expression (%s)",
                                   coda_errno_to_string(coda_errno));
                    return -1;
                }
                generate_escaped_string(value, length);
                printf("\n");
                if (value != NULL)
                {
                    free(value);
                }
            }
            break;
        default:
            assert(0);
            exit(1);
    }

    return 0;
}

/* result:
 *   0: continue with next product (even though we may have printed an error)
 *   1: stop processing (only for an internal error or when all next files would give the same error)
 */
static int eval_expression_for_file(const char *filepath)
{
    coda_product *pf = NULL;
    coda_cursor cursor;

    /* open the file */
    if (coda_open(filepath, &pf) != 0)
    {
        return 1;
    }
    if (coda_cursor_set_product(&cursor, pf) != 0)
    {
        coda_close(pf);
        return 1;
    }

    /* if we have a node expression then evaluate this here to move the cursor */
    if (node_expr != NULL)
    {
        if (coda_expression_eval_node(node_expr, &cursor))
        {
            fprintf(stderr, "ERROR: could not evaluate path expression: %s\n", coda_errno_to_string(coda_errno));
            coda_close(pf);
            return 0;
        }
    }

    if (eval_expression(&cursor) != 0)
    {
        fprintf(stderr, "ERROR: %s for %s\n", coda_errno_to_string(coda_errno), filepath);
    }

    if (coda_close(pf) != 0)
    {
        return 1;
    }

    return 0;
}

int callback(const char *filepath, coda_filefilter_status status, const char *error, void *userdata)
{
    userdata = userdata;        /* prevent unused warnings from the compiler */

    if (status == coda_ffs_error)
    {
        fprintf(stderr, "ERROR: %s for %s\n", error, filepath);
    }
    else if (status == coda_ffs_could_not_access_directory)
    {
        fprintf(stderr, "ERROR: unable to access directory %s\n", filepath);
    }
    else if (status == coda_ffs_could_not_open_file)
    {
        fprintf(stderr, "ERROR: could not open file %s (%s)\n", filepath, error);
    }
    if (status == coda_ffs_match)
    {
        return eval_expression_for_file(filepath);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int perform_conversions;
    int check_only;
    int i;

    perform_conversions = 1;
    check_only = 0;

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
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--check") == 0)
        {
            check_only = 1;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable_conversions") == 0)
        {
            perform_conversions = 0;
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc && argv[i + 1][0] != '-')
        {
            if (coda_expression_from_string(argv[i + 1], &node_expr))
            {
                fprintf(stderr, "ERROR: error in path expression: %s\n", coda_errno_to_string(coda_errno));
                exit(1);
            }
            i++;
            if (coda_expression_get_type(node_expr, &expr_type) != 0)
            {
                fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
                exit(1);
            }
        }
        else if (argv[i][0] != '-')
        {
            /* assume all arguments from here on are the expression and the optional list of files/directories */
            break;
        }
        else
        {
            fprintf(stderr, "ERROR: invalid arguments\n");
            print_help();
            exit(1);
        }
    }

    /* expression parameter */
    if (i >= argc)
    {
        fprintf(stderr, "ERROR: invalid arguments\n");
        print_help();
        exit(1);
    }
    if (coda_expression_from_string(argv[i], &eval_expr) != 0)
    {
        fprintf(stderr, "ERROR: error in expression: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }
    i++;

    if (check_only)
    {
        coda_expression_delete(eval_expr);
        if (node_expr != NULL)
        {
            coda_expression_delete(node_expr);
        }

        return 0;
    }

    if (coda_expression_get_type(eval_expr, &expr_type) != 0)
    {
        fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
        exit(1);
    }

    if (i < argc)
    {
#ifdef WIN32
        const char *definition_path = "../definitions";
#else
        const char *definition_path = "../share/" PACKAGE "/definitions";
#endif

        if (expr_type != coda_expression_boolean && expr_type != coda_expression_integer &&
            expr_type != coda_expression_float && expr_type != coda_expression_string)
        {
            fprintf(stderr, "ERROR: only boolean, integer, floating point, and string expressions can be evaluated on "
                    "products\n");
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

        coda_set_option_perform_conversions(perform_conversions);

        if (coda_match_filefilter(NULL, argc - i, (const char **)&argv[i], &callback, NULL) != 0)
        {
            fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
            exit(1);
        }

        coda_done();
    }
    else
    {
        if (node_expr != NULL)
        {
            fprintf(stderr, "ERROR: invalid arguments (path expression is only allowed if a file/directory list is "
                    "provided)\n");
            exit(1);
        }
        if (!coda_expression_is_constant(eval_expr))
        {
            fprintf(stderr,
                    "ERROR: invalid arguments (file/directory list needs to be provided if expression is not a "
                    "constant expression)\n");
            exit(1);
        }
        if (eval_expression(NULL) != 0)
        {
            fprintf(stderr, "ERROR: %s\n", coda_errno_to_string(coda_errno));
            exit(1);
        }
    }

    coda_expression_delete(eval_expr);
    if (node_expr != NULL)
    {
        coda_expression_delete(node_expr);
    }

    return 0;
}
