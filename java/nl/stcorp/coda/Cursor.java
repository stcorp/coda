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
 * CODA Cursor class.
 *
 * After you have opened a product file (by constructing a Product instance) you
 * will want to access data from this product and retrieve metadata for the data
 * elements. In order to do this, CODA provides the concept of a 'cursor'. A
 * cursor can be thought of as something that keeps track of a position in the
 * product file and it also stores some extra (type) information about the data
 * element it is currently pointing to. Cursors will start their useful life at
 * the 'root' of a product, i.e., pointing to the entire product, with a type
 * that accurately describes the entire product. From there you can navigate the
 * cursor to the specific data element(s) you want *to access. Note that cursors
 * are used for all products that can be opened with CODA. This includes files
 * in ascii, binary, XML, netCDF, HDF4, or HDF5 format.
 * 
 * The Cursor class is the Java representation of the cursor concept and
 * contains the various methods available for manipulating cursors in CODA.
 * 
 */
public class Cursor
{
    private SWIGTYPE_p_coda_Cursor_struct cursor;


    // Constructor used instead of explicit coda_set_product().
    /**
     * Construct a Cursor instance to point to an entire Product. /**
     * 
     * @param product
     *            A Product instance.
     * @throws CodaException
     *             If an error occurred.
     */
    public Cursor(Product product) throws CodaException
    {
        this.cursor = codac.new_coda_Cursor();
        codac.cursor_set_product(this.cursor, product.getSwigRepresentation());
    }


    // Explicit destructor -- we cannot rely on finalize() in
    // Java, but since Cursors are caller-allocated, we must
    // provide this method to allow the user to explicitly
    // clean up as well.
    /**
     * Closes an open cursor. The Cursor instance will be invalid after calling
     * this method and should no longer be used.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void close() throws CodaException
    {
        if (this.cursor != null)
            codac.delete_coda_Cursor(this.cursor);
        this.cursor = null;
    }


    // Internal package access only.
    /**
     * @return
     */
    SWIGTYPE_p_coda_Cursor_struct getSwigRepresentation()
    {
        return this.cursor;
    }


    // Copy constructor to allow users to clone a Cursor
    // instance.
    /**
     * @param cursor
     */
    public Cursor(Cursor cursor)
    {
        this.cursor = codac.deepcopy_coda_Cursor(cursor.getSwigRepresentation());
    }


    /**
     * Moves the cursor to point to the first field of a record.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoFirstRecordField() throws CodaException
    {
        codac.cursor_goto_first_record_field(this.cursor);
    }


    /**
     * Moves the cursor to point to the field at position \a index of a record.
     * 
     * @param index
     *            Index of the field (0 <= \a index < number of fields).
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoRecordFieldByIndex(int index) throws CodaException
    {
        codac.cursor_goto_record_field_by_index(this.cursor, index);
    }


    /**
     * Moves the cursor to point to the field of a record that has fieldname \a
     * name.
     * 
     * @param name
     *            Fieldname of the field.
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoRecordFieldByName(String name) throws CodaException
    {
        codac.cursor_goto_record_field_by_name(this.cursor, name);
    }


    /**
     * Moves the cursor to point to the next field of a record.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoNextRecordField() throws CodaException
    {
        codac.cursor_goto_next_record_field(this.cursor);
    }


    /**
     * Moves the cursor to point to the available union field.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoAvailableUnionField() throws CodaException
    {
        codac.cursor_goto_available_union_field(this.cursor);
    }


    /**
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoFirstArrayElement() throws CodaException
    {
        codac.cursor_goto_first_array_element(this.cursor);
    }


    /**
     * Moves the cursor to point to an array element via an array of subscripts.
     * 
     * @param subs
     *            Array of subscripts that identifies the data array element
     *            ((0, 0, ..., 0) <= \a subs < data array dimensions)
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoArrayElement(int[] subs) throws CodaException
    {
        codac.cursor_goto_array_element(this.cursor, subs.length, subs);
    }


    /**
     * Moves the cursor to point to an array element via an index.
     * 
     * @param index
     *            Index of the array element (0 <= \a index < number of
     *            elements)
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoArrayElementByIndex(int index) throws CodaException
    {
        codac.cursor_goto_array_element_by_index(this.cursor, index);
    }


    /**
     * Moves the cursor to point to the next element of an array.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoNextArrayElement() throws CodaException
    {
        codac.cursor_goto_next_array_element(this.cursor);
    }


    /**
     * Moves the cursor to point to a (virtual) record containing the attributes
     * of the current data element.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoAttributes() throws CodaException
    {
        codac.cursor_goto_attributes(this.cursor);
    }


    /**
     * Moves the cursor one level up in the hierarchy.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoParent() throws CodaException
    {
        codac.cursor_goto_parent(this.cursor);
    }


    /**
     * Moves the cursor to the root of the product.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void gotoRoot() throws CodaException
    {
        codac.cursor_goto_root(this.cursor);
    }


    /**
     * Reinterpret the current special data type using the base type of the
     * special type.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void useBaseTypeOfSpecialType() throws CodaException
    {
        codac.cursor_use_base_type_of_special_type(this.cursor);
    }


    /**
     * Determine wether data at the current cursor position is stored as ascii
     * data.
     * 
     * @return The ascii content status.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean hasAsciiContent() throws CodaException
    {
        int has_ascii_content[] = new int[1];
        codac.cursor_has_ascii_content(this.cursor, has_ascii_content);
        return has_ascii_content[0] == 1;
    }


    /**
     * Get the length in bytes of a string data type.
     * 
     * @return The string length.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getStringLength() throws CodaException
    {
        int length[] = new int[1];
        codac.cursor_get_string_length(this.cursor, length);
        return length[0];
    }


    /**
     * Get the bit size for the data at the current cursor position.
     * 
     * @return The bit size.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getBitSize() throws CodaException
    {
        long bit_size[] = new long[1];
        codac.cursor_get_bit_size(this.cursor, bit_size);
        return bit_size[0];
    }


    /**
     * Get the byte size for the data at the current cursor position.
     * 
     * @return The byte size.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getByteSize() throws CodaException
    {
        long byte_size[] = new long[1];
        codac.cursor_get_byte_size(this.cursor, byte_size);
        return byte_size[0];
    }


    /**
     * Gives the number of elements of the data that is pointed to by the
     * cursor.
     * 
     * @return Number of elements of the data in the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getNumElements() throws CodaException
    {
        int numElements[] = new int[1];
        codac.cursor_get_num_elements(this.cursor, numElements);
        return numElements[0];
    }


    // Use getProduct() instead of the more algorithmically
    // correct getProductFile(). I suppose we could have
    // named the class ProductFile as well, but that would
    // then have been inconsistent with all the methods that
    // are called product_*, not product_file_*. In the end,
    // I feel that conceptually we're talking about a
    // product, not a file, hence the decision to use
    // Product.
    /**
     * Retrieve the Product that was used to initialize this cursor.
     * 
     * @return The originating Product instance.
     * @throws CodaException
     *             If an error occurred.
     */
    public Product getProduct() throws CodaException
    {
        SWIGTYPE_p_coda_ProductFile_struct product = new SWIGTYPE_p_coda_ProductFile_struct();
        codac.cursor_get_product_file(this.cursor, product);
        return new Product(product);
    }


    /**
     * Retrieve the current hierarchical depth of the cursor.
     * 
     * @return The cursor depth.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getDepth() throws CodaException
    {
        int depth[] = new int[1];
        codac.cursor_get_depth(this.cursor, depth);
        return depth[0];
    }


    /**
     * Retrieve the array element or field index of the data element that the
     * cursor points to.
     * 
     * @return The index.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getIndex() throws CodaException
    {
        int index[] = new int[1];
        codac.cursor_get_index(this.cursor, index);
        return index[0];
    }


    /**
     * Retrieve the file offset in bits of the data element that the cursor
     * points to.
     * 
     * @return The file offset in bits.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getFileBitOffset() throws CodaException
    {
        long bit_offset[] = new long[1];
        codac.cursor_get_file_bit_offset(this.cursor, bit_offset);
        return bit_offset[0];
    }


    /**
     * Retrieve the file offset in bytes of the data element that the cursor
     * points to.
     * 
     * @return The (possibly rounded) file offset in bytes.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getFileByteOffset() throws CodaException
    {
        long byte_offset[] = new long[1];
        codac.cursor_get_file_byte_offset(this.cursor, byte_offset);
        return byte_offset[0];
    }


    /**
     * Retrieve the storage format of the data element that the cursor points
     * to.
     * 
     * @return The format.
     * @throws CodaException
     *             If an error occurred.
     */
    public FormatEnum getFormat() throws CodaException
    {
        int format[] = new int[1];
        codac.cursor_get_format(this.cursor, format);
        return FormatEnum.swigToEnum(format[0]);
    }


    /**
     * Retrieve the type class of the data element that the cursor points to.
     * 
     * @return The type class.
     * @throws CodaException
     *             If an error occurred.
     */
    public TypeClassEnum getTypeClass() throws CodaException
    {
        int type_class[] = new int[1];
        codac.cursor_get_type_class(this.cursor, type_class);
        return TypeClassEnum.swigToEnum(type_class[0]);
    }


    /**
     * Get the best native type for reading data at the current cursor position.
     * 
     * @return The read type.
     * @throws CodaException
     *             If an error occurred.
     */
    public NativeTypeEnum getReadType() throws CodaException
    {
        int read_type[] = new int[1];
        codac.cursor_get_read_type(this.cursor, read_type);
        return NativeTypeEnum.swigToEnum(read_type[0]);
    }


    /**
     * Retrieve the special type of the data element that the cursor points to.
     * 
     * @return The special type.
     * @throws CodaException
     *             If an error occurred.
     */
    public SpecialTypeEnum getSpecialType() throws CodaException
    {
        int special_type[] = new int[1];
        codac.cursor_get_special_type(this.cursor, special_type);
        return SpecialTypeEnum.swigToEnum(special_type[0]);
    }


    /**
     * Retrieve the CODA type of the data element that the cursor points to.
     * 
     * @return A Type instance.
     * @throws CodaException
     *             If an error occurred.
     */
    public Type getType() throws CodaException
    {
        SWIGTYPE_p_coda_Type_struct type = new SWIGTYPE_p_coda_Type_struct();
        codac.cursor_get_type(this.cursor, type);
        return new Type(type);
    }


    /**
     * Get the field index from a field name for the record at the current
     * cursor position.
     * 
     * @param name
     *            Name of the record field.
     * @return The field index (0 <= \a index < number of fields).
     * @throws CodaException
     *             If an error occurred.
     */
    public int getRecordFieldIndexFromName(String name) throws CodaException
    {
        int index[] = new int[1];
        codac.cursor_get_record_field_index_from_name(this.cursor, name, index);
        return index[0];
    }


    /**
     * Determines whether a record field is available in the product.
     * 
     * @param index
     *            Index of the field (0 <= \a index < number of fields).
     * @return The available status.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getRecordFieldAvailableStatus(int index) throws CodaException
    {
        int available[] = new int[1];
        codac.cursor_get_record_field_available_status(this.cursor,
                index,
                available);
        return available[0];
    }


    /**
     * Determines which union record field is available in the product.
     * 
     * @return The index of the available record field.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getAvailableUnionFieldIndex() throws CodaException
    {
        int index[] = new int[1];
        codac.cursor_get_available_union_field_index(this.cursor, index);
        return index[0];
    }


    /**
     * Retrieve the dimensions of the data array that the cursor points to.
     * 
     * @return The dimensions array.
     * @throws CodaException
     *             If an error occurred.
     */
    public int[] getArrayDim() throws CodaException
    {
        int dim[] = new int[codacConstants.CODA_MAX_NUM_DIMS];
        int num_dims[] = new int[1];
        codac.cursor_get_array_dim(this.cursor, num_dims, dim);
        int result[] = new int[num_dims[0]];
        for (int i = 0; i < num_dims[0]; i++)
            result[i] = dim[i];
        return result;
    }


    /**
     * Retrieve data as type \c int8 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte readInt8() throws CodaException
    {
        byte dst[] = new byte[1];
        codac.cursor_read_int8(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c uint8 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte readUint8() throws CodaException
    {
        byte dst[] = new byte[1];
        codac.cursor_read_uint8(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c int16 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public short readInt16() throws CodaException
    {
        short dst[] = new short[1];
        codac.cursor_read_int16(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c uint16 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public short readUint16() throws CodaException
    {
        short dst[] = new short[1];
        codac.cursor_read_uint16(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c int32 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public int readInt32() throws CodaException
    {
        int dst[] = new int[1];
        codac.cursor_read_int32(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c uint32 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long readUint32() throws CodaException
    {
        int dst[] = new int[1];
        codac.cursor_read_uint32(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c int64 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long readInt64() throws CodaException
    {
        long dst[] = new long[1];
        codac.cursor_read_int64(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c uint64 from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long readUint64() throws CodaException
    {
        long dst[] = new long[1];
        codac.cursor_read_uint64(this.cursor, dst);
        return dst[0];
    }


    // public BigInteger readBigInteger() throws CodaException
    // {
    // BigInteger dst[] = new BigInteger[1];
    // NativeTypeEnum nativeType = this.getReadType();

    // switch(nativeType)
    // {
    // case coda_native_type_uint64:
    // // codac.helper_cursor_use_read_uint64_to_fill_BigInteger(this.cursor,
    // dst, array_ordering);
    // break;

    // default:
    // // codac.helper_cursor_use_read_int64_to_fill_BigInteger_(this.cursor,
    // dst, array_ordering);
    // }

    // return dst[0];
    // }

    /**
     * Retrieve data as type \c float from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public float readFloat() throws CodaException
    {
        float dst[] = new float[1];
        codac.cursor_read_float(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c double from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public double readDouble() throws CodaException
    {
        double dst[] = new double[1];
        codac.cursor_read_double(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve data as type \c char from the product file.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public char readChar() throws CodaException
    {
        byte dst[] = new byte[1];
        codac.cursor_read_char(this.cursor, dst);
        return (char)(dst[0]);
    }


    /**
     * Retrieve text data as a Java string.
     * 
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public String readString() throws CodaException
    {
        return codac.helper_coda_cursor_read_string(this.cursor);
    }


    /**
     * Read a specified amount of bits.
     * 
     * @param bit_offset
     *            The offset relative to the current cursor position from where
     *            the bits should be read.
     * @param bit_length
     *            The number of bits to read.
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte[] readBits(long bit_offset, long bit_length) throws CodaException
    {
        // TODO: Check if cast to int is okay.
        byte dst[] = new byte[(int) bit_length / 8 + 1];
        codac.cursor_read_bits(this.cursor, dst, bit_offset, bit_length);
        return dst;
    }


    /**
     * Read a specified amount of bytes.
     * 
     * @param offset
     *            The offset relative to the current cursor position from where
     *            the bytes should be read.
     * @param length
     *            The number of bytes to read.
     * @return The value that was read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte[] readBytes(long offset, long length) throws CodaException
    {
        // TODO: Check if cast to int is okay.
        byte dst[] = new byte[(int) length];
        codac.cursor_read_bytes(this.cursor, dst, offset, length);
        return dst;
    }


    /**
     * Retrieve a data array as type \c int8 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte[] readInt8Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        byte dst[] = new byte[this.getNumElements()];
        codac.cursor_read_int8_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c uint8 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public byte[] readUint8Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        byte dst[] = new byte[this.getNumElements()];
        codac.cursor_read_uint8_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c int16 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public short[] readInt16Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        short dst[] = new short[this.getNumElements()];
        codac.cursor_read_int16_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c uint16 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public short[] readUint16Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        short dst[] = new short[this.getNumElements()];
        codac.cursor_read_uint16_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c int32 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public int[] readInt32Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        int dst[] = new int[this.getNumElements()];
        codac.cursor_read_int32_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c uint32 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public int[] readUint32Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        int dst[] = new int[this.getNumElements()];
        codac.cursor_read_uint32_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c int64 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long[] readInt64Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        long dst[] = new long[this.getNumElements()];
        codac.cursor_read_int64_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c uint64 from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long[] readUint64Array(ArrayOrderingEnum array_ordering) throws CodaException
    {
        long dst[] = new long[this.getNumElements()];
        codac.cursor_read_uint64_array(this.cursor, dst, array_ordering);
        return dst;
    }


    // public BigInteger[] readBigIntegerArray(ArrayOrderingEnum array_ordering)
    // throws CodaException
    // {
    // NativeTypeEnum nativeType = this.getReadType();
    // BigInteger dst[] = new BigInteger[this.getNumElements()];

    // switch(nativeType)
    // {
    // case coda_native_type_uint64:
    // //
    // codac.helper_cursor_use_read_uint64_array_to_fill_BigInteger_Array(this.cursor,
    // dst, array_ordering);
    // break;

    // default:
    // //
    // codac.helper_cursor_use_read_int64_array_to_fill_BigInteger_Array(this.cursor,
    // dst, array_ordering);
    // }
    // return dst;
    // }

    /**
     * Retrieve a data array as type \c float from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public float[] readFloatArray(ArrayOrderingEnum array_ordering) throws CodaException
    {
        float dst[] = new float[this.getNumElements()];
        codac.cursor_read_float_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c double from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public double[] readDoubleArray(ArrayOrderingEnum array_ordering) throws CodaException
    {
        double dst[] = new double[this.getNumElements()];
        codac.cursor_read_double_array(this.cursor, dst, array_ordering);
        return dst;
    }


    /**
     * Retrieve a data array as type \c char from the product file.
     * 
     * @param array_ordering
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public char[] readCharArray(ArrayOrderingEnum array_ordering) throws CodaException
    {
        byte dst[] = new byte[this.getNumElements()];
        codac.cursor_read_char_array(this.cursor, dst, array_ordering);
        char chardst[] = new char[dst.length];
        for (int i = 0; i < dst.length; i++)
            chardst[i] = (char) dst[i];
        
        return chardst;
    }


    /**
     * Retrieve complex data as type \c double from the product file.
     * 
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public double readComplexDoublePair() throws CodaException
    {
        if (this.getSpecialType() != SpecialTypeEnum.coda_special_complex)
            throw new CodaException(
                    String.format("Cursor type should be 'coda_special_complex' (was: %d)",
                            this.getSpecialType()));

        double dst[] = new double[1];
        codac.cursor_read_complex_double_pair(this.cursor, dst);
        return dst[0];
    }


    /**
     * Retrieve an array of complex data as type \c double from the product
     * file.
     * 
     * @param array_ordering
     *            Specifies array storage ordering for the return array. Must be
     *            ArrayOrderingEnum.coda_array_ordering_c or
     *            ArrayOrderingEnum.coda_array_ordering_fortran.
     * @return The values read from the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public double[] readComplexDoublePairsArray(ArrayOrderingEnum array_ordering) throws CodaException
    {
        if (this.getSpecialType() != SpecialTypeEnum.coda_special_complex)
            throw new CodaException(
                    String.format("Cursor type should be 'coda_special_complex' (was: %d)",
                            this.getSpecialType()));

        double dst[] = new double[this.getNumElements()];
        codac.cursor_read_complex_double_pairs_array(this.cursor,
                dst,
                array_ordering);
        return dst;
    }

    // CODA Product File methods not exposed in Java yet:
    //
    // cursor_read_complex_double_split;
    // cursor_read_complex_double_split_array;

}
