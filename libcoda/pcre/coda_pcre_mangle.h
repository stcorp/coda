#ifndef CODA_PCRE_MANGLE_H
#define CODA_PCRE_MANGLE_H

/*
 * This header file mangles symbols exported from the PCRE library.
 * This is needed on some platforms because of nameresolving conflicts when
 * CODA is used as a module in an application that has its own version of PCRE.
 * Even though name mangling is not needed for every platform or CODA
 * interface, we always perform the mangling for consitency reasons.
 */

#define _pcre_find_bracket coda__pcre_find_bracket
#define _pcre_is_newline coda__pcre_is_newline
#define _pcre_ord2utf coda__pcre_ord2utf
#define _pcre_try_flipped coda__pcre_try_flipped
#define _pcre_valid_utf coda__pcre_valid_utf
#define _pcre_was_newline coda__pcre_was_newline
#define _pcre_xclass coda__pcre_xclass
#define pcre_callout coda_pcre_callout
#define pcre_compile coda_pcre_compile
#define pcre_compile2 coda_pcre_compile2
#define pcre_config coda_pcre_config
#define pcre_copy_named_substring coda_pcre_copy_named_substring
#define pcre_copy_substring coda_pcre_copy_substring
#define pcre_dfa_exec coda_pcre_dfa_exec
#define pcre_exec coda_pcre_exec
#define pcre_free coda_pcre_free
#define pcre_free_study coda_pcre_free_study
#define pcre_free_substring coda_pcre_free_substring
#define pcre_free_substring_list coda_pcre_free_substring_list
#define pcre_fullinfo coda_pcre_fullinfo
#define pcre_get_named_substring coda_pcre_get_named_substring
#define pcre_get_stringnumber coda_pcre_get_stringnumber
#define pcre_get_stringtable_entries coda_pcre_get_stringtable_entries
#define pcre_get_substring coda_pcre_get_substring
#define pcre_get_substring_list coda_pcre_get_substring_list
#define pcre_info coda_pcre_info
#define pcre_maketables coda_pcre_maketables
#define pcre_malloc coda_pcre_malloc
#define pcre_pattern_to_host_byte_order coda_pcre_pattern_to_host_byte_order
#define pcre_printint coda_pcre_printint
#define pcre_refcount coda_pcre_refcount
#define pcre_stack_free coda_pcre_stack_free
#define pcre_stack_malloc coda_pcre_stack_malloc
#define pcre_study coda_pcre_study
#define pcre_version coda_pcre_version

#endif
