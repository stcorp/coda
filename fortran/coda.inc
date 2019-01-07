C Copyright (C) 2007-2019 S[&]T, The Netherlands.
C All rights reserved.
C
C Redistribution and use in source and binary forms, with or without
C modification, are permitted provided that the following conditions are met:
C
C 1. Redistributions of source code must retain the above copyright notice,
C    this list of conditions and the following disclaimer.
C
C 2. Redistributions in binary form must reproduce the above copyright
C    notice, this list of conditions and the following disclaimer in the
C    documentation and/or other materials provided with the distribution.
C
C 3. Neither the name of the copyright holder nor the names of its
C    contributors may be used to endorse or promote products derived from
C    this software without specific prior written permission.
C
C THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
C AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
C IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
C ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
C LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
C CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
C SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
C INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
C CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
C ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
C POSSIBILITY OF SUCH DAMAGE.


      integer CODA_MAX_NUM_DIMS
      parameter (CODA_MAX_NUM_DIMS = 8)

      integer CODA_SUCCESS
      parameter (CODA_SUCCESS = 0)
      integer CODA_ERROR_OUT_OF_MEMORY
      parameter (CODA_ERROR_OUT_OF_MEMORY = -1)
      integer CODA_ERROR_HDF4
      parameter (CODA_ERROR_HDF4 = -10)
      integer CODA_ERROR_NO_HDF4_SUPPORT
      parameter (CODA_ERROR_NO_HDF4_SUPPORT = -11)
      integer CODA_ERROR_HDF5
      parameter (CODA_ERROR_HDF5 = -12)
      integer CODA_ERROR_NO_HDF5_SUPPORT
      parameter (CODA_ERROR_NO_HDF5_SUPPORT = -13)
      integer CODA_ERROR_XML
      parameter (CODA_ERROR_XML = -14)
      integer CODA_ERROR_FILE_NOT_FOUND
      parameter (CODA_ERROR_FILE_NOT_FOUND = -20)
      integer CODA_ERROR_FILE_OPEN
      parameter (CODA_ERROR_FILE_OPEN = -21)
      integer CODA_ERROR_FILE_READ
      parameter (CODA_ERROR_FILE_READ = -22)
      integer CODA_ERROR_FILE_WRITE
      parameter (CODA_ERROR_FILE_WRITE = -23)
      integer CODA_ERROR_INVALID_ARGUMENT
      parameter (CODA_ERROR_INVALID_ARGUMENT = -100)
      integer CODA_ERROR_INVALID_INDEX
      parameter (CODA_ERROR_INVALID_INDEX = -101)
      integer CODA_ERROR_INVALID_NAME
      parameter (CODA_ERROR_INVALID_NAME = -102)
      integer CODA_ERROR_INVALID_FORMAT
      parameter (CODA_ERROR_INVALID_FORMAT = -103)
      integer CODA_ERROR_INVALID_DATETIME
      parameter (CODA_ERROR_INVALID_DATETIME = -104)
      integer CODA_ERROR_INVALID_TYPE
      parameter (CODA_ERROR_INVALID_TYPE = -105)
      integer CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH
      parameter (CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH = -106)
      integer CODA_ERROR_ARRAY_OUT_OF_BOUNDS
      parameter (CODA_ERROR_ARRAY_OUT_OF_BOUNDS = -107)
      integer CODA_ERROR_NO_PARENT
      parameter (CODA_ERROR_NO_PARENT = -108)
      integer CODA_ERROR_UNSUPPORTED_PRODUCT
      parameter (CODA_ERROR_UNSUPPORTED_PRODUCT = -200)
      integer CODA_ERROR_PRODUCT
      parameter (CODA_ERROR_PRODUCT = -300)
      integer CODA_ERROR_OUT_OF_BOUNDS_READ
      parameter (CODA_ERROR_OUT_OF_BOUNDS_READ = -301)
      integer CODA_ERROR_DATA_DEFINITION
      parameter (CODA_ERROR_DATA_DEFINITION = -400)
      integer CODA_ERROR_EXPRESSION
      parameter (CODA_ERROR_EXPRESSION = -401)

      integer coda_array_ordering_c
      parameter (coda_array_ordering_c = 0)
      integer coda_array_ordering_fortran
      parameter (coda_array_ordering_fortran = 1)

      integer coda_ffs_error
      parameter (coda_ffs_error = 0)
      integer coda_ffs_could_not_open_file
      parameter (coda_ffs_could_not_open_file = 1)
      integer coda_ffs_could_not_access_directory
      parameter (coda_ffs_could_not_access_directory = 2)
      integer coda_ffs_unsupported_file
      parameter (coda_ffs_unsupported_file = 3)
      integer coda_ffs_match
      parameter (coda_ffs_match = 4)
      integer coda_ffs_no_match
      parameter (coda_ffs_no_match = 5)

      integer coda_format_ascii
      parameter (coda_format_ascii = 0)
      integer coda_format_binary
      parameter (coda_format_binary = 1)
      integer coda_format_xml
      parameter (coda_format_xml = 2)
      integer coda_format_hdf4
      parameter (coda_format_hdf4 = 3)
      integer coda_format_hdf5
      parameter (coda_format_hdf5 = 4)
      integer coda_format_cdf
      parameter (coda_format_cdf = 5)
      integer coda_format_netcdf
      parameter (coda_format_netcdf = 6)
      integer coda_format_grib1
      parameter (coda_format_grib1 = 7)
      integer coda_format_grib2
      parameter (coda_format_grib2 = 8)
      integer coda_format_rinex
      parameter (coda_format_rinex = 9)
      integer coda_format_sp3
      parameter (coda_format_sp3 = 10)

      integer coda_record_class
      parameter (coda_record_class = 0)
      integer coda_array_class
      parameter (coda_array_class = 1)
      integer coda_integer_class
      parameter (coda_integer_class = 2)
      integer coda_real_class
      parameter (coda_real_class = 3)
      integer coda_text_class
      parameter (coda_text_class = 4)
      integer coda_raw_class
      parameter (coda_raw_class = 5)
      integer coda_special_class
      parameter (coda_special_class = 6)

      integer coda_special_no_data
      parameter (coda_special_no_data = 0)
      integer coda_special_vsf_integer
      parameter (coda_special_vsf_integer = 1)
      integer coda_special_time
      parameter (coda_special_time = 2)
      integer coda_special_complex
      parameter (coda_special_complex = 3)

      integer coda_native_type_not_available
      parameter (coda_native_type_not_available = -1)
      integer coda_native_type_int8
      parameter (coda_native_type_int8 = 0)
      integer coda_native_type_uint8
      parameter (coda_native_type_uint8 = 1)
      integer coda_native_type_int16
      parameter (coda_native_type_int16 = 2)
      integer coda_native_type_uint16
      parameter (coda_native_type_uint16 = 3)
      integer coda_native_type_int32
      parameter (coda_native_type_int32 = 4)
      integer coda_native_type_uint32
      parameter (coda_native_type_uint32 = 5)
      integer coda_native_type_int64
      parameter (coda_native_type_int64 = 6)
      integer coda_native_type_uint64
      parameter (coda_native_type_uint64 = 7)
      integer coda_native_type_float
      parameter (coda_native_type_float = 8)
      integer coda_native_type_double
      parameter (coda_native_type_double = 9)
      integer coda_native_type_char
      parameter (coda_native_type_char = 10)
      integer coda_native_type_string
      parameter (coda_native_type_string = 11)
      integer coda_native_type_bytes
      parameter (coda_native_type_bytes = 12)

      integer  coda_init

      integer  coda_set_definition_path
      integer  coda_set_definition_path_conditional

      integer  coda_set_option_bypass_special_types
      integer  coda_get_option_bypass_special_types
      integer  coda_set_option_perform_boundary_checks
      integer  coda_get_option_perform_boundary_checks
      integer  coda_set_option_perform_conversions
      integer  coda_get_option_perform_conversions
      integer  coda_set_option_use_fast_size_expressions
      integer  coda_get_option_use_fast_size_expressions
      integer  coda_set_option_use_mmap
      integer  coda_get_option_use_mmap

      double precision  coda_NaN
      integer  coda_isNaN
      double precision  coda_PlusInf
      double precision  coda_MinInf
      integer  coda_isInf
      integer  coda_isPlusInf
      integer  coda_isMinInf

      integer  coda_c_index_to_fortran_index

      double precision  coda_time
      integer  coda_time_double_to_parts
      integer  coda_time_double_to_parts_utc
      integer  coda_time_parts_to_double
      integer  coda_time_parts_to_double_utc
      integer  coda_time_parts_to_string
      integer  coda_time_string_to_parts
      integer  coda_time_double_to_string
      integer  coda_time_double_to_string_utc
      integer  coda_time_string_to_double
      integer  coda_time_string_to_double_utc

      integer  coda_datetime_to_double
      integer  coda_double_to_datetime
      integer  coda_time_to_string
      integer  coda_string_to_time
      integer  coda_utcdatetime_to_double
      integer  coda_double_to_utcdatetime
      integer  coda_time_to_utcstring
      integer  coda_utcstring_to_time

      integer  coda_get_errno

      integer  coda_recognize_file
      integer  coda_open
      integer  coda_open_as
      integer  coda_close
      integer  coda_get_product_filename
      integer  coda_get_product_file_size
      integer  coda_get_product_format
      integer  coda_get_product_class
      integer  coda_get_product_type
      integer  coda_get_product_version
      integer  coda_get_product_definition_file
      integer  coda_get_product_root_type
      integer  coda_get_product_variable_value

      integer  coda_type_has_attributes
      integer  coda_type_get_format
      integer  coda_type_get_class
      integer  coda_type_get_read_type
      integer  coda_type_get_string_length
      integer  coda_type_get_bit_size
      integer  coda_type_get_name
      integer  coda_type_get_description
      integer  coda_type_get_unit
      integer  coda_type_get_fixed_value
      integer  coda_type_get_attributes
      integer  coda_type_get_num_record_fields
      integer  coda_type_get_record_field_index_from_name
      integer  coda_type_get_record_field_index_from_real_name
      integer  coda_type_get_record_field_type
      integer  coda_type_get_record_field_name
      integer  coda_type_get_record_field_real_name
      integer  coda_type_get_record_field_hidden_status
      integer  coda_type_get_record_field_available_status
      integer  coda_type_get_record_union_status
      integer  coda_type_get_array_num_dims
      integer  coda_type_get_array_dim
      integer  coda_type_get_array_base_type
      integer  coda_type_get_special_type
      integer  coda_type_get_special_base_type

C use 'integer coda_cursor_new' for 32-bit
      integer*8  coda_cursor_new
C use 'integer coda_cursor_duplicate' for 32-bit
      integer*8  coda_cursor_duplicate
      integer  coda_cursor_set_product
      integer  coda_cursor_goto
      integer  coda_cursor_goto_first_record_field
      integer  coda_cursor_goto_next_record_field
      integer  coda_cursor_goto_record_field_by_index
      integer  coda_cursor_goto_record_field_by_name
      integer  coda_cursor_goto_available_union_field
      integer  coda_cursor_goto_first_array_element
      integer  coda_cursor_goto_next_array_element
      integer  coda_cursor_goto_array_element
      integer  coda_cursor_goto_array_element_by_index
      integer  coda_cursor_goto_attributes
      integer  coda_cursor_goto_root
      integer  coda_cursor_goto_parent
      integer  coda_cursor_use_base_type_of_special_type
      integer  coda_cursor_has_ascii_content
      integer  coda_cursor_has_attributes
      integer  coda_cursor_get_string_length
      integer  coda_cursor_get_bit_size
      integer  coda_cursor_get_byte_size
      integer  coda_cursor_get_num_elements
      integer  coda_cursor_get_product_file
      integer  coda_cursor_get_depth
      integer  coda_cursor_get_index
      integer  coda_cursor_get_file_bit_offset
      integer  coda_cursor_get_file_byte_offset
      integer  coda_cursor_get_format
      integer  coda_cursor_get_type_class
      integer  coda_cursor_get_read_type
      integer  coda_cursor_get_special_type
      integer  coda_cursor_get_type
      integer  coda_cursor_get_record_field_index_from_name
      integer  coda_cursor_get_record_field_available_status
      integer  coda_cursor_get_available_union_field_index
      integer  coda_cursor_get_array_dim

      integer  coda_cursor_read_int8
      integer  coda_cursor_read_uint8
      integer  coda_cursor_read_int16
      integer  coda_cursor_read_uint16
      integer  coda_cursor_read_int32
      integer  coda_cursor_read_uint32
      integer  coda_cursor_read_int64
      integer  coda_cursor_read_uint64
      integer  coda_cursor_read_float
      integer  coda_cursor_read_double
      integer  coda_cursor_read_char
      integer  coda_cursor_read_string
      integer  coda_cursor_read_bits
      integer  coda_cursor_read_bytes
      integer  coda_cursor_read_int8_array
      integer  coda_cursor_read_uint8_array
      integer  coda_cursor_read_int16_array
      integer  coda_cursor_read_uint16_array
      integer  coda_cursor_read_int32_array
      integer  coda_cursor_read_uint32_array
      integer  coda_cursor_read_int64_array
      integer  coda_cursor_read_uint64_array
      integer  coda_cursor_read_float_array
      integer  coda_cursor_read_double_array
      integer  coda_cursor_read_char_array
      integer  coda_cursor_read_int8_partial_array
      integer  coda_cursor_read_uint8_partial_array
      integer  coda_cursor_read_int16_partial_array
      integer  coda_cursor_read_uint16_partial_array
      integer  coda_cursor_read_int32_partial_array
      integer  coda_cursor_read_uint32_partial_array
      integer  coda_cursor_read_int64_partial_array
      integer  coda_cursor_read_uint64_partial_array
      integer  coda_cursor_read_float_partial_array
      integer  coda_cursor_read_double_partial_array
      integer  coda_cursor_read_char_partial_array
      integer  coda_cursor_read_complex_double_pair
      integer  coda_cursor_read_complex_double_pairs_array
      integer  coda_cursor_read_complex_double_split
      integer  coda_cursor_read_complex_double_split_array

      integer  coda_expression_from_string
      integer  coda_expression_get_type
      integer  coda_expression_is_constant
      integer  coda_expression_is_equal
      integer  coda_expression_eval_bool
      integer  coda_expression_eval_integer
      integer  coda_expression_eval_float
      integer  coda_expression_eval_string
      integer  coda_expression_eval_node
