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

#include "coda-filefilter.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#define NAME_BLOCK_SIZE 1024

typedef struct NameBuffer_struct
{
    int length;
    int room;
    char *buffer;
} NameBuffer;

ff_expr *coda_filefilter_tree;

static int coda_match_filepath(int ignore_other_file_types, coda_expression *expr, NameBuffer *path_name,
                               int (*callback) (const char *, coda_filefilter_status, const char *, void *),
                               void *userdata);

static void name_buffer_init(NameBuffer *name)
{
    assert(name != NULL);

    name->length = 0;
    name->room = NAME_BLOCK_SIZE;
    name->buffer = (char *)malloc(NAME_BLOCK_SIZE);
    assert(name->buffer != NULL);
}

static void name_buffer_done(NameBuffer *name)
{
    assert(name != NULL);

    name->length = 0;
    name->room = 0;
    free(name->buffer);
    name->buffer = NULL;
}

static void append_string_to_name_buffer(NameBuffer *name, const char *str)
{
    int length;

    assert(name != NULL);
    assert(str != NULL);

    length = strlen(str);

    if (name->length + length >= name->room)
    {
        char *new_buffer;
        int new_room;

        new_room = name->room;
        while (new_room < name->length + length)
        {
            new_room += NAME_BLOCK_SIZE;
        }
        new_buffer = (char *)malloc(new_room);
        assert(new_buffer != NULL);
        strcpy(new_buffer, name->buffer);
        free(name->buffer);
        name->buffer = new_buffer;
        name->room = new_room;
    }
    strcpy(&name->buffer[name->length], str);
    name->length += length;
}

static int coda_match_file(coda_expression *expr, NameBuffer *path_name,
                           int (*callback) (const char *, coda_filefilter_status, const char *, void *), void *userdata)
{
    coda_product *product;
    coda_cursor cursor;
    int filter_result;
    int result;

    result = coda_open(path_name->buffer, &product);
    if (result != 0 && coda_errno == CODA_ERROR_FILE_OPEN)
    {
        /* maybe not enough memory space to map the file in memory =>
         * temporarily disable memory mapping of files and try again
         */
        coda_set_option_use_mmap(0);
        result = coda_open(path_name->buffer, &product);
        coda_set_option_use_mmap(1);
    }
    if (result != 0)
    {
        if (coda_errno == CODA_ERROR_UNSUPPORTED_PRODUCT)
        {
            return callback(path_name->buffer, coda_ffs_unsupported_file, NULL, userdata);
        }
        else
        {
            return callback(path_name->buffer, coda_ffs_could_not_open_file, coda_errno_to_string(coda_errno),
                            userdata);
        }
    }

    if (coda_cursor_set_product(&cursor, product) != 0)
    {
        coda_close(product);
        return callback(path_name->buffer, coda_ffs_error, coda_errno_to_string(coda_errno), userdata);
    }
    if (coda_expression_eval_bool(expr, &cursor, &filter_result) != 0)
    {
        return callback(path_name->buffer, coda_ffs_error, coda_errno_to_string(coda_errno), userdata);
    }
    coda_close(product);


    return callback(path_name->buffer, filter_result ? coda_ffs_match : coda_ffs_no_match, NULL, userdata);
}

static int coda_match_dir(coda_expression *expr, NameBuffer *path_name,
                          int (*callback) (const char *, coda_filefilter_status, const char *, void *), void *userdata)
{
#ifdef WIN32
    WIN32_FIND_DATA FileData;

    HANDLE hSearch;
    BOOL fFinished;
    int buffer_length;
    int result;

    buffer_length = path_name->length;
    append_string_to_name_buffer(path_name, "\\*.*");

    /* Create a new directory */
    hSearch = FindFirstFile(path_name->buffer, &FileData);

    /* remove "\*.*" from path */
    path_name->length = buffer_length;
    path_name->buffer[buffer_length] = '\0';

    if (hSearch == INVALID_HANDLE_VALUE)
    {
        DWORD error;

        error = GetLastError();
        if (error == ERROR_ACCESS_DENIED)
        {
            return callback(path_name->buffer, coda_ffs_could_not_access_directory, "could not recurse into directory",
                            userdata);
        }
        else
        {
            return callback(path_name->buffer, coda_ffs_error, "could not retrieve directory entries", userdata);
        }
    }

    fFinished = FALSE;
    while (!fFinished)
    {
        if (strcmp(FileData.cFileName, ".") != 0 && strcmp(FileData.cFileName, "..") != 0)
        {
            /* add filename to path */
            append_string_to_name_buffer(path_name, "\\");
            append_string_to_name_buffer(path_name, FileData.cFileName);
            if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                result = coda_match_dir(expr, path_name, callback, userdata);
                if (result != 0)
                {
                    FindClose(hSearch);
                    return result;
                }
            }
            else
            {
                result = coda_match_file(expr, path_name, callback, userdata);
                if (result != 0)
                {
                    FindClose(hSearch);
                    return result;
                }
            }
        }

        /* reset name */
        path_name->length = buffer_length;
        path_name->buffer[buffer_length] = '\0';

        if (!FindNextFile(hSearch, &FileData))
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                fFinished = TRUE;
            }
            else
            {
                FindClose(hSearch);
                return callback(path_name->buffer, coda_ffs_error, "could not retrieve directory entry", userdata);
            }
        }
    }

    FindClose(hSearch);
#else
    DIR *dirp;
    struct dirent *dp;
    int buffer_length;
    int result;

    dirp = opendir(path_name->buffer);
    if (dirp == NULL)
    {
        return callback(path_name->buffer, coda_ffs_could_not_access_directory, "could not recurse into directory",
                        userdata);
    }
    buffer_length = path_name->length;

    while ((dp = readdir(dirp)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }
        append_string_to_name_buffer(path_name, "/");
        append_string_to_name_buffer(path_name, dp->d_name);

        result = coda_match_filepath(1, expr, path_name, callback, userdata);
        if (result != 0)
        {
            closedir(dirp);
            return result;
        }

        /* reset name */
        path_name->length = buffer_length;
        path_name->buffer[buffer_length] = '\0';
    }
    closedir(dirp);
#endif

    return 0;
}

static int coda_match_filepath(int ignore_other_file_types, coda_expression *expr, NameBuffer *path_name,
                               int (*callback) (const char *, coda_filefilter_status, const char *, void *),
                               void *userdata)
{
    struct stat sb;

    if (stat(path_name->buffer, &sb) != 0)
    {
        if (errno == ENOENT || errno == ENOTDIR)
        {
            return callback(path_name->buffer, coda_ffs_error, "no such file or directory", userdata);
        }
        else
        {
            return callback(path_name->buffer, coda_ffs_error, strerror(errno), userdata);
        }
    }

    if (sb.st_mode & S_IFDIR)
    {
        return coda_match_dir(expr, path_name, callback, userdata);
    }
    else if (sb.st_mode & S_IFREG)
    {
        return coda_match_file(expr, path_name, callback, userdata);
    }
    else if (!ignore_other_file_types)
    {
        return callback(path_name->buffer, coda_ffs_error, "not a directory or regular file", userdata);
    }

    return 0;
}

/** \addtogroup coda_general
 * @{
 */

/** Find product files matching a specific filter.
 * With this function you can match a series of files or directories against a specific filter. The filter needs to be
 * provided as a string. For information about its format please look at the documentation of the codafind tool which is
 * also included with the CODA package. If you leave \a filefilter empty or pass a NULL pointer then each file that
 * can be opened by CODA will be matched positively (this has the same effect as if you had passed a filefilter "true").
 *
 * The names of the files and directories need to be passed as an array of full/relative paths. If an entry is a
 * directory then all files and directories that are contained inside will be added to the filter matching. Directories
 * within directories are processed recursively.
 *
 * For each file that is processed a callback function, which will have to be provided by the user, will be called.
 * The callback function will be passed the path of the file, a status value indicating the match result, optionally
 * an error string in case an error happened, and the \a userdata pointer that was passed to the coda_match_filefilter()
 * function.
 * The return value of the callback function determines whether the rest of the files/directories should be processed.
 * If you return 0 from the callback function then processing will continue normally. If you return a different value,
 * then the coda_match_filefilter() function will stop further processing and return the same return value to your
 * program as you have returned from the callback function. It is recommended not to use -1 as return value in your
 * callback function, since coda_match_filefilter() will already return -1 if it encounters an error internally.
 *
 * A small example of a callback function is given below
 * \code{.c}
 * int callback(const char *filepath, coda_filefilter_status status, const char *error, void *userdata)
 * {
 *     switch (status)
 *     {
 *         case coda_ffs_match:
 *             printf("File \"%s\" matches filter!\n", filepath);
 *             break;
 *         case coda_ffs_unsupported_file:
 *         case coda_ffs_no_match:
 *             // don't print anything if the file does not positively match the filter
 *             break;
 *         default:
 *             if (error != NULL)
 *             {
 *                 printf("ERROR: %s\n", error);
 *             }
 *             break;
 *     }
 *     return 0; // if possible continue processing the other files
 * }
 * \endcode
 *
 * \param filefilter String containing the filter.
 * \param num_filepaths Number of filepaths in \a filepathlist (0 < \a num_filepaths).
 * \param filepathlist Array of strings containing relative or absolute paths to a series of files and/or
 * directories that should be matched against the filefilter.
 * \param callbackfunc  A pointer to a function that should be called with the match result of a file that has been
 * processed.
 * \param userdata A pointer to a user definable data block that will be passed along to the user callback function.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 *   \arg other, The return value from the last call to the callback function.
 */
LIBCODA_API int coda_match_filefilter(const char *filefilter, int num_filepaths, const char **filepathlist,
                                      int (*callbackfunc) (const char *, coda_filefilter_status, const char *, void *),
                                      void *userdata)
{
    NameBuffer path_name;
    coda_expression_type result_type;
    coda_expression *expr;
    int result;
    int i;

    if (num_filepaths <= 0 || filepathlist == NULL || callbackfunc == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (filefilter == NULL || filefilter[0] == '\0')
    {
        filefilter = "true";
    }

    if (coda_expression_from_string(filefilter, &expr) != 0)
    {
        return -1;
    }
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        coda_expression_delete(expr);
        return -1;
    }
    if (result_type != coda_expression_boolean)
    {
        coda_set_error(CODA_ERROR_EXPRESSION, "expression does not result in a boolean value");
        coda_expression_delete(expr);
        return -1;
    }

    name_buffer_init(&path_name);
    for (i = 0; i < num_filepaths; i++)
    {
        append_string_to_name_buffer(&path_name, filepathlist[i]);
        result = coda_match_filepath(0, expr, &path_name, callbackfunc, userdata);
        if (result != 0)
        {
            name_buffer_done(&path_name);
            coda_expression_delete(expr);
            return result;
        }
        path_name.length = 0;
        path_name.buffer[0] = '\0';
    }

    name_buffer_done(&path_name);
    coda_expression_delete(expr);

    return 0;
}

/**
 * @}
 */
