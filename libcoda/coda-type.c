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

#include "coda-internal.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-bin.h"
#include "coda-xml.h"
#include "coda-netcdf.h"
#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif

/** \file */

/** \defgroup coda_types CODA Types
 * Each data element or group of data elements (such as an array or record) in a product file, independent of whether
 * it is an ascii, binary, XML, netCDF, HDF4, or HDF5 product, has a unique description in CODA.
 * Each of those descriptions is refered to as a CODA type (which is of type #coda_Type).
 * For ascii, binary, and XML products the type definition is fixed and is provided by .codadef files.
 * For netCDF, HDF4, and HDF5 files the type definition is taken from the products themselves.
 * /note For XML files CODA also allows taking the definition from the file itself, but CODA will then not know how to
 * interpret the 'leaf elements' (i.e. whether the content of an XML element should be a string, an integer, a time
 * value, etc.) and will treat all 'leaf elements' as ascii text.
 * 
 *
 * As an example, there is a type that describes the MPH of an ENVISAT product (which is a record).
 * This record contains a name, a textual description, the number of fields, and for each of the fields the field name
 * and (again) a CODA type describing that field.
 *
 * CODA types are grouped into several classes (#coda_type_class). The available classes are:
 *  - record (#coda_record_class)
 *  - array (#coda_array_class)
 *  - integer (#coda_integer_class)
 *  - real (#coda_real_class)
 *  - text (#coda_text_class)
 *  - raw (#coda_raw_class)
 *  - special (#coda_special_class)
 *
 * The record and array types are the compound types that structurally define the product. It is possible to have
 * records which fields are again records or arrays and arrays may have again arrays or records as elements.
 * At the deepest level of a product tree (i.e. the 'leaf elements') you will allways find a basic type. These basic
 * types are represented by the classes integer, real, text, and raw for respectively integer numbers, floating point
 * numbers, text strings, and series of uninterpreted bytes.
 *
 * For each of the basic type classes you can use the coda_type_get_read_type() function to determine the best native
 * type (#coda_native_type) in which to store the data as it is read from file into memory. The native types contain
 * signed and unsigned integers ranging from 8 to 64 bits, the float and double types, char and string for textual
 * information, and a special bytes type for raw data.
 *
 * Finally, CODA also supports several special data types (#coda_special_class, #coda_special_type). These are types
 * that provide a mapping from the data in a product to a more convenient type for you as user. For example, there
 * is a special time type that converts the many time formats that are used in products to a double value representing
 * the amount of seconds since 2000-01-01T00:00:00.000000, which is the default time format in CODA.
 * When you encounter a special type you can always use the coda_type_get_special_base_type() function to bypass the
 * special interpretation of the data and look at the data in its actual form (e.g. for an ASCII time string you will
 * get a #coda_text_class type).
 *
 * CODA is able to deal with many dynamic properties that can be encountered in product files.
 * Some of these dynamic properties are: the size of arrays, the availabillity of record fields, the bit/byte offset of
 * record fields, and the size of string data or raw data.
 * For data types where these properties are dynamic, you will only be able to retrieve the actual
 * size/availabillity/etc. by moving a cursor to the data element and use the CODA Cursor functions to retrieve the
 * requested property (e.g. if the size of an array is not fixed, coda_type_get_array_dim() will return a
 * dimension value of -1 and coda_cursor_get_array_dim() will return the real dimension value).
 *
 * CODA also provides mappings of the datatypes that are available in XML, netCDF, HDF4, and HDF5 products to the CODA
 * types.
 * When accessing HDF products via CODA you will therefore get the 'CODA view' on these files.
 * For XML the following mapping is used:
 *  - the root of the product maps to a record with a single field (representing the root xml element)
 *  - an XML element will map to a record if it contains other XML elements
 *  - if an XML element can occur more than once within its parent element, CODA will map this to an array of that
 *    element
 *  - if an element contains ascii content then the content will be described using a CODA type for ascii formatted data
 *  - XML attributes for an element will be described using a record with a field for each attribute
 *
 * For netCDF the following mapping is used:
 *  - the root of the product maps to a record with a field for each variable in the product
 *  - a variable maps to a scalar for 0 dimensional data or to an array of basic types for 1 or higher dimensional data
 *  - global attributes are provides as attributes of the root record
 *  - variable attributes are provided as attributes of the basic type (i.e. of the scalar or the array element)
 *  - for arrays of characters, the last dimension is mapped to a string (if it is not the appendable dimension)
 *
 * For HDF4 the following mapping is used:
 *  - the root of product maps to a record
 *  - a Vgroup maps to a record
 *  - a Vdata maps to a record
 *  - a Vdata field maps to a one dimensional array of a basic type
 *  - an SDS maps to an n-dimensional array of a basic type
 *  - a GRImage maps to a two dimensional array of a basic type (the dimensions are swapped in order to get to a c
 *    array ordering)
 *  - for each HDF4 element the attributes and annotations that belong to this element are grouped together into a
 *    single attribute record. A single attribute can map to a basic type or to an array of a basic type.
 *
 * For HDF5 the following mapping is used:
 *  - a group maps to a record
 *  - an dataset maps to an n-dimensional array of a basic type (only simple dataspaces are supported)
 *  - for each HDF5 element the attributes that belong to this element are grouped together into a single attribute
 *    record
 *  - HDF5 softlinks and datatype objects are not supported by CODA
 *  - CODA only supports datasets and attributes that contain integer, float and string HDF5 data types and these data
 *    types map to their respective basic type in CODA.
 *
 * CODA doesn't use the #coda_special_class types for netCDF, HDF4, and HDF5 products.
 *
 * More information about the CODA types and descriptions of the supported products can be found in the CODA Product
 * Format Definitions documentation that is included with the CODA package.
 */

/** \typedef coda_Type
 * CODA Type handle
 * \ingroup coda_types
 */

/** \enum coda_type_class_enum
 * CODA Type classes
 * \ingroup coda_types
 */

/** \typedef coda_native_type
 * Machine specific data types
 * \ingroup coda_types
 */

/** \enum coda_native_type_enum
 * Machine specific data types
 * \ingroup coda_types
 */

/** \typedef coda_type_class
 * CODA Type classes
 * \ingroup coda_types
 */

/** \enum coda_special_type_enum
 * The special data types 
 * \ingroup coda_types
 */

/** \typedef coda_special_type
 * The special data types 
 * \ingroup coda_types
 */

/** \addtogroup coda_types
 * @{
 */

/** Returns the name of a storage format.
 * \param format CODA storage format
 * \return if the format is known a string containing the name of the format, otherwise the string "unknown".
 */
LIBCODA_API const char *coda_type_get_format_name(coda_format format)
{
    switch (format)
    {
        case coda_format_ascii:
            return "ascii";
        case coda_format_binary:
            return "binary";
        case coda_format_xml:
            return "xml";
        case coda_format_netcdf:
            return "netcdf";
        case coda_format_cdf:
            return "cdf";
        case coda_format_hdf4:
            return "hdf4";
        case coda_format_hdf5:
            return "hdf5";
    }

    return "unknown";
}

/** Returns the name of a type class.
 * In case the type class is not recognised the string "unknown" is returned.
 * \param type_class CODA type class
 * \return if the type class is known a string containing the name of the class, otherwise the string "unknown".
 */
LIBCODA_API const char *coda_type_get_class_name(coda_type_class type_class)
{
    switch (type_class)
    {
        case coda_record_class:
            return "record";
        case coda_array_class:
            return "array";
        case coda_integer_class:
            return "integer";
        case coda_real_class:
            return "real";
        case coda_text_class:
            return "text";
        case coda_raw_class:
            return "raw";
        case coda_special_class:
            return "special";
    }

    return "unknown";
}

/** Returns the name of a native type.
 * In case the native type is not recognised the string "unknown" is returned.
 * \note Mind that there is also a special native type #coda_native_type_not_available which will result in the string
 * 'N/A'.
 * \param native_type CODA native type
 * \return if the native type is known a string containing the name of the native type, otherwise the string "unknown".
 */
LIBCODA_API const char *coda_type_get_native_type_name(coda_native_type native_type)
{
    switch (native_type)
    {
        case coda_native_type_not_available:
            return "N/A";
        case coda_native_type_int8:
            return "int8";
        case coda_native_type_uint8:
            return "uint8";
        case coda_native_type_int16:
            return "int16";
        case coda_native_type_uint16:
            return "uint16";
        case coda_native_type_int32:
            return "int32";
        case coda_native_type_uint32:
            return "uint32";
        case coda_native_type_int64:
            return "int64";
        case coda_native_type_uint64:
            return "uint64";
        case coda_native_type_float:
            return "float";
        case coda_native_type_double:
            return "double";
        case coda_native_type_char:
            return "char";
        case coda_native_type_string:
            return "string";
        case coda_native_type_bytes:
            return "bytes";
    }

    return "unknown";
}

/** Returns the name of a special type.
 * In case the special type is not recognised the string "unknown" is returned.
 * \param special_type CODA special type
 * \return if the special type is known a string containing the name of the special type, otherwise the string "unknown".
 */
LIBCODA_API const char *coda_type_get_special_type_name(coda_special_type special_type)
{
    switch (special_type)
    {
        case coda_special_no_data:
            return "no_data";
        case coda_special_vsf_integer:
            return "vsf_integer";
        case coda_special_time:
            return "time";
        case coda_special_complex:
            return "complex";
    }

    return "unknown";
}

/** Determine wether data of this type is stored as ascii data.
 * You can use this function to determine whether the data is stored in ascii format. If it is in ascii format, you will
 * be able to read the data using coda_cursor_read_string().
 * If, for instance, a record consists of purely ascii data (i.e. it is a structured ascii block in the file)
 * \a has_ascii_content for a cursor pointing to that record will be 1 and you can use the coda_cursor_read_string()
 * function to read the whole record as a block of raw ascii.
 * \param type CODA type.
 * \param has_ascii_content Pointer to a variable where the ascii content status will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_has_ascii_content(const coda_Type *type, int *has_ascii_content)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (has_ascii_content == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "has_ascii_content argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            *has_ascii_content = 1;
            break;
        case coda_format_binary:
            *has_ascii_content = 0;
            break;
        case coda_format_xml:
            return coda_xml_type_has_ascii_content(type, has_ascii_content);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *has_ascii_content = (type->type_class == coda_text_class);
            break;
    }

    return 0;
}

/** Get the storage format of a type.
 * \param type CODA type.
 * \param format Pointer to a variable where the format will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_format(const coda_Type *type, coda_format *format)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *format = type->format;
    return 0;
}

/** Get the class of a type.
 * \param type CODA type.
 * \param type_class Pointer to a variable where the type class will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_class(const coda_Type *type, coda_type_class *type_class)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (type_class == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type_class argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *type_class = type->type_class;
    return 0;
}

/** Get the best native type for reading data of a CODA type.
 * The native type that is returned indicates which storage type can best be used when reading data of this
 * CODA type to memory. Compound types (arrays and records) that can be read directly (using a raw byte
 * array) will return a read type #coda_native_type_bytes. If a type can not be read directly (e.g. compound types in
 * XML, netCDF, HDF4, and HDF5 products) the special native type value #coda_native_type_not_available will be returned.
 * \note Be aware that types of class #coda_integer_class can return a native type #coda_native_type_double if the
 * integer type has a conversion associated with it and conversions are enabled.
 * \see coda_set_option_perform_conversions()
 * \param type CODA type.
 * \param read_type Pointer to a variable where the native type for reading will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_read_type(const coda_Type *type, coda_native_type *read_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (read_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "read_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_read_type(type, read_type);
        case coda_format_binary:
            return coda_bin_type_get_read_type(type, read_type);
        case coda_format_xml:
            return coda_xml_type_get_read_type(type, read_type);
        case coda_format_netcdf:
            return coda_netcdf_type_get_read_type(type, read_type);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_read_type(type, read_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_read_type(type, read_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the length in bytes of a string data type.
 * If the type does not refer to ascii data the function will return an error.
 * If the size is not fixed and can only be determined from information inside a product then \a length will be set
 * to -1.
 * \param type CODA type.
 * \param length Pointer to a variable where the string length (not including terminating 0) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_string_length(const coda_Type *type, long *length)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_text_class)
    {
        int has_ascii_content;

        if (coda_type_has_ascii_content(type, &has_ascii_content) != 0)
        {
            return -1;
        }
        if (!has_ascii_content)
        {
            coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to text (current type is %s)",
                           coda_type_get_class_name(type->type_class));
            return -1;
        }
    }
    if (length == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_string_length(type, length);
        case coda_format_binary:
            assert(0);
            exit(1);
        case coda_format_xml:
            return coda_xml_type_get_string_length(type, length);
        case coda_format_netcdf:
            return coda_netcdf_type_get_string_length(type, length);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_string_length(type, length);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_string_length(type, length);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the bit size for the data type.
 * Depending on the type of data and its format this function will return the following:
 * For data in ascii or binary format all data types will return the amount of bits the data occupies in the product
 * file. This means that e.g. ascii floats and ascii integers will return 8 times the byte size of the ascii
 * representation, records and arrays return the sum of the bit sizes of their fields/array-elements.
 * For XML data you will be able to retrieve bit sizes for all data except arrays and attribute records.
 * You will not be able to retrieve bit/byte sizes for data in netCDF, HDF4, or HDF5 format.
 * If the size is not fixed and can only be determined from information inside a product then \a bit_size will be set
 * to -1.
 * \param type CODA type.
 * \param bit_size Pointer to a variable where the bit size will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_bit_size(const coda_Type *type, int64_t *bit_size)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (bit_size == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_size argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_bit_size(type, bit_size);
        case coda_format_binary:
            return coda_bin_type_get_bit_size(type, bit_size);
        case coda_format_xml:
            return coda_xml_type_get_bit_size(type, bit_size);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *bit_size = -1;
            break;
    }

    return 0;
}

/** Get the name of a type.
 * A type can have an optional name that uniquely defines it within a product class. No two types within the same
 * product class may have the same name. If a type has a name, only a single instance of the definition will be used for
 * all places where the type is used (i.e. a single coda_Type object will be used for all cases where this type is 
 * used).
 * If the type is unnamed a NULL pointer will be returned.
 * The \a name parameter will either be a NULL pointer or a 0 terminated string.
 * \param type CODA type.
 * \param name Pointer to the variable where the name of the type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_name(const coda_Type *type, const char **name)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *name = type->name;

    return 0;
}

/** Get the description of a type.
 * If the type does not have a description a NULL pointer will be returned.
 * The \a description parameter will either be a NULL pointer or a 0 terminated string.
 * \param type CODA type.
 * \param description Pointer to the variable where the description of the type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_description(const coda_Type *type, const char **description)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (description == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "description argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *description = type->description;

    return 0;
}

/** Get the unit of a type.
 * You will only receive unit information for ascii, binary, and xml data (for other formats a NULL pointer will be
 * returned).
 * The unit information is a string with the same text as can be found in the unit column of the CODA Product Format
 * Definition documentation for this type.
 * If you try to retrieve the unit for an array type then the unit of its base type will be returned.
 * The \a unit parameter will either be a NULL pointer or a 0 terminated string.
 * \param type CODA type.
 * \param unit Pointer to the variable where the unit information of the type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_unit(const coda_Type *type, const char **unit)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (unit == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "unit argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (type->type_class == coda_array_class)
    {
        coda_Type *base_type;

        if (coda_type_get_array_base_type(type, &base_type) != 0)
        {
            return -1;
        }
        return coda_type_get_unit(base_type, unit);
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_unit(type, unit);
        case coda_format_binary:
            return coda_bin_type_get_unit(type, unit);
        case coda_format_xml:
            return coda_xml_type_get_unit(type, unit);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *unit = NULL;
            break;
    }

    return 0;
}

/** Get the associated fixed value string of a type if it has one.
 * Fixed values will only occur for #coda_text_class and #coda_raw_class types and only for ascii, binary, or xml
 * formatted data (in all other cases a NULL pointer will be returned).
 * It is possible to pass a NULL pointer for the length parameter to omit the retrieval of the length.
 * If the type does not have a fixed value a NULL pointer will be returned and the \a length parameter (if it is not a
 * NULL pointer) will be set to 0.
 * For ascii and xml data the \a fixed_value will be a 0 terminated string. For binary data there will not be a 0
 * termination character. Since fixed values for raw data can contain \\0 values you should use the returned \a length
 * parameter to determine the size of the fixed value.
 * The \a length parameter will contain the length of the fixed value without taking a terminating '\\0' into account.
 * \param type CODA type.
 * \param fixed_value Pointer to the variable where the pointer to the fixed value for the type will be stored.
 * \param length Pointer to the variable where the string length of the fixed value will be stored (can be NULL).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_fixed_value(const coda_Type *type, const char **fixed_value, long *length)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (fixed_value == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "fixed_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_fixed_value(type, fixed_value, length);
        case coda_format_binary:
            return coda_bin_type_get_fixed_value(type, fixed_value, length);
        case coda_format_xml:
            return coda_xml_type_get_fixed_value(type, fixed_value, length);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *fixed_value = NULL;
            break;
    }

    return 0;
}

/** Get the number of fields of a record type.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param num_fields Pointer to a variable where the number of fields will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_num_record_fields(const coda_Type *type, long *num_fields)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (num_fields == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "num_fields argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_num_record_fields(type, num_fields);
        case coda_format_xml:
            return coda_xml_type_get_num_record_fields(type, num_fields);
        case coda_format_netcdf:
            return coda_netcdf_type_get_num_record_fields(type, num_fields);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_num_record_fields(type, num_fields);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_num_record_fields(type, num_fields);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the field index from a field name for a record type.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param name Name of the record field.
 * \param index Pointer to a variable where the field index will be stored (0 <= \a index < number of fields).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_index_from_name(const coda_Type *type, const char *name, long *index)
{
    int result = 0;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (index == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "index argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            result = coda_ascbin_type_get_record_field_index_from_name(type, name, index);
            break;
        case coda_format_xml:
            result = coda_xml_type_get_record_field_index_from_name(type, name, index);
            break;
        case coda_format_netcdf:
            result = coda_netcdf_type_get_record_field_index_from_name(type, name, index);
            break;
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            result = coda_hdf4_type_get_record_field_index_from_name(type, name, index);
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            result = coda_hdf5_type_get_record_field_index_from_name(type, name, index);
            break;
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    if (result != 0)
    {
        if (coda_errno == CODA_ERROR_INVALID_NAME)
        {
            coda_set_error(CODA_ERROR_INVALID_NAME, "record does not contain a field named '%s'", name);
        }
        return -1;
    }

    return 0;
}

/** Get the CODA type for a record field.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param index Field index (0 <= \a index < number of fields).
 * \param field_type Pointer to the variable where the type of the record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_type(const coda_Type *type, long index, coda_Type **field_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (field_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_record_field_type(type, index, field_type);
        case coda_format_xml:
            return coda_xml_type_get_record_field_type(type, index, field_type);
        case coda_format_netcdf:
            return coda_netcdf_type_get_record_field_type(type, index, field_type);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_record_field_type(type, index, field_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_record_field_type(type, index, field_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the name of a record field.
 * If the type is not a record class the function will return an error.
 * The \a name parameter will be 0 terminated.
 * \param type CODA type.
 * \param index Field index (0 <= \a index < number of fields).
 * \param name Pointer to the variable where the name of the record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_name(const coda_Type *type, long index, const char **name)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_record_field_name(type, index, name);
        case coda_format_xml:
            return coda_xml_type_get_record_field_name(type, index, name);
        case coda_format_netcdf:
            return coda_netcdf_type_get_record_field_name(type, index, name);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_record_field_name(type, index, name);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_record_field_name(type, index, name);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the hidden status of a record field.
 * If the type is not a record class the function will return an error.
 * The hidden property is only applicable for ascii, binary, and xml data (fields can not be hidden for other formats).
 * If the record field has the hidden property \a hidden will be set to 1, otherwise it will be set to 0.
 * \note The C API of CODA does not hide record fields itself. This property is used by interfaces on top of the CODA C
 * interface (such as the MATLAB and IDL interfaces) to eliminate hidden fields when retrieving complete records.
 * \param type CODA type.
 * \param index Field index (0 <= \a index < number of fields).
 * \param hidden Pointer to the variable where the hidden status of the record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_hidden_status(const coda_Type *type, long index, int *hidden)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (hidden == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "hidden argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_record_field_hidden_status(type, index, hidden);
        case coda_format_xml:
            return coda_xml_type_get_record_field_hidden_status(type, index, hidden);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *hidden = 0;
            break;
    }

    return 0;
}

/** Get the available status of a record field.
 * If the type is not a record class the function will return an error.
 * The available status is only applicable for data in ascii, binary, or XML format (fields are always available for
 * netCDF, HDF4, and HDF5 data).
 * The available status is a dynamic property and can thus only really be determined using the function
 * coda_cursor_get_record_field_available_status(). The coda_type_get_record_field_hidden_status() function, however,
 * indicates whether the availability of a field is dynamic or not. If it is not dynamic (i.e. it is always available)
 * \a available will be 1, if not (i.e. it has to be determined dynamically) \a available will be -1.
 * \param type CODA type.
 * \param index Field index (0 <= \a index < number of fields).
 * \param available Pointer to the variable where the available status of the record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_available_status(const coda_Type *type, long index, int *available)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (available == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "available argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_record_field_available_status(type, index, available);
        case coda_format_xml:
            return coda_xml_type_get_record_field_available_status(type, index, available);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *available = 1;
            break;
    }

    return 0;
}

/** Get the union status of a record.
 * If the record is a union (i.e. all fields are dynamically available and only one field can be available at any time)
 * \a is_union will be set to 1, otherwise it will be set to 0.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param is_union Pointer to a variable where the number of fields will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_union_status(const coda_Type *type, int *is_union)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a record (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (is_union == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "is_union argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_record_union_status(type, is_union);
        case coda_format_xml:
            return coda_xml_type_get_record_union_status(type, is_union);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *is_union = 0;
            break;
    }

    return 0;
}

/** Get the number of dimensions for an array.
 * If the type is not an array class the function will return an error.
 * \param type CODA type.
 * \param num_dims Pointer to the variable where the number of dimensions will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_array_num_dims(const coda_Type *type, int *num_dims)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to an array (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (num_dims == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "num_dims argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_array_num_dims(type, num_dims);
        case coda_format_xml:
            return coda_xml_type_get_array_num_dims(type, num_dims);
        case coda_format_netcdf:
            return coda_netcdf_type_get_array_num_dims(type, num_dims);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_array_num_dims(type, num_dims);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_array_num_dims(type, num_dims);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve the dimensions with a constant value for an array.
 * The function returns both the number of dimensions \a num_dims and the size for each of the dimensions \a dim that
 * have a constant/fixed size.
 * \note If the size of a dimension is variable (it differs per product or differs per occurence inside one product)
 * then this function will set the value for that dimension to \c -1. Otherwise it will set the dimension
 * entry in \a dim to the constant value for that dimension as defined by the CODA product format definition.
 * Variable dimension sizes can only occur when a CODA product format definition is used.
 * If the type is not an array class the function will return an error.
 * \param type CODA type.
 * \param num_dims Pointer to the variable where the number of dimensions will be stored.
 * \param dim Pointer to the variable where the dimensions will be stored. Dimensions that will vary per product or
 * within a product will have value \c -1. The caller needs to make sure that the variable has enough room to store the
 * dimensions array. It is guaranteed that the number of dimensions will never exceed #CODA_MAX_NUM_DIMS.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_array_dim(const coda_Type *type, int *num_dims, long dim[])
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to an array (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (num_dims == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "num_dims argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dim == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dim argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_array_dim(type, num_dims, dim);
        case coda_format_xml:
            return coda_xml_type_get_array_dim(type, num_dims, dim);
        case coda_format_netcdf:
            return coda_netcdf_type_get_array_dim(type, num_dims, dim);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_array_dim(type, num_dims, dim);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_array_dim(type, num_dims, dim);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the CODA type for the elements of an array.
 * If the type is not an array class the function will return an error.
 * \param type CODA type.
 * \param base_type Pointer to the variable where the base type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_array_base_type(const coda_Type *type, coda_Type **base_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to an array (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (base_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "base_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_type_get_array_base_type(type, base_type);
        case coda_format_xml:
            return coda_xml_type_get_array_base_type(type, base_type);
        case coda_format_netcdf:
            return coda_netcdf_type_get_array_base_type(type, base_type);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_type_get_array_base_type(type, base_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_type_get_array_base_type(type, base_type);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the special type for a type.
 * This function will return the specific special type for types of class #coda_special_class.
 * If the type is not a special type the function will return an error.
 * \param type CODA type.
 * \param special_type Pointer to a variable where the special type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_special_type(const coda_Type *type, coda_special_type *special_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_special_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a special type (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (special_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "special_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_special_type(type, special_type);
        case coda_format_binary:
            return coda_bin_type_get_special_type(type, special_type);
        case coda_format_xml:
            return coda_xml_type_get_special_type(type, special_type);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    assert(0);
    exit(1);
}

/** Get the base type for a special type.
 * If the type is not a special type the function will return an error.
 * \param type CODA type.
 * \param base_type Pointer to the variable where the base type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_special_base_type(const coda_Type *type, coda_Type **base_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_special_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to a special type (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (base_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "base_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (type->format)
    {
        case coda_format_ascii:
            return coda_ascii_type_get_special_base_type(type, base_type);
        case coda_format_binary:
            return coda_bin_type_get_special_base_type(type, base_type);
        case coda_format_xml:
            return coda_xml_type_get_special_base_type(type, base_type);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    assert(0);
    exit(1);
}

/** @} */
