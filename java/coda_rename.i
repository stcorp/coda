/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

%rename(init) coda_init;
%rename(done) coda_done;
%rename(set_option_bypass_special_types) coda_set_option_bypass_special_types;
%rename(get_option_bypass_special_types) coda_get_option_bypass_special_types;
%rename(set_option_perform_boundary_checks) coda_set_option_perform_boundary_checks;
%rename(get_option_perform_boundary_checks) coda_get_option_perform_boundary_checks;
%rename(set_option_perform_conversions) coda_set_option_perform_conversions;
%rename(get_option_perform_conversions) coda_get_option_perform_conversions;
%rename(set_option_use_fast_size_expressions) coda_set_option_use_fast_size_expressions;
%rename(get_option_use_fast_size_expressions) coda_get_option_use_fast_size_expressions;
%rename(set_option_use_mmap) coda_set_option_use_mmap;
%rename(get_option_use_mmap) coda_get_option_use_mmap;
%rename(set_definition_path) coda_set_definition_path;
%rename(set_definition_path_conditional) coda_set_definition_path_conditional;
%rename(NaN) coda_NaN;
%rename(isNaN) coda_isNaN;
%rename(PlusInf) coda_PlusInf;
%rename(MinInf) coda_MinInf;
%rename(isInf) coda_isInf;
%rename(isPlusInf) coda_isPlusInf;
%rename(isMinInf) coda_isMinInf;
%rename(c_index_to_fortran_index) coda_c_index_to_fortran_index;
%rename(time_double_to_parts) coda_time_double_to_parts;
%rename(time_double_to_parts_utc) coda_time_double_to_parts_utc;
%rename(time_parts_to_double) coda_time_parts_to_double;
%rename(time_parts_to_double_utc) coda_time_parts_to_double_utc;
%rename(time_parts_to_string) coda_time_parts_to_string;
%rename(time_string_to_parts) coda_time_string_to_parts;
%rename(time_double_to_string) coda_time_double_to_string;
%rename(time_double_to_string_utc) coda_time_double_to_string_utc;
%rename(time_string_to_double) coda_time_string_to_double;
%rename(time_string_to_double_utc) coda_time_string_to_double_utc;
%rename(datetime_to_double) coda_datetime_to_double;
%rename(utcdatetime_to_double) coda_utcdatetime_to_double;
%rename(double_to_datetime) coda_double_to_datetime;
%rename(double_to_utcdatetime) coda_double_to_utcdatetime;
%rename(string_to_time) coda_string_to_time;
%rename(utcstring_to_time) coda_utcstring_to_time;
%rename(match_filefilter) coda_match_filefilter;
%rename(set_error) coda_set_error;
%rename(errno_to_string) coda_errno_to_string;
%rename(recognize_file) coda_recognize_file;
%rename(open) coda_open;
%rename(open_as) coda_open_as;
%rename(close) coda_close;
%rename(get_product_filename) coda_get_product_filename;
%rename(get_product_file_size) coda_get_product_file_size;
%rename(get_product_format) coda_get_product_format;
%rename(get_product_class) coda_get_product_class;
%rename(get_product_type) coda_get_product_type;
%rename(get_product_version) coda_get_product_version;
%rename(get_product_root_type) coda_get_product_root_type;
%rename(get_product_definition_file) coda_get_product_definition_file;
%rename(get_product_variable_value) coda_get_product_variable_value;
%rename(type_get_format_name) coda_type_get_format_name;
%rename(type_get_class_name) coda_type_get_class_name;
%rename(type_get_native_type_name) coda_type_get_native_type_name;
%rename(type_get_special_type_name) coda_type_get_special_type_name;
%rename(type_has_attributes) coda_type_has_attributes;
%rename(type_get_format) coda_type_get_format;
%rename(type_get_class) coda_type_get_class;
%rename(type_get_read_type) coda_type_get_read_type;
%rename(type_get_string_length) coda_type_get_string_length;
%rename(type_get_bit_size) coda_type_get_bit_size;
%rename(type_get_name) coda_type_get_name;
%rename(type_get_description) coda_type_get_description;
%rename(type_get_unit) coda_type_get_unit;
%rename(type_get_fixed_value) coda_type_get_fixed_value;
%rename(type_get_attributes) coda_type_get_attributes;
%rename(type_get_num_record_fields) coda_type_get_num_record_fields;
%rename(type_get_record_field_index_from_name) coda_type_get_record_field_index_from_name;
%rename(type_get_record_field_index_from_real_name) coda_type_get_record_field_index_from_real_name;
%rename(type_get_record_field_type) coda_type_get_record_field_type;
%rename(type_get_record_field_name) coda_type_get_record_field_name;
%rename(type_get_record_field_real_name) coda_type_get_record_field_real_name;
%rename(type_get_record_field_hidden_status) coda_type_get_record_field_hidden_status;
%rename(type_get_record_field_available_status) coda_type_get_record_field_available_status;
%rename(type_get_record_union_status) coda_type_get_record_union_status;
%rename(type_get_array_num_dims) coda_type_get_array_num_dims;
%rename(type_get_array_dim) coda_type_get_array_dim;
%rename(type_get_array_base_type) coda_type_get_array_base_type;
%rename(type_get_special_type) coda_type_get_special_type;
%rename(type_get_special_base_type) coda_type_get_special_base_type;
%rename(cursor_set_product) coda_cursor_set_product;
%rename(cursor_goto) coda_cursor_goto;
%rename(cursor_goto_first_record_field) coda_cursor_goto_first_record_field;
%rename(cursor_goto_next_record_field) coda_cursor_goto_next_record_field;
%rename(cursor_goto_record_field_by_index) coda_cursor_goto_record_field_by_index;
%rename(cursor_goto_record_field_by_name) coda_cursor_goto_record_field_by_name;
%rename(cursor_goto_available_union_field) coda_cursor_goto_available_union_field;
%rename(cursor_goto_first_array_element) coda_cursor_goto_first_array_element;
%rename(cursor_goto_next_array_element) coda_cursor_goto_next_array_element;
%rename(cursor_goto_array_element) coda_cursor_goto_array_element;
%rename(cursor_goto_array_element_by_index) coda_cursor_goto_array_element_by_index;
%rename(cursor_goto_attributes) coda_cursor_goto_attributes;
%rename(cursor_goto_root) coda_cursor_goto_root;
%rename(cursor_goto_parent) coda_cursor_goto_parent;
%rename(cursor_use_base_type_of_special_type) coda_cursor_use_base_type_of_special_type;
%rename(cursor_has_ascii_content) coda_cursor_has_ascii_content;
%rename(cursor_has_attributes) coda_cursor_has_attributes;
%rename(cursor_get_string_length) coda_cursor_get_string_length;
%rename(cursor_get_bit_size) coda_cursor_get_bit_size;
%rename(cursor_get_byte_size) coda_cursor_get_byte_size;
%rename(cursor_get_num_elements) coda_cursor_get_num_elements;
%rename(cursor_get_product_file) coda_cursor_get_product_file;
%rename(cursor_get_depth) coda_cursor_get_depth;
%rename(cursor_get_index) coda_cursor_get_index;
%rename(cursor_get_file_bit_offset) coda_cursor_get_file_bit_offset;
%rename(cursor_get_file_byte_offset) coda_cursor_get_file_byte_offset;
%rename(cursor_get_format) coda_cursor_get_format;
%rename(cursor_get_type_class) coda_cursor_get_type_class;
%rename(cursor_get_read_type) coda_cursor_get_read_type;
%rename(cursor_get_special_type) coda_cursor_get_special_type;
%rename(cursor_get_type) coda_cursor_get_type;
%rename(cursor_get_record_field_index_from_name) coda_cursor_get_record_field_index_from_name;
%rename(cursor_get_record_field_available_status) coda_cursor_get_record_field_available_status;
%rename(cursor_get_available_union_field_index) coda_cursor_get_available_union_field_index;
%rename(cursor_get_array_dim) coda_cursor_get_array_dim;
%rename(cursor_read_int8) coda_cursor_read_int8;
%rename(cursor_read_uint8) coda_cursor_read_uint8;
%rename(cursor_read_int16) coda_cursor_read_int16;
%rename(cursor_read_uint16) coda_cursor_read_uint16;
%rename(cursor_read_int32) coda_cursor_read_int32;
%rename(cursor_read_uint32) coda_cursor_read_uint32;
%rename(cursor_read_int64) coda_cursor_read_int64;
%rename(cursor_read_uint64) coda_cursor_read_uint64;
%rename(cursor_read_float) coda_cursor_read_float;
%rename(cursor_read_double) coda_cursor_read_double;
%rename(cursor_read_char) coda_cursor_read_char;
%rename(cursor_read_string) coda_cursor_read_string;
%rename(cursor_read_bits) coda_cursor_read_bits;
%rename(cursor_read_bytes) coda_cursor_read_bytes;
%rename(cursor_read_int8_array) coda_cursor_read_int8_array;
%rename(cursor_read_uint8_array) coda_cursor_read_uint8_array;
%rename(cursor_read_int16_array) coda_cursor_read_int16_array;
%rename(cursor_read_uint16_array) coda_cursor_read_uint16_array;
%rename(cursor_read_int32_array) coda_cursor_read_int32_array;
%rename(cursor_read_uint32_array) coda_cursor_read_uint32_array;
%rename(cursor_read_int64_array) coda_cursor_read_int64_array;
%rename(cursor_read_uint64_array) coda_cursor_read_uint64_array;
%rename(cursor_read_float_array) coda_cursor_read_float_array;
%rename(cursor_read_double_array) coda_cursor_read_double_array;
%rename(cursor_read_char_array) coda_cursor_read_char_array;
%rename(cursor_read_int8_partial_array) coda_cursor_read_int8_partial_array;
%rename(cursor_read_uint8_partial_array) coda_cursor_read_uint8_partial_array;
%rename(cursor_read_int16_partial_array) coda_cursor_read_int16_partial_array;
%rename(cursor_read_uint16_partial_array) coda_cursor_read_uint16_partial_array;
%rename(cursor_read_int32_partial_array) coda_cursor_read_int32_partial_array;
%rename(cursor_read_uint32_partial_array) coda_cursor_read_uint32_partial_array;
%rename(cursor_read_int64_partial_array) coda_cursor_read_int64_partial_array;
%rename(cursor_read_uint64_partial_array) coda_cursor_read_uint64_partial_array;
%rename(cursor_read_float_partial_array) coda_cursor_read_float_partial_array;
%rename(cursor_read_double_partial_array) coda_cursor_read_double_partial_array;
%rename(cursor_read_char_partial_array) coda_cursor_read_char_partial_array;
%rename(cursor_read_complex_double_pair) coda_cursor_read_complex_double_pair;
%rename(cursor_read_complex_double_pairs_array) coda_cursor_read_complex_double_pairs_array;
%rename(cursor_read_complex_double_split) coda_cursor_read_complex_double_split;
%rename(cursor_read_complex_double_split_array) coda_cursor_read_complex_double_split_array;

%rename(expression_get_type_name) coda_expression_get_type_name;
%rename(expression_from_string) coda_expression_from_string;
%rename(expression_delete) coda_expression_delete;
%rename(expression_get_type) coda_expression_get_type;
%rename(expression_is_constant) coda_expression_is_constant;
%rename(expression_is_equal) coda_expression_is_equal;

%rename(expression_eval_bool) coda_expression_eval_bool;
%rename(expression_eval_integer) coda_expression_eval_integer;
%rename(expression_eval_float) coda_expression_eval_float;
%rename(expression_eval_string) coda_expression_eval_string;
%rename(expression_eval_node) coda_expression_eval_node;

