//
// Copyright (C) 2007-2009 S&T, The Netherlands.
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
 * CODA Type class.
 * 
 * Each data element or group of data elements (such as an array
 * or record) in a product file, independent of whether it is an
 * ascii, binary, XML, netCDF, HDF4, or HDF5 product, has a
 * unique description in CODA.
 * 
 * This class represents instances of such type descriptions and provides
 * methods to retrieve information about the type.
 * 
 */

public class Type
{
    private final SWIGTYPE_p_coda_Type_struct type;


    // Users never need to deal with the opaque SWIG pointer
    // themselves, so this Constructor only has internal
    // package access.
    /**
     * @param type
     */
    Type(SWIGTYPE_p_coda_Type_struct type)
    {
        this.type = type;
    }


    /**
     * Returns the name of a storage format.
     * 
     * @param format
     *            CODA storage format
     * @return if the format is known a string containing the name of the
     *         format, otherwise the string "unknown".
     */
    public static String getFormatName(FormatEnum format)
    {
        return codac.type_get_format_name(format);
    }


    /**
     * Returns the name of a type class. In case the type class is not
     * recognised the string "unknown" is returned.
     * 
     * @param typeClass
     *            CODA type class
     * @return if the type class is known a string containing the name of the
     *         class, otherwise the string "unknown".
     */
    public static String getClassName(TypeClassEnum typeClass)
    {
        return codac.type_get_class_name(typeClass);
    }


    /**
     * Returns the name of a native type.
     * 
     * @param nativeType
     *            CODA native type
     * @return if the native type is known a string containing the name of the
     *         native type, otherwise the string "unknown".
     */
    public static String getNativeTypeName(NativeTypeEnum nativeType)
    {
        return codac.type_get_native_type_name(nativeType);
    }


    /**
     * Returns the name of a special type. In case the special type is not
     * recognised the string "unknown" is returned.
     * 
     * @param specialType
     *            CODA special type
     * @return if the special type is known a string containing the name of the
     *         special type, otherwise the string "unknown".
     */
    public static String getSpecialTypeName(SpecialTypeEnum specialType)
    {
        return codac.type_get_special_type_name(specialType);
    }


    /**
     * Determine whether data of this type is stored as ascii data.
     * 
     * @return The ascii content status.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean hasAsciiContent() throws CodaException
    {
        int has_ascii_content[] = new int[1];
        codac.type_has_ascii_content(this.type, has_ascii_content);
        return has_ascii_content[0] == 1;
    }


    /**
     * Get the storage format of a type.
     * 
     * @return The format.
     * @throws CodaException
     *             If an error occurred.
     */
    public FormatEnum getFormat() throws CodaException
    {
        int format[] = new int[1];
        codac.type_get_format(this.type, format);
        return FormatEnum.swigToEnum(format[0]);
    }


    // Method can not be named 'getClass()' because that
    // conflicts with an already-existing java.lang.Object
    // method.
    /**
     * Get the class of a type.
     * 
     * @return The type class.
     * @throws CodaException
     *             If an error occurred.
     */
    public TypeClassEnum getTypeClass() throws CodaException
    {
        int type_class[] = new int[1];
        codac.type_get_class(this.type, type_class);
        return TypeClassEnum.swigToEnum(type_class[0]);
    }


    /**
     * Get the best native type for reading data of a CODA type.
     * 
     * @return The native type for reading.
     * @throws CodaException
     *             If an error occurred.
     */
    public NativeTypeEnum getReadType() throws CodaException
    {
        int read_type[] = new int[1];
        codac.type_get_read_type(this.type, read_type);
        return NativeTypeEnum.swigToEnum(read_type[0]);
    }


    /**
     * Get the length in bytes of a string data type.
     * 
     * @return The string length (not including terminating 0).
     * @throws CodaException
     *             If an error occurred.
     */
    public int getStringLength() throws CodaException
    {
        int length[] = new int[1];
        codac.type_get_string_length(this.type, length);
        return length[0];
    }


    /**
     * Get the bit size for the data type.
     * 
     * @return The bit size.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getBitSize() throws CodaException
    {
        long bit_size[] = new long[1];
        codac.type_get_bit_size(this.type, bit_size);
        return bit_size[0];
    }


    /**
     * Get the name of a type.
     * 
     * @return The name of the type.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getName() throws CodaException
    {
        String name[] = {""};
        codac.type_get_name(this.type, name);
        return name[0];
    }


    /**
     * Get the description of a type.
     * 
     * @return The description of the type.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getDescription() throws CodaException
    {
        String description[] = {""};
        codac.type_get_description(this.type, description);
        return description[0];
    }


    /**
     * Get the unit of a type.
     * 
     * @return The unit information.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getUnit() throws CodaException
    {
        String unit[] = {""};
        codac.type_get_unit(this.type, unit);
        return unit[0];
    }


    /**
     * Get the associated fixed value string of a type if it has one.
     * 
     * @return The string length of the fixed value (can be null).
     * @throws CodaException
     *             If an error occurred.
     */
    public String getFixedValue() throws CodaException
    {
        String fixed_value[] = {""};
        int length[] = new int[1];
        codac.type_get_fixed_value(this.type, fixed_value, length);
        return fixed_value[0];
    }


    /**
     * Get the number of fields of a record type.
     * 
     * @return The number of fields.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getNumRecordFields() throws CodaException
    {
        int num_fields[] = new int[1];
        codac.type_get_num_record_fields(this.type, num_fields);
        return num_fields[0];
    }


    /**
     * Get the field index from a field name for a record type.
     * 
     * @param name
     *            Name of the record field.
     * @return Pointer to a variable where the field index will be stored (0 <=
     *         \a index < number of fields).
     * @throws CodaException
     *             If an error occurred.
     */
    public int getRecordFieldIndexFromName(String name) throws CodaException
    {
        int index[] = new int[1];
        codac.type_get_record_field_index_from_name(this.type, name, index);
        return index[0];
    }


    /**
     * Get the CODA type for a record field.
     * 
     * @param index
     *            Field index (0 <= \a index < number of fields).
     * @return The type of the record field.
     * @throws CodaException
     *             If an error occurred.
     */
    public Type getRecordFieldType(int index) throws CodaException
    {
        SWIGTYPE_p_coda_Type_struct field_type = new SWIGTYPE_p_coda_Type_struct();
        codac.type_get_record_field_type(this.type, index, field_type);
        return new Type(field_type);
    }


    /**
     * Get the name of a record field.
     * 
     * @param index
     *            Field index (0 <= \a index < number of fields).
     * @return The name of the record field.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getRecordFieldName(int index) throws CodaException
    {
        String name[] = {""};
        codac.type_get_record_field_name(this.type, index, name);
        return name[0];
    }


    /**
     * Get the hidden status of a record field.
     * 
     * @param index
     *            Field index (0 <= \a index < number of fields).
     * @return The hidden status of the record field.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean getRecordFieldHiddenStatus(int index) throws CodaException
    {
        int hidden[] = new int[1];
        codac.type_get_record_field_hidden_status(this.type, index, hidden);
        return hidden[0] == 1;
    }


    /**
     * Get the available status of a record field.
     * 
     * @param index
     *            Field index (0 <= \a index < number of fields).
     * @return The available status of the record field.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getRecordFieldAvailableStatus(int index) throws CodaException
    {
        int available[] = new int[1];
        codac.type_get_record_field_available_status(this.type,
                index,
                available);
        return available[0];
    }


    /**
     * Get the union status of a record.
     * 
     * @return 1 if the record is a union (i.e. all fields are dynamically
     *         available and only one field can be available at any time), 0
     *         otherwise.
     * @throws CodaException
     *             If an error occurred.
     */
    public boolean getRecordUnionStatus() throws CodaException
    {
        int is_union[] = new int[1];
        codac.type_get_record_union_status(this.type, is_union);
        return is_union[0] == 1;
    }


    /**
     * Get the number of dimensions for an array.
     * 
     * @return The number of dimensions.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getArrayNumDims() throws CodaException
    {
        int num_dims[] = new int[1];
        codac.type_get_array_num_dims(this.type, num_dims);
        return num_dims[0];
    }


    /**
     * Retrieve the dimensions with a constant value for an array.
     * 
     * @return The dimensions array.
     * @throws CodaException
     *             If an error occurred.
     */
    public int[] getArrayDim() throws CodaException
    {
        int dim[] = new int[codacConstants.CODA_MAX_NUM_DIMS];
        int num_dims[] = new int[1];
        codac.type_get_array_dim(this.type, num_dims, dim);
        int result[] = new int[num_dims[0]];
        for (int i = 0; i < num_dims[0]; i++)
            result[i] = dim[i];
        return result;
    }


    /**
     * Get the CODA type for the elements of an array.
     * 
     * @return The base type.
     * @throws CodaException
     *             If an error occurred.
     */
    public Type getArrayBaseType() throws CodaException
    {
        SWIGTYPE_p_coda_Type_struct base_type = new SWIGTYPE_p_coda_Type_struct();
        codac.type_get_array_base_type(this.type, base_type);
        return new Type(base_type);
    }


    /**
     * Get the special type for a type.
     * 
     * @return The special type.
     * @throws CodaException
     *             If an error occurred.
     */
    public SpecialTypeEnum getSpecialType() throws CodaException
    {
        int special_type[] = new int[1];
        codac.type_get_special_type(this.type, special_type);
        return SpecialTypeEnum.swigToEnum(special_type[0]);
    }


    /**
     * Get the base type for a special type.
     * 
     * @return The base type.
     * @throws CodaException
     *             If an error occurred.
     */
    public Type getSpecialBaseType() throws CodaException
    {
        SWIGTYPE_p_coda_Type_struct base_type = new SWIGTYPE_p_coda_Type_struct();
        codac.type_get_special_base_type(this.type, base_type);
        return new Type(base_type);
    }
}
