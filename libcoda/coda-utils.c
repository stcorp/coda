/*
 * Copyright (C) 2007-2008 S&T, The Netherlands.
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

#include "coda-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/** \file */

const char *coda_element_name_from_xml_name(const char *xml_name)
{
    char *element_name;

    /* find the element name within "<namespace> <element_name>"
     * The namespace (and separation character) are optional
     */
    element_name = (char *)xml_name;
    while (*element_name != ' ' && *element_name != '\0')
    {
        element_name++;
    }
    if (*element_name == '\0')
    {
        /* the name did not contain a namespace */
        return xml_name;
    }
    element_name++;     /* skip space */
    return element_name;
}

int coda_is_identifier(const char *name)
{
    int i;

    if (name == NULL)
    {
        return 0;
    }
    if (!isalpha(*name))
    {
        return 0;
    }
    i = 1;
    while (name[i] != '\0')
    {
        if (!(isalnum(name[i]) || name[i] == '_'))
        {
            return 0;
        }
        i++;
    }
    return 1;
}

char *coda_identifier_from_name(const char *name, hashtable *hash_data)
{
    const int postfix_length = 4;
    char *identifier;
    int length;
    int i;

    if (name == NULL)
    {
        name = "unnamed";
    }
    else
    {
        /* strip leading non-alpha characters */
        while (*name != '\0' && !isalpha(*name))
        {
            name++;
        }
        if (*name == '\0')
        {
            name = "unnamed";
        }
    }

    length = strlen(name);
    /* allow some space for _### postfixes */
    identifier = malloc(length + postfix_length + 1);
    if (identifier == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(length + postfix_length), __FILE__, __LINE__);
        return NULL;
    }

    /* create indentifier */

    /* first character is guaranteed to be an alpha character because of our previous test */
    identifier[0] = name[0];
    /* replace non-alphanumeric characters by a '_' */
    for (i = 1; i < length; i++)
    {
        if (isalnum(name[i]))
        {
            identifier[i] = name[i];
        }
        else
        {
            identifier[i] = '_';
        }
    }
    identifier[length] = '\0';

    /* check for double occurences */
    if (hash_data != NULL)
    {
        int counter;

        counter = 0;
        while (hashtable_get_index_from_name(hash_data, identifier) >= 0)
        {
            counter++;
            assert(counter < 1000);
            sprintf(&identifier[length], "_%d", counter);
        }
    }

    return identifier;
}

char *coda_short_identifier_from_name(const char *name, hashtable *hash_data, int maxlength)
{
    char *identifier;
    int length;
    int i;

    if (name == NULL)
    {
        name = "unnamed";
    }
    else
    {
        /* strip leading non-alpha characters */
        while (*name != '\0' && !isalpha(*name))
        {
            name++;
        }
        if (*name == '\0')
        {
            name = "unnamed";
        }
    }

    length = strlen(name);
    identifier = malloc(length + 1);
    if (identifier == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)(length + 1), __FILE__, __LINE__);
        return NULL;
    }

    /* create indentifier */

    /* first character is guaranteed to be an alpha character because of our previous test */
    identifier[0] = name[0];
    /* replace non-alphanumeric characters by a '_' */
    for (i = 1; i < length; i++)
    {
        if (isalnum(name[i]))
        {
            identifier[i] = name[i];
        }
        else
        {
            identifier[i] = '_';
        }
    }
    identifier[length] = '\0';

    /* shorten string until it fits within maxlength characters */
    while (length > maxlength)
    {
        int cur_part;
        int i;

        /* we try to do it the smart way by shortening individual string segments between '_' */
        cur_part = 0;
        for (i = 0; i < length; i++)
        {
            if (identifier[i] == '_' || identifier[i] == '.')
            {
                if (cur_part > 5)
                {
                    /* we shorten this string segment: always remove the 4th character of a segment */
                    i = i - cur_part + 4;
                    break;
                }
                cur_part = 0;
            }
            else
            {
                cur_part++;
            }
        }
        if (i < length)
        {
            memmove(&identifier[i - 1], &identifier[i], length - i + 1);
            length--;
        }
        else
        {
            /* can't do it by shortening string segments, so just truncate the string to maxlength characters */
            length = maxlength;
            identifier[maxlength] = '\0';
        }
    }

    /* check for double occurences */
    if (hash_data != NULL)
    {
        int counter;

        counter = 0;
        while (hashtable_get_index_from_name(hash_data, identifier) >= 0)
        {
            counter++;
            if (counter < 10)
            {
                sprintf(&identifier[length - 2], "_%d", counter);
            }
            else if (counter < 100)
            {
                sprintf(&identifier[length - 3], "_%d", counter);
            }
            else if (counter < 1000)
            {
                sprintf(&identifier[length - 4], "_%d", counter);
            }
            else
            {
                assert(0);
                exit(1);
            }
        }
    }

    return identifier;
}

/** \addtogroup coda_general
 * @{
 */

/** Convert an index for a multidimensional array that is stored in C-style order to an index for an identical array
 * stored in Fortran-style order.
 *
 * While elements of a multidimensional array are normally referenced via subscripts, CODA also allows referencing
 * through indices (which are one-dimensional). These indices (starting with 0) correspond with the positions of the
 * array elements as they are physically stored. This makes it easy to enumerate all elements of a multi-dimensional
 * array without having to deal with the multidimensional aspects of an array.
 *
 * However, the mapping of an array of subscripts to an index/storage position (and vice versa) can be defined in
 * essentially two ways.
 *
 * The first, which is the way it is done in CODA is such that the <i>last</i> element of a subscript array is the one 
 * that is the fastest running. For example, for a two dimensional array, the second element would have index 1 and
 * would correspond with the subscript (0, 1). This corresponds to the way multi-dimensional arrays are handled
 * in C and therefore this type of index is called a C-style index.
 *
 * The alternative way of providing an index is as it is done in Fortran, which has the <i>first</i> element of a
 * subscript array as the fastest running. In the previous example, the second element of the two dimensional array
 * with index 1 would correspond to the subscript (1, 0).
 *
 * As an example, if we have an array with dimensions (3, 4), then the subscript (0, 2) would refer to the element
 * with index 2 if the array was stored in C-style and would refer to the element with index 6 if it was stored in
 * Fortran-style.
 *
 * In order for a user to fill a multidimensional array in Fortran-style with data from a multi-dimensional array
 * from an Envisat product using CODA (which uses C-style indexing), the user can use this function to provide the
 * index conversions. A small example showing how this would be done with a multi-dimensional array of doubles:
 * \code
 * coda_cursor_get_array_dim(cursor, &num_dim, dims);
 * coda_cursor_get_num_elements(cursor, &num_elements);
 * if (num_elements > 0)
 * {
 *     coda_cursor_goto_first_array_element(cursor);
 *     for (i = 0; i < num_elements; i++)
 *     {
 *         int fortran_index = coda_c_index_to_fortran_index(num_dim, dims, i);
 *         coda_cursor_read_double(cursor, &fortran_multidim_array_ptr[fortran_index]);
 *         if (i < num_elements - 1)
 *         {
 *             coda_cursor_goto_next_array_element(cursor);
 *         }
 *     }
 *     coda_cursor_goto_parent(cursor);
 * }
 * \endcode
 *
 * \see coda_cursor_goto_array_element_by_index()
 * \param num_dims Number of dimensions of the multidimensional array.
 * \param dim      Array with the dimensions of the multidimensional array.
 * \param index    \c C style index.
 * \return
 *   \arg \c >=0, \c Fortran style index.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API long coda_c_index_to_fortran_index(int num_dims, const long dim[], long index)
{
    long i, indexf, multiplier, d[CODA_MAX_NUM_DIMS];

    if (num_dims > CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "num_dims argument (%d) exceeds limit (%d) (%s:%u)", num_dims,
                       CODA_MAX_NUM_DIMS, __FILE__, __LINE__);
        return -1;
    }

    for (i = num_dims - 1; i >= 0; i--)
    {
        d[i] = index % dim[i];
        index /= dim[i];
    }

    indexf = 0;
    multiplier = 1;

    for (i = 0; i < num_dims; i++)
    {
        indexf += multiplier * d[i];
        multiplier *= dim[i];
    }

    return indexf;
}

static void clean_path(char *path)
{
    int from;
    int to;

    if (path == NULL || path[0] == '\0')
    {
        return;
    }

    from = 0;
    to = 0;
    while (path[from] == '.' && path[from + 1] == '/')
    {
        from += 2;
    }
    while (path[from] != '\0')
    {
        if (path[from] == '/' || path[from] == '\\')
        {
            if (path[from + 1] == '/' || path[from + 1] == '\\')
            {
                from++;
                continue;
            }
            if (path[from + 1] == '.')
            {
                if (path[from + 2] == '\0' || path[from + 2] == '/' || path[from + 2] == '\\')
                {
                    from += 2;
                    continue;
                }
                if (path[from + 2] == '.' &&
                    (path[from + 3] == '\0' || path[from + 3] == '/' || path[from + 3] == '\\'))
                {
                    if  (!(to >= 2 && path[to - 1] == '.' && path[to - 2] == '.' &&
                           (to == 2 || path[to - 3] == '/' || path[to - 3] == '\\')))
                    {
                        int prev = to - 1;
                        /* find previous / or \ */
                        while (prev >= 0 && path[prev] != '/' && path[prev] != '\\')
                        {
                            prev--;
                        }
                        if (prev >= 0)
                        {
                            to = prev;
                            from += 3;
                            continue;
                        }
                    }
                }
            }
        }
        path[to] = path[from];
        from++;
        to++;
    }

    /* an empty path is a relative path to the current directory -> use '.' */
    if (to == 0)
    {
        path[to] = '.';
        to++;
    }

    path[to] = '\0';
}

int coda_path_find_file(const char *searchpath, const char *filename, char **location)
{
#ifdef WIN32
    const char path_separator_char = ';';
#else
    const char path_separator_char = ':';
#endif
    char *path;
    char *path_component;
    char *filepath = NULL;
    int filepath_length = 0;
    int filename_length = strlen(filename);
    
    if (searchpath == NULL || searchpath[0] == '\0')
    {
        *location = NULL;
        return 0;
    }
    
    path = strdup(searchpath);
    if (path == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    path_component = path;
    while (*path_component != '\0')
    {
        struct stat sb;
        char *p;
        int path_component_length;
        
        p = path_component;
        while (*p != '\0' && *p != path_separator_char)
        {
            p++;
        }
        if (*p != '\0')
        {
            *p = '\0';
            p++;
        }
        
        path_component_length = strlen(path_component);
        if (filepath_length < path_component_length + filename_length + 1)
        {
            char *new_filepath;
            new_filepath = realloc(filepath, path_component_length + filename_length + 2);
            if (new_filepath == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                               __LINE__);
                if (filepath != NULL)
                {
                    free(filepath);
                }
                return -1;
            }
            filepath = new_filepath;
            filepath_length = path_component_length + filename_length + 1;
        }
        sprintf(filepath, "%s/%s", path_component, filename); 
        
        if (stat(filepath, &sb) == 0)
        {
            if (sb.st_mode & S_IFREG)
            {
                /* we found the file */
                *location = filepath;
                free(path);
                return 0;
            }
        }
        
        path_component = p;
    }
    
    if (filepath != NULL)
    {
        free(filepath);
    }
    free(path);
    
    /* the file was not found */
    *location = NULL;
    return 0;
}
        
int coda_path_from_path(const char *initialpath, int is_filepath, const char *appendpath, char **resultpath)
{
    char *path;
    int initialpath_length;
    int appendpath_length;
    
    initialpath_length = strlen(initialpath);
    appendpath_length = (appendpath == NULL ? 0 : strlen(appendpath));

    if (is_filepath && initialpath_length > 0)
    {
        /* remove trailing parth */
        while (initialpath_length > 0 && initialpath[initialpath_length - 1] != '/' &&
               initialpath[initialpath_length - 1] != '\\')
        {
            initialpath_length--;
        }
    }

    *resultpath = malloc(initialpath_length + 1 + appendpath_length + 1);
    if (*resultpath == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    path = *resultpath;
    if (initialpath_length > 0)
    {
        memcpy(path, initialpath, initialpath_length);
        path += initialpath_length;
        if (appendpath_length > 0)
        {
            *path = '/';
            path++;
        }
    }
    if (appendpath_length > 0)
    {
        memcpy(path, appendpath, appendpath_length);
        path += appendpath_length;
    }
    *path = '\0';

    clean_path(*resultpath);

    return 0;
}

int coda_path_for_program(const char *argv0, char **location)
{
    const char *p;
    int is_path = 0;

    /* default (i.e. not found) is NULL */
    *location = NULL;

    if (argv0 == NULL)
    {
        return 0;
    }

    p = argv0;
    while (*p != '\0')
    {
        if (*p == '/' || *p == '\\')
        {
            is_path = 1;
            break;
        }
        p++;
    }

    if (is_path)
    {
        *location = strdup(argv0);
        if (*location == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    else
    {
        /* use PATH */
#ifdef WIN32
        int argv0_length = strlen(argv0);

        if (argv0_length <= 4 || strcmp(&argv0[argv0_length - 4], ".exe") != 0)
        {
            char *filepath;
            filepath = malloc(argv0_length + 5);
            if (filepath == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                               __LINE__);
                return -1;
            }
            strcpy(filepath, argv0);
            strcpy(&filepath[argv0_length], ".exe");
            if (coda_path_find_file(".", filepath, location) != 0)
            {
                free(filepath);
                return -1;
            }
            if (*location == NULL && getenv("PATH") != NULL)
            {
                if (coda_path_find_file(getenv("PATH"), filepath, location) != 0)
                {
                    free(filepath);
                    return -1;
                }
            }
            free(filepath);
        }
        else
        {
            if (coda_path_find_file(".", argv0, location) != 0)
            {
                return -1;
            }
            if (*location == NULL && getenv("PATH") != NULL)
            {
                if (coda_path_find_file(getenv("PATH"), argv0, location) != 0)
                {
                    return -1;
                }
            }
        }
#else
        if (getenv("PATH") != NULL)
        {
            if (coda_path_find_file(getenv("PATH"), argv0, location) != 0)
            {
                return -1;
            }
        }
        else
        {
            *location = NULL;
        }
#endif
    }

    if (*location != NULL && (*location)[0] != '/' && (*location)[0] != '\\' &&
        !(isalpha((*location)[0]) && (*location)[1] == ':'))
    {
        char cwd[1024 + 1];
        char *relative_location;
        
        /* change relative path into absolute path */

        if (getcwd(cwd, 1024) == NULL)
        {
            /* there is a problem with the current working directory -> return 'not found' */
            return 0;
        }
        cwd[1024] = '\0';
        
        relative_location = *location;
        if (coda_path_from_path(cwd, 0, relative_location, location) != 0)
        {
            free(relative_location);
            return -1;
        }
        free(relative_location);
    }
    
    return 0;    
}

void coda_path_free(char *path)
{
    free(path);
}

/** Find out whether a double value equals NaN (Not a Number).
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals NaN.
 *   \arg \c 0, The double value does not equal NaN.
 */
LIBCODA_API int coda_isNaN(double x)
{
    uint64_t e_mask, f_mask;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkNaN;

    mkNaN.as_double = x;

    e_mask = 0x7ff0;
    e_mask <<= 48;

    if ((mkNaN.as_int & e_mask) != e_mask)
        return 0;       /* e != 2047 */

    f_mask = 1;
    f_mask <<= 52;
    f_mask--;

    /* number is NaN if f does not equal zero. */
    return (mkNaN.as_int & f_mask) != 0;
}

/** Retrieve a double value that respresents NaN (Not a Number).
 * \return The double value 'NaN'.
 */
LIBCODA_API double coda_NaN(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkNaN;

    mkNaN.as_int = 0x7ff8;
    mkNaN.as_int <<= 48;

    return mkNaN.as_double;
}

/** Find out whether a double value equals inf (either positive or negative infinity).
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals inf.
 *   \arg \c 0, The double value does not equal inf.
 */
LIBCODA_API int coda_isInf(double x)
{
    return coda_isPlusInf(x) || coda_isMinInf(x);
}

/** Find out whether a double value equals +inf (positive infinity).
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals +inf.
 *   \arg \c 0, The double value does not equal +inf.
 */
LIBCODA_API int coda_isPlusInf(double x)
{
    uint64_t plusinf;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_double = x;

    plusinf = 0x7ff0;
    plusinf <<= 48;

    return mkInf.as_int == plusinf;
}

/** Find out whether a double value equals -inf (negative infinity).
 * \param x  A double value.
 * \return
 *   \arg \c 1, The double value equals -inf.
 *   \arg \c 0, The double value does not equal -inf.
 */
LIBCODA_API int coda_isMinInf(double x)
{
    uint64_t mininf;

    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_double = x;

    mininf = 0xfff0;
    mininf <<= 48;

    return mkInf.as_int == mininf;
}

/** Retrieve a double value that respresents +inf (positive infinity).
 * \return The double value '+inf'.
 */
LIBCODA_API double coda_PlusInf(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_int = 0x7ff0;
    mkInf.as_int <<= 48;

    return mkInf.as_double;
}

/** Retrieve a double value that respresents -inf (negative infinity).
* \return The double value '-inf'.
*/
LIBCODA_API double coda_MinInf(void)
{
    union
    {
        uint64_t as_int;
        double as_double;
    } mkInf;

    mkInf.as_int = 0xfff0;
    mkInf.as_int <<= 48;

    return mkInf.as_double;
}

int coda_month_to_integer(const char month[3])
{
    char month_str[4];
    
    /* just for safety reasons (strncasecmp behavior) we make 'month' a 0-terminated string */
    month_str[0] = month[0];
    month_str[1] = month[1];
    month_str[2] = month[2];
    month_str[3] = '\0';
    
    if (strncasecmp(month_str, "jan", 3) == 0)
    {
        return 1;
    }
    if (strncasecmp(month_str, "feb", 3) == 0)
    {
        return 2;
    }
    if (strncasecmp(month_str, "mar", 3) == 0)
    {
        return 3;
    }
    if (strncasecmp(month_str, "apr", 3) == 0)
    {
        return 4;
    }
    if (strncasecmp(month_str, "may", 3) == 0)
    {
        return 5;
    }
    if (strncasecmp(month_str, "jun", 3) == 0)
    {
        return 6;
    }
    if (strncasecmp(month_str, "jul", 3) == 0)
    {
        return 7;
    }
    if (strncasecmp(month_str, "aug", 3) == 0)
    {
        return 8;
    }
    if (strncasecmp(month_str, "sep", 3) == 0)
    {
        return 9;
    }
    if (strncasecmp(month_str, "oct", 3) == 0)
    {
        return 10;
    }
    if (strncasecmp(month_str, "nov", 3) == 0)
    {
        return 11;
    }
    if (strncasecmp(month_str, "dec", 3) == 0)
    {
        return 12;
    }
    
    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid month argument (%s) (%s:%u)", month_str, __FILE__, __LINE__);
    return -1;
}

static int y(int Y)
{
    return Y + (Y < 0);
}

static int coda_div(int a, int b)
{
    return a / b - (a % b < 0);
}

static int coda_mod(int a, int b)
{
    return a % b + b * (a % b < 0);
}


/* convert (D,M,Y) to a 4713BC-based Julian day-number.
 *
 * --> (1,1,-4713) yields 0; earlier dates yield negative numbers; later dates yield positive numbers.
 *                           Please note that the zero-day is basically arbitrary; we can extend the system
 *                           to include dates before it.
 * --> Note that the Julian calender was introduced in 45BC, but some initial problems meant that
 *     only after 4AD the Julian calender was kept properly. Therefore, earlier dates do not reflect
 *     dates that were in common use at the time.
 *
 * return value:
 *   -1: error occurred; (D,M,Y) does not refer to a valid Julian date.
 *    0: success; *jd is updated with the correct julian day number.
 *
 * Note that the year 0 does not exist; the year 1(AD) is preceded by 1(BC) or -1.
 */
static int coda_dmy_to_mjd2000_julian(int D, int M, int Y, int *jd)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

    /* check input */
    if (Y == 0 || M < 1 || M > 12 || D < 1 || D > tabel[M] - tabel[M - 1] + ((M == 2) && (y(Y) % 4 == 0)))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                       __FILE__, __LINE__);
        return -1;
    }

    *jd = D + 365 * y(Y) + coda_div(y(Y), 4) + tabel[M - 1] - ((M <= 2) && (y(Y) % 4 == 0)) + 1721058;

    return 0;
}

/* convert (D,M,Y) to a 4-OCT-1586-based Gregorian day-number.
 *
 * --> (4,10,1586) yields 0; earlier dates yield negative numbers; later dates yield positive numbers.
 *                           Please note that the zero-day is basically arbitrary; we can extend the system
 *                           to include dates before it. The date chosen is the last date that the Julian
 *                           Calendar system was in effect, prior to the Gregorian reform.
 * --> Many countries converted from the Gregorian to the Julian calendar after 4-OCT-1586, e.g. the English
 *     commonwealth countries (2-9-1752 was the last Julian date for them).
 *
 * return value:
 *   -1: error occurred; (D,M,Y) does not refer to a valid Gregorian date.
 *    0: success; *gd is updated with the correct Gregorian day number.
 *
 * Note that the year 0 does not exist; the year 1(AD) is preceded by 1(BC) or -1.
 */
static int coda_dmy_to_mjd2000_gregorian(int D, int M, int Y, int *gd)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

    /* check input */
    if (Y == 0 || M < 1 || M > 12 || D < 1 ||
        D > tabel[M] - tabel[M - 1] + ((M == 2) && ((y(Y) % 4 == 0) ^ (y(Y) % 100 == 0) ^ (y(Y) % 400 == 0))))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                       __FILE__, __LINE__);
        return -1;
    }

    *gd = D + tabel[M - 1] + 365 * y(Y) + coda_div(y(Y), 4) - coda_div(y(Y), 100) + coda_div(y(Y), 400) +
        -((M <= 2) && ((y(Y) % 4 == 0) - (y(Y) % 100 == 0) + (y(Y) % 400 == 0))) - 579551;

    return 0;
}

/* this function yields the day number for a date as number-of-days-since-1-1-2000.
 * The (D,M,J) is either given in Julian or Gregorian calendar; the function decides
 * which based upon the Julian (TD,TM,TY)-date, which specifies the last date when
 * the Julian calendar is in effect.
 */

/*
 * date: 01 - 01 - -4713 -> mjd2000 =   -2451545
 * date: 31 - 12 -    -1 -> mjd2000 =    -730122
 * date: 01 - 01 -    +0 -> mjd2000 = INVALID
 * date: 01 - 01 -    +1 -> mjd2000 =    -730121
 * date: 01 - 01 -  +100 -> mjd2000 =    -693962
 * date: 04 - 10 - +1586 -> mjd2000 =    -150924
 * date: 02 - 09 - +1752 -> mjd2000 =     -90324
 * date: 03 - 09 - +1752 -> mjd2000 = INVALID
 * date: 13 - 09 - +1752 -> mjd2000 = INVALID
 * date: 14 - 09 - +1752 -> mjd2000 =     -90323
 * date: 17 - 11 - +1858 -> mjd2000 =     -51544
 * date: 01 - 01 - +1950 -> mjd2000 =     -18262
 * date: 01 - 01 - +1970 -> mjd2000 =     -10957
 * date: 01 - 01 - +2000 -> mjd2000 =          0
 * date: 31 - 12 - +2501 -> mjd2000 =     183351
 */
static int coda_dmy_to_mjd2000(int D, int M, int Y, int *mjd2000)
{
    const int TD = 2;
    const int TM = 9;
    const int TY = 1752;
    int transition;
    int the_date;

    if (coda_dmy_to_mjd2000_julian(D, M, Y, &the_date) != 0 || coda_dmy_to_mjd2000_julian(TD, TM, TY, &transition) != 0)
    {
        return -1;
    }

    if (the_date <= transition)
    {
        /* Julian calendar regime */
        *mjd2000 = the_date - 2451545;
    }
    else
    {
        /* Gregorian calendar regime */
        if (coda_dmy_to_mjd2000_gregorian(D, M, Y, &the_date) != 0)
        {
            return -1;
        }

        if (the_date - 150934 <= transition - 2451545)
        {
            coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                           __FILE__, __LINE__);
            return -1;
        }

        *mjd2000 = the_date - 150934;
    }

    return 0;
}

static void coda_getday_leapyear(int dayno, int *D, int *M)
{
    const int tabel[13] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
    int i;

    assert(dayno >= 0);
    assert(dayno < 366);

    for (i = 1; i <= 12; i++)
    {
        if (dayno < tabel[i])
        {
            break;
        }
    }
    *M = i;
    *D = 1 + dayno - tabel[i - 1];
}

static void coda_getday_nonleapyear(int dayno, int *D, int *M)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
    int i;

    assert(dayno >= 0);
    assert(dayno < 365);

    for (i = 1; i <= 12; i++)
    {
        if (dayno < tabel[i])
        {
            break;
        }
    }
    *M = i;
    *D = 1 + dayno - tabel[i - 1];
}

static void coda_mjd2000_to_dmy_julian(int mjd, int *D, int *M, int *Y)
{
    int date;

    *Y = 2000;
    date = mjd - 13;    /* true date 1-1-2000 is now 0 */

    *Y += 4 * coda_div(date, 1461);
    date = coda_mod(date, 1461);

    if (date < 366)
    {
        /* first year is a leap-year */
        coda_getday_leapyear(date, D, M);
    }
    else
    {
        *Y += 1;
        date -= 366;

        *Y += coda_div(date, 365);
        date = coda_mod(date, 365);

        coda_getday_nonleapyear(date, D, M);
    }
    if (*Y <= 0)
    {
        (*Y)--;
    }
}

static void coda_mjd2000_to_dmy_gregorian(int mjd, int *D, int *M, int *Y)
{
    int date;

    *Y = 2000;
    date = mjd;

    *Y += 400 * coda_div(date, 146097);
    date = coda_mod(date, 146097);

    if (date < 36525)
    {
        /* first century */

        *Y += 4 * coda_div(date, 1461);
        date = coda_mod(date, 1461);

        if (date < 366)
        {
            /* first year is a leap_year */
            coda_getday_leapyear(date, D, M);
        }
        else
        {
            *Y += 1;
            date -= 366;

            *Y += coda_div(date, 365);
            date = coda_mod(date, 365);

            coda_getday_nonleapyear(date, D, M);
        }
    }
    else
    {
        /* second, third and fourth century */
        date -= 36525;
        *Y += 100;

        *Y += 100 * coda_div(date, 36524);
        date = coda_mod(date, 36524);

        /* first 4-year period had 1460 days, others have 1461 days. */

        if (date < 1460)
        {
            *Y += coda_div(date, 365);
            date = coda_mod(date, 365);

            coda_getday_nonleapyear(date, D, M);
        }
        else
        {
            date -= 1460;
            *Y += 4;

            *Y += 4 * coda_div(date, 1461);
            date = coda_mod(date, 1461);

            if (date < 366)
            {
                /* first year is a leap_year */
                coda_getday_leapyear(date, D, M);
            }
            else
            {
                *Y += 1;
                date -= 366;

                *Y += coda_div(date, 365);
                date = coda_mod(date, 365);

                coda_getday_nonleapyear(date, D, M);
            }
        }
    }
    if (*Y <= 0)
    {
        (*Y)--;
    }
}

static int coda_mjd2000_to_dmy(int mjd2000, int *D, int *M, int *Y)
{
    const int TD = 2;
    const int TM = 9;
    const int TY = 1752;

    int transition_date;

    if (coda_dmy_to_mjd2000(TD, TM, TY, &transition_date) != 0)
    {
        return -1;
    }

    if (mjd2000 <= transition_date)
    {
        coda_mjd2000_to_dmy_julian(mjd2000, D, M, Y);
    }
    else
    {
        coda_mjd2000_to_dmy_gregorian(mjd2000, D, M, Y);
    }

    return 0;
}

static int coda_hms_to_daytime(const long HOUR, const long MINUTE, const long SECOND, const long MUSEC, double *daytime)
{
    if (HOUR < 0 || HOUR > 23 || MINUTE < 0 || MINUTE > 59 || SECOND < 0 || SECOND > 60 || MUSEC < 0 || MUSEC > 999999)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02ld:%02ld:%02ld.%06ld) (%s:%u)",
                       HOUR, MINUTE, SECOND, MUSEC, __FILE__, __LINE__);
        return -1;
    }

    *daytime = 3600.0 * HOUR + 60.0 * MINUTE + 1.0 * SECOND + MUSEC / 1000000.0;

    return 0;
}

int coda_dayofyear_to_month_day(int year, int day_of_year, int *month, int *day_of_month)
{
    int mjd;

    if (month == NULL || day_of_month == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* check day of year value */
    if (day_of_year < 0 || day_of_year > 366)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid day of year argument (%03d) (%s:%u)", day_of_year,
                       __FILE__, __LINE__);
        return -1;
    }

    /* retrieve mjd2000 of Jan 1st of the requested year */
    if (coda_dmy_to_mjd2000(1, 1, year, &mjd) != 0)
    {
        return -1;
    }

    /* jump to day_of_year */
    mjd += (day_of_year - 1);

    /* retrieve day/month/year for mjd2000 value */
    if (coda_mjd2000_to_dmy(mjd, day_of_month, month, &year) != 0)
    {
        return -1;
    }

    return 0;
}

/** Retrieve the number of seconds since Jan 1st 2000 for a certain date and time.
 * \warning This function does _not_ perform any leap second correction. The returned value is therefore not an exact
 * UTC time
 * \param YEAR     The year.
 * \param MONTH    The month of the year (1 - 12).
 * \param DAY      The day of the month (1 - 31).
 * \param HOUR     The hour of the day (0 - 23).
 * \param MINUTE   The minute of the hour (0 - 59).
 * \param SECOND   The second of the minute (0 - 59).
 * \param MUSEC    The microseconds of the second (0 - 999999).
 * \param datetime Pointer to the variable where the amount of seconds since Jan 1st 2000 will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_datetime_to_double(int YEAR, int MONTH, int DAY, int HOUR, int MINUTE, int SECOND,
                                        int MUSEC, double *datetime)
{
    double daytime;
    int mjd2000;

    if (datetime == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "datetime argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_dmy_to_mjd2000(DAY, MONTH, YEAR, &mjd2000) != 0)
    {
        return -1;
    }

    if (coda_hms_to_daytime(HOUR, MINUTE, SECOND, MUSEC, &daytime) != 0)
    {
        return -1;
    }

    *datetime = 86400.0 * mjd2000 + daytime;

    return 0;
}

static int coda_seconds_to_hms(int dayseconds, int *H, int *M, int *S)
{
    int sec = dayseconds;

    if (sec < 0 || sec > 86399)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "dayseconds argument (%d) is not in the range [0,86400) (%s:%u)",
                       sec, __FILE__, __LINE__);
        return -1;
    }

    *H = sec / 3600;
    sec %= 3600;
    *M = sec / 60;
    sec %= 60;
    *S = sec;

    return 0;
}

/** Retrieve the decomposed date corresponding with the given amount of seconds since Jan 1st 2000.
 * \warning This function does _not_ perform any leap second correction. The returned value is therefore not an exact
 * UTC time
 * \param datetime The amount of seconds since Jan 1st 2000.
 * \param YEAR     Pointer to the variable where the year will be stored.
 * \param MONTH    Pointer to the variable where the month of the year (1 - 12) will be stored.
 * \param DAY      Pointer to the variable where the day of the month (1 - 31) will be stored.
 * \param HOUR     Pointer to the variable where the hour of the day (0 - 23) will be stored.
 * \param MINUTE   Pointer to the variable where the minute of the hour (0 - 59) will be stored.
 * \param SECOND   Pointer to the variable where the second of the minute (0 - 59) will be stored.
 * \param MUSEC    Pointer to the variable where the microseconds of the second (0 - 999999) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_double_to_datetime(double datetime, int *YEAR, int *MONTH, int *DAY, int *HOUR, int *MINUTE,
                                        int *SECOND, int *MUSEC)
{
    double seconds;
    int d, m, y;
    int h, min, sec, musec;
    int days, dayseconds;

    if (YEAR == NULL || MONTH == NULL || DAY == NULL || HOUR == NULL || MINUTE == NULL || SECOND == NULL ||
        MUSEC == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_isNaN(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is NaN (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (coda_isInf(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is Infinite (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* we add 0.5 milliseconds so the floor() becomes a round() that rounds at the millisecond */
    datetime += 5E-7;

    seconds = floor(datetime);

    days = (int)floor(seconds / 86400.0);

    if (coda_mjd2000_to_dmy(days, &d, &m, &y) != 0)
    {
        return -1;
    }

    dayseconds = (int)(seconds - days * 86400.0);

    if (coda_seconds_to_hms(dayseconds, &h, &min, &sec) != 0)
    {
        return -1;
    }

    musec = (int)floor((datetime - seconds) * 1E6);

    *YEAR = y;
    *MONTH = m;
    *DAY = d;
    *HOUR = h;
    *MINUTE = min;
    *SECOND = sec;
    *MUSEC = musec;

    return 0;

}

/** Convert a floating point time value to a string.
 * The string will be formatted as "YYYY-MM-DD HH:MM:SS.mmmmmm" and it will have a fixed length.<br>
 * The time string will be stored in the \a str parameter. This parameter should be allocated by the user
 * and should be 27 bytes long.
 *
 * The \a str parameter will be 0 terminated.
 *
 * \warning This function does not perform any leap second correction.
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \param str       String representation of the floating point time value.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_to_string(double datetime, char *str)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MUSEC;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "string argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_double_to_datetime(datetime, &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &MUSEC) != 0)
    {
        return -1;
    }
    if (YEAR < 0 || YEAR > 9999)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "the year can not be represented using a positive four digit "
                       "number");
        return -1;
    }
    sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d.%06u", YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC);

    return 0;
}


/** Convert a time string to a floating point time value.
 * The time string needs to have one of the following formats:
 * - "YYYY-MM-DD hh:mm:ss.uuuuuu"
 * - "YYYY-MM-DD hh:mm:ss"
 * - "YYYY-MM-DD"
 * - "DD-MMM-YYYY hh:mm:ss.uuuuuu"
 * - "DD-MMM-YYYY hh:mm:ss"
 * - "DD-MMM-YYYY"
 *
 * As an example, both the strings "2000-01-01 00:00:01.234567" and "01-JAN-2000 00:00:01.234567" will result in a
 * time value of 1.234567.<br>
 *
 * \warning This function does not perform any leap second correction.
 * \param str       String containing the time in one of the supported formats.
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_string_to_time(const char *str, double *datetime)
{
    int n[3];
    int DAY;
    int MONTH;
    int YEAR;
    int HOUR = 0;
    int MINUTE = 0;
    int SECOND = 0;
    int MUSEC = 0;
    int result;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "str argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (datetime == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "time argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    result = sscanf(str, "%4d-%2d-%2d%n %d:%d:%d%n.%6d%n", &YEAR, &MONTH, &DAY, &n[0], &HOUR, &MINUTE, &SECOND, &n[1],
                    &MUSEC, &n[2]);
    if (!((result == 3 && n[0] == 10 && str[10] == '\0') || (result == 6 && n[1] == 19 && str[19] == '\0') ||
          (result == 7 && n[2] == 26 && str[26] == '\0')))
    {
        char month[3];

        result = sscanf(str, "%2d-%c%c%c-%4d%n %d:%d:%d%n.%6d%n", &DAY, &month[0], &month[1], &month[2], &YEAR, &n[0],
                        &HOUR, &MINUTE, &SECOND, &n[1], &MUSEC, &n[2]);
        if (!((result == 5 && n[0] == 11 && str[11] == '\0') || (result == 8 && n[1] == 20 && str[20] == '\0') ||
              (result == 9 && n[2] == 27 && str[27] == '\0')))
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", str);
            return -1;
        }

        MONTH = coda_month_to_integer(month);
        if (MONTH == -1)
        {
            return -1;
        }
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC, datetime);
}

/** @} */
