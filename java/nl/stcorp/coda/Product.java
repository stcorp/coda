// Copyright (C) 2007-2017 S[&]T, The Netherlands.
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
   CODA Product class.
   
 * The CODA Product class contains methods to open, close and retrieve
 * information about product files that are supported by CODA.
 * 
 */
public class Product
{
    private SWIGTYPE_p_coda_product_struct product;


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
        this.product = new SWIGTYPE_p_coda_product_struct();
        codac.open(filename, this.product);
    }

    /**
     * Construct an instance of a Product class representing a product file
     * opened for reading, forcing the use of a specific version of a format definition.
     * 
     * In Java, this constructor is the equivalent of the coda_open_as() function
     * in the C interface.
     * 
     * @param filename
     *            Relative or full path to the product file.
     * @param product_class
     *            Name of the product class for the requested format definition.
     * @param product_type
     *            Name of the product type for the requested format definition.
     * @param version
     *            Format version number of the product type definition.
     *            Use -1 to request the latest available definition.
     * @throws CodaException
     *             If an error occurred.
     */
    public Product(String filename, String product_class, String product_type, int version) throws CodaException
    {
        this.product = new SWIGTYPE_p_coda_product_struct();
        codac.open_as(filename, product_class, product_type, version, this.product);
    }


    /**
     * Construct an instance of a Product class representing a product file
     * opened for reading, forcing the use of the latest version of a specific format definition.
     * 
     * In Java, this constructor is the equivalent of the coda_open_as() function
     * in the C interface.
     * 
     * @param filename
     *            Relative or full path to the product file.
     * @param product_class
     *            Name of the product class for the requested format definition.
     * @param product_type
     *            Name of the product type for the requested format definition.
     * @throws CodaException
     *             If an error occurred.
     */
    public Product(String filename, String product_class, String product_type) throws CodaException
    {
        this.product = new SWIGTYPE_p_coda_product_struct();
        codac.open_as(filename, product_class, product_type, -1, this.product);
    }


    // Users never need to deal with the opaque SWIG pointer
    // themselves, so this Constructur only has internal
    // package access.
    Product(SWIGTYPE_p_coda_product_struct product)
    {
        this.product = product;
    }


    // Internal package access only.
    SWIGTYPE_p_coda_product_struct getSwigRepresentation()
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
        SWIGTYPE_p_coda_type_struct rootType = new SWIGTYPE_p_coda_type_struct();
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
