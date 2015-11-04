/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#include "coda-bin-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "coda-definition.h"
#include "coda-bin-definition.h"

static coda_bin_no_data *no_data_singleton = NULL;

static void delete_bin_integer(coda_bin_integer *integer)
{
    if (integer->name != NULL)
    {
        free(integer->name);
    }
    if (integer->description != NULL)
    {
        free(integer->description);
    }
    if (integer->unit != NULL)
    {
        free(integer->unit);
    }
    if (integer->conversion != NULL)
    {
        coda_conversion_delete(integer->conversion);
    }
    if (integer->bit_size_expr != NULL)
    {
        coda_expression_delete(integer->bit_size_expr);
    }
    free(integer);
}

static void delete_bin_float(coda_bin_float *fl)
{
    if (fl->name != NULL)
    {
        free(fl->name);
    }
    if (fl->description != NULL)
    {
        free(fl->description);
    }
    if (fl->unit != NULL)
    {
        free(fl->unit);
    }
    if (fl->conversion != NULL)
    {
        coda_conversion_delete(fl->conversion);
    }
    free(fl);
}

static void delete_bin_raw(coda_bin_raw *raw)
{
    if (raw->name != NULL)
    {
        free(raw->name);
    }
    if (raw->description != NULL)
    {
        free(raw->description);
    }
    if (raw->bit_size_expr != NULL)
    {
        coda_expression_delete(raw->bit_size_expr);
    }
    if (raw->fixed_value != NULL)
    {
        free(raw->fixed_value);
    }
    free(raw);
}

static void delete_bin_no_data(coda_bin_no_data *no_data)
{
    if (no_data->name != NULL)
    {
        free(no_data->name);
    }
    if (no_data->description != NULL)
    {
        free(no_data->description);
    }
    if (no_data_singleton->base_type != NULL)
    {
        coda_bin_release_type(no_data_singleton->base_type);
    }
    free(no_data);
}

static void delete_bin_vsf_integer(coda_bin_vsf_integer *vsf_integer)
{
    if (vsf_integer->name != NULL)
    {
        free(vsf_integer->name);
    }
    if (vsf_integer->description != NULL)
    {
        free(vsf_integer->description);
    }
    if (vsf_integer->unit != NULL)
    {
        free(vsf_integer->unit);
    }
    if (vsf_integer->base_type != NULL)
    {
        coda_bin_release_type(vsf_integer->base_type);
    }
    free(vsf_integer);
}

static void delete_bin_time(coda_bin_time *time)
{
    if (time->name != NULL)
    {
        free(time->name);
    }
    if (time->description != NULL)
    {
        free(time->description);
    }
    if (time->base_type != NULL)
    {
        coda_bin_release_type(time->base_type);
    }
    free(time);
}

static void delete_bin_complex(coda_bin_complex *compl)
{
    if (compl->name != NULL)
    {
        free(compl->name);
    }
    if (compl->description != NULL)
    {
        free(compl->description);
    }
    if (compl->base_type != NULL)
    {
        coda_bin_release_type(compl->base_type);
    }
    free(compl);
}

void coda_bin_release_type(coda_bin_type *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (((coda_bin_type *)type)->tag)
    {
        case tag_bin_record:
            coda_ascbin_record_delete((coda_ascbin_record *)type);
            break;
        case tag_bin_union:
            coda_ascbin_union_delete((coda_ascbin_union *)type);
            break;
        case tag_bin_array:
            coda_ascbin_array_delete((coda_ascbin_array *)type);
            break;
        case tag_bin_integer:
            delete_bin_integer((coda_bin_integer *)type);
            break;
        case tag_bin_float:
            delete_bin_float((coda_bin_float *)type);
            break;
        case tag_bin_raw:
            delete_bin_raw((coda_bin_raw *)type);
            break;
        case tag_bin_no_data:
            /* this is a singleton -> only free when coda_bin_done() is called */
            break;
        case tag_bin_vsf_integer:
            delete_bin_vsf_integer((coda_bin_vsf_integer *)type);
            break;
        case tag_bin_time:
            delete_bin_time((coda_bin_time *)type);
            break;
        case tag_bin_complex:
            delete_bin_complex((coda_bin_complex *)type);
            break;
    }
}

void coda_bin_release_dynamic_type(coda_dynamic_type *type)
{
    coda_bin_release_type((coda_bin_type *)type);
}

static int number_set_unit(coda_bin_number *number, const char *unit)
{
    char *new_unit = NULL;

    if (number->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "number already has a unit");
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
    number->unit = new_unit;

    return 0;
}


coda_dynamic_type *coda_bin_no_data_singleton(void)
{
    if (no_data_singleton == NULL)
    {
        no_data_singleton = (coda_bin_no_data *)malloc(sizeof(coda_bin_no_data));
        assert(no_data_singleton != NULL);
        no_data_singleton->format = coda_format_binary;
        no_data_singleton->type_class = coda_special_class;
        no_data_singleton->name = NULL;
        no_data_singleton->description = NULL;
        no_data_singleton->tag = tag_bin_no_data;
        no_data_singleton->base_type = (coda_bin_type *)coda_bin_raw_new();
        coda_bin_raw_set_bit_size((coda_bin_raw *)no_data_singleton->base_type, 0);
        no_data_singleton->bit_size = no_data_singleton->base_type->bit_size;
    }

    return (coda_dynamic_type *)no_data_singleton;
}


coda_bin_integer *coda_bin_integer_new(void)
{
    coda_bin_integer *integer;

    integer = (coda_bin_integer *)malloc(sizeof(coda_bin_integer));
    if (integer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_bin_integer), __FILE__, __LINE__);
        return NULL;
    }
    integer->retain_count = 0;
    integer->format = coda_format_binary;
    integer->type_class = coda_integer_class;
    integer->name = NULL;
    integer->description = NULL;
    integer->tag = tag_bin_integer;
    integer->bit_size = -1;
    integer->unit = NULL;
    integer->read_type = coda_native_type_not_available;
    integer->conversion = NULL;
    integer->endianness = coda_big_endian;
    integer->bit_size_expr = NULL;

    return integer;
}

int coda_bin_integer_set_unit(coda_bin_integer *integer, const char *unit)
{
    return number_set_unit((coda_bin_number *)integer, unit);
}

int coda_bin_integer_set_bit_size(coda_bin_integer *integer, long bit_size)
{
    if (integer->bit_size != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "integer already has a bit size");
        return -1;
    }
    if (bit_size <= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size (%ld) must be > 0 for binary integer definition",
                       bit_size);
        return -1;
    }
    integer->bit_size = bit_size;
    if (integer->bit_size_expr != NULL)
    {
        coda_expression_delete(integer->bit_size_expr);
        integer->bit_size_expr = NULL;
    }
    return 0;
}

int coda_bin_integer_set_bit_size_expression(coda_bin_integer *integer, coda_expression *bit_size_expr)
{
    assert(bit_size_expr != NULL);
    if (integer->bit_size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "integer already has a bit size expression");
        return -1;
    }
    integer->bit_size_expr = bit_size_expr;
    integer->bit_size = -1;
    return 0;
}

int coda_bin_integer_set_read_type(coda_bin_integer *integer, coda_native_type read_type)
{
    if (integer->read_type != coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "integer already has a read type");
        return -1;
    }
    if (read_type != coda_native_type_int8 && read_type != coda_native_type_uint8 &&
        read_type != coda_native_type_int16 && read_type != coda_native_type_uint16 &&
        read_type != coda_native_type_int32 && read_type != coda_native_type_uint32 &&
        read_type != coda_native_type_int64 && read_type != coda_native_type_uint64)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for binary integer definition",
                       coda_type_get_native_type_name(read_type));
        return -1;
    }
    integer->read_type = read_type;
    return 0;
}

int coda_bin_integer_set_conversion(coda_bin_integer *integer, coda_conversion *conversion)
{
    if (integer->conversion != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "integer already has a conversion");
        return -1;
    }
    integer->conversion = conversion;
    return 0;
}

int coda_bin_integer_set_endianness(coda_bin_integer *integer, coda_endianness endianness)
{
    integer->endianness = endianness;
    return 0;
}

int coda_bin_integer_validate(coda_bin_integer *integer)
{
    if (integer->bit_size_expr == NULL && integer->bit_size == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "missing bit size or bit size expression for binary integer definition");
        return -1;
    }
    if (integer->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for binary integer definition");
        return -1;
    }
    switch (integer->read_type)
    {
        case coda_native_type_int8:
        case coda_native_type_uint8:
            if (integer->bit_size > 8)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary integer definition - "
                               "it should be <= 8 when the read type is %s", (long)integer->bit_size,
                               coda_type_get_native_type_name(integer->read_type));
                return -1;
            }
            break;
        case coda_native_type_int16:
        case coda_native_type_uint16:
            if (integer->bit_size > 16)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary integer definition - "
                               "it should be <= 16 when the read type is %s", (long)integer->bit_size,
                               coda_type_get_native_type_name(integer->read_type));
                return -1;
            }
            break;
        case coda_native_type_int32:
        case coda_native_type_uint32:
            if (integer->bit_size > 32)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary integer definition - "
                               "it should be <= 32 when the read type is %s", (long)integer->bit_size,
                               coda_type_get_native_type_name(integer->read_type));
                return -1;
            }
            break;
        case coda_native_type_int64:
        case coda_native_type_uint64:
            if (integer->bit_size > 64)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary integer definition - "
                               "it should be <= 64 when the read type is %s", (long)integer->bit_size,
                               coda_type_get_native_type_name(integer->read_type));
                return -1;
            }
            break;
        default:
            assert(0);
            break;
    }
    if (integer->endianness == coda_little_endian && integer->bit_size % 8 != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "bit size (%ld) must be a multiple of 8 for little endian binary integer definition",
                       (long)integer->bit_size);
        return -1;
    }
    return 0;
}

coda_bin_float *coda_bin_float_new(void)
{
    coda_bin_float *fl;

    fl = (coda_bin_float *)malloc(sizeof(coda_bin_float));
    if (fl == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_bin_float), __FILE__, __LINE__);
        return NULL;
    }
    fl->retain_count = 0;
    fl->format = coda_format_binary;
    fl->type_class = coda_real_class;
    fl->name = NULL;
    fl->description = NULL;
    fl->tag = tag_bin_float;
    fl->bit_size = -1;
    fl->unit = NULL;
    fl->read_type = coda_native_type_not_available;
    fl->conversion = NULL;
    fl->endianness = coda_big_endian;

    return fl;
}

int coda_bin_float_set_unit(coda_bin_float *fl, const char *unit)
{
    return number_set_unit((coda_bin_number *)fl, unit);
}

int coda_bin_float_set_bit_size(coda_bin_float *fl, long bit_size)
{
    if (fl->bit_size != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "float already has a bit size");
        return -1;
    }
    if (bit_size != 32 && bit_size != 64)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size (%ld) can only be 32 or 64 for binary float definition",
                       bit_size);
        return -1;
    }
    fl->bit_size = bit_size;
    return 0;
}

int coda_bin_float_set_read_type(coda_bin_float *fl, coda_native_type read_type)
{
    if (fl->read_type != coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "float already has a read type");
        return -1;
    }
    if (read_type != coda_native_type_float && read_type != coda_native_type_double)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for binary float definition",
                       coda_type_get_native_type_name(read_type));
        return -1;
    }
    fl->read_type = read_type;
    return 0;
}

int coda_bin_float_set_conversion(coda_bin_float *fl, coda_conversion *conversion)
{
    if (fl->conversion != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "float already has a conversion");
        return -1;
    }
    fl->conversion = conversion;
    return 0;
}

int coda_bin_float_set_endianness(coda_bin_float *integer, coda_endianness endianness)
{
    integer->endianness = endianness;
    return 0;
}

int coda_bin_float_validate(coda_bin_float *fl)
{
    if (fl->bit_size == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing bit size for binary float definition");
        return -1;
    }
    if (fl->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for binary float definition");
        return -1;
    }
    switch (fl->read_type)
    {
        case coda_native_type_float:
            if (fl->bit_size != 32)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary float definition - "
                               "it should be 32 when the read type is float", (long)fl->bit_size);
                return -1;
            }
            break;
        case coda_native_type_double:
            if (fl->bit_size != 64)
            {
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "incorrect bit size (%ld) for binary float definition - "
                               "it should be 64 when the read type is double", (long)fl->bit_size);
                return -1;
            }
            break;
        default:
            assert(0);
            break;
    }
    return 0;
}

coda_bin_raw *coda_bin_raw_new(void)
{
    coda_bin_raw *raw;

    raw = (coda_bin_raw *)malloc(sizeof(coda_bin_raw));
    if (raw == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_bin_raw), __FILE__, __LINE__);
        return NULL;
    }
    raw->retain_count = 0;
    raw->format = coda_format_binary;
    raw->type_class = coda_raw_class;
    raw->name = NULL;
    raw->description = NULL;
    raw->tag = tag_bin_raw;
    raw->bit_size = -1;
    raw->bit_size_expr = NULL;
    raw->fixed_value_length = -1;
    raw->fixed_value = NULL;

    return raw;
}

int coda_bin_raw_set_bit_size(coda_bin_raw *raw, int64_t bit_size)
{
    if (raw->bit_size != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "raw already has a bit size");
        return -1;
    }
    if (bit_size < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "bit size may not be < 0 for raw definition");
        return -1;
    }
    raw->bit_size = bit_size;
    if (raw->bit_size_expr != NULL)
    {
        coda_expression_delete(raw->bit_size_expr);
        raw->bit_size_expr = NULL;
    }
    return 0;
}

int coda_bin_raw_set_bit_size_expression(coda_bin_raw *raw, coda_expression *bit_size_expr)
{
    assert(bit_size_expr != NULL);
    if (raw->bit_size_expr != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "raw already has a bit size expression");
        return -1;
    }
    raw->bit_size_expr = bit_size_expr;
    raw->bit_size = -1;
    return 0;
}

int coda_bin_raw_set_fixed_value(coda_bin_raw *raw, long length, char *fixed_value)
{
    char *new_fixed_value = NULL;

    if (raw->fixed_value != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "raw already has a fixed value");
        return -1;
    }
    if (fixed_value != NULL && length > 0)
    {
        new_fixed_value = malloc(length);
        if (new_fixed_value == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
        memcpy(new_fixed_value, fixed_value, length);
    }
    else
    {
        length = 0;
    }
    raw->fixed_value = new_fixed_value;
    raw->fixed_value_length = length;

    return 0;
}

int coda_bin_raw_validate(coda_bin_raw *raw)
{
    if (raw->bit_size_expr == NULL && raw->bit_size == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing bit size or bit size expression for raw definition");
        return -1;
    }
    /* if there is a fixed_value its length should equal the byte size of the data element */
    if (raw->fixed_value != NULL)
    {
        int64_t byte_size;

        byte_size = (raw->bit_size >> 3) + (raw->bit_size & 0x7 ? 1 : 0);
        if (byte_size != raw->fixed_value_length)
        {
            char s[21];

            coda_str64(byte_size, s);
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "length of fixed value (%ld) should equal rounded byte size (%s) for raw definition",
                           raw->fixed_value_length, s);
            return -1;
        }
    }
    return 0;
}

coda_bin_vsf_integer *coda_bin_vsf_integer_new(void)
{
    coda_bin_vsf_integer *integer;

    integer = (coda_bin_vsf_integer *)malloc(sizeof(coda_bin_vsf_integer));
    if (integer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_bin_vsf_integer), __FILE__, __LINE__);
        return NULL;
    }
    integer->retain_count = 0;
    integer->format = coda_format_binary;
    integer->type_class = coda_special_class;
    integer->name = NULL;
    integer->description = NULL;
    integer->tag = tag_bin_vsf_integer;
    integer->bit_size = 0;
    integer->unit = NULL;

    integer->base_type = (coda_bin_type *)coda_ascbin_record_new(coda_format_binary);
    coda_type_set_description((coda_type *)integer->base_type, "Variable Scale Factor Integer");

    return integer;
}

int coda_bin_vsf_integer_set_type(coda_bin_vsf_integer *integer, coda_bin_type *base_type)
{
    coda_ascbin_field *field;

    field = coda_ascbin_field_new("value", NULL);
    if (field == NULL)
    {
        return -1;
    }
    if (coda_ascbin_field_set_type(field, (coda_ascbin_type *)base_type) != 0)
    {
        coda_ascbin_field_delete(field);
        return -1;
    }
    if (coda_ascbin_record_add_field((coda_ascbin_record *)integer->base_type, field) != 0)
    {
        coda_ascbin_field_delete(field);
        return -1;
    }
    integer->bit_size = ((coda_ascbin_record *)integer->base_type)->bit_size;

    return 0;
}

int coda_bin_vsf_integer_set_scale_factor(coda_bin_vsf_integer *integer, coda_bin_type *scale_factor)
{
    coda_ascbin_field *field;
    coda_native_type scalefactor_type;

    if (coda_type_get_read_type((coda_type *)scale_factor, &scalefactor_type) != 0)
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
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid scalefactor type (%s) for vsf integer definition",
                           coda_type_get_native_type_name(scalefactor_type));
            return -1;
    }

    field = coda_ascbin_field_new("scale_factor", NULL);
    if (field == NULL)
    {
        return -1;
    }
    if (coda_ascbin_field_set_type(field, (coda_ascbin_type *)scale_factor) != 0)
    {
        coda_ascbin_field_delete(field);
        return -1;
    }
    if (coda_ascbin_record_add_field((coda_ascbin_record *)integer->base_type, field) != 0)
    {
        coda_ascbin_field_delete(field);
        return -1;
    }
    integer->bit_size = ((coda_ascbin_record *)integer->base_type)->bit_size;

    return 0;
}

int coda_bin_vsf_integer_set_unit(coda_bin_vsf_integer *integer, const char *unit)
{
    char *new_unit = NULL;

    if (integer->unit != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "number already has a unit");
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
    integer->unit = new_unit;

    return 0;
}

int coda_bin_vsf_integer_validate(coda_bin_vsf_integer *integer)
{
    if (((coda_ascbin_record *)integer->base_type)->num_fields != 2)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "vsf integer requires both base type and scale factor");
        return -1;
    }
    return 0;
}

coda_bin_time *coda_bin_time_new(const char *format)
{
    coda_bin_time *time;
    coda_ascbin_record *record;
    coda_ascbin_field *field;
    coda_bin_integer *type;

    time = (coda_bin_time *)malloc(sizeof(coda_bin_time));
    if (time == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_bin_time), __FILE__, __LINE__);
        return NULL;
    }
    time->retain_count = 0;
    time->format = coda_format_binary;
    time->type_class = coda_special_class;
    time->name = NULL;
    time->description = NULL;
    time->tag = tag_bin_time;
    time->base_type = NULL;
    time->time_type = 0;

    record = coda_ascbin_record_new(coda_format_binary);
    time->base_type = (coda_bin_type *)record;
    if (strcmp(format, "binary_envisat_datetime") == 0)
    {
        time->time_type = binary_envisat_datetime;
        coda_type_set_description((coda_type *)record, "ENVISAT binary datetime");

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "days since January 1st, 2000 (may be negative)");
        coda_bin_integer_set_unit(type, "days since 2000-01-01");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_int32);
        field = coda_ascbin_field_new("days", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "seconds since start of day");
        coda_bin_integer_set_unit(type, "s");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_uint32);
        field = coda_ascbin_field_new("seconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "microseconds since start of second");
        coda_bin_integer_set_unit(type, "1e-6 s");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_uint32);
        field = coda_ascbin_field_new("microseconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);
    }
    else if (strcmp(format, "binary_gome_datetime") == 0)
    {
        time->time_type = binary_gome_datetime;
        coda_type_set_description((coda_type *)record, "GOME binary datetime");

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "days since January 1st, 1950 (may be negative)");
        coda_bin_integer_set_unit(type, "days since 1950-01-01");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_int32);
        field = coda_ascbin_field_new("days", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "milliseconds since start of day");
        coda_bin_integer_set_unit(type, "1e-3 s");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_uint32);
        field = coda_ascbin_field_new("milliseconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);
    }
    else if (strcmp(format, "binary_eps_datetime") == 0)
    {
        time->time_type = binary_eps_datetime;
        coda_type_set_description((coda_type *)record, "EPS short cds");

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "days since January 1st, 2000 (must be positive)");
        coda_bin_integer_set_unit(type, "days since 2000-01-01");
        coda_bin_integer_set_bit_size(type, 16);
        coda_bin_integer_set_read_type(type, coda_native_type_uint16);
        field = coda_ascbin_field_new("days", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "milliseconds since start of day");
        coda_bin_integer_set_unit(type, "1e-3 s");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_uint32);
        field = coda_ascbin_field_new("milliseconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);
    }
    else if (strcmp(format, "binary_eps_datetime_long") == 0)
    {
        time->time_type = binary_eps_datetime_long;
        coda_type_set_description((coda_type *)record, "EPS long cds");

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "days since January 1st, 2000 (must be positive)");
        coda_bin_integer_set_unit(type, "days since 2000-01-01");
        coda_bin_integer_set_bit_size(type, 16);
        coda_bin_integer_set_read_type(type, coda_native_type_uint16);
        field = coda_ascbin_field_new("days", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "milliseconds since start of day");
        coda_bin_integer_set_unit(type, "1e-3 s");
        coda_bin_integer_set_bit_size(type, 32);
        coda_bin_integer_set_read_type(type, coda_native_type_uint32);
        field = coda_ascbin_field_new("milliseconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);

        type = coda_bin_integer_new();
        coda_type_set_description((coda_type *)type, "microseconds since start of millisecond");
        coda_bin_integer_set_unit(type, "1e-6 s");
        coda_bin_integer_set_bit_size(type, 16);
        coda_bin_integer_set_read_type(type, coda_native_type_uint16);
        field = coda_ascbin_field_new("microseconds", NULL);
        coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
        coda_bin_release_type((coda_bin_type *)type);
        coda_ascbin_record_add_field(record, field);
    }
    else
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid time format (%s) for ascii time definition", format);
        delete_bin_time(time);
        return NULL;
    }

    /* set bit_size */
    time->bit_size = time->base_type->bit_size;

    return time;
}

coda_bin_complex *coda_bin_complex_new(void)
{
    coda_bin_complex *compl;

    compl = (coda_bin_complex *)malloc(sizeof(coda_bin_complex));
    assert(compl != NULL);
    compl->retain_count = 0;
    compl->format = coda_format_binary;
    compl->type_class = coda_special_class;
    compl->name = NULL;
    compl->description = NULL;
    compl->tag = tag_bin_complex;
    compl->bit_size = -1;
    compl->base_type = NULL;

    return compl;
}

int coda_bin_complex_set_type(coda_bin_complex *compl, coda_bin_type *type)
{
    coda_ascbin_field *field;

    if (compl->base_type != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "complex already has a type");
        return -1;
    }
    if (type->type_class != coda_integer_class && type->type_class != coda_real_class)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid type class (%s) for element type of complex definition",
                       coda_type_get_class_name(type->type_class));
        return -1;
    }

    compl->base_type = (coda_bin_type *)coda_ascbin_record_new(coda_format_binary);

    field = coda_ascbin_field_new("real", NULL);
    coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
    coda_ascbin_record_add_field((coda_ascbin_record *)compl->base_type, field);

    field = coda_ascbin_field_new("imaginary", NULL);
    coda_ascbin_field_set_type(field, (coda_ascbin_type *)type);
    coda_ascbin_record_add_field((coda_ascbin_record *)compl->base_type, field);

    /* set bit_size */
    compl->bit_size = compl->base_type->bit_size;

    return 0;
}

int coda_bin_complex_validate(coda_bin_complex *compl)
{
    if (compl->base_type == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing element type for complex definition");
        return -1;
    }
    return 0;
}

void coda_bin_done(void)
{
    if (no_data_singleton != NULL)
    {
        delete_bin_no_data(no_data_singleton);
        no_data_singleton = NULL;
    }
    coda_ascbin_done();
}
