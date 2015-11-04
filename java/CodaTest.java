// Test file for the CODA Java library interface


package nl.stcorp.coda;

import java.lang.reflect.Field;


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
		Class   myClass = ClassLoader.class;
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


    private static void showCodaInfo() throws CodaException
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
                            System.out.println();
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
                    String utcString = Coda.time_to_string(datetime);
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
                    
                    default:
                        System.out.print(String.format("*** Unexpected read type (%s) ***", Type.getNativeTypeName(readType)));
                        break;
                }
            }        
        }
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
