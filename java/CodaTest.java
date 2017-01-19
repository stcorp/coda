// CodaTest.java - Sample file for the CODA Java library interface
//
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


import java.lang.reflect.Field;

import nl.stcorp.coda.Coda;
import nl.stcorp.coda.CodaException;
import nl.stcorp.coda.Cursor;
import nl.stcorp.coda.Expression;
import nl.stcorp.coda.FormatEnum;
import nl.stcorp.coda.NativeTypeEnum;
import nl.stcorp.coda.Product;
import nl.stcorp.coda.SpecialTypeEnum;
import nl.stcorp.coda.Type;
import nl.stcorp.coda.TypeClassEnum;


/**
 * CodaTest class.
 *
 * This is not a unit-testing or regression-testing class, but
 * rather an example class / starting point that illustrates some
 * of the techniques of working with CODA through the Java
 * interface.
 *
 * You can compile and run this test program on one or more
 * product files by saying
 *
 *    ant run -Dargs="<product file ...>"
 *
 * This utility will print some global information about CODA, a
 * very simple CODA Expression example, and it will print the
 * structure of the product files passed on the commandline.
 *
 * If you encounter an "unsupported product" error in
 * coda_open(), it means that the CODA_DEFINITION directory
 * (typically $INSTALL/share/coda/definitions) does not have the
 * codadef files for your particular product in it.
 *
 */
public class CodaTest
{

    static
    {
        loadLibrary("coda_jni");
    }


    private static void loadLibrary(String libraryName)
    {
        loadLibrary(System.getProperty("coda.lib.dir", "."), libraryName);
    }


    private static void loadLibrary(String libraryPath, String libraryName)
    {
        Class<ClassLoader>   myClass = ClassLoader.class;
        Field   myField;
        Object  original;
        boolean bAccessible;

        try {
            /* Get the system paths field */
            myField = myClass.getDeclaredField("sys_paths");

            /* Make the field accessible and record that we did that */
            bAccessible = myField.isAccessible();
            if (!bAccessible)
            {
                myField.setAccessible(true);
            }

            /* Backup the contents of the field */
            original = myField.get(myClass);

            /*
             * Reset it to null so that whenever "System.loadLibrary" is called,
             * it will be reconstructed with the changed value.
             */
            myField.set(myClass, null);
            try
            {
                /* Change the value and load the library. */
                System.setProperty("java.library.path", libraryPath);
                System.loadLibrary(libraryName);
            }
            finally
            {
                /* Revert back the changes. */
                myField.set(myClass, original);
                myField.setAccessible(bAccessible);
            }
        }
        catch (NoSuchFieldException e)
        {
            e.printStackTrace();
        }
        catch (IllegalAccessException e)
        {
            e.printStackTrace();
        }
    }


    private static void showCodaInfo()
    {
        System.out.println("Coda Version: " + Coda.version());
        System.out.println("Sample FormatEnum: " + Type.getFormatName(FormatEnum.coda_format_xml));
        System.out.println("Sample NativeTypeEnum: " + Type.getNativeTypeName(NativeTypeEnum.coda_native_type_uint64));
        System.out.println("Sample TypeClassEnum: " + Type.getClassName(TypeClassEnum.coda_raw_class));
        System.out.println("Sample SpecialTypeEnum: " + Type.getSpecialTypeName(SpecialTypeEnum.coda_special_vsf_integer));
        System.out.println();
    }
    

    private static void showProductInfo(String filename) throws CodaException
    {
        Product product = new Product(filename);
        System.out.println("Product:");
        System.out.println("  Filename = " + product.getFilename());
        System.out.println("  File size = " + product.getFileSize());
        System.out.println("  Class = " + product.getProductClass());
        System.out.println("  Format = " + product.getFormat());
        System.out.println("  Type = " + product.getType());
        System.out.println("  Version = " + product.getVersion());

        Type type = product.getRootType();
        System.out.println("  Root Type:");
        System.out.println("    Description = " + type.getDescription());

        Cursor cursor = new Cursor(product);
        System.out.println("      Number of elements = " + cursor.getNumElements());

        TypeClassEnum rootType = cursor.getTypeClass();
        System.out.println("      Root Type = " + rootType);

            // Uncomment the next line if you also want to print record data.
            // printRecord(cursor); 
        product.close();
    }


    private static void printRecord(Cursor cursor) throws CodaException
    {
        int numFields = cursor.getNumElements();

        if (numFields > 0)
        {
            Type recordType = cursor.getType();
            cursor.gotoFirstRecordField();
            for (int i = 0; i < numFields; i++)
            {
                // We don't print fields that are hidden, like the first MPH
                // field
                // (with value 'PRODUCT=')
                boolean hidden = recordType.getRecordFieldHiddenStatus(i);
                if (!hidden)
                {
                    String fieldName = recordType.getRecordFieldName(i);
                    System.out.print(String.format("%32s : ", fieldName));
                    printData(cursor);
                    System.out.println();
                }

                if (i < numFields - 1)
                    cursor.gotoNextRecordField();
            }
            cursor.gotoParent();
        }
    }

    private static void printData(Cursor cursor) throws CodaException
    {
        TypeClassEnum typeClass = cursor.getTypeClass();
        switch (typeClass) 
        {
            case coda_array_class:
            {
                int numElements = cursor.getNumElements();
                if (numElements > 0)
                {
                    System.out.print("[");
                    cursor.gotoFirstArrayElement();
                    
                    for (int i = 0; i < numElements; i++)
                    {
                        printData(cursor);
                        if (i < numElements - 1)
                        {
                            System.out.print(", ");
                            cursor.gotoNextArrayElement();
                        }                      
                    }
                    System.out.print("]");
                    cursor.gotoParent();
                }
            }
            break;
            
            case coda_special_class:
            {
                SpecialTypeEnum specialType = cursor.getSpecialType();
                if (specialType == SpecialTypeEnum.coda_special_time)
                {
                    double datetime = cursor.readDouble();
                    String utcString = Coda.time_double_to_string(datetime, "yyyy-MM-dd HH:mm:ss.SSSSSS");
                    System.out.print(utcString);
                }
                else
                    System.out.print(String.format("*** Unexpected special type (%s) ***", Type.getSpecialTypeName(specialType)));
                    
            }
            break;
            
            default:
            {
                NativeTypeEnum readType = cursor.getReadType();
                
                switch (readType)
                {
                    case coda_native_type_not_available:
                    {
                        System.out.print("Compound entity that cannot be read directly");
                        break;
                    }
                    
                    case coda_native_type_int8:
                    case coda_native_type_int16:
                    case coda_native_type_int32:
                    {  
                        int data = cursor.readInt32();
                        System.out.print(data);
                        break;
                    }

                    case coda_native_type_uint8:
                    case coda_native_type_uint16:
                    case coda_native_type_uint32:
                    {
                        long data = cursor.readUint32();
                        System.out.print(data);
                        break;
                    }
                    
                    case coda_native_type_int64:
                    {
                        long data = cursor.readInt64();
                        System.out.print(data);
                        break;
                    }
                    
                    case coda_native_type_uint64:
                    {
                        long data = cursor.readUint64();
                        System.out.print(data);
                        break;
                    }
                    
                    case coda_native_type_float:
                    case coda_native_type_double:
                    {
                        double data = cursor.readDouble();
                        System.out.print(String.format("%g", data));
                        break;
                    }
                    
                    case coda_native_type_char:
                    {
                        int data = cursor.readChar();
                        System.out.print(String.format("%c", data));
                        break;
                    }
                    
                    case coda_native_type_string:
                    {
                        String data = cursor.readString();
                        System.out.print(String.format("\"%s\"", data));
                        break;
                    }

                    case coda_native_type_bytes:
                    {
                        int len = cursor.getNumElements();
                        // byte[] data = cursor.readBytes(0, len);
                        // String dataString = new String(data);
                        System.out.print(String.format("%d bytes", len));
                        break;
                    }
                    
                    default:
                        System.out.print(String.format("*** Unknown native type encountered: %s ***", Type.getNativeTypeName(readType)));
                        break;
                }
            }        
        }
    }

    
    private static void showExpressions() throws CodaException
    {
        showIntegerExpression("2 * 21");
//        showIntegerExpression("fnorgle"); // This *should* fail!
//        showIntegerExpression("2 / 0"); // This *should* fail!
    }

    
    private static void showIntegerExpression(String exprString) throws CodaException
    {
        Expression expr = new Expression(exprString);
        System.out.println("Expression:");
        System.out.println("  init string = " + exprString);
        System.out.println("  type        = " + expr.getType());
        System.out.println("  isConstant? = " + expr.isConstant());
        System.out.println("  value       = " + expr.evalInteger());
        expr.delete();
    }
        

    public static void main(String argv[])
    {
        if (argv.length < 1)
        {
            System.err.println("Usage: CodaTest <product file ...>");
            System.exit(1);
        }

        String[] productFiles = argv;

        try
        {
            Coda.init();
            showCodaInfo();
            showExpressions();
            
            for (String productFile : productFiles)
            {
                showProductInfo(productFile);
                System.out.println();
            }
        }
        catch (CodaException eeks)
        {
            System.err.println("Error: " + eeks.getMessage());
        }
        
        Coda.done();
    }
}
