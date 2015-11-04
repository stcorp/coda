/*
 * Copyright (C) 2009-2011 S[&]T, The Netherlands.
 *
 * This file is part of the QIAP Toolkit.
 *
 * The QIAP Toolkit is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The QIAP Toolkit is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the QIAP Toolkit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef QIAP_H
#define QIAP_H

/** \file */

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C"
{
#endif
/* *INDENT-ON* */

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) && defined(LIBQIAPDLL)
#ifdef LIBQIAPDLL_EXPORTS
#define LIBQIAP_API __declspec(dllexport)
#else
#define LIBQIAP_API __declspec(dllimport)
#endif
#else
#define LIBQIAP_API
#endif

LIBQIAP_API extern const char *libqiap_version;

LIBQIAP_API extern int qiap_errno;

LIBQIAP_API extern int qiap_option_debug;

#define QIAP_SUCCESS                                          (0)
#define QIAP_ERROR_OUT_OF_MEMORY                             (-1)
#define QIAP_ERROR_CODA                                     (-10)
#define QIAP_ERROR_XML                                      (-11)
#define QIAP_ERROR_SERVER                                   (-20)
#define QIAP_ERROR_FILE_NOT_FOUND                           (-30)
#define QIAP_ERROR_FILE_OPEN                                (-31)
#define QIAP_ERROR_FILE_READ                                (-32)
#define QIAP_ERROR_FILE_WRITE                               (-33)
#define QIAP_ERROR_INVALID_ARGUMENT                         (-40)
#define QIAP_ERROR_DISCARD                                  (-50)

enum qiap_action_type_enum
{
    qiap_action_discard_product,
    qiap_action_discard_value,
    qiap_action_correct_value,
    qiap_action_custom_correction
};

struct qiap_query_struct
{
    int num_entries;
    char **mission;
    char **product_type;
};

struct qiap_algorithm_struct
{
    char *name;
    char *reference;
    int num_parameters;
    char **parameter_key;
    char **parameter_value;
};

struct qiap_action_struct
{
    char *last_modification_date;
    enum qiap_action_type_enum action_type;
    long order;
    char *correction_string;
    struct coda_expression_struct *correction;
    struct qiap_algorithm_struct *algorithm;
};

struct qiap_affected_value_struct
{
    long affected_value_id;
    char *parameter;
    char *extent_string;
    struct coda_expression_struct *extent;
    int num_parameter_values;
    char **parameter_value_path;
    int num_actions;
    struct qiap_action_struct **action;
};

struct qiap_affected_product_struct
{
    long affected_product_id;
    char *product_type;
    char *extent_string;
    struct coda_expression_struct *extent;
    int num_products;
    char **product;
    int num_affected_values;
    struct qiap_affected_value_struct **affected_value;
    int num_actions;
    struct qiap_action_struct **action;
};

struct qiap_quality_issue_struct
{
    long issue_id;
    char *last_modification_date;
    char *mission;
    char *title;
    char *description;
    char *instrument;
    char *cause;
    char *resolution;
    int num_affected_products;
    struct qiap_affected_product_struct **affected_product;
};

struct qiap_quality_issue_report_struct
{
    char *organisation;
    int num_quality_issues;
    struct qiap_quality_issue_struct **quality_issue;
};

typedef enum qiap_action_type_enum qiap_action_type;
typedef struct qiap_query_struct qiap_query;
typedef struct qiap_algorithm_struct qiap_algorithm;
typedef struct qiap_action_struct qiap_action;
typedef struct qiap_affected_value_struct qiap_affected_value;
typedef struct qiap_affected_product_struct qiap_affected_product;
typedef struct qiap_quality_issue_struct qiap_quality_issue;
typedef struct qiap_quality_issue_report_struct qiap_quality_issue_report;

LIBQIAP_API void qiap_set_error(int err, const char *message, ...);
LIBQIAP_API void qiap_add_error_message(const char *message, ...);
LIBQIAP_API const char *qiap_errno_to_string(int err);

LIBQIAP_API const char *qiap_get_action_type_name(qiap_action_type action_type);

LIBQIAP_API qiap_query *qiap_query_new(void);
LIBQIAP_API int qiap_query_add_entry(qiap_query *query, const char *mission, const char *product_type);
LIBQIAP_API void qiap_query_delete(qiap_query *query);

LIBQIAP_API qiap_algorithm *qiap_algorithm_new(const char *name, const char *reference);
LIBQIAP_API int qiap_algorithm_add_parameter(qiap_algorithm *algorithm, const char *key, const char *value);
LIBQIAP_API void qiap_algorithm_delete(qiap_algorithm *algorithm);

LIBQIAP_API qiap_action *qiap_action_new(const char *last_modification_date, qiap_action_type action_type);
LIBQIAP_API int qiap_action_set_order(qiap_action *action, long order);
LIBQIAP_API int qiap_action_set_correction(qiap_action *action, const char *correction);
LIBQIAP_API int qiap_action_set_algorithm(qiap_action *action, qiap_algorithm *algorithm);
LIBQIAP_API void qiap_action_delete(qiap_action *action);

LIBQIAP_API qiap_affected_value *qiap_affected_value_new(long affected_value_id, const char *parameter);
LIBQIAP_API int qiap_affected_value_set_extent(qiap_affected_value *affected_value, const char *extent);
LIBQIAP_API int qiap_affected_value_add_value(qiap_affected_value *affected_value, const char *parameter_value_path);
LIBQIAP_API int qiap_affected_value_add_action(qiap_affected_value *affected_value, qiap_action *action);
LIBQIAP_API void qiap_affected_value_delete(qiap_affected_value *affected_value);

LIBQIAP_API qiap_affected_product *qiap_affected_product_new(long affected_product_id, const char *product_type);
LIBQIAP_API int qiap_affected_product_set_extent(qiap_affected_product *affected_product, const char *extent);
LIBQIAP_API int qiap_affected_product_add_product(qiap_affected_product *affected_product, const char *product);
LIBQIAP_API int qiap_affected_product_add_affected_value(qiap_affected_product *affected_product,
                                                         qiap_affected_value *affected_value);
LIBQIAP_API int qiap_affected_product_add_action(qiap_affected_product *affected_product, qiap_action *action);
LIBQIAP_API void qiap_affected_product_delete(qiap_affected_product *affected_product);

LIBQIAP_API qiap_quality_issue *qiap_quality_issue_new(long issue_id, const char *last_modification_date,
                                                       const char *mission);
LIBQIAP_API int qiap_quality_issue_set_title(qiap_quality_issue *quality_issue, const char *title);
LIBQIAP_API int qiap_quality_issue_set_description(qiap_quality_issue *quality_issue, const char *description);
LIBQIAP_API int qiap_quality_issue_set_instrument(qiap_quality_issue *quality_issue, const char *instrument);
LIBQIAP_API int qiap_quality_issue_set_cause(qiap_quality_issue *quality_issue, const char *cause);
LIBQIAP_API int qiap_quality_issue_set_resolution(qiap_quality_issue *quality_issue, const char *resolution);
LIBQIAP_API int qiap_quality_issue_add_affected_product(qiap_quality_issue *quality_issue,
                                                        qiap_affected_product *affected_product);
LIBQIAP_API void qiap_quality_issue_delete(qiap_quality_issue *quality_issue);

LIBQIAP_API qiap_quality_issue_report *qiap_quality_issue_report_new(const char *organisation);
LIBQIAP_API int qiap_quality_issue_report_add_quality_issue(qiap_quality_issue_report *quality_issue_report,
                                                            qiap_quality_issue *quality_issue);
LIBQIAP_API void qiap_quality_issue_report_delete(qiap_quality_issue_report *quality_issue_report);


LIBQIAP_API int qiap_query_server(const char *serverurl, const char *username, const char *password,
                                  const qiap_query *query, qiap_quality_issue_report **quality_issue_report);
LIBQIAP_API int qiap_read_report(const char *filename, qiap_quality_issue_report **quality_issue_report);
LIBQIAP_API int qiap_write_report(const char *filename, const qiap_quality_issue_report *quality_issue_report);


/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
