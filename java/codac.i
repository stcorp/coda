/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

%module codac

%include "typemaps.i"
%include "various.i"
%include "arrays_java.i"
%include "enums.swg"
%javaconst(1);


typedef signed char     int8_t;
typedef short int       int16_t;
typedef int             int32_t;

typedef char            uint8_t;
typedef short int       uint16_t;
typedef int             uint32_t;

typedef long long int   int64_t;
typedef long long int	uint64_t;

%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "coda.h"
%}

/*
----------------------------------------------------------------------------------------
- RENAME AND IGNORE                                                                    -
----------------------------------------------------------------------------------------
*/

/*
  Include coda_rename.i. this is a list of %rename directives to
  strip the 'coda_' part from the Java declarations. (This is
  done because in Java we will already be in the 'coda' package
  namespace, so the prefix is redundant.
*/
%include "coda_rename.i"


 /*
   Rename enums to more Java-like equivalents.
 */
%rename(ArrayOrderingEnum) coda_array_ordering_enum;
%rename(FilefilterStatusEnum) coda_filefilter_status_enum;
%rename(FormatEnum) coda_format_enum;
%rename(NativeTypeEnum) coda_native_type_enum;
%rename(SpecialTypeEnum) coda_special_type_enum;
%rename(TypeClassEnum) coda_type_class_enum;
%rename(ExpressionTypeEnum) coda_expression_type_enum;


/*
  Include coda_ignore.i. This is a list of %ignore directives to
  stop SWIG from exposing internal constants and #defines at the
  Java level. Currently, all #define's starting with 'CODA_' and
  'HAVE_' are ignored.
*/
%include "coda_ignore.i"


/*
  Declarations not included in coda_ignore.i that nevertheless
  should be ignored. these declarations are related to error
  reporting, but this is handled through or custom exceptions for
  Java.
*/
%ignore coda_errno;
%ignore coda_set_error;
%ignore coda_errno_to_string;


/*
  Functions that are not currently exposed to the Java layer,
  because there was not enough time to get them working and/or
  they had lower priority. TODO: fix this.
*/ 

/*
  Multiple output values, will probably require some sort of
  record class to store properly. Java does not have native (or
  even library) support for complex numbers.
 */
%ignore coda_recognize_file;
%ignore cursor_read_complex_double_split;
%ignore cursor_read_complex_double_split_array;

/*
  Not yet implemented due to difficulty with callback function.
 */
%ignore coda_match_filefilter;


/*
----------------------------------------------------------------------------------------
- CUSTOM WRAPPERS AND HELPER FUNCTIONS THAT DO NOT NEED THE GLOBAL EXCEPTION MECHANISM -
----------------------------------------------------------------------------------------
*/


/*
  Helper function to expose the global 'libcoda_version' variable
  as a static method 'version()' at the Java level.
*/
%inline
%{
    static const char *helper_version()
    {
        return libcoda_version;
    }
%}
%ignore libcoda_version;


/*
  Helper functions for creating, deleting, and copying
  coda_cursor structs. These structs are treated as an opaque
  pointer type (i.e. the underlying C implementation is
  unreachable from Java, and you can only pass references to
  binary blobs around).

  The handwritten nl.stcorp.coda.Cursor class will wrap such a
  coda_cursor opaque object, provide constructors, and expose all
  CODA functions that act on a coda_cursor struct as methods of
  the Cursor object.

  Product and Type structs are also opaque objects wrapped by
  handwritten Java classes (with appropriate CODA functions
  attached as methods), but the allocation and freeing of these
  structs is always handled by CODA, so we do not need to create
  and expose explicit memory management methods for those.
  
*/
%{
    coda_cursor *new_coda_cursor()
    {
        return (coda_cursor *)malloc(sizeof(coda_cursor));
    }

    void delete_coda_cursor(coda_cursor *self)
    {
        free(self);
    }

    coda_cursor *deepcopy_coda_cursor(coda_cursor *self)
    {
        coda_cursor *new_cursor;

        new_cursor = (coda_cursor *)malloc(sizeof(coda_cursor));
        if( new_cursor != NULL )
        {
            memcpy(new_cursor, self, sizeof(coda_cursor));
        }
        return new_cursor;
    }

%};

coda_cursor *new_coda_cursor();
void delete_coda_cursor(coda_cursor *self);
coda_cursor *deepcopy_coda_cursor(coda_cursor *self);


/*
  Helper functions created to replace those CODA functions that
  require pre-allocated strings (i.e. memory management is the
  responsiblilty of the caller).

  These functions will be wrapped and exposed upwards instead of
  the underlying CODA equivalents. The latter must therefore also
  be ignored afterwards.
*/

%newobject helper_coda_cursor_read_string; /* make sure free() will be called */
%inline
%{
    char *helper_coda_cursor_read_string(const coda_cursor *cursor)
    {
        long dst_size;
        char *dst;
        
        coda_cursor_get_string_length(cursor, &dst_size);
        dst = (char *)malloc(dst_size+1);
        coda_cursor_read_string(cursor, dst, dst_size+1);
        return dst;
    }
%}
%ignore coda_cursor_read_string; /* ignore the real method being wrapped */


%newobject helper_coda_time_to_string;
%inline
%{
    char *helper_coda_time_to_string(double datetime)
    {
        char *dst = (char *)malloc(27);
        coda_time_to_string(datetime, dst);
        return dst;
    }
%}
%ignore coda_time_to_string; /* ignore the real method being wrapped */

%newobject helper_coda_time_to_utcstring;
%inline
%{
    char *helper_coda_time_to_utcstring(double datetime)
    {
        char *dst = (char *)malloc(27);
        coda_time_to_utcstring(datetime, dst);
        return dst;
    }
%}
%ignore coda_time_to_utcstring; /* ignore the real method being wrapped */
    

/*
----------------------------------------------------------------------------------------
- GLOBAL EXCEPTION MECHANISM                                                           -
----------------------------------------------------------------------------------------
*/

 /*
   First, those declarations to which the exception clause should
   not be attached are declared here and subsequently %ignore'd.
   (i.e. these declarations are ignored when parsing the coda.h
   file; see below). the declarations can be devided into
   two classes:
        - functions that do not return an int (0/-1) flag
        - functions that _do_ return an int, but the return value
          does not represent a (0/-1) error flag.
 */

 /*
   Functions that do not return an int.
 */
void coda_done(void);
double coda_NaN(void);
double coda_PlusInf(void);
double coda_MinInf(void);
const char *coda_type_get_format_name(coda_format format);
const char *coda_type_get_class_name(coda_type_class type_class);
const char *coda_type_get_native_type_name(coda_native_type native_type);
const char *coda_type_get_special_type_name(coda_special_type special_type);
const char *coda_expression_get_type_name(coda_expression_type type);
void coda_expression_delete(coda_expression *expr);
%ignore coda_done;
%ignore coda_NaN;
%ignore coda_PlusInf;
%ignore coda_MinInf;
%ignore coda_type_get_format_name;
%ignore coda_type_get_class_name;
%ignore coda_type_get_native_type_name;
%ignore coda_type_get_special_type_name;
%ignore coda_expression_get_type_name;
%ignore coda_expression_delete;

                 
/*
  Functions that return an int that represents a return value
  instead of, or in addition to, being an error flag.
*/
int coda_get_option_bypass_special_types(void);
int coda_get_option_perform_boundary_checks(void);
int coda_get_option_perform_conversions(void);
int coda_get_option_use_fast_size_expressions(void);
int coda_get_option_use_mmap(void);
int coda_isNaN(const double x);
int coda_isInf(const double x);
int coda_isPlusInf(const double x);
int coda_isMinInf(const double x);
int coda_expression_is_constant(const coda_expression *expr);
%ignore coda_get_option_bypass_special_types;
%ignore coda_get_option_perform_boundary_checks;
%ignore coda_get_option_perform_conversions;
%ignore coda_get_option_use_fast_size_expressions;
%ignore coda_get_option_use_mmap;
%ignore coda_isNaN;
%ignore coda_isInf;
%ignore coda_isPlusInf;
%ignore coda_isMinInf;
%ignore coda_expression_is_constant;



/*
    Default exception clause for CODA errors. this will raise a
    nl.stcorp.coda.CodaException exception in Java (which is a
    handwritten class).
*/
%javaexception("nl.stcorp.coda.CodaException")
{
    $action

    if (result < 0)
    {
        int namelen = strlen("$name");
        const char *codamsg = coda_errno_to_string(coda_errno);
        char *fullMessage = malloc(namelen + 4 + strlen(codamsg) + 1);
        jclass clazz = (*jenv)->FindClass(jenv, "nl/stcorp/coda/CodaException");
        
        sprintf(fullMessage, "$name(): %s", codamsg);
        (*jenv)->ThrowNew(jenv, clazz, fullMessage);
        free(fullMessage);
        return $null;
    }
}


/*
  Turn all the wrappers for int-returning functions into
  void-returning functions, because that return int is only the
  error flag, and has therefore now been superseded by the
  exception mechanism.

  NOTE: this should of course not be done for functions that
  return an int that actually returns a value. Hence the explicit
  inclusion and then %ignoring of these functions earlier in this
  section. All remaining int-returning functions are now
  implicitly assumed to be error-flag returning ones.

  However, this means that those function where the return value
  is *both* an error flag and something useful will now not get
  the exception handling applied.

  TODO: Fix this.  
*/
%typemap(out) int;
%typemap(out) int = void;


/*
----------------------------------------------------------------------------------------
- GLOBAL TYPEMAP ASSIGNMENTS                                                           -
----------------------------------------------------------------------------------------
*/


/*
  Typemap for scalar output arguments of type 'int':

  coda_double_to_datetime()::int *YEAR, int *MONTH, int *DAY,
                             int *HOUR, int *MINUTE, int *SECOND, int *MUSEC
  coda_double_to_utcdatetime()::int *YEAR, int *MONTH, int *DAY,
                                int *HOUR, int *MINUTE, int *SECOND, int *MUSEC
  coda_get_product_version()::int *version
  coda_type_get_record_field_hidden_status()::int *hidden
  coda_type_get_record_field_available_status()::int *available
  coda_type_get_record_union_status()::int *is_union
  coda_type_get_array_num_dims()::int *num_dims
  coda_type_get_array_dim()::int *num_dims
  coda_cursor_get_depth()::int *depth
  coda_cursor_get_record_field_available_status()::int *available
  coda_recognize_file()::int *product_version
  coda_expression_eval_boolean()::int *value
*/
%apply int *OUTPUT { int *YEAR, int *MONTH, int *DAY,
         int *HOUR, int *MINUTE, int *SECOND, int *MUSEC,
         int *version,
         int *has_ascii_content,
         int *hidden,
         int *available,
         int *is_union,
         int *num_dims,
         int *depth,
         int *product_version,
         int *value};


/*
  Typemap for scalar output arguments of type 'long':

  coda_type_get_string_length()::long *length
  coda_cursor_get_string_length()::long *length
  coda_type_get_num_record_fields()::long *num_fields
  coda_type_get_record_field_index_from_name()::long *index
  coda_type_get_record_field_index_from_real_name()::long *index
  coda_cursor_get_index()::long *index
  coda_cursor_get_record_field_index_from_name()::long *index
  coda_cursor_get_available_union_field_index()::long *index
  coda_cursor_get_num_elements()::long *num_elements
*/
%apply long *OUTPUT { long *length,
         long *num_fields,
         long *index,
         long *num_elements };


/*
  Typemap for scalar output arguments of type 'int64_t':
  
  coda_get_product_file_size()::int64_t *file_size
  coda_get_product_variable_value()::int64_t *value
  coda_type_get_bit_size()::int64_t *bit_size
  coda_cursor_get_bit_size()::int64_t *bit_size
  coda_cursor_get_byte_size()::int64_t *byte_size
  coda_cursor_get_file_bit_offset()::int64_t *bit_size
  coda_cursor_get_file_byte_offset()::int64_t *byte_size
  coda_recognize_file()::int64_t *file_size

  coda_expression_eval_integer()::int64_t *value

*/
%apply int64_t *OUTPUT { int64_t *file_size,
                         int64_t *bit_size,
                         int64_t *byte_size,
                         int64_t *bit_offset,
                         int64_t *byte_offset,
                         int64_t *value };


/*
  Typemap for scalar output arguments of type 'uint64_t':
*/

%apply uint64_t *OUTPUT { uint64_t *dst };


/*
  Typemap for scalar output arguments of type 'double':

  coda_cursor_read_complex_double_split()::double *dst_re, double *dst_im
  coda_datetime_to_double()::double *datetime
  coda_expression_eval_float()::double *value
  coda_utcdatetime_to_double()::double *datetime
  coda_string_to_time()::double *datetime
  coda_utcstring_to_time()::double *datetime
*/
%apply double *OUTPUT { double *dst_re, double *dst_im,
         double *datetime, double *value };


%apply double *OUTPUT { double *dst };
int coda_cursor_read_double(const coda_cursor *cursor, double *dst);
%ignore coda_cursor_read_double;

%apply uint64_t *OUTPUT { uint64_t *dst };
int coda_cursor_read_uint64(const coda_cursor *cursor, uint64_t *dst);
%ignore coda_cursor_read_uint64;



/* There appears to be no working array typemap for "unsigned
   long long" in arrays_java.i, so the following does not work
   (it generates the scalar code instead.
   
%apply unsigned long long[] { uint64_t *dst };
int coda_cursor_read_uint64_array(const coda_cursor *cursor, uint64_t *dst, coda_array_ordering array_ordering);
%ignore coda_cursor_read_uint64_array;

So for now we just ignore the array case.
*/
    

/*
  Typemaps for array output arguments of the primitive types.

  Note that these typemaps are implicitly applied only to the
  various read_* functions. For the read_*array variants, the
  output parameter actually *is* an array. For the read_*
  variants, the output parameter could also have used the scalar
  OUTPUT maps, but since we will be wrapping these functions
  anyway, we save ourselves the trouble and accept the array
  variant there as well. (Except for char*, see below, which
  needs to be handled separately and explicitly, because the
  default SWIG behaviour for that type is to convert it to a
  String input parameter.)

  TODO: the typemap for the uint64 array reading function does
  not work.
*/

%apply float[] { float *dst };
%apply double[] { double *dst };
%apply int8_t[]   { int8_t *dst };
%apply int8_t[]   { uint8_t *dst };
%apply int16_t[]  { int16_t *dst };
%apply uint16_t[] { uint16_t *dst };
%apply int32_t[]  { int32_t *dst };
%apply uint32_t[] { uint32_t *dst };
%apply int64_t[]  { int64_t *dst };
/* %apply uint64_t[]  { int64_t *dst }; */


/*
  Typemap for scalar output arguments of type 'char'.

  A 'signed char' typemap is applied, because just using 'char'
  will cause the default mapping to 'String' to be used.

  coda_cursor_read_char()::char *dst

  We explicitly mention the full function here and then ignore it
  afterwards, so that it will get processed here, and not with
  the other functions, which is in turn so that the subsequent
  array typemap (used for the char *dst parameter to
  read_array_char) will not be applied to the scalar version of
  that function.
  
*/
%apply int8_t *OUTPUT { char *dst };
int coda_cursor_read_char(const coda_cursor *cursor, char *dst);
%ignore coda_cursor_read_char;

%apply int8_t[] { char *dst };
int coda_cursor_read_char_array(const coda_cursor *cursor, char *dst, coda_array_ordering array_ordering);
%ignore coda_cursor_read_char_array;


 /*
   Typemap for char ** (i.e. string) output arguments that are
   memory-managed by CODA itself:

   coda_recognize_file()::const char **product_class, const char **product_type
   coda_get_product_filename()::const char **filename
   coda_get_product_class()::const char **product_class
   coda_get_product_type()::const char **product_type
   coda_get_product_definition_file()::const char **definition_file
   coda_type_get_name()::const char **name
   coda_type_get_description()::const char **description
   coda_type_get_unit()::const char **unit
   coda_type_get_fixed_value()::const char **fixed_value
   coda_type_get_record_field_name()::const char **name
   coda_type_get_record_field_real_name()::const char **real_name

   coda_expression_eval_string()::char **value
   
*/
%apply char **STRING_OUT { const char **product_class,
         const char **product_type,
         const char **definition_file,
         const char **filename,
         const char **name,
         const char **description,
         const char **unit,
         const char **fixed_value,
         const char **real_name,
         char **value};

/*
  Typemap for enum output arguments.

  NOTE: this implicit map assumes all 'enum SWIGTYPE *' arguments
  are output arguments.
*/
%apply int *OUTPUT { enum SWIGTYPE * };


/*
  Typemap for coda_product ** output arguments that are
  memory-managed by CODA itself:

  coda_open()::coda_product **pf;
  coda_open_as()::coda_product **pf;
  coda_cursor_get_product_file()::coda_product **pf;

  Code adapted from the Java section of the SWIG manual.

  TODO: Refactor the duplicate use of these typemaps for Products
  and Types into a single macro.
*/

%typemap(jni) coda_product ** "jobject"
%typemap(jtype) coda_product ** "SWIGTYPE_p_coda_product_struct"
%typemap(jstype) coda_product ** "SWIGTYPE_p_coda_product_struct"

%typemap(in) coda_product ** (coda_product *ppcoda_product = 0)
%{
    $1 = &ppcoda_product;
%}

%typemap(argout) coda_product **
{
    /* Give Java proxy the C pointer (of newly created object) */
    jclass clazz = (*jenv)->FindClass(jenv, "nl/stcorp/coda/SWIGTYPE_p_coda_product_struct");
    jfieldID fid = (*jenv)->GetFieldID(jenv, clazz, "swigCPtr", "J");
    jlong cPtr = 0;
    *(coda_product **)&cPtr = *$1;
    (*jenv)->SetLongField(jenv, $input, fid, cPtr);
}

%typemap(javain) coda_product ** "$javainput"


/*
  Typemap for coda_type ** output arguments that are
  memory-managed by CODA itself:

  coda_get_product_root_type()::coda_type **type
  coda_type_get_attributes()::coda_type **attributes
  coda_type_get_record_field_type()::coda_type **field_type
  coda_type_get_array_base_type()::coda_type **base_type
  coda_type_get_special_base_type()::coda_type **base_type
  coda_cursor_get_type()::coda_type **type

  Code adapted from the Java section of the SWIG manual.

  TODO: Refactor the duplicate use of these typemaps for Products
  and Types into a single macro.
*/

%typemap(jni) coda_type ** "jobject"
%typemap(jtype) coda_type ** "SWIGTYPE_p_coda_type_struct"
%typemap(jstype) coda_type ** "SWIGTYPE_p_coda_type_struct"

%typemap(in) coda_type ** (coda_type *ppcoda_type = 0)
%{
    $1 = &ppcoda_type;
%}

%typemap(argout) coda_type **
{
    /* Give Java proxy the C pointer (of newly created object) */
    jclass clazz = (*jenv)->FindClass(jenv, "nl/stcorp/coda/SWIGTYPE_p_coda_type_struct");
    jfieldID fid = (*jenv)->GetFieldID(jenv, clazz, "swigCPtr", "J");
    jlong cPtr = 0;
    *(coda_type **)&cPtr = *$1;
    (*jenv)->SetLongField(jenv, $input, fid, cPtr);
}

%typemap(javain) coda_type ** "$javainput"




/*
  Typemap for coda_expression ** output arguments that are
  memory-managed by CODA itself:

  coda_expression_from_string()::coda_expression **expr

  Code adapted from the Java section of the SWIG manual.

  TODO: Refactor the duplicate use of these typemaps for Products
  and Types Expressions into a single macro.
*/

%typemap(jni) coda_expression ** "jobject"
%typemap(jtype) coda_expression ** "SWIGTYPE_p_coda_expression_struct"
%typemap(jstype) coda_expression ** "SWIGTYPE_p_coda_expression_struct"

%typemap(in) coda_expression ** (coda_expression *ppcoda_expression = 0)
%{
    $1 = &ppcoda_expression;
%}

%typemap(argout) coda_expression **
{
    /* Give Java proxy the C pointer (of newly created object) */
    jclass clazz = (*jenv)->FindClass(jenv, "nl/stcorp/coda/SWIGTYPE_p_coda_expression_struct");
    jfieldID fid = (*jenv)->GetFieldID(jenv, clazz, "swigCPtr", "J");
    jlong cPtr = 0;
    *(coda_expression **)&cPtr = *$1;
    (*jenv)->SetLongField(jenv, $input, fid, cPtr);
}

%typemap(javain) coda_expression ** "$javainput"



 /* This typemap is used to ensure that the **value string
   allocated by CODA for the coda_expression_eval_string() method
   (see also the STRING_OUT typemaps below) is properly freed
   using coda_free.
 */   
%typemap(freearg) char **value 
{
        coda_free(*$1);
}
 

/*
----------------------------------------------------------------------------------------
- MAIN HEADER FILE INCLUDE (coda.h)                                                    -
----------------------------------------------------------------------------------------
*/

/*
    Wrap everything in coda.h
*/
%include "coda.h"

