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
   CODA Product class.
   
 * The CODA Product class contains methods to open, close and retrieve
 * information about product files that are supported by CODA.
 * 
 */
public class Product
{
    private SWIGTYPE_p_coda_ProductFile_struct product;


    // Constructor used instead of explicit coda_open().
    /**
     * Construct an instance of a Product class representing a product file
     * opened for reading.
     * 
     * In Java, this constructor is the equivalent of the coda_open() function
     * in the C interface.
     * 
     * @param filename
     *            Relative or full path to the product file.
     * @throws CodaException
     *             If an error occurred.
     */
    public Product(String filename) throws CodaException
    {
        this.product = new SWIGTYPE_p_coda_ProductFile_struct();
        codac.open(filename, this.product);
    }


    // Users never need to deal with the opaque SWIG pointer
    // themselves, so this Constructur only has internal
    // package access.
    Product(SWIGTYPE_p_coda_ProductFile_struct product)
    {
        this.product = product;
    }


    // Internal package access only.
    SWIGTYPE_p_coda_ProductFile_struct getSwigRepresentation()
    {
        return this.product;
    }


    // Explicit destructor -- we cannot rely on finalize() in
    // Java, but since Products are caller-allocated, we must
    // provide this method to allow the user to explicitly
    // clean up as well.
    /**
     * Closes the open product file.
     * 
     * After calling this method the Product instance is no longer valid as the
     * underlying file handle will have been released.
     * 
     * @throws CodaException
     *             If an error occurred.
     */
    public void close() throws CodaException
    {
        if (this.product != null)
            codac.close(this.product);
        this.product = null;
    }


    /**
     * Get the filename of a product file.
     * 
     * @return The filename of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getFilename() throws CodaException
    {
        String filename[] = {""};
        codac.get_product_filename(this.product, filename);
        return filename[0];
    }


    /**
     * Get the actual file size of a product file.
     * 
     * @return The actual file size (in bytes) of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getFileSize() throws CodaException
    {
        long size[] = new long[1];
        codac.get_product_file_size(this.product, size);
        return size[0];
    }


    /**
     * Get the basic file format of the product.
     * 
     * @return The format.
     * @throws CodaException
     *             If an error occurred.
     */
    public FormatEnum getFormat() throws CodaException
    {
        int format[] = new int[1];
        codac.get_product_format(this.product, format);
        return FormatEnum.swigToEnum(format[0]);
    }


    // Method can not be named 'getClass()' because that
    // conflicts with an already-existing java.lang.Object
    // method.
    /**
     * Get the product class of a product file.
     * 
     * @return The class name of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getProductClass() throws CodaException
    {
        String cls[] = {""};
        codac.get_product_class(this.product, cls);
        return cls[0];
    }


    /**
     * Get the product type of a product file.
     * 
     * @return The product type name of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public String getType() throws CodaException
    {
        String type[] = {""};
        codac.get_product_type(this.product, type);
        return type[0];
    }


    /**
     * Get the product type version of a product file.
     * 
     * @return The product version of the product type of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public int getVersion() throws CodaException
    {
        int version[] = new int[1];
        codac.get_product_version(this.product, version);
        return version[0];
    }


    /**
     * Get the CODA type of the root of the product.
     * 
     * @return The Type of the product.
     * @throws CodaException
     *             If an error occurred.
     */
    public Type getRootType() throws CodaException
    {
        SWIGTYPE_p_coda_Type_struct rootType = new SWIGTYPE_p_coda_Type_struct();
        codac.get_product_root_type(this.product, rootType);
        return new Type(rootType);
    }


    /**
     * Get the value for a product variable.
     * 
     * @param variable
     *            The name of the product variable.
     * @param index
     *            The array index of the product variable (pass 0 if the
     *            variable is a scalar).
     * @return The product variable value.
     * @throws CodaException
     *             If an error occurred.
     */
    public long getVariableValue(String variable, int index) throws CodaException
    {
        long value[] = new long[1];
        codac.get_product_variable_value(this.product, variable, index, value);
        return value[0];
    }

    // CODA Product File methods not exposed in Java yet:
    //
    // coda_recognize_file
}
