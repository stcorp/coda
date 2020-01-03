// Copyright (C) 2007-2020 S[&]T, The Netherlands.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

package nl.stcorp.coda;

/**
 * CODA General class.
 * 
 * The CODA general class 'Coda' contains the global CODA utility
 * functions that do not require a specific CODA data structure.
 * 
 * It is not possible or necessary to construct an instance of the Coda class.
 * All methods in this class are static class methods, and can be invoked as
 * 'Coda.methodName()'.
 * 
 */
public class Coda
{
    // Suppress default constructor to ensure
    // non-instantiability
    private Coda()
    {
        throw new AssertionError();
    }


    /**
     * Convert an index for a multidimensional array that is stored in C-style
     * order to an index for an identical array stored in Fortran-style order.
     * 
     * @param dim
     *            Number of dimensions of the multidimensional array.
     * @param index
     *            \c C style index.
     * @return \c Fortran style index.
     * @throws CodaException
     *             If an error occurred.
     */
    public static int c_index_to_fortran_index(int[] dim, int index) throws CodaException
    {
        return codac.c_index_to_fortran_index(dim.length, dim, index);
    }


    /**
     * Find out whether a double value equals NaN (Not a Number).
     * 
     * @param x
     *            A double value.
     * @return \arg \c 1, The double value equals NaN. \arg \c 0, The double
     *         value does not equal NaN.
     */
    public static int isNaN(double x)
    {
        return codac.isNaN(x);
    }


    /**
     * Retrieve a double value that respresents NaN (Not a Number).
     * 
     * @return The double value 'NaN'.
     */
    public static double NaN()
    {
        return codac.NaN();
    }


    /**
     * Find out whether a double value equals inf (either positive or negative
     * infinity).
     * 
     * @param x
     *            A double value.
     * @return \arg \c 1, The double value equals inf. \arg \c 0, The double
     *         value does not equal inf.
     */
    public static int isInf(double x)
    {
        return codac.isInf(x);
    }


    /**
     * Find out whether a double value equals +inf (positive infinity).
     * 
     * @param x
     *            A double value.
     * @return \arg \c 1, The double value equals +inf. \arg \c 0, The double
     *         value does not equal +inf.
     */
    public static int isPlusInf(double x)
    {

        return codac.isPlusInf(x);
    }


    /**
     * Find out whether a double value equals -inf (negative infinity).
     * 
     * @param x
     *            A double value.
     * @return \arg \c 1, The double value equals -inf. \arg \c 0, The double
     *         value does not equal -inf.
     */
    public static int isMinInf(double x)
    {

        return codac.isMinInf(x);
    }


    /**
     * Retrieve a double value that respresents +inf (positive infinity).
     * 
     * @return The double value '+inf'.
     */
    public static double PlusInf()
    {
        return codac.PlusInf();
    }


    /**
     * Retrieve a double value that respresents -inf (negative infinity).
     * 
     * @return The double value '-inf'.
     */
    public static double MinInf()
    {
        return codac.MinInf();
    }


    /**
     * Retrieve the number of seconds since Jan 1st 2000 for a certain date and
     * time.
     * 
     * @warning This function does _not_ perform any leap second correction.
     *          The returned value is just a straightforward conversion using
     *          86400 seconds per day.
     * @param year
     *            The year.
     * @param month
     *            The month of the year (1 - 12).
     * @param day
     *            The day of the month (1 - 31).
     * @param hour
     *            The hour of the day (0 - 23).
     * @param minute
     *            The minute of the hour (0 - 59).
     * @param second
     *            The second of the minute (0 - 59).
     * @param musec
     *            The microseconds of the second (0 - 999999).
     * @return Variable where the amount of seconds since Jan
     *            1st 2000 will be stored.
     * @throws CodaException
     *             If an error occurred.
     */
    public static double time_parts_to_double(int year, int month, int day,
            int hour, int minute, int second, int musec) throws CodaException
    {
        double[] datetime = new double[1];
        codac.time_parts_to_double(year,
                month,
                day,
                hour,
                minute,
                second,
                musec,
                datetime);
        return datetime[0];
    }


    /**
     * Retrieve the number of TAI seconds since Jan 1st 2000 for a certain
     * UTC date and time using leap second correction. This function assumes
     * the input to be an UTC datetime. The returned value will be the 
     * seconds since 2000-01-01 in the TAI time system (using proper leap
     * second handling for the UTC to TAI conversion).
     * For example:
     * 1972-01-01 00:00:00 UTC will be -883612790
     * 2000-01-01 00:00:00 UTC will be 32
     * 2008-12-31 23:59:59 UTC will be 284083232
     * 2008-12-31 23:59:60 UTC will be 284083233
     * 2009-01-01 00:00:00 UTC will be 284083234
     * @warning For dates before 1972-01-01 UTC a fixed leap second offset of
     *          10 is used.
     * @note CODA has a built in table of leap seconds. To use a more recent
     *       leap second table, download the most recent file from
     *       ftp://maia.usno.navy.mil/ser7/tai-utc.dat and set the environment
     *       variable CODA_LEAP_SECOND_TABLE with a full path to this file.
     * @param year
     *            The year.
     * @param month
     *            The month of the year (1 - 12).
     * @param day
     *            The day of the month (1 - 31).
     * @param hour
     *            The hour of the day (0 - 23).
     * @param minute
     *            The minute of the hour (0 - 59).
     * @param second
     *            The second of the minute (0 - 59).
     * @param musec
     *            The microseconds of the second (0 - 999999).
     * @return Variable where the amount of seconds since Jan
     *            1st 2000 will be stored.
     * @throws CodaException
     *             If an error occurred.
     */
    public static double time_parts_to_double_utc(int year, int month, int day,
            int hour, int minute, int second, int musec) throws CodaException
    {
        double[] datetime = new double[1];
        codac.time_parts_to_double_utc(year,
                month,
                day,
                hour,
                minute,
                second,
                musec,
                datetime);
        return datetime[0];
    }


    /**
     * Retrieve the decomposed date corresponding with the given amount of
     * seconds since Jan 1st 2000.
     * 
     * @warning This function does _not_ perform any leap second correction.
     *          The returned value is just a straightforward conversion using
     *          86400 seconds per day.
     * @param datetime
     *            Floating point value representing the number of seconds since
     *            January 1st, 2000 00:00:00.000000.
     * @return A 7-element array containing the following representation of 
     *            the date:
     * \arg \c [0] year - The year.
     * \arg \c [1] month - The month of the year (1 - 12)
     * \arg \c [2] day - The day of the month (1 - 31)
     * \arg \c [3] hour - The hour of the day (0 - 23)
     * \arg \c [4] minute - The minute of the hour (0 - 59)
     * \arg \c [5] second - The second of the minute (0 - 59)
     * \arg \c [6] musec - The microseconds of the second (0 - 999999)
     * @throws CodaException
     *             If an error occurred.
     */
    public static int[] time_double_to_parts(double datetime) throws CodaException
    {
        int[] year = new int[1];
        int[] month = new int[1];
        int[] day = new int[1];
        int[] hour = new int[1];
        int[] minute = new int[1];
        int[] second = new int[1];
        int[] musec = new int[1];

        codac.time_double_to_parts(datetime,
                year,
                month,
                day,
                hour,
                minute,
                second,
                musec);
        int[] result = new int[7];
        result[0] = year[0];
        result[1] = month[0];
        result[2] = day[0];
        result[3] = hour[0];
        result[4] = minute[0];
        result[5] = second[0];
        result[6] = musec[0];
        return result;
    }


    /**
     * Retrieve the decomposed UTC date corresponding with the given amount of
     * TAI seconds since Jan 1st 2000.
     * This function assumes the input to by the number of seconds since
     * 2000-01-01 in the TAI system. The returned date/time components will be
     * the corresponding UTC datetime (using proper leap second handling for
     * the TAI to UTC conversion).
     * For example:
     * -88361290 will be 1972-01-01 00:00:00 UTC
     * 0 will be 1999-31-12 23:59:28 UTC
     * 284083232 will be 2008-12-31 23:59:59 UTC
     * 284083233 will be 2008-12-31 23:59:60 UTC
     * 284083234 will be 2009-01-01 00:00:00 UTC
     * @warning For dates before 1972-01-01 UTC a fixed leap second offset of
     *          10 is used.
     * @note CODA has a built in table of leap seconds. To use a more recent
     *       leap second table, download the most recent file from
     *       ftp://maia.usno.navy.mil/ser7/tai-utc.dat and set the environment
     *       variable CODA_LEAP_SECOND_TABLE with a full path to this file.
     * @param datetime
     *            Floating point value representing the number of seconds since
     *            January 1st, 2000 00:00:00.000000.
     * @return A 7-element array containing the following representation of 
     *            the date:
     * \arg \c [0] year - The year.
     * \arg \c [1] month - The month of the year (1 - 12)
     * \arg \c [2] day - The day of the month (1 - 31)
     * \arg \c [3] hour - The hour of the day (0 - 23)
     * \arg \c [4] minute - The minute of the hour (0 - 59)
     * \arg \c [5] second - The second of the minute (0 - 59)
     * \arg \c [6] musec - The microseconds of the second (0 - 999999)
     * @throws CodaException
     *             If an error occurred.
     */
    public static int[] time_double_to_parts_utc(double datetime) throws CodaException
    {
        int[] year = new int[1];
        int[] month = new int[1];
        int[] day = new int[1];
        int[] hour = new int[1];
        int[] minute = new int[1];
        int[] second = new int[1];
        int[] musec = new int[1];

        codac.time_double_to_parts(datetime,
                year,
                month,
                day,
                hour,
                minute,
                second,
                musec);
        int[] result = new int[7];
        result[0] = year[0];
        result[1] = month[0];
        result[2] = day[0];
        result[3] = hour[0];
        result[4] = minute[0];
        result[5] = second[0];
        result[6] = musec[0];
        return result;
    }


    /**
     * Create a string representation for a specific data and time.
     * The string will be formatted using the format that is provided as
     * first parameter. The time string will be stored in the \a str
     * parameter. This parameter should be allocated by the user and should
     * be long enough to hold the formatted time string and a 0 termination
     * character.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     *
     * @param year
     *            The year.
     * @param month
     *            The month of the year (1 - 12).
     * @param day
     *            The day of the month (1 - 31).
     * @param hour
     *            The hour of the day (0 - 23).
     * @param minute
     *            The minute of the hour (0 - 59).
     * @param second
     *            The second of the minute (0 - 59).
     * @param musec
     *            The microseconds of the second (0 - 999999).
     * @param format
     *            Date/time format to use for the string representation of
     *            the datetime value.
     * @return Variable where the amount of seconds since Jan
     *            1st 2000 will be stored.
     * @throws CodaException
     *             If an error occurred.
     */
    public static String time_parts_to_string(int year, int month, int day,
            int hour, int minute, int second, int musec, String format) throws CodaException
    {
        return codac.helper_coda_time_parts_to_string(year,
                month,
                day,
                hour,
                minute,
                second,
                musec,
                format);
    }

    /**
     * Convert a time string to a date and time using a specified format.
     * The string will be parsed using the format that is provided as first
     * parameter. This can be a '|' separated list of formats that will be
     * tried in sequence until one succeeds.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     *
     * @param format
     *            Date/time format to use for the string representation of the
     *            datetime value.
     * @param str
     *            String representation of the floating point time value.
     * @return A 7-element array containing the following representation of 
     *            the date:
     * \arg \c [0] year - The year.
     * \arg \c [1] month - The month of the year (1 - 12)
     * \arg \c [2] day - The day of the month (1 - 31)
     * \arg \c [3] hour - The hour of the day (0 - 23)
     * \arg \c [4] minute - The minute of the hour (0 - 59)
     * \arg \c [5] second - The second of the minute (0 - 59)
     * \arg \c [6] musec - The microseconds of the second (0 - 999999)
     * @throws CodaException
     *             If an error occurred.
     */
    public static int[] time_string_to_parts(String format, String str) throws CodaException
    {
        int[] year = new int[1];
        int[] month = new int[1];
        int[] day = new int[1];
        int[] hour = new int[1];
        int[] minute = new int[1];
        int[] second = new int[1];
        int[] musec = new int[1];

        codac.time_string_to_parts(format,
                str,
                year,
                month,
                day,
                hour,
                minute,
                second,
                musec);
        int[] result = new int[7];
        result[0] = year[0];
        result[1] = month[0];
        result[2] = day[0];
        result[3] = hour[0];
        result[4] = minute[0];
        result[5] = second[0];
        result[6] = musec[0];
        return result;
    }

    /**
     * Convert a floating point time value to a string using a specified
     * format.
     * The string will be formatted using the format that is provided as second
     * parameter.
     * The time string will be stored in the \a str parameter. This parameter
     * should be allocated by the user and should be long enough to hold the
     * formatted time string and a 0 termination character.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     * 
     * @param datetime
     *            Floating point value representing the number of seconds since
     *            January 1st, 2000 00:00:00.000000.
     * @param format
     *            Date/time format to use for the string representation of the
     *            datetime value.
     * @return String representation of the floating point time value.
     * @throws CodaException
     *             If an error occurred.
     */
    public static String time_double_to_string(double datetime, String format) throws CodaException
    {
        return codac.helper_coda_time_double_to_string(datetime, format);
    }

    /**
     * Convert a floating point TAI time value to a UTC string.
     * The string will be formatted using the format that is provided as
     * second parameter.
     * The time string will be stored in the \a str parameter. This parameter
     * should be allocated by the user and should be long enough to hold the
     * formatted time string and a 0 termination character.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     *
     * This function performs proper leap second correction in the conversion
     * from TAI to UTC (see also \a time_double_to_parts_utc()).
     * 
     * @param datetime
     *            Floating point value representing the number of seconds since
     *            January 1st, 2000 00:00:00.000000.
     * @param format
     *            Date/time format to use for the string representation of the
     *            datetime value.
     * @return String representation of the floating point time value.
     * @throws CodaException
     *             If an error occurred.
     */
    public static String time_double_to_string_utc(double datetime, String format) throws CodaException
    {
        return codac.helper_coda_time_double_to_string_utc(datetime, format);
    }

    /**
     * Convert a time string to a floating point time value.
     * The string will be parsed using the format that is provided as first
     * parameter. This can be a '|' separated list of formats that will be
     * tried in sequence until one succeeds.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     *
     * @param format
     *            Date/time format to use for the string representation of the
     *            datetime value.
     * @param str
     *            String containing the time in one of the supported formats.
     * @return datetime Floating point value representing the number of seconds
     *         since January 1st, 2000 00:00:00.000000.
     * @throws CodaException
     *             If an error occurred.
     */
    public static double time_string_to_double(String format, String str) throws CodaException
    {
        double[] datetime = new double[1];
        codac.time_string_to_double(format,	 str, datetime);
        return datetime[0];
    }

    /**
     * Convert a UTC time string to a TAI floating point time value.
     * The string will be parsed using the format that is provided as first
     * parameter. This can be a '|' separated list of formats that will be
     * tried in sequence until one succeeds.
     *
     * The specification for the time format parameter is the same as the
     * <a href="../codadef/codadef-expressions.html#timeformat">date/time
     * format patterns in coda expressions</a>.
     *
     * This function performs proper leap second correction in the conversion
     * from UTC to TAI (see also \a time_parts_to_double_utc()).
     *
     * @param format
     *            Date/time format to use for the string representation of the
     *            datetime value.
     * @param str
     *            String containing the time in one of the supported formats.
     * @return datetime Floating point value representing the number of seconds
     *         since January 1st, 2000 00:00:00.000000.
     * @throws CodaException
     *             If an error occurred.
     */
    public static double time_string_to_double_utc(String format, String str) throws CodaException
    {
        double[] datetime = new double[1];
        codac.time_string_to_double_utc(format,	 str, datetime);
        return datetime[0];
    }

    /* deprecated datetime functions */
    public static double datetime_to_double(int YEAR, int MONTH, int DAY,
            int HOUR, int MINUTE, int SECOND, int MUSEC) throws CodaException
    {
        double[] datetime = new double[1];
        codac.datetime_to_double(YEAR,
                MONTH,
                DAY,
                HOUR,
                MINUTE,
                SECOND,
                MUSEC,
                datetime);
        return datetime[0];
    }


    public static int[] double_to_datetime(double datetime) throws CodaException
    {
        int[] YEAR = new int[1];
        int[] MONTH = new int[1];
        int[] DAY = new int[1];
        int[] HOUR = new int[1];
        int[] MINUTE = new int[1];
        int[] SECOND = new int[1];
        int[] MUSEC = new int[1];

        codac.double_to_datetime(datetime,
                YEAR,
                MONTH,
                DAY,
                HOUR,
                MINUTE,
                SECOND,
                MUSEC);
        int[] result = new int[7];
        result[0] = YEAR[0];
        result[1] = MONTH[0];
        result[2] = DAY[0];
        result[3] = HOUR[0];
        result[4] = MINUTE[0];
        result[5] = SECOND[0];
        result[6] = MUSEC[0];
        return result;
    }


    public static String time_to_string(double datetime) throws CodaException
    {
        return codac.helper_coda_time_to_string(datetime);
    }


    public static double string_to_time(String str) throws CodaException
    {
        double[] datetime = new double[1];
        codac.string_to_time(str, datetime);
        return datetime[0];
    }


    /**
     * Enable/Disable the use of special types.
     * 
     * @param enable
     *            \arg 0: Disable bypassing of special types. \arg 1: Enable
     *            bypassing of special types.
     * @throws CodaException
     *             If an error occurred.
     */
    public static void set_option_bypass_special_types(int enable) throws CodaException
    {

        codac.set_option_bypass_special_types(enable);
    }


    /**
     * Retrieve the current setting for the special types bypass option.
     * 
     * @return \arg \c 0, Bypassing of special types is disabled. \arg \c 1,
     *         Bypassing of special types is enabled.
     */
    public static int get_option_bypass_special_types()
    {
        return codac.get_option_bypass_special_types();
    }


    /**
     * Enable/Disable boundary checking.
     * 
     * @param enable
     *            \arg 0: Disable boundary checking. \arg 1: Enable boundary
     *            checking.
     * @throws CodaException
     *             If an error occurred.
     */
    public static void set_option_perform_boundary_checks(int enable) throws CodaException
    {
        codac.set_option_perform_boundary_checks(enable);
    }


    /**
     * Retrieve the current setting for the boundary check option.
     * 
     * @return \arg \c 0, Boundary checking is disabled. \arg \c 1, Boundary
     *         checking is enabled.
     */
    public static int get_option_perform_boundary_checks()
    {
        return codac.get_option_perform_boundary_checks();
    }


    /**
     * Enable/Disable unit/value conversions.
     * 
     * @param enable
     *            \arg 0: Disable unit/value conversions. \arg 1: Enable
     *            unit/value conversions.
     * @throws CodaException
     *             If an error occurred.
     */
    public static void set_option_perform_conversions(int enable) throws CodaException
    {

        codac.set_option_perform_conversions(enable);
    }


    /**
     * Retrieve the current setting for the value/unit conversion option.
     * 
     * @return \arg \c 0, Unit/value conversions are disabled. \arg \c 1,
     *         Unit/value conversions are enabled.
     */
    public static int get_option_perform_conversions()
    {

        return codac.get_option_perform_conversions();
    }


    /**
     * Enable/Disable the use of fast size expressions.
     * 
     * @param enable
     *            \arg 0: Disable the use of fast size expressions. \arg 1:
     *            Enable the use of fast size expressions.
     * @throws CodaException
     *             If an error occurred.
     */
    public static void set_option_use_fast_size_expressions(int enable) throws CodaException
    {
        codac.set_option_use_fast_size_expressions(enable);
    }


    /**
     * Retrieve the current setting for the use of fast size expressions option.
     * 
     * @return \arg \c 0, Unit/value conversions are disabled. \arg \c 1,
     *         Unit/value conversions are enabled.
     */
    public static int get_option_use_fast_size_expressions()
    {

        return codac.get_option_use_fast_size_expressions();
    }


    /**
     * Enable/Disable the use of memory mapping of files. 
     * 
     * @param enable
     *            \arg 0: Disable the use of memory mapping. \arg 1: Enable
     *            the use of memory mapping.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public static void set_option_use_mmap(int enable) throws CodaException
    {

        codac.set_option_use_mmap(enable);
    }


    /**
     * Retrieve the current setting for the use of memory mapping of files.
     * 
     * @return \arg \c 0, Memory mapping of files is disabled. \arg \c 1, Memory
     *         mapping of files is enabled.
     */
    public static int get_option_use_mmap()
    {

        return codac.get_option_use_mmap();
    }


    /**
     * Set the searchpath for CODA product definition files.
     * 
     * @param path
     *            Search path for .codadef files
     * @throws CodaException
     *             If an error occurred.
     */
    public static void setDefinitionPath(String path) throws CodaException
    {
        codac.set_definition_path(path);
    }


    /**
     * Set the location of CODA product definition file(s) based on the location of another file.
     * 
     * @param file
     *            Filename of the file to search for
     * @param searchpath
     *            Search path where to look for the file
     * @param relativeLocation
     *            Filepath relative to the directory from searchpath where file was found that should be used to
     *            determine the CODA definition path
     * @throws CodaException
     *             If an error occurred.
     */
    public static void setDefinitionPathConditional(String file, String searchpath, String relativeLocation) throws CodaException
    {
        codac.set_definition_path_conditional(file, searchpath, relativeLocation);
    }
    

    /**
     * Initializes CODA.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public static void init() throws CodaException
    {
        codac.init();
    }


    /**
     * Finalizes CODA.
     * 
     */
    public static void done()
    {
        codac.done();
    }


    // This function does not exist in libcoda, but is a
    // wrapper around a global variable.
    /**
     * Current version of CODA as a string.
     * 
     * @return The CODA version.
     */
    public static String version()
    {
        return codac.helper_version();
    }

    // CODA General methods not exposed in Java yet:
    //
    // coda_match_filefilter
}
