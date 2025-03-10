/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.1.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package nl.stcorp.coda;

public class codacJNI {
  public final static native String helper_version();
  public final static native long new_coda_cursor();
  public final static native void delete_coda_cursor(long jarg1);
  public final static native long deepcopy_coda_cursor(long jarg1);
  public final static native String helper_coda_cursor_read_string(long jarg1);
  public final static native String helper_coda_time_parts_to_string(int jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, String jarg8);
  public final static native String helper_coda_time_double_to_string(double jarg1, String jarg2);
  public final static native String helper_coda_time_double_to_string_utc(double jarg1, String jarg2);
  public final static native String helper_coda_time_to_string(double jarg1);
  public final static native String helper_coda_time_to_utcstring(double jarg1);
  public final static native void done();
  public final static native double NaN();
  public final static native double PlusInf();
  public final static native double MinInf();
  public final static native String type_get_format_name(int jarg1);
  public final static native String type_get_class_name(int jarg1);
  public final static native String type_get_native_type_name(int jarg1);
  public final static native String type_get_special_type_name(int jarg1);
  public final static native String expression_get_type_name(int jarg1);
  public final static native void expression_delete(long jarg1);
  public final static native int get_option_bypass_special_types();
  public final static native int get_option_perform_boundary_checks();
  public final static native int get_option_perform_conversions();
  public final static native int get_option_use_fast_size_expressions();
  public final static native int get_option_use_mmap();
  public final static native int isNaN(double jarg1);
  public final static native int isInf(double jarg1);
  public final static native int isPlusInf(double jarg1);
  public final static native int isMinInf(double jarg1);
  public final static native int expression_is_constant(long jarg1);
  public final static native int expression_is_equal(long jarg1, long jarg2);
  public final static native int cursor_read_double(long jarg1, double[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint64(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_char(long jarg1, byte[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_char_array(long jarg1, byte[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_char_partial_array(long jarg1, int jarg2, int jarg3, byte[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int init() throws nl.stcorp.coda.CodaException;
  public final static native int set_definition_path(String jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int set_definition_path_conditional(String jarg1, String jarg2, String jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int set_option_bypass_special_types(int jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int set_option_perform_boundary_checks(int jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int set_option_perform_conversions(int jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int set_option_use_fast_size_expressions(int jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int set_option_use_mmap(int jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int c_index_to_fortran_index(int jarg1, int[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int time_double_to_parts(double jarg1, int[] jarg2, int[] jarg3, int[] jarg4, int[] jarg5, int[] jarg6, int[] jarg7, int[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int time_double_to_parts_utc(double jarg1, int[] jarg2, int[] jarg3, int[] jarg4, int[] jarg5, int[] jarg6, int[] jarg7, int[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int time_parts_to_double(int jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, double[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int time_parts_to_double_utc(int jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, double[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int time_string_to_parts(String jarg1, String jarg2, int[] jarg3, int[] jarg4, int[] jarg5, int[] jarg6, int[] jarg7, int[] jarg8, int[] jarg9) throws nl.stcorp.coda.CodaException;
  public final static native int time_string_to_double(String jarg1, String jarg2, double[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int time_string_to_double_utc(String jarg1, String jarg2, double[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int datetime_to_double(int jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, double[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int double_to_datetime(double jarg1, int[] jarg2, int[] jarg3, int[] jarg4, int[] jarg5, int[] jarg6, int[] jarg7, int[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int string_to_time(String jarg1, double[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int utcdatetime_to_double(int jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, double[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int double_to_utcdatetime(double jarg1, int[] jarg2, int[] jarg3, int[] jarg4, int[] jarg5, int[] jarg6, int[] jarg7, int[] jarg8) throws nl.stcorp.coda.CodaException;
  public final static native int utcstring_to_time(String jarg1, double[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int open(String jarg1, SWIGTYPE_p_coda_product_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int open_as(String jarg1, String jarg2, String jarg3, int jarg4, SWIGTYPE_p_coda_product_struct jarg5) throws nl.stcorp.coda.CodaException;
  public final static native int close(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_filename(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_file_size(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_format(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_class(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_type(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_version(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_definition_file(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_root_type(long jarg1, SWIGTYPE_p_coda_type_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int get_product_variable_value(long jarg1, String jarg2, int jarg3, long[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int type_has_attributes(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_format(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_class(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_read_type(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_string_length(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_bit_size(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_name(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_description(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_unit(long jarg1, String[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_fixed_value(long jarg1, String[] jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_attributes(long jarg1, SWIGTYPE_p_coda_type_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_num_record_fields(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_index_from_name(long jarg1, String jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_index_from_real_name(long jarg1, String jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_type(long jarg1, int jarg2, SWIGTYPE_p_coda_type_struct jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_name(long jarg1, int jarg2, String[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_real_name(long jarg1, int jarg2, String[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_hidden_status(long jarg1, int jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_field_available_status(long jarg1, int jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_record_union_status(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_array_num_dims(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_array_dim(long jarg1, int[] jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_array_base_type(long jarg1, SWIGTYPE_p_coda_type_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_special_type(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int type_get_special_base_type(long jarg1, SWIGTYPE_p_coda_type_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_set_product(long jarg1, long jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto(long jarg1, String jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_first_record_field(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_next_record_field(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_record_field_by_index(long jarg1, int jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_record_field_by_name(long jarg1, String jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_available_union_field(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_first_array_element(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_next_array_element(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_array_element(long jarg1, int jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_array_element_by_index(long jarg1, int jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_attributes(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_root(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_goto_parent(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_use_base_type_of_special_type(long jarg1) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_has_ascii_content(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_has_attributes(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_string_length(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_bit_size(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_byte_size(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_num_elements(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_product_file(long jarg1, SWIGTYPE_p_coda_product_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_depth(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_index(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_file_bit_offset(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_file_byte_offset(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_format(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_type_class(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_read_type(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_special_type(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_type(long jarg1, SWIGTYPE_p_coda_type_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_record_field_index_from_name(long jarg1, String jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_record_field_available_status(long jarg1, int jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_available_union_field_index(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_get_array_dim(long jarg1, int[] jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int8(long jarg1, byte[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint8(long jarg1, byte[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int16(long jarg1, short[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint16(long jarg1, short[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int32(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint32(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int64(long jarg1, long[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_float(long jarg1, float[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_bits(long jarg1, byte[] jarg2, long jarg3, long jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_bytes(long jarg1, byte[] jarg2, long jarg3, long jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int8_array(long jarg1, byte[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint8_array(long jarg1, byte[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int16_array(long jarg1, short[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint16_array(long jarg1, short[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int32_array(long jarg1, int[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint32_array(long jarg1, int[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int64_array(long jarg1, long[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint64_array(long jarg1, long[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_float_array(long jarg1, float[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_double_array(long jarg1, double[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int8_partial_array(long jarg1, int jarg2, int jarg3, byte[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint8_partial_array(long jarg1, int jarg2, int jarg3, byte[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int16_partial_array(long jarg1, int jarg2, int jarg3, short[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint16_partial_array(long jarg1, int jarg2, int jarg3, short[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int32_partial_array(long jarg1, int jarg2, int jarg3, int[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint32_partial_array(long jarg1, int jarg2, int jarg3, int[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_int64_partial_array(long jarg1, int jarg2, int jarg3, long[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_uint64_partial_array(long jarg1, int jarg2, int jarg3, long[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_float_partial_array(long jarg1, int jarg2, int jarg3, float[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_double_partial_array(long jarg1, int jarg2, int jarg3, double[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_complex_double_pair(long jarg1, double[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_complex_double_pairs_array(long jarg1, double[] jarg2, int jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_complex_double_split(long jarg1, double[] jarg2, double[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int cursor_read_complex_double_split_array(long jarg1, double[] jarg2, double[] jarg3, int jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int expression_from_string(String jarg1, SWIGTYPE_p_coda_expression_struct jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int expression_get_type(long jarg1, int[] jarg2) throws nl.stcorp.coda.CodaException;
  public final static native int expression_eval_bool(long jarg1, long jarg2, int[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int expression_eval_integer(long jarg1, long jarg2, long[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int expression_eval_float(long jarg1, long jarg2, double[] jarg3) throws nl.stcorp.coda.CodaException;
  public final static native int expression_eval_string(long jarg1, long jarg2, String[] jarg3, int[] jarg4) throws nl.stcorp.coda.CodaException;
  public final static native int expression_eval_node(long jarg1, long jarg2) throws nl.stcorp.coda.CodaException;
}
