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
 * from a product using CODA (which uses C-style indexing), the user can use this function to provide the index
 * conversions. A small example showing how this would be done with a multi-dimensional array of doubles:
 * \code
 * coda_cursor_get_num_elements(cursor, &num_elements);
 * if (num_elements > 0)
 * {
 *     coda_cursor_get_array_dim(cursor, &num_dim, dims);
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
                    if (!(to >= 2 && path[to - 1] == '.' && path[to - 2] == '.' &&
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

/** Write 64 bit signed integer to a string.
 * The string \a s will be 0 terminated.
 * \param a  A signed 64 bit integer value.
 * \param s  A character buffer that is at least 21 bytes long.
 */
LIBCODA_API void coda_str64(int64_t a, char *s)
{
    if (a < 0)
    {
        s[0] = '-';
        coda_str64u((uint64_t)(-a), &s[1]);
    }
    else
    {
        coda_str64u((uint64_t)a, s);
    }
}

/** Write 64 bit unsigned integer to a string.
 * The string \a s will be 0 terminated.
 * \param a  An unsigned 64 bit integer value.
 * \param s  A character buffer that is at least 20 bytes long.
 */
LIBCODA_API void coda_str64u(uint64_t a, char *s)
{
    if (a <= 4294967295UL)
    {
        sprintf(s, "%ld", (long)a);
    }
    else
    {
        long a1, a2;

        a1 = (long)(a % 100000000);
        a /= 100000000;
        a2 = (long)(a % 100000000);
        a /= 100000000;
        if (a != 0)
        {
            sprintf(s, "%ld%08ld%08ld", (long)a, a2, a1);
        }
        else
        {
            sprintf(s, "%ld%08ld", a2, a1);
        }
    }
}

/** Write floating point value to a string.
 * This will write the floating point value to a string using the printf '%.16g' format.
 * However, if the floating point is NaN or Inf it will write 'nan', '+inf', or '-inf' (instead of the platform
 * specific alternative that printf might have used).
 * The string \a s will be 0 terminated.
 * \param a  A floating point value.
 * \param s  A character buffer that is at least 24 bytes long.
 */
LIBCODA_API void coda_strfl(double a, char *s)
{
    if (coda_isNaN(a))
    {
        strcpy(s, "nan");
    }
    else if (coda_isPlusInf(a))
    {
        strcpy(s, "+inf");
    }
    else if (coda_isMinInf(a))
    {
        strcpy(s, "-inf");
    }
    else
    {
        sprintf(s, "%.16g", a);
    }
}

/** @} */
