/*
 * Copyright (C) 2007-2020 S[&]T, The Netherlands.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda-type.h"
#include "coda-expr.h"

#include "coda-ascbin.h"
#include "coda-ascii.h"
#include "coda-bin.h"
#include "coda-xml.h"
#include "coda-netcdf.h"
#include "coda-grib.h"
#ifdef HAVE_HDF4
#include "coda-hdf4.h"
#endif
#ifdef HAVE_HDF5
#include "coda-hdf5.h"
#endif

/** \defgroup coda_types CODA Types
 * Each data element or group of data elements (such as an array or record) in a product file has a unique description,
 * in CODA. This description is independent of the file format of the product (e.g. ascii, binary, XML, netCDF, etc.)
 * Each of those descriptions is referred to as a CODA type (which is of type #coda_type).
 * For self describing formats such as netCDF, HDF4, and HDF5 files the type definition is taken from the products
 * themselves. For other formats, such as ascii and binary products the type definition is fixed and is provided by
 * .codadef files.
 * For some file formats CODA can use a predefined format stored in a .codadef file to further restricit the format
 * of a self describing file. For XML files, for instance, CODA will treat all 'leaf elements' as ascii text if no
 * definition for the product is available in a .codadef. However, with a definition, CODA will know how to interpret
 * the 'leaf elements' (i.e. whether the content of an XML element should be a string, an integer, a time value, etc.).
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
 * Some of these dynamic properties are: the size of arrays, the availabillity of optional record fields, the bit/byte
 * offset of record fields, and the size of string data or raw data.
 * For data types where these properties are dynamic, you will only be able to retrieve the actual
 * size/availabillity/etc. by moving a cursor to the data element and use the CODA Cursor functions to retrieve the
 * requested property (e.g. if the size of an array is not fixed, coda_type_get_array_dim() will return a
 * dimension value of -1 and coda_cursor_get_array_dim() will return the real dimension value).
 *
 * More information about the CODA types and descriptions of the mappings of self describing formats to CODA types
 * can be found in other parts of the CODA documentation that is included with the CODA package.
 */

/** \typedef coda_type
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

static THREAD_LOCAL coda_type_record *empty_record_singleton[] =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define num_empty_record_singletons ((int)(sizeof(empty_record_singleton)/sizeof(empty_record_singleton[0])))

static THREAD_LOCAL coda_type_raw *raw_file_singleton = NULL;

static THREAD_LOCAL coda_type_special *no_data_singleton[] =
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

#define num_no_data_singletons ((int)(sizeof(no_data_singleton)/sizeof(no_data_singleton[0])))

coda_conversion *coda_conversion_new(double numerator, double denominator, double add_offset, double invalid_value)
{
    coda_conversion *conversion;

    if (denominator == 0.0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "denominator may not be 0 for conversion");
        return NULL;
    }
    conversion = (coda_conversion *)malloc(sizeof(coda_conversion));
    if (conversion == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_conversion), __FILE__, __LINE__);
        return NULL;
    }
    conversion->numerator = numerator;
    conversion->denominator = denominator;
    conversion->add_offset = add_offset;
    conversion->invalid_value = invalid_value;
    conversion->unit = NULL;

    return conversion;
}

int coda_conversion_set_unit(coda_conversion *conversion, const char *unit)
{
    char *new_unit = NULL;

    if (conversion->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "conversion already has a unit");
        return -1;
    }
    if (unit != NULL)
    {
        new_unit = strdup(unit);
        if (new_unit == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    conversion->unit = new_unit;

    return 0;
}

void coda_conversion_delete(coda_conversion *conversion)
{
    if (conversion == NULL)
    {
        return;
    }
    if (conversion->unit != NULL)
    {
        free(conversion->unit);
    }
    free(conversion);
}

static void mapping_delete(coda_ascii_mapping *mapping)
{
    if (mapping == NULL)
    {
        return;
    }
    if (mapping->str != NULL)
    {
        free(mapping->str);
    }
    free(mapping);
}

static void mappings_delete(coda_ascii_mappings *mappings)
{
    if (mappings == NULL)
    {
        return;
    }
    if (mappings->mapping != NULL)
    {
        int i;

        for (i = 0; i < mappings->num_mappings; i++)
        {
            if (mappings->mapping[i] != NULL)
            {
                mapping_delete(mappings->mapping[i]);
            }
        }
        free(mappings->mapping);
    }
    free(mappings);
}

coda_ascii_integer_mapping *coda_ascii_integer_mapping_new(const char *str, int64_t value)
{
    coda_ascii_integer_mapping *mapping;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty string value (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (strlen(str) > MAX_ASCII_NUMBER_LENGTH)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "string too large (%ld) for ascii integer mapping",
                       (long)strlen(str));
        return NULL;
    }

    mapping = (coda_ascii_integer_mapping *)malloc(sizeof(coda_ascii_integer_mapping));
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascii_integer_mapping), __FILE__, __LINE__);
        return NULL;
    }
    mapping->length = 0;
    mapping->str = NULL;
    mapping->value = value;

    mapping->str = strdup(str);
    if (mapping->str == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        free(mapping);
        return NULL;
    }
    mapping->length = (int)strlen(str);

    return mapping;
}

void coda_ascii_integer_mapping_delete(coda_ascii_integer_mapping *mapping)
{
    mapping_delete((coda_ascii_mapping *)mapping);
}

coda_ascii_float_mapping *coda_ascii_float_mapping_new(const char *str, double value)
{
    coda_ascii_float_mapping *mapping;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty string value (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (strlen(str) > MAX_ASCII_NUMBER_LENGTH)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "string too large (%ld) for ascii float mapping", (long)strlen(str));
        return NULL;
    }

    mapping = (coda_ascii_float_mapping *)malloc(sizeof(coda_ascii_float_mapping));
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_ascii_float_mapping), __FILE__, __LINE__);
        return NULL;
    }
    mapping->length = 0;
    mapping->str = NULL;
    mapping->value = value;

    mapping->str = strdup(str);
    if (mapping->str == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        free(mapping);
        return NULL;
    }
    mapping->length = (int)strlen(str);

    return mapping;
}

void coda_ascii_float_mapping_delete(coda_ascii_float_mapping *mapping)
{
    mapping_delete((coda_ascii_mapping *)mapping);
}

static int mapping_type_add_mapping(coda_type *type, coda_ascii_mappings **mappings, coda_ascii_mapping *mapping)
{
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty mapping (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (mappings == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty mappings (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (*mappings == NULL)
    {
        *mappings = malloc(sizeof(coda_ascii_mappings));
        if (*mappings == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           sizeof(coda_ascii_mappings), __FILE__, __LINE__);
            return -1;
        }
        (*mappings)->default_bit_size = (type->bit_size >= 0 ? type->bit_size : -1);
        (*mappings)->num_mappings = 0;
        (*mappings)->mapping = NULL;
    }

    if ((*mappings)->num_mappings % BLOCK_SIZE == 0)
    {
        coda_ascii_mapping **new_mapping;

        new_mapping = realloc((*mappings)->mapping,
                              ((*mappings)->num_mappings + BLOCK_SIZE) * sizeof(coda_ascii_mapping *));
        if (new_mapping == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           ((*mappings)->num_mappings + BLOCK_SIZE) * sizeof(coda_ascii_mapping *), __FILE__, __LINE__);
            return -1;
        }
        (*mappings)->mapping = new_mapping;
    }
    (*mappings)->mapping[(*mappings)->num_mappings] = mapping;
    (*mappings)->num_mappings++;

    if (type->bit_size >= 0 && (*mappings)->default_bit_size >= 0 &&
        mapping->length != ((*mappings)->default_bit_size >> 3))
    {
        type->bit_size = -1;
    }

    return 0;
}

static int mapping_type_set_bit_size(coda_type *type, coda_ascii_mappings *mappings, int64_t bit_size)
{
    int i;

    assert(mappings != NULL);
    assert(bit_size >= 0);

    if (mappings->default_bit_size >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "ascii type already has a size");
        return -1;
    }
    mappings->default_bit_size = bit_size;
    type->bit_size = bit_size;
    for (i = 0; i < mappings->num_mappings; i++)
    {
        if (mappings->mapping[i]->length != (bit_size >> 3))
        {
            type->bit_size = -1;
            break;
        }
    }

    return 0;
}

void coda_type_record_field_delete(coda_type_record_field *field)
{
    if (field == NULL)
    {
        return;
    }
    if (field->name != NULL)
    {
        free(field->name);
    }
    if (field->real_name != NULL)
    {
        free(field->real_name);
    }
    if (field->type != NULL)
    {
        coda_type_release(field->type);
    }
    if (field->available_expr != NULL)
    {
        coda_expression_delete(field->available_expr);
    }
    if (field->bit_offset_expr != NULL)
    {
        coda_expression_delete(field->bit_offset_expr);
    }
    free(field);
}

static void record_delete(coda_type_record *type)
{
    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->hash_data != NULL)
    {
        hashtable_delete(type->hash_data);
    }
    if (type->real_name_hash_data != NULL)
    {
        hashtable_delete(type->real_name_hash_data);
    }
    if (type->num_fields > 0)
    {
        int i;

        for (i = 0; i < type->num_fields; i++)
        {
            coda_type_record_field_delete(type->field[i]);
        }
        free(type->field);
    }
    if (type->union_field_expr != NULL)
    {
        coda_expression_delete(type->union_field_expr);
    }
    free(type);
}

static void array_delete(coda_type_array *type)
{
    int i;

    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->base_type != NULL)
    {
        coda_type_release(type->base_type);
    }
    for (i = 0; i < type->num_dims; i++)
    {
        if (type->dim_expr[i] != NULL)
        {
            coda_expression_delete(type->dim_expr[i]);
        }
    }
    free(type);
}

static void number_delete(coda_type_number *type)
{
    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->unit != NULL)
    {
        free(type->unit);
    }
    if (type->conversion != NULL)
    {
        coda_conversion_delete(type->conversion);
    }
    if (type->mappings != NULL)
    {
        mappings_delete(type->mappings);
    }
    free(type);
}

static void text_delete(coda_type_text *type)
{
    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->fixed_value != NULL)
    {
        free(type->fixed_value);
    }
    free(type);
}

static void raw_delete(coda_type_raw *type)
{
    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->fixed_value != NULL)
    {
        free(type->fixed_value);
    }
    free(type);
}

static void special_delete(coda_type_special *type)
{
    if (type == NULL)
    {
        return;
    }
    if (type->name != NULL)
    {
        free(type->name);
    }
    if (type->description != NULL)
    {
        free(type->description);
    }
    if (type->size_expr != NULL)
    {
        coda_expression_delete(type->size_expr);
    }
    if (type->attributes != NULL)
    {
        coda_type_release((coda_type *)type->attributes);
    }
    if (type->base_type != NULL)
    {
        coda_type_release(type->base_type);
    }
    if (type->unit != NULL)
    {
        free(type->unit);
    }
    if (type->value_expr != NULL)
    {
        coda_expression_delete(type->value_expr);
    }
    free(type);
}

void coda_type_release(coda_type *type)
{
    if (type == NULL)
    {
        return;
    }

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (type->type_class)
    {
        case coda_record_class:
            record_delete((coda_type_record *)type);
            break;
        case coda_array_class:
            array_delete((coda_type_array *)type);
            break;
        case coda_integer_class:
        case coda_real_class:
            number_delete((coda_type_number *)type);
            break;
        case coda_text_class:
            text_delete((coda_type_text *)type);
            break;
        case coda_raw_class:
            raw_delete((coda_type_raw *)type);
            break;
        case coda_special_class:
            special_delete((coda_type_special *)type);
            break;
    }
}

int coda_type_set_read_type(coda_type *type, coda_native_type read_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    switch (type->type_class)
    {
        case coda_record_class:
        case coda_array_class:
        case coda_raw_class:
        case coda_special_class:
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "read type cannot be set explicitly for %s type",
                           coda_type_get_class_name(type->type_class));
            return -1;
        case coda_integer_class:
            if (read_type == coda_native_type_int8 || read_type == coda_native_type_uint8 ||
                read_type == coda_native_type_int16 || read_type == coda_native_type_uint16 ||
                read_type == coda_native_type_int32 || read_type == coda_native_type_uint32 ||
                read_type == coda_native_type_int64 || read_type == coda_native_type_uint64)
            {
                type->read_type = read_type;
                return 0;
            }
            break;
        case coda_real_class:
            if (read_type == coda_native_type_float || read_type == coda_native_type_double)
            {
                type->read_type = read_type;
                return 0;
            }
            break;
        case coda_text_class:
            if (read_type == coda_native_type_char || read_type == coda_native_type_string)
            {
                type->read_type = read_type;
                return 0;
            }
            break;
    }

    coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for %s type",
                   coda_type_get_native_type_name(read_type), coda_type_get_class_name(type->type_class));
    return -1;
}

int coda_type_set_name(coda_type *type, const char *name)
{
    char *new_name = NULL;

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
    if (type->name != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a name");
        return -1;
    }
    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "name '%s' is not a valid identifier", name);
        return -1;
    }
    new_name = strdup(name);
    if (new_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    type->name = new_name;

    return 0;
}

int coda_type_set_description(coda_type *type, const char *description)
{
    char *new_description = NULL;

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
    if (type->description != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a description");
        return -1;
    }
    if (description != NULL)
    {
        new_description = strdup(description);
        if (new_description == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    type->description = new_description;

    return 0;
}

int coda_type_set_bit_size(coda_type *type, int64_t bit_size)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->bit_size >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a bit size");
        return -1;
    }
    if (type->size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a bit size expression");
        return -1;
    }
    if (bit_size < 0)
    {
        char s[21];

        coda_str64(bit_size, s);
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size (%s) must be >= 0", s);
        return -1;
    }

    if (type->format == coda_format_ascii)
    {
        if ((bit_size & 0x7) != 0)
        {
            char s[21];

            coda_str64(bit_size, s);
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "bit size (%s) should be a rounded number of bytes for ascii type", s);
            return -1;
        }
    }
    switch (type->type_class)
    {

        case coda_integer_class:
        case coda_real_class:
            if (((coda_type_number *)type)->mappings != NULL)
            {
                return mapping_type_set_bit_size(type, ((coda_type_number *)type)->mappings, bit_size);
            }
            break;
        case coda_special_class:
        case coda_record_class:
        case coda_array_class:
        case coda_text_class:
        case coda_raw_class:
            break;
    }

    type->bit_size = bit_size;
    return 0;
}

int coda_type_set_byte_size(coda_type *type, int64_t byte_size)
{
    return coda_type_set_bit_size(type, 8 * byte_size);
}

int coda_type_set_bit_size_expression(coda_type *type, coda_expression *bit_size_expr)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_size_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_size_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a bit size expression");
        return -1;
    }
    if (type->type_class == coda_record_class || type->type_class == coda_array_class)
    {
        /* for compound types we also allow setting a bit size expression if the bit_size is currently 0 */
        if (type->bit_size > 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a bit size");
            return -1;
        }
    }
    else
    {
        if (type->bit_size >= 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a bit size");
            return -1;
        }
    }
    type->size_expr = bit_size_expr;
    type->bit_size = -1;
    return 0;
}

int coda_type_set_byte_size_expression(coda_type *type, coda_expression *byte_size_expr)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (byte_size_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "byte_size_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a byte size expression");
        return -1;
    }
    if (type->type_class == coda_record_class || type->type_class == coda_array_class)
    {
        /* for compound types we also allow setting a byte size expression if the bit_size is currently 0 */
        if (type->bit_size > 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a byte size");
            return -1;
        }
    }
    else
    {
        if (type->bit_size >= 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a byte size");
            return -1;
        }
    }
    type->size_expr = byte_size_expr;
    type->bit_size = -8;
    return 0;
}

int coda_type_add_attribute(coda_type *type, coda_type_record_field *attribute)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (attribute == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attribute argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->attributes == NULL)
    {
        type->attributes = coda_type_record_new(type->format);
        if (type->attributes == NULL)
        {
            return -1;
        }
    }
    return coda_type_record_add_field(type->attributes, attribute);
}

int coda_type_set_attributes(coda_type *type, coda_type_record *attributes)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (attributes == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attributes argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->attributes != NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attributes are already set (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    type->attributes = attributes;
    attributes->retain_count++;
    return 0;
}

coda_type_record_field *coda_type_record_field_new(const char *name)
{
    coda_type_record_field *field;

    if (name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (!coda_is_identifier(name))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field name '%s' is not a valid identifier for field definition",
                       name);
        return NULL;
    }

    field = (coda_type_record_field *)malloc(sizeof(coda_type_record_field));
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_record_field), __FILE__, __LINE__);
        return NULL;
    }
    field->name = NULL;
    field->real_name = NULL;
    field->type = NULL;
    field->hidden = 0;
    field->optional = 0;
    field->available_expr = NULL;
    field->bit_offset = -1;
    field->bit_offset_expr = NULL;

    field->name = strdup(name);
    if (field->name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        coda_type_record_field_delete(field);
        return NULL;
    }

    return field;
}

int coda_type_record_field_set_real_name(coda_type_record_field *field, const char *real_name)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (real_name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "real_name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->real_name != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a real name");
        return -1;
    }
    field->real_name = strdup(real_name);
    if (field->real_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_type_record_field_set_type(coda_type_record_field *field, coda_type *type)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a type");
        return -1;
    }
    field->type = type;
    type->retain_count++;
    return 0;
}

int coda_type_record_field_set_hidden(coda_type_record_field *field)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    field->hidden = 1;
    return 0;
}

int coda_type_record_field_set_optional(coda_type_record_field *field)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    field->optional = 1;
    return 0;
}

int coda_type_record_field_set_available_expression(coda_type_record_field *field, coda_expression *available_expr)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (available_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "available_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->available_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has an available expression");
        return -1;
    }
    field->available_expr = available_expr;
    field->optional = 1;
    return 0;
}

int coda_type_record_field_set_bit_offset_expression(coda_type_record_field *field, coda_expression *bit_offset_expr)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_offset_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_offset_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->bit_offset_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "field already has a bit offset expression");
        return -1;
    }
    if (field->type->format != coda_format_ascii && field->type->format != coda_format_binary)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit offset expression not allowed for record field with %s format",
                       coda_type_get_format_name(field->type->format));
        return -1;
    }
    field->bit_offset_expr = bit_offset_expr;
    return 0;
}

int coda_type_record_field_validate(const coda_type_record_field *field)
{
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing type for field definition");
        return -1;
    }
    return 0;
}

int coda_type_record_field_get_type(const coda_type_record_field *field, coda_type **type)
{
    *type = field->type;
    return 0;
}

coda_type_record *coda_type_record_new(coda_format format)
{
    coda_type_record *type;

    type = (coda_type_record *)malloc(sizeof(coda_type_record));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_record), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_record_class;
    type->read_type = coda_native_type_not_available;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->hash_data = NULL;
    type->real_name_hash_data = NULL;
    type->num_fields = 0;
    type->field = NULL;
    type->has_hidden_fields = 0;
    type->has_optional_fields = 0;
    type->is_union = 0;
    type->union_field_expr = NULL;

    if (format == coda_format_ascii || format == coda_format_binary)
    {
        type->read_type = coda_native_type_bytes;
        type->bit_size = 0;
    }

    type->hash_data = hashtable_new(0);
    if (type->hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        record_delete(type);
        return NULL;
    }

    type->real_name_hash_data = hashtable_new(1);
    if (type->real_name_hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not create hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        record_delete(type);
        return NULL;
    }

    return type;
}

coda_type_record *coda_type_union_new(coda_format format)
{
    coda_type_record *type;

    type = coda_type_record_new(format);
    if (type != NULL)
    {
        type->is_union = 1;
    }

    return type;
}

coda_type_record *coda_type_empty_record(coda_format format)
{
    assert(format < num_empty_record_singletons);
    if (empty_record_singleton[format] == NULL)
    {
        empty_record_singleton[format] = coda_type_record_new(format);
        assert(empty_record_singleton[format] != NULL);
    }

    return empty_record_singleton[format];
}

int coda_type_record_insert_field(coda_type_record *type, long index, coda_type_record_field *field)
{
    const char *real_name;
    long i;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field->type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type of field argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (type->is_union && !field->optional)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "fields added to a union need to be optional");
        return -1;

    }

    if (type->format != field->type->format)
    {
        /* we only allow switching from binary or xml to ascii */
        if (!(field->type->format == coda_format_ascii &&
              (type->format == coda_format_binary || type->format == coda_format_xml)))
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add field with %s format to record with %s format",
                           coda_type_get_format_name(field->type->format), coda_type_get_format_name(type->format));
            return -1;
        }
    }

    if (type->num_fields % BLOCK_SIZE == 0)
    {
        coda_type_record_field **new_field;

        new_field = realloc(type->field, (type->num_fields + BLOCK_SIZE) * sizeof(coda_type_record_field *));
        if (new_field == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->num_fields + BLOCK_SIZE) * sizeof(coda_type_record_field *), __FILE__, __LINE__);
            return -1;
        }
        type->field = new_field;
    }
    if (hashtable_insert_name(type->hash_data, index, field->name) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "duplicate field with name %s for record definition", field->name);
        return -1;
    }
    real_name = (field->real_name != NULL ? field->real_name : field->name);
    if (hashtable_get_index_from_name(type->real_name_hash_data, real_name) < 0)
    {
        /* only add the 'real_name' to the hash table if it was not there yet */
        hashtable_insert_name(type->real_name_hash_data, index, real_name);
    }
    if (index < type->num_fields)
    {
        for (i = type->num_fields; i > index; i--)
        {
            type->field[i] = type->field[i - 1];
        }
    }
    type->num_fields++;
    type->field[index] = field;

    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        if (type->is_union)
        {
            /* set bit_offset */
            if (field->bit_offset_expr != NULL)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit offset expression not allowed for union field");
                return -1;
            }
            field->bit_offset = 0;

            /* update bit_size */
            if (type->num_fields == 1)
            {
                type->bit_size = field->type->bit_size;
            }
            else if (type->bit_size != field->type->bit_size)
            {
                type->bit_size = -1;
            }
        }
        else
        {
            /* set bit_offset */
            if (field->bit_offset_expr == NULL)
            {
                if (index == 0)
                {
                    field->bit_offset = 0;
                }
                else if (type->field[index - 1]->bit_offset >= 0 && type->field[index - 1]->type->bit_size >= 0 &&
                         !type->field[index - 1]->optional)
                {
                    field->bit_offset = type->field[index - 1]->bit_offset + type->field[index - 1]->type->bit_size;
                }
            }
            for (i = index + 1; i < type->num_fields; i++)
            {
                if (type->field[i]->bit_offset_expr == NULL)
                {
                    if (type->field[i - 1]->bit_offset >= 0 && type->field[i - 1]->type->bit_size >= 0 &&
                        !type->field[i - 1]->optional)
                    {
                        type->field[i]->bit_offset =
                            type->field[i - 1]->bit_offset + type->field[i - 1]->type->bit_size;
                    }
                }
            }

            /* update bit_size */
            if (type->bit_size >= 0)
            {
                if (field->type->bit_size >= 0 && !type->field[type->num_fields - 1]->optional)
                {
                    type->bit_size += field->type->bit_size;
                }
                else
                {
                    type->bit_size = -1;
                }
            }
        }
    }

    return 0;
}

int coda_type_record_add_field(coda_type_record *type, coda_type_record_field *field)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    return coda_type_record_insert_field(type, type->num_fields, field);
}

int coda_type_record_create_field(coda_type_record *type, const char *real_name, coda_type *field_type)
{
    coda_type_record_field *field;
    char *field_name;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (real_name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "real_name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    field_name = coda_type_record_get_unique_field_name(type, real_name);
    if (field_name == NULL)
    {
        return -1;
    }
    field = coda_type_record_field_new(field_name);
    if (field == NULL)
    {
        free(field_name);
        return -1;
    }
    if (strcmp(field_name, real_name) != 0)
    {
        if (coda_type_record_field_set_real_name(field, real_name) != 0)
        {
            coda_type_record_field_delete(field);
            free(field_name);
            return -1;
        }
    }
    free(field_name);
    if (coda_type_record_field_set_type(field, field_type) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }
    if (coda_type_record_add_field(type, field) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }

    return 0;
}

int coda_type_union_set_field_expression(coda_type_record *type, coda_expression *field_expr)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (field_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "field_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (!type->is_union)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "record type is not a union");
        return -1;
    }
    if (type->union_field_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "record type already has a union field expression");
        return -1;
    }
    type->union_field_expr = field_expr;
    if (type->num_fields > 0)
    {
        long i;

        for (i = 0; i < type->num_fields; i++)
        {
            /* set bit_offset */
            if (type->field[i]->bit_offset_expr != NULL)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit offset expression not allowed for union field '%s'",
                               type->field[i]->name);
                return -1;
            }
            type->field[i]->bit_offset = 0;

            /* update bit_size */
            if (i == 0)
            {
                type->bit_size = type->field[i]->type->bit_size;
            }
            else if (type->bit_size != type->field[i]->type->bit_size)
            {
                type->bit_size = -1;
            }
        }
    }
    return 0;
}

int coda_type_record_validate(const coda_type_record *type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->is_union)
    {
        if (type->num_fields == 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "number of fields should be >= 1 for union type");
            return -1;
        }
        if (type->format == coda_format_ascii || type->format == coda_format_binary)
        {
            if (type->union_field_expr == NULL)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing union field expression");
                return -1;
            }
        }
    }
    return 0;
}

char *coda_type_record_get_unique_field_name(const coda_type_record *type, const char *name)
{
    if (type->format == coda_format_xml)
    {
        name = coda_element_name_from_xml_name(name);
    }
    return coda_identifier_from_name(name, type->hash_data);
}

coda_type_array *coda_type_array_new(coda_format format)
{
    coda_type_array *type;

    type = (coda_type_array *)malloc(sizeof(coda_type_array));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_array), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_array_class;
    if (format == coda_format_ascii || format == coda_format_binary)
    {
        type->read_type = coda_native_type_bytes;
    }
    else
    {
        type->read_type = coda_native_type_not_available;
    }
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->base_type = NULL;
    type->num_elements = 1;
    type->num_dims = 0;

    return type;
}

int coda_type_array_set_base_type(coda_type_array *type, coda_type *base_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "array already has a base type");
        return -1;
    }
    if (type->format != base_type->format)
    {
        /* we only allow switching from binary or xml to ascii */
        if (!(base_type->format == coda_format_ascii &&
              (type->format == coda_format_binary || type->format == coda_format_xml)))
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add element with %s format to array with %s format",
                           coda_type_get_format_name(base_type->format), coda_type_get_format_name(type->format));
            return -1;
        }
    }
    if (type->format == coda_format_xml)
    {
        /* we don't allow arrays of arrays */
        if (base_type->format == coda_format_xml && base_type->type_class == coda_array_class)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "Arrays of arrays are not allowed for xml format");
            return -1;
        }
    }
    type->base_type = base_type;
    base_type->retain_count++;

    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        /* update bit_size */
        if (type->num_elements >= 0 && base_type->bit_size >= 0)
        {
            type->bit_size = type->num_elements * base_type->bit_size;
        }
    }

    return 0;
}

int coda_type_array_add_fixed_dimension(coda_type_array *type, long dim)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dim < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid dimension size (%ld) for array type", dim);
        return -1;
    }
    if (type->num_dims == CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "maximum number of dimensions (%d) exceeded for array type",
                       CODA_MAX_NUM_DIMS);
        return -1;
    }
    type->dim[type->num_dims] = dim;
    type->dim_expr[type->num_dims] = NULL;
    type->num_dims++;

    /* update num_elements */
    if (type->num_elements != -1)
    {
        if (type->num_dims == 1)
        {
            type->num_elements = dim;
        }
        else
        {
            type->num_elements *= dim;
        }

        if (type->format == coda_format_ascii || type->format == coda_format_binary)
        {
            /* update bit_size */
            if (type->base_type != NULL && type->base_type->bit_size >= 0)
            {
                type->bit_size = type->num_elements * type->base_type->bit_size;
            }
        }
    }

    return 0;
}

int coda_type_array_add_variable_dimension(coda_type_array *type, coda_expression *dim_expr)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->num_dims == CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "maximum number of dimensions (%d) exceeded for array definition",
                       CODA_MAX_NUM_DIMS);
        return -1;
    }
    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        if (dim_expr == NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "dimension without size specification not allowed for %s array",
                           coda_type_get_format_name(type->format));
            return -1;
        }
    }
    type->dim[type->num_dims] = -1;
    type->dim_expr[type->num_dims] = dim_expr;
    type->num_dims++;

    /* update num_elements */
    type->num_elements = -1;
    if ((type->format == coda_format_ascii || type->format == coda_format_binary) && type->bit_size >= 0)
    {
        /* update bit_size */
        type->bit_size = -1;
    }

    return 0;
}

int coda_type_array_validate(const coda_type_array *type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->num_dims == 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "number of dimensions is 0 for array definition");
        return -1;
    }
    return 0;
}


coda_type_number *coda_type_number_new(coda_format format, coda_type_class type_class)
{
    coda_type_number *type;

    if (type_class != coda_integer_class && type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid type class (%s) for number type",
                       coda_type_get_class_name(type_class));
        return NULL;
    }

    type = (coda_type_number *)malloc(sizeof(coda_type_number));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_number), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = type_class;
    type->read_type = (type_class == coda_integer_class ? coda_native_type_int64 : coda_native_type_double);
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->unit = NULL;
    type->endianness = coda_big_endian;
    type->conversion = NULL;
    type->mappings = NULL;

    return type;
}

int coda_type_number_set_unit(coda_type_number *type, const char *unit)
{
    if (unit == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "unit argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a unit");
        return -1;
    }
    type->unit = strdup(unit);
    if (type->unit == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_type_number_set_endianness(coda_type_number *type, coda_endianness endianness)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    type->endianness = endianness;
    return 0;
}

int coda_type_number_set_conversion(coda_type_number *type, coda_conversion *conversion)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->conversion != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a conversion");
        return -1;
    }
    type->conversion = conversion;
    return 0;
}

int coda_type_number_add_ascii_float_mapping(coda_type_number *type, coda_ascii_float_mapping *mapping)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "mapping argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add floating point ascii mapping to integer type");
        return -1;
    }
    return mapping_type_add_mapping((coda_type *)type, &type->mappings, (coda_ascii_mapping *)mapping);
}

int coda_type_number_add_ascii_integer_mapping(coda_type_number *type, coda_ascii_integer_mapping *mapping)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "mapping argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_integer_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add integer ascii mapping to floating point type");
        return -1;
    }
    return mapping_type_add_mapping((coda_type *)type, &type->mappings, (coda_ascii_mapping *)mapping);
}

int coda_type_number_validate(const coda_type_number *type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->format == coda_format_binary)
    {
        if (type->bit_size >= 0)
        {
            switch (type->read_type)
            {
                case coda_native_type_int8:
                case coda_native_type_uint8:
                    if (type->bit_size > 8)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for integer type - "
                                       "it should be <= 8 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                case coda_native_type_int16:
                case coda_native_type_uint16:
                    if (type->bit_size > 16)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for integer type - "
                                       "it should be <= 16 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                case coda_native_type_int32:
                case coda_native_type_uint32:
                    if (type->bit_size > 32)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for integer type - "
                                       "it should be <= 32 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                case coda_native_type_int64:
                case coda_native_type_uint64:
                    if (type->bit_size > 64)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for integer type - "
                                       "it should be <= 64 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                case coda_native_type_float:
                    if (type->bit_size != 32)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for floating point type - "
                                       "it should be 32 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                case coda_native_type_double:
                    if (type->bit_size != 64)
                    {
                        coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for floating point type - "
                                       "it should be 64 when the read type is %s", (long)type->bit_size,
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
        }
        else if (type->size_expr == NULL)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "missing bit size or bit size expression for binary integer type");
            return -1;
        }
        if (type->endianness == coda_little_endian && type->bit_size >= 0 && type->bit_size % 8 != 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "bit size (%ld) must be a multiple of 8 for little endian binary integer type",
                           (long)type->bit_size);
            return -1;
        }
    }
    return 0;
}

coda_type_text *coda_type_text_new(coda_format format)
{
    coda_type_text *type;

    type = (coda_type_text *)malloc(sizeof(coda_type_text));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_text), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_text_class;
    type->read_type = coda_native_type_string;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->fixed_value = NULL;
    type->special_text_type = ascii_text_default;

    return type;
}

int coda_type_text_set_fixed_value(coda_type_text *type, const char *fixed_value)
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
    if (type->fixed_value != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text type already has a fixed value");
        return -1;
    }
    type->fixed_value = strdup(fixed_value);
    if (type->fixed_value == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_type_text_set_special_text_type(coda_type_text *type, coda_ascii_special_text_type special_text_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->format != coda_format_ascii)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "special ascii text type not allowed for %s format",
                       coda_type_get_format_name(type->format));
        return -1;
    }
    if (type->special_text_type != ascii_text_default)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text type already has a special text type set");
        return -1;
    }
    type->special_text_type = special_text_type;
    return 0;
}

int coda_type_text_validate(const coda_type_text *type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        if (type->size_expr == NULL && type->bit_size < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing bit size or bit size expression for text type");
            return -1;
        }
        if (type->bit_size >= 0 && type->bit_size % 8 != 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size (%ld) must be a multiple of 8 for text type",
                           (long)type->bit_size);
            return -1;
        }
    }
    if (type->read_type == coda_native_type_char && type->bit_size != 8)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size (%ld) must be 8 for text type when read type is 'char'",
                       (long)type->bit_size);
        return -1;
    }
    if (type->fixed_value != NULL)
    {
        if (type->bit_size < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "bit size for text type should be fixed if a fixed value is provided");
            return -1;
        }
        /* if there is a fixed_value its length should equal the byte size of the data element */
        if ((type->bit_size >> 3) != (int64_t)strlen(type->fixed_value))
        {
            char s[21];

            coda_str64(type->bit_size >> 3, s);
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "byte size of fixed value (%ld) should equal byte size (%s) for text type",
                           strlen(type->fixed_value), s);
            return -1;
        }
    }
    return 0;
}

coda_type_raw *coda_type_raw_new(coda_format format)
{
    coda_type_raw *type;

    type = (coda_type_raw *)malloc(sizeof(coda_type_raw));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_raw), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_raw_class;
    type->read_type = coda_native_type_bytes;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->fixed_value_length = -1;
    type->fixed_value = NULL;

    return type;
}

int coda_type_raw_set_fixed_value(coda_type_raw *type, long length, const char *fixed_value)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (length > 0 && fixed_value == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "fixed_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->fixed_value != NULL || type->fixed_value_length >= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text type already has a fixed value");
        return -1;
    }
    if (length > 0)
    {
        type->fixed_value = malloc(length);
        if (type->fixed_value == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
        memcpy(type->fixed_value, fixed_value, length);
        type->fixed_value_length = length;
    }
    else
    {
        type->fixed_value_length = 0;
    }

    return 0;
}

int coda_type_raw_validate(const coda_type_raw *type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->format == coda_format_ascii || type->format == coda_format_binary)
    {
        if (type->size_expr == NULL && type->bit_size < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing bit size or bit size expression for raw type");
            return -1;
        }
    }

    if (type->fixed_value != NULL)
    {
        int64_t byte_size;

        if (type->bit_size < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "bit size for raw type should be fixed if a fixed value is provided");
            return -1;
        }

        /* if there is a fixed_value its length should equal the byte size of the data element */
        byte_size = (type->bit_size >> 3) + (type->bit_size & 0x7 ? 1 : 0);
        if (byte_size != type->fixed_value_length)
        {
            char s[21];

            coda_str64(byte_size, s);
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "length of fixed value (%ld) should equal rounded byte size (%s) for raw type",
                           type->fixed_value_length, s);
            return -1;
        }
    }
    return 0;
}

coda_type_raw *coda_type_raw_file_singleton(void)
{
    if (raw_file_singleton == NULL)
    {
        coda_type_raw *type;
        coda_expression *byte_size_expr;

        type = coda_type_raw_new(coda_format_binary);
        if (type == NULL)
        {
            return NULL;
        }
        if (coda_expression_from_string("filesize()", &byte_size_expr) != 0)
        {
            raw_delete(type);
        }
        if (coda_type_set_byte_size_expression((coda_type *)type, byte_size_expr) != 0)
        {
            coda_expression_delete(byte_size_expr);
            raw_delete(type);
            return NULL;
        }

        raw_file_singleton = type;
    }

    return raw_file_singleton;
}

coda_type_special *coda_type_no_data_singleton(coda_format format)
{
    assert(format < num_no_data_singletons);

    if (no_data_singleton[format] == NULL)
    {
        coda_type_special *type;

        type = (coda_type_special *)malloc(sizeof(coda_type_special));
        if (type == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)sizeof(coda_type_special), __FILE__, __LINE__);
            return NULL;
        }
        type->format = format;
        type->retain_count = 0;
        type->type_class = coda_special_class;
        type->read_type = coda_native_type_not_available;
        type->name = NULL;
        type->description = NULL;
        type->bit_size = 0;
        type->size_expr = NULL;
        type->attributes = NULL;
        type->special_type = coda_special_no_data;
        type->base_type = NULL;
        type->unit = NULL;
        type->value_expr = NULL;

        type->base_type = (coda_type *)coda_type_raw_new(format);
        if (type->base_type == NULL)
        {
            special_delete(type);
            return NULL;
        }
        if (coda_type_set_bit_size(type->base_type, 0) != 0)
        {
            special_delete(type);
            return NULL;
        }

        no_data_singleton[format] = type;
    }

    return no_data_singleton[format];
}

coda_type_special *coda_type_vsf_integer_new(coda_format format)
{
    coda_type_special *type;

    type = (coda_type_special *)malloc(sizeof(coda_type_special));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_special), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_special_class;
    type->read_type = coda_native_type_double;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->special_type = coda_special_vsf_integer;
    type->base_type = NULL;
    type->unit = NULL;
    type->value_expr = NULL;

    type->base_type = (coda_type *)coda_type_record_new(format);
    coda_type_set_description(type->base_type, "Variable Scale Factor Integer");

    return type;
}

int coda_type_vsf_integer_set_type(coda_type_special *type, coda_type *base_type)
{
    coda_type_record_field *field;

    if (type->format != base_type->format)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "cannot use element type with %s format for vsf integer with %s format",
                       coda_type_get_format_name(base_type->format), coda_type_get_format_name(type->format));
        return -1;
    }
    if (((coda_type_record *)type->base_type)->num_fields != 1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "value should be second field of a vsf integer record");
        return -1;
    }

    field = coda_type_record_field_new("value");
    if (field == NULL)
    {
        return -1;
    }
    if (coda_type_record_field_set_type(field, base_type) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }
    if (coda_type_record_add_field((coda_type_record *)type->base_type, field) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }
    /* update bit_size */
    type->bit_size = type->base_type->bit_size;
    return 0;
}

int coda_type_vsf_integer_set_scale_factor(coda_type_special *type, coda_type *scale_factor)
{
    coda_type_record_field *field;
    coda_native_type scalefactor_type;

    if (type->format != scale_factor->format)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "cannot use scale factor type with %s format for vsf integer with %s format",
                       coda_type_get_format_name(scale_factor->format), coda_type_get_format_name(type->format));
        return -1;
    }
    if (((coda_type_record *)type->base_type)->num_fields != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "scale factor should be first field of a vsf integer record");
        return -1;
    }

    if (coda_type_get_read_type(scale_factor, &scalefactor_type) != 0)
    {
        return -1;
    }

    switch (scalefactor_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
        case coda_native_type_int16:
        case coda_native_type_uint16:
        case coda_native_type_int32:
            break;
        default:
            /* we do not support uint32_t/int64_t/uint64_t scale factors.
             * This allows us to use a more accurate pow10 function when applying the scale factor.
             */
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid scalefactor type (%s) for vsf integer type",
                           coda_type_get_native_type_name(scalefactor_type));
            return -1;
    }

    field = coda_type_record_field_new("scale_factor");
    if (field == NULL)
    {
        return -1;
    }
    if (coda_type_record_field_set_type(field, scale_factor) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }
    if (coda_type_record_add_field((coda_type_record *)type->base_type, field) != 0)
    {
        coda_type_record_field_delete(field);
        return -1;
    }
    /* update bit_size */
    type->bit_size = type->base_type->bit_size;
    return 0;
}

int coda_type_vsf_integer_set_unit(coda_type_special *type, const char *unit)
{
    if (unit == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "unit argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "type already has a unit");
        return -1;
    }
    type->unit = strdup(unit);
    if (type->unit == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

int coda_type_vsf_integer_validate(coda_type_special *type)
{
    if (((coda_type_record *)type->base_type)->num_fields != 2)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "vsf integer type requires both a base type and scale factor");
        return -1;
    }
    return 0;
}

coda_type_special *coda_type_time_new(coda_format format, coda_expression *value_expr)
{
    coda_type_special *type;

    if (value_expr == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "value_expr argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    type = (coda_type_special *)malloc(sizeof(coda_type_special));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_special), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_special_class;
    type->read_type = coda_native_type_double;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->special_type = coda_special_time;
    type->base_type = NULL;
    type->unit = NULL;
    type->value_expr = value_expr;

    type->unit = strdup("s since 2000-01-01");
    if (type->unit == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        special_delete(type);
        return NULL;
    }

    return type;
}

int coda_type_time_add_ascii_float_mapping(coda_type_special *type, coda_ascii_float_mapping *mapping)
{
    char strexpr[64];
    coda_expression *cond_expr;
    coda_expression *value_expr;
    coda_expression *node_expr;

    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "mapping argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->special_type != coda_special_time)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add floating point ascii mapping to '%s' special type",
                       coda_type_get_special_type_name(type->special_type));
        return -1;
    }
    if (type->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "special type does not have a base type");
        return -1;
    }
    if (type->base_type->type_class != coda_text_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot add floating point ascii mapping to time type with '%s'"
                       " base class", coda_type_get_class_name(type->base_type->type_class));
        return -1;
    }

    sprintf(strexpr, "%d", mapping->length);
    value_expr = coda_expression_new(expr_constant_integer, strdup(strexpr), NULL, NULL, NULL, NULL);
    if (mapping->length == 0)
    {
        /* wrap existing value_expr in if-construct: if(length(.)==0,<value>,<value_expr>) */
        node_expr = coda_expression_new(expr_goto_here, NULL, NULL, NULL, NULL, NULL);
        cond_expr = coda_expression_new(expr_length, NULL, node_expr, NULL, NULL, NULL);
    }
    else
    {
        /* wrap existing value_expr in if-construct: if(str(.,<length>)=="<str>",<value>,<value_expr>) */
        node_expr = coda_expression_new(expr_goto_here, NULL, NULL, NULL, NULL, NULL);
        cond_expr = coda_expression_new(expr_string, NULL, node_expr, value_expr, NULL, NULL);
        value_expr = coda_expression_new(expr_constant_string, strdup(mapping->str), NULL, NULL, NULL, NULL);
    }
    cond_expr = coda_expression_new(expr_equal, NULL, cond_expr, value_expr, NULL, NULL);
    coda_strfl(mapping->value, strexpr);
    value_expr = coda_expression_new(expr_constant_float, strdup(strexpr), NULL, NULL, NULL, NULL);
    type->value_expr = coda_expression_new(expr_if, NULL, cond_expr, value_expr, type->value_expr, NULL);

    coda_ascii_float_mapping_delete(mapping);

    return 0;
}

int coda_type_time_set_base_type(coda_type_special *type, coda_type *base_type)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (base_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "base_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->special_type != coda_special_time)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "cannot set base type for '%s' special type",
                       coda_type_get_special_type_name(type->special_type));
        return -1;
    }
    if (type->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "special type already has a base type");
        return -1;
    }

    type->base_type = base_type;
    base_type->retain_count++;

    /* update bit_size */
    type->bit_size = type->base_type->bit_size;

    return 0;
}

int coda_type_time_validate(coda_type_special *type)
{
    if (type->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing base type for time type");
        return -1;
    }
    return 0;
}

coda_type_special *coda_type_complex_new(coda_format format)
{
    coda_type_special *type;

    type = (coda_type_special *)malloc(sizeof(coda_type_special));
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_type_special), __FILE__, __LINE__);
        return NULL;
    }
    type->format = format;
    type->retain_count = 0;
    type->type_class = coda_special_class;
    type->read_type = coda_native_type_not_available;
    type->name = NULL;
    type->description = NULL;
    type->bit_size = -1;
    type->size_expr = NULL;
    type->attributes = NULL;
    type->special_type = coda_special_complex;
    type->base_type = NULL;
    type->unit = NULL;
    type->value_expr = NULL;

    return type;
}

int coda_type_complex_set_type(coda_type_special *type, coda_type *element_type)
{
    coda_type_record_field *field;

    if (type->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "complex type already has an element type");
        return -1;
    }
    if (element_type->type_class != coda_integer_class && element_type->type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid type class (%s) for element type of complex type",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (type->format != element_type->format)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "cannot use element type with %s format for complex type with %s format",
                       coda_type_get_format_name(element_type->format), coda_type_get_format_name(type->format));
        return -1;
    }

    type->base_type = (coda_type *)coda_type_record_new(type->format);
    field = coda_type_record_field_new("real");
    coda_type_record_field_set_type(field, element_type);
    coda_type_record_add_field((coda_type_record *)type->base_type, field);
    field = coda_type_record_field_new("imaginary");
    coda_type_record_field_set_type(field, element_type);
    coda_type_record_add_field((coda_type_record *)type->base_type, field);

    /* set bit_size */
    type->bit_size = type->base_type->bit_size;

    return 0;
}

int coda_type_complex_validate(coda_type_special *type)
{
    if (type->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing element type for complex type");
        return -1;
    }
    return 0;
}

void coda_type_done(void)
{
    int i;

    for (i = 0; i < num_empty_record_singletons; i++)
    {
        if (empty_record_singleton[i] != NULL)
        {
            coda_type_release((coda_type *)empty_record_singleton[i]);
        }
        empty_record_singleton[i] = NULL;
    }
    for (i = 0; i < num_no_data_singletons; i++)
    {
        if (no_data_singleton[i] != NULL)
        {
            coda_type_release((coda_type *)no_data_singleton[i]);
        }
        no_data_singleton[i] = NULL;
    }
}


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
        case coda_format_grib:
            return "grib";
        case coda_format_cdf:
            return "cdf";
        case coda_format_hdf4:
            return "hdf4";
        case coda_format_hdf5:
            return "hdf5";
        case coda_format_rinex:
            return "rinex";
        case coda_format_sp3:
            return "sp3";
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

/** Determine whether the type has any attributes.
 * If the record returned by coda_type_get_attributes() has one or more fields then \a has_attributes will be set to 1,
 * otherwise it will be set to 0.
 * \param type CODA type.
 * \param has_attributes Pointer to the variable where attribute availability status will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_has_attributes(const coda_type *type, int *has_attributes)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (has_attributes == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "has_attributes argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *has_attributes = (type->attributes != NULL);
    return 0;
}

/** Get the storage format of a type.
 * \param type CODA type.
 * \param format Pointer to a variable where the format will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_format(const coda_type *type, coda_format *format)
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
LIBCODA_API int coda_type_get_class(const coda_type *type, coda_type_class *type_class)
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
LIBCODA_API int coda_type_get_read_type(const coda_type *type, coda_native_type *read_type)
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

    if ((type->type_class == coda_integer_class || type->type_class == coda_real_class) &&
        coda_option_perform_conversions && ((coda_type_number *)type)->conversion != NULL)
    {
        *read_type = coda_native_type_double;
        return 0;
    }

    *read_type = type->read_type;
    return 0;
}

/** Get the length in bytes of a string data type.
 * If the type does not refer to text data the function will return an error.
 * If the size is not fixed and can only be determined from information inside a product then \a length will be set
 * to -1.
 * \param type CODA type.
 * \param length Pointer to a variable where the string length (not including terminating 0) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_string_length(const coda_type *type, long *length)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->type_class != coda_text_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "type does not refer to text (current type is %s)",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }
    if (length == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *length = (type->bit_size < 0 ? -1 : (long)(type->bit_size >> 3));
    return 0;
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
LIBCODA_API int coda_type_get_bit_size(const coda_type *type, int64_t *bit_size)
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

    *bit_size = (type->bit_size >= 0 ? type->bit_size : -1);
    return 0;
}

/** Get the name of a type.
 * A type can have an optional name that uniquely defines it within a product class. This is something that is used
 * internally within CODA to allow reuse of type definitions. If a type has a name, only a single instance of
 * the definition will be used for all places where the type is used (i.e. a single coda_type object will be used for
 * all cases where this type is used). For this reason type names are unique within the scope of a product class.
 * You should never rely in your code on types having a specific name, or having a name at all. The internal type reuse
 * approach within a product class may change unannounced.
 * If the type is unnamed a NULL pointer will be returned.
 * The \a name parameter will either be a NULL pointer or a 0 terminated string.
 * \param type CODA type.
 * \param name Pointer to the variable where the name of the type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_name(const coda_type *type, const char **name)
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
LIBCODA_API int coda_type_get_description(const coda_type *type, const char **description)
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
LIBCODA_API int coda_type_get_unit(const coda_type *type, const char **unit)
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

    switch (type->type_class)
    {
        case coda_array_class:
            {
                coda_type *base_type;

                if (coda_type_get_array_base_type(type, &base_type) != 0)
                {
                    return -1;
                }
                return coda_type_get_unit(base_type, unit);
            }
        case coda_integer_class:
        case coda_real_class:
            if (coda_option_perform_conversions && ((coda_type_number *)type)->conversion != NULL)
            {
                *unit = ((coda_type_number *)type)->conversion->unit;
            }
            else
            {
                *unit = ((coda_type_number *)type)->unit;
            }
            break;
        case coda_special_class:
            *unit = ((coda_type_special *)type)->unit;
            break;
        default:
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
LIBCODA_API int coda_type_get_fixed_value(const coda_type *type, const char **fixed_value, long *length)
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

    switch (type->type_class)
    {
        case coda_text_class:
            *fixed_value = ((coda_type_text *)type)->fixed_value;
            if (length != NULL)
            {
                *length = ((*fixed_value == NULL) ? 0 : (long)strlen(*fixed_value));
            }
            break;
        case coda_raw_class:
            *fixed_value = ((coda_type_raw *)type)->fixed_value;
            if (length != NULL)
            {
                *length = ((*fixed_value == NULL) ? 0 : ((coda_type_raw *)type)->fixed_value_length);
            }
            break;
        default:
            *fixed_value = NULL;
            break;
    }

    return 0;
}

/** Get the type for the associated attribute record.
 * Note that this record may not have any fields if there are no attributes for this type.
 * \param type CODA type.
 * \param attributes Pointer to the variable where the pointer to the type defining the attribute record will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_attributes(const coda_type *type, coda_type **attributes)
{
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (attributes == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "attributes argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type->attributes == NULL)
    {
        *attributes = (coda_type *)coda_type_empty_record(type->format);
    }
    else
    {
        *attributes = (coda_type *)type->attributes;
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
LIBCODA_API int coda_type_get_num_record_fields(const coda_type *type, long *num_fields)
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

    *num_fields = ((coda_type_record *)type)->num_fields;
    return 0;
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
LIBCODA_API int coda_type_get_record_field_index_from_name(const coda_type *type, const char *name, long *index)
{
    long field_index;

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

    field_index = hashtable_get_index_from_name(((coda_type_record *)type)->hash_data, name);
    if (field_index < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_NAME, "record does not contain a field named '%s'", name);
        return -1;
    }
    *index = field_index;
    return 0;
}

/* Get the field index from a field name for a record type where the field name may not be zero terminated.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param name Name of the record field.
 * \param name_length Maximum length of the name parameter.
 * \param index Pointer to a variable where the field index will be stored (0 <= \a index < number of fields).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_index_from_name_n(const coda_type *type, const char *name, int name_length,
                                                             long *index)
{
    long field_index;

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

    field_index = hashtable_get_index_from_name_n(((coda_type_record *)type)->hash_data, name, name_length);
    if (field_index < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_NAME, "record does not contain a field named '%.*s'", name_length, name);
        return -1;
    }
    *index = field_index;
    return 0;
}

/** Get the field index based on the 'real name' of the field for a record type.
 * If the type is not a record class the function will return an error.
 * If a field has no explicit 'real name' set, a match against the regular field name will be performed.
 * \param type CODA type.
 * \param real_name Real name of the record field.
 * \param index Pointer to a variable where the field index will be stored (0 <= \a index < number of fields).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_index_from_real_name(const coda_type *type, const char *real_name,
                                                                long *index)
{
    long field_index;

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
    if (real_name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "real_name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (index == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "index argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    field_index = hashtable_get_index_from_name(((coda_type_record *)type)->real_name_hash_data, real_name);
    if (field_index < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_NAME, "record does not contain a field with real name '%s'", real_name);
        return -1;
    }
    *index = field_index;
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
LIBCODA_API int coda_type_get_record_field_type(const coda_type *type, long index, coda_type **field_type)
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

    if (index < 0 || index >= ((coda_type_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_type_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *field_type = ((coda_type_record *)type)->field[index]->type;
    return 0;
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
LIBCODA_API int coda_type_get_record_field_name(const coda_type *type, long index, const char **name)
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

    if (index < 0 || index >= ((coda_type_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_type_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *name = ((coda_type_record *)type)->field[index]->name;
    return 0;
}

/** Get the unaltered name of a record field.
 * The real name of a field is the name of the field without the identifier restriction.
 * For (partially) self-describing formats such as XML, HDF, and netCDF, the name of a field as used by CODA will
 * actually be a conversion of the name of the stored element to something that conforms to the rules of an identifier
 * (i.e. only allowing a-z, A-Z, 0-9 and underscores characters and names have to start with an alpha character).
 * The real name property of a field represents the original name of the element (e.g. XML element name, HDF5 DataSet
 * name, netCDF variable name, etc.).
 * If the concept of a real name does not apply, this function will return the same result as
 * coda_type_get_record_field_name().
 *
 * If the type is not a record class the function will return an error.
 * The \a real_name parameter will be 0 terminated.
 * \param type CODA type.
 * \param index Field index (0 <= \a index < number of fields).
 * \param real_name Pointer to the variable where the real name of the record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_field_real_name(const coda_type *type, long index, const char **real_name)
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
    if (real_name == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (index < 0 || index >= ((coda_type_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_type_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    if (((coda_type_record *)type)->field[index]->real_name != NULL)
    {
        if (type->format == coda_format_xml)
        {
            *real_name = coda_element_name_from_xml_name(((coda_type_record *)type)->field[index]->real_name);
        }
        else
        {
            *real_name = ((coda_type_record *)type)->field[index]->real_name;
        }
    }
    else
    {
        *real_name = ((coda_type_record *)type)->field[index]->name;
    }
    return 0;
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
LIBCODA_API int coda_type_get_record_field_hidden_status(const coda_type *type, long index, int *hidden)
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

    if (index < 0 || index >= ((coda_type_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_type_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *hidden = ((coda_type_record *)type)->field[index]->hidden;
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
LIBCODA_API int coda_type_get_record_field_available_status(const coda_type *type, long index, int *available)
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

    if (index < 0 || index >= ((coda_type_record *)type)->num_fields)
    {
        coda_set_error(CODA_ERROR_INVALID_INDEX, "field index (%ld) is not in the range [0,%ld) (%s:%u)", index,
                       ((coda_type_record *)type)->num_fields, __FILE__, __LINE__);
        return -1;
    }
    *available = (((coda_type_record *)type)->field[index]->optional ? -1 : 1);
    return 0;
}

/** Get the union status of a record.
 * If the record is a union (i.e. all fields are dynamically available and only one field can be available at any time)
 * \a is_union will be set to 1, otherwise it will be set to 0.
 * If the type is not a record class the function will return an error.
 * \param type CODA type.
 * \param is_union Pointer to a variable where the union status will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_record_union_status(const coda_type *type, int *is_union)
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

    *is_union = ((coda_type_record *)type)->is_union;
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
LIBCODA_API int coda_type_get_array_num_dims(const coda_type *type, int *num_dims)
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

    *num_dims = ((coda_type_array *)type)->num_dims;
    return 0;
}

/** Retrieve the dimensions with a constant value for an array.
 * The function returns both the number of dimensions \a num_dims and the size for each of the dimensions \a dim that
 * have a constant/fixed size.
 * \note If the size of a dimension is variable (it differs per product or differs per occurrence inside one product)
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
LIBCODA_API int coda_type_get_array_dim(const coda_type *type, int *num_dims, long dim[])
{
    int i;

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

    *num_dims = ((coda_type_array *)type)->num_dims;
    for (i = 0; i < ((coda_type_array *)type)->num_dims; i++)
    {
        dim[i] = ((coda_type_array *)type)->dim[i];
    }
    return 0;
}

/** Get the CODA type for the elements of an array.
 * If the type is not an array class the function will return an error.
 * \param type CODA type.
 * \param base_type Pointer to the variable where the base type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_array_base_type(const coda_type *type, coda_type **base_type)
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

    *base_type = ((coda_type_array *)type)->base_type;
    return 0;
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
LIBCODA_API int coda_type_get_special_type(const coda_type *type, coda_special_type *special_type)
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

    *special_type = ((coda_type_special *)type)->special_type;
    return 0;
}

/** Get the base type for a special type.
 * If the type is not a special type the function will return an error.
 * \param type CODA type.
 * \param base_type Pointer to the variable where the base type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_type_get_special_base_type(const coda_type *type, coda_type **base_type)
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

    *base_type = ((coda_type_special *)type)->base_type;
    return 0;
}

/** @} */
