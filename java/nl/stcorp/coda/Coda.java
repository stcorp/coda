//
// Copyright (C) 2007-2010 S[&]T, The Netherlands.
//
// This file is part of CODA.
//
// CODA is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// CODA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CODA; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

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
     * @warning This function does _not_ perform any leap second correction. The
     *          returned value is therefore not an exact UTC time
     * @param YEAR
     *            The year.
     * @param MONTH
     *            The month of the year (1 - 12).
     * @param DAY
     *            The day of the month (1 - 31).
     * @param HOUR
     *            The hour of the day (0 - 23).
     * @param MINUTE
     *            The minute of the hour (0 - 59).
     * @param SECOND
     *            The second of the minute (0 - 59).
     * @param MUSEC
     *            The microseconds of the second (0 - 999999).
     * @return Variable where the amount of seconds since Jan
     *            1st 2000 will be stored.
     * @throws CodaException
     *             If an error occurred.
     */
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


    /**
     * Retrieve the decomposed date corresponding with the given amount of
     * seconds since Jan 1st 2000.
     * 
     * @warning This function does _not_ perform any leap second correction. The
     *          returned value is therefore not an exact UTC time
     * @param datetime
     * @return A 7-element array containing the following representation of 
     *            the date:
     * \arg \c [0] YEAR - The year.
     * \arg \c [1] MONTH - The month of the year (1 - 12)
     * \arg \c [2] DAY - The day of the month (1 - 31)
     * \arg \c [3] HOUR - The hour of the day (0 - 23)
     * \arg \c [4] MINUTE - The minute of the hour (0 - 59)
     * \arg \c [5] SECOND - The second of the minute (0 - 59)
     * \arg \c [6] MUSEC - The microseconds of the second (0 - 999999)
     * @throws CodaException
     *             If an error occurred.
     */
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


    /**
     * Convert a floating point time value to a string.
     * 
     * @param datetime
     *            Floating point value representing the number of seconds since
     *            January 1st, 2000 00:00:00.000000.
     * @return String representation of the floating point time value.
     * @throws CodaException
     *             If an error occurred.
     */
    public static String time_to_string(double datetime) throws CodaException
    {
        return codac.helper_coda_time_to_string(datetime);
    }


    /**
     * Convert a time string to a floating point time value.
     * 
     * @param str
     *            String containing the time in one of the supported formats.
     * @return datetime Floating point value representing the number of seconds
     *         since January 1st, 2000 00:00:00.000000.
     * @throws CodaException
     *             If an error occurred.
     */
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
     * @param enable \arg 0: Disable the
     * use of memory mapping. \arg 1: Enable the use of memory mapping.
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
     * Initializes CODA. /**
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
