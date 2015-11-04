/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

/** \defgroup coda_time CODA Time
 * The CODA Time module contains all functions and procedures related to time handling.
 *
 * Most of the time functions assume that you stay within the same time system. In CODA a single time value can be
 * represented in three different forms and CODA provides functions to convert between them:
 * - as a floating point value indicating the amount of seconds since 2000-01-01 00:00:00.00000 (this Jan 1st 2000
 *   is an 'epoch' in the same time system as your time value is in)
 * - as a decomposition of individual year, month, day, hour, minute, second, and microsecond values.
 * - as a string (e.g. "2005-04-03 02:01:00.00000").
 *
 * The problem is that most time values are provided using the UTC time system. However, because of leap seconds, the
 * only valid representations for a UTC time value are the last two. The problem with the floating point representation
 * is that there is no unique definition of the Jan 1st 2000 epoch. Whenever a leap second is introduced this also
 * shifts the epoch by one second. For instance, you cannot calculate the difference in seconds between UTC 2006-01-02
 * 00:00:00 and UTC 2005-12-30 00:00:00 by just subtracting the two floating point representations of those times
 * because of the leap second at 2005-12-31 23:59:60. If you would just subtract the values you would get 172800
 * seconds, but the actual answer should be 172801 seconds.
 * If you want to calculate leap-second-accurate time differences the only solution is to convert your UTC time values
 * to a time system that is based on actual clock ticks such as TAI or GPS.
 *
 * In CODA this problem is solved by introducing special UTC leap-second-aware functions for converting from a floating
 * point value to a string or datetime decomposition (and vice versa). The floating point value is always in TAI
 * whereas the string and datetime decomposition values represent the time value in UTC (be aware that the value for
 * 'amount of seconds in a minute' can range from 0 to 60 inclusive for UTC!)
 *
 * For each public time function there are thus two variants. One that treats all days as having 86400 seconds (and
 * where both the input(s) and output(s) are of the same time system) and one that is able to deal with leap seconds.
 * The functions that deal with leap seconds have 'utc' in the name and assume that the number of seconds since
 * 2000-01-01 is a value in the TAI time system whereas the datetime decomposition (or string value) is using the UTC
 * time system.
 *
 * Below is an overview of time systems that are commonly used:
 * - TAI: International Atomic Time scale
 * International Atomic Time (TAI) represents the mean of readings of several atomic clocks, and its fundamental unit
 * is exactly one SI second at mean sea level and is, therefore, constant and continuous.
 * d TAI = TAI - UTC is the increment to be applied to UTC to give TAI. 
 * - UTC: Coordinated Universal Time
 * The time system generally used is the Coordinated Universal Time (UTC), previously called Greenwich Mean Time.
 * The UTC is piece wise uniform and continuous, i.e. the time difference between UTC and TAI is equal to an integer
 * number of seconds and is constant except for occasional jumps from inserted integer leap seconds.
 * The leap seconds are inserted to cause UTC to follow the rotation of the Earth, which is expressed by means of the
 * non uniform time reference Universal Time UT1. 
 * If UT1 is predicted to lag behind UTC by more than 0.9 seconds, a leap second is inserted.
 * CODA has a built-in table of leap seconds up to 2012-07-01. You can use a more recent table by downloading the
 * recent list of leap seconds from ftp://maia.usno.navy.mil/ser7/tai-utc.dat and set the environment variable
 * CODA_LEAP_SECOND_TABLE with a full path to this file.
 * - UT1: Universal Time
 * Universal Time (UT1) is a time reference that conforms, within a close approximation, to the mean diurnal motion of
 * the Earth. It is determined from observations of the diurnal motions of the stars, and then corrected for the shift
 * in the longitude of the observing stations caused by the polar motion. 
 * - GPS: GPS Time
 * GPS Time is an atomic clock time similar to but not the same as UTC time. It is synchronised to UTC but the main
 * difference relies in the fact that GPS time does not introduce any leap second. Thus, the introduc tion of UTC leap
 * second causes the GPS time and UTC time to differ by a known integer number of cumulative leap seconds; i.e. the
 * leap seconds that have been accumulated since GPS epoch in midnight January 5, 1980. 
 * d GPS = TAI - GPS is the increment to be applied to GPS to give TAI, being a constant value of 19 seconds.
 */

/** \addtogroup coda_time
 * @{
 */

static int num_leap_seconds = 0;
static double *leap_second_table = NULL;

int coda_month_to_integer(const char month[3])
{
    char month_str[4];

    /* just for safety reasons (strncasecmp behavior) we make 'month' a 0-terminated string */
    month_str[0] = month[0];
    month_str[1] = month[1];
    month_str[2] = month[2];
    month_str[3] = '\0';

    if (strncasecmp(month_str, "jan", 3) == 0)
    {
        return 1;
    }
    if (strncasecmp(month_str, "feb", 3) == 0)
    {
        return 2;
    }
    if (strncasecmp(month_str, "mar", 3) == 0)
    {
        return 3;
    }
    if (strncasecmp(month_str, "apr", 3) == 0)
    {
        return 4;
    }
    if (strncasecmp(month_str, "may", 3) == 0)
    {
        return 5;
    }
    if (strncasecmp(month_str, "jun", 3) == 0)
    {
        return 6;
    }
    if (strncasecmp(month_str, "jul", 3) == 0)
    {
        return 7;
    }
    if (strncasecmp(month_str, "aug", 3) == 0)
    {
        return 8;
    }
    if (strncasecmp(month_str, "sep", 3) == 0)
    {
        return 9;
    }
    if (strncasecmp(month_str, "oct", 3) == 0)
    {
        return 10;
    }
    if (strncasecmp(month_str, "nov", 3) == 0)
    {
        return 11;
    }
    if (strncasecmp(month_str, "dec", 3) == 0)
    {
        return 12;
    }

    coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid month argument (%s) (%s:%u)", month_str, __FILE__, __LINE__);
    return -1;
}

static int parse_integer(const char *buffer, int num_digits, int use_leading_spaces, int *value)
{
    if (use_leading_spaces)
    {
        while (num_digits > 1 && *buffer == ' ')
        {
            buffer++;
            num_digits--;
        }
    }
    *value = 0;
    while (num_digits > 0)
    {
        if (*buffer < '0' || *buffer > '9')
        {
            return -1;
        }
        *value = (*value) * 10 + (*buffer - '0');
        buffer++;
        num_digits--;
    }

    return 0;
}

static int y(int Y)
{
    return Y + (Y < 0);
}

static int int_div(int a, int b)
{
    return a / b - (a % b < 0);
}

static int int_mod(int a, int b)
{
    return a % b + b * (a % b < 0);
}


/* convert (D,M,Y) to a 4713BC-based Julian day-number.
 *
 * --> (1,1,-4713) yields 0; earlier dates yield negative numbers; later dates yield positive numbers.
 *                           Please note that the zero-day is basically arbitrary; we can extend the system
 *                           to include dates before it.
 * --> Note that the Julian calender was introduced in 45BC, but some initial problems meant that
 *     only after 4AD the Julian calender was kept properly. Therefore, earlier dates do not reflect
 *     dates that were in common use at the time.
 *
 * return value:
 *   -1: error occurred; (D,M,Y) does not refer to a valid Julian date.
 *    0: success; *jd is updated with the correct julian day number.
 *
 * Note that the year 0 does not exist; the year 1(AD) is preceded by 1(BC) or -1.
 */
static int dmy_to_mjd2000_julian(int D, int M, int Y, int *jd)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

    /* check input */
    if (Y == 0 || M < 1 || M > 12 || D < 1 || D > tabel[M] - tabel[M - 1] + ((M == 2) && (y(Y) % 4 == 0)))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                       __FILE__, __LINE__);
        return -1;
    }

    *jd = D + 365 * y(Y) + int_div(y(Y), 4) + tabel[M - 1] - ((M <= 2) && (y(Y) % 4 == 0)) + 1721058;

    return 0;
}

/* convert (D,M,Y) to a 4-OCT-1586-based Gregorian day-number.
 *
 * --> (4,10,1586) yields 0; earlier dates yield negative numbers; later dates yield positive numbers.
 *                           Please note that the zero-day is basically arbitrary; we can extend the system
 *                           to include dates before it. The date chosen is the last date that the Julian
 *                           Calendar system was in effect, prior to the Gregorian reform.
 * --> Many countries converted from the Gregorian to the Julian calendar after 4-OCT-1586, e.g. the English
 *     commonwealth countries (2-9-1752 was the last Julian date for them).
 *
 * return value:
 *   -1: error occurred; (D,M,Y) does not refer to a valid Gregorian date.
 *    0: success; *gd is updated with the correct Gregorian day number.
 *
 * Note that the year 0 does not exist; the year 1(AD) is preceded by 1(BC) or -1.
 */
static int dmy_to_mjd2000_gregorian(int D, int M, int Y, int *gd)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

    /* check input */
    if (Y == 0 || M < 1 || M > 12 || D < 1 ||
        D > tabel[M] - tabel[M - 1] + ((M == 2) && ((y(Y) % 4 == 0) ^ (y(Y) % 100 == 0) ^ (y(Y) % 400 == 0))))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                       __FILE__, __LINE__);
        return -1;
    }

    *gd = D + tabel[M - 1] + 365 * y(Y) + int_div(y(Y), 4) - int_div(y(Y), 100) + int_div(y(Y), 400) +
        -((M <= 2) && ((y(Y) % 4 == 0) - (y(Y) % 100 == 0) + (y(Y) % 400 == 0))) - 579551;

    return 0;
}

/* this function yields the day number for a date as number-of-days-since-1-1-2000.
 * The (D,M,J) is either given in Julian or Gregorian calendar; the function decides
 * which based upon the Julian (TD,TM,TY)-date, which specifies the last date when
 * the Julian calendar is in effect.
 */

/*
 * date: 01 - 01 - -4713 -> mjd2000 =   -2451545
 * date: 31 - 12 -    -1 -> mjd2000 =    -730122
 * date: 01 - 01 -    +0 -> mjd2000 = INVALID
 * date: 01 - 01 -    +1 -> mjd2000 =    -730121
 * date: 01 - 01 -  +100 -> mjd2000 =    -693962
 * date: 04 - 10 - +1586 -> mjd2000 =    -150924
 * date: 02 - 09 - +1752 -> mjd2000 =     -90324
 * date: 03 - 09 - +1752 -> mjd2000 = INVALID
 * date: 13 - 09 - +1752 -> mjd2000 = INVALID
 * date: 14 - 09 - +1752 -> mjd2000 =     -90323
 * date: 17 - 11 - +1858 -> mjd2000 =     -51544
 * date: 01 - 01 - +1950 -> mjd2000 =     -18262
 * date: 01 - 01 - +1970 -> mjd2000 =     -10957
 * date: 01 - 01 - +2000 -> mjd2000 =          0
 * date: 31 - 12 - +2501 -> mjd2000 =     183351
 */
static int dmy_to_mjd2000(int D, int M, int Y, int *mjd2000)
{
    const int TD = 2;
    const int TM = 9;
    const int TY = 1752;
    int transition;
    int the_date;

    if (dmy_to_mjd2000_julian(D, M, Y, &the_date) != 0 || dmy_to_mjd2000_julian(TD, TM, TY, &transition) != 0)
    {
        return -1;
    }

    if (the_date <= transition)
    {
        /* Julian calendar regime */
        *mjd2000 = the_date - 2451545;
    }
    else
    {
        /* Gregorian calendar regime */
        if (dmy_to_mjd2000_gregorian(D, M, Y, &the_date) != 0)
        {
            return -1;
        }

        if (the_date - 150934 <= transition - 2451545)
        {
            coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02d-%02d-%04d) (%s:%u)", D, M, Y,
                           __FILE__, __LINE__);
            return -1;
        }

        *mjd2000 = the_date - 150934;
    }

    return 0;
}

static void getday_leapyear(int dayno, int *D, int *M)
{
    const int tabel[13] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };
    int i;

    assert(dayno >= 0);
    assert(dayno < 366);

    for (i = 1; i <= 12; i++)
    {
        if (dayno < tabel[i])
        {
            break;
        }
    }
    *M = i;
    *D = 1 + dayno - tabel[i - 1];
}

static void getday_nonleapyear(int dayno, int *D, int *M)
{
    const int tabel[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
    int i;

    assert(dayno >= 0);
    assert(dayno < 365);

    for (i = 1; i <= 12; i++)
    {
        if (dayno < tabel[i])
        {
            break;
        }
    }
    *M = i;
    *D = 1 + dayno - tabel[i - 1];
}

static void mjd2000_to_dmy_julian(int mjd, int *D, int *M, int *Y)
{
    int date;

    *Y = 2000;
    date = mjd - 13;    /* true date 1-1-2000 is now 0 */

    *Y += 4 * int_div(date, 1461);
    date = int_mod(date, 1461);

    if (date < 366)
    {
        /* first year is a leap-year */
        getday_leapyear(date, D, M);
    }
    else
    {
        *Y += 1;
        date -= 366;

        *Y += int_div(date, 365);
        date = int_mod(date, 365);

        getday_nonleapyear(date, D, M);
    }
    if (*Y <= 0)
    {
        (*Y)--;
    }
}

static void mjd2000_to_dmy_gregorian(int mjd, int *D, int *M, int *Y)
{
    int date;

    *Y = 2000;
    date = mjd;

    *Y += 400 * int_div(date, 146097);
    date = int_mod(date, 146097);

    if (date < 36525)
    {
        /* first century */

        *Y += 4 * int_div(date, 1461);
        date = int_mod(date, 1461);

        if (date < 366)
        {
            /* first year is a leap_year */
            getday_leapyear(date, D, M);
        }
        else
        {
            *Y += 1;
            date -= 366;

            *Y += int_div(date, 365);
            date = int_mod(date, 365);

            getday_nonleapyear(date, D, M);
        }
    }
    else
    {
        /* second, third and fourth century */
        date -= 36525;
        *Y += 100;

        *Y += 100 * int_div(date, 36524);
        date = int_mod(date, 36524);

        /* first 4-year period had 1460 days, others have 1461 days. */

        if (date < 1460)
        {
            *Y += int_div(date, 365);
            date = int_mod(date, 365);

            getday_nonleapyear(date, D, M);
        }
        else
        {
            date -= 1460;
            *Y += 4;

            *Y += 4 * int_div(date, 1461);
            date = int_mod(date, 1461);

            if (date < 366)
            {
                /* first year is a leap_year */
                getday_leapyear(date, D, M);
            }
            else
            {
                *Y += 1;
                date -= 366;

                *Y += int_div(date, 365);
                date = int_mod(date, 365);

                getday_nonleapyear(date, D, M);
            }
        }
    }
    if (*Y <= 0)
    {
        (*Y)--;
    }
}

static int mjd2000_to_dmy(int mjd2000, int *D, int *M, int *Y)
{
    const int TD = 2;
    const int TM = 9;
    const int TY = 1752;

    int transition_date;

    if (dmy_to_mjd2000(TD, TM, TY, &transition_date) != 0)
    {
        return -1;
    }

    if (mjd2000 <= transition_date)
    {
        mjd2000_to_dmy_julian(mjd2000, D, M, Y);
    }
    else
    {
        mjd2000_to_dmy_gregorian(mjd2000, D, M, Y);
    }

    return 0;
}

static int hms_to_daytime(const long hour, const long minute, const long second, const long musec, double *daytime)
{
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60 || musec < 0 || musec > 999999)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid date/time argument (%02ld:%02ld:%02ld.%06ld) (%s:%u)",
                       hour, minute, second, musec, __FILE__, __LINE__);
        return -1;
    }

    *daytime = 3600.0 * hour + 60.0 * minute + 1.0 * second + musec / 1000000.0;

    return 0;
}

int coda_dayofyear_to_month_day(int year, int day_of_year, int *month, int *day_of_month)
{
    int mjd;

    if (month == NULL || day_of_month == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* check day of year value */
    if (day_of_year < 0 || day_of_year > 366)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "invalid day of year argument (%03d) (%s:%u)", day_of_year,
                       __FILE__, __LINE__);
        return -1;
    }

    /* retrieve mjd2000 of Jan 1st of the requested year */
    if (dmy_to_mjd2000(1, 1, year, &mjd) != 0)
    {
        return -1;
    }

    /* jump to day_of_year */
    mjd += (day_of_year - 1);

    /* retrieve day/month/year for mjd2000 value */
    if (mjd2000_to_dmy(mjd, day_of_month, month, &year) != 0)
    {
        return -1;
    }

    return 0;
}

static int seconds_to_hms(int dayseconds, int *hour, int *minute, int *second)
{
    int s = dayseconds;

    if (s < 0 || s > 86399)
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "dayseconds argument (%d) is not in the range [0,86400) (%s:%u)",
                       s, __FILE__, __LINE__);
        return -1;
    }

    *hour = s / 3600;
    s %= 3600;
    *minute = s / 60;
    s %= 60;
    *second = s;

    return 0;
}

static int register_leap_second(double leap_second)
{
    int i;

    if (num_leap_seconds % BLOCK_SIZE == 0)
    {
        double *new_leap_second_table;

        new_leap_second_table = realloc(leap_second_table, (num_leap_seconds + BLOCK_SIZE) * sizeof(double));
        if (new_leap_second_table == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (num_leap_seconds + BLOCK_SIZE) * sizeof(double), __FILE__, __LINE__);
            return -1;
        }
        leap_second_table = new_leap_second_table;
    }
    if (num_leap_seconds > 0 && leap_second <= leap_second_table[num_leap_seconds - 1])
    {
        /* do sorted insert */
        for (i = 0; i < num_leap_seconds; i++)
        {
            if (leap_second <= leap_second_table[i])
            {
                double tmp;

                if (leap_second == leap_second_table[i])
                {
                    /* ignore double leap_second entries */
                    return 0;
                }
                tmp = leap_second_table[i];
                leap_second_table[i] = leap_second;
                leap_second = tmp;
            }
        }
    }
    leap_second_table[num_leap_seconds] = leap_second;
    num_leap_seconds++;

    return 0;
}

static int read_leap_second_table(char *filename)
{
    char buffer[100];
    int count = 0;
    FILE *f;

    f = fopen(filename, "r");
    if (f == NULL)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "Could not open %s\n", filename);
        return -1;
    }

    for (;;)
    {
        int num_read;
        int result;
        int year;
        char month[3];
        int day;
        double jd;
        double leap_sec;
        double mjd_offset;
        double scalefactor;

        num_read = 0;
        result = fscanf(f, "%100[^\n]%n", buffer, &num_read);
        if (num_read <= 0)
        {
            break;
        }
        if (num_read != 80)
        {
            fclose(f);
            coda_set_error(CODA_ERROR_FILE_READ, "%s is not a valid leap second file\n", filename);
            return -1;
        }
        buffer[num_read] = '\0';
        result = fscanf(f, "%*c");      /* read single '\n' */

        result = sscanf(buffer, " %04d %c%c%c %2d =JD %lf  TAI-UTC= %lf S + (MJD - %lf) X %lf S%n",
                        &year, &month[0], &month[1], &month[2], &day, &jd, &leap_sec, &mjd_offset, &scalefactor,
                        &num_read);
        if (result != 9 && num_read != 80)
        {
            fclose(f);
            coda_set_error(CODA_ERROR_FILE_READ, "%s is not a valid leap second file\n", filename);
            return -1;
        }
        if (jd > 2441317.5)
        {
            if (register_leap_second((jd - 2451544.5) * 86400.0 + (10 + count)) != 0)
            {
                fclose(f);
                return -1;
            }
            count++;
        }
    }

    fclose(f);

    return 0;
}

void coda_leap_second_table_done(void)
{
    if (leap_second_table != NULL)
    {
        free(leap_second_table);
        leap_second_table = NULL;
    }
    num_leap_seconds = 0;
}

int coda_leap_second_table_init(void)
{
    coda_leap_second_table_done();

    if (getenv("CODA_LEAP_SECOND_TABLE") != NULL)
    {
        if (read_leap_second_table(getenv("CODA_LEAP_SECOND_TABLE")) != 0)
        {
            return -1;
        }
    }
    else
    {
        /* create default leap second table */
        register_leap_second(-867887990.0);     /* 1972-07-01 UTC */
        register_leap_second(-851990389.0);     /* 1973-01-01 UTC */
        register_leap_second(-820454388.0);     /* 1974-01-01 UTC */
        register_leap_second(-788918387.0);     /* 1975-01-01 UTC */
        register_leap_second(-757382386.0);     /* 1976-01-01 UTC */
        register_leap_second(-725759985.0);     /* 1977-01-01 UTC */
        register_leap_second(-694223984.0);     /* 1978-01-01 UTC */
        register_leap_second(-662687983.0);     /* 1979-01-01 UTC */
        register_leap_second(-631151982.0);     /* 1980-01-01 UTC */
        register_leap_second(-583891181.0);     /* 1981-07-01 UTC */
        register_leap_second(-552355180.0);     /* 1982-07-01 UTC */
        register_leap_second(-520819179.0);     /* 1983-07-01 UTC */
        register_leap_second(-457660778.0);     /* 1985-07-01 UTC */
        register_leap_second(-378691177.0);     /* 1988-01-01 UTC */
        register_leap_second(-315532776.0);     /* 1990-01-01 UTC */
        register_leap_second(-283996775.0);     /* 1991-01-01 UTC */
        register_leap_second(-236735974.0);     /* 1992-07-01 UTC */
        register_leap_second(-205199973.0);     /* 1993-07-01 UTC */
        register_leap_second(-173663972.0);     /* 1994-07-01 UTC */
        register_leap_second(-126230371.0);     /* 1996-01-01 UTC */
        register_leap_second(-78969570.0);      /* 1997-07-01 UTC */
        register_leap_second(-31535969.0);      /* 1999-01-01 UTC */
        register_leap_second(189388832.0);      /* 2006-01-01 UTC */
        register_leap_second(284083233.0);      /* 2009-01-01 UTC */
        register_leap_second(394416034.0);      /* 2012-07-01 UTC */
        register_leap_second(489024035.0);      /* 2015-07-01 UTC */
    }

    return 0;
}

/** Retrieve the decomposed date corresponding with the given amount of seconds since Jan 1st 2000.
 * \warning This function does _not_ perform any leap second correction. The returned value is just a straightforward
 * conversion using 86400 seconds per day.
 * \param datetime The amount of seconds since Jan 1st 2000.
 * \param year     Pointer to the variable where the year will be stored.
 * \param month    Pointer to the variable where the month of the year (1 - 12) will be stored.
 * \param day      Pointer to the variable where the day of the month (1 - 31) will be stored.
 * \param hour     Pointer to the variable where the hour of the day (0 - 23) will be stored.
 * \param minute   Pointer to the variable where the minute of the hour (0 - 59) will be stored.
 * \param second   Pointer to the variable where the second of the minute (0 - 59) will be stored.
 * \param musec    Pointer to the variable where the microseconds of the second (0 - 999999) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_double_to_parts(double datetime, int *year, int *month, int *day, int *hour, int *minute,
                                          int *second, int *musec)
{
    double seconds;
    int d, m, y;
    int h, min, s, us;
    int days, dayseconds;

    if (year == NULL || month == NULL || day == NULL || hour == NULL || minute == NULL || second == NULL ||
        musec == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_isNaN(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is NaN (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (coda_isInf(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is Infinite (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* we add 0.5 milliseconds so the floor() becomes a round() that rounds at the millisecond */
    datetime += 5E-7;

    seconds = floor(datetime);

    days = (int)floor(seconds / 86400.0);

    if (mjd2000_to_dmy(days, &d, &m, &y) != 0)
    {
        return -1;
    }

    dayseconds = (int)(seconds - days * 86400.0);

    if (seconds_to_hms(dayseconds, &h, &min, &s) != 0)
    {
        return -1;
    }

    us = (int)floor((datetime - seconds) * 1E6);

    *year = y;
    *month = m;
    *day = d;
    *hour = h;
    *minute = min;
    *second = s;
    *musec = us;

    return 0;

}

/** Retrieve the decomposed UTC date corresponding with the given amount of TAI seconds since Jan 1st 2000.
 * This function assumes the input to by the number of seconds since 2000-01-01 in the TAI system. The returned
 * date/time components will be the corresponding UTC datetime (using proper leap second handling for the TAI to UTC
 * conversion).
 * For example:
 * -88361290 will be 1972-01-01 00:00:00 UTC
 * 0 will be 1999-31-12 23:59:28 UTC
 * 284083232 will be 2008-12-31 23:59:59 UTC
 * 284083233 will be 2008-12-31 23:59:60 UTC
 * 284083234 will be 2009-01-01 00:00:00 UTC
 * \warning For dates before 1972-01-01 UTC a fixed leap second offset of 10 is used.
 * \note CODA has a built in table of leap seconds. To use a more recent leap second table, download
 * the most recent file from ftp://maia.usno.navy.mil/ser7/tai-utc.dat and set the environment variable
 * CODA_LEAP_SECOND_TABLE with a full path to this file.
 * \param datetime The amount of seconds since Jan 1st 2000.
 * \param year     Pointer to the variable where the year will be stored.
 * \param month    Pointer to the variable where the month of the year (1 - 12) will be stored.
 * \param day      Pointer to the variable where the day of the month (1 - 31) will be stored.
 * \param hour     Pointer to the variable where the hour of the day (0 - 23) will be stored.
 * \param minute   Pointer to the variable where the minute of the hour (0 - 59) will be stored.
 * \param second   Pointer to the variable where the second of the minute (0 - 60) will be stored.
 * \param musec    Pointer to the variable where the microseconds of the second (0 - 999999) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_double_to_parts_utc(double datetime, int *year, int *month, int *day, int *hour, int *minute,
                                              int *second, int *musec)
{
    double seconds;
    int d, m, y;
    int h, min, s, us;
    int days, dayseconds;
    int leap_sec;
    int is_leap_sec;

    if (year == NULL || month == NULL || day == NULL || hour == NULL || minute == NULL || second == NULL ||
        musec == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_isNaN(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is NaN (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (coda_isInf(datetime))
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "datetime argument is Infinite (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* we add 0.5 milliseconds so the floor() becomes a round() that rounds at the millisecond */
    datetime += 5E-7;

    seconds = floor(datetime);

    assert(leap_second_table != NULL && num_leap_seconds > 0);
    leap_sec = 0;
    while (leap_sec < num_leap_seconds && seconds > leap_second_table[leap_sec])
    {
        leap_sec++;
    }
    is_leap_sec = fabs(seconds - leap_second_table[leap_sec]) < 0.1;
    seconds -= 10 + leap_sec + is_leap_sec;
    datetime -= 10 + leap_sec + is_leap_sec;

    days = (int)floor(seconds / 86400.0);

    if (mjd2000_to_dmy(days, &d, &m, &y) != 0)
    {
        return -1;
    }

    dayseconds = (int)(seconds - days * 86400.0);

    if (seconds_to_hms(dayseconds, &h, &min, &s) != 0)
    {
        return -1;
    }

    us = (int)floor((datetime - seconds) * 1E6);

    *year = y;
    *month = m;
    *day = d;
    *hour = h;
    *minute = min;
    *second = s + is_leap_sec;
    *musec = us;

    return 0;
}

/** Retrieve the number of seconds since Jan 1st 2000 for a certain date and time.
 * \warning This function does _not_ perform any leap second correction. The returned value is just a straightforward
 * conversion using 86400 seconds per day.
 * \param year     The year.
 * \param month    The month of the year (1 - 12).
 * \param day      The day of the month (1 - 31).
 * \param hour     The hour of the day (0 - 23).
 * \param minute   The minute of the hour (0 - 59).
 * \param second   The second of the minute (0 - 59).
 * \param musec    The microseconds of the second (0 - 999999).
 * \param datetime Pointer to the variable where the amount of seconds since Jan 1st 2000 will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_parts_to_double(int year, int month, int day, int hour, int minute, int second, int musec,
                                          double *datetime)
{
    double daytime;
    int mjd2000;

    if (datetime == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "datetime argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (dmy_to_mjd2000(day, month, year, &mjd2000) != 0)
    {
        return -1;
    }

    if (hms_to_daytime(hour, minute, second, musec, &daytime) != 0)
    {
        return -1;
    }

    *datetime = 86400.0 * mjd2000 + daytime;

    return 0;
}

/** Retrieve the number of TAI seconds since Jan 1st 2000 for a certain UTC date and time using leap second correction.
 * This function assumes the input to be an UTC datetime. The returned value will be the seconds since
 * 2000-01-01 in the TAI time system (using proper leap second handling for the UTC to TAI conversion).
 * For example:
 * 1972-01-01 00:00:00 UTC will be -883612790
 * 2000-01-01 00:00:00 UTC will be 32
 * 2008-12-31 23:59:59 UTC will be 284083232
 * 2008-12-31 23:59:60 UTC will be 284083233
 * 2009-01-01 00:00:00 UTC will be 284083234
 * \warning For dates before 1972-01-01 UTC a fixed leap second offset of 10 is used.
 * \note CODA has a built in table of leap seconds. To use a more recent leap second table, download
 * the most recent file from ftp://maia.usno.navy.mil/ser7/tai-utc.dat and set the environment variable
 * CODA_LEAP_SECOND_TABLE with a full path to this file.
 * \param year     The year.
 * \param month    The month of the year (1 - 12).
 * \param day      The day of the month (1 - 31).
 * \param hour     The hour of the day (0 - 23).
 * \param minute   The minute of the hour (0 - 59).
 * \param second   The second of the minute (0 - 60).
 * \param musec    The microseconds of the second (0 - 999999).
 * \param datetime Pointer to the variable where the amount of seconds since Jan 1st 2000 will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_parts_to_double_utc(int year, int month, int day, int hour, int minute, int second,
                                              int musec, double *datetime)
{
    double daytime;
    double t;
    int mjd2000;
    int leap_sec;

    if (datetime == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "datetime argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (dmy_to_mjd2000(day, month, year, &mjd2000) != 0)
    {
        return -1;
    }

    if (hms_to_daytime(hour, minute, second, musec, &daytime) != 0)
    {
        return -1;
    }

    t = 86400.0 * mjd2000 + 10;

    assert(leap_second_table != NULL && num_leap_seconds > 0);
    leap_sec = 0;
    while (leap_sec < num_leap_seconds && t >= leap_second_table[leap_sec])
    {
        t++;
        leap_sec++;
    }

    *datetime = t + daytime;

    return 0;
}

/** Create a string representation for a specific data and time.
 * The string will be formatted using the format that is provided as first parameter.
 * The time string will be stored in the \a str parameter. This parameter should be allocated by the user
 * and should be long enough to hold the formatted time string and a 0 termination character.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * \warning This function does not perform any leap second correction.
 * \param year     The year.
 * \param month    The month of the year (1 - 12).
 * \param day      The day of the month (1 - 31).
 * \param hour     The hour of the day (0 - 23).
 * \param minute   The minute of the hour (0 - 59).
 * \param second   The second of the minute (0 - 60).
 * \param musec    The microseconds of the second (0 - 999999).
 * \param format    Date/time format to use for the string representation of the datetime value.
 * \param str       String representation of the given date and time.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_parts_to_string(int year, int month, int day, int hour, int minute, int second, int musec,
                                          const char *format, char *str)
{
    const char *month_name[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    int literal = 0;
    int fi = 0;
    int si = 0;

    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "str argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    while (format[fi] != '\0' && (literal || format[fi] != '|'))
    {
        if (format[fi] == '\'')
        {
            fi++;
            if (format[fi] != '\'')
            {
                literal = !literal;
                continue;
            }
        }
        if (literal)
        {
            str[si] = format[fi];
            fi++;
            si++;
        }
        else if (format[fi] == 'y' && format[fi + 1] == 'y' && format[fi + 2] == 'y' && format[fi + 3] == 'y')
        {
            if (year < 0 || year > 9999)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the year can not be represented using a positive four "
                               "digit number");
                return -1;
            }
            sprintf(&str[si], "%04d", year);
            fi += 4;
            si += 4;
        }
        else if (format[fi] == 'M' && format[fi + 1] == 'M')
        {
            if (month < 1 || month > 12)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the month value is not within range (1 - 12)");
                return -1;
            }
            if (format[fi + 2] == 'M')
            {
                sprintf(&str[si], "%s", month_name[month]);
                fi += 3;
                si += 3;
            }
            else
            {
                sprintf(&str[si], "%02d", month);
                fi += 2;
                si += 2;
            }
        }
        else if (format[fi] == 'd' && format[fi + 1] == 'd')
        {
            if (day < 1 || day > 31)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the day value is not within range (1 - 31)");
                return -1;
            }
            sprintf(&str[si], "%02d", day);
            fi += 2;
            si += 2;
        }
        else if (format[fi] == 'D' && format[fi + 1] == 'D' && format[fi + 2] == 'D')
        {
            int mjd, mjd_offset;

            if (dmy_to_mjd2000(day, month, year, &mjd) != 0)
            {
                return -1;
            }
            if (dmy_to_mjd2000(1, 1, year, &mjd_offset) != 0)
            {
                return -1;
            }
            sprintf(&str[si], "%03d", mjd - mjd_offset + 1);
            fi += 3;
            si += 3;
        }
        else if (format[fi] == 'H' && format[fi + 1] == 'H')
        {
            if (hour < 0 || hour > 23)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the hour value is not within range (0 - 23)");
                return -1;
            }
            sprintf(&str[si], "%02d", hour);
            fi += 2;
            si += 2;
        }
        else if (format[fi] == 'm' && format[fi + 1] == 'm')
        {
            if (minute < 0 || minute > 59)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the minute value is not within range (0 - 59)");
                return -1;
            }
            sprintf(&str[si], "%02d", minute);
            fi += 2;
            si += 2;
        }
        else if (format[fi] == 's' && format[fi + 1] == 's')
        {
            if (second < 0 || second > 60)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the second value is not within range (0 - 60)");
                return -1;
            }
            sprintf(&str[si], "%02d", second);
            fi += 2;
            si += 2;
        }
        else if (format[fi] == 'S')
        {
            int fraction = musec;
            int n = 0;
            int i;

            if (musec < 0 || musec > 999999)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "the microsecond value is not within range (0 - 999999)");
                return -1;
            }
            while (format[fi] == 'S')
            {
                fi++;
                n++;
            }
            for (i = n; i < 6; i++)
            {
                fraction /= 10;
            }
            sprintf(&str[si], "%0*d", n, fraction);
            si += n;
        }
        else if ((format[fi] >= 'A' && format[fi] <= 'Z') || (format[fi] >= 'a' && format[fi] <= 'z'))
        {
            /* reserved character */
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "unsuppored character sequence in date/time format (%s)", format);
            return -1;
        }
        else
        {
            str[si] = format[fi];
            fi++;
            si++;
        }
    }
    if (literal)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "missing closing ' in date/time format (%s)", format);
        return -1;
    }

    return 0;
}

static int string_to_parts(const char *format, const char *str, int *year, int *month, int *day, int *hour,
                           int *minute, int *second, int *musec)
{
    int use_leading_spaces;
    int format_index = 0;
    int string_index = 0;
    int literal = 0;
    int n;

    /* initialize with epoch 2000-01-01T00:00:00.000000 */
    *year = 2000;
    *month = 1;
    *day = 1;
    *hour = 0;
    *minute = 0;
    *second = 0;
    *musec = 0;

    while (format[format_index] != '\0' && (literal || format[format_index] != '|'))
    {
        if (format[format_index] == '\'')
        {
            format_index++;
            if (format[format_index] != '\'')
            {
                literal = !literal;
                continue;
            }
        }
        if (literal)
        {
            /* literal match */
            if (format[format_index] != str[string_index])
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect fixed character "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index++;
            string_index++;
        }
        else if (format[format_index] == 'y' && format[format_index + 1] == 'y' && format[format_index + 2] == 'y' &&
                 format[format_index + 3] == 'y')
        {
            use_leading_spaces = format[format_index + 4] == '*';
            if (parse_integer(&str[string_index], 4, use_leading_spaces, year) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect year value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 4 + (format[format_index] == '*');
            string_index += 4;
        }
        else if (format[format_index] == 'M' && format[format_index + 1] == 'M')
        {
            if (format[format_index + 2] == 'M')
            {
                /* coda_month_to_integer already limits comparison to only 3 characters */
                *month = coda_month_to_integer(&str[string_index]);
                if (*month == -1)
                {
                    coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect month value "
                                   "(format: %s)", str, format);
                    return -1;
                }
                format_index += 3;
                string_index += 3;
            }
            else
            {
                use_leading_spaces = format[format_index + 2] == '*';
                if (parse_integer(&str[string_index], 2, use_leading_spaces, month) != 0)
                {
                    coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect month value "
                                   "(format: %s)", str, format);
                    return -1;
                }
                format_index += 2 + (format[format_index + 2] == '*');
                string_index += 2;
            }
        }
        else if (format[format_index] == 'd' && format[format_index + 1] == 'd')
        {
            use_leading_spaces = format[format_index + 2] == '*';
            if (parse_integer(&str[string_index], 2, use_leading_spaces, day) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect day value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 2 + (format[format_index + 2] == '*');
            string_index += 2;
        }
        else if (format[format_index] == 'D' && format[format_index + 1] == 'D' && format[format_index + 2] == 'D')
        {
            int day_of_year;

            use_leading_spaces = format[format_index + 3] == '*';
            /* uses currently parsed year value to determine the actual month/day within the year */
            if (parse_integer(&str[string_index], 3, use_leading_spaces, &day_of_year) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect day value "
                               "(format: %s)", str, format);
                return -1;
            }
            if (coda_dayofyear_to_month_day(*year, day_of_year, month, day) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an invalid day value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 3 + (format[format_index + 3] == '*');
            string_index += 3;
        }
        else if (format[format_index] == 'H' && format[format_index + 1] == 'H')
        {
            use_leading_spaces = format[format_index + 2] == '*';
            if (parse_integer(&str[string_index], 2, use_leading_spaces, hour) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect hour value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 2 + (format[format_index + 2] == '*');
            string_index += 2;
        }
        else if (format[format_index] == 'm' && format[format_index + 1] == 'm')
        {
            use_leading_spaces = format[format_index + 2] == '*';
            if (parse_integer(&str[string_index], 2, use_leading_spaces, minute) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect minute value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 2 + (format[format_index + 2] == '*');
            string_index += 2;
        }
        else if (format[format_index] == 's' && format[format_index + 1] == 's')
        {
            use_leading_spaces = format[format_index + 2] == '*';
            if (parse_integer(&str[string_index], 2, use_leading_spaces, second) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect second value "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index += 2 + (format[format_index + 2] == '*');
            string_index += 2;
        }
        else if (format[format_index] == 'S')
        {
            n = 0;
            while (format[format_index] == 'S')
            {
                format_index++;
                n++;
            }
            if (parse_integer(&str[string_index], n > 6 ? 6 : n, 0, musec) != 0)
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect fractional "
                               "second value (format: %s)", str, format);
                return -1;
            }
            string_index += n;
            while (n < 6)
            {
                *musec *= 10;
                n++;
            }
        }
        else if ((format[format_index] >= 'A' && format[format_index] <= 'Z') ||
                 (format[format_index] >= 'a' && format[format_index] <= 'z') || format[format_index] == '*')
        {
            /* reserved character */
            coda_set_error(CODA_ERROR_INVALID_FORMAT, "unsuppored character sequence in date/time format (%s)", format);
            return -1;
        }
        else
        {
            /* literal match */
            if (format[format_index] != str[string_index])
            {
                coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) has an incorrect fixed character "
                               "(format: %s)", str, format);
                return -1;
            }
            format_index++;
            string_index++;
        }
    }
    if (literal)
    {
        coda_set_error(CODA_ERROR_INVALID_FORMAT, "missing closing ' in date/time format (%s)", format);
        return -1;
    }
    if (str[string_index] != '\0')
    {
        coda_set_error(CODA_ERROR_INVALID_DATETIME, "date/time argument (%s) contains additional characters "
                       "(format: %s)", str, format);
        return -1;
    }

    return 0;
}

/** Convert a time string to a date and time using a specified format.
 * The string will be parsed using the format that is provided as first parameter. This can be a '|' separated list
 * of formats that will be tried in sequence until one succeeds.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * \warning This function does not perform any leap second correction.
 * \param format String containing the datetime format(s) to use for parsing the datetime value.
 * \param str    String containing the time in one of the supported formats.
 * \param year   Pointer to the variable where the year will be stored.
 * \param month  Pointer to the variable where the month of the year (1 - 12) will be stored.
 * \param day    Pointer to the variable where the day of the month (1 - 31) will be stored.
 * \param hour   Pointer to the variable where the hour of the day (0 - 23) will be stored.
 * \param minute Pointer to the variable where the minute of the hour (0 - 59) will be stored.
 * \param second Pointer to the variable where the second of the minute (0 - 59) will be stored.
 * \param musec  Pointer to the variable where the microseconds of the second (0 - 999999) will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_string_to_parts(const char *format, const char *str, int *year, int *month, int *day,
                                          int *hour, int *minute, int *second, int *musec)
{
    int literal = 0;
    int n = 0;

    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (str == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "str argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (year == NULL || month == NULL || day == NULL || hour == NULL || minute == NULL || second == NULL ||
        musec == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "date/time argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    while (format[n] != '\0' && (literal || format[n] != '|'))
    {
        if (format[n] == '\'')
        {
            literal = !literal;
        }
        n++;
    }
    if (format[n] == '|')
    {
        int offset = 0;

        /* try multiple formats */
        while (1)
        {
            if (string_to_parts(&format[offset], str, year, month, day, hour, minute, second, musec) == 0)
            {
                /* found a format that works */
                return 0;
            }
            if (format[n] == '\0')
            {
                /* the string matched none of the formats */
                coda_set_error(CODA_ERROR_INVALID_DATETIME,
                               "date/time argument (%s) did not match any of the formats (%s)", str, format);
                return -1;
            }
            n++;
            offset = n;
            while (format[n] != '\0' && (literal || format[n] != '|'))
            {
                if (format[n] == '\'')
                {
                    literal = !literal;
                }
                n++;
            }
        }
    }

    return string_to_parts(format, str, year, month, day, hour, minute, second, musec);
}

/** Convert a floating point time value to a string using a specified format.
 * The string will be formatted using the format that is provided as second parameter.
 * The time string will be stored in the \a str parameter. This parameter should be allocated by the user
 * and should be long enough to hold the formatted time string and a 0 termination character.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * \warning This function does not perform any leap second correction.
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \param format    Date/time format to use for the string representation of the datetime value.
 * \param str       String representation of the floating point time value.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
/* the date/time formats that we support guarantee that strlen(format) >= strlen(str) for all formats */
LIBCODA_API int coda_time_double_to_string(double datetime, const char *format, char *str)
{
    int year, month, day, hour, minute, second, musec;

    if (coda_time_double_to_parts(datetime, &year, &month, &day, &hour, &minute, &second, &musec) != 0)
    {
        return -1;
    }

    return coda_time_parts_to_string(year, month, day, hour, minute, second, musec, format, str);
}

/** Convert a floating point TAI time value to a UTC string.
 * The string will be formatted using the format that is provided as second parameter.
 * The time string will be stored in the \a str parameter. This parameter should be allocated by the user
 * and should be long enough to hold the formatted time string and a 0 termination character.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * This function performs proper leap second correction in the conversion from TAI to UTC
 * (see also \a coda_time_double_to_parts_utc()).
 *
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \param format    Date/time format to use for the string representation of the datetime value.
 * \param str       String representation of the floating point time value.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
/* the date/time formats that we support guarantee that strlen(format) >= strlen(str) for all formats */
LIBCODA_API int coda_time_double_to_string_utc(double datetime, const char *format, char *str)
{
    int year, month, day, hour, minute, second, musec;

    if (coda_time_double_to_parts_utc(datetime, &year, &month, &day, &hour, &minute, &second, &musec) != 0)
    {
        return -1;
    }

    return coda_time_parts_to_string(year, month, day, hour, minute, second, musec, format, str);
}

/** Convert a time string to a floating point time value.
 * The string will be parsed using the format that is provided as first parameter. This can be a '|' separated list
 * of formats that will be tried in sequence until one succeeds.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * \warning This function does not perform any leap second correction.
 * \param format    String containing the datetime format(s) to use for parsing the datetime value.
 * \param str       String containing the time in one of the supported formats.
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_time_string_to_double(const char *format, const char *str, double *datetime)
{
    int year, month, day, hour, minute, second, musec;

    if (coda_time_string_to_parts(format, str, &year, &month, &day, &hour, &minute, &second, &musec) != 0)
    {
        return -1;
    }
    return coda_time_parts_to_double(year, month, day, hour, minute, second, musec, datetime);
}

/** Convert a UTC time string to a TAI floating point time value.
 * The string will be parsed using the format that is provided as first parameter. This can be a '|' separated list
 * of formats that will be tried in sequence until one succeeds.
 *
 * The specification for the time format parameter is the same as the
 * <a href="../codadef/codadef-expressions.html#timeformat">date/time format patterns in coda expressions</a>.
 *
 * This function performs proper leap second correction in the conversion from UTC to TAI
 * (see also \a coda_time_parts_to_double_utc()).
 *
 * \param format    String containing the datetime format(s) to use for parsing the datetime value.
 * \param str       String containing the time in one of the supported formats.
 * \param datetime  Floating point value representing the number of seconds since January 1st, 2000 00:00:00.000000.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
int coda_time_string_to_double_utc(const char *format, const char *str, double *datetime)
{
    int year, month, day, hour, minute, second, musec;

    if (coda_time_string_to_parts(format, str, &year, &month, &day, &hour, &minute, &second, &musec) != 0)
    {
        return -1;
    }
    return coda_time_parts_to_double_utc(year, month, day, hour, minute, second, musec, datetime);
}


/* Deprecated backward compatibility functions */

LIBCODA_API int coda_datetime_to_double(int year, int month, int day, int hour, int minute, int second, int musec,
                                        double *datetime)
{
    return coda_time_parts_to_double(year, month, day, hour, minute, second, musec, datetime);
}

LIBCODA_API int coda_double_to_datetime(double datetime, int *year, int *month, int *day, int *hour, int *minute,
                                        int *second, int *musec)
{
    return coda_time_double_to_parts(datetime, year, month, day, hour, minute, second, musec);
}

LIBCODA_API int coda_time_to_string(double datetime, char *str)
{
    return coda_time_double_to_string(datetime, "yyyy-MM-dd HH:mm:ss.SSSSSS", str);
}

LIBCODA_API int coda_string_to_time(const char *str, double *datetime)
{
    const char *format = "yyyy-MM-dd HH:mm:ss.SSSSSS|yyyy-MM-dd HH:mm:ss|yyyy-MM-dd|"
        "dd-MMM-yyyy HH:mm:ss.SSSSSS|dd-MMM-yyyy HH:mm:ss|dd-MMM-yyyy";
    return coda_time_string_to_double(format, str, datetime);
}

LIBCODA_API int coda_utcdatetime_to_double(int year, int month, int day, int hour, int minute, int second, int musec,
                                           double *datetime)
{
    return coda_time_parts_to_double_utc(year, month, day, hour, minute, second, musec, datetime);
}

LIBCODA_API int coda_double_to_utcdatetime(double datetime, int *year, int *month, int *day, int *hour, int *minute,
                                           int *second, int *musec)
{
    return coda_time_double_to_parts_utc(datetime, year, month, day, hour, minute, second, musec);
}

LIBCODA_API int coda_time_to_utcstring(double datetime, char *str)
{
    return coda_time_double_to_string_utc(datetime, "yyyy-MM-dd HH:mm:ss.SSSSSS", str);
}

LIBCODA_API int coda_utcstring_to_time(const char *str, double *datetime)
{
    const char *format = "yyyy-MM-dd HH:mm:ss.SSSSSS|yyyy-MM-dd HH:mm:ss|yyyy-MM-dd|"
        "dd-MMM-yyyy HH:mm:ss.SSSSSS|dd-MMM-yyyy HH:mm:ss|dd-MMM-yyyy";
    return coda_time_string_to_double_utc(format, str, datetime);
}

/** @} */
