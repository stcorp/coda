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

#include "coda-internal.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
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

/** \defgroup coda_cursor CODA Cursor
 * After you have opened a product file with coda_open() (see \link coda_productfile CODA Product File\endlink) you will
 * want to access data from this product and retrieve metadata for the data elements (see \link coda_types CODA 
 * Types\endlink). In order to do this, CODA provides the concept of a 'cursor'. A cursor can be thought
 * of as something that keeps track of a position in the product file and it also stores some extra (type) information
 * about the data element it is currently pointing to. Cursors will start their useful life at the 'root' of a product,
 * i.e., pointing to the entire product, with a type that accurately describes the entire product.
 * From there you can navigate the cursor to the specific data element(s) you want to access.
 * Note that cursors are used for all products that can be opened with CODA. This includes files in ascii, binary, XML,
 * netCDF, HDF4, or HDF5 format.
 *
 * You can initialize a cursor to point to the product root via the following function:
 *
 * - coda_cursor_set_product()
 *
 * Suppose that we want to read the absolute orbit number value from the MPH of an ESA ENVISAT product file. In order
 * to do this, we first have to use the coda_cursor_set_product() function to initialize a cursor to point to the
 * complete product.
 * As explained in the CODA Types section (see \link coda_types CODA Types\endlink) all data elements of a product file
 * can be categorized in arrays, records, and basic types (int16, double, complex float, etc.). The product root of an
 * ENVISAT file is a record, which means that we are able to call the coda_cursor_goto_record_field_by_name() to
 * navigate to the MPH.
 * The MPH to which the cursor is then pointing is again a record, so we can call the
 * coda_cursor_goto_record_field_by_name() function again to move the cursor to a certain MPH field. We are interested
 * in the absolute orbit, so if we call this function with the fieldname "abs_orbit". Our cursor will now point to the
 * location in the product file that contains the value we want.
 *
 * Once we have moved the cursor to the right location we have to read the data that it is pointing to.
 * In order to do that, we first want to find out the best native type to read the absolute orbit value.
 * You can either determine this by looking it up in the CODA Product Format Definition documentation for the product
 * or you can use the coda_cursor_get_read_type() on the cursor. Both methods will show you that the absolute orbit is
 * stored as a \c int32 value.
 *
 * Since the value is a signed 32-bit integer, we will call coda_cursor_read_int32() to read the absolute orbit value
 * (Notice that this function returns the value as an int32_t type. CODA use these bit-specific types because not every
 * platform that CODA runs on uses the same amount of bits for the C types short, int, and long).
 * It is however also possible to read the 32-bit integer using a CODA read function that returns a native type that is
 * \e larger than an int32. For instance, you can also read the 32-bit integer value using coda_cursor_read_int64() or
 * coda_cursor_read_double(), which would return the value as an int64_t or double value (you can't however read the 
 * signed 32 bit integer using an unsigned integer type (e.g. uint32) or using a shorter type (e.g. int16)).
 *
 * A small example that performs all these steps and prints the retrieved orbit number is given below. Note
 * that, for the sake of clarity, we omit error checking:
 * \code
 * coda_ProductFile *pf;
 * coda_Cursor cursor;
 * int32_t abs_orbit_val;
 * coda_init();
 * pf = coda_open("... path to envisat product file ...");
 * coda_cursor_set_product(&cursor, pf);
 * coda_cursor_goto_record_field_by_name(&cursor, "mph");
 * coda_cursor_goto_record_field_by_name(&cursor, "abs_orbit");
 * coda_cursor_read_int32(&cursor, &abs_orbit_val);
 * printf("absolute orbit: %ld\n", (long)abs_orbit_val);
 * coda_close(pf);
 * coda_done();
 * \endcode
 *
 * After you have moved a cursor to a specific data element, it is possible to reuse the cursor and move it to other
 * data elements. Suppose that, after reading the absolute orbit, we now want to read the relative orbit. In order to do
 * that we first we have to go back to the MPH record. To have the cursor move to its encapsulating record or array you
 * can use the coda_cursor_goto_parent() function. After that, we can call coda_cursor_goto_record_field_by_name() again
 * but now with the string "rel_orbit" as fieldname parameter.
 *
 * An important aspect of using cursors is that you do not have to clean up a cursor. Memory can be reserved for a
 * cursor simply by declaring it; the initialization of the cursor with a coda_cursor_set_product() function does not
 * require any memory allocation. Another advantage of this kind of implementation is that you can easily make a copy
 * of a cursor. Suppose we have a cursor \c record_cursor that points to a record and we want to have an extra cursor
 * \c field_cursor that points to the 'dsr_time' field of this record. This can be done as follows:
 * \code
 * coda_Cursor field_cursor;
 * field_cursor = record_cursor;
 * coda_cursor_goto_record_field_by_name(&field_cursor, "dsr_time");
 * \endcode
 *
 * First we copy the record cursor's contents into the field cursor through a simple assignment. The \c field_cursor
 * now also points to the full record. Then we move \c field_cursor to the dsr_time field (after this,
 * \c record_cursor still points to the whole record).
 */

/** \typedef coda_Cursor
 * CODA Cursor
 * \ingroup coda_cursor
 */

/** \enum coda_array_ordering_enum
 * Ordering of elements within arrays (C or Fortran variant)
 * \ingroup coda_cursor
 */

/** \typedef coda_array_ordering
 * Ordering of elements within arrays (C or Fortran variant)
 * \ingroup coda_cursor
 */

/** \addtogroup coda_cursor
 * @{
 */

/** Initialize the cursor to point to the entire product.
 * \param cursor Pointer to a CODA cursor.
 * \param pf Pointer to a product file handle.
 * \return
 *   \arg \c  0, Succes.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_set_product(coda_Cursor *cursor, coda_ProductFile *pf)
{
    if (cursor == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "cursor argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (pf == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "product file argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (pf->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_set_product(cursor, pf);
        case coda_format_xml:
            return coda_xml_cursor_set_product(cursor, pf);
        case coda_format_netcdf:
            return coda_netcdf_cursor_set_product(cursor, pf);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_set_product(cursor, pf);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_set_product(cursor, pf);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to the first field of a record.
 * If the field is a dynamically available record field and if it is not available in the current record, the cursor
 * will point to a special no-data data type after completion of this function.
 * \warning If the record contains no fields the function will return an error.
 * \param cursor Pointer to a CODA cursor that references a record.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_first_record_field(coda_Cursor *cursor)
{
    return coda_cursor_goto_record_field_by_index(cursor, 0);
}

/** Moves the cursor to point to the field at position \a index of a record.
 * If the field is a dynamically available record field and if it is not available in the current record, the cursor
 * will point to a special no-data data type after completion of this function.
 * \see coda_cursor_get_num_elements()
 * \param cursor Pointer to a CODA cursor that references a record.
 * \param index Index of the field (0 <= \a index < number of fields).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_record_field_by_index(coda_Cursor *cursor, long index)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a record (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (cursor->n == CODA_CURSOR_MAXDEPTH)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "maximum depth in cursor (%d) reached (%s:%u)", cursor->n, __FILE__,
                       __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_record_field_by_index(cursor, index);
        case coda_format_xml:
            return coda_xml_cursor_goto_record_field_by_index(cursor, index);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_record_field_by_index(cursor, index);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_record_field_by_index(cursor, index);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_record_field_by_index(cursor, index);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}


/** Moves the cursor to point to the field of a record that has fieldname \a name.
 * If the field is a dynamically available record field and if it is not available in the current record, the cursor
 * will point to a special no-data data type after completion of this function.
 * If \a name does not correspond with a fieldname of the record the function will return an error.
 * \param cursor Pointer to a CODA cursor that references a record.
 * \param name Fieldname of the field.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_record_field_by_name(coda_Cursor *cursor, const char *name)
{
    long index;

    if (coda_cursor_get_record_field_index_from_name(cursor, name, &index) != 0)
    {
        return -1;
    }

    return coda_cursor_goto_record_field_by_index(cursor, index);
}

/** Moves the cursor to point to the next field of a record.
 * If the field is a dynamically available record field and if it is not available in the current record, the cursor
 * will point to a special no-data data type after completion of this function.
 * \warning If the cursor already points to the last field of a record the function will return an error. So if you
 * want to enumerate all fields of a record use something like
 * \code
 * coda_cursor_get_num_elements(cursor, &num_fields);
 * if (num_fields > 0)
 * {
 *     coda_cursor_goto_first_record_field(cursor);
 *     for (i = 0; i < num_fields; i++)
 *     {
 *         ...
 *         if (i < num_fields - 1)
 *         {
 *              coda_cursor_goto_next_record_field(cursor);
 *         }
 *     }
 *     coda_cursor_goto_parent(cursor);
 * }
 * \endcode
 * \param cursor Pointer to a CODA cursor that references a field.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_next_record_field(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 1 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 2].type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE,
                       "parent of cursor does not refer to a record (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 2].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    /* check whether we are perhaps pointing to the attributes of the record */
    if (cursor->stack[cursor->n - 1].index == -1)
    {
        assert(cursor->stack[cursor->n - 1].type->type_class == coda_record_class);
        coda_set_error(CODA_ERROR_INVALID_TYPE,
                       "cursor does not refer to a record field (currently pointing to the record attributes) (%s:%u)",
                       __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 2].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_next_record_field(cursor);
        case coda_format_xml:
            return coda_xml_cursor_goto_next_record_field(cursor);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_next_record_field(cursor);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_next_record_field(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_next_record_field(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to the available union field.
 * CODA treats unions as a special kind of records (i.e. unions are records where all fields are dynamically available
 * and only one field will be available at a time). You can use the coda_type_get_record_union_status() to determine
 * whether a record is a union. If it is, you can use the coda_cursor_goto_available_union_field() function to go to
 * the single available record field.
 * \param cursor Pointer to a CODA cursor that references a union record.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_available_union_field(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a record (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_available_union_field(cursor);
        case coda_format_xml:
            return coda_xml_cursor_goto_available_union_field(cursor);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a union");
            return -1;
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to the first element of an array.
 * For an n-dimensional array this means going to the element with index (0, 0, ..., 0).
 * \warning If the array has 0 elements then this function will return an error.
 * \param cursor Pointer to a CODA cursor that references an array.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_first_array_element(coda_Cursor *cursor)
{
    return coda_cursor_goto_array_element_by_index(cursor, 0);
}

/** Moves the cursor to point to an array element via an array of subscripts.
 * This function takes a subscript array to specify the index of the data array element. The length of the array
 * \a subs, the number of dimensions of the data array and the value of \a num_subs should all be the same.
 * \note In contrast to coda_cursor_goto_array_element_by_index() this function will always perform a boundary check on
 * the \a num_subs and \a subs parameters, even if the option to check boundaries was turned off with
 * coda_set_option_perform_boundary_checks().
 * \param cursor Pointer to a CODA cursor that references an array.
 * \param num_subs Size of the parameter \a subs. This should be equal to the number of dimensions of the array which
 * the cursor is pointing to.
 * \param subs Array of subscripts that identifies the data array element
 * ((0, 0, ..., 0) <= \a subs < data array dimensions)
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_array_element(coda_Cursor *cursor, int num_subs, const long subs[])
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (cursor->n == CODA_CURSOR_MAXDEPTH)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "maximum depth in cursor (%d) reached (%s:%u)", cursor->n, __FILE__,
                       __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_array_element(cursor, num_subs, subs);
        case coda_format_xml:
            return coda_xml_cursor_goto_array_element(cursor, num_subs, subs);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_array_element(cursor, num_subs, subs);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_array_element(cursor, num_subs, subs);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_array_element(cursor, num_subs, subs);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to an array element via an index.
 * This function treats all multidimensional arrays as a one dimensional array (with the same number of elements).
 * The ordering in such a one dimensional array is by definition chosen to be equal to the way the array elements
 * are stored as a sequence in the product file.
 * The mapping of a one dimensional index for each multidimensional data array to an array of subscripts (and vice
 * versa) is defined in such a way that the last element of a subscript array is the one that is the fastest running
 * index (i.e. C array ordering). All multidimensional arrays have their dimensions defined using C array ordering in
 * CODA.<br>
 * For example if we have a two dimensional array with dimensions (2,4) then the index (0) would map
 * to the subscript array (0, 0). (1) would map to (0, 1), (4) would map to (1, 0) and (7) would map to (1, 3).
 * <br>
 * If the data array is one dimensional then this function will have the same result as calling
 * coda_cursor_goto_array_element() with \a num_subs = 1 and \a subs[0] = \a index.
 * \param cursor Pointer to a CODA cursor that references an array.
 * \param index Index of the array element (0 <= \a index < number of elements)
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_array_element_by_index(coda_Cursor *cursor, long index)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    if (cursor->n == CODA_CURSOR_MAXDEPTH)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "maximum depth in cursor (%d) reached (%s:%u)", cursor->n, __FILE__,
                       __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_array_element_by_index(cursor, index);
        case coda_format_xml:
            return coda_xml_cursor_goto_array_element_by_index(cursor, index);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_array_element_by_index(cursor, index);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_array_element_by_index(cursor, index);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_array_element_by_index(cursor, index);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to the next element of an array.
 * This function treats all multidimensional arrays as a one dimensional array in the same way as
 * coda_cursor_goto_array_element_by_index() does. It will move the cursor to the array element with \a index =
 * \a current_index + 1.
 * \warning If the cursor already points to the last element of an array the function will return an error. So if you
 * want to enumerate all elements of an array (as a one dimensional sequence) use something like
 * \code
 * n_elements = coda_cursor_count_elements(cursor);
 * if (n_elements > 0)
 * {
 *     coda_cursor_goto_first_array_element(cursor);
 *     for (i = 0; i < n_elements; i++)
 *     {
 *         ...
 *         if (i < n_elements - 1)
 *         {
 *             coda_cursor_goto_next_array_element(cursor);
 *         }
 *     }
 *     coda_cursor_goto_parent(cursor);
 * }
 * \endcode
 * \param cursor Pointer to a CODA cursor that references an array.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_next_array_element(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 1 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 2].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE,
                       "parent of cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 2].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    /* check whether we are perhaps pointing to the attributes of the array */
    if (cursor->stack[cursor->n - 1].index == -1)
    {
        assert(cursor->stack[cursor->n - 1].type->type_class == coda_record_class);
        coda_set_error(CODA_ERROR_INVALID_TYPE,
                       "cursor does not refer to an array element (currently pointing to the array attributes) (%s:%u)",
                       __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 2].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_next_array_element(cursor);
        case coda_format_xml:
            return coda_xml_cursor_goto_next_array_element(cursor);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_next_array_element(cursor);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_next_array_element(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_next_array_element(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor to point to a (virtual) record containing the attributes of the current data element.
 * This function will move the cursor to a record containing all attributes of the data element that the cursor was
 * previously pointing to.
 * If there are no attributes available the cursor will point to an empty record (i.e. a record with 0 fields).
 * It only makes sense to retrieve attributes when the HDF4, HDF5, netCDF or XML backend is used. Ascii and binary
 * files will always return an empty record. You can use the CODA Type functions to retrieve the fixed attributes
 * (such as unit and description) for files that are stored in a structured ascii or binary format.
 * \param cursor Pointer to a CODA cursor.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_attributes(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->n == CODA_CURSOR_MAXDEPTH)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "maximum depth in cursor (%d) reached (%s:%u)", cursor->n, __FILE__,
                       __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_goto_attributes(cursor);
        case coda_format_xml:
            return coda_xml_cursor_goto_attributes(cursor);
        case coda_format_netcdf:
            return coda_netcdf_cursor_goto_attributes(cursor);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_goto_attributes(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_goto_attributes(cursor);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Moves the cursor one level up in the hierarchy.
 * If the cursor points to a field this function will move the cursor to its record and if the cursor points to an
 * array element the cursor will be moved to the array.
 * If the cursor is already at the topmost level (it points to the root of a product) the function will return an error.
 * \param cursor Pointer to a CODA cursor that references either a field or array element.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_parent(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->n <= 1)
    {
        coda_set_error(CODA_ERROR_NO_PARENT, NULL);
        return -1;
    }

    cursor->n--;

    return 0;
}

/** Moves the cursor to the root of the product.
 * The cursor will be at the same position as it was after its initialization with coda_cursor_set_product().
 * \param cursor Pointer to a CODA cursor.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_goto_root(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    cursor->n = 1;

    return 0;
}

/** Reinterpret the current special data type using the base type of the special type.
 * All data types with a type class #coda_special_class have a base type that can be used to read the data in its raw
 * form (e.g. for ascii time data the type will change to a string type and for binary compound time data the type will
 * change to a record with fields containing binary numbers). With this function the cursor is modified such that it
 * will interpret the current data element using this base type.
 * The cursor should point to data of class #coda_special_class or an error will be returned.
 * \warning Using coda_cursor_goto_parent() on the cursor after calling coda_cursor_use_base_type_of_special_type()
 * will move the cursor to the parent of the special type and not back to the special type itself. In other words,
 * coda_cursor_use_base_type_of_special_type() does not 'move' the cursor, it only changes the data type that is used
 * to interpret the underlying data.
 * \param cursor Pointer to a CODA cursor.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_use_base_type_of_special_type(coda_Cursor *cursor)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_special_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE,
                       "cursor does not refer to a special type (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_use_base_type_of_special_type(cursor);
        case coda_format_binary:
            return coda_bin_cursor_use_base_type_of_special_type(cursor);
        case coda_format_xml:
            return coda_xml_cursor_use_base_type_of_special_type(cursor);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    assert(0);
    exit(1);
}

/** Determine wether data at the current cursor position is stored as ascii data.
 * You can use this function to determine whether the data is stored in ascii format. If it is in ascii format, you will
 * be able to read the data using coda_cursor_read_string().
 * If, for instance, a record consists of purely ascii data (i.e. it is a structured ascii block in the file)
 * \a has_ascii_content for a cursor pointing to that record will be 1 and you can use the coda_cursor_read_string()
 * function to read the whole record as a block of raw ascii.
 * \param cursor Pointer to a valid CODA cursor.
 * \param has_ascii_content Pointer to a variable where the ascii content status will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_has_ascii_content(const coda_Cursor *cursor, int *has_ascii_content)
{
    coda_Type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
    {
        return -1;
    }

    return coda_type_has_ascii_content(type, has_ascii_content);
}

/** Get the length in bytes of a string data type.
 * The length that is returned does not include the additional byte needed for the terminating 0. This means that if
 * you want to call coda_cursor_read_string() you should allocate \a length + 1 amount of bytes and pass a value of
 * \a length + 1 for the dst_size parameter of coda_cursor_read_string().
 * If the cursor does not point to string data the function will return an error.
 * \param cursor Pointer to a valid CODA cursor.
 * \param length Pointer to the variable where the string length will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_string_length(const coda_Cursor *cursor, long *length)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->type_class != coda_text_class)
    {
        coda_Type *type;
        int has_ascii_content;

        if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
        {
            return -1;
        }
        if (coda_type_has_ascii_content(type, &has_ascii_content) != 0)
        {
            return -1;
        }
        if (!has_ascii_content)
        {
            coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to text (current type is %s) (%s:%u)",
                           coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
            return -1;
        }
    }
    if (length == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_get_string_length(cursor, length, -1);
        case coda_format_binary:
            assert(0);
            exit(1);
        case coda_format_xml:
            return coda_xml_cursor_get_string_length(cursor, length);
        case coda_format_netcdf:
            return coda_netcdf_cursor_get_string_length(cursor, length);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_get_string_length(cursor, length);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_get_string_length(cursor, length);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Get the bit size for the data at the current cursor position.
 * Depending on the type of data and its format this function will return the following:
 * For data in ascii or binary format all data types will return the amount of bits the data occupies in the product
 * file. This means that e.g. ascii floats and ascii integers will return 8 times the byte size of the ascii
 * representation, records and arrays return the sum of the bit sizes of their fields/array-elements.
 * For XML data you will be able to retrieve bit sizes for all data except arrays and attribute records.
 * You will not be able to retrieve bit/byte sizes for data in netCDF, HDF4 or HDF5 format.
 * If a bit size is not available \a bit_size will be set to -1.
 * \param cursor Pointer to a valid CODA cursor.
 * \param bit_size Pointer to the variable where the bit size will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_bit_size(const coda_Cursor *cursor, int64_t *bit_size)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_get_bit_size(cursor, bit_size, -1);
        case coda_format_binary:
            return coda_bin_cursor_get_bit_size(cursor, bit_size);
        case coda_format_xml:
            return coda_xml_cursor_get_bit_size(cursor, bit_size);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *bit_size = -1;
            break;
    }

    return 0;
}

/** Get the byte size for the data at the current cursor position.
 * This function will retrieve the bit_size using coda_cursor_get_bit_size(), convert it to a byte size by rounding
 * it up to the nearest byte, and return this byte size.
 * If the bit size is -1, then this function will also return -1 for \a byte_size.
 * \param cursor Pointer to a valid CODA cursor.
 * \param byte_size Pointer to the variable where the byte size will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_byte_size(const coda_Cursor *cursor, int64_t *byte_size)
{
    int64_t bit_size;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_cursor_get_bit_size(cursor, &bit_size) != 0)
    {
        return -1;
    }
    if (bit_size == -1)
    {
        *byte_size = -1;
        return 0;
    }

    /* round up to the nearest byte */
    *byte_size = (bit_size >> 3) + ((bit_size & 0x7) != 0 ? 1 : 0);

    return 0;
}

/** Gives the number of elements of the data that is pointed to by the cursor.
 * If the cursor points to an array the function will return the total number of elements of the array. If the cursor
 * references a record then the number of fields of the record will be returned. For all other types the function will
 * return 1.
 * \param cursor Pointer to a valid CODA cursor.
 * \param num_elements Pointer to the variable where the number of elements will be stored.
 * \return
 *   \arg \c >=0, Number of elements of the data in the product.
 *   \arg \c  -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_num_elements(const coda_Cursor *cursor, long *num_elements)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_get_num_elements(cursor, num_elements);
        case coda_format_binary:
            return coda_bin_cursor_get_num_elements(cursor, num_elements);
        case coda_format_xml:
            return coda_xml_cursor_get_num_elements(cursor, num_elements);
        case coda_format_netcdf:
            return coda_netcdf_cursor_get_num_elements(cursor, num_elements);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_get_num_elements(cursor, num_elements);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_get_num_elements(cursor, num_elements);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve the Product File handle that was used to initialize this cursor.
 * \param cursor Pointer to a CODA cursor.
 * \param pf Pointer to a product file handle pointer.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_product_file(const coda_Cursor *cursor, coda_ProductFile **pf)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *pf = cursor->pf;

    return 0;
}

/** Retrieve the current hierarchical depth of the cursor.
 * The depth indicates how deep one has traversed into a product file and equals the amount of calls to
 * coda_cursor_goto_parent() one has to call to end up at the root of the product.
 * \param cursor Pointer to a CODA cursor.
 * \param depth Pointer to the variable where the cursor depth will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_depth(const coda_Cursor *cursor, int *depth)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (depth == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "depth argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *depth = cursor->n - 1;

    return 0;
}

/** Retrieve the array element or field index of the data element that the cursor points to.
 * If the parent of the cursor points to a record then this function will return the field index of the current data
 * element. In case the parent points to an array then the array element index (the same kind of index that is used for
 * coda_cursor_goto_array_element_by_index()) will be returned.
 * If the cursor has no parent or if the cursor points to an attribute record then this function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param index Pointer to the variable where the index will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_index(const coda_Cursor *cursor, long *index)
{
    if (cursor == NULL || cursor->n <= 1 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (index == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "index argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *index = cursor->stack[cursor->n - 1].index;

    return 0;
}

/** Retrieve the file offset in bits of the data element that the cursor points to.
 * You will not be able to retrieve bit/byte offsets for data in netCDF, HDF4, or HDF5 format.
 * For data in XML format you will not be able to retrieve bit/byte offsets for arrays or attribute records.
 * \param cursor Pointer to a CODA cursor.
 * \param bit_offset Pointer to the variable where the file offset in bits will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
int coda_cursor_get_file_bit_offset(const coda_Cursor *cursor, int64_t *bit_offset)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (bit_offset == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_offset argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
        case coda_format_xml:
            *bit_offset = cursor->stack[cursor->n - 1].bit_offset;
            break;
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *bit_offset = -1;
            break;
    }

    return 0;
}

/** Retrieve the file offset in bytes of the data element that the cursor points to.
 * The byte offset is determined by the bit offset of the data element. If the current bit offset does not end at a
 * byte boundary the returned byte offest will be determined by rounding the bit offset down to the nearest byte.
 * \param cursor Pointer to a CODA cursor.
 * \param byte_offset Pointer to the variable where the (possibly rounded) file offset in bytes will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
int coda_cursor_get_file_byte_offset(const coda_Cursor *cursor, int64_t *byte_offset)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (byte_offset == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "byte_offset argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
        case coda_format_xml:
            if (cursor->stack[cursor->n - 1].bit_offset == -1)
            {
                *byte_offset = -1;
            }
            else
            {
                *byte_offset = (cursor->stack[cursor->n - 1].bit_offset >> 3);
            }
            break;
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            *byte_offset = -1;
            return -1;
    }

    return 0;
}

/** Retrieve the storage format of the data element that the cursor points to.
 * This has the same result as calling coda_type_get_format() with the result from coda_cursor_get_type().
 * \param cursor Pointer to a CODA cursor.
 * \param format Pointer to the variable where the format will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_format(const coda_Cursor *cursor, coda_format *format)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (format == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "format argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *format = cursor->stack[cursor->n - 1].type->format;

    return 0;
}

/** Retrieve the type class of the data element that the cursor points to.
 * This has the same result as calling coda_type_get_class() with the result from coda_cursor_get_type().
 * \param cursor Pointer to a CODA cursor.
 * \param type_class Pointer to the variable where the type class will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_type_class(const coda_Cursor *cursor, coda_type_class *type_class)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type_class == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type_class argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    *type_class = cursor->stack[cursor->n - 1].type->type_class;

    return 0;
}

/** Get the best native type for reading data at the current cursor position.
 * This has the same result as calling coda_type_get_read_type() with the result from coda_cursor_get_type().
 * \see coda_type_get_read_type()
 * \param cursor Pointer to a CODA cursor.
 * \param read_type Pointer to the variable where the read type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_read_type(const coda_Cursor *cursor, coda_native_type *read_type)
{
    coda_Type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
    {
        return -1;
    }

    return coda_type_get_read_type(type, read_type);
}

/** Retrieve the special type of the data element that the cursor points to.
 * This has the same result as calling coda_type_get_special_type() with the result from coda_cursor_get_type().
 * The class of the data type that the cursor points to should be #coda_special_class, otherwise this function
 * will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param special_type Pointer to the variable where the special type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_special_type(const coda_Cursor *cursor, coda_special_type *special_type)
{
    coda_Type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (special_type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "special_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
    {
        return -1;
    }

    return coda_type_get_special_type(type, special_type);
}

/** Retrieve the CODA type of the data element that the cursor points to.
 * Refer to the section about \link coda_types CODA Types \endlink to find more information on how to use the CODA type
 * to get more information about a data element.
 * \param cursor Pointer to a CODA cursor.
 * \param type Pointer to the variable where the type will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_type(const coda_Cursor *cursor, coda_Type **type)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    return coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, type);
}

/** Get the field index from a field name for the record at the current cursor position.
 * If the cursor does not point to a record the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param name Name of the record field.
 * \param index Pointer to a variable where the field index will be stored (0 <= \a index < number of fields).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_record_field_index_from_name(const coda_Cursor *cursor, const char *name, long *index)
{
    coda_Type *type;

    if (cursor == NULL || cursor->n <= 0 || cursor->pf == NULL || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
    {
        return -1;
    }

    return coda_type_get_record_field_index_from_name(type, name, index);
}

/** Determines whether a record field is available in the product.
 * This function allows you to check whether a dynamically available field in a record is available or not. Such fields
 * will only occur for ascii, binary, or xml formatted data (for other formats \a available will always be 1).
 * If the field is available then \a available will be 1, otherwise it will be 0.
 * \note If a record is a union then only one field in the record will be available.
 * \note It is allowed to move a CODA cursor to an unavailable field. In that case the data type for the field will be
 * set to the special #coda_special_no_data data type (with type class #coda_special_class), which has a bit/byte
 * size of 0.
 * \param cursor Pointer to a CODA cursor.
 * \param index Index of the field (0 <= \a index < number of fields).
 * \param available Pointer to the variable where the available status will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_record_field_available_status(const coda_Cursor *cursor, long index, int *available)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->pf == NULL || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (available == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "available argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a record (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_get_record_field_available_status(cursor, index, available);
        case coda_format_xml:
            return coda_xml_cursor_get_record_field_available_status(cursor, index, available);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            /* fields are always available in HDF */
            *available = 1;
            break;
    }

    return 0;
}

/** Determines which union record field is available in the product.
 * \note It is allowed to move a cursor to an unavailable union record field. In that case the data type for the
 * field will be set to the special #coda_special_no_data data type (with type class #coda_special_class), which has a
 * bit/byte length of 0.
 * \param cursor Pointer to a CODA cursor.
 * \param index Pointer to the variable where the index of the available record field will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_available_union_field_index(const coda_Cursor *cursor, long *index)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->pf == NULL || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (index == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "index argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->type_class != coda_record_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to a record (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_get_available_union_field_index(cursor, index);
        case coda_format_xml:
            return coda_xml_cursor_get_available_union_field_index(cursor, index);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    assert(0);
    exit(1);
}

/** Retrieve the dimensions of the data array that the cursor points to.
 * The function returns both the number of dimensions \a num_dims and the size of the dimensions \a dim.
 * \note If the size of the dimensions is variable (it differs per product or differs per data element inside one
 * product) then this function will calculate the dimensions from the necessary properties inside the product.
 * Depending on the complexity of this calculation the determination of variable sized dimensions could impact
 * performance.
 * \param cursor Pointer to a CODA cursor.
 * \param num_dims Pointer to the variable where the number of dimensions will be stored.
 * \param dim Pointer to the variable where the dimensions will be stored. The caller needs to make sure that the
 * variable has enough room to store the dimensions array. It is guaranteed that the number of dimensions will never
 * exceed #CODA_MAX_NUM_DIMS.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_get_array_dim(const coda_Cursor *cursor, int *num_dims, long dim[])
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class));
        return -1;
    }
    if (num_dims == NULL || dim == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dimension argument(s) are NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
        case coda_format_binary:
            return coda_ascbin_cursor_get_array_dim(cursor, num_dims, dim);
        case coda_format_xml:
            return coda_xml_cursor_get_array_dim(cursor, num_dims, dim);
        case coda_format_netcdf:
            return coda_netcdf_cursor_get_array_dim(cursor, num_dims, dim);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_get_array_dim(cursor, num_dims, dim);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_get_array_dim(cursor, num_dims, dim);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c int8 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int8(const coda_Cursor *cursor, int8_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int8(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int8(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_int8(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int8(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c uint8 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint8(const coda_Cursor *cursor, uint8_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint8(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint8(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_uint8(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint8(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint8(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c int16 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int16(const coda_Cursor *cursor, int16_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int16(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int16(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_int16(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int16(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c uint16 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint16(const coda_Cursor *cursor, uint16_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint16(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint16(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_uint16(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint16(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint16(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c int32 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int32(const coda_Cursor *cursor, int32_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int32(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int32(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_int32(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int32(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c uint32 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint32(const coda_Cursor *cursor, uint32_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint32(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint32(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_uint32(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint32(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint32(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c int64 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int64(const coda_Cursor *cursor, int64_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int64(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int64(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_int64(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int64(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c uint64 from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 * - \c uint64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint64(const coda_Cursor *cursor, uint64_t *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint64(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint64(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_uint64(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint64(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint64(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c float from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_float(const coda_Cursor *cursor, float *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_float(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_float(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_float(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_float(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_float(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_float(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c double from the product file. The value is stored in \a dst.
 * The cursor must point to data with one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_double(const coda_Cursor *cursor, double *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_double(cursor, dst, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_double(cursor, dst);
        case coda_format_xml:
            return coda_xml_cursor_read_double(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_double(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_double(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_double(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve data as type \c char from the product file. The value is stored in \a dst.
 * The cursor must point to data with read type \c char to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_char(const coda_Cursor *cursor, char *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_char(cursor, dst, -1);
        case coda_format_binary:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
            return -1;
        case coda_format_xml:
            return coda_xml_cursor_read_char(cursor, dst);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_char(cursor, dst);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_char(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_char(cursor, dst);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve text data as a 0 terminated string. The value is stored in \a dst.
 * You will be able to read data as a string if the cursor points to data with type class #coda_text_class or if
 * it points to ASCII data (see coda_cursor_has_ascii_content()). For other cases the function will return an error.
 * The function will fill at most \a dst_size bytes in the dst memory space. The last character that is put in \a dst
 * will always be a zero termination character, which means that at most \a dst_size - 1 characters of text data will
 * be read.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param dst_size The maximum number of bytes to write in \a dst.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_string(const coda_Cursor *cursor, char *dst, long dst_size)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_size <= 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_size (%ld) argument is <= 0 (%s:%u)", dst_size, __FILE__,
                       __LINE__);
        return -1;
    }
    if (cursor->stack[cursor->n - 1].type->type_class != coda_text_class)
    {
        coda_Type *type;
        int has_ascii_content;

        if (coda_get_type_for_dynamic_type(cursor->stack[cursor->n - 1].type, &type) != 0)
        {
            return -1;
        }
        if (coda_type_has_ascii_content(type, &has_ascii_content) != 0)
        {
            return -1;
        }
        if (!has_ascii_content)
        {
            coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to text (current type is %s) (%s:%u)",
                           coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
            return -1;
        }
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_string(cursor, dst, dst_size, -1);
        case coda_format_binary:
            assert(0);
            exit(1);
        case coda_format_xml:
            return coda_xml_cursor_read_string(cursor, dst, dst_size);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_string(cursor, dst, dst_size);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_string(cursor, dst, dst_size);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_string(cursor, dst, dst_size);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Read a specified amount of bits. The data is stored in \a dst.
 * This function will work independent of the type of data at the current cursor position, but it will not work on
 * ASCII, XML, HDF4, and HDF5 data.
 * The function will read a \a bit_length amount of bits starting from the sum of the cursor offset position and the
 * amount of bits given by the \a bit_offset parameter. If \a bit_length is not a rounded amount of bytes the data will
 * be put in the first \a bit_length/8 + 1 bytes and will be right adjusted (i.e. padding bits with value 0 will be put
 * in the most significant bits of the first byte of \a dst).
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param bit_offset The offset relative to the current cursor position from where the bits should be read.
 * \param bit_length The number of bits to read.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_bits(const coda_Cursor *cursor, uint8_t *dst, int64_t bit_offset, int64_t bit_length)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_length < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "bit_length argument is negative (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (bit_length == 0)
    {
        return 0;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_format_binary:
            return coda_bin_cursor_read_bits(cursor, dst, bit_offset, bit_length);
        case coda_format_xml:
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bits data type");
    return -1;
}

/** Read a specified amount of bytes. The data is stored in \a dst.
 * This function will work independent of the type of data at the current cursor position, but it will not work on HDF4
 * and HDF5 files. For XML files it will only work if the cursor points to a single element (i.e. you will get an error
 * when the cursor points to an XML element array or an attribute record).
 * The function will read a \a length amount of bytes starting from the sum of the cursor offset position and the
 * amount of bytes given by the \a offset parameter.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \param offset The offset relative to the current cursor position from where the bytes should be read.
 * \param length The number of bytes to read.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_bytes(const coda_Cursor *cursor, uint8_t *dst, int64_t offset, int64_t length)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (length < 0)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "length argument is negative (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (length == 0)
    {
        return 0;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_bytes(cursor, dst, offset, length);
        case coda_format_binary:
            return coda_bin_cursor_read_bytes(cursor, dst, offset, length);
        case coda_format_xml:
            return coda_xml_cursor_read_bytes(cursor, dst, offset, length);
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a raw bytes data type");
    return -1;
}

/** Retrieve a data array as type \c int8 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int8_array(const coda_Cursor *cursor, int8_t *dst, coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int8_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int8_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_int8_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int8_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int8_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int8_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c uint8 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint8_array(const coda_Cursor *cursor, uint8_t *dst,
                                             coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint8_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint8_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_uint8_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint8_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint8_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint8_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c int16 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int16_array(const coda_Cursor *cursor, int16_t *dst,
                                             coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int16_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int16_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_int16_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int16_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int16_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int16_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c uint16 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint16_array(const coda_Cursor *cursor, uint16_t *dst,
                                              coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint16_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint16_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_uint16_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint16_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint16_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint16_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c int32 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int32_array(const coda_Cursor *cursor, int32_t *dst,
                                             coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int32_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int32_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_int32_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int32_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int32_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int32_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c uint32 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint32_array(const coda_Cursor *cursor, uint32_t *dst,
                                              coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint32_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint32_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_uint32_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint32_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint32_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint32_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c int64 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_int64_array(const coda_Cursor *cursor, int64_t *dst,
                                             coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_int64_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_int64_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_int64_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_int64_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_int64_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_int64_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c uint64 from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c uint8
 * - \c uint16
 * - \c uint32
 * - \c uint64
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_uint64_array(const coda_Cursor *cursor, uint64_t *dst,
                                              coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_uint64_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_uint64_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_uint64_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_uint64_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_uint64_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_uint64_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c float from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_float_array(const coda_Cursor *cursor, float *dst, coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_float_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_float_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_float_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_float_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_float_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_float_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c double from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has one of the following read types to succeed:
 * - \c int8
 * - \c uint8
 * - \c int16
 * - \c uint16
 * - \c int32
 * - \c uint32
 * - \c int64
 * - \c uint64
 * - \c float
 * - \c double
 *
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_double_array(const coda_Cursor *cursor, double *dst,
                                              coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_double_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            return coda_bin_cursor_read_double_array(cursor, dst, array_ordering);
        case coda_format_xml:
            return coda_xml_cursor_read_double_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_double_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_double_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_double_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve a data array as type \c char from the product file. The values are stored in \a dst.
 * The cursor must point to an array with a basic type that has read type \c char to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_char_array(const coda_Cursor *cursor, char *dst, coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            return coda_ascii_cursor_read_char_array(cursor, dst, array_ordering, -1);
        case coda_format_binary:
            coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a char data type");
            return -1;
        case coda_format_xml:
            return coda_xml_cursor_read_char_array(cursor, dst, array_ordering);
        case coda_format_netcdf:
            return coda_netcdf_cursor_read_char_array(cursor, dst, array_ordering);
        case coda_format_cdf:
        case coda_format_hdf4:
#ifdef HAVE_HDF4
            return coda_hdf4_cursor_read_char_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF4_SUPPORT, NULL);
            return -1;
#endif
        case coda_format_hdf5:
#ifdef HAVE_HDF5
            return coda_hdf5_cursor_read_char_array(cursor, dst, array_ordering);
#else
            coda_set_error(CODA_ERROR_NO_HDF5_SUPPORT, NULL);
            return -1;
#endif
    }

    assert(0);
    exit(1);
}

/** Retrieve complex data as type \c double from the product file.
 * The real and imaginary values are stored consecutively in \a dst.
 * The cursor must point to data with special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_pair(const coda_Cursor *cursor, double *dst)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            break;
        case coda_format_binary:
            return coda_bin_cursor_read_double_pair(cursor, dst);
        case coda_format_xml:
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
    return -1;
}

/** Retrieve an array of complex data as type \c double from the product file.
 * All complex array elements are stored consecutively in \a dst (for each element the real and imaginary values are 
 * stored next to each other).
 * The cursor must point to data with a base type that has special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst Pointer to the variable where the values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_pairs_array(const coda_Cursor *cursor, double *dst,
                                                            coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            break;
        case coda_format_binary:
            return coda_bin_cursor_read_double_pairs_array(cursor, dst, array_ordering);
        case coda_format_xml:
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
    return -1;
}

/** Retrieve complex data as type \c double from the product file.
 * The real and imaginary values are stored in \a dst_re and \a dst_im.
 * The cursor must point to data with special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst_re Pointer to the variable where the real value that was read from the product will be stored.
 * \param dst_im Pointer to the variable where the imaginary value that was read from the product will be stored.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_split(const coda_Cursor *cursor, double *dst_re, double *dst_im)
{
    double dst[2];

    if (coda_cursor_read_complex_double_pair(cursor, dst) != 0)
    {
        return -1;
    }
    *dst_re = dst[0];
    *dst_im = dst[1];

    return 0;
}

/** Retrieve an array of complex data as type \c double from the product file.
 * All real array elements are stored in \a dst_re and all imaginary array elements in \a dsr_im.
 * The cursor must point to data with a base type that has special type #coda_special_complex to succeed.
 * For all other data types the function will return an error.
 * \param cursor Pointer to a CODA cursor.
 * \param dst_re Pointer to the variable where the real values read from the product will be stored.
 * \param dst_im Pointer to the variable where the imaginary values read from the product will be stored.
 * \param array_ordering Specifies array storage ordering for \a dst_im and \a dst_re: must be #coda_array_ordering_c or
 * #coda_array_ordering_fortran.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #coda_errno).
 */
LIBCODA_API int coda_cursor_read_complex_double_split_array(const coda_Cursor *cursor, double *dst_re, double *dst_im,
                                                            coda_array_ordering array_ordering)
{
    if (cursor == NULL || cursor->n <= 0 || cursor->stack[cursor->n - 1].type == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid cursor argument (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_re == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_re argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (dst_im == NULL)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "dst_im argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (cursor->stack[cursor->n - 1].type->type_class != coda_array_class)
    {
        coda_set_error(CODA_ERROR_INVALID_TYPE, "cursor does not refer to an array (current type is %s) (%s:%u)",
                       coda_type_get_class_name(cursor->stack[cursor->n - 1].type->type_class), __FILE__, __LINE__);
        return -1;
    }

    switch (cursor->stack[cursor->n - 1].type->format)
    {
        case coda_format_ascii:
            break;
        case coda_format_binary:
            return coda_bin_cursor_read_double_split_array(cursor, dst_re, dst_im, array_ordering);
        case coda_format_xml:
        case coda_format_netcdf:
        case coda_format_cdf:
        case coda_format_hdf4:
        case coda_format_hdf5:
            break;
    }

    coda_set_error(CODA_ERROR_INVALID_TYPE, "can not read this data using a complex double data type");
    return -1;
}

/** @} */
