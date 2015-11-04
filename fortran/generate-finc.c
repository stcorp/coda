/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coda.h"

static void print_copyright_notice(void)
{
    printf("C Copyright (C) 2007-2010 S[&]T, The Netherlands.\n");
    printf("C\n");
    printf("C This file is part of CODA.\n");
    printf("C\n");
    printf("C CODA is free software; you can redistribute it and/or modify\n");
    printf("C it under the terms of the GNU General Public License as published by\n");
    printf("C the Free Software Foundation; either version 2 of the License, or\n");
    printf("C (at your option) any later version.\n");
    printf("C\n");
    printf("C CODA is distributed in the hope that it will be useful,\n");
    printf("C but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("C MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("C GNU General Public License for more details.\n");
    printf("C\n");
    printf("C You should have received a copy of the GNU General Public License\n");
    printf("C along with CODA; if not, write to the Free Software\n");
    printf("C Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    printf("C\n");
    printf("\n");
}

static void print_typedef(const char *format, const char *name)
{
    printf("      ");
    switch (format[0])
    {
        case 'c':
            printf("character");
            break;
        case 'i':
            printf("integer");
            break;
    }
    printf(" %s\n", name);
    printf("\n");
}

static void print_parameterdef1(const char *name)
{
    printf("      parameter (%s =", name);
    if (strlen(name) > 45)
    {
        printf("\n     $");
    }
    printf(" ");
}

static void print_i(int i)
{
    printf("%i", i);
}

static void print_parameterdef2(void)
{
    printf(")\n");
}

static void print_funcdef(const char *name, const char *returntype)
{
    if (strcmp(returntype, "void") == 0)
    {
        return;
    }
    printf("      ");
    if (strcmp(returntype, "int") == 0)
    {
        printf("integer ");
    }
    else if (strcmp(returntype, "double") == 0)
    {
        printf("double precision ");
    }
    else if (strcmp(returntype, "void *") == 0)
    {
        printf("integer ");
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown return type '%s' for function '%s'\n", returntype, name);
        exit(1);
    }
    printf("%s\n", name);
}

#define PRINT_EMPTY printf("\n")

#define PRINT_CONSTANT(NAME,FORMAT) { print_typedef(# FORMAT, # NAME); print_parameterdef1(# NAME); print_##FORMAT(NAME); print_parameterdef2();}

#define PRINT_FUNCDEF(NAME,RETURNTYPE) print_funcdef(# NAME, # RETURNTYPE)

static void print_constants(void)
{
    PRINT_CONSTANT(CODA_MAX_NUM_DIMS, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(CODA_SUCCESS, i);
    PRINT_CONSTANT(CODA_ERROR_OUT_OF_MEMORY, i);
    PRINT_CONSTANT(CODA_ERROR_HDF4, i);
    PRINT_CONSTANT(CODA_ERROR_NO_HDF4_SUPPORT, i);
    PRINT_CONSTANT(CODA_ERROR_HDF5, i);
    PRINT_CONSTANT(CODA_ERROR_NO_HDF5_SUPPORT, i);
    PRINT_CONSTANT(CODA_ERROR_XML, i);

    PRINT_CONSTANT(CODA_ERROR_FILE_NOT_FOUND, i);
    PRINT_CONSTANT(CODA_ERROR_FILE_OPEN, i);
    PRINT_CONSTANT(CODA_ERROR_FILE_READ, i);
    PRINT_CONSTANT(CODA_ERROR_FILE_WRITE, i);

    PRINT_CONSTANT(CODA_ERROR_INVALID_ARGUMENT, i);
    PRINT_CONSTANT(CODA_ERROR_INVALID_INDEX, i);
    PRINT_CONSTANT(CODA_ERROR_INVALID_NAME, i);
    PRINT_CONSTANT(CODA_ERROR_INVALID_FORMAT, i);
    PRINT_CONSTANT(CODA_ERROR_INVALID_DATETIME, i);
    PRINT_CONSTANT(CODA_ERROR_INVALID_TYPE, i);
    PRINT_CONSTANT(CODA_ERROR_ARRAY_NUM_DIMS_MISMATCH, i);
    PRINT_CONSTANT(CODA_ERROR_ARRAY_OUT_OF_BOUNDS, i);
    PRINT_CONSTANT(CODA_ERROR_NO_PARENT, i);

    PRINT_CONSTANT(CODA_ERROR_UNSUPPORTED_PRODUCT, i);

    PRINT_CONSTANT(CODA_ERROR_PRODUCT, i);
    PRINT_CONSTANT(CODA_ERROR_OUT_OF_BOUNDS_READ, i);

    PRINT_CONSTANT(CODA_ERROR_DATA_DEFINITION, i);
    PRINT_CONSTANT(CODA_ERROR_EXPRESSION, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_array_ordering_c, i);
    PRINT_CONSTANT(coda_array_ordering_fortran, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_ffs_error, i);
    PRINT_CONSTANT(coda_ffs_could_not_open_file, i);
    PRINT_CONSTANT(coda_ffs_could_not_access_directory, i);
    PRINT_CONSTANT(coda_ffs_unsupported_file, i);
    PRINT_CONSTANT(coda_ffs_match, i);
    PRINT_CONSTANT(coda_ffs_no_match, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_format_ascii, i);
    PRINT_CONSTANT(coda_format_binary, i);
    PRINT_CONSTANT(coda_format_xml, i);
    PRINT_CONSTANT(coda_format_hdf4, i);
    PRINT_CONSTANT(coda_format_hdf5, i);
    PRINT_CONSTANT(coda_format_cdf, i);
    PRINT_CONSTANT(coda_format_netcdf, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_record_class, i);
    PRINT_CONSTANT(coda_array_class, i);
    PRINT_CONSTANT(coda_integer_class, i);
    PRINT_CONSTANT(coda_real_class, i);
    PRINT_CONSTANT(coda_text_class, i);
    PRINT_CONSTANT(coda_raw_class, i);
    PRINT_CONSTANT(coda_special_class, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_special_no_data, i);
    PRINT_CONSTANT(coda_special_vsf_integer, i);
    PRINT_CONSTANT(coda_special_time, i);
    PRINT_CONSTANT(coda_special_complex, i);

    PRINT_EMPTY;

    PRINT_CONSTANT(coda_native_type_not_available, i);
    PRINT_CONSTANT(coda_native_type_int8, i);
    PRINT_CONSTANT(coda_native_type_uint8, i);
    PRINT_CONSTANT(coda_native_type_int16, i);
    PRINT_CONSTANT(coda_native_type_uint16, i);
    PRINT_CONSTANT(coda_native_type_int32, i);
    PRINT_CONSTANT(coda_native_type_uint32, i);
    PRINT_CONSTANT(coda_native_type_int64, i);
    PRINT_CONSTANT(coda_native_type_uint64, i);
    PRINT_CONSTANT(coda_native_type_float, i);
    PRINT_CONSTANT(coda_native_type_double, i);
    PRINT_CONSTANT(coda_native_type_char, i);
    PRINT_CONSTANT(coda_native_type_string, i);
    PRINT_CONSTANT(coda_native_type_bytes, i);
}

static void print_function_definitions(void)
{
    PRINT_FUNCDEF(coda_version, void);
    PRINT_FUNCDEF(coda_init, int);
    PRINT_FUNCDEF(coda_done, void);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_set_definition_path, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_set_option_bypass_special_types, int);
    PRINT_FUNCDEF(coda_get_option_bypass_special_types, int);
    PRINT_FUNCDEF(coda_set_option_perform_boundary_checks, int);
    PRINT_FUNCDEF(coda_get_option_perform_boundary_checks, int);
    PRINT_FUNCDEF(coda_set_option_perform_conversions, int);
    PRINT_FUNCDEF(coda_get_option_perform_conversions, int);
    PRINT_FUNCDEF(coda_set_option_use_fast_size_expressions, int);
    PRINT_FUNCDEF(coda_get_option_use_fast_size_expressions, int);
    PRINT_FUNCDEF(coda_set_option_use_mmap, int);
    PRINT_FUNCDEF(coda_get_option_use_mmap, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_NaN, double);
    PRINT_FUNCDEF(coda_isNaN, int);
    PRINT_FUNCDEF(coda_PlusInf, double);
    PRINT_FUNCDEF(coda_MinInf, double);
    PRINT_FUNCDEF(coda_isInf, int);
    PRINT_FUNCDEF(coda_isPlusInf, int);
    PRINT_FUNCDEF(coda_isMinInf, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_c_index_to_fortran_index, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_time, double);
    PRINT_FUNCDEF(coda_datetime_to_double, int);
    PRINT_FUNCDEF(coda_double_to_datetime, int);
    PRINT_FUNCDEF(coda_time_to_string, int);
    PRINT_FUNCDEF(coda_string_to_time, int);
    PRINT_FUNCDEF(coda_utcdatetime_to_double, int);
    PRINT_FUNCDEF(coda_double_to_utcdatetime, int);
    PRINT_FUNCDEF(coda_time_to_utcstring, int);
    PRINT_FUNCDEF(coda_utcstring_to_time, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_get_errno, int);
    PRINT_FUNCDEF(coda_errno_to_string, void);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_recognize_file, int);

    PRINT_FUNCDEF(coda_open, int);
    PRINT_FUNCDEF(coda_close, int);

    PRINT_FUNCDEF(coda_get_product_filename, int);
    PRINT_FUNCDEF(coda_get_product_file_size, int);
    PRINT_FUNCDEF(coda_get_product_format, int);
    PRINT_FUNCDEF(coda_get_product_class, int);
    PRINT_FUNCDEF(coda_get_product_type, int);
    PRINT_FUNCDEF(coda_get_product_version, int);
    PRINT_FUNCDEF(coda_get_product_definition_file, int);
    PRINT_FUNCDEF(coda_get_product_root_type, int);

    PRINT_FUNCDEF(coda_get_product_variable_value, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_type_get_format_name, void);
    PRINT_FUNCDEF(coda_type_get_class_name, void);
    PRINT_FUNCDEF(coda_type_get_native_type_name, void);
    PRINT_FUNCDEF(coda_type_get_special_type_name, void);

    PRINT_FUNCDEF(coda_type_has_ascii_content, int);

    PRINT_FUNCDEF(coda_type_get_format, int);
    PRINT_FUNCDEF(coda_type_get_class, int);
    PRINT_FUNCDEF(coda_type_get_read_type, int);
    PRINT_FUNCDEF(coda_type_get_string_length, int);
    PRINT_FUNCDEF(coda_type_get_bit_size, int);
    PRINT_FUNCDEF(coda_type_get_name, int);
    PRINT_FUNCDEF(coda_type_get_description, int);
    PRINT_FUNCDEF(coda_type_get_unit, int);
    PRINT_FUNCDEF(coda_type_get_fixed_value, int);

    PRINT_FUNCDEF(coda_type_get_num_record_fields, int);
    PRINT_FUNCDEF(coda_type_get_record_field_index_from_name, int);
    PRINT_FUNCDEF(coda_type_get_record_field_type, int);
    PRINT_FUNCDEF(coda_type_get_record_field_name, int);
    PRINT_FUNCDEF(coda_type_get_record_field_real_name, int);
    PRINT_FUNCDEF(coda_type_get_record_field_hidden_status, int);
    PRINT_FUNCDEF(coda_type_get_record_field_available_status, int);
    PRINT_FUNCDEF(coda_type_get_record_union_status, int);

    PRINT_FUNCDEF(coda_type_get_array_num_dims, int);
    PRINT_FUNCDEF(coda_type_get_array_dim, int);
    PRINT_FUNCDEF(coda_type_get_array_base_type, int);

    PRINT_FUNCDEF(coda_type_get_special_type, int);
    PRINT_FUNCDEF(coda_type_get_special_base_type, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_cursor_new, void *);
    PRINT_FUNCDEF(coda_cursor_duplicate, void *);
    PRINT_FUNCDEF(coda_cursor_delete, void);

    PRINT_FUNCDEF(coda_cursor_set_product, int);

    PRINT_FUNCDEF(coda_cursor_goto_first_record_field, int);
    PRINT_FUNCDEF(coda_cursor_goto_next_record_field, int);
    PRINT_FUNCDEF(coda_cursor_goto_record_field_by_index, int);
    PRINT_FUNCDEF(coda_cursor_goto_record_field_by_name, int);
    PRINT_FUNCDEF(coda_cursor_goto_available_union_field, int);

    PRINT_FUNCDEF(coda_cursor_goto_first_array_element, int);
    PRINT_FUNCDEF(coda_cursor_goto_next_array_element, int);
    PRINT_FUNCDEF(coda_cursor_goto_array_element, int);
    PRINT_FUNCDEF(coda_cursor_goto_array_element_by_index, int);

    PRINT_FUNCDEF(coda_cursor_goto_attributes, int);

    PRINT_FUNCDEF(coda_cursor_goto_root, int);
    PRINT_FUNCDEF(coda_cursor_goto_parent, int);

    PRINT_FUNCDEF(coda_cursor_use_base_type_of_special_type, int);

    PRINT_FUNCDEF(coda_cursor_has_ascii_content, int);

    PRINT_FUNCDEF(coda_cursor_get_string_length, int);
    PRINT_FUNCDEF(coda_cursor_get_bit_size, int);
    PRINT_FUNCDEF(coda_cursor_get_byte_size, int);
    PRINT_FUNCDEF(coda_cursor_get_num_elements, int);

    PRINT_FUNCDEF(coda_cursor_get_product_file, int);

    PRINT_FUNCDEF(coda_cursor_get_depth, int);
    PRINT_FUNCDEF(coda_cursor_get_index, int);

    PRINT_FUNCDEF(coda_cursor_get_file_bit_offset, int);
    PRINT_FUNCDEF(coda_cursor_get_file_byte_offset, int);

    PRINT_FUNCDEF(coda_cursor_get_format, int);
    PRINT_FUNCDEF(coda_cursor_get_type_class, int);
    PRINT_FUNCDEF(coda_cursor_get_read_type, int);
    PRINT_FUNCDEF(coda_cursor_get_special_type, int);
    PRINT_FUNCDEF(coda_cursor_get_type, int);

    PRINT_FUNCDEF(coda_cursor_get_record_field_index_from_name, int);
    PRINT_FUNCDEF(coda_cursor_get_record_field_available_status, int);
    PRINT_FUNCDEF(coda_cursor_get_available_union_field_index, int);

    PRINT_FUNCDEF(coda_cursor_get_array_dim, int);

    PRINT_EMPTY;

    PRINT_FUNCDEF(coda_cursor_read_int8, int);
    PRINT_FUNCDEF(coda_cursor_read_uint8, int);
    PRINT_FUNCDEF(coda_cursor_read_int16, int);
    PRINT_FUNCDEF(coda_cursor_read_uint16, int);
    PRINT_FUNCDEF(coda_cursor_read_int32, int);
    PRINT_FUNCDEF(coda_cursor_read_uint32, int);
    PRINT_FUNCDEF(coda_cursor_read_int64, int);
    PRINT_FUNCDEF(coda_cursor_read_uint64, int);

    PRINT_FUNCDEF(coda_cursor_read_float, int);
    PRINT_FUNCDEF(coda_cursor_read_double, int);

    PRINT_FUNCDEF(coda_cursor_read_char, int);
    PRINT_FUNCDEF(coda_cursor_read_string, int);

    PRINT_FUNCDEF(coda_cursor_read_bits, int);
    PRINT_FUNCDEF(coda_cursor_read_bytes, int);

    PRINT_FUNCDEF(coda_cursor_read_int8_array, int);
    PRINT_FUNCDEF(coda_cursor_read_uint8_array, int);
    PRINT_FUNCDEF(coda_cursor_read_int16_array, int);
    PRINT_FUNCDEF(coda_cursor_read_uint16_array, int);
    PRINT_FUNCDEF(coda_cursor_read_int32_array, int);
    PRINT_FUNCDEF(coda_cursor_read_uint32_array, int);
    PRINT_FUNCDEF(coda_cursor_read_int64_array, int);
    PRINT_FUNCDEF(coda_cursor_read_uint64_array, int);
    PRINT_FUNCDEF(coda_cursor_read_float_array, int);
    PRINT_FUNCDEF(coda_cursor_read_double_array, int);
    PRINT_FUNCDEF(coda_cursor_read_char_array, int);

    PRINT_FUNCDEF(coda_cursor_read_complex_double_pair, int);
    PRINT_FUNCDEF(coda_cursor_read_complex_double_pairs_array, int);
    PRINT_FUNCDEF(coda_cursor_read_complex_double_split, int);
    PRINT_FUNCDEF(coda_cursor_read_complex_double_split_array, int);
}

int main()
{
    print_copyright_notice();

    PRINT_EMPTY;

    print_constants();

    PRINT_EMPTY;

    print_function_definitions();

    return 0;
}
