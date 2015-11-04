/*
 * Copyright (C) 2007-2013 S[&]T, The Netherlands.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "coda.h"

#ifdef WRAPFORTRAN_USE_UPPERCASE_IDENTIFIERS
#  define FNAME(UCN,LCN) UCN##_
#else
#  define FNAME(UCN,LCN) LCN##_
#endif

/* We use the UFNAME macro for identifiers that contain '_' characters.
 * The necessity for such a macro comes from the fact that for some fortran
 * compilers (such as g77) we need to add an additional '_' to identifiers
 * that contain one or more '_' characters.
 */
#ifdef WRAPFORTRAN_USE_ADDITIONAL_UNDERSCORE
#  define UFNAME(UCN,LCN) FNAME(UCN##_,LCN##_)
#else
#  define UFNAME(UCN,LCN) FNAME(UCN,LCN)
#endif

#define INSTR_BEGIN_DEF(STR) char * STR##_s; int STR##_l = STR##_size;
#define INSTR_BEGIN_INIT(STR) while (STR##_l > 0 && STR[STR##_l - 1] == ' ') { STR##_l--; } STR##_s = malloc(STR##_l + 1); memcpy(STR##_s, STR, STR##_l); STR##_s[STR##_l] = '\0';
#define INSTR_BEGIN(STR) INSTR_BEGIN_DEF(STR) INSTR_BEGIN_INIT(STR)
#define INSTR_END(STR) free(STR##_s);

#define OUTSTR_BEGIN(STR) const char * STR##_s = NULL; int STR##_l;
#define OUTSTR_END(STR) if (STR##_s != NULL) { STR##_l = strlen(STR##_s); if (STR##_l > STR##_size) { memcpy(STR, STR##_s, STR##_size); } else { memcpy(STR, STR##_s, STR##_l); while (STR##_l < STR##_size) { STR[STR##_l] = ' '; STR##_l++; } } } else { memset(STR, ' ', STR##_size); }

#define INOUTSTR_BEGIN_DEF(STR) char * STR##_s; int STR##_l = STR##_size;
#define INOUTSTR_BEGIN_INIT(STR) while (STR##_l > 0 && STR[STR##_l - 1] == ' ') { STR##_l--; } STR##_s = malloc(STR##_size + 1); memcpy(STR##_s, STR, STR##_l); STR##_s[STR##_l] = '\0';
#define INOUTSTR_BEGIN(STR) INOUTSTR_BEGIN_DEF(STR) INOUTSTR_BEGIN_INIT(STR)
#define INOUTSTR_END(STR) STR##_l = strlen(STR##_s); assert(STR##_l <= STR##_size); memcpy(STR, STR##_s, STR##_l); while (STR##_l < STR##_size) { STR[STR##_l] = ' '; STR##_l++; } free(STR##_s);

void UFNAME(CODA_VERSION,coda_version)(char *version, int version_size)
{
    OUTSTR_BEGIN(version)
    version_s = libcoda_version;
    OUTSTR_END(version)
}

int UFNAME(CODA_INIT,coda_init)(void)
{
    return coda_init();
}

void UFNAME(CODA_DONE,coda_done)(void)
{
    coda_done();
}

int UFNAME(CODA_SET_DEFINITION_PATH,coda_set_definition_path)(char *path, int path_size)
{
    int result;
    INSTR_BEGIN(path)
    result = coda_set_definition_path(path_s);
    INSTR_END(path)
    return result;
}

int UFNAME(CODA_SET_DEFINITION_PATH_CONDITIONAL,coda_set_definition_path_conditional)(char *file, char *searchpath, char *relative_location, int file_size, int searchpath_size, int relative_location_size)
{
    int result;
    INSTR_BEGIN_DEF(file)
    INSTR_BEGIN_DEF(searchpath)
    INSTR_BEGIN_DEF(relative_location)
    INSTR_BEGIN_INIT(file)
    INSTR_BEGIN_INIT(searchpath)
    INSTR_BEGIN_INIT(relative_location)
    if (searchpath_l > 0)
    {
        result = coda_set_definition_path_conditional(file_s, searchpath_s, relative_location_s);
    }
    else
    {
        result = coda_set_definition_path_conditional(file_s, NULL, relative_location_s);
    }
    INSTR_END(relative_location)
    INSTR_END(searchpath)
    INSTR_END(file)
    return result;
}

int UFNAME(CODA_SET_OPTION_BYPASS_SPECIAL_TYPES,coda_set_option_bypass_special_types)(int *enable)
{
    return coda_set_option_bypass_special_types(*enable);
}

int UFNAME(CODA_GET_OPTION_BYPASS_SPECIAL_TYPES,coda_get_option_bypass_special_types)(void)
{
    return coda_get_option_bypass_special_types();
}

int UFNAME(CODA_SET_OPTION_PERFORM_BOUNDARY_CHECKS,coda_set_option_perform_boundary_checks)(int *enable)
{
    return coda_set_option_perform_boundary_checks(*enable);
}

int UFNAME(CODA_GET_OPTION_PERFORM_BOUNDARY_CHECKS,coda_get_option_perform_boundary_checks)(void)
{
    return coda_get_option_perform_boundary_checks();
}

int UFNAME(CODA_SET_OPTION_PERFORM_CONVERSIONS,coda_set_option_perform_conversions)(int *enable)
{
    return coda_set_option_perform_conversions(*enable);
}

int UFNAME(CODA_GET_OPTION_PERFORM_CONVERSIONS,coda_get_option_perform_conversions)(void)
{
    return coda_get_option_perform_conversions();
}

int UFNAME(CODA_SET_OPTION_USE_FAST_SIZE_EXPRESSIONS,coda_set_option_use_fast_size_expressions)(int *enable)
{
    return coda_set_option_use_fast_size_expressions(*enable);
}

int UFNAME(CODA_GET_OPTION_USE_FAST_SIZE_EXPRESSIONS,coda_get_option_use_fast_size_expressions)(void)
{
    return coda_get_option_use_fast_size_expressions();
}

int UFNAME(CODA_SET_OPTION_USE_MMAP,coda_set_option_use_mmap)(int *enable)
{
    return coda_set_option_use_mmap(*enable);
}

int UFNAME(CODA_GET_OPTION_USE_MMAP,coda_get_option_use_mmap)(void)
{
    return coda_get_option_use_mmap();
}

double UFNAME(CODA_NAN,coda_nan)(void)
{
    return coda_NaN();
}

int UFNAME(CODA_ISNAN,coda_isnan)(double *x)
{
    return coda_isNaN(*x);
}

double UFNAME(CODA_PLUSINF,coda_plusinf)(void)
{
    return coda_PlusInf();
}

double UFNAME(CODA_MININF,coda_mininf)(void)
{
    return coda_MinInf();
}

int UFNAME(CODA_ISINF,coda_isinf)(double *x)
{
    return coda_isInf(*x);
}

int UFNAME(CODA_ISPLUSINF,coda_isplusinf)(double *x)
{
    return coda_isPlusInf(*x);
}

int UFNAME(CODA_ISMININF,coda_ismininf)(double *x)
{
    return coda_isMinInf(*x);
}

int UFNAME(CODA_C_INDEX_TO_FORTRAN_INDEX,coda_c_index_to_fortran_index)(int *n_dims, long *dim, long *index)
{
    return coda_c_index_to_fortran_index(*n_dims, dim, *index);
}

double UFNAME(CODA_TIME,coda_time)(void)
{
    /* time() returns the amount of seconds since 1-JAN-1970.
     * We convert this to an amount of seconds since 1-JAN-2000 by substracting
     * 10957 days times 86400 seconds per day.
     */
    return time(NULL) - 10957 * 86400;
}

int UFNAME(CODA_DATETIME_TO_DOUBLE,coda_datetime_to_double)(int *YEAR, int *MONTH, int *DAY, int *HOUR, int *MINUTE, int *SECOND, int *MUSEC, double *datetime)
{
    return coda_datetime_to_double(*YEAR, *MONTH, *DAY, *HOUR, *MINUTE, *SECOND, *MUSEC, datetime);
}

int UFNAME(CODA_DOUBLE_TO_DATETIME,coda_double_to_datetime)(double *datetime, int *YEAR, int *MONTH, int *DAY, int *HOUR, int *MINUTE, int *SECOND, int *MUSEC)
{
    return coda_double_to_datetime(*datetime, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC);
}

int UFNAME(CODA_TIME_TO_STRING,coda_time_to_string)(double *time, char *str, int str_size)
{
    int result;
    INOUTSTR_BEGIN_DEF(str)
    if (str_size < 26)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "string argument should have at least 26 characters (%s:%u)",
                       __FILE__, __LINE__);
        return -1;
    }
    INOUTSTR_BEGIN_INIT(str)
    result = coda_time_to_string(*time, str_s);
    INOUTSTR_END(str)
    return result;
}

int UFNAME(CODA_STRING_TO_TIME,coda_string_to_time)(const char *str, double *time, int str_size)
{
    int result;
    INSTR_BEGIN(str)
    result = coda_string_to_time(str_s, time);
    INSTR_END(str)
    return result;
}

int UFNAME(CODA_UTCDATETIME_TO_DOUBLE,coda_utcdatetime_to_double)(int *YEAR, int *MONTH, int *DAY, int *HOUR, int *MINUTE, int *SECOND, int *MUSEC, double *datetime)
{
    return coda_utcdatetime_to_double(*YEAR, *MONTH, *DAY, *HOUR, *MINUTE, *SECOND, *MUSEC, datetime);
}

int UFNAME(CODA_DOUBLE_TO_UTCDATETIME,coda_double_to_utcdatetime)(double *datetime, int *YEAR, int *MONTH, int *DAY, int *HOUR, int *MINUTE, int *SECOND, int *MUSEC)
{
    return coda_double_to_utcdatetime(*datetime, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, MUSEC);
}

int UFNAME(CODA_TIME_TO_UTCSTRING,coda_time_to_utcstring)(double *time, char *str, int str_size)
{
    int result;
    INOUTSTR_BEGIN_DEF(str)
    if (str_size < 26)
    {
        coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "string argument should have at least 26 characters (%s:%u)",
                       __FILE__, __LINE__);
        return -1;
    }
    INOUTSTR_BEGIN_INIT(str)
    result = coda_time_to_utcstring(*time, str_s);
    INOUTSTR_END(str)
    return result;
}

int UFNAME(CODA_UTCSTRING_TO_TIME,coda_utcstring_to_time)(const char *str, double *time, int str_size)
{
    int result;
    INSTR_BEGIN(str)
    result = coda_utcstring_to_time(str_s, time);
    INSTR_END(str)
    return result;
}

int UFNAME(CODA_GET_ERRNO,coda_get_errno)(void)
{
    return coda_errno;
}

void UFNAME(CODA_ERRNO_TO_STRING,coda_errno_to_string)(int *err, char *str, int str_size)
{
    OUTSTR_BEGIN(str)
    str_s = coda_errno_to_string(*err);
    OUTSTR_END(str)
}

int UFNAME(CODA_RECOGNIZE_FILE,coda_recognize_file)(char *filename, int64_t *file_size, int *file_format, char *product_class, char *product_type, int *product_version, int filename_size, int product_class_size, int product_type_size)
{
    int result;
    INSTR_BEGIN_DEF(filename)
    OUTSTR_BEGIN(product_class)
    OUTSTR_BEGIN(product_type)
    INSTR_BEGIN_INIT(filename)
    result = coda_recognize_file(filename_s, file_size, (coda_format *)file_format, &product_class_s, &product_type_s, product_version);
    OUTSTR_END(product_type)
    OUTSTR_END(product_class)
    INSTR_END(filename)
    return result;
}

int UFNAME(CODA_OPEN,coda_open)(char *filename, void *pf, int filename_size)
{
    int result;
    INSTR_BEGIN(filename)
    result = coda_open(filename_s, (coda_product **)pf);
    INSTR_END(filename)
    return result;
}

int UFNAME(CODA_OPEN_AS,coda_open_as)(char *filename, char *product_class, char *product_type, int version, void *pf,
                                      int filename_size, int product_class_size, int product_type_size)
{
    int result;
    INSTR_BEGIN_DEF(filename)
    INSTR_BEGIN_DEF(product_class)
    INSTR_BEGIN_DEF(product_type)
    INSTR_BEGIN_INIT(filename)
    INSTR_BEGIN_INIT(product_class)
    INSTR_BEGIN_INIT(product_type)
    result = coda_open_as(filename_s, product_class_s, product_type_s, version, (coda_product **)pf);
    INSTR_END(product_type)
    INSTR_END(product_class)
    INSTR_END(filename)
    return result;
}

int UFNAME(CODA_CLOSE,coda_close)(void *pf)
{
    return coda_close(*(coda_product **)pf);
}

int UFNAME(CODA_GET_PRODUCT_FILENAME,coda_get_product_filename)(void *pf, char *filename, int filename_size)
{
    int result;
    OUTSTR_BEGIN(filename)
    result = coda_get_product_filename(*(coda_product **)pf, &filename_s);
    OUTSTR_END(filename)
    return result;
}

int UFNAME(CODA_GET_PRODUCT_FILE_SIZE,coda_get_product_file_size)(void *pf, int64_t *file_size)
{
    return coda_get_product_file_size(*(coda_product **)pf, file_size);
}

int UFNAME(CODA_GET_PRODUCT_FORMAT,coda_get_product_format)(void *pf, void *format)
{
    return coda_get_product_format(*(coda_product **)pf, (coda_format *)format);
}

int UFNAME(CODA_GET_PRODUCT_CLASS,coda_get_product_class)(void *pf, char *product_class, int product_class_size)
{
    int result;
    OUTSTR_BEGIN(product_class)
    result = coda_get_product_class(*(coda_product **)pf, &product_class_s);
    OUTSTR_END(product_class)
    return result;
}

int UFNAME(CODA_GET_PRODUCT_TYPE,coda_get_product_type)(void *pf, char *product_type, int product_type_size)
{
    int result;
    OUTSTR_BEGIN(product_type)
    result = coda_get_product_type(*(coda_product **)pf, &product_type_s);
    OUTSTR_END(product_type)
    return result;
}

int UFNAME(CODA_GET_PRODUCT_VERSION,coda_get_product_version)(void *pf, int *version)
{
    return coda_get_product_version(*(coda_product **)pf, version);
}

int UFNAME(CODA_GET_PRODUCT_DEFINITION_FILE,coda_get_product_definition_file)(void *pf, char *definition_file, int definition_file_size)
{
    int result;
    OUTSTR_BEGIN(definition_file)
    result = coda_get_product_class(*(coda_product **)pf, &definition_file_s);
    OUTSTR_END(definition_file)
    return result;
}

int UFNAME(CODA_GET_PRODUCT_ROOT_TYPE,coda_get_product_root_type)(void *pf, void *type)
{
    return coda_get_product_root_type(*(coda_product **)pf, (coda_type **)type);
}

int UFNAME(CODA_GET_PRODUCT_VARIABLE_VALUE,coda_get_product_variable_value)(void *pf, char *variable, int *index, int64_t *value, int variable_size)
{
    int result;
    INSTR_BEGIN(variable)
    result = coda_get_product_variable_value(*(coda_product **)pf, variable_s, *index, value);
    INSTR_END(variable)
    return result;
}

void UFNAME(CODA_TYPE_GET_FORMAT_NAME,coda_type_get_format_name)(int *format, char *format_name, int format_name_size)
{
    OUTSTR_BEGIN(format_name)
    format_name_s = coda_type_get_format_name(*format);
    OUTSTR_END(format_name)
}

void UFNAME(CODA_TYPE_GET_CLASS_NAME,coda_type_get_class_name)(int *type_class, char *class_name, int class_name_size)
{
    OUTSTR_BEGIN(class_name)
    class_name_s = coda_type_get_class_name(*type_class);
    OUTSTR_END(class_name)
}

void UFNAME(CODA_TYPE_GET_NATIVE_TYPE_NAME,coda_type_get_native_type_name)(int *native_type, char *native_type_name, int native_type_name_size)
{
    OUTSTR_BEGIN(native_type_name)
    native_type_name_s = coda_type_get_native_type_name(*native_type);
    OUTSTR_END(native_type_name)
}

void UFNAME(CODA_TYPE_GET_SPECIAL_TYPE_NAME,coda_type_get_special_type_name)(int *special_type, char *special_type_name, int special_type_name_size)
{
    OUTSTR_BEGIN(special_type_name)
    special_type_name_s = coda_type_get_special_type_name(*special_type);
    OUTSTR_END(special_type_name)
}

int UFNAME(CODA_TYPE_HAS_ATTRIBUTES,coda_type_has_attributes)(void *type, int *has_attributes)
{
    return coda_type_has_attributes(*(coda_type **)type, has_attributes);
}

int UFNAME(CODA_TYPE_GET_FORMAT,coda_type_get_format)(void *type, int *format)
{
    return coda_type_get_format(*(coda_type **)type, (coda_format *)format);
}

int UFNAME(CODA_TYPE_GET_CLASS,coda_type_get_class)(void *type, int *type_class)
{
    return coda_type_get_class(*(coda_type **)type, (coda_type_class *)type_class);
}

int UFNAME(CODA_TYPE_GET_READ_TYPE,coda_type_get_read_type)(void *type, int *read_type)
{
    return coda_type_get_read_type(*(coda_type **)type, (coda_native_type *)read_type);
}

int UFNAME(CODA_TYPE_GET_STRING_LENGTH,coda_type_get_string_length)(void *type, long *length)
{
    return coda_type_get_string_length(*(coda_type **)type, length);
}

int UFNAME(CODA_TYPE_GET_BIT_SIZE,coda_type_get_bit_size)(void *type, int64_t *bit_size)
{
    return coda_type_get_bit_size(*(coda_type **)type, bit_size);
}

int UFNAME(CODA_TYPE_GET_NAME,coda_type_get_name)(void *type, char *name, int name_size)
{
    int result;
    OUTSTR_BEGIN(name)
    result = coda_type_get_name(*(coda_type **)type, &name_s);
    OUTSTR_END(name)
    return result;
}

int UFNAME(CODA_TYPE_GET_DESCRIPTION,coda_type_get_description)(void *type, char *description, int description_size)
{
    int result;
    OUTSTR_BEGIN(description)
    result = coda_type_get_description(*(coda_type **)type, &description_s);
    OUTSTR_END(description)
    return result;
}

int UFNAME(CODA_TYPE_GET_UNIT,coda_type_get_unit)(void *type, char *unit, int unit_size)
{
    int result;
    OUTSTR_BEGIN(unit)
    result = coda_type_get_unit(*(coda_type **)type, &unit_s);
    OUTSTR_END(unit)
    return result;
}

int UFNAME(CODA_TYPE_GET_FIXED_VALUE,coda_type_get_fixed_value)(void *type, char *fixed_value, long *length,
                                                                int fixed_value_size)
{
    int result;
    OUTSTR_BEGIN(fixed_value)
    result = coda_type_get_fixed_value(*(coda_type **)type, &fixed_value_s, length);
    OUTSTR_END(fixed_value)
    return result;
}

int UFNAME(CODA_TYPE_GET_ATTRIBUTES,coda_type_get_attributes)(void *type, void *attributes)
{
    return coda_type_get_attributes(*(coda_type **)type, (coda_type **)attributes);
}

int UFNAME(CODA_TYPE_GET_NUM_RECORD_FIELDS,coda_type_get_num_record_fields)(void *type, long *n_fields)
{
    return coda_type_get_num_record_fields(*(coda_type **)type, n_fields);
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_INDEX_FROM_NAME,coda_type_get_record_field_index_from_name)(void *type, char *name, long *index, int name_size)
{
    int result;
    INSTR_BEGIN(name)
    result = coda_type_get_record_field_index_from_name(*(coda_type **)type, name_s, index);
    INSTR_END(name)
    return result;
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_INDEX_FROM_REAL_NAME,coda_type_get_record_field_index_from_real_name)(void *type, char *real_name, long *index, int real_name_size)
{
    int result;
    INSTR_BEGIN(real_name)
    result = coda_type_get_record_field_index_from_real_name(*(coda_type **)type, real_name_s, index);
    INSTR_END(real_name)
    return result;
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_TYPE,coda_type_get_record_field_type)(void *type, long *index, void *field_type)
{
    return coda_type_get_record_field_type(*(coda_type **)type, *index, (coda_type **)field_type);
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_NAME,coda_type_get_record_field_name)(void *type, long *index, char *name, int name_size)
{
    int result;
    OUTSTR_BEGIN(name)
    result = coda_type_get_record_field_name(*(coda_type **)type, *index, &name_s);
    OUTSTR_END(name)
    return result;
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_REAL_NAME,coda_type_get_record_field_real_name)(void *type, long *index, char *real_name, int real_name_size)
{
    int result;
    OUTSTR_BEGIN(real_name)
    result = coda_type_get_record_field_real_name(*(coda_type **)type, *index, &real_name_s);
    OUTSTR_END(real_name)
    return result;
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_HIDDEN_STATUS,coda_type_get_record_field_hidden_status)(void *type, long *index, int *hidden)
{
    return coda_type_get_record_field_hidden_status(*(coda_type **)type, *index, hidden);
}

int UFNAME(CODA_TYPE_GET_RECORD_FIELD_AVAILABLE_STATUS,coda_type_get_record_field_available_status)(void *type, long *index, int *available)
{
    return coda_type_get_record_field_available_status(*(coda_type **)type, *index, available);
}

int UFNAME(CODA_TYPE_GET_RECORD_UNION_STATUS,coda_type_get_record_union_status)(void *type, int *is_union)
{
    return coda_type_get_record_union_status(*(coda_type **)type, is_union);
}

int UFNAME(CODA_TYPE_GET_ARRAY_NUM_DIMS,coda_type_get_array_num_dims)(void *type, int *num_dims)
{
    return coda_type_get_array_num_dims(*(coda_type **)type, num_dims);
}

int UFNAME(CODA_TYPE_GET_ARRAY_DIM,coda_type_get_array_dim)(void *type, int *num_dims, long *dim)
{
    return coda_type_get_array_dim(*(coda_type **)type, num_dims, dim);    
}

int UFNAME(CODA_TYPE_GET_ARRAY_BASE_TYPE,coda_type_get_array_base_type)(void *type, void *base_type)
{
    return coda_type_get_array_base_type(*(coda_type **)type, (coda_type **)base_type);
}

int UFNAME(CODA_TYPE_GET_SPECIAL_TYPE,coda_type_get_special_type)(void *type, int *special_type)
{
    return coda_type_get_special_type(*(coda_type **)type, (coda_special_type *)special_type);
}

int UFNAME(CODA_TYPE_GET_SPECIAL_BASE_TYPE,coda_type_get_special_base_type)(void *type, void *base_type)
{
    return coda_type_get_special_base_type(*(coda_type **)type, (coda_type **)base_type);
}

void *UFNAME(CODA_CURSOR_NEW,coda_cursor_new)()
{
    return malloc(sizeof(coda_cursor));
}

void *UFNAME(CODA_CURSOR_DUPLICATE,coda_cursor_duplicate)(void *cursor)
{
    coda_cursor *new_cursor;

    new_cursor = malloc(sizeof(coda_cursor));
    memcpy(new_cursor, *(coda_cursor **)cursor, sizeof(coda_cursor));

    return new_cursor;
}

void UFNAME(CODA_CURSOR_DELETE,coda_cursor_delete)(void *cursor)
{
    free(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_SET_PRODUCT,coda_cursor_set_product)(void *cursor, void *pf)
{
    return coda_cursor_set_product(*(coda_cursor **)cursor, *(coda_product **)pf);
}

int UFNAME(CODA_CURSOR_GOTO,coda_cursor_goto)(void *cursor, char *path, int path_size)
{
    int result;
    INSTR_BEGIN(path)
    result = coda_cursor_goto(*(coda_cursor **)cursor, path_s);
    INSTR_END(path)
    return result;
}

int UFNAME(CODA_CURSOR_GOTO_FIRST_RECORD_FIELD,coda_cursor_goto_first_record_field)(void *cursor)
{
    return coda_cursor_goto_first_record_field(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_NEXT_RECORD_FIELD,coda_cursor_goto_next_record_field)(void *cursor)
{
    return coda_cursor_goto_next_record_field(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_RECORD_FIELD_BY_INDEX,coda_cursor_goto_record_field_by_index)(void *cursor, long *index)
{
    return coda_cursor_goto_record_field_by_index(*(coda_cursor **)cursor, *index);
}

int UFNAME(CODA_CURSOR_GOTO_RECORD_FIELD_BY_NAME,coda_cursor_goto_record_field_by_name)(void *cursor, char *name, int name_size)
{
    int result;
    INSTR_BEGIN(name)
    result = coda_cursor_goto_record_field_by_name(*(coda_cursor **)cursor, name_s);
    INSTR_END(name)
    return result;
}

int UFNAME(CODA_CURSOR_GOTO_AVAILABLE_UNION_FIELD,coda_cursor_goto_available_union_field)(void *cursor)
{
    return coda_cursor_goto_available_union_field(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_FIRST_ARRAY_ELEMENT,coda_cursor_goto_first_array_element)(void *cursor)
{
    return coda_cursor_goto_first_array_element(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_NEXT_ARRAY_ELEMENT,coda_cursor_goto_next_array_element)(void *cursor)
{
    return coda_cursor_goto_next_array_element(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_ARRAY_ELEMENT,coda_cursor_goto_array_element)(void *cursor, int *n_subs, long *subs)
{
    return coda_cursor_goto_array_element(*(coda_cursor **)cursor, *n_subs, subs);
}

int UFNAME(CODA_CURSOR_GOTO_ARRAY_ELEMENT_BY_INDEX,coda_cursor_goto_array_element_by_index)(void *cursor, long *index)
{
    return coda_cursor_goto_array_element_by_index(*(coda_cursor **)cursor, *index);
}

int UFNAME(CODA_CURSOR_GOTO_ATTRIBUTES,coda_cursor_goto_attributes)(void *cursor)
{
    return coda_cursor_goto_attributes(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_ROOT,coda_cursor_goto_root)(void *cursor)
{
    return coda_cursor_goto_root(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_GOTO_PARENT,coda_cursor_goto_parent)(void *cursor)
{
    return coda_cursor_goto_parent(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_USE_BASE_TYPE_OF_SPECIAL_TYPE,coda_cursor_use_base_type_of_special_type)(void *cursor)
{
    return coda_cursor_use_base_type_of_special_type(*(coda_cursor **)cursor);
}

int UFNAME(CODA_CURSOR_HAS_ASCII_CONTENT,coda_cursor_has_ascii_content)(void *cursor, int *has_ascii_content)
{
    return coda_cursor_has_ascii_content(*(coda_cursor **)cursor, has_ascii_content);
}

int UFNAME(CODA_CURSOR_HAS_ATTRIBUTES,coda_cursor_has_attributes)(void *cursor, int *has_attributes)
{
    return coda_cursor_has_attributes(*(coda_cursor **)cursor, has_attributes);
}

int UFNAME(CODA_CURSOR_GET_STRING_LENGTH,coda_cursor_get_string_length)(void *cursor, long *length)
{
    return coda_cursor_get_string_length(*(coda_cursor **)cursor, length);
}

int UFNAME(CODA_CURSOR_GET_BIT_SIZE,coda_cursor_get_bit_size)(void *cursor, int64_t *bit_size)
{
    return coda_cursor_get_bit_size(*(coda_cursor **)cursor, bit_size);
}

int UFNAME(CODA_CURSOR_GET_BYTE_SIZE,coda_cursor_get_byte_size)(void *cursor, int64_t *byte_size)
{
    return coda_cursor_get_byte_size(*(coda_cursor **)cursor, byte_size);
}

int UFNAME(CODA_CURSOR_GET_NUM_ELEMENTS,coda_cursor_get_num_elements)(void *cursor, long *num_elements)
{
    return coda_cursor_get_num_elements(*(coda_cursor **)cursor, num_elements);
}

int UFNAME(CODA_CURSOR_GET_PRODUCT_FILE,coda_cursor_get_product_file)(void *cursor, void *pf)
{
    return coda_cursor_get_product_file(*(coda_cursor **)cursor, (coda_product **)pf);
}

int UFNAME(CODA_CURSOR_GET_DEPTH,coda_cursor_get_depth)(void *cursor, int *depth)
{
    return coda_cursor_get_depth(*(coda_cursor **)cursor, depth);
}

int UFNAME(CODA_CURSOR_GET_INDEX,coda_cursor_get_index)(void *cursor, long *index)
{
    return coda_cursor_get_index(*(coda_cursor **)cursor, index);
}

int UFNAME(CODA_CURSOR_GET_FILE_BIT_OFFSET,coda_cursor_get_file_bit_offset)(void *cursor, int64_t *bit_offset)
{
    return coda_cursor_get_file_bit_offset(*(coda_cursor **)cursor, bit_offset);
}

int UFNAME(CODA_CURSOR_GET_FILE_BYTE_OFFSET,coda_cursor_get_file_byte_offset)(void *cursor, int64_t *byte_offset)
{
    return coda_cursor_get_file_byte_offset(*(coda_cursor **)cursor, byte_offset);
}

int UFNAME(CODA_CURSOR_GET_FORMAT,coda_cursor_get_format)(void *cursor, int *format)
{
    return coda_cursor_get_format(*(coda_cursor **)cursor, (coda_format *)format);
}

int UFNAME(CODA_CURSOR_GET_TYPE_CLASS,coda_cursor_get_type_class)(void *cursor, int *type_class)
{
    return coda_cursor_get_type_class(*(coda_cursor **)cursor, (coda_type_class *)type_class);
}

int UFNAME(CODA_CURSOR_GET_READ_TYPE,coda_cursor_get_read_type)(void *cursor, int *read_type)
{
    return coda_cursor_get_read_type(*(coda_cursor **)cursor, (coda_native_type *)read_type);
}

int UFNAME(CODA_CURSOR_GET_SPECIAL_TYPE,coda_cursor_get_special_type)(void *cursor, int *special_type)
{
    return coda_cursor_get_special_type(*(coda_cursor **)cursor, (coda_special_type *)special_type);
}

int UFNAME(CODA_CURSOR_GET_TYPE,coda_cursor_get_type)(void *cursor, void *type)
{
    return coda_cursor_get_type(*(coda_cursor **)cursor, (coda_type **)type);
}

int UFNAME(CODA_CURSOR_GET_RECORD_FIELD_INDEX_FROM_NAME,coda_cursor_get_record_field_index_from_name)(void *cursor, char *name, long *index, int name_size)
{
    int result;
    INSTR_BEGIN(name)
    result = coda_cursor_get_record_field_index_from_name(*(coda_cursor **)cursor, name_s, index);
    INSTR_END(name)
    return result;
}

int UFNAME(CODA_CURSOR_GET_RECORD_FIELD_AVAILABLE_STATUS,coda_cursor_get_record_field_available_status)(void *cursor, long *index, int *available)
{
    return coda_cursor_get_record_field_available_status(*(coda_cursor **)cursor, *index, available);
}

int UFNAME(CODA_CURSOR_GET_AVAILABLE_UNION_FIELD_INDEX,coda_cursor_get_available_union_field_index)(void *cursor, long *index)
{
    return coda_cursor_get_available_union_field_index(*(coda_cursor **)cursor, index);
}

int UFNAME(CODA_CURSOR_GET_ARRAY_DIM,coda_cursor_get_array_dim)(void *cursor, int *num_dims, long *dim)
{
    return coda_cursor_get_array_dim(*(coda_cursor **)cursor, num_dims, dim);
}

int UFNAME(CODA_CURSOR_READ_INT8,coda_cursor_read_int8)(void *cursor, int8_t *dst)
{
    return coda_cursor_read_int8(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_UINT8,coda_cursor_read_uint8)(void *cursor, uint8_t *dst)
{
    return coda_cursor_read_uint8(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_INT16,coda_cursor_read_int16)(void *cursor, int16_t *dst)
{
    return coda_cursor_read_int16(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_UINT16,coda_cursor_read_uint16)(void *cursor, uint16_t *dst)
{
    return coda_cursor_read_uint16(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_INT32,coda_cursor_read_int32)(void *cursor, int32_t *dst)
{
    return coda_cursor_read_int32(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_UINT32,coda_cursor_read_uint32)(void *cursor, uint32_t *dst)
{
    return coda_cursor_read_uint32(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_INT64,coda_cursor_read_int64)(void *cursor, int64_t *dst)
{
    return coda_cursor_read_int64(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_UINT64,coda_cursor_read_uint64)(void *cursor, uint64_t *dst)
{
    return coda_cursor_read_uint64(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_FLOAT,coda_cursor_read_float)(void *cursor, float *dst)
{
    return coda_cursor_read_float(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_DOUBLE,coda_cursor_read_double)(void *cursor, double *dst)
{
    return coda_cursor_read_double(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_CHAR,coda_cursor_read_char)(void *cursor, char *dst)
{
    return coda_cursor_read_char(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_STRING,coda_cursor_read_string)(void *cursor, char *dst, int dst_size)
{
    int result;
    INOUTSTR_BEGIN(dst)
    result = coda_cursor_read_string(*(coda_cursor **)cursor, dst_s, dst_size + 1);
    INOUTSTR_END(dst)
    return result;
}

int UFNAME(CODA_CURSOR_READ_BITS,coda_cursor_read_bits)(void *cursor, int8_t *dst, int64_t *bit_offset,
                                                        int64_t *bit_length)
{
    return coda_cursor_read_bits(*(coda_cursor **)cursor, (uint8_t *)dst, *bit_offset, *bit_length);
}

int UFNAME(CODA_CURSOR_READ_BYTES,coda_cursor_read_bytes)(void *cursor, int8_t *dst, int64_t *offset, int64_t *length)
{
    return coda_cursor_read_bytes(*(coda_cursor **)cursor, (uint8_t *)dst, *offset, *length);
}

int UFNAME(CODA_CURSOR_READ_INT8_ARRAY,coda_cursor_read_int8_array)(void *cursor, int8_t *dst, int *array_ordering)
{
    return coda_cursor_read_int8_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_UINT8_ARRAY,coda_cursor_read_uint8_array)(void *cursor, uint8_t *dst, int *array_ordering)
{
    return coda_cursor_read_uint8_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_INT16_ARRAY,coda_cursor_read_int16_array)(void *cursor, int16_t *dst, int *array_ordering)
{
    return coda_cursor_read_int16_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_UINT16_ARRAY,coda_cursor_read_uint16_array)(void *cursor, uint16_t *dst, int *array_ordering)
{
    return coda_cursor_read_uint16_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_INT32_ARRAY,coda_cursor_read_int32_array)(void *cursor, int32_t *dst, int *array_ordering)
{
    return coda_cursor_read_int32_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_UINT32_ARRAY,coda_cursor_read_uint32_array)(void *cursor, uint32_t *dst, int *array_ordering)
{
    return coda_cursor_read_uint32_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_INT64_ARRAY,coda_cursor_read_int64_array)(void *cursor, int64_t *dst, int *array_ordering)
{
    return coda_cursor_read_int64_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_UINT64_ARRAY,coda_cursor_read_uint64_array)(void *cursor, uint64_t *dst, int *array_ordering)
{
    return coda_cursor_read_uint64_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_FLOAT_ARRAY,coda_cursor_read_float_array)(void *cursor, float *dst, int *array_ordering)
{
    return coda_cursor_read_float_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_DOUBLE_ARRAY,coda_cursor_read_double_array)(void *cursor, double *dst, int *array_ordering)
{
    return coda_cursor_read_double_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_CHAR_ARRAY,coda_cursor_read_char_array)(void *cursor, char *dst, int *array_ordering)
{
    return coda_cursor_read_char_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_COMPLEX_DOUBLE_PAIR,coda_cursor_read_complex_double_pair)(void *cursor, double *dst)
{
    return coda_cursor_read_complex_double_pair(*(coda_cursor **)cursor, dst);
}

int UFNAME(CODA_CURSOR_READ_COMPLEX_DOUBLE_PAIRS_ARRAY,coda_cursor_read_complex_double_pairs_array)(void *cursor, double *dst, int *array_ordering)
{
    return coda_cursor_read_complex_double_pairs_array(*(coda_cursor **)cursor, dst, (coda_array_ordering)*array_ordering);
}

int UFNAME(CODA_CURSOR_READ_COMPLEX_DOUBLE_SPLIT,coda_cursor_read_complex_double_split)(void *cursor, double *dst_re, double *dst_im)
{
    return coda_cursor_read_complex_double_split(*(coda_cursor **)cursor, dst_re, dst_im);
}

int UFNAME(CODA_CURSOR_READ_COMPLEX_DOUBLE_SPLIT_ARRAY,coda_cursor_read_complex_double_split_array)(void *cursor, double *dst_re, double *dst_im, int *array_ordering)
{
    return coda_cursor_read_complex_double_split_array(*(coda_cursor **)cursor, dst_re, dst_im, (coda_array_ordering)*array_ordering);
}

void UFNAME(CODA_EXPRESSION_GET_TYPE_NAME,coda_expression_get_type_name)(int *expression_type, char *expression_type_name, int expression_type_name_size)
{
    OUTSTR_BEGIN(expression_type_name)
    expression_type_name_s = coda_type_get_native_type_name(*expression_type);
    OUTSTR_END(expression_type_name)
}

int UFNAME(CODA_EXPRESSION_FROM_STRING,coda_expression_from_string)(char *expression_string, void *expression, int expression_string_size)
{
    int result;
    INSTR_BEGIN(expression_string)
    result = coda_expression_from_string(expression_string_s, (coda_expression **)expression);
    INSTR_END(expression_string)
    return result;
}

void UFNAME(CODA_EXPRESSION_DELETE,coda_expression_delete)(void *expression)
{
    coda_expression_delete(*(coda_expression **)expression);
}

int UFNAME(CODA_EXPRESSION_GET_TYPE,coda_expression_get_type)(void *expression, int *expression_type)
{
    return coda_expression_get_type(*(coda_expression **)expression, (coda_expression_type *)expression_type);
}

int UFNAME(CODA_EXPRESSION_IS_CONSTANT,coda_expression_is_constant)(void *expression)
{
    return coda_expression_is_constant(*(coda_expression **)expression);
}

int UFNAME(CODA_EXPRESSION_EVAL_BOOL,coda_expression_eval_bool)(void *expression, void *cursor, int *value)
{
    return coda_expression_eval_bool(*(coda_expression **)expression, *(coda_cursor **)cursor, value);
}

int UFNAME(CODA_EXPRESSION_EVAL_INTEGER,coda_expression_eval_integer)(void *expression, void *cursor, int64_t *value)
{
    return coda_expression_eval_integer(*(coda_expression **)expression, *(coda_cursor **)cursor, value);
}

int UFNAME(CODA_EXPRESSION_EVAL_FLOAT,coda_expression_eval_float)(void *expression, void *cursor, double *value)
{
    return coda_expression_eval_float(*(coda_expression **)expression, *(coda_cursor **)cursor, value);
}

int UFNAME(CODA_EXPRESSION_EVAL_STRING,coda_expression_eval_string)(void *expression, void *cursor, char *value, int value_size)
{
    char *value_s = NULL;
    long value_l;
    int result;

    result = coda_expression_eval_string(*(coda_expression **)expression, *(coda_cursor **)cursor, &value_s, &value_l);
    if (value_s != NULL)
    {
        if (value_l > value_size)
        {
            memcpy(value, value_s, value_size);
        }
        else
        {
            memcpy(value, value_s, value_l);
            while (value_l < value_size)
            {
                value[value_l] = ' ';
                value_l++;
            }
        }
        coda_free(value_s);
    }
    else
    {
        memset(value, ' ', value_size);
    }
    return result;
}

int UFNAME(CODA_EXPRESSION_EVAL_NODE,coda_expression_eval_node)(void *expression, void *cursor)
{
    return coda_expression_eval_node(*(coda_expression **)expression, *(coda_cursor **)cursor);
}

