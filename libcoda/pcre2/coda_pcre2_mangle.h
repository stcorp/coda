#ifndef CODA_PCRE2_MANGLE_H
#define CODA_PCRE2_MANGLE_H

/*
 * This header file mangles symbols exported from the PCRE library.
 * This is needed on some platforms because of nameresolving conflicts when
 * CODA is used as a module in an application that has its own version of PCRE2.
 * Even though name mangling is not needed for every platform or CODA
 * interface, we always perform the mangling for consitency reasons.
 */

#define _pcre2_auto_possessify_8 coda__pcre2_auto_possessify_8
#define _pcre2_callout_end_delims_8 coda__pcre2_callout_end_delims_8
#define _pcre2_callout_start_delims_8 coda__pcre2_callout_start_delims_8
#define _pcre2_check_escape_8 coda__pcre2_check_escape_8
#define _pcre2_ckd_smul coda__pcre2_ckd_smul
#define _pcre2_default_tables_8 coda__pcre2_default_tables_8
#define _pcre2_default_compile_context_8 coda__pcre2_default_compile_context_8
#define _pcre2_default_convert_context_8 coda__pcre2_default_convert_context_8
#define _pcre2_default_match_context_8 coda__pcre2_default_match_context_8
#define _pcre2_extuni_8 coda__pcre2_extuni_8
#define _pcre2_find_bracket_8 coda__pcre2_find_bracket_8
#define _pcre2_hspace_list_8 coda__pcre2_hspace_list_8
#define _pcre2_is_newline_8 coda__pcre2_is_newline_8
#define _pcre2_memctl_malloc_8 coda__pcre2_memctl_malloc_8
#define _pcre2_ord2utf_8 coda__pcre2_ord2utf_8
#define _pcre2_OP_lengths_8 coda__pcre2_OP_lengths_8
#define _pcre2_script_run_8 coda__pcre2_script_run_8
#define _pcre2_strcmp_8 coda__pcre2_strcmp_8
#define _pcre2_strcmp_c8_8 coda__pcre2_strcmp_c8_8
#define _pcre2_strcpy_c8_8 coda__pcre2_strcpy_c8_8
#define _pcre2_strlen_8 coda__pcre2_strlen_8
#define _pcre2_strncmp_8 coda__pcre2_strncmp_8
#define _pcre2_strncmp_c8_8 coda__pcre2_strncmp_c8_8
#define _pcre2_study_8 coda__pcre2_study_8
#define _pcre2_ucd_caseless_sets_8 coda__pcre2_ucd_caseless_sets_8
#define _pcre2_ucd_records_8 coda__pcre2_ucd_records_8
#define _pcre2_ucd_stage1_8 coda__pcre2_ucd_stage1_8
#define _pcre2_ucd_stage2_8 coda__pcre2_ucd_stage2_8
#define _pcre2_valid_utf_8 coda__pcre2_valid_utf_8
#define _pcre2_vspace_list_8 coda__pcre2_vspace_list_8
#define _pcre2_was_newline_8 coda__pcre2_was_newline_8
#define _pcre2_xclass_8 coda__pcre2_xclass_8
#define pcre2_callout_enumerate_8 coda_pcre2_callout_enumerate_8
#define pcre2_code_copy_8 coda_pcre2_code_copy_8
#define pcre2_code_copy_with_tables_8 coda_pcre2_code_copy_with_tables_8
#define pcre2_code_free_8 coda_pcre2_code_free_8
#define pcre2_compile_8 coda_pcre2_compile_8
#define pcre2_compile_context_copy_8 coda_pcre2_compile_context_copy_8
#define pcre2_compile_context_create_8 coda_pcre2_compile_context_create_8
#define pcre2_compile_context_free_8 coda_pcre2_compile_context_free_8
#define pcre2_config_8 coda_pcre2_config_8
#define pcre2_convert_context_copy_8 coda_pcre2_convert_context_copy_8
#define pcre2_convert_context_create_8 coda_pcre2_convert_context_create_8
#define pcre2_convert_context_free_8 coda_pcre2_convert_context_free_8
#define pcre2_dfa_match_8 coda_pcre2_dfa_match_8
#define pcre2_general_context_copy_8 coda_pcre2_general_context_copy_8
#define pcre2_general_context_create_8 coda_pcre2_general_context_create_8
#define pcre2_general_context_free_8 coda_pcre2_general_context_free_8
#define pcre2_get_error_message_8 coda_pcre2_get_error_message_8
#define pcre2_get_mark_8 coda_pcre2_get_mark_8
#define pcre2_get_match_data_size_8 coda_pcre2_get_match_data_size_8
#define pcre2_get_ovector_count_8 coda_pcre2_get_ovector_count_8
#define pcre2_get_ovector_pointer_8 coda_pcre2_get_ovector_pointer_8
#define pcre2_get_startchar_8 coda_pcre2_get_startchar_8
#define pcre2_maketables_8 coda_pcre2_maketables_8
#define pcre2_maketables_free_8 coda_pcre2_maketables_free_8
#define pcre2_match_8 coda_pcre2_match_8
#define pcre2_match_context_copy_8 coda_pcre2_match_context_copy_8
#define pcre2_match_context_create_8 coda_pcre2_match_context_create_8
#define pcre2_match_context_free_8 coda_pcre2_match_context_free_8
#define pcre2_match_data_create_8 coda_pcre2_match_data_create_8
#define pcre2_match_data_create_from_pattern_8 coda_pcre2_match_data_create_from_pattern_8
#define pcre2_match_data_free_8 coda_pcre2_match_data_free_8
#define pcre2_pattern_info_8 coda_pcre2_pattern_info_8
#define pcre2_set_bsr_8 coda_pcre2_set_bsr_8
#define pcre2_set_callout_8 coda_pcre2_set_callout_8
#define pcre2_set_character_tables_8 coda_pcre2_set_character_tables_8
#define pcre2_set_compile_extra_options_8 coda_pcre2_set_compile_extra_options_8
#define pcre2_set_compile_recursion_guard_8 coda_pcre2_set_compile_recursion_guard_8
#define pcre2_set_depth_limit_8 coda_pcre2_set_depth_limit_8
#define pcre2_set_glob_escape_8 coda_pcre2_set_glob_escape_8
#define pcre2_set_glob_separator_8 coda_pcre2_set_glob_separator_8
#define pcre2_set_heap_limit_8 coda_pcre2_set_heap_limit_8
#define pcre2_get_match_data_heapframes_size_8 coda_pcre2_get_match_data_heapframes_size_8
#define pcre2_set_match_limit_8 coda_pcre2_set_match_limit_8
#define pcre2_set_max_pattern_compiled_length_8 coda_pcre2_set_max_pattern_compiled_length_8
#define pcre2_set_max_pattern_length_8 coda_pcre2_set_max_pattern_length_8
#define pcre2_set_max_varlookbehind_8 coda_pcre2_set_max_varlookbehind_8
#define pcre2_set_newline_8 coda_pcre2_set_newline_8
#define pcre2_set_offset_limit_8 coda_pcre2_set_offset_limit_8
#define pcre2_set_parens_nest_limit_8 coda_pcre2_set_parens_nest_limit_8
#define pcre2_set_recursion_limit_8 coda_pcre2_set_recursion_limit_8
#define pcre2_set_recursion_memory_management_8 coda_pcre2_set_recursion_memory_management_8
#define pcre2_set_substitute_callout_8 coda_pcre2_set_substitute_callout_8
#define pcre2_substitute_8 coda_pcre2_substitute_8
#define pcre2_substring_copy_byname_8 coda_pcre2_substring_copy_byname_8
#define pcre2_substring_copy_bynumber_8 coda_pcre2_substring_copy_bynumber_8
#define pcre2_substring_free_8 coda_pcre2_substring_free_8
#define pcre2_substring_get_byname_8 coda_pcre2_substring_get_byname_8
#define pcre2_substring_get_bynumber_8 coda_pcre2_substring_get_bynumber_8
#define pcre2_substring_length_byname_8 coda_pcre2_substring_length_byname_8
#define pcre2_substring_length_bynumber_8 coda_pcre2_substring_length_bynumber_8
#define pcre2_substring_list_free_8 coda_pcre2_substring_list_free_8
#define pcre2_substring_list_get_8 coda_pcre2_substring_list_get_8
#define pcre2_substring_nametable_scan_8 coda_pcre2_substring_nametable_scan_8
#define pcre2_substring_number_from_name_8 coda_pcre2_substring_number_from_name_8

#endif
