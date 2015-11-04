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
#include "coda-xml.h"
#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif
#include "coda-definition.h"

/** \defgroup coda_productfile CODA Product File
 * The CODA Product File module contains functions and procedures to open, close and retrieve information about product
 * files that are supported by CODA.
 *
 * Under the hood CODA uses five different backends to access data from products. There are backends for structured
 * ascii, structured binary, XML, HDF4, and HDF5 data formats.
 * HDF4 and HDF5 are self describing product format. This means that CODA will retrieve information about the structural
 * layout and contents from the file itself. For XML, CODA can either use an external definition (from a .codadef file)
 * to interpret an XML file (similar like an XML Schema) or it can try to retrieve structural layout of the file from
 * the file itself. This last option will result in a reduced form of access, since 'leaf elements' can not be
 * interpreted as e.g. integer/float/time but will only be accessible as string data.
 * For the interpretation of structured ascii and structured binary files (or a combination of both) CODA purely relies
 * on the format definitions that are provided in the .codadef files.
 *
 * In order to be able to open product files with CODA you will first have to initialize CODA with coda_init() (see
 * \link coda_general CODA General\endlink). This initialization routine will initialize all available backends and will
 * search for all .codadef files in your CODA definition path to read the necessary descriptions of all non
 * self-describing products.
 * As a user you can access all supported products in the same way no matter which format the product uses underneath.
 * This means that you can use the same functions for opening, traversing, reading,
 * and closing a product no matter whether you are accessing an ascii, binary, XML, HDF4, or HDF5 file.
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
 * be opened and an error will be returned. For XML, HDF4, and HDF5 files CODA will open and interpret the data based
 * on the file contents.
 * If everything was successful, the coda_open() function will provide you a file handle (of type #coda_ProductFile)
 * that can be passed to a range of other functions to retrieve information like the product class, type and version,
 * or to read data from the file with the help of CODA cursors (see \link coda_cursor CODA Cursor\endlink).
 * After you are done with a file you should close it with coda_close(). This function will also free the memory
 * that was allocated for the file handle by coda_open().
 * Below is a simple example that opens a file called productfile.dat and closes it again.
 * \code
 * coda_ProductFile *pf;
 * coda_init();
 * pf = coda_open("productfile.dat");
 * if (pf == NULL)
 * {
 *   fprintf(stderr, "Error: %s\n", coda_errno_to_string(coda_errno));
 *   exit(1);
 * }
 * coda_close(pf);
 * coda_done();
 * \endcode
 *
 * It is possible to have multiple product files open at the same time. Just call coda_open() again on a different file
 * and you will get a new file handle. It is also possible to open a single product file multiple times (although this
 * is a feature we encourage you to avoid because of the mmap() limitations - see coda_set_option_use_mmap()).
 * In that case CODA will just return a second product file handle which is completely independent of the first product
 * file handle you already had.
 */

/** \typedef coda_ProductFile
 * CODA Product File handle
 * \ingroup coda_productfile
 */

/** \enum coda_format_enum
 * The data storage formats that are supported by CODA
 * \ingroup coda_productfile
 */

/** \typedef coda_format
 * The data storage formats that are supported by CODA
 * \ingroup coda_productfile
 */

/** \addtogroup coda_productfile
 * @{
 */

#define DETECTION_BLOCK_SIZE 12

static int get_format_and_size(const char *filename, coda_format *format, int64_t *file_size)
{
    unsigned char buffer[DETECTION_BLOCK_SIZE];
    struct stat statbuf;
    int open_flags;
    int fd;

    assert(filename != NULL && format != NULL && file_size != NULL);

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

    /* open file and read detection block */

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    fd = open(filename, open_flags);
    if (fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open %s (%s)", filename, strerror(errno));
        return -1;
    }

    if (*file_size > 0)
    {
        if (read(fd, buffer, *file_size < DETECTION_BLOCK_SIZE ? (size_t)*file_size : DETECTION_BLOCK_SIZE) == -1)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", filename, strerror(errno));
            close(fd);
            return -1;
        }
    }

    close(fd);

    /* HDF4 */
    if (*file_size >= 4)
    {
        if (memcmp(buffer, "\016\003\023\001", 4) == 0 ||       /* HDF4 */
            memcmp(buffer, "\000\000\377\377", 4) == 0 ||       /* (NASA) CDF */
            memcmp(buffer, "CDF\001", 4) == 0)  /* netCDF */
        {
            *format = coda_format_hdf4;
            return 0;
        }
    }

    /* HDF5 */
    if (*file_size >= 8)
    {
        if (memcmp(buffer, "\211HDF\r\n\032\n", 8) == 0)
        {
            *format = coda_format_hdf5;
            return 0;
        }
    }

    /* XML */
    if (*file_size >= 8)
    {
        /* we do not support UTF-16, but otherwise the following compares should be used:
         * UTF-16 BE no BOM   : memcmp(buffer, "\000<\000?\000x\000m\000l", 10)
         * UTF-16 LE no BOM   : memcmp(buffer, "<\000?\000x\000m\000l\000", 10)
         * UTF-16 BE with BOM : memcmp(buffer, "\376\377\000<\000?\000x\000m\000l", 12) == 0
         * UTF-16 LE with BOM : memcmp(buffer, "\377\376<\000?\000x\000m\000l\000", 12) == 0)
         * also increase the file_size check to '>= 12'
         */
        if (memcmp(buffer, "<?xml", 5) == 0 ||  /* UTF-8 no BOM */
            memcmp(buffer, "\357\273\277<?xml", 8) == 0)        /* UTF-8 with BOM */
        {
            *format = coda_format_xml;
            return 0;
        }
    }

    *format = coda_format_binary;
    return 0;
}


/** Determine the file size, format, product class, product type, and format version of a product file.
 * This function will perform an open and close on the product file and will try to automatically recognize
 * the product class, type, and version of the product file.
 * If the file is an HDF4 or HDF5 file the \a file_format will be set, but \a product_class and \a product_type will
 * be set to NULL and \a version will be set to -1.
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
 * If the product could not be recognized and is not a self describing product an error will be returned.
 * \param filename Relative or full path to the product file.
 * \param file_size Pointer to the variable where the actual file size in bytes will be stored.
 * \param file_format Pointer to the variable where the file format value will be stored.
 * \param product_class Pointer to the variable where the product class string will be stored.
 * \param product_type Pointer to the variable where the product type string will be stored.
 * \param version Pointer to the variable where the product format version number will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occured (check #coda_errno).
 */
LIBCODA_API int coda_recognize_file(const char *filename, int64_t *file_size, coda_format *file_format,
                                    const char **product_class, const char **product_type, int *version)
{
    coda_ProductDefinition *definition = NULL;
    coda_format format;
    int64_t size;

    if (filename == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "filename argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_format_and_size(filename, &format, &size) != 0)
    {
        return -1;
    }
    switch (format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            if (coda_ascbin_recognize_file(filename, size, &definition) != 0)
            {
                return -1;
            }
            break;
        case coda_format_xml:
            if (coda_xml_recognize_file(filename, size, &definition) != 0)
            {
                return -1;
            }
            break;
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    if (file_size != NULL)
    {
        *file_size = size;
    }
    if (file_format != NULL)
    {
        *file_format = (definition != NULL ? definition->format : format);
    }
    if (product_class != NULL)
    {
        *product_class = (definition != NULL ? definition->product_type->product_class->name : NULL);
    }
    if (product_type != NULL)
    {
        *product_type = (definition != NULL ? definition->product_type->name : NULL);
    }
    if (version != NULL)
    {
        *version = (definition != NULL ? definition->version : -1);
    }

    return 0;
}

/** Open a product file for reading.
 * This function will try to open the specified file for reading. On success a newly allocated file handle will be
 * returned. The memory for this file handle will be released when coda_close() is called for this handle.
 * \param filename Relative or full path to the product file.
 * \param pf Pointer to the variable where the pointer to the product file handle will be storeed.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_open(const char *filename, coda_ProductFile **pf)
{
    coda_ProductFile *product_file;
    coda_format format;
    int64_t file_size;

    if (filename == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "filename argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "pf argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (get_format_and_size(filename, &format, &file_size) != 0)
    {
        return -1;
    }

    switch (format)
    {
        case coda_format_ascii:
            if (coda_ascii_open(filename, file_size, &product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_binary:
            if (coda_bin_open(filename, file_size, &product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_xml:
            if (coda_xml_open(filename, file_size, &product_file) != 0)
            {
                return -1;
            }
            break;
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            if (coda_hdf4_open(filename, file_size, &product_file) != 0)
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
            if (coda_hdf5_open(filename, file_size, &product_file) != 0)
            {
                return -1;
            }
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    /* initialize product variables */
    if (product_file->product_definition != NULL && product_file->product_definition->num_product_variables > 0)
    {
        int num_product_variables;
        int i;

        num_product_variables = product_file->product_definition->num_product_variables;
        product_file->product_variable_size = malloc(num_product_variables * sizeof(long *));
        if (product_file->product_variable_size == NULL)
        {
            coda_close(product_file);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           num_product_variables * sizeof(long *), __FILE__, __LINE__);
            return -1;
        }
        product_file->product_variable = malloc(num_product_variables * sizeof(int64_t **));
        if (product_file->product_variable == NULL)
        {
            coda_close(product_file);
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           num_product_variables * sizeof(int64_t **), __FILE__, __LINE__);
            return -1;
        }

        for (i = 0; i < num_product_variables; i++)
        {
            product_file->product_variable[i] = NULL;
        }
    }

    *pf = product_file;

    return 0;
}

/** Close an open product file.
 * This function will close the file associated with the file handle and release the memory for the handle.
 * The file handle will be released even if unmapping or closing of the product file produced an error.
 * \param pf Pointer to a product file handle. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_close(coda_ProductFile *pf)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* remove product variable information */
    if (pf->product_variable_size != NULL)
    {
        free(pf->product_variable_size);
        pf->product_variable_size = NULL;
    }
    if (pf->product_variable != NULL)
    {
        int i;

        for (i = 0; i < pf->product_definition->num_product_variables; i++)
        {
            if (pf->product_variable[i] != NULL)
            {
                free(pf->product_variable[i]);
            }
        }
        free(pf->product_variable);
        pf->product_variable = NULL;
    }

    switch (pf->format)
    {
        case coda_format_ascii:
            return coda_ascii_close(pf);
        case coda_format_binary:
            return coda_bin_close(pf);
        case coda_format_xml:
            return coda_xml_close(pf);
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_close(pf);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_close(pf);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
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
 * \param pf Pointer to a product file handle. 
 * \param filename Pointer to the variable where the filename of the product will be stored. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_filename(const coda_ProductFile *pf, const char **filename)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *filename = pf->filename;

    return 0;
}

/** Get the actual file size of a product file.
 * \param pf Pointer to a product file handle. 
 * \param file_size Pointer to the variable where the actual file size (in bytes) of the product is stored. 
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_file_size(const coda_ProductFile *pf, int64_t *file_size)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (file_size == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "file_size argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *file_size = pf->file_size;

    return 0;
}

/** Get the basic file format of the product.
 * Possible formats are ascii, binary, xml, hdf4, and hdf5.
 * Mind that inside a product different typed data can exist. For instance, both xml and binary products can have
 * part of their content be ascii typed data.
 * \param pf Pointer to a product file handle. 
 * \param format Pointer to the variable where the format will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_format(const coda_ProductFile *pf, coda_format *format)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *format = pf->format;

    return 0;
}

/** Get the product class of a product file.
 * This function will return the name of the product class of a product.
 * The name of the product class will be stored in the \a product_class parameter and will be 0 terminated.
 * The string pointer that is returned for \a product_class does not have to be freed by the user and will remain valid
 * until coda_done() is called.
 * \param pf Pointer to a product file handle. 
 * \param product_class Pointer to the variable where the class name of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_class(const coda_ProductFile *pf, const char **product_class)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_class == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product_class argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (pf->product_definition != NULL)
    {
        *product_class = pf->product_definition->product_type->product_class->name;
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
 * \param pf Pointer to a product file handle. 
 * \param product_type Pointer to the variable where the product type name of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_type(const coda_ProductFile *pf, const char **product_type)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (pf->product_definition != NULL)
    {
        *product_type = pf->product_definition->product_type->name;
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
 * \param pf Pointer to a product file handle. 
 * \param version Pointer to the variable where the product version of the product type of the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_version(const coda_ProductFile *pf, int *version)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (version == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "version argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (pf->product_definition != NULL)
    {
        *version = pf->product_definition->version;
    }
    else
    {
        *version = -1;
    }

    return 0;
}

/** Get the CODA type of the root of the product.
 * \param pf Pointer to a product file handle. 
 * \param type Pointer to the variable where the Type handle will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_root_type(const coda_ProductFile *pf, coda_Type **type)
{
    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    return coda_get_type_for_dynamic_type(pf->root_type, type);
}

/** Get the value for a product variable.
 * CODA supports a mechanism called product variables to store frequently needed information of a product (i.e.
 * information that is needed to calculate byte offsets or array sizes within a product). With this function you
 * can retrieve the values for those product variables (consult the CODA Product Definition Documentation for an
 * overview of product variables for a certain product type).
 * Product variables can be one dimensional arrays, in which case you will have to pass an array index using the
 * \a index parameter. If the product variable is a scalar you should pass 0 for \a index.
 * The value of a product variable is always a 64-bit integer and will be stored in \a value.
 * \param pf Pointer to a product file handle. 
 * \param variable The name of the product variable.
 * \param index The array index of the product variable (pass 0 if the variable is a scalar).
 * \param value Pointer to the variable where the product variable value will be stored.
 * \return
 *   \arg \c 0, Success
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_get_product_variable_value(coda_ProductFile *pf, const char *variable, long index, int64_t *value)
{
    int64_t *variable_ptr;
    long size;

    if (pf == NULL)
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
    if (coda_product_variable_get_size(pf, variable, &size) != 0)
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

    if (coda_product_variable_get_pointer(pf, variable, index, &variable_ptr) != 0)
    {
        return -1;
    }

    *value = *variable_ptr;

    return 0;
}

/** @} */
