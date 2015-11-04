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

#include "coda-ascii-internal.h"

#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-definition.h"
#include "coda-ascii-definition.h"

#ifndef MAXINT8
#define MAXINT8 0x7F
#endif
#ifndef MAXUINT8
#define MAXUINT8 0xFFu
#endif
#ifndef MAXINT16
#define MAXINT16 0x7FFF
#endif
#ifndef MAXUINT16
#define MAXUINT16 0xFFFFu
#endif
#ifndef MAXINT32
#define MAXINT32 0x7FFFFFFFl
#endif
#ifndef MAXUINT32
#define MAXUINT32 0xFFFFFFFFul
#endif
#ifndef MAXINT64
#define MAXINT64 ((((int64_t)MAXINT32) << 32) | (int64_t)MAXUINT32)
#endif
#ifndef MAXUINT64
#define MAXUINT64 ((((uint64_t)MAXUINT32) << 32) | (uint64_t)MAXUINT32)
#endif

#define MAXBUFFERSIZE 64


typedef int (*read_function) (const coda_cursor *, void *, int64_t);

/* calculates a * 10 ^ b, with a of type double and b of type long */
static double a_pow10_b(double a, long b)
{
    register long i = labs(b);
    double val = 1.0;

    while (--i >= 0)
    {
        val *= 10;
    }

    if (b < 0)
    {
        return a / val;
    }
    return a * val;
}

static int parse_mapping_size(const char *buffer, long buffer_length, coda_ascii_mappings *mappings, int64_t *bit_size)
{
    int i;

    for (i = 0; i < mappings->num_mappings; i++)
    {
        if (mappings->mapping[i]->length == 0)
        {
            if (buffer_length == 0)
            {
                *bit_size = 0;
                return 1;
            }
        }
        else if (mappings->mapping[i]->length <= buffer_length &&
                 memcmp(mappings->mapping[i]->str, buffer, mappings->mapping[i]->length) == 0)
        {
            *bit_size = (mappings->mapping[i]->length << 3);
            return 1;
        }
    }
    if (mappings->default_bit_size != -1)
    {
        *bit_size = mappings->default_bit_size;
        return 1;
    }

    return 0;
}

static int parse_integer_mapping(const char *buffer, long buffer_length, coda_ascii_mappings *mappings,
                                 int dynamic_size, int64_t *dst)
{
    int i;

    for (i = 0; i < mappings->num_mappings; i++)
    {
        if (mappings->mapping[i]->length == 0)
        {
            if (buffer_length == 0)
            {
                *dst = ((coda_ascii_integer_mapping *)mappings->mapping[i])->value;
                return 1;
            }
        }
        else if (mappings->mapping[i]->length <= buffer_length &&
                 memcmp(mappings->mapping[i]->str, buffer, mappings->mapping[i]->length) == 0)
        {
            if (!dynamic_size && mappings->mapping[i]->length != buffer_length)
            {
                coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii integer");
                return -1;
            }
            *dst = ((coda_ascii_integer_mapping *)mappings->mapping[i])->value;
            return 1;
        }
    }

    return 0;
}

static int parse_float_mapping(const char *buffer, long buffer_length, coda_ascii_mappings *mappings, int dynamic_size,
                               double *dst)
{
    int i;

    for (i = 0; i < mappings->num_mappings; i++)
    {
        if (mappings->mapping[i]->length == 0)
        {
            if (buffer_length == 0)
            {
                *dst = ((coda_ascii_float_mapping *)mappings->mapping[i])->value;
                return 1;
            }
        }
        else if (mappings->mapping[i]->length <= buffer_length &&
                 memcmp(mappings->mapping[i]->str, buffer, mappings->mapping[i]->length) == 0)
        {
            if (!dynamic_size && mappings->mapping[i]->length != buffer_length)
            {
                coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii float");
                return -1;
            }
            *dst = ((coda_ascii_float_mapping *)mappings->mapping[i])->value;
            return 1;
        }
    }

    return 0;
}

long coda_ascii_parse_int64(const char *buffer, long buffer_length, int64_t *dst, int ignore_trailing_bytes)
{
    long length;
    int integer_length;
    int64_t value;
    int negative = 0;

    length = buffer_length;

    while (length > 0 && *buffer == ' ')
    {
        buffer++;
        length--;
    }

    if (*buffer == '+' || *buffer == '-')
    {
        negative = (*buffer == '-');
        buffer++;
        length--;
    }

    value = 0;
    integer_length = 0;
    while (length > 0)
    {
        int64_t digit;

        if (*buffer < '0' || *buffer > '9')
        {
            break;
        }
        digit = *buffer - '0';
        if (value > (MAXINT64 - digit) / 10)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "value too large for ascii integer");
            return -1;
        }
        value = 10 * value + digit;
        integer_length++;
        buffer++;
        length--;
    }
    if (integer_length == 0)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii integer (no digits)");
        return -1;
    }
    if (!ignore_trailing_bytes && length != 0)
    {
        while (length > 0 && *buffer == ' ')
        {
            buffer++;
            length--;
        }
        if (length != 0)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii integer");
            return -1;
        }
    }

    if (negative)
    {
        value = -value;
    }

    *dst = value;

    return buffer_length - length;
}

long coda_ascii_parse_uint64(const char *buffer, long buffer_length, uint64_t *dst, int ignore_trailing_bytes)
{
    long length;
    int integer_length;
    uint64_t value;

    length = buffer_length;

    while (length > 0 && *buffer == ' ')
    {
        buffer++;
        length--;
    }

    if (*buffer == '+')
    {
        buffer++;
        length--;
    }

    value = 0;
    integer_length = 0;
    while (length > 0)
    {
        int64_t digit;

        if (*buffer < '0' || *buffer > '9')
        {
            break;
        }
        digit = *buffer - '0';
        if (value > (MAXUINT64 - digit) / 10)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "value too large for ascii integer");
            return -1;
        }
        value = 10 * value + digit;
        integer_length++;
        buffer++;
        length--;
    }
    if (integer_length == 0)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii integer (no digits)");
        return -1;
    }
    if (!ignore_trailing_bytes && length != 0)
    {
        while (length > 0 && *buffer == ' ')
        {
            buffer++;
            length--;
        }
        if (length != 0)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii integer");
            return -1;
        }
    }

    *dst = value;

    return buffer_length - length;
}

long coda_ascii_parse_double(const char *buffer, long buffer_length, double *dst, int ignore_trailing_bytes)
{
    long length;
    int value_length;
    int exponent_length;
    int has_sign;
    double value;
    long exponent;
    int negative = 0;

    length = buffer_length;

    while (length > 0 && *buffer == ' ')
    {
        buffer++;
        length--;
    }

    has_sign = 0;
    if (length > 0)
    {
        if (*buffer == '+' || *buffer == '-')
        {
            negative = (*buffer == '-');
            has_sign = 1;
            buffer++;
            length--;
        }
    }

    /* check for NaN/Inf */
    if (length >= 3)
    {
        if ((buffer[0] == 'N' || buffer[0] == 'n') && (buffer[1] == 'A' || buffer[1] == 'a') &&
            (buffer[2] == 'N' || buffer[2] == 'n') && !has_sign)
        {
            length -= 3;
            if (!ignore_trailing_bytes && length != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii floating point value");
                return -1;
            }
            *dst = coda_NaN();
            return buffer_length - length;
        }
        else if ((buffer[0] == 'I' || buffer[0] == 'i') && (buffer[1] == 'N' || buffer[1] == 'n') &&
                 (buffer[2] == 'F' || buffer[2] == 'f'))
        {
            length -= 3;
            if (!ignore_trailing_bytes && length != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii floating point value");
                return -1;
            }
            *dst = negative ? coda_MinInf() : coda_PlusInf();
            return buffer_length - length;
        }
    }

    value = 0;
    exponent = 0;
    value_length = 0;
    /* read mantissa part before the digit */
    while (length > 0)
    {
        if (*buffer < '0' || *buffer > '9')
        {
            break;
        }
        value = 10 * value + (*buffer - '0');
        value_length++;
        buffer++;
        length--;
    }
    /* read digit and mantissa part after the digit */
    if (length > 0)
    {
        if (*buffer == '.')
        {
            buffer++;
            length--;
            while (length > 0)
            {
                if (*buffer < '0' || *buffer > '9')
                {
                    break;
                }
                value = 10 * value + (*buffer - '0');
                exponent--;
                value_length++;
                buffer++;
                length--;
            }
        }
    }
    if (value_length == 0)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii floating point value (no digits)");
        return -1;
    }

    if (negative)
    {
        value = -value;
    }
    /* read exponent part */
    if (length > 0 && (*buffer == 'd' || *buffer == 'D' || *buffer == 'e' || *buffer == 'E'))
    {
        long exponent_value;

        buffer++;
        length--;
        negative = 0;
        if (length > 0)
        {
            if (*buffer == '+' || *buffer == '-')
            {
                negative = (*buffer == '-');
                buffer++;
                length--;
            }
        }
        exponent_value = 0;
        exponent_length = 0;
        while (length > 0)
        {
            if (*buffer < '0' || *buffer > '9')
            {
                break;
            }
            exponent_value = 10 * exponent_value + (*buffer - '0');
            exponent_length++;
            buffer++;
            length--;
        }
        if (exponent_length == 0)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT,
                           "invalid format for ascii floating point value (empty exponent value)");
            return -1;
        }
        if (negative)
        {
            exponent_value = -exponent_value;
        }
        exponent += exponent_value;
    }

    if (!ignore_trailing_bytes && length != 0)
    {
        while (length > 0 && *buffer == ' ')
        {
            buffer++;
            length--;
        }
        if (length != 0)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "invalid format for ascii floating point value");
            return -1;
        }
    }

    if (exponent != 0)
    {
        value = a_pow10_b(value, exponent);
    }

    *dst = value;

    return buffer_length - length;
}

static int parse_envisat_datetime(char *buffer, long buffer_length, double *dst)
{
    char month[3];
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MUSEC;
    int result;
    int n;

    if (buffer_length < 27)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "DD-MMM-YYYY hh:mm:ss.uuuuuu" */

    buffer[27] = '\0';
    n = 0;
    result = sscanf(buffer, "%2d-%c%c%c-%4d %2d:%2d:%2d.%6d%n", &DAY, &month[0], &month[1], &month[2], &YEAR, &HOUR,
                    &MINUTE, &SECOND, &MUSEC, &n);
    if (result != 9 || n != 27)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }
    MONTH = coda_month_to_integer(month);
    if (MONTH == -1)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect month value", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC, dst);
}

static int parse_gome_datetime(char *buffer, long buffer_length, double *dst)
{
    char month[3];
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MSEC;
    int result;
    int n;

    if (buffer_length < 24)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "DD-MMM-YYYY hh:mm:ss.uuu" */

    buffer[24] = '\0';
    n = 0;
    result = sscanf(buffer, "%2d-%c%c%c-%4d %2d:%2d:%2d.%3d%n", &DAY, &month[0], &month[1], &month[2], &YEAR, &HOUR,
                    &MINUTE, &SECOND, &MSEC, &n);
    if (result != 9 || n != 24)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }
    MONTH = coda_month_to_integer(month);
    if (MONTH == -1)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect month value", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 1000 * MSEC, dst);
}

static int parse_eps_datetime(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND;
    int result;
    int n;

    if (buffer_length < 15)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYYMMDDhhmmssZ" */

    buffer[15] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d%2d%2d%2d%2d%2dZ%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &n);
    if (result != 6 || n != 15)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 0, dst);
}

static int parse_eps_datetime_long(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MSEC;
    int result;
    int n;

    if (buffer_length < 18)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYYMMDDhhmmssuuu" */

    buffer[18] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d%2d%2d%2d%2d%2d%3dZ%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &MSEC, &n);
    if (result != 7 || n != 18)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 1000 * MSEC, dst);
}

static int parse_ccsds_datetime_ymd1(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND;
    int result;
    int n;

    if (buffer_length < 19)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYY-MM-DDThh:mm:ss" */

    buffer[19] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d-%2d-%2dT%2d:%2d:%2d%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &n);
    if (result != 6 || n != 19)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 0, dst);
}

static int parse_ccsds_datetime_ymd1_with_ref(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND;
    int result;
    int n;

    if (buffer_length < 23)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "RRR=YYYY-MM-DDThh:mm:ss" */

    if (memcmp(buffer, "UT1=", 4) != 0 && memcmp(buffer, "UTC=", 4) != 0 && memcmp(buffer, "TAI=", 4) != 0 &&
        memcmp(buffer, "GPS=", 4) != 0)
    {
        buffer[4] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument has an invalid time reference (%s)", buffer);
        return -1;
    }

    buffer[23] = '\0';
    n = 0;
    result = sscanf(&buffer[4], "%4d-%2d-%2dT%2d:%2d:%2d%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &n);
    if (result != 6 || n != 19)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 0, dst);
}

static int parse_ccsds_datetime_ymd2(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MUSEC;
    int result;
    int n;

    if (buffer_length < 26)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYY-MM-DDThh:mm:ss.uuuuuu" */

    buffer[26] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d-%2d-%2dT%2d:%2d:%2d.%6d%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &MUSEC, &n);
    if (result != 7 || n != 26)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC, dst);
}

static int parse_ccsds_datetime_ymd2_with_ref(char *buffer, long buffer_length, double *dst)
{
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MUSEC;
    int result;
    int n;

    if (buffer_length < 30)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "RRR=YYYY-MM-DDThh:mm:ss.uuuuuu" */

    if (memcmp(buffer, "UT1=", 4) != 0 && memcmp(buffer, "UTC=", 4) != 0 && memcmp(buffer, "TAI=", 4) != 0 &&
        memcmp(buffer, "GPS=", 4) != 0)
    {
        buffer[4] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument has an invalid time reference (%s)", buffer);
        return -1;
    }

    buffer[30] = '\0';
    n = 0;
    result = sscanf(&buffer[4], "%4d-%2d-%2dT%2d:%2d:%2d.%6d%n", &YEAR, &MONTH, &DAY, &HOUR, &MINUTE, &SECOND, &MUSEC,
                    &n);
    if (result != 7 || n != 26)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC, dst);
}

static int parse_ccsds_datetime_utc1(char *buffer, long buffer_length, double *dst)
{
    int DAY_OF_YEAR;
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND;
    int result;
    int n;

    if (buffer_length < 17)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYY-DDDThh:mm:ss" */

    buffer[17] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d-%3dT%2d:%2d:%2d%n", &YEAR, &DAY_OF_YEAR, &HOUR, &MINUTE, &SECOND, &n);
    if (result != 5 || n != 17)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }
    if (coda_dayofyear_to_month_day(YEAR, DAY_OF_YEAR, &MONTH, &DAY) != 0)
    {
        return -1;
    }
    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, 0, dst);
}

static int parse_ccsds_datetime_utc2(char *buffer, long buffer_length, double *dst)
{
    int DAY_OF_YEAR;
    int DAY, MONTH, YEAR, HOUR, MINUTE, SECOND, MUSEC;
    int result;
    int n;

    if (buffer_length < 24)
    {
        buffer[buffer_length] = '\0';
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }

    /* format: "YYYY-DDDThh:mm:ss.uuuuuu"
     * microseconds can be written using less digits (1-6 digits): e.g.: "YYYY-DDDThh:mm:ss.u     "
     */

    buffer[24] = '\0';
    n = 0;
    result = sscanf(buffer, "%4d-%3dT%2d:%2d:%2d.%6d%n", &YEAR, &DAY_OF_YEAR, &HOUR, &MINUTE, &SECOND, &MUSEC, &n);
    if (result != 6 || n < 19)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
        return -1;
    }
    if (n < 24)
    {
        int i;

        if (memcmp(&buffer[n], "     ", 24 - n) != 0)
        {
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "date/time argument (%s) has an incorrect format", buffer);
            return -1;
        }
        for (i = n; i < 24; i++)
        {
            MUSEC *= 10;
        }
    }
    if (coda_dayofyear_to_month_day(YEAR, DAY_OF_YEAR, &MONTH, &DAY) != 0)
    {
        return -1;
    }
    return coda_datetime_to_double(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC, dst);
}

static int read_bytes_in_bounds(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    coda_ascbin_product *product_file = (coda_ascbin_product *)product;

    if (product_file->use_mmap)
    {
        memcpy(dst, product_file->mmap_ptr + byte_offset, (size_t)length);
    }
    else
    {
#if HAVE_PREAD
        if (pread(product_file->fd, dst, (size_t)length, (off_t)byte_offset) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#else
        if (lseek(product_file->fd, (off_t)byte_offset, SEEK_SET) < 0)
        {
            char byte_offset_str[21];

            coda_str64(byte_offset, byte_offset_str);
            coda_set_error(CODA_ERROR_FILE_READ, "could not move to byte position %s in file %s (%s)",
                           byte_offset_str, product_file->filename, strerror(errno));
            return -1;
        }
        if (read(product_file->fd, dst, (size_t)length) < 0)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not read from file %s (%s)", product_file->filename,
                           strerror(errno));
            return -1;
        }
#endif
    }

    return 0;
}

static int read_bytes(coda_product *product, int64_t byte_offset, int64_t length, void *dst)
{
    if (((uint64_t)byte_offset + length) > ((uint64_t)product->file_size))
    {
        coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
        return -1;
    }
    return read_bytes_in_bounds(product, byte_offset, length, dst);
}

static long get_buffer_size(int64_t bit_size, int64_t size_boundary, int64_t remaining_bits, int *dynamic_size)
{
    if (bit_size >= 0)
    {
        if (bit_size > remaining_bits)
        {
            coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read beyond the end of the file");
            return -1;
        }
        if (size_boundary >= 0 && bit_size > size_boundary)
        {
            char bs[21];
            char sb[21];
            
            coda_str64(bit_size, bs);
            coda_str64(size_boundary, sb);
            coda_set_error(CODA_ERROR_PRODUCT, "trying to read %s bits from %s bit buffer", bs, sb);
            return -1;
        }
        *dynamic_size = 0;
        return (long)(bit_size >> 3);
    }

    *dynamic_size = 1;
    if (size_boundary == -1 || size_boundary > remaining_bits)
    {
        size_boundary = remaining_bits;
    }
    size_boundary >>= 3;
    if (size_boundary < MAXBUFFERSIZE)
    {
        return (long)size_boundary;
    }
    return MAXBUFFERSIZE;
}

int coda_ascii_cursor_set_asciilines(coda_cursor *cursor, coda_product *product)
{
    coda_ascbin_product *product_file = (coda_ascbin_product *)product;

    if (product_file->asciiline_end_offset == NULL)
    {
        if (coda_ascii_init_asciilines(product) != 0)
        {
            return -1;
        }
    }

    if (product_file->asciilines == NULL)
    {
        coda_ascbin_array *array;
        coda_ascii_line *asciiline;

        array = coda_ascbin_array_new(coda_format_ascii);
        if (array == NULL)
        {
            return -1;
        }
        if (coda_ascbin_array_add_fixed_dimension(array, product_file->num_asciilines) != 0)
        {
            coda_release_type((coda_type *)array);
            return -1;
        }
        asciiline = coda_ascii_line_new(1);
        if (asciiline == NULL)
        {
            coda_release_type((coda_type *)array);
            return -1;
        }
        if (coda_ascbin_array_set_base_type(array, (coda_ascbin_type *)asciiline) != 0)
        {
            coda_release_type((coda_type *)array);
            coda_release_type((coda_type *)asciiline);
            return -1;
        }

        product_file->asciilines = (coda_dynamic_type *)array;
    }

    cursor->product = product;
    cursor->n = 1;
    cursor->stack[0].type = product_file->asciilines;
    cursor->stack[0].index = -1;        /* there is no index for the root of the product */
    cursor->stack[0].bit_offset = 0;
    return 0;
}

int coda_ascii_cursor_use_base_type_of_special_type(coda_cursor *cursor)
{
    coda_ascii_special_type *type;

    type = (coda_ascii_special_type *)cursor->stack[cursor->n - 1].type;
    cursor->stack[cursor->n - 1].type = (coda_dynamic_type *)type->base_type;

    return 0;
}

int coda_ascii_cursor_get_string_length(const coda_cursor *cursor, long *length, int64_t size_boundary)
{
    int64_t bit_size;

    if (coda_ascii_cursor_get_bit_size(cursor, &bit_size, size_boundary) != 0)
    {
        return -1;
    }
    if (bit_size < 0)
    {
        *length = -1;
    }
    else
    {
        *length = (long)(bit_size >> 3);
    }

    return 0;
}

int coda_ascii_cursor_get_bit_size(const coda_cursor *cursor, int64_t *bit_size, int64_t size_boundary)
{
    coda_ascii_type *type = (coda_ascii_type *)cursor->stack[cursor->n - 1].type;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    char buffer[MAXBUFFERSIZE];
    long buffer_size = -1;
    int dynamic_size;

    if (type->bit_size >= 0)
    {
        *bit_size = type->bit_size;
        return 0;
    }

    switch (type->tag)
    {
        case tag_ascii_record:
        case tag_ascii_union:
        case tag_ascii_array:
            return coda_ascbin_cursor_get_bit_size(cursor, bit_size);
        case tag_ascii_integer:
        case tag_ascii_float:
        case tag_ascii_text:
            {
                buffer_size = get_buffer_size(-1, size_boundary, (cursor->product->file_size << 3) - bit_offset,
                                              &dynamic_size);
                if (buffer_size < 0)
                {
                    return -1;
                }
                if (read_bytes_in_bounds(cursor->product, bit_offset >> 3, buffer_size, buffer) != 0)
                {
                    return -1;
                }

                if (((coda_ascii_mappings_type *)type)->mappings != NULL)
                {
                    if (parse_mapping_size(buffer, buffer_size, ((coda_ascii_mappings_type *)type)->mappings, bit_size))
                    {
                        return 0;
                    }
                }
            }
            break;
        case tag_ascii_line_separator:
        case tag_ascii_line:
        case tag_ascii_white_space:
            break;
        case tag_ascii_time:
            {
                coda_cursor spec_cursor;

                spec_cursor = *cursor;
                spec_cursor.stack[spec_cursor.n - 1].type =
                    (coda_dynamic_type *)((coda_ascii_special_type *)type)->base_type;
                return coda_ascii_cursor_get_bit_size(&spec_cursor, bit_size, size_boundary);
            }
    }

    switch (type->tag)
    {
        case tag_ascii_integer:
            {
                int64_t value;
                long size;

                switch (((coda_ascii_integer *)type)->read_type)
                {
                    case coda_native_type_int8:
                    case coda_native_type_int16:
                    case coda_native_type_int32:
                    case coda_native_type_int64:
                        size = coda_ascii_parse_int64(buffer, buffer_size, &value, 1);
                        break;
                    case coda_native_type_uint8:
                    case coda_native_type_uint16:
                    case coda_native_type_uint32:
                    case coda_native_type_uint64:
                        size = coda_ascii_parse_uint64(buffer, buffer_size, (uint64_t *)&value, 1);
                        break;
                    default:
                        assert(0);
                        exit(1);
                }
                if (size < 0)
                {
                    return -1;
                }
                *bit_size = (size << 3);
            }
            break;
        case tag_ascii_float:
            {
                double value;
                long size;

                size = coda_ascii_parse_double(buffer, buffer_size, &value, 1);
                if (size < 0)
                {
                    return -1;
                }
                *bit_size = (size << 3);
            }
            break;
        case tag_ascii_text:
            if (coda_expression_eval_integer(((coda_ascii_text *)type)->byte_size_expr, cursor, bit_size) != 0)
            {
                return -1;
            }
            /* convert byte size to bit size */
            *bit_size *= 8;
            break;
        case tag_ascii_line_separator:
            switch (((coda_ascbin_product *)cursor->product)->end_of_line)
            {
                case eol_lf:
                case eol_cr:
                    *bit_size = 8;
                    break;
                case eol_crlf:
                    *bit_size = 16;
                    break;
                case eol_unknown:
                    {
                        uint8_t buffer[1];

                        /* accept either LINEFEED, CARRIAGE_RETURN, or CARRIAGE-RETURN/LINEFEED */

                        if ((cursor->stack[cursor->n - 1].bit_offset & 0x7) != 0)
                        {
                            coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii line separator does "
                                           "not start at byte boundary)");
                            return -1;
                        }
                        if (coda_ascii_cursor_read_bytes(cursor, buffer, 0, 1) != 0)
                        {
                            return -1;
                        }

                        switch (*buffer)
                        {
                            case '\n': /* linefeed */
                                /* just a linefeed -> unix platform convention */
                                *bit_size = 8;
                                ((coda_ascbin_product *)cursor->product)->end_of_line = eol_lf;
                                break;
                            case '\r': /* carriage return */
                                if (cursor->product->file_size - (cursor->stack[cursor->n - 1].bit_offset >> 3) >= 2)
                                {
                                    if (coda_ascii_cursor_read_bytes(cursor, buffer, 1, 1) != 0)
                                    {
                                        return -1;
                                    }
                                    if (*buffer == '\n')
                                    {
                                        /* carriage return followed by linefeed -> dos platform convention */
                                        *bit_size = 16;
                                        ((coda_ascbin_product *)cursor->product)->end_of_line = eol_crlf;
                                    }
                                    else
                                    {
                                        /* just a carriage return -> mac platform convention */
                                        *bit_size = 8;
                                        ((coda_ascbin_product *)cursor->product)->end_of_line = eol_cr;
                                    }
                                }
                                else
                                {
                                    /* just a carriage return -> mac platform convention */
                                    *bit_size = 8;
                                    ((coda_ascbin_product *)cursor->product)->end_of_line = eol_cr;
                                }
                                break;
                            default:
                                {
                                    char s[21];

                                    coda_str64(cursor->stack[cursor->n - 1].bit_offset >> 3, s);
                                    coda_set_error(CODA_ERROR_PRODUCT,
                                                   "product error detected in %s (invalid end-of-line sequence - not a "
                                                   "carriage return or linefeed character - byte offset = %s)",
                                                   cursor->product->filename, s);
                                    return -1;
                                }
                        }
                    }
            }
            break;
        case tag_ascii_line:
            if (cursor->product->format == coda_format_ascii)
            {
                int64_t byte_offset;
                long *asciiline_end_offset;
                long bottom_index;
                long top_index;

                if ((cursor->stack[cursor->n - 1].bit_offset & 0x7) != 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT,
                                   "product error detected (ascii line does not start at byte boundary)");
                    return -1;
                }

                if (((coda_ascbin_product *)cursor->product)->asciiline_end_offset == NULL)
                {
                    if (coda_ascii_init_asciilines(cursor->product) != 0)
                    {
                        return -1;
                    }
                }
                if (((coda_ascbin_product *)cursor->product)->num_asciilines == 0)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_BOUNDS_READ, "trying to read from an empty file");
                    return -1;
                }

                bottom_index = 0;
                top_index = ((coda_ascbin_product *)cursor->product)->num_asciilines - 1;
                asciiline_end_offset = ((coda_ascbin_product *)cursor->product)->asciiline_end_offset;
                byte_offset = cursor->stack[cursor->n - 1].bit_offset >> 3;

                while (top_index != bottom_index)
                {
                    long index;

                    index = (bottom_index + top_index) / 2;
                    if (byte_offset < asciiline_end_offset[index])
                    {
                        top_index = index;
                    }
                    else
                    {
                        bottom_index = index + 1;
                    }
                }
                *bit_size = ((asciiline_end_offset[top_index] - byte_offset) << 3);

                /* remove length of eol if eol was not included in the asciiline type */
                /* and if the line does not end with eof */
                if (!((coda_ascii_line *)cursor->stack[cursor->n - 1].type)->include_eol)
                {
                    if (!(top_index == ((coda_ascbin_product *)cursor->product)->num_asciilines - 1 &&
                          ((coda_ascbin_product *)cursor->product)->lastline_ending == eol_unknown))
                    {
                        *bit_size -= 8;
                        if (((coda_ascbin_product *)cursor->product)->end_of_line == eol_crlf)
                        {
                            *bit_size -= 8;
                        }
                    }
                }
            }
            else
            {
                int64_t available_bytes;
                int64_t byte_offset;
                int64_t byte_size;
                char buffer[1];

                /* not a pure ascii file -> don't use asciiline cache */

                if ((cursor->stack[cursor->n - 1].bit_offset & 0x7) != 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT,
                                   "product error detected (ascii line does not start at byte boundary)");
                    return -1;
                }
                byte_offset = cursor->stack[cursor->n - 1].bit_offset >> 3;
                available_bytes = cursor->product->file_size - byte_offset;
                byte_size = 0;

                while (byte_size < available_bytes)
                {
                    if (read_bytes_in_bounds(cursor->product, byte_offset + byte_size, 1, buffer) != 0)
                    {
                        return -1;
                    }
                    if (*buffer == '\r' || *buffer == '\n')
                    {
                        break;
                    }
                    byte_size++;
                }
                if (((coda_ascii_line *)cursor->stack[cursor->n - 1].type)->include_eol)
                {
                    if (*buffer == '\r' && byte_size + 1 < available_bytes && buffer[1] == '\n')
                    {
                        byte_size++;
                    }
                    byte_size++;
                }
                *bit_size = (byte_size << 3);
            }
            break;
        case tag_ascii_white_space:
            {
                int64_t available_bytes;
                int64_t byte_offset;
                int64_t byte_size;
                char buffer[1];

                if ((cursor->stack[cursor->n - 1].bit_offset & 0x7) != 0)
                {
                    coda_set_error(CODA_ERROR_PRODUCT,
                                   "product error detected (ascii white space does not start at byte boundary)");
                    return -1;
                }
                byte_offset = cursor->stack[cursor->n - 1].bit_offset >> 3;
                available_bytes = cursor->product->file_size - byte_offset;
                byte_size = 0;

                while (byte_size < available_bytes)
                {
                    if (read_bytes_in_bounds(cursor->product, byte_offset + byte_size, 1, buffer) != 0)
                    {
                        return -1;
                    }
                    if (*buffer != ' ' && *buffer != '\t')
                    {
                        break;
                    }
                    byte_size++;
                }
                *bit_size = (byte_size << 3);
            }
            break;
        case tag_ascii_record:
        case tag_ascii_union:
        case tag_ascii_array:
        case tag_ascii_time:
            assert(0);
            exit(1);
    }

    return 0;
}

int coda_ascii_cursor_get_num_elements(const coda_cursor *cursor, long *num_elements)
{
    switch (cursor->stack[cursor->n - 1].type->type_class)
    {
        case coda_record_class:
        case coda_array_class:
            return coda_ascbin_cursor_get_num_elements(cursor, num_elements);
        default:
            *num_elements = 1;  /* non-compound types type */
            break;
    }

    return 0;
}

static int read_int64(const coda_cursor *cursor, int64_t *dst, int64_t size_boundary)
{
    coda_ascii_integer *type = (coda_ascii_integer *)cursor->stack[cursor->n - 1].type;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    char buffer[MAXBUFFERSIZE];
    long buffer_size;
    int dynamic_size;

    if (bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "reading of ascii data does not start at byte boundary");
        return -1;
    }
    buffer_size = get_buffer_size(type->bit_size, size_boundary, (cursor->product->file_size << 3) - bit_offset,
                                  &dynamic_size);
    if (buffer_size < 0)
    {
        return -1;
    }
    assert(buffer_size <= MAXBUFFERSIZE);
    if (read_bytes_in_bounds(cursor->product, bit_offset >> 3, buffer_size, buffer) != 0)
    {
        return -1;
    }
    if (type->mappings != NULL)
    {
        switch (parse_integer_mapping(buffer, buffer_size, type->mappings, dynamic_size, dst))
        {
            case 0:
                /* no mapping applied */
                break;
            case 1:
                /* applicable mapping found */
                return 0;
            default:
                return -1;
        }
    }
    if (coda_ascii_parse_int64(buffer, buffer_size, dst, dynamic_size) < 0)
    {
        return -1;
    }

    return 0;
}

static int read_uint64(const coda_cursor *cursor, uint64_t *dst, int64_t size_boundary)
{
    coda_ascii_integer *type = (coda_ascii_integer *)cursor->stack[cursor->n - 1].type;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    char buffer[MAXBUFFERSIZE];
    long buffer_size;
    int dynamic_size;

    if (bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "reading of ascii data does not start at byte boundary");
        return -1;
    }
    buffer_size = get_buffer_size(type->bit_size, size_boundary, (cursor->product->file_size << 3) - bit_offset,
                                  &dynamic_size);
    if (buffer_size < 0)
    {
        return -1;
    }
    assert(buffer_size <= MAXBUFFERSIZE);
    if (read_bytes_in_bounds(cursor->product, bit_offset >> 3, buffer_size, buffer) != 0)
    {
        return -1;
    }
    if (type->mappings != 0)
    {
        switch (parse_integer_mapping(buffer, buffer_size, type->mappings, dynamic_size, (int64_t *)dst))
        {
            case 0:
                /* no mapping applied */
                break;
            case 1:
                /* applicable mapping found */
                return 0;
            default:
                return -1;
        }
    }
    if (coda_ascii_parse_uint64(buffer, buffer_size, dst, dynamic_size) < 0)
    {
        return -1;
    }

    return 0;
}

static int read_int8(const coda_cursor *cursor, int8_t *dst, int64_t size_boundary)
{
    int64_t value;

    if (read_int64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXINT8 || value < -MAXINT8 - 1)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for int8)");
        return -1;
    }
    *dst = (int8_t)value;

    return 0;
}

static int read_uint8(const coda_cursor *cursor, uint8_t *dst, int64_t size_boundary)
{
    uint64_t value;

    if (read_uint64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXUINT8)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for uint8)");
        return -1;
    }
    *dst = (uint8_t)value;

    return 0;
}

static int read_int16(const coda_cursor *cursor, int16_t *dst, int64_t size_boundary)
{
    int64_t value;

    if (read_int64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXINT16 || value < -MAXINT16 - 1)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for int16)");
        return -1;
    }
    *dst = (int16_t)value;

    return 0;
}

static int read_uint16(const coda_cursor *cursor, uint16_t *dst, int64_t size_boundary)
{
    uint64_t value;

    if (read_uint64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXUINT16)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for uint16)");
        return -1;
    }
    *dst = (uint16_t)value;

    return 0;
}

static int read_int32(const coda_cursor *cursor, int32_t *dst, int64_t size_boundary)
{
    int64_t value;

    if (read_int64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXINT32 || value < -MAXINT32 - 1)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for int32)");
        return -1;
    }
    *dst = (int32_t)value;

    return 0;
}

static int read_uint32(const coda_cursor *cursor, uint32_t *dst, int64_t size_boundary)
{
    uint64_t value;

    if (read_uint64(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    if (value > MAXUINT32)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (value for ascii integer too large for uint32)");
        return -1;
    }
    *dst = (uint32_t)value;

    return 0;
}

static int read_double(const coda_cursor *cursor, double *dst, int64_t size_boundary)
{
    coda_ascii_float *type = (coda_ascii_float *)cursor->stack[cursor->n - 1].type;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    char buffer[MAXBUFFERSIZE];
    long buffer_size;
    int dynamic_size;

    if (bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "reading of ascii data does not start at byte boundary");
        return -1;
    }
    buffer_size = get_buffer_size(type->bit_size, size_boundary, (cursor->product->file_size << 3) - bit_offset,
                                  &dynamic_size);
    if (buffer_size < 0)
    {
        return -1;
    }
    assert(buffer_size <= MAXBUFFERSIZE);
    if (read_bytes_in_bounds(cursor->product, bit_offset >> 3, buffer_size, buffer) != 0)
    {
        return -1;
    }
    if (type->mappings != NULL)
    {
        switch (parse_float_mapping(buffer, buffer_size, type->mappings, dynamic_size, dst))
        {
            case 0:
                /* no mapping applied */
                break;
            case 1:
                /* applicable mapping found */
                return 0;
            default:
                return -1;
        }
    }
    if (coda_ascii_parse_double(buffer, buffer_size, dst, dynamic_size) < 0)
    {
        return -1;
    }

    return 0;
}

static int read_float(const coda_cursor *cursor, float *dst, int64_t size_boundary)
{
    double value;

    if (read_double(cursor, &value, size_boundary) != 0)
    {
        return -1;
    }
    *dst = (float)value;

    return 0;
}

static int read_time(const coda_cursor *cursor, double *dst, int64_t size_boundary)
{
    coda_ascii_time *type = (coda_ascii_time *)cursor->stack[cursor->n - 1].type;
    int64_t bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    char buffer[MAXBUFFERSIZE];
    long buffer_size;
    int dynamic_size;

    if (bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_FILE_READ, "reading of ascii data does not start at byte boundary");
        return -1;
    }
    buffer_size = get_buffer_size(type->bit_size, size_boundary, (cursor->product->file_size << 3) - bit_offset,
                                  &dynamic_size);
    if (buffer_size < 0)
    {
        return -1;
    }
    assert(buffer_size <= MAXBUFFERSIZE);
    if (read_bytes_in_bounds(cursor->product, bit_offset >> 3, buffer_size, buffer) != 0)
    {
        return -1;
    }
    if (((coda_ascii_text *)type->base_type)->mappings != 0)
    {
        switch (parse_float_mapping(buffer, buffer_size, ((coda_ascii_text *)type->base_type)->mappings, dynamic_size,
                                    dst))
        {
            case 0:
                /* no mapping applied */
                break;
            case 1:
                /* applicable mapping found */
                return 0;
            default:
                return -1;
        }
    }
    switch (type->time_type)
    {
        case ascii_envisat_datetime:
            return parse_envisat_datetime(buffer, buffer_size, dst);
        case ascii_gome_datetime:
            return parse_gome_datetime(buffer, buffer_size, dst);
        case ascii_eps_datetime:
            return parse_eps_datetime(buffer, buffer_size, dst);
        case ascii_eps_datetime_long:
            return parse_eps_datetime_long(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_ymd1:
            return parse_ccsds_datetime_ymd1(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_ymd1_with_ref:
            return parse_ccsds_datetime_ymd1_with_ref(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_ymd2:
            return parse_ccsds_datetime_ymd2(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_ymd2_with_ref:
            return parse_ccsds_datetime_ymd2_with_ref(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_utc1:
            return parse_ccsds_datetime_utc1(buffer, buffer_size, dst);
        case ascii_ccsds_datetime_utc2:
            return parse_ccsds_datetime_utc2(buffer, buffer_size, dst);
    }

    assert(0);
    exit(1);
}

static int read_char(const coda_cursor *cursor, char *dst, int64_t size_boundary)
{
    int64_t bit_offset;
    int dynamic_size;

    bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    if ((bit_offset & 0x7) != 0)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii text does not start at byte boundary)");
        return -1;
    }
    if (get_buffer_size(8, size_boundary, (cursor->product->file_size << 3) - bit_offset, &dynamic_size) == -1)
    {
        return -1;
    }
    return read_bytes_in_bounds(cursor->product, bit_offset >> 3, 1, dst);
}

static int read_string(const coda_cursor *cursor, char *dst, long dst_size, int64_t size_boundary)
{
    coda_ascii_type *type;
    int64_t bit_offset;
    int64_t read_size = 0;

    type = (coda_ascii_type *)cursor->stack[cursor->n - 1].type;
    bit_offset = cursor->stack[cursor->n - 1].bit_offset;
    if ((bit_offset & 0x7) != 0)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii text does not start at byte boundary)");
        return -1;
    }
    if (type->bit_size == -1)
    {
        int64_t bit_size;

        if (coda_ascii_cursor_get_bit_size(cursor, &bit_size, size_boundary) != 0)
        {
            return -1;
        }
        if ((bit_size & 0x7) != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii text does not have a rounded byte size)");
            return -1;
        }
        read_size = bit_size >> 3;
    }
    else
    {
        read_size = type->bit_size >> 3;
    }
    if (read_size + 1 > dst_size)       /* account for terminating zero */
    {
        read_size = dst_size - 1;
    }
    if (read_size > 0)
    {
        if (size_boundary >= 0 && read_size > (size_boundary >> 3))
        {
            char rs[21];
            char sb[21];
            
            coda_str64(read_size, rs);
            coda_str64(size_boundary >> 3, sb);
            coda_set_error(CODA_ERROR_PRODUCT, "trying to read %s bytes from %s byte buffer", rs, sb);
            return -1;
        }
        if (read_bytes(cursor->product, bit_offset >> 3, read_size, dst) != 0)
        {
            return -1;
        }
        dst[read_size] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }

    return 0;
}

static int read_array(const coda_cursor *cursor, read_function read_basic_type_function, uint8_t *dst,
                      int basic_type_size, coda_array_ordering array_ordering, int64_t size_boundary)
{
    coda_cursor array_cursor;
    long dim[CODA_MAX_NUM_DIMS];
    int num_dims;
    long num_elements;
    int i;

    if (coda_cursor_get_array_dim(cursor, &num_dims, dim) != 0)
    {
        return -1;
    }

    array_cursor = *cursor;
    if (num_dims <= 1 || array_ordering != coda_array_ordering_fortran)
    {
        /* C-style array ordering */

        num_elements = 1;
        for (i = 0; i < num_dims; i++)
        {
            num_elements *= dim[i];
        }

        if (num_elements > 0)
        {
            if (coda_ascbin_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (i = 0; i < num_elements; i++)
            {
                int64_t element_size_boundary;

                element_size_boundary = size_boundary - (array_cursor.stack[cursor->n - 1].bit_offset -
                                                         cursor->stack[cursor->n - 1].bit_offset);
                if ((*read_basic_type_function)(&array_cursor, &dst[i * basic_type_size], element_size_boundary) != 0)
                {
                    return -1;
                }
                if (i < num_elements - 1)
                {
                    if (coda_ascbin_cursor_goto_next_array_element(&array_cursor) != 0)
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else
    {
        long incr[CODA_MAX_NUM_DIMS + 1];
        long increment;
        long c_index;
        long fortran_index;

        /* Fortran-style array ordering */

        incr[0] = 1;
        for (i = 0; i < num_dims; i++)
        {
            incr[i + 1] = incr[i] * dim[i];
        }

        increment = incr[num_dims - 1];
        num_elements = incr[num_dims];

        if (num_elements > 0)
        {
            c_index = 0;
            fortran_index = 0;
            if (coda_ascbin_cursor_goto_array_element_by_index(&array_cursor, 0) != 0)
            {
                return -1;
            }
            for (;;)
            {
                do
                {
                    int64_t element_size_boundary;

                    element_size_boundary = size_boundary - (array_cursor.stack[cursor->n - 1].bit_offset -
                                                             cursor->stack[cursor->n - 1].bit_offset);
                    if ((*read_basic_type_function)(&array_cursor, &dst[fortran_index * basic_type_size],
                                                    element_size_boundary) != 0)
                    {
                        return -1;
                    }
                    c_index++;
                    if (c_index < num_elements)
                    {
                        if (coda_ascbin_cursor_goto_next_array_element(&array_cursor) != 0)
                        {
                            return -1;
                        }
                    }
                    fortran_index += increment;
                } while (fortran_index < num_elements);

                if (c_index == num_elements)
                {
                    break;
                }

                fortran_index += incr[num_dims - 2] - incr[num_dims];
                i = num_dims - 3;
                while (i >= 0 && fortran_index >= incr[i + 2])
                {
                    fortran_index += incr[i] - incr[i + 2];
                    i--;
                }
            }
        }
    }

    return 0;
}

int coda_ascii_cursor_read_int8(const coda_cursor *cursor, int8_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int8 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int8_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int8 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int8 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_uint8(const coda_cursor *cursor, uint8_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint8_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint8 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint8 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_int16(const coda_cursor *cursor, int16_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int16 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int16_t)value;
                        }
                        break;
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int16_t)value;
                        }
                        break;
                    case coda_native_type_int16:
                        {
                            int16_t value;

                            if (read_int16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int16_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int16 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int16 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_uint16(const coda_cursor *cursor, uint16_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint16 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint16_t)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint16_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint16 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint16 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_int32(const coda_cursor *cursor, int32_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int32 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int32_t)value;
                        }
                        break;
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int32_t)value;
                        }
                        break;
                    case coda_native_type_int16:
                        {
                            int16_t value;

                            if (read_int16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int32_t)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int32_t)value;
                        }
                        break;
                    case coda_native_type_int32:
                        {
                            int32_t value;

                            if (read_int32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int32_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int32 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int32 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_uint32(const coda_cursor *cursor, uint32_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint32 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint32_t)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint32_t)value;
                        }
                        break;
                    case coda_native_type_uint32:
                        {
                            uint32_t value;

                            if (read_uint32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint32_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint32 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint32 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_int64(const coda_cursor *cursor, int64_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a int64 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_int16:
                        {
                            int16_t value;

                            if (read_int16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_int32:
                        {
                            int32_t value;

                            if (read_int32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_uint32:
                        {
                            uint32_t value;

                            if (read_uint32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    case coda_native_type_int64:
                        {
                            int64_t value;

                            if (read_int64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (int64_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a int64 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a int64 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read converted data using a uint64 data type");
                    return -1;
                }
                switch (type->read_type)
                {
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint64_t)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint64_t)value;
                        }
                        break;
                    case coda_native_type_uint32:
                        {
                            uint32_t value;

                            if (read_uint32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint64_t)value;
                        }
                        break;
                    case coda_native_type_uint64:
                        {
                            uint64_t value;

                            if (read_uint64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (uint64_t)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a uint64 data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a uint64 data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_float(const coda_cursor *cursor, float *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_int16:
                        {
                            int16_t value;

                            if (read_int16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_int32:
                        {
                            int32_t value;

                            if (read_int32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_uint32:
                        {
                            uint32_t value;

                            if (read_uint32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_int64:
                        {
                            int64_t value;

                            if (read_int64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_uint64:
                        {
                            uint64_t value;

                            if (read_uint64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)(int64_t)value;
                        }
                        break;
                    case coda_native_type_float:
                        {
                            float value;

                            if (read_float(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    case coda_native_type_double:
                        {
                            double value;

                            if (read_double(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (float)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a float data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    *dst = (float)((*dst * type->conversion->numerator) / type->conversion->denominator);
                }
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a float data type");
            return -1;
    }
    return 0;
}

int coda_ascii_cursor_read_double(const coda_cursor *cursor, double *dst, int64_t size_boundary)
{
    switch (((coda_ascii_type *)cursor->stack[cursor->n - 1].type)->tag)
    {
        case tag_ascii_integer:
        case tag_ascii_float:
            {
                coda_ascii_number *type;

                type = (coda_ascii_number *)cursor->stack[cursor->n - 1].type;
                switch (type->read_type)
                {
                    case coda_native_type_int8:
                        {
                            int8_t value;

                            if (read_int8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_uint8:
                        {
                            uint8_t value;

                            if (read_uint8(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_int16:
                        {
                            int16_t value;

                            if (read_int16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_uint16:
                        {
                            uint16_t value;

                            if (read_uint16(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_int32:
                        {
                            int32_t value;

                            if (read_int32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_uint32:
                        {
                            uint32_t value;

                            if (read_uint32(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_int64:
                        {
                            int64_t value;

                            if (read_int64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_uint64:
                        {
                            uint64_t value;

                            if (read_uint64(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)(int64_t)value;
                        }
                        break;
                    case coda_native_type_float:
                        {
                            float value;

                            if (read_float(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    case coda_native_type_double:
                        {
                            double value;

                            if (read_double(cursor, &value, size_boundary) != 0)
                            {
                                return -1;
                            }
                            *dst = (double)value;
                        }
                        break;
                    default:
                        coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read %s data using a double data type",
                                       coda_type_get_native_type_name(type->read_type));
                        return -1;
                }
                if (coda_option_perform_conversions && type->conversion != NULL)
                {
                    *dst = (*dst * type->conversion->numerator) / type->conversion->denominator;
                }
            }
            break;
        case tag_ascii_time:
            if (read_time(cursor, dst, size_boundary) != 0)
            {
                return -1;
            }
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a double data type");
            return -1;
    }

    return 0;
}

int coda_ascii_cursor_read_char(const coda_cursor *cursor, char *dst, int64_t size_boundary)
{
    coda_ascii_type *type;

    type = (coda_ascii_type *)cursor->stack[cursor->n - 1].type;
    if (type->tag == tag_ascii_text && type->bit_size == 8)
    {
        return read_char(cursor, dst, size_boundary);
    }
    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
    return -1;
}

int coda_ascii_cursor_read_string(const coda_cursor *cursor, char *dst, long dst_size, int64_t size_boundary)
{
    return read_string(cursor, dst, dst_size, size_boundary);
}

int coda_ascii_cursor_read_bits(const coda_cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    if (bit_length & 0x7)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT,
                       "cannot read ascii data using a bitsize that is not a multiple of 8");
        return -1;
    }
    if ((cursor->stack[cursor->n - 1].bit_offset + bit_offset) & 0x7)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii text does not start at byte boundary)");
        return -1;
    }
    return read_bytes(cursor->product, (cursor->stack[cursor->n - 1].bit_offset + bit_offset) >> 3, bit_length >> 3,
                      dst);
}

int coda_ascii_cursor_read_bytes(const coda_cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    if (cursor->stack[cursor->n - 1].bit_offset & 0x7)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "product error detected (ascii text does not start at byte boundary)");
        return -1;
    }
    return read_bytes(cursor->product, (cursor->stack[cursor->n - 1].bit_offset >> 3) + offset, length, dst);
}

int coda_ascii_cursor_read_int8_array(const coda_cursor *cursor, int8_t *dst, coda_array_ordering array_ordering,
                                      int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_int8, (uint8_t *)dst, sizeof(int8_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_uint8_array(const coda_cursor *cursor, uint8_t *dst, coda_array_ordering array_ordering,
                                       int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_uint8, (uint8_t *)dst, sizeof(uint8_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_int16_array(const coda_cursor *cursor, int16_t *dst, coda_array_ordering array_ordering,
                                       int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_int16, (uint8_t *)dst, sizeof(int16_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_uint16_array(const coda_cursor *cursor, uint16_t *dst, coda_array_ordering array_ordering,
                                        int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_uint16, (uint8_t *)dst, sizeof(uint16_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_int32_array(const coda_cursor *cursor, int32_t *dst, coda_array_ordering array_ordering,
                                       int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_int32, (uint8_t *)dst, sizeof(int32_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_uint32_array(const coda_cursor *cursor, uint32_t *dst, coda_array_ordering array_ordering,
                                        int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_uint32, (uint8_t *)dst, sizeof(uint32_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_int64_array(const coda_cursor *cursor, int64_t *dst, coda_array_ordering array_ordering,
                                       int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_int64, (uint8_t *)dst, sizeof(int64_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering,
                                        int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_uint64, (uint8_t *)dst, sizeof(uint64_t),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_float_array(const coda_cursor *cursor, float *dst, coda_array_ordering array_ordering,
                                       int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_float, (uint8_t *)dst, sizeof(float),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_double_array(const coda_cursor *cursor, double *dst, coda_array_ordering array_ordering,
                                        int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_double, (uint8_t *)dst, sizeof(double),
                      array_ordering, size_boundary);
}

int coda_ascii_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering,
                                      int64_t size_boundary)
{
    return read_array(cursor, (read_function)&coda_ascii_cursor_read_char, (uint8_t *)dst, sizeof(char),
                      array_ordering, size_boundary);
}

/** @} */
