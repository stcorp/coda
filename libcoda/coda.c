/*
 * Copyright (C) 2007-2022 S[&]T, The Netherlands.
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

#include "coda-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "coda-type.h"
#include "coda-mem.h"
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif
#include "coda-grib.h"
#include "coda-rinex.h"
#include "coda-sp3.h"
#include "coda-path.h"

/** \defgroup coda_general CODA General
 * The CODA General module contains all general and miscellaneous functions and procedures of CODA.
 *
 * This module also contains the initialization and finalization functions of CODA. Before you call any other
 * function of CODA you should initialize CODA with a call to coda_init(). This function sets up the Data Dictionary
 * that describes all supported product files by reading all .codadef files from the CODA definition path.
 * After you are finished with CODA you should call coda_done(). This ensures that all resources that were claimed by
 * coda_init() are properly deallocated again. You should make sure, however, that all open product files are closed
 * before you call coda_done() (the function will not close the files for you). After a coda_done() all product file
 * handles and CODA cursors that still exist will become invalid and will stay invalid even after you call coda_init()
 * again. Having invalid CODA cursors is not a real problem, as long as you do not use them anymore, but having invalid
 * product file handles means you will be stuck with unfreed memory and unclosed files.
 *
 * In order to let CODA know where your .codadef files are stored you will either have to set the CODA_DEFINITION
 * environment variable or call the coda_set_definition_path() function (before calling coda_init()).
 *
 * If no .codadef files are loaded, CODA will still be able to provide access to HDF4, HDF5, netCDF, and XML products
 * by taking the format definition from the product files itself (for XML this will be a reduced form of access, since
 * 'leaf elements' can not be interpreted as e.g. integer/float/time but will only be accessible as string data).
 */

/** \typedef coda_filefilter_status
 * Status code that is passed to the callback function of coda_match_filefilter()
 * \ingroup coda_general
 */

/** \addtogroup coda_general
 * @{
 */

#ifndef CODA_VERSION
#define CODA_VERSION "unknown"
#endif
/** Current version of CODA as a string.
 * \hideinitializer
 */
THREAD_LOCAL const char *libcoda_version = CODA_VERSION;

LIBCODA_API const char *coda_get_libcoda_version(void)
{
    return libcoda_version;
}

static THREAD_LOCAL int coda_init_counter = 0;

THREAD_LOCAL int coda_option_bypass_special_types = 0;
THREAD_LOCAL int coda_option_perform_boundary_checks = 1;
THREAD_LOCAL int coda_option_perform_conversions = 1;
THREAD_LOCAL int coda_option_read_all_definitions = 0;
THREAD_LOCAL int coda_option_use_fast_size_expressions = 1;
THREAD_LOCAL int coda_option_use_mmap = 1;

/** Enable/Disable the use of special types.
 * The CODA type system contains a series of special types that were introduced to make it easier for the user to
 * read certain types of information. Examples of special types are the 'time', 'complex', and 'no data'
 * types. Each special data type is an abstraction on top of another non-special data type. Sometimes you want to
 * access a file using just the non-special data types (e.g. if you want to get to the raw time data in a file).
 * CODA already contains the coda_cursor_use_base_type_of_special_type() function that allows you to reinterpret the
 * current special data type using the base type of the special type. However, if you enable the bypassing of special
 * types option then CODA automatically calls the coda_cursor_use_base_type_of_special_type() for you whenever you move
 * a cursor to a data item that is of a special type.
 * By default bypassing of special types is disabled.
 * \note Bypassing of special types only works on CODA cursors and not on coda_type objects
 * (e.g. if a record field is of a special type the coda_type_get_record_field_type() function will still give you the
 * special type and not the non-special base type).
 * \param enable
 *   \arg 0: Disable bypassing of special types.
 *   \arg 1: Enable bypassing of special types.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_option_bypass_special_types(int enable)
{
    if (enable != 0 && enable != 1)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid", enable);
        return -1;
    }

    coda_option_bypass_special_types = enable;

    return 0;
}

/** Retrieve the current setting for the special types bypass option.
* \see coda_set_option_bypass_special_types()
* \return
*   \arg \c 0, Bypassing of special types is disabled.
*   \arg \c 1, Bypassing of special types is enabled.
*/
LIBCODA_API int coda_get_option_bypass_special_types(void)
{
    return coda_option_bypass_special_types;
}

/** Enable/Disable boundary checking.
 * By default all functions in libcoda perform boundary checks. However some boundary checks are quite compute
 * intensive. In order to increase performance you can turn off those compute intensive boundary checks with this
 * option. The boundary checks that are affected by this option are the ones in
 * coda_cursor_goto_array_element_by_index() and coda_cursor_goto_next_array_element().
 * Some internal functions of libcoda also call these functions so you might see speed improvements for other functions
 * too if you disable the boundary checks.
 * Mind that this option does not control the out-of-bounds check for trying to read beyond the end of the product
 * (i.e. #CODA_ERROR_OUT_OF_BOUNDS_READ).
 * \param enable
 *   \arg 0: Disable boundary checking.
 *   \arg 1: Enable boundary checking.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_option_perform_boundary_checks(int enable)
{
    if (enable != 0 && enable != 1)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid", enable);
        return -1;
    }

    coda_option_perform_boundary_checks = enable;

    return 0;
}

/** Retrieve the current setting for the boundary check option.
 * \see coda_set_option_perform_boundary_checks()
 * \return
 *   \arg \c 0, Boundary checking is disabled.
 *   \arg \c 1, Boundary checking is enabled.
 */
LIBCODA_API int coda_get_option_perform_boundary_checks(void)
{
    return coda_option_perform_boundary_checks;
}

/** Enable/Disable unit/value conversions.
 * This options allows conversions to be performed as specified in the data-dictionary.
 * If this option is enabled (the default), values that have a conversion specified will be converted to a value of
 * type double and scaled according to the conversion parameters when read.
 *
 * Both the type, unit, and value-as-read are influenced by this option for types that
 * have an associated conversion. If conversions are disabled, the type, unit, and value
 * will reflect how data is actually stored in the product file (i.e. without conversion).
 *
 * \param enable
 *   \arg 0: Disable unit/value conversions.
 *   \arg 1: Enable unit/value conversions.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_option_perform_conversions(int enable)
{
    if (!(enable == 0 || enable == 1))
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid", enable);
        return -1;
    }

    coda_option_perform_conversions = enable;

    return 0;
}

/** Retrieve the current setting for the value/unit conversion option.
 * \see coda_set_option_perform_conversions()
 * \return
 *   \arg \c 0, Unit/value conversions are disabled.
 *   \arg \c 1, Unit/value conversions are enabled.
 */
LIBCODA_API int coda_get_option_perform_conversions(void)
{
    return coda_option_perform_conversions;
}

/** Enable/Disable the use of fast size expressions.
 * Sometimes product files contain information that can be used to directly retrieve the size (or offset) of a data
 * element. If this information is redundant (i.e. the size and/or offset can also be determined in another way) then
 * CODA has a choice whether to use this information or not.
 *
 * For instance, CODA normally calculates the size of a record by calculating the sizes of all the fields and adding
 * them up. But if one of the first fields of the record contains the total size of the record, CODA can also use the
 * (often) faster approach of determining the record size by using the contents of this field.
 *
 * If the use of fast size expressions is enabled (the default), CODA will use the 'faster' method of retrieving
 * the size/offset information for a data element (e.g. use the contents of the record field that contains the record
 * size). Note that this faster method only occurs when the data element, such as the record, also has a
 * 'fast expression' associated with it (if this is the case then this expression is shown in the Product Format
 * Definition documentation for the data element).
 *
 * If this option is disabled then CODA will only use the traditional method for calculating the size (or offset) and
 * thus ignore any 'fast expressions' that may exist.
 *
 * Sometimes the size (or offset) information in a product is incorrect. If this is the case, you can disable the use
 * of fast size expressions with this option so CODA might still access the product correctly.
 *
 * \param enable
 *   \arg 0: Disable the use of fast size expressions.
 *   \arg 1: Enable the use of fast size expressions.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_option_use_fast_size_expressions(int enable)
{
    if (!(enable == 0 || enable == 1))
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid", enable);
        return -1;
    }

    coda_option_use_fast_size_expressions = enable;

    return 0;
}

/** Retrieve the current setting for the use of fast size expressions option.
 * \see coda_set_option_use_fast_size_expressions()
 * \return
 *   \arg \c 0, Unit/value conversions are disabled.
 *   \arg \c 1, Unit/value conversions are enabled.
 */
LIBCODA_API int coda_get_option_use_fast_size_expressions(void)
{
    return coda_option_use_fast_size_expressions;
}

/** Enable/Disable the use of memory mapping of files.
 * By default CODA uses a technique called 'memory mapping' to open and access data from product files.
 * The memory mapping approach is a very fast approach that uses the mmap() function to (as the term suggests) map
 * a file in memory. Accessing data from a file using mmap() greatly outperforms the alternative approach of reading
 * data using the open()/read() combination (often by a factor of 5 and sometimes even more).
 *
 * The downside of mapping a file into memory is that it takes away valuable address space. When you run a 32-bit
 * Operating System your maximum addressable memory range is 4GB (or 2GB) and if you simultaneously try to keep a few
 * large product files open your memory space can quickly become full. Opening additional files will then produce 'out
 * of memory' errors. Note that this 'out of memory' situation has nothing to do with the amount of RAM you have
 * installed in your computer. It is only related to the size of a memory pointer on your system, which is limited to
 * 4GB for a 32 bits pointer.
 *
 * If you are using CODA in a situation where you need to have multiple large product files open at the same time you
 * can turn off the use of memory mapping by using this function. Disabling the use of mmap() means that CODA will fall
 * back to the mechanism of open()/read().
 *
 * In addition, the open()/read() functionality in CODA is able to handle files that are over 4GB in size. If you are
 * running a 32-bit operating system or if your system does not support a 64-bit version of mmap then you can still
 * access such large files by disabling the mmap functionality and falling back to the open()/read() mechanism.
 *
 * \note If you change the memory mapping usage option, the new setting will only be applicable for files that will be
 * opened after you changed the option. Any files that were already open will keep using the mechanism with which they
 * were opened.
 *
 * \param enable
 *   \arg 0: Disable the use of memory mapping.
 *   \arg 1: Enable the use of memory mapping.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_option_use_mmap(int enable)
{
    if (!(enable == 0 || enable == 1))
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "enable argument (%d) is not valid", enable);
        return -1;
    }

    coda_option_use_mmap = enable;

    return 0;
}

/** Retrieve the current setting for the use of memory mapping of files.
 * \see coda_set_option_use_mmap()
 * \return
 *   \arg \c 0, Memory mapping of files is disabled.
 *   \arg \c 1, Memory mapping of files is enabled.
 */
LIBCODA_API int coda_get_option_use_mmap(void)
{
    return coda_option_use_mmap;
}


static THREAD_LOCAL char *coda_definition_path = NULL;

/** Set the searchpath for CODA product definition files.
 * This function should be called before coda_init() is called.
 *
 * The path should be a searchpath for CODA .codadef files similar like the PATH environment variable of your system.
 * Path components should be separated by ';' on Windows and by ':' on other systems.
 *
 * The path may contain both references to files and directories.
 * CODA will load all .codadef files in the path. Any specified files should be valid .codadef files. For directories,
 * CODA will (non-recursively) search the directory for all .codadef files.
 *
 * If multiple files for the same product class exist in the path, CODA will only use the one with the highest revision
 * number (this is normally equal to a last modification date that is stored in a .codadef file).
 * If there are two files for the same product class with identical revision numbers, CODA will use the definitions of
 * the first .codadef file in the path and ingore the second one.
 *
 * Specifying a path using this function will prevent CODA from using the CODA_DEFINITION environment variable.
 * If you still want CODA to acknowledge the CODA_DEFINITION environment variable then use something like this in your
 * code:
 * \code{.c}
 * if (getenv("CODA_DEFINITION") == NULL)
 * {
 *     coda_set_definition_path("<your path>");
 * }
 * \endcode
 *
 * \param path  Search path for .codadef files
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_definition_path(const char *path)
{
    if (coda_definition_path != NULL)
    {
        free(coda_definition_path);
        coda_definition_path = NULL;
    }
    if (path == NULL)
    {
        return 0;
    }
    coda_definition_path = strdup(path);
    if (coda_definition_path == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set the directory for CODA product definition files based on the location of another file.
 * This function should be called before coda_init() is called.
 *
 * This function will try to find the file with filename \a file in the provided searchpath \a searchpath.
 * The first directory in the searchpath where the file \a file exists will be appended with the relative directory
 * \a relative_location to determine the CODA product definition path. This path will be used as CODA definition path.
 * If the file could not be found in the searchpath then the CODA definition path will not be set.
 *
 * If the CODA_DEFINITION environment variable was set then this function will not perform a search or set the
 * definition path (i.e. the CODA definition path will be taken from the CODA_DEFINITION variable).
 *
 * If you provide NULL for \a searchpath then the PATH environment variable will be used as searchpath.
 * For instance, you can use coda_set_definition_path_conditional(argv[0], NULL, "../somedir") to set the CODA
 * definition path to a location relative to the location of your executable.
 *
 * The searchpath, if provided, should have a similar format as the PATH environment variable of your system. Path
 * components should be separated by ';' on Windows and by ':' on other systems.
 *
 * The \a relative_location parameter can point either to a directory (in which case all .codadef files in this
 * directory will be used) or to a single .codadef file.
 *
 * Note that this function differs from coda_set_definition_path() in two important ways:
 *  - it will not modify the definition path if the CODA_DEFINITION variable was set
 *  - it will set the definition path to just a single location (either a single file or a single directory)
 *
 * \param file Filename of the file to search for
 * \param searchpath Search path where to look for the file \a file (can be NULL)
 * \param relative_location Filepath relative to the directory from \a searchpath where \a file was found that should be
 * used to determine the CODA definition path.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_set_definition_path_conditional(const char *file, const char *searchpath,
                                                     const char *relative_location)
{
    char *location;

    if (getenv("CODA_DEFINITION") != NULL)
    {
        return 0;
    }

    if (searchpath == NULL)
    {
        if (coda_path_for_program(file, &location) != 0)
        {
            return -1;
        }
    }
    else
    {
        if (coda_path_find_file(searchpath, file, &location) != 0)
        {
            return -1;
        }
    }
    if (location != NULL)
    {
        char *path;

        if (coda_path_from_path(location, 1, relative_location, &path) != 0)
        {
            free(location);
            return -1;
        }
        free(location);
        if (coda_set_definition_path(path) != 0)
        {
            free(path);
            return -1;
        }
        free(path);
    }

    return 0;
}


/** Initializes CODA.
 * This function should be called before any other CODA function is called (except for coda_set_definition_path()).
 *
 * If you want to use CODA to access non self-describing products (i.e. where the definition is provided via a .codadef
 * file), you will have the set the CODA definition path to the location of your .codadef files before you call
 * coda_init(). This can be done either via coda_set_definition_path() or via the CODA_DEFINITION environment variable.
 *
 * It is valid to perform multiple calls to coda_init() after each other. Only the first call to coda_init() will do
 * the actual initialization and all following calls to coda_init() will only increase an initialization counter (this
 * also means that it is important that you set the CODA definition path before the first call to coda_init() is
 * performed; changing the CODA definition path afterwards will have no effect).
 * Each call to coda_init() needs to be matched by a call to coda_done() at clean-up time (i.e. the amount of calls
 * to coda_done() needs to be equal to the amount of calls to coda_init()). Only the last coda_done() call (when
 * the initialization counter has reached 0) will do the actual clean-up of CODA.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_init(void)
{
    if (coda_init_counter == 0)
    {
        if (coda_leap_second_table_init() != 0)
        {
            return -1;
        }
        if (coda_data_dictionary_init() != 0)
        {
            coda_leap_second_table_done();
            return -1;
        }
        if (coda_definition_path == NULL)
        {
            if (getenv("CODA_DEFINITION") != NULL)
            {
                coda_definition_path = strdup(getenv("CODA_DEFINITION"));
                if (coda_definition_path == NULL)
                {
                    coda_data_dictionary_done();
                    coda_leap_second_table_done();
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    return -1;
                }
            }
        }
        if (coda_definition_path != NULL)
        {
            if (coda_read_definitions(coda_definition_path) != 0)
            {
                coda_data_dictionary_done();
                /* don't clear coda_definition_path */
                coda_leap_second_table_done();
                return -1;
            }
        }
        coda_option_perform_boundary_checks = 1;
        coda_option_perform_conversions = 1;
#ifdef HAVE_HDF5
        if (coda_hdf5_init() != 0)
        {
            coda_data_dictionary_done();
            /* don't clear coda_definition_path */
            coda_leap_second_table_done();
            return -1;
        }
#endif
    }
    coda_init_counter++;

    return 0;
}

/** Finalizes CODA.
 * This function should be called to let the CODA library free up any resources it has claimed since initialization.
 * It won't however clean up any product file handlers or close any product files that are still open. So you should
 * first close any products that are still open with coda_close() before calling this function.
 *
 * It is valid to perform multiple calls to coda_init() after each other. Only the first call to coda_init() will do
 * the actual initialization and all following calls to coda_init() will only increase an initialization counter.
 * Each call to coda_init() needs to be matched by a call to coda_done() at clean-up time (i.e. the amount of calls
 * to coda_done() needs to be equal to the amount of calls to coda_init()). Only the last coda_done() call (when
 * the initialization counter has reached 0) will do the actual clean-up of CODA. The clean-up will also reset any
 * definition path that was set with coda_set_definition_path() or coda_set_definition_path_conditional().
 *
 * Calling a CODA function other than coda_init() after the final coda_done() will result in undefined behavior.
 * After reinitializing CODA again, accessing a product that was left open from a previous CODA 'session' will also
 * result in undefined behavior.
 */
LIBCODA_API void coda_done(void)
{
    if (coda_init_counter > 0)
    {
        coda_init_counter--;
        if (coda_init_counter == 0)
        {
            coda_sp3_done();
            coda_rinex_done();
            coda_grib_done();
            coda_data_dictionary_done();
            if (coda_definition_path != NULL)
            {
                free(coda_definition_path);
                coda_definition_path = NULL;
            }
            coda_mem_done();
            coda_type_done();
            coda_leap_second_table_done();
        }
    }
}

/** Free a memory block that was allocated by the CODA library.
 * In some environments the library that performs the malloc is also the one that needs to perform the free.
 * With this function memory that was allocated within the CODA library can be deallocated for such environments.
 * It should be used in the following cases:
 * - to deallocate the memory for the 'value' variables of coda_expression_eval_string()
 * \param ptr The pointer whose memory should be freed.
 */
void coda_free(void *ptr)
{
    free(ptr);
}

/** @} */
