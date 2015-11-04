/*
 * Copyright (C) 2009-2013 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qiap.h"
#include "coda.h"

/* This defines the amount of items that will be allocated per block for an auto-growing array (using realloc) */
#define BLOCK_SIZE 16
#define MAX_ERROR_INFO_LENGTH 4096

/** \defgroup qiap QIAP Interface
 * The QIAP interface consists of functions to query, read, manipulate and write Quality Issue Reports.
 * The interface provides transparent data structures for all data types, which means that you can access the contents
 * of these data structures directly. It is however strongly recommended to only access the data structures directly
 * for reading purposes. For creating, modifying, and deleting the structures use the interface functions that are
 * provided by the QIAP interface.
 */

/** \addtogroup qiap
 * @{
 */

LIBQIAP_API const char *libqiap_version = QIAP_VERSION;

LIBQIAP_API int qiap_option_debug = 0;

static char qiap_error_message_buffer[MAX_ERROR_INFO_LENGTH + 1];

/** Variable that contains the error type.
 * If no error has occurred the variable contains #QIAP_SUCCESS (0).
 * \hideinitializer
 */
LIBQIAP_API int qiap_errno = QIAP_SUCCESS;

/** \name Error values
 * \ingroup coda_error
 * \note Error values in the range -900..-999 are reserved for use by layers built on top of the CODA library.
 * @{
 */

/** \def QIAP_SUCCESS
 * Success (no error).
 */
/** \def QIAP_ERROR_OUT_OF_MEMORY
 * Out of memory.
 */
/** \def QIAP_ERROR_CODA
 * An error occurred in the CODA library.
 */
/** \def QIAP_ERROR_XML
 * An error occurred while parsing an XML data block.
 */

/** \def QIAP_ERROR_SERVER
 * An error occurred while trying to connect with the QIAP server.
 */

/** \def QIAP_ERROR_FILE_NOT_FOUND
 * File not found.
 */
/** \def QIAP_ERROR_FILE_OPEN
 * Could not open file.
 */
/** \def QIAP_ERROR_FILE_READ
 * Could not read data from file.
 */
/** \def QIAP_ERROR_FILE_WRITE
 * Could not write data to file.
 */

/** \def QIAP_ERROR_INVALID_ARGUMENT
 * Invalid argument.
 */

/** \def QIAP_ERROR_DISCARD
 * An applicable QIAP action determined that the product or parameter needs to be discarded.
 */

/** @} */

/** \typedef qiap_action_type
 * QIAP Action Type
 * Contains the type of action to be performed (e.g. discard or correct data)
 */

/** \typedef qiap_query
 * QIAP Query
 *
 * Contains the query parameters with which to request a Quality Issue Report from a QIAP server.
 */

/** \typedef qiap_algorithm
 * QIAP Algorithm
 *
 * Details on the custom action that needs to be performed on a product or product parameter.
 */

/** \typedef qiap_action
 * QIAP Action
 *
 * Contains the action that should be performed to the whole product or a specific product parameter.
 */

/** \typedef qiap_affected_value
 * QIAP Affected Value
 *
 * Details on a specific affected parameter in a product. Covers information on the extent and any applicable actions.
 */

/** \typedef qiap_affected_product
 * QIAP Affected Product
 *
 * Details on a specific affected product. Covers information on the extent, any affected values and applicable actions.
 */

/** \typedef qiap_quality_issue
 * QIAP Quality Issue
 *
 * Full information regarding a quality issue, including any affected product/values and associated actions.
 */

/** \typedef qiap_quality_issue_report
 * QIAP Quality Issue Report
 *
 * A collection of Quality Issues coming from a single data provider.
 */

void qiap_add_error_message_vargs(const char *message, va_list ap)
{
    int current_length;

    if (message == NULL)
    {
        return;
    }

    current_length = strlen(qiap_error_message_buffer);
    if (current_length >= MAX_ERROR_INFO_LENGTH)
    {
        return;
    }
    vsnprintf(&qiap_error_message_buffer[current_length], MAX_ERROR_INFO_LENGTH - current_length, message, ap);
    qiap_error_message_buffer[MAX_ERROR_INFO_LENGTH] = '\0';
}

void qiap_add_error_message(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    qiap_add_error_message_vargs(message, ap);
    va_end(ap);
}

void qiap_set_error_message_vargs(const char *message, va_list ap)
{
    if (message == NULL)
    {
        qiap_error_message_buffer[0] = '\0';
    }
    else
    {
        vsnprintf(qiap_error_message_buffer, MAX_ERROR_INFO_LENGTH, message, ap);
        qiap_error_message_buffer[MAX_ERROR_INFO_LENGTH] = '\0';
    }
}

void qiap_set_error_message(const char *message, ...)
{
    va_list ap;

    va_start(ap, message);
    qiap_set_error_message_vargs(message, ap);
    va_end(ap);
}

/** Set the error value and optionally set a custom error message.
 * If \a message is NULL then the default error message for the error number will be used.
 * \param err Value of #qiap_errno.
 * \param message Optional error message using printf() format.
 */
LIBQIAP_API void qiap_set_error(int err, const char *message, ...)
{
    va_list ap;

    qiap_errno = err;

    va_start(ap, message);
    qiap_set_error_message_vargs(message, ap);
    va_end(ap);

    if (err == QIAP_ERROR_CODA && message == NULL)
    {
        qiap_add_error_message("[CODA] %s", coda_errno_to_string(coda_errno));
    }
}

/** Returns a string with the description of the QIAP error.
 * If \a err equals the current QIAP error status then this function will return the error message that was last set using
 * qiap_set_error(). If the error message argument to qiap_set_error() was NULL or if \a err does not equal the current
 * QIAP error status then the default error message for \a err will be returned.
 * \param err Value of #qiap_errno.
 * \return String with a description of the QIAP error.
 */
LIBQIAP_API const char *qiap_errno_to_string(int err)
{
    if (err == qiap_errno && qiap_error_message_buffer[0] != '\0')
    {
        /* return the custom error message for the current QIAP error */
        return qiap_error_message_buffer;
    }
    else
    {
        switch (err)
        {
            case QIAP_SUCCESS:
                return "success (no error)";
            case QIAP_ERROR_OUT_OF_MEMORY:
                return "out of memory";
            case QIAP_ERROR_CODA:
                return "CODA error";
            case QIAP_ERROR_XML:
                return "unkown error while parsing XML data";

            case QIAP_ERROR_SERVER:
                return "unknown error while trying to connect with the QIAP server";

            case QIAP_ERROR_FILE_NOT_FOUND:
                return "file not found";
            case QIAP_ERROR_FILE_OPEN:
                return "could not open file";
            case QIAP_ERROR_FILE_READ:
                return "could not read data from file";
            case QIAP_ERROR_FILE_WRITE:
                return "could not write data to file";

            case QIAP_ERROR_INVALID_ARGUMENT:
                return "invalid argument";

            case QIAP_ERROR_DISCARD:
                return "data should be discarded";

            default:
                if (err == qiap_errno)
                {
                    return qiap_error_message_buffer;
                }
                else
                {
                    return "";
                }
        }
    }
}

/** Returns a string with the name of the QIAP action type.
 * Will be either 'discard product', 'discard value', 'correct value', or 'custom correction'.
 * \param action_type QIAP action type.
 * \return String with the name of the action type.
 */
LIBQIAP_API const char *qiap_get_action_type_name(qiap_action_type action_type)
{
    switch (action_type)
    {
        case qiap_action_discard_product:
            return "discard product";
        case qiap_action_discard_value:
            return "discard value";
        case qiap_action_correct_value:
            return "correct value";
        case qiap_action_custom_correction:
            return "custom correction";
    }
    assert(0);
    exit(1);
}

/** Returns a new QIAP Query data structure.
 * \return Pointer to a newly allocated and initialised QIAP Query data structure.
 */
LIBQIAP_API qiap_query *qiap_query_new(void)
{
    qiap_query *query;

    query = malloc(sizeof(qiap_query));
    if (query == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_query), __FILE__, __LINE__);
        return NULL;
    }
    query->num_entries = 0;
    query->mission = NULL;
    query->product_type = NULL;

    return query;
}

/** Add a query item to a QIAP Query.
 * \param query QIAP Query data structure to which the filter should be added.
 * \param mission Mission name for which \a product_type is applicable.
 * \param product_type Name of the product type for which Quality Issues should be retrieved.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_query_add_entry(qiap_query *query, const char *mission, const char *product_type)
{
    int i;

    if (query == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "query argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (mission == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "mission argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product_type == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "product_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    /* find out if the mission/product_type combiniation is already in the list */
    for (i = 0; i < query->num_entries; i++)
    {
        if (strcmp(query->mission[i], mission) == 0)
        {
            if (strcmp(query->product_type[i], product_type) == 0)
            {
                /* already exists -> return success */
                return 0;
            }
        }
    }

    if (query->num_entries % BLOCK_SIZE == 0)
    {
        char **new_mission;
        char **new_product_type;

        new_mission = realloc(query->mission, (query->num_entries + BLOCK_SIZE) * sizeof(char *));
        if (new_mission == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (query->num_entries + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        query->mission = new_mission;
        new_product_type = realloc(query->product_type, (query->num_entries + BLOCK_SIZE) * sizeof(char *));
        if (new_product_type == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (query->num_entries + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        query->product_type = new_product_type;
    }
    query->num_entries++;
    query->mission[query->num_entries - 1] = NULL;
    query->product_type[query->num_entries - 1] = NULL;

    query->mission[query->num_entries - 1] = strdup(mission);
    if (query->mission[query->num_entries - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    query->product_type[query->num_entries - 1] = strdup(product_type);
    if (query->product_type[query->num_entries - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Remove a QIAP Query data structure
 * Free any memory allocated for this data structure and its contents.
 * \param query QIAP Query data structure that should be removed.
 */
LIBQIAP_API void qiap_query_delete(qiap_query *query)
{
    int i;

    if (query == NULL)
    {
        return;
    }

    if (query->mission != NULL)
    {
        for (i = 0; i < query->num_entries; i++)
        {
            if (query->mission[i] != NULL)
            {
                free(query->mission[i]);
            }
        }
        free(query->mission);
    }
    if (query->product_type != NULL)
    {
        for (i = 0; i < query->num_entries; i++)
        {
            if (query->product_type[i] != NULL)
            {
                free(query->product_type[i]);
            }
        }
        free(query->product_type);
    }
    free(query);
}

/** Returns a new QIAP Algorithm data structure.
 * \param name Name of the algorithm
 * \param reference Reference to the algorithm (e.g. URL to website or formal document reference)
 * \return Pointer to a newly allocated and initialised QIAP Algorithm data structure.
 */
LIBQIAP_API qiap_algorithm *qiap_algorithm_new(const char *name, const char *reference)
{
    qiap_algorithm *algorithm;

    if (name == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "name argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }
    if (reference == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "reference argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    algorithm = malloc(sizeof(qiap_algorithm));
    if (algorithm == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_algorithm), __FILE__, __LINE__);
        return NULL;
    }
    algorithm->name = NULL;
    algorithm->reference = NULL;
    algorithm->num_parameters = 0;
    algorithm->parameter_key = NULL;
    algorithm->parameter_value = NULL;

    algorithm->name = strdup(name);
    if (algorithm->name == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_algorithm_delete(algorithm);
        return NULL;
    }
    algorithm->reference = strdup(reference);
    if (algorithm->reference == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_algorithm_delete(algorithm);
        return NULL;
    }

    return algorithm;
}

/** Add a parameter to a QIAP Algorithm.
 * \param algorithm QIAP Algorithm data structure to which the parameter should be added.
 * \param key Name of the parameter.
 * \param value Value of the parameter (as a string).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_algorithm_add_parameter(qiap_algorithm *algorithm, const char *key, const char *value)
{
    if (algorithm == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "algorithm argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (key == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "key argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (value == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (algorithm->num_parameters % BLOCK_SIZE == 0)
    {
        char **new_parameter_key;
        char **new_parameter_value;

        new_parameter_key = realloc(algorithm->parameter_key, (algorithm->num_parameters + BLOCK_SIZE) *
                                    sizeof(char *));
        if (new_parameter_key == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (algorithm->num_parameters + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        algorithm->parameter_key = new_parameter_key;
        new_parameter_value = realloc(algorithm->parameter_value, (algorithm->num_parameters + BLOCK_SIZE) *
                                      sizeof(char *));
        if (new_parameter_value == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (algorithm->num_parameters + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        algorithm->parameter_value = new_parameter_value;
    }
    algorithm->num_parameters++;
    algorithm->parameter_key[algorithm->num_parameters - 1] = NULL;
    algorithm->parameter_value[algorithm->num_parameters - 1] = NULL;

    algorithm->parameter_key[algorithm->num_parameters - 1] = strdup(key);
    if (algorithm->parameter_key[algorithm->num_parameters - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    algorithm->parameter_value[algorithm->num_parameters - 1] = strdup(value);
    if (algorithm->parameter_value[algorithm->num_parameters - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Remove a QIAP Algorithm data structure
 * Free any memory allocated for this data structure and its contents.
 * \param algorithm QIAP Algorithm data structure that should be removed.
 */
LIBQIAP_API void qiap_algorithm_delete(qiap_algorithm *algorithm)
{
    int i;

    if (algorithm == NULL)
    {
        return;
    }

    if (algorithm->name != NULL)
    {
        free(algorithm->name);
    }
    if (algorithm->reference != NULL)
    {
        free(algorithm->reference);
    }
    if (algorithm->parameter_key != NULL)
    {
        for (i = 0; i < algorithm->num_parameters; i++)
        {
            if (algorithm->parameter_key[i] != NULL)
            {
                free(algorithm->parameter_key[i]);
            }
        }
        free(algorithm->parameter_key);
    }
    if (algorithm->parameter_value != NULL)
    {
        for (i = 0; i < algorithm->num_parameters; i++)
        {
            if (algorithm->parameter_value[i] != NULL)
            {
                free(algorithm->parameter_value[i]);
            }
        }
        free(algorithm->parameter_value);
    }
    free(algorithm);
}

/** Returns a new QIAP Action data structure.
 * \param last_modification_date Last modification date of the information regarding the action (in ISO format)
 * \param action_type The type of action that is to be performed.
 * \return Pointer to a newly allocated and initialised QIAP Action data structure.
 */
LIBQIAP_API qiap_action *qiap_action_new(const char *last_modification_date, qiap_action_type action_type)
{
    qiap_action *action;

    if (last_modification_date == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "last_modification_date argument is NULL (%s:%u)", __FILE__,
                       __LINE__);
        return NULL;
    }

    action = malloc(sizeof(qiap_action));
    if (action == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_action), __FILE__, __LINE__);
        return NULL;
    }
    action->last_modification_date = NULL;
    action->action_type = action_type;
    action->order = 0;
    action->correction_string = NULL;
    action->correction = NULL;
    action->algorithm = NULL;

    action->last_modification_date = strdup(last_modification_date);
    if (action->last_modification_date == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_action_delete(action);
        return NULL;
    }

    return action;
}

/** Set the priority order of the QIAP Action.
 * \param action QIAP Action.
 * \param order Integer value giving the priority order with which to apply the action.
 *        Higher values have higher precedence.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_action_set_order(qiap_action *action, long order)
{
    if (action->action_type == qiap_action_discard_product || action->action_type == qiap_action_discard_value)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "order can not be set for a '%s' action",
                       qiap_get_action_type_name(action->action_type));
        return -1;
    }
    action->order = order;

    return 0;
}

/** Set the corrective action for a QIAP Action.
 * This function should only be called for actions of type qiap_action_correct_value.
 * \param action QIAP Action.
 * \param correction String containing a correction expression (see QIAP standard for details).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_action_set_correction(qiap_action *action, const char *correction)
{
    if (action == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (correction == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "correction argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (action->action_type != qiap_action_correct_value)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "can not assign a 'correction expression' to a '%s' action",
                       qiap_get_action_type_name(action->action_type));
        return -1;
    }
    if (action->correction_string != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action already has a correction definition");
        return -1;
    }
    action->correction_string = strdup(correction);
    if (action->correction_string == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    if (coda_expression_from_string(correction, &action->correction) != 0)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid correction expression (%s)",
                       coda_errno_to_string(coda_errno));
        return -1;
    }

    return 0;
}

/** Set the algorithm for a QIAP Action.
 * This function should only be called for actions of type qiap_action_custom.
 * \param action QIAP Action.
 * \param algorithm Pointer to a QIAP algorithm data structure.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_action_set_algorithm(qiap_action *action, qiap_algorithm *algorithm)
{
    if (action == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (algorithm == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "algorithm argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (action->action_type != qiap_action_custom_correction)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "can not assign an algorithm to a '%s' action",
                       qiap_get_action_type_name(action->action_type));
        return -1;
    }
    if (action->algorithm != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action already has an algorithm specification");
        return -1;
    }
    action->algorithm = algorithm;

    return 0;
}

/** Remove a QIAP Action data structure
 * Free any memory allocated for this data structure and its contents.
 * This function will also call qiap_algorithm_delete() if a QIAP Algorithm was set.
 * \param action QIAP Action data structure that should be removed.
 */
LIBQIAP_API void qiap_action_delete(qiap_action *action)
{
    if (action == NULL)
    {
        return;
    }

    if (action->last_modification_date != NULL)
    {
        free(action->last_modification_date);
    }
    if (action->correction_string != NULL)
    {
        free(action->correction_string);
    }
    if (action->correction != NULL)
    {
        coda_expression_delete(action->correction);
    }
    if (action->algorithm != NULL)
    {
        qiap_algorithm_delete(action->algorithm);
    }
    free(action);
}

/** Returns a new QIAP Affected Value data structure.
 * \param affected_value_id Numeric identifier of the affected value.
 * \param parameter String containing the internal path of the parameter within the product.
 * \return Pointer to a newly allocated and initialised QIAP Affected Value data structure.
 */
LIBQIAP_API qiap_affected_value *qiap_affected_value_new(long affected_value_id, const char *parameter)
{
    qiap_affected_value *affected_value;

    if (parameter == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "parameter argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    affected_value = malloc(sizeof(qiap_affected_value));
    if (affected_value == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_affected_value), __FILE__, __LINE__);
        return NULL;
    }
    affected_value->affected_value_id = affected_value_id;
    affected_value->parameter = NULL;
    affected_value->extent_string = NULL;
    affected_value->extent = NULL;
    affected_value->num_parameter_values = 0;
    affected_value->parameter_value_path = NULL;
    affected_value->num_actions = 0;
    affected_value->action = NULL;

    affected_value->parameter = strdup(parameter);
    if (affected_value->parameter == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_affected_value_delete(affected_value);
        return NULL;
    }

    return affected_value;
}

/** Set the extent for a QIAP Affected Value.
 * You should either call this function or qiap_affected_value_add_value() to define the extent, but not both.
 * \param affected_value QIAP Affected Value.
 * \param extent String containing an extent expression (see QIAP standard for details).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_value_set_extent(qiap_affected_value *affected_value, const char *extent)
{
    coda_expression_type type;

    if (affected_value == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (extent == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "extent argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (affected_value->extent_string != NULL || affected_value->num_parameter_values > 0)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected value already has an extent definition");
        return -1;
    }
    affected_value->extent_string = strdup(extent);
    if (affected_value->extent_string == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    if (coda_expression_from_string(extent, &affected_value->extent) != 0)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid extent expression (%s)", coda_errno_to_string(coda_errno));
        return -1;
    }
    if (coda_expression_get_type(affected_value->extent, &type) != 0)
    {
        qiap_set_error(QIAP_ERROR_CODA, NULL);
        return -1;
    }
    if (type != coda_expression_boolean)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "extent expression is not a boolean expression");
        return -1;
    }

    return 0;
}

/** Add a specific affected value for a QIAP Affected Value.
 * You should either call this function or qiap_affected_value_set_extent() to define the extent, but not both.
 * \param affected_value QIAP Affected Value.
 * \param parameter_value_path String containing a local path within a product that is affected by the issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_value_add_value(qiap_affected_value *affected_value, const char *parameter_value_path)
{
    if (affected_value == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (parameter_value_path == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "parameter_value_path argument is NULL (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    if (affected_value->num_parameter_values == 0 && affected_value->extent_string != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected value already has an extent definition");
        return -1;
    }

    if (affected_value->num_parameter_values % BLOCK_SIZE == 0)
    {
        char **new_parameter_value_path;

        new_parameter_value_path = realloc(affected_value->parameter_value_path,
                                           (affected_value->num_parameter_values + BLOCK_SIZE) * sizeof(char *));
        if (new_parameter_value_path == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (affected_value->num_parameter_values + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        affected_value->parameter_value_path = new_parameter_value_path;
    }
    affected_value->num_parameter_values++;
    affected_value->parameter_value_path[affected_value->num_parameter_values - 1] = NULL;

    affected_value->parameter_value_path[affected_value->num_parameter_values - 1] = strdup(parameter_value_path);
    if (affected_value->parameter_value_path[affected_value->num_parameter_values - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Add an action for a QIAP Affected Value.
 * \param affected_value QIAP Affected Value.
 * \param action Action that is to be performed for this kind of affected value.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_value_add_action(qiap_affected_value *affected_value, qiap_action *action)
{
    int i;

    if (affected_value == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (action == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    for (i = 0; i < affected_value->num_actions; i++)
    {
        if (affected_value->action[i]->action_type == action->action_type)
        {
            qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected value already has an action of type '%s'",
                           qiap_get_action_type_name(action->action_type));
            return -1;
        }
    }

    if (affected_value->num_actions % BLOCK_SIZE == 0)
    {
        qiap_action **new_action;

        new_action = realloc(affected_value->action,
                             (affected_value->num_actions + BLOCK_SIZE) * sizeof(qiap_action *));
        if (new_action == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (affected_value->num_actions + BLOCK_SIZE) * sizeof(qiap_action *), __FILE__, __LINE__);
            return -1;
        }
        affected_value->action = new_action;
    }
    affected_value->num_actions++;
    affected_value->action[affected_value->num_actions - 1] = action;

    return 0;
}

/** Remove a QIAP Affected Value data structure
 * Free any memory allocated for this data structure and its contents.
 * This function will also call qiap_action_delete() for each QIAP Action that was set.
 * \param affected_value QIAP Affected Value data structure that should be removed.
 */
LIBQIAP_API void qiap_affected_value_delete(qiap_affected_value *affected_value)
{
    int i;

    if (affected_value == NULL)
    {
        return;
    }

    if (affected_value->parameter != NULL)
    {
        free(affected_value->parameter);
    }
    if (affected_value->extent_string != NULL)
    {
        free(affected_value->extent_string);
    }
    if (affected_value->extent != NULL)
    {
        coda_expression_delete(affected_value->extent);
    }
    if (affected_value->parameter_value_path != NULL)
    {
        for (i = 0; i < affected_value->num_parameter_values; i++)
        {
            if (affected_value->parameter_value_path[i] != NULL)
            {
                free(affected_value->parameter_value_path[i]);
            }
        }
        free(affected_value->parameter_value_path);
    }
    if (affected_value->action != NULL)
    {
        for (i = 0; i < affected_value->num_actions; i++)
        {
            if (affected_value->action[i] != NULL)
            {
                qiap_action_delete(affected_value->action[i]);
            }
        }
        free(affected_value->action);
    }
    free(affected_value);
}

/** Returns a new QIAP Affected Product data structure.
 * \param affected_product_id Numeric identifier of the affected product.
 * \param product_type String containing the name of the product type for this set of affected products.
 * \return Pointer to a newly allocated and initialised QIAP Affected Product data structure.
 */
LIBQIAP_API qiap_affected_product *qiap_affected_product_new(long affected_product_id, const char *product_type)
{
    qiap_affected_product *affected_product;

    if (product_type == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "product_type argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    affected_product = malloc(sizeof(qiap_affected_product));
    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_affected_product), __FILE__, __LINE__);
        return NULL;
    }
    affected_product->affected_product_id = affected_product_id;
    affected_product->product_type = NULL;
    affected_product->extent_string = NULL;
    affected_product->extent = NULL;
    affected_product->num_products = 0;
    affected_product->product = NULL;
    affected_product->num_affected_values = 0;
    affected_product->affected_value = NULL;
    affected_product->num_actions = 0;
    affected_product->action = NULL;

    affected_product->product_type = strdup(product_type);
    if (affected_product->product_type == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_affected_product_delete(affected_product);
        return NULL;
    }

    return affected_product;
}

/** Set the extent for a QIAP Affected Product.
 * You should either call this function or qiap_affected_product_add_product() to define the extent, but not both.
 * \param affected_product QIAP Affected Product.
 * \param extent String containing an extent expression (see QIAP standard for details).
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_product_set_extent(qiap_affected_product *affected_product, const char *extent)
{
    coda_expression_type type;

    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (extent == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "extent argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (affected_product->extent_string != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected product already has an extent definition");
        return -1;
    }
    affected_product->extent_string = strdup(extent);
    if (affected_product->extent_string == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    if (coda_expression_from_string(extent, &affected_product->extent) != 0)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "invalid extent expression (%s)", coda_errno_to_string(coda_errno));
        return -1;
    }
    if (coda_expression_get_type(affected_product->extent, &type) != 0)
    {
        qiap_set_error(QIAP_ERROR_CODA, NULL);
        return -1;
    }
    if (type != coda_expression_boolean)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "extent expression is not a boolean expression");
        return -1;
    }

    return 0;
}

/** Add a specific affected product for a QIAP Affected Product.
 * You should either call this function or qiap_affected_product_set_extent() to define the extent, but not both.
 * \param affected_product QIAP Affected Product.
 * \param product String containing the filename of the product that is affected.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_product_add_product(qiap_affected_product *affected_product, const char *product)
{
    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (affected_product->num_products % BLOCK_SIZE == 0)
    {
        char **new_product;

        new_product = realloc(affected_product->product,
                              (affected_product->num_products + BLOCK_SIZE) * sizeof(char *));
        if (new_product == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (affected_product->num_products + BLOCK_SIZE) * sizeof(char *), __FILE__, __LINE__);
            return -1;
        }
        affected_product->product = new_product;
    }
    affected_product->num_products++;
    affected_product->product[affected_product->num_products - 1] = NULL;

    affected_product->product[affected_product->num_products - 1] = strdup(product);
    if (affected_product->product[affected_product->num_products - 1] == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Add an affected value for a QIAP Affected Product.
 * \param affected_product QIAP Affected Product.
 * \param affected_value QIAP Affected Value data structure for an affected value within the affected product.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_product_add_affected_value(qiap_affected_product *affected_product,
                                                         qiap_affected_value *affected_value)
{
    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (affected_value == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_value argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (affected_product->num_affected_values % BLOCK_SIZE == 0)
    {
        qiap_affected_value **new_affected_value;

        new_affected_value = realloc(affected_product->affected_value,
                                     (affected_product->num_affected_values + BLOCK_SIZE) *
                                     sizeof(qiap_affected_value *));
        if (new_affected_value == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (affected_product->num_affected_values + BLOCK_SIZE) * sizeof(qiap_affected_value *),
                           __FILE__, __LINE__);
            return -1;
        }
        affected_product->affected_value = new_affected_value;
    }
    affected_product->num_affected_values++;
    affected_product->affected_value[affected_product->num_affected_values - 1] = affected_value;

    return 0;
}

/** Add an action for a QIAP Affected Product.
 * \param affected_product QIAP Affected Product.
 * \param action Action that is to be performed for this kind of affected product.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_affected_product_add_action(qiap_affected_product *affected_product, qiap_action *action)
{
    int i;

    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (action == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "action argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (action->action_type == qiap_action_discard_value || action->action_type == qiap_action_correct_value)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected product can not have an action of type '%s'",
                       qiap_get_action_type_name(action->action_type));
        return -1;
    }

    for (i = 0; i < affected_product->num_actions; i++)
    {
        if (affected_product->action[i]->action_type == action->action_type)
        {
            qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected value already has an action of type '%s'",
                           qiap_get_action_type_name(action->action_type));
            return -1;
        }
    }

    if (affected_product->num_actions % BLOCK_SIZE == 0)
    {
        qiap_action **new_action;

        new_action = realloc(affected_product->action,
                             (affected_product->num_actions + BLOCK_SIZE) * sizeof(qiap_action *));
        if (new_action == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (affected_product->num_actions + BLOCK_SIZE) * sizeof(qiap_action *), __FILE__, __LINE__);
            return -1;
        }
        affected_product->action = new_action;
    }
    affected_product->num_actions++;
    affected_product->action[affected_product->num_actions - 1] = action;

    return 0;
}

/** Remove a QIAP Affected Product data structure
 * Free any memory allocated for this data structure and its contents.
 * This function will also call qiap_action_delete() for each QIAP Action that was set and call
 * qiap_affected_value_delete() for each QIAP Affected Value that was set.
 * \param affected_product QIAP Affected Product data structure that should be removed.
 */
LIBQIAP_API void qiap_affected_product_delete(qiap_affected_product *affected_product)
{
    int i;

    if (affected_product == NULL)
    {
        return;
    }

    if (affected_product->product_type != NULL)
    {
        free(affected_product->product_type);
    }
    if (affected_product->extent_string != NULL)
    {
        free(affected_product->extent_string);
    }
    if (affected_product->extent != NULL)
    {
        coda_expression_delete(affected_product->extent);
    }
    if (affected_product->product != NULL)
    {
        for (i = 0; i < affected_product->num_products; i++)
        {
            if (affected_product->product[i] != NULL)
            {
                free(affected_product->product[i]);
            }
        }
        free(affected_product->product);
    }
    if (affected_product->affected_value != NULL)
    {
        for (i = 0; i < affected_product->num_affected_values; i++)
        {
            if (affected_product->affected_value[i] != NULL)
            {
                qiap_affected_value_delete(affected_product->affected_value[i]);
            }
        }
        free(affected_product->affected_value);
    }
    if (affected_product->action != NULL)
    {
        for (i = 0; i < affected_product->num_actions; i++)
        {
            if (affected_product->action[i] != NULL)
            {
                qiap_action_delete(affected_product->action[i]);
            }
        }
        free(affected_product->action);
    }
    free(affected_product);
}

/** Returns a new QIAP Quality Issue data structure.
 * \param issue_id Numeric identifier of the Quality Issue.
 * \param last_modification_date Last modification date of the information regarding the issue (in ISO format)
 * \param mission String containing the name of the mission for which this issue is applicable.
 * \return Pointer to a newly allocated and initialised QIAP Quality Issue data structure.
 */
LIBQIAP_API qiap_quality_issue *qiap_quality_issue_new(long issue_id, const char *last_modification_date,
                                                       const char *mission)
{
    qiap_quality_issue *quality_issue;

    if (last_modification_date == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "last_modification_date argument is NULL (%s:%u)", __FILE__,
                       __LINE__);
        return NULL;
    }
    if (mission == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "mission argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    quality_issue = malloc(sizeof(qiap_quality_issue));
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_quality_issue), __FILE__, __LINE__);
        return NULL;
    }
    quality_issue->issue_id = issue_id;
    quality_issue->last_modification_date = NULL;
    quality_issue->mission = NULL;
    quality_issue->title = NULL;
    quality_issue->description = NULL;
    quality_issue->instrument = NULL;
    quality_issue->cause = NULL;
    quality_issue->resolution = NULL;
    quality_issue->num_affected_products = 0;
    quality_issue->affected_product = NULL;

    quality_issue->last_modification_date = strdup(last_modification_date);
    if (quality_issue->last_modification_date == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_quality_issue_delete(quality_issue);
        return NULL;
    }
    quality_issue->mission = strdup(mission);
    if (quality_issue->mission == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_quality_issue_delete(quality_issue);
        return NULL;
    }

    return quality_issue;
}

/** Set a title for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param title String containing the title of the Quality Issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_set_title(qiap_quality_issue *quality_issue, const char *title)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (title == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "title argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (title == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "title argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (quality_issue->title != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality issue already has a title");
        return -1;
    }
    quality_issue->title = strdup(title);
    if (quality_issue->title == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set a description for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param description String containing the description of the Quality Issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_set_description(qiap_quality_issue *quality_issue, const char *description)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (description == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "description argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue->description != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality issue already has a description");
        return -1;
    }
    quality_issue->description = strdup(description);
    if (quality_issue->description == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set an applicable instrument for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param instrument String containing the name of the instrument that is affected by the Quality Issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_set_instrument(qiap_quality_issue *quality_issue, const char *instrument)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (instrument == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "instrument argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue->instrument != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality issue already has an instrument specification");
        return -1;
    }
    quality_issue->instrument = strdup(instrument);
    if (quality_issue->instrument == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set a cause for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param cause String containing a description of the likely cause of the issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_set_cause(qiap_quality_issue *quality_issue, const char *cause)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (cause == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "cause argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue->cause != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality issue already has a cause");
        return -1;
    }
    quality_issue->cause = strdup(cause);
    if (quality_issue->cause == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Set a resolution for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param resolution String containing information on what the final resolution will be that will be performed by the
 *        data provider to resolve the issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_set_resolution(qiap_quality_issue *quality_issue, const char *resolution)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (resolution == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "resolution argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue->resolution != NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality issue already has a resolution");
        return -1;
    }
    quality_issue->resolution = strdup(resolution);
    if (quality_issue->resolution == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }

    return 0;
}

/** Add an affected product for a QIAP Quality Issue.
 * \param quality_issue QIAP Quality Issue.
 * \param affected_product QIAP Affected Product data structure for an affected product of the quality issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_add_affected_product(qiap_quality_issue *quality_issue,
                                                        qiap_affected_product *affected_product)
{
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }
    if (affected_product == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "affected_product argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue->num_affected_products % BLOCK_SIZE == 0)
    {
        qiap_affected_product **new_affected_product;

        new_affected_product = realloc(quality_issue->affected_product,
                                       (quality_issue->num_affected_products + BLOCK_SIZE) *
                                       sizeof(qiap_affected_product *));
        if (new_affected_product == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (quality_issue->num_affected_products + BLOCK_SIZE) * sizeof(qiap_affected_product *),
                           __FILE__, __LINE__);
            return -1;
        }
        quality_issue->affected_product = new_affected_product;
    }
    quality_issue->num_affected_products++;
    quality_issue->affected_product[quality_issue->num_affected_products - 1] = affected_product;

    return 0;
}

/** Remove a QIAP Quality Issue data structure
 * Free any memory allocated for this data structure and its contents.
 * This function will also call qiap_affected_product_delete() for each QIAP Affected Product that was set.
 * \param quality_issue QIAP Quality Issue data structure that should be removed.
 */
LIBQIAP_API void qiap_quality_issue_delete(qiap_quality_issue *quality_issue)
{
    int i;

    if (quality_issue == NULL)
    {
        return;
    }

    if (quality_issue->last_modification_date != NULL)
    {
        free(quality_issue->last_modification_date);
    }
    if (quality_issue->mission != NULL)
    {
        free(quality_issue->mission);
    }
    if (quality_issue->title != NULL)
    {
        free(quality_issue->title);
    }
    if (quality_issue->description != NULL)
    {
        free(quality_issue->description);
    }
    if (quality_issue->instrument != NULL)
    {
        free(quality_issue->instrument);
    }
    if (quality_issue->cause != NULL)
    {
        free(quality_issue->cause);
    }
    if (quality_issue->resolution != NULL)
    {
        free(quality_issue->resolution);
    }
    if (quality_issue->affected_product != NULL)
    {
        for (i = 0; i < quality_issue->num_affected_products; i++)
        {
            if (quality_issue->affected_product[i] != NULL)
            {
                qiap_affected_product_delete(quality_issue->affected_product[i]);
            }
        }
        free(quality_issue->affected_product);
    }
    free(quality_issue);
}

/** Returns a new QIAP Quality Issue Report data structure.
 * \param organisation Name of the organisation that publishes the report (i.e. the name of the data provider).
 * \return Pointer to a newly allocated and initialised QIAP Quality Issue Report data structure.
 */
LIBQIAP_API qiap_quality_issue_report *qiap_quality_issue_report_new(const char *organisation)
{
    qiap_quality_issue_report *quality_issue_report;

    if (organisation == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "organisation argument is NULL (%s:%u)", __FILE__, __LINE__);
        return NULL;
    }

    quality_issue_report = malloc(sizeof(qiap_quality_issue_report));
    if (quality_issue_report == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %ld bytes) (%s:%u)",
                       (long)sizeof(qiap_quality_issue_report), __FILE__, __LINE__);
        return NULL;
    }
    quality_issue_report->organisation = NULL;
    quality_issue_report->num_quality_issues = 0;
    quality_issue_report->quality_issue = NULL;

    quality_issue_report->organisation = strdup(organisation);
    if (quality_issue_report->organisation == NULL)
    {
        qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        qiap_quality_issue_report_delete(quality_issue_report);
        return NULL;
    }

    return quality_issue_report;
}

/** Add a Quality Issue to a QIAP Quality Issue Report.
 * \param quality_issue_report QIAP Quality Issue Report.
 * \param quality_issue QIAP Quality Issue.
 * \return
 *   \arg \c 0, Success.
 *   \arg \c -1, Error occurred (check #qiap_errno).
 */
LIBQIAP_API int qiap_quality_issue_report_add_quality_issue(qiap_quality_issue_report *quality_issue_report,
                                                            qiap_quality_issue *quality_issue)
{
    if (quality_issue_report == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue_report argument is NULL (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    if (quality_issue == NULL)
    {
        qiap_set_error(QIAP_ERROR_INVALID_ARGUMENT, "quality_issue argument is NULL (%s:%u)", __FILE__, __LINE__);
        return -1;
    }

    if (quality_issue_report->num_quality_issues % BLOCK_SIZE == 0)
    {
        qiap_quality_issue **new_quality_issue;

        new_quality_issue = realloc(quality_issue_report->quality_issue,
                                    (quality_issue_report->num_quality_issues + BLOCK_SIZE) *
                                    sizeof(qiap_quality_issue *));
        if (new_quality_issue == NULL)
        {
            qiap_set_error(QIAP_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (quality_issue_report->num_quality_issues + BLOCK_SIZE) * sizeof(qiap_quality_issue *),
                           __FILE__, __LINE__);
            return -1;
        }
        quality_issue_report->quality_issue = new_quality_issue;
    }
    quality_issue_report->num_quality_issues++;
    quality_issue_report->quality_issue[quality_issue_report->num_quality_issues - 1] = quality_issue;

    return 0;
}

/** Remove a QIAP Quality Issue Report data structure
 * Free any memory allocated for this data structure and its contents.
 * This function will also call qiap_quality_issue_delete() for each QIAP Quality Issue that was set.
 * \param quality_issue_report QIAP Quality Issue Report data structure that should be removed.
 */
LIBQIAP_API void qiap_quality_issue_report_delete(qiap_quality_issue_report *quality_issue_report)
{
    int i;

    if (quality_issue_report == NULL)
    {
        return;
    }

    if (quality_issue_report->organisation != NULL)
    {
        free(quality_issue_report->organisation);
    }
    if (quality_issue_report->quality_issue != NULL)
    {
        for (i = 0; i < quality_issue_report->num_quality_issues; i++)
        {
            if (quality_issue_report->quality_issue[i] != NULL)
            {
                qiap_quality_issue_delete(quality_issue_report->quality_issue[i]);
            }
        }
        free(quality_issue_report->quality_issue);
    }
    free(quality_issue_report);
}


/** @} */
