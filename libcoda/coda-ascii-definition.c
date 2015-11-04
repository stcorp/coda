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

#include "coda-ascii-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "coda-definition.h"
#include "coda-ascii-definition.h"
#include "coda-expr-internal.h"

static void delete_mapping(coda_asciiMapping *mapping)
{
    if (mapping->str != NULL)
    {
        free(mapping->str);
    }
    free(mapping);
}

static void delete_mappings(coda_asciiMappings *mappings)
{
    if (mappings->mapping != NULL)
    {
        int i;

        for (i = 0; i < mappings->num_mappings; i++)
        {
            if (mappings->mapping[i] != NULL)
            {
                delete_mapping(mappings->mapping[i]);
            }
        }
        free(mappings->mapping);
    }
    free(mappings);
}

static void delete_ascii_integer(coda_asciiInteger *integer)
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
    if (integer->mappings != NULL)
    {
        delete_mappings(integer->mappings);
    }
    free(integer);
}

static void delete_ascii_float(coda_asciiFloat *fl)
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
    if (fl->mappings != NULL)
    {
        delete_mappings(fl->mappings);
    }
    free(fl);
}

static void delete_ascii_text(coda_asciiText *text)
{
    if (text->name != NULL)
    {
        free(text->name);
    }
    if (text->description != NULL)
    {
        free(text->description);
    }
    if (text->byte_size_expr != NULL)
    {
        coda_expr_delete(text->byte_size_expr);
    }
    if (text->fixed_value != NULL)
    {
        free(text->fixed_value);
    }
    if (text->mappings != NULL)
    {
        delete_mappings(text->mappings);
    }
    free(text);
}

static void delete_ascii_line_separator(coda_asciiLineSeparator *text)
{
    if (text->name != NULL)
    {
        free(text->name);
    }
    if (text->description != NULL)
    {
        free(text->description);
    }
    free(text);
}

static void delete_ascii_line(coda_asciiLine *text)
{
    if (text->name != NULL)
    {
        free(text->name);
    }
    if (text->description != NULL)
    {
        free(text->description);
    }
    free(text);
}

static void delete_ascii_white_space(coda_asciiWhiteSpace *text)
{
    if (text->name != NULL)
    {
        free(text->name);
    }
    if (text->description != NULL)
    {
        free(text->description);
    }
    free(text);
}

static void delete_ascii_time(coda_asciiTime *time)
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
        coda_ascii_release_type(time->base_type);
    }
    free(time);
}

void coda_ascii_release_type(coda_asciiType *type)
{
    assert(type != NULL);

    if (type->retain_count > 0)
    {
        type->retain_count--;
        return;
    }

    switch (((coda_asciiType *)type)->tag)
    {
        case tag_ascii_record:
            coda_ascbin_record_delete((coda_ascbinRecord *)type);
            break;
        case tag_ascii_union:
            coda_ascbin_union_delete((coda_ascbinUnion *)type);
            break;
        case tag_ascii_array:
            coda_ascbin_array_delete((coda_ascbinArray *)type);
            break;
        case tag_ascii_integer:
            delete_ascii_integer((coda_asciiInteger *)type);
            break;
        case tag_ascii_float:
            delete_ascii_float((coda_asciiFloat *)type);
            break;
        case tag_ascii_text:
            delete_ascii_text((coda_asciiText *)type);
            break;
        case tag_ascii_line_separator:
            delete_ascii_line_separator((coda_asciiLineSeparator *)type);
            break;
        case tag_ascii_line:
            delete_ascii_line((coda_asciiLine *)type);
            break;
        case tag_ascii_white_space:
            delete_ascii_white_space((coda_asciiWhiteSpace *)type);
            break;
        case tag_ascii_time:
            delete_ascii_time((coda_asciiTime *)type);
            break;
    }
}

void coda_ascii_release_dynamic_type(coda_DynamicType *type)
{
    coda_ascii_release_type((coda_asciiType *)type);
}

static int number_set_unit(coda_asciiNumber *number, const char *unit)
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

static int mapping_type_add_mapping(coda_asciiMappingsType *type, coda_asciiMapping *mapping)
{
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty mapping (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (type->mappings == NULL)
    {
        type->mappings = malloc(sizeof(coda_asciiMappings));
        if (type->mappings == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           sizeof(coda_asciiMappings), __FILE__, __LINE__);
            return -1;
        }
        type->mappings->default_bit_size = type->bit_size;
        type->mappings->num_mappings = 0;
        type->mappings->mapping = NULL;

    }

    if (type->mappings->num_mappings % BLOCK_SIZE == 0)
    {
        coda_asciiMapping **new_mapping;
        new_mapping = realloc(type->mappings->mapping,
                              (type->mappings->num_mappings + BLOCK_SIZE) * sizeof(coda_asciiMapping *));
        if (new_mapping == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (type->mappings->num_mappings + BLOCK_SIZE) * sizeof(coda_asciiMapping *), __FILE__,
                           __LINE__);
            return -1;
        }
        type->mappings->mapping = new_mapping;
    }
    type->mappings->mapping[type->mappings->num_mappings] = mapping;
    type->mappings->num_mappings++;
    if (type->bit_size != -1 && type->mappings->default_bit_size != -1 &&
        mapping->length != (type->mappings->default_bit_size >> 3))
    {
        type->bit_size = -1;
    }

    return 0;
}

static int mapping_type_set_bit_size(coda_asciiMappingsType *type, int64_t bit_size)
{
    if (type->mappings != NULL)
    {
        int i;

        if (type->mappings->default_bit_size != -1)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "ascii type already has a size");
            return -1;
        }
        type->mappings->default_bit_size = bit_size;
        type->bit_size = bit_size;
        for (i = 0; i < type->mappings->num_mappings; i++)
        {
            if (type->mappings->mapping[i]->length != (bit_size >> 3))
            {
                type->bit_size = -1;
                break;
            }
        }
    }
    else
    {
        if (type->bit_size != -1)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "ascii type already has a size");
            return -1;
        }
        type->bit_size = bit_size;
    }

    return 0;
}


coda_asciiInteger *coda_ascii_integer_new(void)
{
    coda_asciiInteger *integer;

    integer = (coda_asciiInteger *)malloc(sizeof(coda_asciiInteger));
    if (integer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiInteger), __FILE__, __LINE__);
        return NULL;
    }
    integer->retain_count = 0;
    integer->format = coda_format_ascii;
    integer->type_class = coda_integer_class;
    integer->name = NULL;
    integer->description = NULL;
    integer->tag = tag_ascii_integer;
    integer->bit_size = -1;
    integer->mappings = NULL;
    integer->unit = NULL;
    integer->read_type = coda_native_type_not_available;
    integer->conversion = NULL;

    return integer;
}

int coda_ascii_integer_set_unit(coda_asciiInteger *integer, const char *unit)
{
    return number_set_unit((coda_asciiNumber *)integer, unit);
}

int coda_ascii_integer_set_byte_size(coda_asciiInteger *integer, long byte_size)
{
    if (byte_size <= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "byte size may not be <= 0 for ascii integer definition");
        return -1;
    }
    return mapping_type_set_bit_size((coda_asciiMappingsType *)integer, byte_size << 3);
}

int coda_ascii_integer_set_read_type(coda_asciiInteger *integer, coda_native_type read_type)
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
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for ascii integer definition",
                       coda_type_get_native_type_name(read_type));
        return -1;
    }
    integer->read_type = read_type;
    return 0;
}

int coda_ascii_integer_set_conversion(coda_asciiInteger *integer, coda_Conversion *conversion)
{
    if (integer->conversion != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "integer already has a conversion");
        return -1;
    }
    integer->conversion = conversion;
    return 0;
}

int coda_ascii_integer_add_mapping(coda_asciiInteger *integer, coda_asciiIntegerMapping *mapping)
{
    return mapping_type_add_mapping((coda_asciiMappingsType *)integer, (coda_asciiMapping *)mapping);
}

int coda_ascii_integer_validate(coda_asciiInteger *integer)
{
    if (integer->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for ascii integer definition");
        return -1;
    }
    return 0;
}

coda_asciiFloat *coda_ascii_float_new(void)
{
    coda_asciiFloat *fl;

    fl = (coda_asciiFloat *)malloc(sizeof(coda_asciiFloat));
    if (fl == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiFloat), __FILE__, __LINE__);
        return NULL;
    }
    fl->retain_count = 0;
    fl->format = coda_format_ascii;
    fl->type_class = coda_real_class;
    fl->name = NULL;
    fl->description = NULL;
    fl->tag = tag_ascii_float;
    fl->bit_size = -1;
    fl->mappings = NULL;
    fl->unit = NULL;
    fl->read_type = coda_native_type_not_available;
    fl->conversion = NULL;

    return fl;
}

int coda_ascii_float_set_unit(coda_asciiFloat *fl, const char *unit)
{
    return number_set_unit((coda_asciiNumber *)fl, unit);
}

int coda_ascii_float_set_byte_size(coda_asciiFloat *fl, long byte_size)
{
    if (byte_size <= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "byte size may not be <= 0 for ascii float definition");
        return -1;
    }
    return mapping_type_set_bit_size((coda_asciiMappingsType *)fl, byte_size << 3);
}

int coda_ascii_float_set_read_type(coda_asciiFloat *fl, coda_native_type read_type)
{
    if (fl->read_type != coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "float already has a read type");
        return -1;
    }
    if (read_type != coda_native_type_float && read_type != coda_native_type_double)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for ascii float definition",
                       coda_type_get_native_type_name(read_type));
        return -1;
    }
    fl->read_type = read_type;
    return 0;
}

int coda_ascii_float_set_conversion(coda_asciiFloat *fl, coda_Conversion *conversion)
{
    if (fl->conversion != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "float already has a conversion");
        return -1;
    }
    fl->conversion = conversion;
    return 0;
}

int coda_ascii_float_add_mapping(coda_asciiFloat *fl, coda_asciiFloatMapping *mapping)
{
    return mapping_type_add_mapping((coda_asciiMappingsType *)fl, (coda_asciiMapping *)mapping);
}

int coda_ascii_float_validate(coda_asciiFloat *fl)
{
    if (fl->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for ascii float definition");
        return -1;
    }
    return 0;
}

coda_asciiText *coda_ascii_text_new(void)
{
    coda_asciiText *text;

    text = (coda_asciiText *)malloc(sizeof(coda_asciiText));
    if (text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiText), __FILE__, __LINE__);
        return NULL;
    }
    text->retain_count = 0;
    text->format = coda_format_ascii;
    text->type_class = coda_text_class;
    text->name = NULL;
    text->description = NULL;
    text->tag = tag_ascii_text;
    text->bit_size = -1;
    text->mappings = NULL;
    text->read_type = coda_native_type_not_available;
    text->byte_size_expr = NULL;
    text->fixed_value = NULL;

    return text;
}

int coda_ascii_text_set_byte_size(coda_asciiText *text, int64_t byte_size)
{
    if (text->byte_size_expr != NULL || text->bit_size != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text already has a byte size");
        return -1;
    }
    if (byte_size <= 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "byte size may not be <= 0 for text definition");
        return -1;
    }
    text->bit_size = (byte_size << 3);
    return 0;
}

int coda_ascii_text_set_byte_size_expression(coda_asciiText *text, coda_Expr *byte_size_expr)
{
    if (text->byte_size_expr != NULL || text->bit_size != -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text already has a byte size");
        return -1;
    }
    assert(byte_size_expr != NULL);
    text->byte_size_expr = byte_size_expr;
    return 0;
}

int coda_ascii_text_set_read_type(coda_asciiText *text, coda_native_type read_type)
{
    if (text->read_type != coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text already has a read type");
        return -1;
    }
    if (read_type != coda_native_type_char && read_type != coda_native_type_string)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid read type (%s) for text definition",
                       coda_type_get_native_type_name(read_type));
        return -1;
    }
    text->read_type = read_type;
    return 0;
}

int coda_ascii_text_set_fixed_value(coda_asciiText *text, const char *fixed_value)
{
    char *new_fixed_value = NULL;

    if (text->fixed_value != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "text already has a fixed value");
        return -1;
    }
    if (fixed_value != NULL)
    {
        new_fixed_value = strdup(fixed_value);
        if (new_fixed_value == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            return -1;
        }
    }
    text->fixed_value = new_fixed_value;

    return 0;
}

int coda_ascii_text_validate(coda_asciiText *text)
{
    if (text->byte_size_expr == NULL && text->bit_size == -1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing byte size or byte size expression for text definition");
        return -1;
    }
    if (text->read_type == coda_native_type_not_available)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing read type for text definition");
        return -1;
    }
    if (text->bit_size == -1 && text->fixed_value != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "byte size should be fixed if a fixed value is provided for text definition");
        return -1;
    }
    /* if there is a fixed_value its length should equal the byte size of the data element */
    if (text->fixed_value != NULL && text->bit_size != 8 * (int64_t)strlen(text->fixed_value))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "byte size of fixed value (%ld) should equal byte size (%ld) for text definition",
                       8 * strlen(text->fixed_value), (long)text->bit_size);
        return -1;
    }
    return 0;
}

coda_asciiLineSeparator *coda_ascii_line_separator_new(void)
{
    coda_asciiLineSeparator *text;

    text = (coda_asciiLineSeparator *)malloc(sizeof(coda_asciiLineSeparator));
    if (text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiLineSeparator), __FILE__, __LINE__);
        return NULL;
    }
    text->retain_count = 0;
    text->format = coda_format_ascii;
    text->type_class = coda_text_class;
    text->name = NULL;
    text->description = NULL;
    text->tag = tag_ascii_line_separator;
    text->bit_size = -1;

    return text;
}

coda_asciiLine *coda_ascii_line_new(int include_eol)
{
    coda_asciiLine *text;

    text = (coda_asciiLine *)malloc(sizeof(coda_asciiLine));
    if (text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiLine), __FILE__, __LINE__);
        return NULL;
    }
    text->retain_count = 0;
    text->format = coda_format_ascii;
    text->type_class = coda_text_class;
    text->name = NULL;
    text->description = NULL;
    text->tag = tag_ascii_line;
    text->bit_size = -1;
    text->include_eol = include_eol;

    return text;
}

coda_asciiWhiteSpace *coda_ascii_white_space_new(void)
{
    coda_asciiWhiteSpace *text;

    text = (coda_asciiWhiteSpace *)malloc(sizeof(coda_asciiWhiteSpace));
    if (text == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiWhiteSpace), __FILE__, __LINE__);
        return NULL;
    }
    text->retain_count = 0;
    text->format = coda_format_ascii;
    text->type_class = coda_text_class;
    text->name = NULL;
    text->description = NULL;
    text->tag = tag_ascii_white_space;
    text->bit_size = -1;

    return text;
}

coda_asciiTime *coda_ascii_time_new(const char *format)
{
    coda_asciiTime *time;
    coda_asciiText *type;

    time = (coda_asciiTime *)malloc(sizeof(coda_asciiTime));
    if (time == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiTime), __FILE__, __LINE__);
        return NULL;
    }
    time->retain_count = 0;
    time->format = coda_format_ascii;
    time->type_class = coda_special_class;
    time->name = NULL;
    time->description = NULL;
    time->tag = tag_ascii_time;
    time->base_type = NULL;
    time->time_type = 0;

    type = coda_ascii_text_new();
    coda_ascii_text_set_read_type(type, coda_native_type_string);
    if (strcmp(format, "ascii_envisat_datetime") == 0)
    {
        time->time_type = ascii_envisat_datetime;
        coda_type_set_description((coda_Type *)type, "ENVISAT ASCII datetime \"DD-MMM-YYYY hh:mm:ss.uuuuuu\".");
        coda_ascii_text_set_byte_size(type, 27);
    }
    else if (strcmp(format, "ascii_gome_datetime") == 0)
    {
        time->time_type = ascii_gome_datetime;
        coda_type_set_description((coda_Type *)type, "GOME ASCII datetime \"DD-MMM-YYYY hh:mm:ss.uuu\".");
        coda_ascii_text_set_byte_size(type, 24);
    }
    else if (strcmp(format, "ascii_eps_datetime") == 0)
    {
        time->time_type = ascii_eps_datetime;
        coda_type_set_description((coda_Type *)type, "EPS generalised time \"YYYYMMDDHHMMSSZ\".");
        coda_ascii_text_set_byte_size(type, 15);
    }
    else if (strcmp(format, "ascii_eps_datetime_long") == 0)
    {
        time->time_type = ascii_eps_datetime_long;
        coda_type_set_description((coda_Type *)type, "EPS long generalised time \"YYYYMMDDHHMMSSmmmZ\".");
        coda_ascii_text_set_byte_size(type, 18);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_ymd1") == 0)
    {
        time->time_type = ascii_ccsds_datetime_ymd1;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime \"YYYY-MM-DDThh:mm:ss\".");
        coda_ascii_text_set_byte_size(type, 19);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_ymd1_with_ref") == 0)
    {
        time->time_type = ascii_ccsds_datetime_ymd1_with_ref;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime with time reference "
                                  "\"RRR=YYYY-MM-DDThh:mm:ss\". The reference RRR can be any of \"UT1\", \"UTC\", "
                                  "\"TAI\", or \"GPS\".");
        coda_ascii_text_set_byte_size(type, 23);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_ymd2") == 0)
    {
        time->time_type = ascii_ccsds_datetime_ymd2;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime \"YYYY-MM-DDThh:mm:ss.uuuuuu\".");
        coda_ascii_text_set_byte_size(type, 26);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_ymd2_with_ref") == 0)
    {
        time->time_type = ascii_ccsds_datetime_ymd2_with_ref;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime with time reference "
                                  "\"RRR=YYYY-MM-DDThh:mm:ss.uuuuuu\". The reference RRR can be any of \"UT1\", "
                                  "\"UTC\", \"TAI\", or \"GPS\".");
        coda_ascii_text_set_byte_size(type, 30);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_utc1") == 0)
    {
        time->time_type = ascii_ccsds_datetime_utc1;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime \"YYYY-DDDThh:mm:ss\".");
        coda_ascii_text_set_byte_size(type, 17);
    }
    else if (strcmp(format, "ascii_ccsds_datetime_utc2") == 0)
    {
        time->time_type = ascii_ccsds_datetime_utc2;
        coda_type_set_description((coda_Type *)type, "CCSDS ASCII datetime \"YYYY-DDDThh:mm:ss.uuuuuu\". "
                                  "Microseconds can be written using less digits (1-6 digits): e.g.: "
                                  "\"YYYY-DDDThh:mm:ss.u     \"");
        coda_ascii_text_set_byte_size(type, 24);
    }
    else
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid time format (%s) for ascii time definition", format);
        delete_ascii_time(time);
        return NULL;
    }
    time->base_type = (coda_asciiType *)type;

    /* set bit_size */
    time->bit_size = time->base_type->bit_size;

    return time;
}

int coda_ascii_time_add_mapping(coda_asciiTime *time, coda_asciiFloatMapping *mapping)
{
    if (mapping_type_add_mapping((coda_asciiMappingsType *)time->base_type, (coda_asciiMapping *)mapping) != 0)
    {
        return -1;
    }

    /* update bit_size */
    time->bit_size = time->base_type->bit_size;

    return 0;
}

coda_asciiIntegerMapping *coda_ascii_integer_mapping_new(const char *str, int64_t value)
{
    coda_asciiIntegerMapping *mapping;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty string value (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    mapping = (coda_asciiIntegerMapping *)malloc(sizeof(coda_asciiIntegerMapping));
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiIntegerMapping), __FILE__, __LINE__);
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
    mapping->length = strlen(str);

    return mapping;
}

void coda_ascii_integer_mapping_delete(coda_asciiIntegerMapping *mapping)
{
    delete_mapping((coda_asciiMapping *)mapping);
}

coda_asciiFloatMapping *coda_ascii_float_mapping_new(const char *str, double value)
{
    coda_asciiFloatMapping *mapping;

    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "empty string value (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    mapping = (coda_asciiFloatMapping *)malloc(sizeof(coda_asciiFloatMapping));
    if (mapping == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(coda_asciiFloatMapping), __FILE__, __LINE__);
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
    mapping->length = strlen(str);

    return mapping;
}

void coda_ascii_float_mapping_delete(coda_asciiFloatMapping *mapping)
{
    delete_mapping((coda_asciiMapping *)mapping);
}

void coda_ascii_done(void)
{
    coda_ascbin_done();
}
