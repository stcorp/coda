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

#include "coda-internal.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-bin.h"
#include "coda-cdf.h"
#include "coda-xml.h"
#include "coda-netcdf.h"
#include "coda-grib.h"
#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif
#include "coda-rinex.h"
#include "coda-sp3.h"
#include "coda-definition.h"

/** \defgroup coda_product CODA Product
 * The CODA Product module contains functions and procedures to open, close and retrieve information about product
 * files that are supported by CODA.
 *
 * Under the hood CODA uses several different backends to access data from products. There are backends for structured
 * ascii, structured binary, XML, netCDF, HDF4, HDF5, and several other data formats.
 * Some formats such as netCDF, HDF4, and HDF5 are self describing product formats. This means that CODA will retrieve
 * information about the structural layout and contents from the file itself. For other formats, such as XML, CODA can
 * either use an external definition (from a .codadef file) to interpret an XML file (similar like an XML Schema) or it
 * can try to retrieve structural layout of the file from the file itself.
 * For XML this last option will result in a reduced form of access, since 'leaf elements' can not be interpreted as
 * e.g. integer/float/time but will only be accessible as string data.
 * For the interpretation of structured ascii and structured binary files (or a combination of both) CODA purely relies
 * on the format definitions that are provided in the .codadef files.
 *
 * In order to be able to open product files with CODA you will first have to initialize CODA with coda_init() (see
 * \link coda_general CODA General\endlink). This initialization routine will initialize all available backends and will
 * search for all .codadef files in your CODA definition path to read the necessary descriptions of all non
 * self-describing products (note that you want to use .codadef files, you will need to have set the location of your
 * CODA definition path using coda_set_definition_path() or via the CODA_DEFINITION environment variable before
 * calling coda_init()).
 * As a user you can access all supported products in the same way no matter which format the product uses underneath.
 * This means that you can use the same functions for opening, traversing, reading, and closing a product no matter
 * whether you are accessing an ascii, binary, XML, netCDF, HDF4, HDF5, etc. formatted file.
 *
 * To open a product file you will have to use the coda_open() function. This function takes as only parameter the
 * filename of the product file. CODA will then open the file and automatically check what type of file it is.
 * If it is an HDF4 or HDF5 file it will use the HDF4/HDF5 backends for further access. In all other cases CODA will
 * consult the data dictionary to determine whether there is a product definition for that file in one of the available
 * product classes.
 *
 * Within CODA a product class is a grouping of related product types. Usually all data products for a single satellite
 * mission belong to the same product class. Within a product class there can be several product types and each product
 * type can have multiple versions of its format (this is because product format descriptions will sometimes change
 * during the lifetime of a product type). The combination of product class, product type and product version number
 * uniquely defines the description that will be used to interpret a product file.
 *
 * If CODA can not determine the product class, type, or version of a structured ascii/binary file, the file will not
 * be opened and an error will be returned. For other formats such as XML, netCDF, HDF4, and HDF5 files CODA will open
 * and interpret the data based on the file contents.
 * If everything was successful, the coda_open() function will provide you a file handle (of type #coda_product)
 * that can be passed to a range of other functions to retrieve information like the product class, type and version,
 * or to read data from the file with the help of CODA cursors (see \link coda_cursor CODA Cursor\endlink).
 * After you are done with a file you should close it with coda_close(). This function will also free the memory
 * that was allocated for the file handle by coda_open().
 * Below is a simple example that opens a file called productfile.dat and closes it again.
 * \code{.c}
 * coda_product *product;
 * if (coda_init() != 0)
 * {
 *     fprintf(stderr, "Error: %s\n", coda_errno_to_string(coda_errno));
 *     exit(1);
 * }
 * if (coda_open("productfile.dat", &product) != 0)
 * {
 *     fprintf(stderr, "Error: %s\n", coda_errno_to_string(coda_errno));
 *     exit(1);
 * }
 * coda_close(product);
 * coda_done();
 * \endcode
 *
 * It is possible to have multiple product files open at the same time. Just call coda_open() again on a different file
 * and you will get a new file handle. It is also possible to open a single product file multiple times (although this
 * is a feature we encourage you to avoid on 32-bit systems because of the mmap() limitations - see
 * coda_set_option_use_mmap()). In that case CODA will just return a second product file handle which is completely
 * independent of the first product file handle you already had.
 */

/** \typedef coda_product
 * CODA Product handle
 * \ingroup coda_product
 */

/** \enum coda_format_enum
 * The data storage formats that are supported by CODA
 * \ingroup coda_product
 */

/** \typedef coda_format
 * The data storage formats that are supported by CODA
 * \ingroup coda_product
 */

/** \addtogroup coda_product
 * @{
 */

#define DETECTION_BLOCK_SIZE 80

static int get_file_size(const char *filename, int64_t *file_size)
{
    struct stat statbuf;

    assert(filename != NULL && file_size != NULL);

    /* stat() the file to be opened */

    if (stat(filename, &statbuf) != 0)
    {
        if (errno == ENOENT)
        {
            coda_set_error(CODA_ERROR_FILE_NOT_FOUND, "could not find %s", filename);
        }
        else
        {
            coda_set_error(CODA_ERROR_FILE_OPEN, "could not open %s (%s)", filename, strerror(errno));
        }
        return -1;
    }

    /* check that the file is a regular file */

    if ((statbuf.st_mode & S_IFREG) == 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open %s (not a regular file)", filename);
        return -1;
    }

    /* get file size */

    *file_size = statbuf.st_size;

    return 0;
}

static int get_format(coda_product *raw_product, coda_format *format)
{
    unsigned char buffer[DETECTION_BLOCK_SIZE];
    coda_cursor cursor;
    int64_t file_size;
    int64_t offset;

    file_size = raw_product->file_size;

    if (coda_cursor_set_product(&cursor, raw_product) != 0)
    {
        return -1;
    }

    /* default is binary */
    *format = coda_format_binary;

    if (file_size < 4)
    {
        return 0;
    }

    if (coda_cursor_read_bytes(&cursor, buffer, 0, 4) != 0)
    {
        return -1;
    }

    /* netCDF */
    if (memcmp(buffer, "CDF", 3) == 0 && (buffer[3] == '\001' || buffer[3] == '\002'))
    {
        *format = coda_format_netcdf;
        return 0;
    }

    /* HDF4 */
    if (memcmp(buffer, "\016\003\023\001", 4) == 0)
    {
        *format = coda_format_hdf4;
        return 0;
    }

    if (file_size < 8)
    {
        *format = coda_format_binary;
        return 0;
    }

    /* read additional 4 bytes so we end up with the first 8 bytes */
    if (coda_cursor_read_bytes(&cursor, &buffer[4], 4, 4) != 0)
    {
        return -1;
    }

    /* HDF5 */
    if (memcmp(buffer, "\211HDF\r\n\032\n", 8) == 0)
    {
        *format = coda_format_hdf5;
        return 0;
    }

    /* CDF */
    if (memcmp(buffer, "\000\000\377\377\000\000\377\377", 8) == 0 ||   /* 0x0000FFFF 0x0000FFFF */
        memcmp(buffer, "\315\362\140\002\000\000\377\377", 8) == 0 ||   /* 0xCDF26002 0x0000FFFF */
        memcmp(buffer, "\315\362\140\002\314\314\000\001", 8) == 0 ||   /* 0xCDF26002 0xCCCC0001 */
        memcmp(buffer, "\315\363\000\001\000\000\377\377", 8) == 0 ||   /* 0xCDF30001 0x0000FFFF */
        memcmp(buffer, "\315\363\000\001\314\314\000\001", 8) == 0)     /* 0xCDF30001 0xCCCC0001 */
    {
        *format = coda_format_cdf;
        return 0;
    }

    /* GRIB */
    if (memcmp(buffer, "GRIB", 4) == 0)
    {
        if (buffer[7] == '\001' || buffer[7] == '\002')
        {
            *format = coda_format_grib;
            return 0;
        }
    }

    /* XML */
    /* we do not support UTF-16, but otherwise the following compares should be used:
     * UTF-16 BE no BOM   : memcmp(buffer, "\000<\000?\000x\000m\000l", 10)
     * UTF-16 LE no BOM   : memcmp(buffer, "<\000?\000x\000m\000l\000", 10)
     * UTF-16 BE with BOM : memcmp(buffer, "\376\377\000<\000?\000x\000m\000l", 12) == 0
     * UTF-16 LE with BOM : memcmp(buffer, "\377\376<\000?\000x\000m\000l\000", 12) == 0)
     * also increase the file_size check to '>= 12'
     */
    if (memcmp(buffer, "<?xml", 5) == 0 ||      /* UTF-8 no BOM */
        memcmp(buffer, "\357\273\277<?xml", 8) == 0)    /* UTF-8 with BOM */
    {
        *format = coda_format_xml;
        return 0;
    }

    if (file_size < 40)
    {
        return 0;
    }

    /* read additional 32 bytes */
    if (coda_cursor_read_bytes(&cursor, &buffer[8], 8, 32) != 0)
    {
        return -1;
    }

    /* SP3 */
    if (file_size >= 60)
    {
        if (buffer[0] == '#' && (buffer[1] == 'a' || buffer[1] == 'b' || buffer[1] == 'c') &&
            (buffer[2] == 'P' || buffer[2] == 'V') && buffer[3] >= '0' && buffer[3] <= '9' && buffer[4] >= '0' &&
            buffer[4] <= '9' && buffer[5] >= '0' && buffer[5] <= '9' && buffer[6] >= '0' && buffer[6] <= '9' &&
            buffer[7] == ' ' && buffer[10] == ' ' && buffer[13] == ' ' && buffer[16] == ' ' && buffer[19] == ' ' &&
            buffer[31] == ' ' && buffer[39] == ' ')
        {
            *format = coda_format_sp3;
            return 0;
        }
    }

    if (file_size < 80)
    {
        return 0;
    }

    /* read 20 bytes at offset 60 */
    if (coda_cursor_read_bytes(&cursor, buffer, 60, 20) != 0)
    {
        return -1;
    }

    /* RINEX */
    if (memcmp(buffer, "RINEX VERSION / TYPE", 20) == 0)
    {
        *format = coda_format_rinex;
        return 0;
    }

    /* HDF5 with custom header information */
    offset = 512;
    while (offset + 8 < file_size)
    {
        if (coda_cursor_read_bytes(&cursor, buffer, offset, 8) != 0)
        {
            return -1;
        }
        if (memcmp(buffer, "\211HDF\r\n\032\n", 8) == 0)
        {
            *format = coda_format_hdf5;
            return 0;
        }
        offset *= 2;
    }

    return 0;
}

static int reopen_with_backend(coda_product **product_file, coda_format format)
{
    /* the input product_file is a raw binary product
     * this will be changed to whatever product file that is applicable for the backend for the format
     * the coda_<backend>_reopen functions are responsible for closing the input raw product (even when errors occur)
     */
    switch (format)
    {
        case coda_format_ascii:
            /* at this stage ascii/binary products are still treated as binary */
            assert(0);
            exit(1);
        case coda_format_binary:
            /* nothing to do */
            break;
        case coda_format_xml:
            if (coda_xml_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            if (coda_hdf5_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_cdf:
            if (coda_cdf_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_netcdf:
            if (coda_netcdf_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_grib:
            if (coda_grib_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_rinex:
            if (coda_rinex_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_sp3:
            if (coda_sp3_reopen(product_file) != 0)
            {
                return -1;
            }
            break;
    }

    return 0;
}

static int set_definition(coda_product **product, coda_product_definition *definition)
{
    if (definition == NULL)
    {
        if ((*product)->format == coda_format_binary || (*product)->format == coda_format_ascii)
        {
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, NULL);
            return -1;
        }
        /* if definition == NULL we have nothing to do */
        return 0;
    }

    if ((*product)->format != definition->format &&
        !((*product)->format == coda_format_binary && definition->format == coda_format_ascii))
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "cannot use %s definition for %s product",
                       coda_type_get_format_name(definition->format), coda_type_get_format_name((*product)->format));
        return -1;
    }

    if (!definition->initialized)
    {
        /* make sure that the root type and product variables of the product definition are initialized */
        if (coda_read_product_definition(definition) != 0)
        {
            return -1;
        }
    }

    /* unlike the coda_<backend>_reopen functions, the coda_<backend>_reopen_with_definition functions are _not_
     * responsible for closing the input product (even when errors occur)
     */
    switch (definition->format)
    {
        case coda_format_ascii:
            if (coda_ascii_reopen_with_definition(product, definition) != 0)
            {
                return -1;
            }
            break;
        case coda_format_binary:
            if (coda_bin_reopen_with_definition(product, definition) != 0)
            {
                return -1;
            }
            break;
        case coda_format_xml:
            if (coda_xml_reopen_with_definition(product, definition) != 0)
            {
                return -1;
            }
            break;
        default:
            /* self describing format */
            (*product)->product_definition = definition;
            break;
    }

    /* initialize product variables */
    if ((*product)->product_definition->num_product_variables > 0)
    {
        int num_product_variables;
        int i;

        num_product_variables = (*product)->product_definition->num_product_variables;
        (*product)->product_variable_size = malloc(num_product_variables * sizeof(long *));
        if ((*product)->product_variable_size == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           num_product_variables * sizeof(long *), __FILE__, __LINE__);
            return -1;
        }
        (*product)->product_variable = malloc(num_product_variables * sizeof(int64_t **));
        if ((*product)->product_variable == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           num_product_variables * sizeof(int64_t **), __FILE__, __LINE__);
            return -1;
        }

        for (i = 0; i < num_product_variables; i++)
        {
            (*product)->product_variable[i] = NULL;
        }
    }

    return 0;
}

static int open_file(const char *filename, coda_product **product_file, int force_binary)
{
    coda_product *product;
    int64_t file_size;
    coda_format format;

    if (get_file_size(filename, &file_size) != 0)
    {
        return -1;
    }

    /* we open the file as a 'raw file' which maps the whole file as a single binary raw data block */
    if (coda_bin_open(filename, file_size, &product) != 0)
    {
        return -1;
    }

    if (force_binary)
    {
        format = coda_format_binary;
    }
    else
    {
        if (get_format(product, &format) != 0)
        {
            coda_close(product);
            return -1;
        }
    }

    if (reopen_with_backend(&product, format) != 0)
    {
        /* no need to close 'product' as this should already have been done by the backend */
        return -1;
    }

    *product_file = product;

    return 0;
}


/** Determine the file size, format, product class, product type, and format version of a product file.
 * This function will perform an open and close on the product file and will try to automatically recognize
 * the product class, type, and version of the product file.
 * If the file is a netCDF, HDF4, or HDF5 file the \a file_format will be set, but \a product_class and \a product_type
 * will be set to NULL and \a version will be set to -1.
 * For XML the \a product_class, \a product_type, and \a version will only be set if there is an external definition
 * available for the product (i.e. from one of the .codadef files in your CODA definition path).
 * Otherwise the values will be NULL/-1.
 * If a description of the product file is included in the data dictionary the product class, type, and version will be
 * set according to what the automatic recognition rules have determined.
 * The file_size will be set to the actual byte size of the file.
 * It is possible to pass a NULL pointer for one or more of the parameters \a file_size, \a file_format,
 * \a product_class, \a product_type, and \a product_version. If the parameter is NULL no value for this parameter is
 * returned.
 * The string pointers that are returned for \a product_class and \a product_type do not have to be freed by the user
 * and will remain valid until coda_done() is called.
 * \param filename Relative or full path to the product file.
 * \param file_size Pointer to the variable where the actual file size in bytes will be stored.
 * \param file_format Pointer to the variable where the file format value will be stored.
 * \param product_class Pointer to the variable where the product class string will be stored.
 * \param product_type Pointer to the variable where the product type string will be stored.
 * \param version Pointer to the variable where the product format version number will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_recognize_file(const char *filename, int64_t *file_size, coda_format *file_format,
                                    const char **product_class, const char **product_type, int *version)
{
    coda_product_definition *definition = NULL;
    coda_product *product;

    if (open_file(filename, &product, 0) != 0)
    {
        return -1;
    }
    if (coda_data_dictionary_find_definition_for_product(product, &definition) != 0)
    {
        coda_close(product);
        return -1;
    }

    if (file_size != NULL)
    {
        *file_size = product->file_size;
    }
    if (definition == NULL)
    {
        if (file_format != NULL)
        {
            *file_format = product->format;
        }
        if (product_class != NULL)
        {
            *product_class = NULL;
        }
        if (product_type != NULL)
        {
            *product_type = NULL;
        }
        if (version != NULL)
        {
            *version = -1;
        }
    }
    else
    {
        if (file_format != NULL)
        {
            *file_format = definition->format;
        }
        if (product_class != NULL)
        {
            *product_class = definition->product_type->product_class->name;
        }
        if (product_type != NULL)
        {
            *product_type = definition->product_type->name;
        }
        if (version != NULL)
        {
            *version = definition->version;
        }
    }

    coda_close(product);

    return 0;
}

/** Open a product file for reading.
 * This function will try to open the specified file for reading. On success a newly allocated file handle will be
 * returned. The memory for this file handle will be released when coda_close() is called for this handle.
 * \param filename Relative or full path to the product file.
 * \param product Pointer to the variable where the pointer to the product file handle will be storeed.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_open(const char *filename, coda_product **product)
{
    coda_product_definition *definition = NULL;
    coda_product *product_file;

    if (filename == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "filename argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (open_file(filename, &product_file, 0) != 0)
    {
        return -1;
    }
    if (coda_data_dictionary_find_definition_for_product(product_file, &definition) != 0)
    {
        coda_close(product_file);
        return -1;
    }
    if (set_definition(&product_file, definition) != 0)
    {
        coda_close(product_file);
        return -1;
    }

    *product = product_file;

    return 0;
}

/** Open a product file for reading using a specific format definition.
 * This function will try to open the specified file for reading similar to coda_open(), but instead of trying to
 * automatically recognise the applicable product class/type/version as coda_open() does, this function will impose
 * the format definition that is associated with the given \a product_class, \a product_type, and \a version parameters.
 * \param filename Relative or full path to the product file.
 * \param product_class Name of the product class for the requested format definition.
 * \param product_type Name of the product type for the requested format definition.
 * \param version Format version number of the product type definition. Use -1 to request the latest available definition.
 * \param product Pointer to the variable where the pointer to the product file handle will be storeed.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_open_as(const char *filename, const char *product_class, const char *product_type, int version,
                             coda_product **product)
{
    coda_product_definition *definition = NULL;
    coda_product *product_file;
    int open_as_binary = 0;

    if (filename == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "filename argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_class != NULL)
    {
        if (product_type == NULL)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product_type argument is NULL (%s:%u)", __FILE__, __LINE__);
            return -1;
        }
        if (product == NULL)
        {
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product argument is NULL (%s:%u)", __FILE__, __LINE__);
            return -1;
        }
    }

    if (product_class != NULL)
    {
        if (coda_data_dictionary_get_definition(product_class, product_type, version, &definition) != 0)
        {
            return -1;
        }
        if (definition != NULL)
        {
            /* we allow self-describing file formats to be opened as a binary/ascii file with a definition */
            open_as_binary = definition->format == coda_format_ascii || definition->format == coda_format_binary;
        }
    }

    if (open_file(filename, &product_file, open_as_binary) != 0)
    {
        return -1;
    }
    /* make sure to also set definition if definition==NULL (to trigger checks on whether that is allowed) */
    if (set_definition(&product_file, definition) != 0)
    {
        coda_close(product_file);
        return -1;
    }

    *product = product_file;

    return 0;
}

/** Close an open product file.
 * This function will close the file associated with the file handle and release the memory for the handle.
 * The file handle will be released even if unmapping or closing of the product file produced an error.
 * \param product Pointer to a product file handle. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_close(coda_product *product)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* remove product variable information */
    if (product->product_variable_size != NULL)
    {
        free(product->product_variable_size);
        product->product_variable_size = NULL;
    }
    if (product->product_variable != NULL)
    {
        int i;

        for (i = 0; i < product->product_definition->num_product_variables; i++)
        {
            if (product->product_variable[i] != NULL)
            {
                free(product->product_variable[i]);
            }
        }
        free(product->product_variable);
        product->product_variable = NULL;
    }

    switch (product->format)
    {
        case coda_format_ascii:
            return coda_ascii_close(product);
        case coda_format_binary:
            return coda_bin_close(product);
        case coda_format_xml:
            return coda_xml_close(product);
        case coda_format_cdf:
            return coda_cdf_close(product);
        case coda_format_netcdf:
            return coda_netcdf_close(product);
        case coda_format_grib:
            return coda_grib_close(product);
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_close(product);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_close(product);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_rinex:
            return coda_rinex_close(product);
        case coda_format_sp3:
            return coda_sp3_close(product);
    }

    assert(0);
    exit(1);
}

/** Get the filename of a product file.
 * This function returns the same name that was used in the coda_open() call for this product file.
 * The pointer to the filename string is valid as long as the file is open. When you call coda_close() on the
 * product file the filename string will automatically be removed and the pointer that will be stored in \a filename
 * will become invalid.
 * The name of the product file will be stored in the \a filename parameter and will be 0 terminated.
 * \param product Pointer to a product file handle. 
 * \param filename Pointer to the variable where the filename of the product will be stored. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_filename(const coda_product *product, const char **filename)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *filename = product->filename;

    return 0;
}

/** Get the actual file size of a product file.
 * \param product Pointer to a product file handle. 
 * \param file_size Pointer to the variable where the actual file size (in bytes) of the product is stored. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_file_size(const coda_product *product, int64_t *file_size)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (file_size == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "file_size argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *file_size = product->file_size;

    return 0;
}

/** Get the basic file format of the product.
 * Possible formats are ascii, binary, xml, netcdf, grib, hdf4, cdf, and hdf5.
 * Mind that inside a product different typed data can exist. For instance, both xml and binary products can have
 * part of their content be ascii typed data.
 * \param product Pointer to a product file handle. 
 * \param format Pointer to the variable where the format will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_format(const coda_product *product, coda_format *format)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *format = product->format;

    return 0;
}

/** Get the product class of a product file.
 * This function will return the name of the product class of a product.
 * The name of the product class will be stored in the \a product_class parameter and will be 0 terminated.
 * The string pointer that is returned for \a product_class does not have to be freed by the user and will remain valid
 * until coda_done() is called.
 * \param product Pointer to a product file handle. 
 * \param product_class Pointer to the variable where the class name of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_class(const coda_product *product, const char **product_class)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_class == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product_class argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (product->product_definition != NULL)
    {
        *product_class = product->product_definition->product_type->product_class->name;
    }
    else
    {
        *product_class = NULL;
    }

    return 0;
}

/** Get the product type of a product file.
 * This function will return the name of the product type of a product.
 * The name of the product type will be stored in the \a product_type parameter and will be 0 terminated.
 * The string pointer that is returned for \a product_type does not have to be freed by the user and will remain valid
 * until coda_done() is called.
 * \param product Pointer to a product file handle. 
 * \param product_type Pointer to the variable where the product type name of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_type(const coda_product *product, const char **product_type)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (product->product_definition != NULL)
    {
        *product_type = product->product_definition->product_type->name;
    }
    else
    {
        *product_type = NULL;
    }

    return 0;
}

/** Get the product type version of a product file.
 * This function will return the format version number of a product. This version number is a rounded number and newer
 * versions of a format will always have a version number that is higher than that of older formats.
 * \param product Pointer to a product file handle. 
 * \param version Pointer to the variable where the product version of the product type of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_version(const coda_product *product, int *version)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (version == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "version argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (product->product_definition != NULL)
    {
        *version = product->product_definition->version;
    }
    else
    {
        *version = -1;
    }

    return 0;
}

/** Get the CODA type of the root of the product.
 * For self-describing data formats the definition from the codadef file will be returned if it exists,
 * otherwise the definition based on the format as extracted from the product itself will be returned.
 *
 * Note that for self-describing products with a codadef definition (except for xml) the product itself will always
 * be interpreted using the definition as extracted from the product itself. The coda_get_product_root_type() function
 * is then the means to retrieve the definition from the codadef and calling coda_cursor_get_type() for a cursor that
 * points to the root of the product will return the definition as extracted from the product.
 *
 * \param product Pointer to a product file handle. 
 * \param type Pointer to the variable where the Type handle will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_root_type(const coda_product *product, coda_type **type)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (product->product_definition != NULL && product->product_definition->root_type != NULL)
    {
        *type = product->product_definition->root_type;
    }
    else
    {
        *type = coda_get_type_for_dynamic_type(product->root_type);
    }

    return 0;
}

/** Get path to the coda definition file that describes the format for this product.
 * This function will return a full path to the coda definition (.codadef) file that contains the format description
 * for this product. If the format is not taken from an external coda definition description but based on the
 * self-describing format information from the file itself or based on a hardcoded format definition within one of the
 * coda backends then \a definition_file will be set to NULL.
 * \param product Pointer to a product file handle. 
 * \param definition_file Pointer to the variable where the path to used coda definition file will be stored (or NULL if
 * not applicable).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_definition_file(const coda_product *product, const char **definition_file)
{
    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (definition_file == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "definition_file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (product->product_definition != NULL)
    {
        *definition_file = product->product_definition->product_type->product_class->definition_file;
    }
    else
    {
        *definition_file = NULL;
    }

    return 0;
}

/** Get the value for a product variable.
 * CODA supports a mechanism called product variables to store frequently needed information of a product (i.e.
 * information that is needed to calculate byte offsets or array sizes within a product). With this function you
 * can retrieve the values for those product variables (consult the CODA Product Definition Documentation for an
 * overview of product variables for a certain product type).
 * Product variables can be one dimensional arrays, in which case you will have to pass an array index using the
 * \a index parameter. If the product variable is a scalar you should pass 0 for \a index.
 * The value of a product variable is always a 64-bit integer and will be stored in \a value.
 * \param product Pointer to a product file handle. 
 * \param variable The name of the product variable.
 * \param index The array index of the product variable (pass 0 if the variable is a scalar).
 * \param value Pointer to the variable where the product variable value will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_variable_value(coda_product *product, const char *variable, long index, int64_t *value)
{
    int64_t *variable_ptr;
    long size;

    if (product == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (variable == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "variable argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* use the size retrieval function to check the existence of the variable */
    if (coda_product_variable_get_size(product, variable, &size) != 0)
    {
        coda_set_error(CODA_ERROR_INVALID_NAME, "product variable %s not available", variable);
        return -1;
    }
    if (index < 0 || index >= size)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "request for index (%ld) exceeds size of product variable %s",
                       index, variable);
        return -1;
    }

    if (coda_product_variable_get_pointer(product, variable, index, &variable_ptr) != 0)
    {
        return -1;
    }

    *value = *variable_ptr;

    return 0;
}

/** @} */
