/*
 * Copyright (C) 2007-2014 S[&]T, The Netherlands.
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
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "export.h"
#include "coda.h"

static int coda_idl_option_filter_record_fields = 1;
static int coda_idl_option_swap_dimensions = 1;
static int coda_idl_option_time_unit_days = 0;
static int coda_idl_option_verbose = 1;

/* the maximum number of product-files that may be open simultaneously */

#define NUM_PF_SLOTS 100

struct IDL_CodaError
{
    short number;
    IDL_STRING message;
};

struct IDL_CodaNoData
{
    UCHAR opaque;
};

/* this array holds all currently opened product files. Product files are represented in IDL by a positive ULONG64;
 * internally, product #product_id will reside in slot product_id % NUM_PF_SLOTS.
 */

struct
{
    coda_product *product;
    int64_t product_id; /* used to check if the product in slot [product_id] still matches */
} product_slot[NUM_PF_SLOTS];

struct IDL_CodaDataHandle
{
    coda_cursor cursor;
    int64_t product_id; /* used to check if the product in slot [product_id] still matches */
};

static int64_t unique_id_counter;       /* incremented each time a product is succesfully opened */
static int idl_coda_loaded = 0;

IDL_StructDefPtr coda_datahandle_sdef;
IDL_StructDefPtr coda_error_sdef;
IDL_StructDefPtr coda_no_data_sdef;

static void idl_coda_set_definition_path(void)
{
    if (getenv("IDL_DLM_PATH") != NULL)
    {
#ifdef CODA_DEFINITION_IDL
        coda_set_definition_path_conditional("coda-idl.dlm", getenv("IDL_DLM_PATH"), CODA_DEFINITION_IDL);
#else
        coda_set_definition_path_conditional("coda-idl.dlm", getenv("IDL_DLM_PATH"),
                                             "../../../share/" PACKAGE "/definitions");
#endif
    }
}

static int idl_coda_init()
{
    if (!idl_coda_loaded)
    {
        assert(sizeof(UCHAR) == sizeof(uint8_t));
        assert(sizeof(IDL_UINT) == sizeof(uint16_t));
        assert(sizeof(IDL_ULONG) == sizeof(uint32_t));
        assert(sizeof(IDL_ULONG64) == sizeof(uint64_t));

        idl_coda_set_definition_path();

        if (coda_init() != 0)
        {
            return -1;
        }
        idl_coda_loaded = 1;
    }

    return 0;
}

static void idl_coda_done()
{
    if (idl_coda_loaded)
    {
        coda_done();
        idl_coda_loaded = 0;
    }
}

static double day2sec(double day)
{
    return floor(day * 86400000000.0 + 0.5) / 1000000.0;
}

static double sec2day(double sec)
{
    return floor(sec * 1000000.0 + 0.5) / 86400000000.0;
}

#define CODA_IDL_ERR_EXPECTED_SCALAR                                (-901)
#define CODA_IDL_ERR_EXPECTED_DATAHANDLE                            (-904)
#define CODA_IDL_ERR_EXPECTED_DATAHANDLE_VALUE_GOT_ARRAY            (-905)
#define CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR                       (-907)
#define CODA_IDL_ERR_WRONG_NUM_DIMS_ARRAY                           (-908)
#define CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR_INTEGER               (-909)
#define CODA_IDL_ERR_UNKNOWN_OPTION                                 (-910)
#define CODA_IDL_ERR_MAX_OPEN_FILES                                 (-920)
#define CODA_IDL_ERR_PROD_ID_NONPOSITIVE                            (-923)
#define CODA_IDL_ERR_PROD_ID_NOGOOD                                 (-922)
#define CODA_IDL_ERR_SIZE_OF_NONARRAY                               (-990)
#define CODA_IDL_ERR_ARRAY_NUM_DIMS_MISMATCH                        (-991)
#define CODA_IDL_ERR_MULTIPLE_VARIABLE_INDICES                      (-992)
#define CODA_IDL_ERR_RECORD_FIELD_NOT_AVAILABLE                     (-993)
#define CODA_IDL_ERR_NOT_A_RECORD                                   (-994)
#define CODA_IDL_ERR_VOID_EXPRESSION_NOT_SUPPORTED                  (-995)

static void fill_error(struct IDL_CodaError *fill, int err)
{
    const char *message;

    message = coda_errno_to_string(err);
    if (message[0] == '\0')
    {
        switch (err)
        {
            case CODA_IDL_ERR_EXPECTED_SCALAR:
                message = "scalar numerical argument expected";
                break;
            case CODA_IDL_ERR_EXPECTED_DATAHANDLE:
                message = "expected a CODA_DATAHANDLE structure or LONG64 product-file identifier";
                break;
            case CODA_IDL_ERR_EXPECTED_DATAHANDLE_VALUE_GOT_ARRAY:
                message = "expected a *single* CODA_DATAHANDLE single structure; use (datahandle_array[x], ...) "
                    "instead of (datahandle_array, x, ...)";
                break;
            case CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR:
                message = "only strings, integer-type scalars, or integer-type vectors may be used to select a "
                    "data item";
                break;
            case CODA_IDL_ERR_WRONG_NUM_DIMS_ARRAY:
                message = "array specification (integer-type vector) has too many elements";
                break;
            case CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR_INTEGER:
                message = "single-integer data-item selector may only be used for one-dimensional arrays";
                break;
            case CODA_IDL_ERR_UNKNOWN_OPTION:
                message = "unknown option";
                break;
            case CODA_IDL_ERR_MAX_OPEN_FILES:
                message = "maximum number of simultaneously opened product files reached";
                break;
            case CODA_IDL_ERR_PROD_ID_NONPOSITIVE:
                message = "the product-id must be a positive integer";
                break;
            case CODA_IDL_ERR_PROD_ID_NOGOOD:
                message = "the LONG64 product ID specified does not refer to a currently opened file";
                break;
            case CODA_IDL_ERR_SIZE_OF_NONARRAY:
                message = "attempt to get size of non-array";
                break;
            case CODA_IDL_ERR_ARRAY_NUM_DIMS_MISMATCH:
                message = "incorrect number of dimensions";
                break;
            case CODA_IDL_ERR_MULTIPLE_VARIABLE_INDICES:
                message = "variable index specified for more than one field";
                break;
            case CODA_IDL_ERR_RECORD_FIELD_NOT_AVAILABLE:
                message = "record field not available";
                break;
            case CODA_IDL_ERR_NOT_A_RECORD:
                message = "arguments do not point to a record";
                break;
            case CODA_IDL_ERR_VOID_EXPRESSION_NOT_SUPPORTED:
                message = "cannot evaluate void expressions";
                break;
            default:
                message = "unknown error";
                break;
        }
    }
    /* fill ERRNO and MESSAGE fields */
    fill->number = err;
    IDL_StrStore(&fill->message, (char *)message);
}

static void fill_no_data(struct IDL_CodaNoData *fill)
{
    fill->opaque = 0;
}

static IDL_VPTR mk_coda_error(int error)
{
    IDL_VPTR retval;
    struct IDL_CodaError *data;

    data = (struct IDL_CodaError *)IDL_MakeTempStructVector(coda_error_sdef, 1, &retval, FALSE);
    fill_error(data, error);

    if (data->number != CODA_SUCCESS && coda_idl_option_verbose)
    {
        char errmsg[1001];

        snprintf(errmsg, 1000, "CODA-IDL ERROR %d: \"%s\"\n", data->number, IDL_STRING_STR(&data->message));
        errmsg[1000] = '\0';
        IDL_Message(IDL_M_GENERIC, IDL_MSG_INFO, errmsg);
    }

    return retval;
}

static IDL_VPTR mk_coda_no_data(void)
{
    IDL_VPTR retval;
    struct IDL_CodaNoData *data;

    data = (struct IDL_CodaNoData *)IDL_MakeTempStructVector(coda_no_data_sdef, 1, &retval, FALSE);
    fill_no_data(data);

    return retval;
}

static IDL_VPTR x_coda_is_error(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_INT;
    retval->value.i = 0;

    if ((argv[0]->type == IDL_TYP_STRUCT) && (argv[0]->value.s.sdef == coda_error_sdef))
    {
        struct IDL_CodaError *error = (struct IDL_CodaError *)argv[0]->value.s.arr->data;

        if (error->number != CODA_SUCCESS)
        {
            retval->value.i = 1;
        }
    }

    return retval;
}

static IDL_VPTR x_coda_is_no_data(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_INT;
    retval->value.i = 0;

    if ((argv[0]->type == IDL_TYP_STRUCT) && (argv[0]->value.s.sdef == coda_no_data_sdef))
    {
        retval->value.i = 1;
    }

    return retval;
}

static IDL_VPTR x_coda_open(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;
    const char *fname;
    int product_index;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);

    /* find first free slot */
    for (product_index = 0; product_index < NUM_PF_SLOTS; product_index++)
    {
        if (product_slot[product_index].product == NULL)
        {
            break;
        }
    }

    if (product_index == NUM_PF_SLOTS)  /* all slots in use! */
    {
        return mk_coda_error(CODA_IDL_ERR_MAX_OPEN_FILES);
    }

    fname = IDL_STRING_STR(&argv[0]->value.str);
    if (coda_open(fname, &product_slot[product_index].product) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* opened succesfully! Update unique_id_counter */

    do
    {
        unique_id_counter++;
    } while ((unique_id_counter - 1) % NUM_PF_SLOTS != product_index);

    product_slot[product_index].product_id = unique_id_counter;

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_ULONG64;
    retval->value.ul64 = unique_id_counter;

    return retval;
}

static IDL_VPTR x_coda_open_as(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;
    const char *fname;
    const char *product_class;
    const char *product_type;
    int version;
    int product_index;

    assert(argc == 4);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_STRING(argv[1]);
    IDL_ENSURE_STRING(argv[2]);
    IDL_ENSURE_SCALAR(argv[0]);
    IDL_ENSURE_SCALAR(argv[1]);
    IDL_ENSURE_SCALAR(argv[2]);

    /* find first free slot */
    for (product_index = 0; product_index < NUM_PF_SLOTS; product_index++)
    {
        if (product_slot[product_index].product == NULL)
        {
            break;
        }
    }

    if (product_index == NUM_PF_SLOTS)  /* all slots in use! */
    {
        return mk_coda_error(CODA_IDL_ERR_MAX_OPEN_FILES);
    }

    fname = IDL_STRING_STR(&argv[0]->value.str);
    product_class = IDL_STRING_STR(&argv[1]->value.str);
    product_type = IDL_STRING_STR(&argv[2]->value.str);
    version = IDL_LongScalar(argv[3]);
    if (coda_open_as(fname, product_class, product_type, version, &product_slot[product_index].product) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* opened succesfully! Update unique_id_counter */

    do
    {
        unique_id_counter++;
    } while ((unique_id_counter - 1) % NUM_PF_SLOTS != product_index);

    product_slot[product_index].product_id = unique_id_counter;

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_ULONG64;
    retval->value.ul64 = unique_id_counter;

    return retval;
}

static IDL_VPTR x_coda_close(int argc, IDL_VPTR *argv)
{
    int64_t product_id;
    int product_index;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    product_id = IDL_LongScalar(argv[0]);
    if (product_id <= 0)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE); /* product file id must be a positive integer */
    }

    product_index = (int)((product_id - 1) % NUM_PF_SLOTS);

    /* check that the product file id specifies an open product file */
    if (product_slot[product_index].product_id != product_id)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NOGOOD);
    }

    if (coda_close(product_slot[product_index].product) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    product_slot[product_index].product = 0;    /* remove product file from slots */
    product_slot[product_index].product_id = 0;

    coda_set_error(CODA_SUCCESS, NULL);
    return mk_coda_error(coda_errno);
}

static IDL_VPTR x_coda_version(int argc, IDL_VPTR *argv)
{
    assert(argc == 0);
    argv = argv;
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return IDL_StrToSTRING(VERSION);
}

static IDL_VPTR x_coda_product_class(int argc, IDL_VPTR *argv)
{
    const char *product_class;
    int product_index;
    int64_t product_id;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    product_id = IDL_LongScalar(argv[0]);
    if (product_id <= 0)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE); /* product file product_id must be a positive integer */
    }

    product_index = (int)((product_id - 1) % NUM_PF_SLOTS);

    /* check that the product file product_id specifies an open product file */
    if (product_slot[product_index].product_id != product_id)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NOGOOD);
    }

    if (coda_get_product_class(product_slot[product_index].product, &product_class) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return IDL_StrToSTRING((product_class == NULL ? "" : (char *)product_class));
}

static IDL_VPTR x_coda_product_type(int argc, IDL_VPTR *argv)
{
    const char *product_type;
    int product_index;
    int64_t product_id;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    product_id = IDL_LongScalar(argv[0]);
    if (product_id <= 0)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE); /* product file product_id must be a positive integer */
    }

    product_index = (int)((product_id - 1) % NUM_PF_SLOTS);

    /* check that the product file product_id specifies an open product file */
    if (product_slot[product_index].product_id != product_id)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NOGOOD);
    }

    if (coda_get_product_type(product_slot[product_index].product, &product_type) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return IDL_StrToSTRING((product_type == NULL ? "" : (char *)product_type));
}

static IDL_VPTR x_coda_product_version(int argc, IDL_VPTR *argv)
{
    int version;
    int product_index;
    int64_t product_id;
    IDL_VPTR retval;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    product_id = IDL_LongScalar(argv[0]);
    if (product_id <= 0)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE); /* product file product_id must be a positive integer */
    }

    product_index = (int)((product_id - 1) % NUM_PF_SLOTS);

    /* check that the product file product_id specifies an open product file */
    if (product_slot[product_index].product_id != product_id)
    {
        return mk_coda_error(CODA_IDL_ERR_PROD_ID_NOGOOD);
    }

    if (coda_get_product_version(product_slot[product_index].product, &version))
    {
        return mk_coda_error(coda_errno);
    }

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_INT;
    retval->value.i = version;

    return retval;
}

static void x_coda_unload(int argc, IDL_VPTR *argv)
{
    int i;

    assert(argc == 0);
    argv = argv;
    if (!idl_coda_loaded)
    {
        return;
    }

    /* close all currently opened product files */
    for (i = 0; i < NUM_PF_SLOTS; i++)
    {
        if (product_slot[i].product)
        {
            coda_close(product_slot[i].product);

            product_slot[i].product = 0;
            product_slot[i].product_id = 0;
        }
    }

    /* unload the data dictionary */
    idl_coda_done();
}

static IDL_VPTR x_coda_time_to_string(int argc, IDL_VPTR *argv)
{
    assert(argc == 1);  /* ensured by the definition */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    switch (argv[0]->type)
    {
        case IDL_TYP_BYTE:
        case IDL_TYP_INT:
        case IDL_TYP_UINT:
        case IDL_TYP_LONG:
        case IDL_TYP_ULONG:
        case IDL_TYP_LONG64:
        case IDL_TYP_ULONG64:
        case IDL_TYP_FLOAT:
        case IDL_TYP_DOUBLE:
            if ((argv[0]->flags & IDL_V_ARR) == 0)      /* make sure it is not an array... */
            {
                char str[27];

                if (coda_idl_option_time_unit_days)
                {
                    if (coda_time_double_to_string(day2sec(IDL_DoubleScalar(argv[0])), "yyyy-MM-dd HH:mm:ss.SSSSSS",
                                                   str) != 0)
                    {
                        return mk_coda_error(coda_errno);
                    }
                }
                else
                {
                    if (coda_time_double_to_string(IDL_DoubleScalar(argv[0]), "yyyy-MM-dd HH:mm:ss.SSSSSS", str) != 0)
                    {
                        return mk_coda_error(coda_errno);
                    }
                }
                return IDL_StrToSTRING(str);
            }
            /* fall-through if array */
        default:
            return mk_coda_error(CODA_IDL_ERR_EXPECTED_SCALAR);
    }
}

static int idl_coda_fetch_datahandle_array_filldata(struct IDL_CodaDataHandle *datahandle, const char *fill,
                                                    int num_dims, const long dim[], coda_type *basetype,
                                                    long number_of_elements)
{
    coda_type_class type_class;

    assert(number_of_elements > 0);

    if (coda_type_get_class(basetype, &type_class) != 0)
    {
        return -1;
    }
    if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
    {
        if (coda_type_get_special_base_type(basetype, &basetype) != 0)
        {
            return -1;
        }
        if (coda_type_get_class(basetype, &type_class) != 0)
        {
            return -1;
        }
    }
    switch (type_class)
    {
        case coda_record_class:
        case coda_array_class:
        case coda_raw_class:
            {
                int i;

                /* copy the cursors (IDL leaves us with little choice...) */

                if (coda_cursor_goto_first_array_element(&datahandle->cursor) != 0)
                {
                    return -1;
                }
                for (i = 0; i < number_of_elements; i++)
                {
                    long index = coda_idl_option_swap_dimensions ? coda_c_index_to_fortran_index(num_dims, dim, i) : i;

                    ((struct IDL_CodaDataHandle *)fill)[index] = *datahandle;

                    if (i < number_of_elements - 1)
                    {
                        if (coda_cursor_goto_next_array_element(&datahandle->cursor) != 0)
                        {
                            return -1;
                        }
                    }
                }
                if (coda_cursor_goto_parent(&datahandle->cursor) != 0)
                {
                    return -1;
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
            {
                coda_native_type read_type;
                coda_array_ordering array_ordering =
                    coda_idl_option_swap_dimensions ? coda_array_ordering_fortran : coda_array_ordering_c;

                if (coda_type_get_read_type(basetype, &read_type) != 0)
                {
                    return -1;
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_cursor_read_int16_array(&datahandle->cursor, (int16_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_cursor_read_uint8_array(&datahandle->cursor, (uint8_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_cursor_read_int16_array(&datahandle->cursor, (int16_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_cursor_read_uint16_array(&datahandle->cursor, (uint16_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_cursor_read_int32_array(&datahandle->cursor, (int32_t *)(IDL_LONG *)fill,
                                                         array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_cursor_read_uint32_array(&datahandle->cursor, (uint32_t *)(IDL_ULONG *)fill,
                                                          array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_cursor_read_int64_array(&datahandle->cursor, (int64_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_cursor_read_uint64_array(&datahandle->cursor, (uint64_t *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_cursor_read_float_array(&datahandle->cursor, (float *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_double:
                        if (coda_cursor_read_double_array(&datahandle->cursor, (double *)fill, array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_char:
                        {
                            int i;

                            if (coda_cursor_goto_first_array_element(&datahandle->cursor) != 0)
                            {
                                return -1;
                            }
                            for (i = 0; i < number_of_elements; i++)
                            {
                                int index = i;
                                char str[2];

                                if (coda_cursor_read_char(&datahandle->cursor, &str[0]) != 0)
                                {
                                    return -1;
                                }
                                str[1] = '\0';
                                if (coda_idl_option_swap_dimensions)
                                {
                                    index = coda_c_index_to_fortran_index(num_dims, dim, index);
                                }
                                IDL_StrStore(&((IDL_STRING *)fill)[index], str);
                                if (i < number_of_elements - 1)
                                {
                                    if (coda_cursor_goto_next_array_element(&datahandle->cursor) != 0)
                                    {
                                        return -1;
                                    }
                                }
                            }
                            if (coda_cursor_goto_parent(&datahandle->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                        break;
                    case coda_native_type_string:
                        {
                            int i;

                            if (coda_cursor_goto_first_array_element(&datahandle->cursor) != 0)
                            {
                                return -1;
                            }
                            for (i = 0; i < number_of_elements; i++)
                            {
                                long length;
                                char *str;
                                int index = i;

                                if (coda_cursor_get_string_length(&datahandle->cursor, &length) != 0)
                                {
                                    return -1;
                                }
                                str = (char *)malloc(length + 1);
                                assert(str != NULL);
                                str[length] = '\0';
                                if (coda_cursor_read_string(&datahandle->cursor, str, length + 1) != 0)
                                {
                                    return -1;
                                }
                                if (coda_idl_option_swap_dimensions)
                                {
                                    index = coda_c_index_to_fortran_index(num_dims, dim, index);
                                }
                                IDL_StrStore(&((IDL_STRING *)fill)[index], str);
                                free(str);

                                if (i < number_of_elements - 1)
                                {
                                    if (coda_cursor_goto_next_array_element(&datahandle->cursor) != 0)
                                    {
                                        return -1;
                                    }
                                }
                            }
                            if (coda_cursor_goto_parent(&datahandle->cursor) != 0)
                            {
                                return -1;
                            }
                        }
                        break;
                    case coda_native_type_bytes:
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;
                coda_array_ordering array_ordering =
                    coda_idl_option_swap_dimensions ? coda_array_ordering_fortran : coda_array_ordering_c;

                if (coda_type_get_special_type(basetype, &special_type) != 0)
                {
                    return -1;
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        {
                            int i;

                            if (coda_cursor_read_double_array(&datahandle->cursor, (double *)fill, array_ordering) != 0)
                            {
                                return -1;
                            }
                            if ((special_type == coda_special_time) && coda_idl_option_time_unit_days)
                            {
                                for (i = 0; i < number_of_elements; i++)
                                {
                                    ((double *)fill)[i] = sec2day(((double *)fill)[i]);
                                }
                            }
                        }
                        break;
                    case coda_special_complex:
                        if (coda_cursor_read_complex_double_pairs_array(&datahandle->cursor, (double *)fill,
                                                                        array_ordering) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_special_no_data:
                        {
                            int i;

                            if (coda_cursor_goto_first_array_element(&datahandle->cursor) != 0)
                            {
                                return -1;
                            }
                            for (i = 0; i < number_of_elements; i++)
                            {
                                fill_no_data(&((struct IDL_CodaNoData *)fill)[i]);
                            }
                        }
                        break;
                }
            }
            break;
    }

    return 0;
}

static int idl_coda_fetch_datahandle_get_array_type(coda_type *type, int *idl_type, IDL_StructDefPtr *sdef)
{
    coda_type_class type_class;

    /* Initialize return values */
    *idl_type = IDL_TYP_UNDEF;
    *sdef = NULL;

    if (coda_type_get_class(type, &type_class) != 0)
    {
        return -1;
    }
    if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
    {
        if (coda_type_get_special_base_type(type, &type) != 0)
        {
            return -1;
        }
        if (coda_type_get_class(type, &type_class) != 0)
        {
            return -1;
        }
    }
    switch (type_class)
    {
        case coda_record_class:
        case coda_array_class:
        case coda_raw_class:
            /* for now, always return ARRAY OF DATAHANDLE */
            *idl_type = IDL_TYP_STRUCT;
            *sdef = coda_datahandle_sdef;
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
            {
                coda_native_type read_type;

                if (coda_type_get_read_type(type, &read_type) != 0)
                {
                    return -1;
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        *idl_type = IDL_TYP_INT;
                        break;
                    case coda_native_type_uint8:
                        *idl_type = IDL_TYP_BYTE;
                        break;
                    case coda_native_type_int16:
                        *idl_type = IDL_TYP_INT;
                        break;
                    case coda_native_type_uint16:
                        *idl_type = IDL_TYP_UINT;
                        break;
                    case coda_native_type_int32:
                        *idl_type = IDL_TYP_LONG;
                        break;
                    case coda_native_type_uint32:
                        *idl_type = IDL_TYP_ULONG;
                        break;
                    case coda_native_type_int64:
                        *idl_type = IDL_TYP_LONG64;
                        break;
                    case coda_native_type_uint64:
                        *idl_type = IDL_TYP_ULONG64;
                        break;
                    case coda_native_type_float:
                        *idl_type = IDL_TYP_FLOAT;
                        break;
                    case coda_native_type_double:
                        *idl_type = IDL_TYP_DOUBLE;
                        break;
                    case coda_native_type_char:
                    case coda_native_type_string:
                        *idl_type = IDL_TYP_STRING;
                        break;
                    case coda_native_type_bytes:
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_type_get_special_type(type, &special_type) != 0)
                {
                    return -1;
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        *idl_type = IDL_TYP_DOUBLE;
                        break;
                    case coda_special_complex:
                        *idl_type = IDL_TYP_DCOMPLEX;
                        break;
                    case coda_special_no_data:
                        *idl_type = IDL_TYP_STRUCT;
                        *sdef = coda_no_data_sdef;
                        break;
                }
            }
            break;
    }

    return 0;
}

static int idl_coda_fetch_datahandle_array_to_VPTR(struct IDL_CodaDataHandle *datahandle, IDL_VPTR *retval)
{
    coda_type *type;
    coda_type *basetype;
    IDL_MEMINT idl_dimspec[IDL_MAX_ARRAY_DIM];
    IDL_StructDefPtr sdef;
    IDL_VPTR tmpval;
    int32_t number_of_elements;
    char *fill;
    long dim[IDL_MAX_ARRAY_DIM];
    int num_dims;
    int idl_type;
    int i;

    number_of_elements = 1;
    if (coda_cursor_get_array_dim(&datahandle->cursor, &num_dims, dim) != 0)
    {
        return -1;
    }
    for (i = 0; i < num_dims; i++)
    {
        long local_dim = coda_idl_option_swap_dimensions ? dim[i] : dim[num_dims - i - 1];

        if (local_dim == 0)
        {
            /* IDL can not handle empty arrays, so return a no_data struct */
            *retval = mk_coda_no_data();
            return 0;
        }
        number_of_elements *= local_dim;
        idl_dimspec[i] = local_dim;
    }

    /* Get record type */
    if (coda_cursor_get_type(&datahandle->cursor, &type) != 0)
    {
        return -1;
    }
    if (coda_type_get_array_base_type(type, &basetype) != 0)
    {
        return -1;
    }

    if (idl_coda_fetch_datahandle_get_array_type(basetype, &idl_type, &sdef) != 0)
    {
        return -1;
    }

    if (sdef == NULL)
    {
        fill = IDL_MakeTempArray(idl_type, num_dims, idl_dimspec, FALSE, &tmpval);
    }
    else
    {
        fill = IDL_MakeTempStruct(sdef, num_dims, idl_dimspec, &tmpval, FALSE);
    }

    if (idl_coda_fetch_datahandle_array_filldata(datahandle, fill, num_dims, dim, basetype, number_of_elements) != 0)
    {
        IDL_Deltmp(tmpval);
        return -1;
    }

    *retval = tmpval;
    return 0;
}

static int idl_coda_fetch_cursor_to_StructDefPtr(coda_cursor *cursor, IDL_StructDefPtr *sdef)
{
    IDL_STRUCT_TAG_DEF *record_tags;
    coda_cursor record_cursor;
    coda_type *record_type;
    long field_index;
    long num_fields;
    int result = 0;
    int i;

    if (coda_cursor_get_type(cursor, &record_type) != 0)
    {
        return -1;
    }
    if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
    {
        return -1;
    }

    record_tags = (IDL_STRUCT_TAG_DEF *)malloc(sizeof(IDL_STRUCT_TAG_DEF) * (num_fields + 1));
    if (record_tags == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)sizeof(IDL_STRUCT_TAG_DEF) * (num_fields + 1), __FILE__, __LINE__);
        return -1;
    }

    record_cursor = *cursor;

    field_index = -1;
    if (num_fields > 0)
    {
        if (coda_cursor_goto_first_record_field(cursor) != 0)
        {
            free(record_tags);
            return -1;
        }

        for (i = 0; i < num_fields; i++)
        {
            int include_field;
            int available;

            include_field = 1;
            result = coda_cursor_get_record_field_available_status(&record_cursor, i, &available);
            if (result != 0)
            {
                break;
            }
            if (!available)
            {
                include_field = 0;
            }
            else if (coda_idl_option_filter_record_fields)
            {
                int hidden;

                result = coda_type_get_record_field_hidden_status(record_type, i, &hidden);
                if (result != 0)
                {
                    break;
                }
                if (hidden)
                {
                    include_field = 0;
                }
            }
            if (include_field)
            {
                coda_type_class type_class;
                coda_type *field_type;
                const char *field_name;
                int k;

                field_index++;

                record_tags[field_index].name = NULL;
                record_tags[field_index].dims = NULL;
                record_tags[field_index].type = 0;
                record_tags[field_index].flags = 0;

                result = coda_type_get_record_field_name(record_type, i, &field_name);
                if (result != 0)
                {
                    break;
                }

                record_tags[field_index].name = strdup(field_name);
                if (record_tags[field_index].name == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)",
                                   __FILE__, __LINE__);
                    result = -1;
                    break;
                }
                k = 0;
                while (record_tags[field_index].name[k] != '\0')
                {
                    record_tags[field_index].name[k] = toupper(record_tags[field_index].name[k]);
                    k++;
                }

                result = coda_cursor_get_type(cursor, &field_type);
                if (result != 0)
                {
                    break;
                }
                result = coda_type_get_class(field_type, &type_class);
                if (result != 0)
                {
                    break;
                }
                if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
                {
                    result = coda_type_get_special_base_type(field_type, &field_type);
                    if (result != 0)
                    {
                        break;
                    }
                    result = coda_type_get_class(field_type, &type_class);
                    if (result != 0)
                    {
                        break;
                    }
                }
                switch (type_class)
                {
                    case coda_record_class:
                        /* insert cursor type (recursive call) */
                        result = idl_coda_fetch_cursor_to_StructDefPtr(cursor,
                                                                       (IDL_StructDefPtr *)
                                                                       &record_tags[field_index].type);
                        break;
                    case coda_array_class:
                        {
                            long num_elements;
                            coda_type *basetype;
                            int idl_type;
                            IDL_StructDefPtr sdef;
                            long dims[IDL_MAX_ARRAY_DIM];
                            int num_dims;
                            int j;

                            result = coda_cursor_get_num_elements(cursor, &num_elements);
                            if (result != 0)
                            {
                                break;
                            }
                            /* handle the zero-element case */
                            if (num_elements == 0)
                            {
                                /* IDL can not handle empty arrays, so return a no_data struct */
                                record_tags[field_index].type = coda_no_data_sdef;
                                break;
                            }

                            result = coda_type_get_array_base_type(field_type, &basetype);
                            if (result != 0)
                            {
                                break;
                            }

                            result = idl_coda_fetch_datahandle_get_array_type(basetype, &idl_type, &sdef);
                            if (result != 0)
                            {
                                break;
                            }
                            if (sdef != NULL)
                            {
                                record_tags[field_index].type = sdef;
                            }
                            else
                            {
                                record_tags[field_index].type = (void *)(long)idl_type;
                            }

                            /* create the array descriptor */

                            result = coda_cursor_get_array_dim(cursor, &num_dims, dims);
                            if (result != 0)
                            {
                                break;
                            }

                            record_tags[field_index].dims = (IDL_MEMINT *)malloc((num_dims + 1) * sizeof(IDL_MEMINT));
                            if (record_tags[field_index].dims == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)(num_dims + 1) * sizeof(IDL_MEMINT), __FILE__, __LINE__);
                                result = -1;
                                break;
                            }
                            record_tags[field_index].dims[0] = num_dims;
                            for (j = 0; j < num_dims; j++)
                            {
                                record_tags[field_index].dims[j + 1] =
                                    coda_idl_option_swap_dimensions ? dims[j] : dims[num_dims - j - 1];
                            }
                        }
                        break;
                    case coda_raw_class:
                        {
                            int64_t size;

                            result = coda_cursor_get_byte_size(cursor, &size);
                            if (result != 0)
                            {
                                break;
                            }

                            /* handle the zero-element case */
                            if (size == 0)
                            {
                                record_tags[field_index].type = coda_no_data_sdef;
                                break;
                            }

                            record_tags[field_index].type = (void *)IDL_TYP_BYTE;
                            record_tags[field_index].dims = (IDL_MEMINT *)malloc(2 * sizeof(IDL_MEMINT));
                            if (record_tags[field_index].dims == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not allocate %lu bytes) (%s:%u)",
                                               (long)2 * sizeof(IDL_MEMINT), __FILE__, __LINE__);
                                result = -1;
                                break;
                            }
                            record_tags[field_index].dims[0] = 1;
                            record_tags[field_index].dims[1] = (long)size;
                        }
                        break;
                    case coda_integer_class:
                    case coda_real_class:
                    case coda_text_class:
                        {
                            coda_native_type read_type;

                            result = coda_cursor_get_read_type(cursor, &read_type);
                            if (result != 0)
                            {
                                break;
                            }
                            switch (read_type)
                            {
                                case coda_native_type_int8:
                                    record_tags[field_index].type = (void *)IDL_TYP_INT;
                                    break;
                                case coda_native_type_uint8:
                                    record_tags[field_index].type = (void *)IDL_TYP_BYTE;
                                    break;
                                case coda_native_type_int16:
                                    record_tags[field_index].type = (void *)IDL_TYP_INT;
                                    break;
                                case coda_native_type_uint16:
                                    record_tags[field_index].type = (void *)IDL_TYP_UINT;
                                    break;
                                case coda_native_type_int32:
                                    record_tags[field_index].type = (void *)IDL_TYP_LONG;
                                    break;
                                case coda_native_type_uint32:
                                    record_tags[field_index].type = (void *)IDL_TYP_ULONG;
                                    break;
                                case coda_native_type_int64:
                                    record_tags[field_index].type = (void *)IDL_TYP_LONG64;
                                    break;
                                case coda_native_type_uint64:
                                    record_tags[field_index].type = (void *)IDL_TYP_ULONG64;
                                    break;
                                case coda_native_type_float:
                                    record_tags[field_index].type = (void *)IDL_TYP_FLOAT;
                                    break;
                                case coda_native_type_double:
                                    record_tags[field_index].type = (void *)IDL_TYP_DOUBLE;
                                    break;
                                case coda_native_type_char:
                                case coda_native_type_string:
                                    record_tags[field_index].type = (void *)IDL_TYP_STRING;
                                    break;
                                case coda_native_type_bytes:
                                case coda_native_type_not_available:
                                    assert(0);
                                    exit(1);
                            }
                        }
                        break;
                    case coda_special_class:
                        {
                            coda_special_type special_type;

                            result = coda_cursor_get_special_type(cursor, &special_type);
                            if (result != 0)
                            {
                                break;
                            }
                            switch (special_type)
                            {
                                case coda_special_vsf_integer:
                                case coda_special_time:
                                    record_tags[field_index].type = (void *)IDL_TYP_DOUBLE;
                                    break;
                                case coda_special_complex:
                                    record_tags[field_index].type = (void *)IDL_TYP_DCOMPLEX;
                                    break;
                                case coda_special_no_data:
                                    record_tags[field_index].type = coda_no_data_sdef;
                                    break;
                            }
                        }
                        break;
                }
                if (result != 0)
                {
                    break;
                }
            }

            if (i < num_fields - 1)
            {
                result = coda_cursor_goto_next_record_field(cursor);
                if (result != 0)
                {
                    break;
                }
            }
        }
        if (result == 0)
        {
            coda_cursor_goto_parent(cursor);
        }
    }

    if (result == 0)
    {
        if (field_index == -1)
        {
            /* IDL can not handle empty records, so return a no_data struct */
            *sdef = coda_no_data_sdef;
        }
        else
        {
            record_tags[field_index + 1].name = NULL;
            record_tags[field_index + 1].dims = NULL;
            record_tags[field_index + 1].type = 0;
            record_tags[field_index + 1].flags = 0;

            *sdef = IDL_MakeStruct(0, record_tags);
        }
    }

    for (i = 0; i <= field_index; i++)
    {
        if (record_tags[i].name != NULL)
        {
            free(record_tags[i].name);
        }
        if (record_tags[i].dims != NULL)
        {
            free(record_tags[i].dims);
        }
    }
    free(record_tags);

    return result;
}

static int idl_coda_fetch_datahandle_scalar_filldata(struct IDL_CodaDataHandle *datahandle, coda_type *field_type,
                                                     char *data)
{
    coda_type_class type_class;

    if (coda_type_get_class(field_type, &type_class) != 0)
    {
        return -1;
    }

    switch (type_class)
    {
        case coda_record_class:
        case coda_array_class:
        case coda_raw_class:
            assert(0);
            exit(1);
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(&datahandle->cursor, &read_type) != 0)
                {
                    return -1;
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        if (coda_cursor_read_int16(&datahandle->cursor, (short *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint8:
                        if (coda_cursor_read_uint8(&datahandle->cursor, (UCHAR *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int16:
                        if (coda_cursor_read_int16(&datahandle->cursor, (short *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint16:
                        if (coda_cursor_read_uint16(&datahandle->cursor, (IDL_UINT *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int32:
                        if (coda_cursor_read_int32(&datahandle->cursor, (int32_t *)(IDL_LONG *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint32:
                        if (coda_cursor_read_uint32(&datahandle->cursor, (uint32_t *)(IDL_ULONG *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_int64:
                        if (coda_cursor_read_int64(&datahandle->cursor, (int64_t *)(IDL_LONG64 *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_uint64:
                        if (coda_cursor_read_uint64(&datahandle->cursor, (uint64_t *)(IDL_ULONG64 *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_float:
                        if (coda_cursor_read_float(&datahandle->cursor, (float *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_double:
                        if (coda_cursor_read_double(&datahandle->cursor, (double *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_native_type_char:
                        {
                            char str[2];

                            if (coda_cursor_read_char(&datahandle->cursor, &str[0]) != 0)
                            {
                                return -1;
                            }
                            str[1] = '\0';
                            IDL_StrStore((IDL_STRING *)data, str);
                        }
                        break;
                    case coda_native_type_string:
                        {
                            long length;
                            char *str;

                            if (coda_cursor_get_string_length(&datahandle->cursor, &length) != 0)
                            {
                                return -1;
                            }
                            str = malloc(length + 1);
                            str[length] = '\0';

                            if (coda_cursor_read_string(&datahandle->cursor, str, length + 1) != 0)
                            {
                                return -1;
                            }
                            IDL_StrStore((IDL_STRING *)data, str);
                            free(str);
                        }
                        break;
                    case coda_native_type_bytes:
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(&datahandle->cursor, &special_type) != 0)
                {
                    return -1;
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        if (coda_cursor_read_double(&datahandle->cursor, (double *)data) != 0)
                        {
                            return -1;
                        }
                        if ((special_type == coda_special_time) && coda_idl_option_time_unit_days)
                        {
                            ((double *)data)[0] = sec2day(((double *)data)[0]);
                        }
                        break;
                    case coda_special_complex:
                        if (coda_cursor_read_complex_double_pair(&datahandle->cursor, (double *)data) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_special_no_data:
                        fill_no_data((struct IDL_CodaNoData *)data);
                        break;
                }
            }
            break;
    }

    return 0;
}

static int idl_coda_fetch_datahandle_record_filldata(struct IDL_CodaDataHandle *datahandle, IDL_StructDefPtr sdef,
                                                     char *data)
{
    coda_type *record_type;
    long num_fields;
    long i;

    if (coda_cursor_get_type(&datahandle->cursor, &record_type) != 0)
    {
        return -1;
    }
    if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
    {
        return -1;
    }

    if (num_fields == 0)
    {
        fill_no_data((struct IDL_CodaNoData *)data);
    }
    else
    {
        coda_cursor record_cursor;
        long field_index;

        record_cursor = datahandle->cursor;
        field_index = -1;

        if (coda_cursor_goto_first_record_field(&datahandle->cursor) != 0)
        {
            return -1;
        }

        for (i = 0; i < num_fields; i++)
        {
            int include_field;
            int available;

            include_field = 1;
            if (coda_cursor_get_record_field_available_status(&record_cursor, i, &available) != 0)
            {
                return -1;
            }
            if (!available)
            {
                include_field = 0;
            }
            else if (coda_idl_option_filter_record_fields)
            {
                int hidden;

                if (coda_type_get_record_field_hidden_status(record_type, i, &hidden) != 0)
                {
                    return -1;
                }
                if (hidden)
                {
                    include_field = 0;
                }
            }
            if (include_field)
            {
                coda_type_class type_class;
                coda_type *field_type;
                IDL_VPTR field_info;
                char *fill;

                field_index++;

                fill = data + IDL_StructTagInfoByIndex(sdef, field_index, IDL_MSG_LONGJMP, &field_info);

                if (coda_cursor_get_type(&datahandle->cursor, &field_type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_class(field_type, &type_class) != 0)
                {
                    return -1;
                }
                if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
                {
                    if (coda_type_get_special_base_type(field_type, &field_type) != 0)
                    {
                        return -1;
                    }
                    if (coda_type_get_class(field_type, &type_class) != 0)
                    {
                        return -1;
                    }
                }
                switch (type_class)
                {
                    case coda_record_class:
                        /* This will expand the record in-situ, recursively */
                        if (idl_coda_fetch_datahandle_record_filldata(datahandle, field_info->value.s.sdef, fill) != 0)
                        {
                            return -1;
                        }
                        break;
                    case coda_array_class:
                        {
                            long num_elements;
                            long dim[IDL_MAX_ARRAY_DIM];
                            int num_dims;
                            coda_type *basetype;

                            if (coda_type_get_array_base_type(field_type, &basetype) != 0)
                            {
                                return -1;
                            }
                            if (coda_cursor_get_num_elements(&datahandle->cursor, &num_elements) != 0)
                            {
                                return -1;
                            }
                            /* handle the zero-element case */
                            if (num_elements == 0)
                            {
                                fill_no_data((struct IDL_CodaNoData *)fill);
                                break;
                            }

                            if (coda_cursor_get_array_dim(&datahandle->cursor, &num_dims, dim) != 0)
                            {
                                return -1;
                            }

                            if (idl_coda_fetch_datahandle_array_filldata(datahandle, fill, num_dims, dim, basetype,
                                                                         num_elements) != 0)
                            {
                                return -1;
                            }
                        }
                        break;
                    case coda_raw_class:
                        {
                            int64_t size;

                            if (coda_cursor_get_byte_size(&datahandle->cursor, &size) != 0)
                            {
                                return -1;
                            }
                            if (coda_cursor_read_bytes(&datahandle->cursor, (uint8_t *)fill, 0, size) != 0)
                            {
                                return -1;
                            }
                        }
                        break;
                    default:
                        /* Scalar or string */
                        if (idl_coda_fetch_datahandle_scalar_filldata(datahandle, field_type, fill) != 0)
                        {
                            return -1;
                        }
                        break;
                }
            }

            if (i < num_fields - 1)
            {
                if (coda_cursor_goto_next_record_field(&datahandle->cursor) != 0)
                {
                    return -1;
                }
            }
        }

        coda_cursor_goto_parent(&datahandle->cursor);
    }

    return 0;
}

static int idl_coda_fetch_datahandle_record_to_VPTR(struct IDL_CodaDataHandle *datahandle, IDL_VPTR *retval)
{
    IDL_StructDefPtr sdef;
    IDL_VPTR tmpval;
    char *data;

    if (idl_coda_fetch_cursor_to_StructDefPtr(&datahandle->cursor, &sdef) != 0)
    {
        return -1;
    }
    data = IDL_MakeTempStructVector(sdef, 1, &tmpval, IDL_TRUE);

    if (idl_coda_fetch_datahandle_record_filldata(datahandle, sdef, data) != 0)
    {
        IDL_Deltmp(tmpval);
        return -1;
    }

    *retval = tmpval;
    return 0;
}

static int idl_coda_fetch_datahandle_to_VPTR(struct IDL_CodaDataHandle *datahandle, IDL_VPTR *retval)
{
    coda_type_class type_class;
    IDL_VPTR tmpval;

    if (coda_cursor_get_type_class(&datahandle->cursor, &type_class) != 0)
    {
        return -1;
    }
    switch (type_class)
    {
        case coda_array_class:
            return idl_coda_fetch_datahandle_array_to_VPTR(datahandle, retval);
        case coda_record_class:
            return idl_coda_fetch_datahandle_record_to_VPTR(datahandle, retval);
        case coda_raw_class:
            {
                int64_t size;

                if (coda_cursor_get_byte_size(&datahandle->cursor, &size) != 0)
                {
                    return -1;
                }

                /* handle the zero-element case */
                if (size == 0)
                {
                    tmpval = mk_coda_no_data();
                }
                else
                {
                    IDL_MEMINT dims;
                    char *fill;

                    dims = (long)size;
                    fill = IDL_MakeTempArray(IDL_TYP_BYTE, 1, &dims, FALSE, &tmpval);
                    if (coda_cursor_read_bytes(&datahandle->cursor, (uint8_t *)fill, 0, dims) != 0)
                    {
                        IDL_Deltmp(tmpval);
                        return -1;
                    }
                }
            }
            break;
        case coda_integer_class:
        case coda_real_class:
        case coda_text_class:
            {
                coda_native_type read_type;

                if (coda_cursor_get_read_type(&datahandle->cursor, &read_type) != 0)
                {
                    return -1;
                }
                switch (read_type)
                {
                    case coda_native_type_int8:
                        /* read as int16 */
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_INT;
                        if (coda_cursor_read_int16(&datahandle->cursor, &tmpval->value.i) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_uint8:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_BYTE;
                        if (coda_cursor_read_uint8(&datahandle->cursor, &tmpval->value.c) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_int16:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_INT;
                        if (coda_cursor_read_int16(&datahandle->cursor, &tmpval->value.i) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_uint16:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_UINT;
                        if (coda_cursor_read_uint16(&datahandle->cursor, &tmpval->value.ui) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_int32:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_LONG;
                        if (coda_cursor_read_int32(&datahandle->cursor, (int32_t *)&tmpval->value.l) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_uint32:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_ULONG;
                        if (coda_cursor_read_uint32(&datahandle->cursor, (uint32_t *)&tmpval->value.ul) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_int64:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_LONG64;
                        if (coda_cursor_read_int64(&datahandle->cursor, (int64_t *)&tmpval->value.l64) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_uint64:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_ULONG64;
                        if (coda_cursor_read_uint64(&datahandle->cursor, (uint64_t *)&tmpval->value.ul64) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_float:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_FLOAT;
                        if (coda_cursor_read_float(&datahandle->cursor, &tmpval->value.f) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_double:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_DOUBLE;
                        if (coda_cursor_read_double(&datahandle->cursor, &tmpval->value.d) != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_native_type_char:
                        {
                            char str[2];

                            coda_cursor_read_char(&datahandle->cursor, &str[0]);
                            str[1] = '\0';
                            tmpval = IDL_StrToSTRING(str);
                        }
                        break;
                    case coda_native_type_string:
                        {
                            long length;
                            char *str;

                            if (coda_cursor_get_string_length(&datahandle->cursor, &length) != 0)
                            {
                                return -1;
                            }
                            str = malloc(length + 1);
                            if (str == NULL)
                            {
                                coda_set_error(CODA_ERROR_OUT_OF_MEMORY,
                                               "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                                               __LINE__);
                                return -1;
                            }
                            str[length] = '\0';

                            if (coda_cursor_read_string(&datahandle->cursor, str, length + 1) != 0)
                            {
                                return -1;
                            }
                            tmpval = IDL_StrToSTRING(str);
                            free(str);
                        }
                        break;
                    case coda_native_type_bytes:
                    case coda_native_type_not_available:
                        assert(0);
                        exit(1);
                }
            }
            break;
        case coda_special_class:
            {
                coda_special_type special_type;

                if (coda_cursor_get_special_type(&datahandle->cursor, &special_type) != 0)
                {
                    return -1;
                }
                switch (special_type)
                {
                    case coda_special_vsf_integer:
                    case coda_special_time:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_DOUBLE;
                        coda_cursor_read_double(&datahandle->cursor, &tmpval->value.d);
                        if ((special_type == coda_special_time) && coda_idl_option_time_unit_days)
                        {
                            tmpval->value.d = sec2day(tmpval->value.d);
                        }
                        break;
                    case coda_special_complex:
                        tmpval = IDL_Gettmp();
                        tmpval->type = IDL_TYP_DCOMPLEX;
                        if (coda_cursor_read_complex_double_pair(&datahandle->cursor, (double *)&tmpval->value.dcmp)
                            != 0)
                        {
                            IDL_Deltmp(tmpval);
                            return -1;
                        }
                        break;
                    case coda_special_no_data:
                        tmpval = mk_coda_no_data();
                        break;
                }
            }
            break;
    }

    *retval = tmpval;
    return 0;
}

static int idl_coda_parse_vector_dimensions(IDL_VPTR arg, int *num_dims, long *index)
{
    int i;

    *num_dims = arg->value.arr->dim[0];

    if (*num_dims > CODA_MAX_NUM_DIMS)
    {
        coda_set_error(CODA_IDL_ERR_WRONG_NUM_DIMS_ARRAY, NULL);
        return -1;
    }

    for (i = 0; i < *num_dims; i++)
    {
        switch (arg->type)
        {
            case IDL_TYP_BYTE:
                index[i] = ((UCHAR *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_INT:
                index[i] = ((int16_t *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_UINT:
                index[i] = ((IDL_UINT *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_LONG:
                index[i] = ((IDL_LONG *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_ULONG:
                index[i] = ((IDL_ULONG *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_LONG64:
                index[i] = (long)((IDL_LONG64 *)arg->value.arr->data)[i];
                break;
            case IDL_TYP_ULONG64:
                index[i] = (long)((IDL_ULONG64 *)arg->value.arr->data)[i];
                break;
            default:
                /* we shouldn't get here! */
                assert(0);
                exit(1);
        }
    }

    return 0;
}

static int idl_coda_do_fetchspec_to_datahandle(int argc, IDL_VPTR *argv, struct IDL_CodaDataHandle *datahandle,
                                               int *ret_num_dims, long *ret_dim, int *argv_index)
{
    int command;

    /* Reset the number of dimensions */
    if (ret_num_dims != NULL)
    {
        *ret_num_dims = 0;
    }

    for (command = 0; command < argc; command++)        /* traverse all the commands (if any) */
    {
        enum
        {
            cmd_error,
            cmd_string,
            cmd_integer,
            cmd_integer_vector
        } cmd_type;

        /* check the command... */

        switch (argv[command]->type)
        {
            case IDL_TYP_STRING:
                if (argv[command]->flags & IDL_V_ARR)
                {
                    cmd_type = cmd_error;       /* array of strings not allowed */
                }
                else
                {
                    cmd_type = cmd_string;      /* single string */
                }
                break;

            case IDL_TYP_BYTE:
            case IDL_TYP_INT:
            case IDL_TYP_UINT:
            case IDL_TYP_LONG:
            case IDL_TYP_ULONG:
            case IDL_TYP_LONG64:
            case IDL_TYP_ULONG64:
                if (argv[command]->flags & IDL_V_ARR)   /* integer-array? */
                {
                    /* yes: make sure it's a vector (1-dimensional array) with at least one element */
                    if ((argv[command]->value.arr->n_dim == 1) && (argv[command]->value.arr->dim[0] >= 1))
                    {
                        cmd_type = cmd_integer_vector;
                    }
                    else
                    {
                        cmd_type = cmd_error;
                    }
                }
                else
                {
                    /* no: integer scalar */
                    cmd_type = cmd_integer;
                }
                break;

            default:
                cmd_type = cmd_error;
        }

        switch (cmd_type)
        {

            case cmd_error:
                coda_set_error(CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR, NULL);
                return -1;

            case cmd_string:   /* handle 'goto record field' command */
                {
                    char *fieldname = IDL_STRING_STR(&argv[command]->value.str);
                    int available_status;
                    long field_index;

                    if (coda_cursor_get_record_field_index_from_name(&datahandle->cursor, fieldname, &field_index) != 0)
                    {
                        return -1;
                    }
                    if (coda_cursor_get_record_field_available_status(&datahandle->cursor, field_index,
                                                                      &available_status) != 0)
                    {
                        return -1;
                    }
                    if (available_status == 0)
                    {
                        coda_set_error(CODA_IDL_ERR_RECORD_FIELD_NOT_AVAILABLE, "record field %s is not available",
                                       fieldname);
                        return -1;
                    }
                    if (coda_cursor_goto_record_field_by_index(&datahandle->cursor, field_index) != 0)
                    {
                        return -1;
                    }
                    break;
                }

            case cmd_integer:  /* handle single integer-value command (goto array element) */
                {
                    long index = IDL_LongScalar(argv[command]);
                    coda_type_class type_class;
                    int num_dims;

                    if (coda_cursor_get_type_class(&datahandle->cursor, &type_class) != 0)
                    {
                        return -1;
                    }
                    if (type_class != coda_array_class)
                    {
                        coda_set_error(CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR_INTEGER, NULL);
                        return -1;
                    }

                    if (index == -1)
                    {
                        if ((ret_num_dims != NULL) && (ret_dim != NULL))
                        {
                            if (*ret_num_dims > 0)
                            {
                                /* Variable index at two parameters */
                                coda_set_error(CODA_IDL_ERR_MULTIPLE_VARIABLE_INDICES, NULL);
                                return -1;
                            }

                            if (coda_cursor_get_array_dim(&datahandle->cursor, ret_num_dims, ret_dim) != 0)
                            {
                                return -1;
                            }

                            if (*ret_num_dims > 1)
                            {
                                /* Dimension count mismatch */
                                coda_set_error(CODA_IDL_ERR_ARRAY_NUM_DIMS_MISMATCH, NULL);
                                return -1;
                            }
                        }

                        if (argv_index != NULL)
                        {
                            *argv_index = command;
                            return 0;   /* Return the cursor where the variable data starts */
                        }
                        index = 0;
                    }

                    num_dims = 1;
                    if (index == 0)
                    {
                        coda_type *type;

                        /* use zero dimensions if needed */
                        if (coda_cursor_get_type(&datahandle->cursor, &type) != 0)
                        {
                            return -1;
                        }
                        if (coda_type_get_array_num_dims(type, &num_dims) != 0)
                        {
                            return -1;
                        }
                        if (num_dims > 1)
                        {
                            /* Dimension count mismatch */
                            coda_set_error(CODA_IDL_ERR_ARRAY_NUM_DIMS_MISMATCH, NULL);
                            return -1;
                        }
                    }
                    if (coda_cursor_goto_array_element(&datahandle->cursor, num_dims, &index) != 0)
                    {
                        return -1;
                    }
                    break;
                }

            case cmd_integer_vector:   /* handle integer-value vector command (goto array element) */
                {
                    long local_index[CODA_MAX_NUM_DIMS];
                    long index[CODA_MAX_NUM_DIMS];
                    int num_dims;       /* # of dimensions == # of elements in list-var */
                    long i;

                    if (idl_coda_parse_vector_dimensions(argv[command], &num_dims, index) != 0)
                    {
                        return -1;
                    }

                    if ((ret_num_dims != NULL) && (ret_dim != NULL))
                    {
                        long arr_dim[CODA_MAX_NUM_DIMS];
                        int arr_num_dims;

                        if (coda_cursor_get_array_dim(&datahandle->cursor, &arr_num_dims, arr_dim) != 0)
                        {
                            return -1;
                        }

                        if (num_dims != arr_num_dims)
                        {
                            coda_set_error(CODA_IDL_ERR_WRONG_NUM_DIMS_ARRAY, NULL);
                            return -1;
                        }

                        /* Check for variable index values */
                        for (i = 0; i < num_dims; i++)
                        {
                            if (index[i] == -1)
                            {
                                break;
                            }
                        }

                        if (i != num_dims)
                        {
                            if (*ret_num_dims != 0)
                            {
                                /* Variable index at two parameters */
                                coda_set_error(CODA_IDL_ERR_MULTIPLE_VARIABLE_INDICES, NULL);
                                return -1;
                            }

                            for (i = 0; i < num_dims; i++)
                            {
                                if (index[i] == -1)
                                {
                                    ret_dim[i] =
                                        coda_idl_option_swap_dimensions ? arr_dim[i] : arr_dim[num_dims - i - 1];
                                    index[i] = 0;
                                }
                                else
                                {
                                    ret_dim[i] = 1;
                                }
                            }

                            *ret_num_dims = num_dims;
                        }
                    }

                    if (argv_index != NULL)
                    {
                        for (i = 0; i < num_dims; i++)
                        {
                            if (index[i] == -1)
                            {
                                *argv_index = command;
                                return 0;
                            }
                        }
                    }

                    for (i = 0; i < num_dims; i++)
                    {
                        local_index[i] = coda_idl_option_swap_dimensions ? index[i] : index[num_dims - i - 1];
                    }
                    if (coda_cursor_goto_array_element(&datahandle->cursor, num_dims, local_index) != 0)
                    {
                        return -1;
                    }
                    break;
                }

            default:
                assert(0);
                exit(1);        /* getting here implies a programming error */

        }       /* switch on cmd_type */
    }   /* traverse the commands */

    return 0;
}

static int idl_coda_fetchspec_to_datahandle(int argc, IDL_VPTR *argv, struct IDL_CodaDataHandle *datahandle,
                                            int *ret_num_dims, long *ret_dim, int *argv_index)
{
    /* several forms of this call exist:
     *
     * coda_fetch(CODA_DATAHANDLE, ...)
     *
     * coda_fetch(product_id)
     * coda_fetch(product_id, 'DSD', ...)
     *
     */

    assert(argc >= 1);

    /* handle case #1: the first argument is a CODA_DATAHANDLE */

    if (argv[0]->flags & IDL_V_STRUCT)  /* ok, it's a struct. */
    {
        int product_index;

        /* only a single CODA_DATAHANDLE structure is acceptable */

        if (argv[0]->value.s.sdef != coda_datahandle_sdef)
        {
            coda_set_error(CODA_IDL_ERR_EXPECTED_DATAHANDLE, NULL);
            return -1;
        }

        if (argv[0]->value.s.arr->n_dim > 1 || argv[0]->value.s.arr->dim[0] > 1)
        {
            coda_set_error(CODA_IDL_ERR_EXPECTED_DATAHANDLE_VALUE_GOT_ARRAY, NULL);
            return -1;
        }

        *datahandle = *(struct IDL_CodaDataHandle *)argv[0]->value.s.arr->data;

        /* product file id must be a positive integer */
        if (datahandle->product_id <= 0)
        {
            coda_set_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE, NULL);
            return -1;
        }

        product_index = (int)((datahandle->product_id - 1) % NUM_PF_SLOTS);

        /* check that the product file id specifies an open product file */
        if (product_slot[product_index].product_id != datahandle->product_id)
        {
            coda_set_error(CODA_IDL_ERR_PROD_ID_NOGOOD, NULL);
            return -1;
        }

    }
    else        /* handle case #2: the first argument should be a SCALAR INTEGER (product file id) */
    {
        int product_index;

        /* the first argument is a product file id.
         *
         * The call starts with a request for a cursor-root.
         * There should (at least) be a second argument of
         * type STRING or INT.
         */

        datahandle->product_id = IDL_LongScalar(argv[0]);
        /* product file product_id must be a positive integer */
        if (datahandle->product_id <= 0)
        {
            coda_set_error(CODA_IDL_ERR_PROD_ID_NONPOSITIVE, NULL);
            return -1;
        }

        product_index = (int)((datahandle->product_id - 1) % NUM_PF_SLOTS);

        /* check that the product file product_id specifies an open product file */
        if (product_slot[product_index].product_id != datahandle->product_id)
        {
            coda_set_error(CODA_IDL_ERR_PROD_ID_NOGOOD, NULL);
            return -1;
        }

        if (coda_cursor_set_product(&datahandle->cursor, product_slot[product_index].product) != 0)
        {
            return -1;
        }
    }

    return idl_coda_do_fetchspec_to_datahandle(argc - 1, argv + 1, datahandle, ret_num_dims, ret_dim, argv_index);
}

static int idl_coda_fetch_datahandle_create_multi_VPTR(struct IDL_CodaDataHandle *datahandle, IDL_VPTR *retval,
                                                       int num_dims, long *dim, int handlesOnlyFlag)
{
    IDL_MEMINT idl_dimspec[CODA_MAX_NUM_DIMS];
    IDL_StructDefPtr sdef;
    coda_type *type;
    char *fill;
    int idl_type;
    int i;

    /* Convert the dimension type */
    for (i = 0; i < num_dims; i++)
    {
        idl_dimspec[i] = (IDL_MEMINT)dim[i];
    }

    /* Get the type of the cursor */
    if (coda_cursor_get_type(&datahandle->cursor, &type) != 0)
    {
        return -1;
    }

    /* Determine/create the variable type */
    if (handlesOnlyFlag == 0)
    {
        if (idl_coda_fetch_datahandle_get_array_type(type, &idl_type, &sdef) != 0)
        {
            return -1;
        }
    }
    else
    {
        sdef = coda_datahandle_sdef;
    }

    /* Instantiate the IDL variable */
    if (sdef == NULL)
    {
        fill = IDL_MakeTempArray(idl_type, num_dims, idl_dimspec, FALSE, retval);
    }
    else
    {
        fill = IDL_MakeTempStruct(sdef, num_dims, idl_dimspec, retval, FALSE);
    }

    return 0;
}

static int idl_coda_fetch_datahandle_fill_multi_VPTR(int argc, IDL_VPTR *argv, IDL_VPTR retval, int handlesOnlyFlag)
{
    struct IDL_CodaDataHandle datahandleBase;
    void *sdef;
    char *dataptr;
    char *data;
    long index[CODA_MAX_NUM_DIMS];
    long dims[CODA_MAX_NUM_DIMS];
    long local_index[CODA_MAX_NUM_DIMS];
    long result_dims[CODA_MAX_NUM_DIMS];
    long num_elements;
    int num_dims;
    int argv_index;
    long result_index;
    long stride;
    long i;
    long j;
    int tmp;

    /* Set cursor to array with variable indices */
    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandleBase, NULL, NULL, &argv_index) != 0)
    {
        return -1;
    }

    /* Get array dimensions from parameter */
    if (((argv[argv_index + 1]->flags) & IDL_V_ARR) == IDL_V_ARR)
    {
        if (idl_coda_parse_vector_dimensions(argv[argv_index + 1], &num_dims, index) != 0)
        {
            return -1;
        }
    }
    else
    {
        num_dims = 1;
        index[0] = IDL_LongScalar(argv[argv_index + 1]);
    }

    /* Get the real number of dimensions and dimension extents */
    if (coda_cursor_get_array_dim(&datahandleBase.cursor, &tmp, dims) != 0)
    {
        return -1;
    }
    if (tmp != num_dims)
    {
        coda_set_error(CODA_IDL_ERR_ARRAY_NUM_DIMS_MISMATCH, NULL);
        return -1;
    }

    /* reset local index and set the dimensions of the result array. */
    num_elements = 1;
    for (i = 0; i < num_dims; i++)
    {
        long local_dim = coda_idl_option_swap_dimensions ? dims[i] : dims[num_dims - i - 1];

        num_elements *= local_dim;
        if (index[i] == -1)
        {
            result_dims[i] = local_dim;
        }
        else
        {
            result_dims[i] = 1;
        }
        local_index[i] = 0;
    }

    /* Get result variable data, stride and possible structure definition */
    if ((retval->flags & IDL_V_STRUCT) == IDL_V_STRUCT)
    {
        dataptr = (char *)retval->value.s.arr->data;
        stride = retval->value.s.arr->elt_len;
        sdef = retval->value.s.sdef;
    }
    else
    {
        dataptr = (char *)retval->value.arr->data;
        stride = retval->value.arr->elt_len;
        sdef = NULL;
    }

    /* Traverse all variable indices */
    if (coda_cursor_goto_first_array_element(&datahandleBase.cursor) != 0)
    {
        return -1;
    }
    result_index = 0;
    for (i = 0; i < num_elements; i++)
    {
        int read_array_element;

        read_array_element = 1;
        for (j = 0; j < num_dims; j++)
        {
            long ind = coda_idl_option_swap_dimensions ? local_index[j] : local_index[num_dims - j - 1];

            if (index[j] != -1 && ind != index[j])
            {
                read_array_element = 0;
                break;
            }
        }

        if (read_array_element)
        {
            struct IDL_CodaDataHandle datahandle;
            coda_type *type;
            coda_type_class type_class;

            /* Set datahandle to base datahandle */
            datahandle = datahandleBase;

            /* Set data pointer */
            if (coda_idl_option_swap_dimensions)
            {
                data = &dataptr[coda_c_index_to_fortran_index(num_dims, result_dims, result_index) * stride];
            }
            else
            {
                data = &dataptr[result_index * stride];
            }

            /* Traverse remaining parameters (should contain no more -1 indices) */
            if (idl_coda_do_fetchspec_to_datahandle(argc - (argv_index + 2), argv + (argv_index + 2), &datahandle, NULL,
                                                    NULL, NULL) != 0)
            {
                return -1;
            }

            /* Get the type of the cursor */
            if (coda_cursor_get_type(&datahandle.cursor, &type) != 0)
            {
                return -1;
            }
            if (coda_type_get_class(type, &type_class) != 0)
            {
                return -1;
            }
            if (coda_get_option_bypass_special_types() && type_class == coda_special_class)
            {
                if (coda_type_get_special_base_type(type, &type) != 0)
                {
                    return -1;
                }
                if (coda_type_get_class(type, &type_class) != 0)
                {
                    return -1;
                }
            }

            /* Store data or datahandle */
            if (handlesOnlyFlag)
            {
                ((struct IDL_CodaDataHandle *)data)[0] = datahandle;
            }
            else
            {
                switch (type_class)
                {
                    case coda_record_class:
                    case coda_array_class:
                    case coda_raw_class:
                        ((struct IDL_CodaDataHandle *)data)[0] = datahandle;
                        break;
                    default:
                        /* Scalar */
                        if (idl_coda_fetch_datahandle_scalar_filldata(&datahandle, type, data) != 0)
                        {
                            return -1;
                        }
                        break;
                }
            }
            result_index++;
        }

        /* Update indices and goto next array element */
        for (j = num_dims - 1; j >= 0; j--)
        {
            local_index[j]++;
            if (local_index[j] < dims[j])
            {
                break;
            }
            local_index[j] = 0;
        }
        if (i < num_elements - 1)
        {
            if (coda_cursor_goto_next_array_element(&datahandleBase.cursor) != 0)
            {
                return -1;
            }
        }
    }
    if (coda_cursor_goto_parent(&datahandleBase.cursor) != 0)
    {
        return -1;
    }

    return 0;
}

static IDL_VPTR x_coda_fetch(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    IDL_VPTR retval;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    num_dims = 0;
    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, &num_dims, dim, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (num_dims == 0)
    {
        if (idl_coda_fetch_datahandle_to_VPTR(&datahandle, &retval) != 0)
        {
            return mk_coda_error(coda_errno);
        }
    }
    else
    {
        if (idl_coda_fetch_datahandle_create_multi_VPTR(&datahandle, &retval, num_dims, dim, 0) != 0)
        {
            return mk_coda_error(coda_errno);
        }
        if (idl_coda_fetch_datahandle_fill_multi_VPTR(argc, argv, retval, 0) != 0)
        {
            IDL_Deltmp(retval);
            return mk_coda_error(coda_errno);
        }
    }

    return retval;
}

static IDL_VPTR x_coda_fetch_datahandle(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    IDL_VPTR retval;
    int num_dims;
    long dim[CODA_MAX_NUM_DIMS];

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    num_dims = 0;
    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, &num_dims, dim, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (num_dims == 0)
    {
        char *data;

        data = IDL_MakeTempStructVector(coda_datahandle_sdef, 1, &retval, FALSE);
        ((struct IDL_CodaDataHandle *)data)[0] = datahandle;
    }
    else
    {
        if (idl_coda_fetch_datahandle_create_multi_VPTR(&datahandle, &retval, num_dims, dim, 1) != 0)
        {
            return mk_coda_error(coda_errno);
        }
        if (idl_coda_fetch_datahandle_fill_multi_VPTR(argc, argv, retval, 1) != 0)
        {
            IDL_Deltmp(retval);
            return mk_coda_error(coda_errno);
        }
    }

    return retval;
}

static IDL_VPTR x_coda_attributes(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;
    struct IDL_CodaDataHandle datahandle;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (coda_cursor_goto_attributes(&datahandle.cursor) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetch_datahandle_to_VPTR(&datahandle, &retval) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return retval;
}

static IDL_VPTR x_coda_eval(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_expression_type type;
    coda_cursor *cursor = NULL;
    coda_expression *expr;
    IDL_VPTR retval;
    char *exprstring;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);
    exprstring = IDL_STRING_STR(&argv[0]->value.str);
    if (coda_expression_from_string(exprstring, &expr) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (coda_expression_get_type(expr, &type) != 0)
    {
        coda_expression_delete(expr);
        return mk_coda_error(coda_errno);
    }

    if (argc > 1)
    {
        /* we move the datahandle to the requested cursor position */
        if (idl_coda_fetchspec_to_datahandle(argc - 1, &argv[1], &datahandle, NULL, NULL, NULL) != 0)
        {
            coda_expression_delete(expr);
            return mk_coda_error(coda_errno);
        }
        cursor = &datahandle.cursor;
    }
    else if (!coda_expression_is_constant(expr))
    {
        coda_expression_delete(expr);
        return mk_coda_error(CODA_IDL_ERR_EXPECTED_DATAHANDLE);
    }

    switch (type)
    {
        case coda_expression_boolean:
            {
                int value;

                if (coda_expression_eval_bool(expr, cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    return mk_coda_error(coda_errno);
                }
                retval = IDL_Gettmp();
                retval->type = IDL_TYP_INT;
                retval->value.i = value;
            }
            break;
        case coda_expression_integer:
            {
                int64_t value;

                if (coda_expression_eval_integer(expr, cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    return mk_coda_error(coda_errno);
                }
                retval = IDL_Gettmp();
                retval->type = IDL_TYP_LONG64;
                retval->value.l64 = value;
            }
            break;
        case coda_expression_float:
            {
                double value;

                if (coda_expression_eval_float(expr, cursor, &value) != 0)
                {
                    coda_expression_delete(expr);
                    return mk_coda_error(coda_errno);
                }
                retval = IDL_Gettmp();
                retval->type = IDL_TYP_DOUBLE;
                retval->value.d = value;
            }
            break;
        case coda_expression_string:
            {
                char *value;
                long length;

                if (coda_expression_eval_string(expr, cursor, &value, &length) != 0)
                {
                    coda_expression_delete(expr);
                    return mk_coda_error(coda_errno);
                }
                retval = IDL_StrToSTRING(value != NULL ? value : "");
            }
            break;
        case coda_expression_node:
            {
                char *data;

                if (coda_expression_eval_node(expr, cursor) != 0)
                {
                    coda_expression_delete(expr);
                    return mk_coda_error(coda_errno);
                }
                data = IDL_MakeTempStructVector(coda_datahandle_sdef, 1, &retval, FALSE);
                ((struct IDL_CodaDataHandle *)data)[0] = datahandle;
            }
            break;
        case coda_expression_void:
            coda_expression_delete(expr);
            return mk_coda_error(CODA_IDL_ERR_VOID_EXPRESSION_NOT_SUPPORTED);
    }
    coda_expression_delete(expr);

    return retval;
}

static IDL_VPTR x_coda_size(int argc, IDL_VPTR *argv)
{
    IDL_VPTR retval;
    struct IDL_CodaDataHandle datahandle;
    IDL_ULONG *data;
    coda_type_class type_class;
    long dims[IDL_MAX_ARRAY_DIM];
    int num_dims;
    int i;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* we now have a valid cursor! */

    if (coda_cursor_get_type_class(&datahandle.cursor, &type_class) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (type_class != coda_array_class)
    {
        /* it is a scalar, return 0 */
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_ULONG;
        retval->value.ul = 0;
        return retval;
    }

    /* create the array descriptor */

    if (coda_cursor_get_array_dim(&datahandle.cursor, &num_dims, dims) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    data = (IDL_ULONG *)IDL_MakeTempVector(IDL_TYP_ULONG, num_dims, 0, &retval);
    for (i = 0; i < num_dims; i++)
    {
        data[i] = coda_idl_option_swap_dimensions ? dims[i] : dims[num_dims - i - 1];
    }

    return retval;
}

static IDL_VPTR x_coda_unit(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_type *type;
    const char *unit;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (coda_cursor_get_type(&datahandle.cursor, &type) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (coda_type_get_unit(type, &unit) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return IDL_StrToSTRING(unit != NULL ? (char *)unit : "not available");
}

static IDL_VPTR x_coda_description(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_type *type;
    const char *description;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (coda_cursor_get_type(&datahandle.cursor, &type) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (coda_type_get_description(type, &description) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    return IDL_StrToSTRING(description != NULL ? (char *)description : "not available");
}

static IDL_VPTR x_coda_getopt(int argc, IDL_VPTR *argv)
{
    char *name;
    IDL_VPTR retval;

    assert(argc == 1);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);

    name = IDL_STRING_STR(&argv[0]->value.str);

    if (strcasecmp("FilterRecordFields", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_filter_record_fields;
    }
    else if (strcasecmp("PerformConversions", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_get_option_perform_conversions();
    }
    else if (strcasecmp("PerformBoundaryChecks", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_get_option_perform_boundary_checks();
    }
    else if (strcasecmp("SwapDimensions", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_swap_dimensions;
    }
    else if (strcasecmp("TimeUnitDays", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_time_unit_days;
    }
    else if (strcasecmp("UseSpecialTypes", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = !coda_get_option_bypass_special_types();
    }
    else if (strcasecmp("UseMMap", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_get_option_use_mmap();
    }
    else if (strcasecmp("Verbose", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_verbose;
    }
    else
    {
        retval = mk_coda_error(CODA_IDL_ERR_UNKNOWN_OPTION);
    }
    return retval;
}

static IDL_VPTR x_coda_setopt(int argc, IDL_VPTR *argv)
{
    char *name;
    int value;
    IDL_VPTR retval;

    assert(argc == 2);
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);

    name = IDL_STRING_STR(&argv[0]->value.str);
    value = IDL_LongScalar(argv[1]);

    if (strcasecmp("FilterRecordFields", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_filter_record_fields;
        coda_idl_option_filter_record_fields = (value != 0);
    }
    else if (strcasecmp("PerformConversions", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_get_option_perform_conversions();
        coda_set_option_perform_conversions(value != 0);
    }
    else if (strcasecmp("PerformBoundaryChecks", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_get_option_perform_boundary_checks();
        coda_set_option_perform_boundary_checks(value != 0);
    }
    else if (strcasecmp("SwapDimensions", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_swap_dimensions;
        coda_idl_option_swap_dimensions = (value != 0);
    }
    else if (strcasecmp("UseSpecialTypes", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = !coda_get_option_bypass_special_types();
        coda_set_option_bypass_special_types(value == 0);
    }
    else if (strcasecmp("UseMMap", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = !coda_get_option_use_mmap();
        coda_set_option_use_mmap(value != 0);
    }
    else if (strcasecmp("TimeUnitDays", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_time_unit_days;
        coda_idl_option_time_unit_days = (value != 0);
    }
    else if (strcasecmp("Verbose", name) == 0)
    {
        retval = IDL_Gettmp();
        retval->type = IDL_TYP_INT;
        retval->value.i = coda_idl_option_verbose;
        coda_idl_option_verbose = (value != 0);
    }
    else
    {
        retval = mk_coda_error(CODA_IDL_ERR_UNKNOWN_OPTION);
    }
    return retval;
}

static IDL_VPTR x_coda_fieldavailable(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_type_class type_class;
    char *fieldname;
    IDL_VPTR retval;
    int available_status;
    long field_index;

    assert(argc > 1);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* we move the datahandle to the record and parse the final fieldname argument ourselves */
    if (idl_coda_fetchspec_to_datahandle(argc - 1, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* we now have a valid cursor that should point to a record */

    if (coda_cursor_get_type_class(&datahandle.cursor, &type_class) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (type_class != coda_record_class)
    {
        coda_set_error(CODA_IDL_ERR_NOT_A_RECORD, "arguments do not point to a record field");
        return mk_coda_error(coda_errno);
    }

    /* it is a record, now parse the final argument */

    if (argv[argc - 1]->type != IDL_TYP_STRING)
    {
        coda_set_error(CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR, "string argument expected");
        return mk_coda_error(coda_errno);
    }
    if (argv[argc - 1]->flags & IDL_V_ARR)
    {
        return mk_coda_error(CODA_IDL_ERR_WRONG_DATA_ITEM_SELECTOR);
    }

    fieldname = IDL_STRING_STR(&argv[argc - 1]->value.str);
    if (coda_cursor_get_record_field_index_from_name(&datahandle.cursor, fieldname, &field_index) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (coda_cursor_get_record_field_available_status(&datahandle.cursor, field_index, &available_status) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_INT;
    retval->value.i = available_status;

    return retval;
}

static IDL_VPTR x_coda_fieldcount(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_type_class type_class;
    coda_type *record_type;
    IDL_VPTR retval;
    long num_fields;
    int num_fields_create;
    int i;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* we now have a valid cursor */

    if (coda_cursor_get_type_class(&datahandle.cursor, &type_class) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (type_class != coda_record_class)
    {
        /* 'coda_fieldcount' will only work for records. */
        return mk_coda_error(CODA_IDL_ERR_NOT_A_RECORD);
    }

    /* it is a record; determine the number of fields */

    if (coda_cursor_get_type(&datahandle.cursor, &record_type) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    num_fields_create = 0;
    for (i = 0; i < num_fields; i++)
    {
        int available;

        if (coda_cursor_get_record_field_available_status(&datahandle.cursor, i, &available) != 0)
        {
            return mk_coda_error(coda_errno);
        }
        if (available)
        {
            if (coda_idl_option_filter_record_fields)
            {
                int hidden;

                if (coda_type_get_record_field_hidden_status(record_type, i, &hidden) != 0)
                {
                    return mk_coda_error(coda_errno);
                }
                if (!hidden)
                {
                    num_fields_create++;
                }
            }
            else
            {
                num_fields_create++;
            }
        }
    }

    retval = IDL_Gettmp();
    retval->type = IDL_TYP_INT;
    retval->value.i = num_fields_create;

    return retval;
}

static IDL_VPTR x_coda_fieldnames(int argc, IDL_VPTR *argv)
{
    struct IDL_CodaDataHandle datahandle;
    coda_type_class type_class;
    coda_type *record_type;
    const char **field_name;
    IDL_STRING *data;
    IDL_VPTR retval;
    long num_fields;
    int num_fields_create;
    int i;

    assert(argc > 0);   /* this is guaranteed by the limits set in 'coda-idl.dlm' */
    if (idl_coda_init() != 0)
    {
        return mk_coda_error(coda_errno);
    }

    if (idl_coda_fetchspec_to_datahandle(argc, argv, &datahandle, NULL, NULL, NULL) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    /* we now have a valid cursor to a record */

    coda_cursor_get_type_class(&datahandle.cursor, &type_class);
    if (type_class != coda_record_class)
    {
        /* 'coda_fieldnames' will only work for records. */
        return mk_coda_error(CODA_IDL_ERR_NOT_A_RECORD);
    }

    /* it is a record; determine the number of fields */

    if (coda_cursor_get_type(&datahandle.cursor, &record_type) != 0)
    {
        return mk_coda_error(coda_errno);
    }
    if (coda_type_get_num_record_fields(record_type, &num_fields) != 0)
    {
        return mk_coda_error(coda_errno);
    }

    field_name = malloc(num_fields * sizeof(*field_name));
    if (field_name == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)num_fields * sizeof(*field_name), __FILE__, __LINE__);
        return mk_coda_error(coda_errno);
    }

    num_fields_create = 0;
    for (i = 0; i < num_fields; i++)
    {
        int available;

        if (coda_cursor_get_record_field_available_status(&datahandle.cursor, i, &available) != 0)
        {
            free(field_name);
            return mk_coda_error(coda_errno);
        }
        if (available)
        {
            if (coda_idl_option_filter_record_fields)
            {
                int hidden;

                if (coda_type_get_record_field_hidden_status(record_type, i, &hidden) != 0)
                {
                    free(field_name);
                    return mk_coda_error(coda_errno);
                }
                if (!hidden)
                {
                    if (coda_type_get_record_field_name(record_type, i, &field_name[num_fields_create]) != 0)
                    {
                        free(field_name);
                        return mk_coda_error(coda_errno);
                    }
                    num_fields_create++;
                }
            }
            else
            {
                if (coda_type_get_record_field_name(record_type, i, &field_name[num_fields_create]) != 0)
                {
                    free(field_name);
                    return mk_coda_error(coda_errno);
                }
                num_fields_create++;
            }
        }
    }

    data = (IDL_STRING *)IDL_MakeTempVector(IDL_TYP_STRING, num_fields_create, 0, &retval);

    for (i = 0; i < num_fields_create; i++)
    {
        int k;
        char *upper_name;

        upper_name = strdup(field_name[i]);
        if (upper_name == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                           __LINE__);
            free(field_name);
            return mk_coda_error(coda_errno);
        }
        k = 0;
        while (upper_name[k] != '\0')
        {
            upper_name[k] = toupper(upper_name[k]);
            k++;
        }
        IDL_StrStore(&data[i], upper_name);
        free(upper_name);
    }

    free(field_name);

    return retval;
}

static void register_idl_struct_types(void)
{
    /* define the CODA_DATAHANDLE structure type */

    static IDL_MEMINT coda_datahandle_opaque_dim[] = { 1, sizeof(coda_cursor) };
    static IDL_STRUCT_TAG_DEF coda_datahandle_tags[] = {
        {"OPAQUE", coda_datahandle_opaque_dim, (void *)IDL_TYP_BYTE, 0},
        {"PF_ID", 0, (void *)IDL_TYP_ULONG64, 0},
        {0, 0, 0, 0}
    };

    /* define the CODA_ERROR structure type */

    static IDL_STRUCT_TAG_DEF coda_error_tags[] = {
        {"ERRNO", 0, (void *)IDL_TYP_INT, 0},
        {"MESSAGE", 0, (void *)IDL_TYP_STRING, 0},
        {0, 0, 0, 0}
    };

    /* define the CODA_NO_DATA structure type */

    static IDL_STRUCT_TAG_DEF coda_no_data_tags[] = {
        {"OPAQUE", 0, (void *)IDL_TYP_BYTE, 0},
        {0, 0, 0, 0}
    };

    coda_datahandle_sdef = IDL_MakeStruct("CODA_DATAHANDLE", coda_datahandle_tags);
    coda_error_sdef = IDL_MakeStruct("CODA_ERROR", coda_error_tags);
    coda_no_data_sdef = IDL_MakeStruct("CODA_NO_DATA", coda_no_data_tags);
}

static int register_idl_functions_and_procedures(void)
{

#ifdef HAVE_IDL_SYSFUN_DEF2

    /* new-style function declarations */

#ifdef HAVE_IDL_SYSRTN_UNION

    /* sysrtn is a union in sysfun_def2 */

    static IDL_SYSFUN_DEF2 idl_func_addr[] = {
        {{x_coda_attributes}, "CODA_ATTRIBUTES", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_close}, "CODA_CLOSE", 1, 1, 0, 0},
        {{x_coda_description}, "CODA_DESCRIPTION", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_eval}, "CODA_EVAL", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_fetch}, "CODA_FETCH", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_fetch_datahandle}, "CODA_FETCH_DATAHANDLE", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_fieldavailable}, "CODA_FIELDAVAILABLE", 2, IDL_MAXPARAMS, 0, 0},
        {{x_coda_fieldcount}, "CODA_FIELDCOUNT", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_fieldnames}, "CODA_FIELDNAMES", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_getopt}, "CODA_GETOPT", 1, 1, 0, 0},
        {{x_coda_is_no_data}, "CODA_IS_NO_DATA", 1, 1, 0, 0},
        {{x_coda_is_error}, "CODA_IS_ERROR", 1, 1, 0, 0},
        {{x_coda_open}, "CODA_OPEN", 1, 1, 0, 0},
        {{x_coda_open_as}, "CODA_OPEN_AS", 4, 4, 0, 0},
        {{x_coda_product_class}, "CODA_PRODUCT_CLASS", 1, 1, 0, 0},
        {{x_coda_product_type}, "CODA_PRODUCT_TYPE", 1, 1, 0, 0},
        {{x_coda_product_version}, "CODA_PRODUCT_VERSION", 1, 1, 0, 0},
        {{x_coda_setopt}, "CODA_SETOPT", 2, 2, 0, 0},
        {{x_coda_size}, "CODA_SIZE", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_time_to_string}, "CODA_TIME_TO_STRING", 1, 1, 0, 0},
        {{x_coda_unit}, "CODA_UNIT", 1, IDL_MAXPARAMS, 0, 0},
        {{x_coda_version}, "CODA_VERSION", 0, 0, 0, 0}
    };

    /* new-style procedure declarations */

    static IDL_SYSFUN_DEF2 idl_proc_addr[] = {
        {{(IDL_SYSRTN_GENERIC)x_coda_unload}, "CODA_UNLOAD", 0, 0, 0, 0}
    };

#else

    /* sysrtn is of type IDL_FUN_RET */

    static IDL_SYSFUN_DEF2 idl_func_addr[] = {
        {x_coda_attributes, "CODA_ATTRIBUTES", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_close, "CODA_CLOSE", 1, 1, 0, 0},
        {x_coda_description, "CODA_DESCRIPTION", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_eval, "CODA_EVAL", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_fetch, "CODA_FETCH", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_fetch_datahandle, "CODA_FETCH_DATAHANDLE", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_fieldavailable, "CODA_FIELDAVAILABLE", 2, IDL_MAXPARAMS, 0, 0},
        {x_coda_fieldcount, "CODA_FIELDCOUNT", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_fieldnames, "CODA_FIELDNAMES", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_getopt, "CODA_GETOPT", 1, 1, 0, 0},
        {x_coda_is_no_data, "CODA_IS_NO_DATA", 1, 1, 0, 0},
        {x_coda_is_error, "CODA_IS_ERROR", 1, 1, 0, 0},
        {x_coda_open, "CODA_OPEN", 1, 1, 0, 0},
        {x_coda_open_as, "CODA_OPEN_AS", 4, 4, 0, 0},
        {x_coda_product_class, "CODA_PRODUCT_CLASS", 1, 1, 0, 0},
        {x_coda_product_type, "CODA_PRODUCT_TYPE", 1, 1, 0, 0},
        {x_coda_product_version, "CODA_PRODUCT_VERSION", 1, 1, 0, 0},
        {x_coda_setopt, "CODA_SETOPT", 2, 2, 0, 0},
        {x_coda_size, "CODA_SIZE", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_time_to_string, "CODA_TIME_TO_STRING", 1, 1, 0, 0},
        {x_coda_unit, "CODA_UNIT", 1, IDL_MAXPARAMS, 0, 0},
        {x_coda_version, "CODA_VERSION", 0, 0, 0, 0}
    };

    /* new-style procedure declarations */

    static IDL_SYSFUN_DEF2 idl_proc_addr[] = {
        {(IDL_FUN_RET)x_coda_unload, "CODA_UNLOAD", 0, 0, 0, 0}
    };

#endif

    return IDL_SysRtnAdd(idl_func_addr, TRUE, sizeof(idl_func_addr) / sizeof(IDL_SYSFUN_DEF2)) &&
        IDL_SysRtnAdd(idl_proc_addr, FALSE, sizeof(idl_proc_addr) / sizeof(IDL_SYSFUN_DEF2));

#else

    /* old-style function declarations */

    static IDL_SYSFUN_DEF idl_func_addr[] = {
        {x_coda_attributes, "CODA_ATTRIBUTES", 1, IDL_MAXPARAMS, 0},
        {x_coda_close, "CODA_CLOSE", 1, 1, 0},
        {x_coda_description, "CODA_DESCRIPTION", 1, IDL_MAXPARAMS, 0},
        {x_coda_eval, "CODA_EVAL", 1, IDL_MAXPARAMS, 0},
        {x_coda_fetch, "CODA_FETCH", 1, IDL_MAXPARAMS, 0},
        {x_coda_fetch_datahandle, "CODA_FETCH_DATAHANDLE", 1, IDL_MAXPARAMS, 0},
        {x_coda_fieldavailable, "CODA_FIELDAVAILABLE", 2, IDL_MAXPARAMS, 0},
        {x_coda_fieldcount, "CODA_FIELDCOUNT", 1, IDL_MAXPARAMS, 0},
        {x_coda_fieldnames, "CODA_FIELDNAMES", 1, IDL_MAXPARAMS, 0},
        {x_coda_getopt, "CODA_GETOPT", 1, 1, 0},
        {x_coda_is_no_data, "CODA_IS_NO_DATA", 1, 1, 0},
        {x_coda_is_error, "CODA_IS_ERROR", 1, 1, 0},
        {x_coda_open, "CODA_OPEN", 1, 1, 0},
        {x_coda_open_as, "CODA_OPEN_AS", 4, 4, 0},
        {x_coda_product_class, "CODA_PRODUCT_CLASS", 1, 1, 0},
        {x_coda_product_type, "CODA_PRODUCT_TYPE", 1, 1, 0},
        {x_coda_product_version, "CODA_PRODUCT_VERSION", 1, 1, 0},
        {x_coda_setopt, "CODA_SETOPT", 2, 2, 0},
        {x_coda_size, "CODA_SIZE", 1, IDL_MAXPARAMS, 0},
        {x_coda_time_to_string, "CODA_TIME_TO_STRING", 1, 1, 0},
        {x_coda_unit, "CODA_UNIT", 1, IDL_MAXPARAMS, 0},
        {x_coda_version, "CODA_VERSION", 0, 0, 0}
    };

    /* old-style procedure declarations */

    static IDL_SYSFUN_DEF idl_proc_addr[] = {
        {(IDL_FUN_RET)x_coda_unload, "CODA_UNLOAD", 0, 0, 0}
    };

    return IDL_AddSystemRoutine(idl_func_addr, TRUE, sizeof(idl_func_addr) / sizeof(IDL_SYSFUN_DEF)) &&
        IDL_AddSystemRoutine(idl_proc_addr, FALSE, sizeof(idl_proc_addr) / sizeof(IDL_SYSFUN_DEF));

#endif

}

void init_dlm_state(void)
{
    int i;

    coda_idl_option_filter_record_fields = 1;
    coda_idl_option_verbose = 1;

    unique_id_counter = 0;

    for (i = 0; i < NUM_PF_SLOTS; i++)  /* erase all slots */
    {
        product_slot[i].product = NULL;
        product_slot[i].product_id = 0;
    }
}

int IDL_Load(void)
{
    /* initialize DLM state variables */

    init_dlm_state();

    /* register types, functions, and procedures */

    register_idl_struct_types();
    return register_idl_functions_and_procedures();
}
