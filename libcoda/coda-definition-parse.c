/*
 * Copyright (C) 2007-2011 S[&]T, The Netherlands.
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

#include "coda-internal.h"

#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "expat.h"

#include "ziparchive.h"

#include "coda-ascii.h"
#include "coda-definition.h"
#include "coda-type.h"

#define CODA_DEFINITION_NAMESPACE "http://www.stcorp.nl/coda/definition/2008/07"

#define FORMAT_UNDEFINED 100

typedef struct parser_info_struct parser_info;

typedef int (*init_handler) (parser_info *, const char **attr);
typedef int (*finalise_handler) (parser_info *);
typedef int (*add_element_to_parent_handler) (parser_info *);
typedef void (*free_data_handler) (void *);

typedef enum xml_element_tag_enum
{
    no_element = -1,
    element_cd_array,
    element_cd_ascii_line,
    element_cd_ascii_line_separator,
    element_cd_ascii_white_space,
    element_cd_attribute,
    element_cd_available,
    element_cd_bit_offset,
    element_cd_bit_size,
    element_cd_byte_size,
    element_cd_complex,
    element_cd_conversion,
    element_cd_description,
    element_cd_detection_rule,
    element_cd_dimension,
    element_cd_field,
    element_cd_field_expression,
    element_cd_fixed_value,
    element_cd_float,
    element_cd_hidden,
    element_cd_init,
    element_cd_integer,
    element_cd_little_endian,
    element_cd_mapping,
    element_cd_match_data,
    element_cd_match_filename,
    element_cd_match_size,
    element_cd_named_type,
    element_cd_native_type,
    element_cd_optional,
    element_cd_product_class,
    element_cd_product_definition,
    element_cd_product_type,
    element_cd_product_variable,
    element_cd_raw,
    element_cd_record,
    element_cd_scale_factor,
    element_cd_text,
    element_cd_time,
    element_cd_type,
    element_cd_union,
    element_cd_unit,
    element_cd_vsf_integer
} xml_element_tag;

static const char *xml_full_element_name[] = {
    CODA_DEFINITION_NAMESPACE " Array",
    CODA_DEFINITION_NAMESPACE " AsciiLine",
    CODA_DEFINITION_NAMESPACE " AsciiLineSeparator",
    CODA_DEFINITION_NAMESPACE " AsciiWhiteSpace",
    CODA_DEFINITION_NAMESPACE " Attribute",
    CODA_DEFINITION_NAMESPACE " Available",
    CODA_DEFINITION_NAMESPACE " BitOffset",
    CODA_DEFINITION_NAMESPACE " BitSize",
    CODA_DEFINITION_NAMESPACE " ByteSize",
    CODA_DEFINITION_NAMESPACE " Complex",
    CODA_DEFINITION_NAMESPACE " Conversion",
    CODA_DEFINITION_NAMESPACE " Description",
    CODA_DEFINITION_NAMESPACE " DetectionRule",
    CODA_DEFINITION_NAMESPACE " Dimension",
    CODA_DEFINITION_NAMESPACE " Field",
    CODA_DEFINITION_NAMESPACE " FieldExpression",
    CODA_DEFINITION_NAMESPACE " FixedValue",
    CODA_DEFINITION_NAMESPACE " Float",
    CODA_DEFINITION_NAMESPACE " Hidden",
    CODA_DEFINITION_NAMESPACE " Init",
    CODA_DEFINITION_NAMESPACE " Integer",
    CODA_DEFINITION_NAMESPACE " LittleEndian",
    CODA_DEFINITION_NAMESPACE " Mapping",
    CODA_DEFINITION_NAMESPACE " MatchData",
    CODA_DEFINITION_NAMESPACE " MatchFilename",
    CODA_DEFINITION_NAMESPACE " MatchSize",
    CODA_DEFINITION_NAMESPACE " NamedType",
    CODA_DEFINITION_NAMESPACE " NativeType",
    CODA_DEFINITION_NAMESPACE " Optional",
    CODA_DEFINITION_NAMESPACE " ProductClass",
    CODA_DEFINITION_NAMESPACE " ProductDefinition",
    CODA_DEFINITION_NAMESPACE " ProductType",
    CODA_DEFINITION_NAMESPACE " ProductVariable",
    CODA_DEFINITION_NAMESPACE " Raw",
    CODA_DEFINITION_NAMESPACE " Record",
    CODA_DEFINITION_NAMESPACE " ScaleFactor",
    CODA_DEFINITION_NAMESPACE " Text",
    CODA_DEFINITION_NAMESPACE " Time",
    CODA_DEFINITION_NAMESPACE " Type",
    CODA_DEFINITION_NAMESPACE " Union",
    CODA_DEFINITION_NAMESPACE " Unit",
    CODA_DEFINITION_NAMESPACE " VSFInteger",
};

#define num_xml_elements ((int)(sizeof(xml_full_element_name)/sizeof(char *)))

enum zip_entry_type_enum
{
    ze_index,
    ze_type,
    ze_product
};
typedef enum zip_entry_type_enum zip_entry_type;

struct node_info_struct
{
    xml_element_tag tag;
    int empty;
    void *data;
    char *char_data;
    int64_t integer_data;
    double float_data;
    int expect_char_data;
    finalise_handler finalise_element;
    free_data_handler free_data;

    coda_format format;

    /* handlers for sub elements */
    init_handler init_sub_element[num_xml_elements];
    add_element_to_parent_handler add_element_to_parent[num_xml_elements];

    struct node_info_struct *parent;
};
typedef struct node_info_struct node_info;

struct parser_info_struct
{
    node_info *node;
    XML_Parser parser;
    hashtable *hash_data;
    char *buffer;
    za_file *zf;
    const char *entry_base_name;
    coda_product_class *product_class;
    coda_product_definition *product_definition;
    int product_class_revision;
    int abort_parser;
    int ignore_file;    /* if set on abort, just ignore everything and return success */
    int add_error_location;
    int unparsed_depth; /* keep track of how deep we are in the XML hierarchy within 'unparsed' XML elements */
};

static int parse_entry(za_file *zf, zip_entry_type type, const char *name, coda_product_class *current_product_class,
                       coda_product_definition *current_product_definition);

static int dummy_init(parser_info *info, const char **attr);
static int bool_expression_init(parser_info *info, const char **attr);
static int integer_expression_init(parser_info *info, const char **attr);
static int integer_constant_or_expression_init(parser_info *info, const char **attr);
static int optional_integer_constant_or_expression_init(parser_info *info, const char **attr);
static int string_data_init(parser_info *info, const char **attr);
static int void_expression_init(parser_info *info, const char **attr);
static int xml_root_init(parser_info *info, const char **attr);
static int cd_array_init(parser_info *info, const char **attr);
static int cd_ascii_line_init(parser_info *info, const char **attr);
static int cd_ascii_line_separator_init(parser_info *info, const char **attr);
static int cd_ascii_white_space_init(parser_info *info, const char **attr);
static int cd_attribute_init(parser_info *info, const char **attr);
static int cd_complex_init(parser_info *info, const char **attr);
static int cd_conversion_init(parser_info *info, const char **attr);
static int cd_detection_rule_init(parser_info *info, const char **attr);
static int cd_field_init(parser_info *info, const char **attr);
static int cd_float_init(parser_info *info, const char **attr);
static int cd_integer_init(parser_info *info, const char **attr);
static int cd_named_type_init(parser_info *info, const char **attr);
static int cd_native_type_init(parser_info *info, const char **attr);
static int cd_mapping_init(parser_info *info, const char **attr);
static int cd_match_data_init(parser_info *info, const char **attr);
static int cd_match_filename_init(parser_info *info, const char **attr);
static int cd_match_size_init(parser_info *info, const char **attr);
static int cd_product_class_init(parser_info *info, const char **attr);
static int cd_product_definition_init(parser_info *info, const char **attr);
static int cd_product_definition_sub_init(parser_info *info, const char **attr);
static int cd_product_type_init(parser_info *info, const char **attr);
static int cd_product_variable_init(parser_info *info, const char **attr);
static int cd_raw_init(parser_info *info, const char **attr);
static int cd_record_init(parser_info *info, const char **attr);
static int cd_scale_factor_init(parser_info *info, const char **attr);
static int cd_text_init(parser_info *info, const char **attr);
static int cd_time_init(parser_info *info, const char **attr);
static int cd_type_init(parser_info *info, const char **attr);
static int cd_union_init(parser_info *info, const char **attr);
static int cd_vsf_integer_init(parser_info *info, const char **attr);

static const char *xml_element_name(xml_element_tag tag)
{
    const char *str;

    if (tag == no_element)
    {
        return "--none--";
    }

    str = xml_full_element_name[tag];
    while (*str != ' ')
    {
        str++;
    }
    str++;
    return str;
}

static void handle_ziparchive_error(const char *message, ...)
{
    va_list ap;

    coda_set_error(CODA_ERROR_DATA_DEFINITION, "could not read data from definition file: ");

    va_start(ap, message);
    coda_add_error_message_vargs(message, ap);
    va_end(ap);
}

static int is_whitespace(const char *s, int len)
{
    int i;

    assert(s != NULL);

    for (i = 0; i < len; i++)
    {
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
        {
            return 0;
        }
    }

    return 1;
}

static int decode_escaped_string(char *str)
{
    int from;
    int to;

    if (str == NULL)
    {
        return 0;
    }

    from = 0;
    to = 0;

    while (str[from] != '\0')
    {
        if (str[from] == '\\')
        {
            from++;
            switch (str[from])
            {
                case 'e':
                    str[to++] = '\033'; /* windows does not recognize '\e' */
                    break;
                case 'a':
                    str[to++] = '\a';
                    break;
                case 'b':
                    str[to++] = '\b';
                    break;
                case 'f':
                    str[to++] = '\f';
                    break;
                case 'n':
                    str[to++] = '\n';
                    break;
                case 'r':
                    str[to++] = '\r';
                    break;
                case 't':
                    str[to++] = '\t';
                    break;
                case 'v':
                    str[to++] = '\v';
                    break;
                case '\\':
                    str[to++] = '\\';
                    break;
                default:
                    if (str[from] < '0' || str[from] > '9')
                    {
                        return -1;
                    }
                    str[to] = str[from] - '0';
                    if (str[from + 1] >= '0' && str[from + 1] <= '9')
                    {
                        from++;
                        str[to] = str[to] * 8 + (str[from] - '0');
                        if (str[from + 1] >= '0' && str[from + 1] <= '9')
                        {
                            from++;
                            str[to] = str[to] * 8 + (str[from] - '0');
                        }
                    }
                    to++;
            }
        }
        else
        {
            str[to++] = str[from];
        }
        from++;
    }

    str[to] = '\0';

    return to;
}

static int decode_xml_string(char *str)
{
    int from;
    int to;

    if (str == NULL)
    {
        return 0;
    }

    from = 0;
    to = 0;

    while (str[from] != '\0')
    {
        if (str[from] == '&')
        {
            if (strncmp(&str[from + 1], "amp;", 4) == 0)
            {
                str[to++] = '&';
                from += 5;
            }
            else if (strncmp(&str[from + 1], "apos;", 5) == 0)
            {
                str[to++] = '\'';
                from += 6;
            }
            else if (strncmp(&str[from + 1], "lt;", 3) == 0)
            {
                str[to++] = '<';
                from += 4;
            }
            else if (strncmp(&str[from + 1], "gt;", 3) == 0)
            {
                str[to++] = '<';
                from += 4;
            }
            else if (strncmp(&str[from + 1], "quot;", 5) == 0)
            {
                str[to++] = '"';
                from += 6;
            }
            else
            {
                /* just keep entity */
                str[to++] = str[from];
                from++;
            }
        }
        else
        {
            str[to++] = str[from];
            from++;
        }
    }

    str[to] = '\0';

    return to;
}

static int format_from_string(const char *str, coda_format *format)
{
    if (strcmp(str, "ascii") == 0)
    {
        *format = coda_format_ascii;
    }
    else if (strcmp(str, "binary") == 0)
    {
        *format = coda_format_binary;
    }
    else if (strcmp(str, "xml") == 0)
    {
        *format = coda_format_xml;
    }
    else if (strcmp(str, "hdf4") == 0)
    {
        *format = coda_format_hdf4;
    }
    else if (strcmp(str, "hdf5") == 0)
    {
        *format = coda_format_hdf5;
    }
    else if (strcmp(str, "netcdf") == 0)
    {
        *format = coda_format_netcdf;
    }
    else if (strcmp(str, "grib1") == 0)
    {
        *format = coda_format_grib1;
    }
    else if (strcmp(str, "grib2") == 0)
    {
        *format = coda_format_grib2;
    }
    else if (strcmp(str, "rinex") == 0)
    {
        *format = coda_format_rinex;
    }
    else if (strcmp(str, "sp3") == 0)
    {
        *format = coda_format_sp3;
    }
    else
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'format' attribute value '%s'", str);
        return -1;
    }

    return 0;
}

static const char *get_attribute_value(const char **attr, const char *name)
{
    while (*attr != NULL)
    {
        if (strcmp(attr[0], name) == 0)
        {
            return attr[1];
        }
        attr = &attr[2];
    }
    return NULL;
}

static const char *get_mandatory_attribute_value(const char **attr, const char *name, xml_element_tag tag)
{
    const char *value;

    value = get_attribute_value(attr, name);
    if (value == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "mandatory attribute '%s' missing for element '%s'", name,
                       xml_element_name(tag));
    }
    return value;
}

static int handle_name_attribute_for_type(parser_info *info, const char **attr)
{
    const char *name;

    if (info->node->parent->parent == NULL)
    {
        /* this is a top-level type, we require a name */
        name = get_mandatory_attribute_value(attr, "name", info->node->tag);
        if (name == NULL)
        {
            return -1;
        }
        /* verify that the name matches the filename */
        if (strcmp(name, info->entry_base_name) != 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "definition for named type '%s' has incorrect 'name' attribute", info->entry_base_name);
            return -1;
        }
        if (coda_type_set_name((coda_type *)info->node->data, name) != 0)
        {
            return -1;
        }
    }
    else
    {
        name = get_attribute_value(attr, "name");
        if (name != NULL)
        {
            /* we have a named type that is not a top-level type - this is not allowed */
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "type may not have a 'name' attribute unless it is a top level element");
            return -1;
        }
    }

    return 0;
}

static int handle_format_attribute_for_type(parser_info *info, const char **attr)
{
    const char *format_string;

    if (info->node->parent->format == FORMAT_UNDEFINED)
    {
        /* we can't inherit the format of the parent, so a format attribute is required */
        format_string = get_mandatory_attribute_value(attr, "format", info->node->tag);
        if (format_string == NULL)
        {
            return -1;
        }
        if (format_from_string(format_string, &info->node->format) != 0)
        {
            return -1;
        }
    }
    else
    {
        format_string = get_attribute_value(attr, "format");
        if (format_string == NULL)
        {
            /* no format attribute available -> inherit format from parent */
            info->node->format = info->node->parent->format;
        }
        else
        {
            if (format_from_string(format_string, &info->node->format) != 0)
            {
                return -1;
            }
        }
    }

    return 0;
}

static int handle_xml_name(parser_info *info, const char **attr)
{
    const char *xmlname;
    node_info *node;

    if (info->node->format != coda_format_xml)
    {
        return 0;
    }
    xmlname = get_mandatory_attribute_value(attr, "namexml", info->node->tag);
    if (xmlname == NULL)
    {
        return -1;
    }
    node = info->node->parent;
    while (node->tag != element_cd_field)
    {
        if (node->tag == no_element)
        {
            return 0;
        }
        node = node->parent;
    }
    if (coda_type_record_field_set_real_name((coda_type_record_field *)node->data, xmlname) != 0)
    {
        return -1;
    }
    return 0;
}

static int get_named_type(parser_info *info, const char *id, coda_type **type)
{
    assert(info->product_class != NULL);
    if (!coda_product_class_has_named_type(info->product_class, id))
    {
        if (parse_entry(info->zf, ze_type, id, info->product_class, info->product_definition) != 0)
        {
            info->add_error_location = 0;
            return -1;
        }
    }
    *type = coda_product_class_get_named_type(info->product_class, id);
    (*type)->retain_count++;
    return 0;
}

static void abort_parser(parser_info *info)
{
    XML_StopParser(info->parser, 0);
    /* we need to explicitly check in the end handlers for parsing abort because expat may still call the end handler
     * after an abort in the start handler */
    info->abort_parser = 1;
}

static void register_sub_element(node_info *node, xml_element_tag tag, init_handler init_sub_element,
                                 add_element_to_parent_handler add_element_to_parent)
{
    assert(init_sub_element != NULL);
    node->init_sub_element[tag] = init_sub_element;
    node->add_element_to_parent[tag] = add_element_to_parent;
}

static void register_type_elements(node_info *node, add_element_to_parent_handler add_element_to_parent)
{
    register_sub_element(node, element_cd_ascii_line, cd_ascii_line_init, add_element_to_parent);
    register_sub_element(node, element_cd_ascii_line_separator, cd_ascii_line_separator_init, add_element_to_parent);
    register_sub_element(node, element_cd_ascii_white_space, cd_ascii_white_space_init, add_element_to_parent);
    register_sub_element(node, element_cd_array, cd_array_init, add_element_to_parent);
    register_sub_element(node, element_cd_complex, cd_complex_init, add_element_to_parent);
    register_sub_element(node, element_cd_float, cd_float_init, add_element_to_parent);
    register_sub_element(node, element_cd_integer, cd_integer_init, add_element_to_parent);
    register_sub_element(node, element_cd_named_type, cd_named_type_init, add_element_to_parent);
    register_sub_element(node, element_cd_raw, cd_raw_init, add_element_to_parent);
    register_sub_element(node, element_cd_record, cd_record_init, add_element_to_parent);
    register_sub_element(node, element_cd_text, cd_text_init, add_element_to_parent);
    register_sub_element(node, element_cd_time, cd_time_init, add_element_to_parent);
    register_sub_element(node, element_cd_type, cd_type_init, add_element_to_parent);
    register_sub_element(node, element_cd_union, cd_union_init, add_element_to_parent);
    register_sub_element(node, element_cd_vsf_integer, cd_vsf_integer_init, add_element_to_parent);
}

static int data_dictionary_add_product_class(parser_info *info)
{
    if (coda_data_dictionary_add_product_class((coda_product_class *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static void dummy_free_handler(void *data)
{
    data = data;        /* do nothing */
}

static int dummy_init(parser_info *info, const char **attr)
{
    info = info;
    attr = attr;
    return 0;
}

static int bool_expression_finalise(parser_info *info)
{
    coda_expression_type result_type;
    coda_expression *expr;

    if (info->node->char_data != NULL)
    {
        if (is_whitespace(info->node->char_data, strlen(info->node->char_data)))
        {
            free(info->node->char_data);
            info->node->char_data = NULL;
        }
    }
    if (info->node->char_data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "empty boolean expression");
        return -1;
    }
    if (coda_expression_from_string(info->node->char_data, &expr) != 0)
    {
        return -1;
    }
    free(info->node->char_data);
    info->node->char_data = NULL;
    info->node->data = expr;
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        return -1;
    }
    if (result_type != coda_expression_boolean)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "not a boolean expression");
        return -1;
    }

    return 0;
}

static int bool_expression_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->expect_char_data = 1;
    info->node->free_data = (free_data_handler)coda_expression_delete;
    info->node->finalise_element = bool_expression_finalise;

    return 0;
}

static int integer_expression_finalise(parser_info *info)
{
    coda_expression_type result_type;
    coda_expression *expr;

    if (info->node->char_data != NULL)
    {
        if (is_whitespace(info->node->char_data, strlen(info->node->char_data)))
        {
            free(info->node->char_data);
            info->node->char_data = NULL;
        }
    }
    if (info->node->char_data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "empty integer expression");
        return -1;
    }
    if (coda_expression_from_string(info->node->char_data, &expr) != 0)
    {
        return -1;
    }
    free(info->node->char_data);
    info->node->char_data = NULL;
    info->node->data = expr;
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        return -1;
    }
    if (result_type != coda_expression_integer)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "not an integer expression");
        return -1;
    }

    return 0;
}

static int integer_expression_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->expect_char_data = 1;
    info->node->free_data = (free_data_handler)coda_expression_delete;
    info->node->finalise_element = integer_expression_finalise;

    return 0;
}

static int integer_constant_or_expression_finalise(parser_info *info)
{
    coda_expression_type result_type;
    coda_expression *expr;

    if (info->node->char_data != NULL)
    {
        if (is_whitespace(info->node->char_data, strlen(info->node->char_data)))
        {
            free(info->node->char_data);
            info->node->char_data = NULL;
        }
    }
    if (info->node->char_data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "empty integer expression");
        return -1;
    }
    if (coda_expression_from_string(info->node->char_data, &expr) != 0)
    {
        return -1;
    }
    free(info->node->char_data);
    info->node->char_data = NULL;
    info->node->data = expr;
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        coda_expression_delete(expr);
        return -1;
    }
    if (result_type != coda_expression_integer)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "not an integer expression");
        return -1;
    }
    if (coda_expression_is_constant(expr))
    {
        /* it is a constant value */
        if (coda_expression_eval_integer(expr, NULL, &info->node->integer_data) != 0)
        {
            return -1;
        }
        info->node->data = NULL;
        coda_expression_delete(expr);
    }

    return 0;
}

static int integer_constant_or_expression_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->expect_char_data = 1;
    info->node->free_data = (free_data_handler)coda_expression_delete;
    info->node->finalise_element = integer_constant_or_expression_finalise;

    return 0;
}

static int optional_integer_constant_or_expression_finalise(parser_info *info)
{
    coda_expression_type result_type;
    coda_expression *expr;

    if (info->node->char_data != NULL)
    {
        if (is_whitespace(info->node->char_data, strlen(info->node->char_data)))
        {
            free(info->node->char_data);
            info->node->char_data = NULL;
        }
    }
    if (info->node->char_data == NULL)
    {
        /* no integer or expression provided */
        info->node->empty = 1;
        return 0;
    }
    if (coda_expression_from_string(info->node->char_data, &expr) != 0)
    {
        return -1;
    }
    free(info->node->char_data);
    info->node->char_data = NULL;
    info->node->data = expr;
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        coda_expression_delete(expr);
        return -1;
    }
    if (result_type != coda_expression_integer)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "not an integer expression");
        return -1;
    }
    if (coda_expression_is_constant(expr))
    {
        /* it is a constant value */
        if (coda_expression_eval_integer(expr, NULL, &info->node->integer_data) != 0)
        {
            return -1;
        }
        info->node->data = NULL;
        coda_expression_delete(expr);
    }

    return 0;
}

static int optional_integer_constant_or_expression_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->expect_char_data = 1;
    info->node->free_data = (free_data_handler)coda_expression_delete;
    info->node->finalise_element = optional_integer_constant_or_expression_finalise;

    return 0;
}

static int product_class_add_named_type(parser_info *info)
{
    assert(info->product_class != NULL);
    if (coda_product_class_add_named_type(info->product_class, (coda_type *)info->node->data) != 0)
    {
        return -1;
    }
    return 0;
}

static int string_data_finalise(parser_info *info)
{
    decode_xml_string(info->node->char_data);
    return 0;
}

static int string_data_init(parser_info *info, const char **attr)
{
    attr = attr;
    info->node->expect_char_data = 1;
    info->node->finalise_element = string_data_finalise;
    return 0;
}

static int type_set_description(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_type_set_description((coda_type *)info->node->parent->data, "");
    }
    return coda_type_set_description((coda_type *)info->node->parent->data, info->node->char_data);
}

static int type_set_bit_size(parser_info *info)
{
    if (info->node->data != NULL)
    {
        if (coda_type_set_bit_size_expression((coda_type *)info->node->parent->data,
                                              (coda_expression *)info->node->data) != 0)
        {
            return -1;
        }
        info->node->data = NULL;
    }
    else
    {
        if (coda_type_set_bit_size((coda_type *)info->node->parent->data, (long)info->node->integer_data) != 0)
        {
            return -1;
        }
    }
    return 0;
}

static int type_set_byte_size(parser_info *info)
{
    if (info->node->data != NULL)
    {
        if (coda_type_set_byte_size_expression((coda_type *)info->node->parent->data,
                                               (coda_expression *)info->node->data) != 0)
        {
            return -1;
        }
        info->node->data = NULL;
    }
    else
    {
        if (coda_type_set_byte_size((coda_type *)info->node->parent->data, (long)info->node->integer_data) != 0)
        {
            return -1;
        }
    }
    return 0;
}

static int void_expression_finalise(parser_info *info)
{
    coda_expression_type result_type;
    coda_expression *expr;

    if (info->node->char_data != NULL)
    {
        if (is_whitespace(info->node->char_data, strlen(info->node->char_data)))
        {
            free(info->node->char_data);
            info->node->char_data = NULL;
        }
    }
    if (info->node->char_data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "empty void expression");
        return -1;
    }
    if (coda_expression_from_string(info->node->char_data, &expr) != 0)
    {
        return -1;
    }
    free(info->node->char_data);
    info->node->char_data = NULL;
    info->node->data = expr;
    if (coda_expression_get_type(expr, &result_type) != 0)
    {
        return -1;
    }
    if (result_type != coda_expression_void)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "not a void expression");
        return -1;
    }

    return 0;
}

static int void_expression_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->expect_char_data = 1;
    info->node->free_data = (free_data_handler)coda_expression_delete;
    info->node->finalise_element = void_expression_finalise;

    return 0;
}

static int type_add_attribute(parser_info *info)
{
    if (coda_type_add_attribute((coda_type *)info->node->parent->data, (coda_type_record_field *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int xml_root_set_field(parser_info *info)
{
    if (coda_type_record_add_field((coda_type_record *)info->node->parent->data,
                                   (coda_type_record_field *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int xml_root_init(parser_info *info, const char **attr)
{
    const char *name = NULL;

    assert(info->product_definition != NULL);
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    if (info->node->format != coda_format_xml)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "%s record not allowed for xml product definition %s",
                       coda_type_get_format_name(info->node->format), info->product_definition->name);
        return -1;
    }
    name = get_attribute_value(attr, "name");
    if (name != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'name' not allowed for xml root record");
        return -1;
    }
    name = get_attribute_value(attr, "namexml");
    if (name != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'namexml' not allowed for xml root record");
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_record_new(coda_format_xml);

    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_field, cd_field_init, xml_root_set_field);

    return 0;
}

static int cd_array_set_type(parser_info *info)
{
    return coda_type_array_set_base_type((coda_type_array *)info->node->parent->data, (coda_type *)info->node->data);
}

static int cd_array_add_dimension(parser_info *info)
{
    if (info->node->data != NULL || info->node->empty)
    {
        if (coda_type_array_add_variable_dimension((coda_type_array *)info->node->parent->data,
                                                   (coda_expression *)info->node->data) != 0)
        {
            return -1;
        }
        info->node->data = NULL;
    }
    else
    {
        if (coda_type_array_add_fixed_dimension((coda_type_array *)info->node->parent->data,
                                                (long)info->node->integer_data) != 0)
        {
            return -1;
        }
    }
    return 0;
}

static int cd_array_finalise(parser_info *info)
{
    return coda_type_array_validate((coda_type_array *)info->node->data);
}

static int cd_array_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_array_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_type_elements(info->node, cd_array_set_type);
    register_sub_element(info->node, element_cd_dimension, optional_integer_constant_or_expression_init,
                         cd_array_add_dimension);
    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    info->node->finalise_element = cd_array_finalise;

    return 0;
}

static int cd_ascii_line_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_text_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (coda_type_text_set_special_text_type((coda_type_text *)info->node->data, ascii_text_line_without_eol) != 0)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);

    return 0;
}

static int cd_ascii_line_separator_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_text_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (coda_type_text_set_special_text_type((coda_type_text *)info->node->data, ascii_text_line_separator) != 0)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);

    return 0;
}

static int cd_ascii_white_space_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_text_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (coda_type_text_set_special_text_type((coda_type_text *)info->node->data, ascii_text_whitespace) != 0)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);

    return 0;
}

static int cd_attribute_set_optional(parser_info *info)
{
    return coda_type_record_field_set_optional((coda_type_record_field *)info->node->parent->data);
}

static int cd_attribute_set_fixed_value(parser_info *info)
{
    coda_type *type;

    if (decode_escaped_string(info->node->char_data) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid escape sequence in string");
        return -1;
    }
    if (coda_type_record_field_get_type((coda_type_record_field *)info->node->parent->data, &type) != 0)
    {
        return -1;
    }
    if (coda_type_text_set_fixed_value((coda_type_text *)type, info->node->char_data) != 0)
    {
        return -1;
    }
    if (coda_type_set_byte_size(type, strlen(info->node->char_data)) != 0)
    {
        return -1;
    }
    return 0;
}

static int cd_attribute_init(parser_info *info, const char **attr)
{
    char *name;
    const char *xmlname;
    coda_type *type;

    info->node->format = info->node->parent->format;
    xmlname = get_mandatory_attribute_value(attr, "name", info->node->tag);
    if (xmlname == NULL)
    {
        return -1;
    }
    name = coda_identifier_from_name(coda_element_name_from_xml_name(xmlname), NULL);
    if (name == NULL)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_record_field_delete;
    info->node->data = coda_type_record_field_new(name);
    free(name);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (coda_type_record_field_set_real_name((coda_type_record_field *)info->node->data, xmlname) != 0)
    {
        return -1;
    }
    type = (coda_type *)coda_type_text_new(info->node->format);
    if (type == NULL)
    {
        return -1;
    }
    if (coda_type_record_field_set_type((coda_type_record_field *)info->node->data, type) != 0)
    {
        coda_type_release(type);
        return -1;
    }
    coda_type_release(type);

    register_sub_element(info->node, element_cd_optional, dummy_init, cd_attribute_set_optional);
    register_sub_element(info->node, element_cd_fixed_value, string_data_init, cd_attribute_set_fixed_value);

    return 0;
}

static int cd_complex_set_type(parser_info *info)
{
    return coda_type_complex_set_type((coda_type_special *)info->node->parent->data, (coda_type *)info->node->data);
}

static int cd_complex_finalise(parser_info *info)
{
    return coda_type_complex_validate((coda_type_special *)info->node->data);
}

static int cd_complex_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_complex_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_float, cd_float_init, cd_complex_set_type);
    register_sub_element(info->node, element_cd_integer, cd_integer_init, cd_complex_set_type);
    info->node->finalise_element = cd_complex_finalise;

    return 0;
}

static int cd_conversion_set_unit(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_conversion_set_unit((coda_conversion *)info->node->parent->data, "");
    }
    return coda_conversion_set_unit((coda_conversion *)info->node->parent->data, info->node->char_data);
}

static int cd_conversion_init(parser_info *info, const char **attr)
{
    const char *numerator_string;
    const char *denominator_string;
    double numerator;
    double denominator;

    numerator_string = get_mandatory_attribute_value(attr, "numerator", info->node->tag);
    if (numerator_string == NULL)
    {
        return -1;
    }
    denominator_string = get_mandatory_attribute_value(attr, "denominator", info->node->tag);
    if (denominator_string == NULL)
    {
        return -1;
    }
    if (coda_ascii_parse_double(numerator_string, strlen(numerator_string), &numerator, 1) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid value '%s' for numerator", numerator_string);
        return -1;
    }
    if (coda_ascii_parse_double(denominator_string, strlen(denominator_string), &denominator, 1) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid value '%s' for denominator", denominator_string);
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_conversion_delete;
    info->node->data = coda_conversion_new(numerator, denominator, 0, coda_NaN());
    register_sub_element(info->node, element_cd_unit, string_data_init, cd_conversion_set_unit);

    return 0;
}

static int cd_detection_rule_add_entry(parser_info *info)
{
    if (coda_detection_rule_add_entry((coda_detection_rule *)info->node->parent->data,
                                      (coda_detection_rule_entry *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_detection_rule_init(parser_info *info, const char **attr)
{
    attr = attr;

    info->node->free_data = (free_data_handler)coda_detection_rule_delete;
    info->node->data = coda_detection_rule_new();
    if (info->node->data == NULL)
    {
        return -1;
    }
    register_sub_element(info->node, element_cd_match_data, cd_match_data_init, cd_detection_rule_add_entry);
    register_sub_element(info->node, element_cd_match_filename, cd_match_filename_init, cd_detection_rule_add_entry);
    register_sub_element(info->node, element_cd_match_size, cd_match_size_init, cd_detection_rule_add_entry);

    return 0;
}

static int cd_field_set_type(parser_info *info)
{
    return coda_type_record_field_set_type((coda_type_record_field *)info->node->parent->data,
                                           (coda_type *)info->node->data);
}

static int cd_field_set_hidden(parser_info *info)
{
    return coda_type_record_field_set_hidden((coda_type_record_field *)info->node->parent->data);
}

static int cd_field_set_optional(parser_info *info)
{
    return coda_type_record_field_set_optional((coda_type_record_field *)info->node->parent->data);
}

static int cd_field_set_available(parser_info *info)
{
    if (coda_type_record_field_set_available_expression((coda_type_record_field *)info->node->parent->data,
                                                        (coda_expression *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_field_set_bit_offset(parser_info *info)
{
    if (coda_type_record_field_set_bit_offset_expression((coda_type_record_field *)info->node->parent->data,
                                                         (coda_expression *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_field_finalise(parser_info *info)
{
    return coda_type_record_field_validate((coda_type_record_field *)info->node->data);
}

static int cd_field_init(parser_info *info, const char **attr)
{
    const char *format = NULL;
    const char *name = NULL;

    format = get_attribute_value(attr, "format");
    if (format != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'format' not allowed for Field");
        return -1;
    }
    info->node->format = info->node->parent->format;
    if (get_attribute_value(attr, "namexml") != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'namexml' not allowed for Field");
        return -1;
    }
    name = get_mandatory_attribute_value(attr, "name", info->node->tag);
    if (name == NULL)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_record_field_delete;
    info->node->data = coda_type_record_field_new(name);
    if (info->node->data == NULL)
    {
        return -1;
    }

    register_type_elements(info->node, cd_field_set_type);
    register_sub_element(info->node, element_cd_hidden, dummy_init, cd_field_set_hidden);
    register_sub_element(info->node, element_cd_optional, dummy_init, cd_field_set_optional);
    register_sub_element(info->node, element_cd_available, bool_expression_init, cd_field_set_available);
    register_sub_element(info->node, element_cd_bit_offset, integer_expression_init, cd_field_set_bit_offset);
    info->node->finalise_element = cd_field_finalise;

    return 0;
}

static int cd_float_set_unit(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_type_number_set_unit((coda_type_number *)info->node->parent->data, "");
    }
    return coda_type_number_set_unit((coda_type_number *)info->node->parent->data, info->node->char_data);
}

static int cd_float_set_read_type(parser_info *info)
{
    return coda_type_set_read_type((coda_type *)info->node->parent->data, (int)info->node->integer_data);
}

static int cd_float_set_conversion(parser_info *info)
{
    if (coda_type_number_set_conversion((coda_type_number *)info->node->parent->data,
                                        (coda_conversion *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_float_set_little_endian(parser_info *info)
{
    return coda_type_number_set_endianness((coda_type_number *)info->node->parent->data, coda_little_endian);
}

static int cd_float_add_mapping(parser_info *info)
{
    if (coda_type_number_add_ascii_float_mapping((coda_type_number *)info->node->parent->data,
                                                 (coda_ascii_float_mapping *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_float_finalise(parser_info *info)
{
    return coda_type_number_validate((coda_type_number *)info->node->data);
}

static int cd_float_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_number_new(info->node->format, coda_real_class);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_unit, string_data_init, cd_float_set_unit);
    register_sub_element(info->node, element_cd_native_type, cd_native_type_init, cd_float_set_read_type);
    register_sub_element(info->node, element_cd_conversion, cd_conversion_init, cd_float_set_conversion);
    register_sub_element(info->node, element_cd_bit_size, integer_constant_or_expression_init, type_set_bit_size);
    register_sub_element(info->node, element_cd_byte_size, integer_constant_or_expression_init, type_set_byte_size);
    register_sub_element(info->node, element_cd_little_endian, dummy_init, cd_float_set_little_endian);
    register_sub_element(info->node, element_cd_mapping, cd_mapping_init, cd_float_add_mapping);
    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    info->node->finalise_element = cd_float_finalise;

    return 0;
}

static int cd_integer_set_unit(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_type_number_set_unit((coda_type_number *)info->node->parent->data, "");
    }
    return coda_type_number_set_unit((coda_type_number *)info->node->parent->data, info->node->char_data);
}

static int cd_integer_set_read_type(parser_info *info)
{
    return coda_type_set_read_type((coda_type *)info->node->parent->data, (int)info->node->integer_data);
}

static int cd_integer_set_conversion(parser_info *info)
{
    if (coda_type_number_set_conversion((coda_type_number *)info->node->parent->data,
                                        (coda_conversion *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_integer_set_little_endian(parser_info *info)
{
    return coda_type_number_set_endianness((coda_type_number *)info->node->parent->data, coda_little_endian);
}

static int cd_integer_add_mapping(parser_info *info)
{
    if (coda_type_number_add_ascii_integer_mapping((coda_type_number *)info->node->parent->data,
                                                   (coda_ascii_integer_mapping *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_integer_finalise(parser_info *info)
{
    return coda_type_number_validate((coda_type_number *)info->node->data);
}

static int cd_integer_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_number_new(info->node->format, coda_integer_class);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_unit, string_data_init, cd_integer_set_unit);
    register_sub_element(info->node, element_cd_byte_size, integer_constant_or_expression_init, type_set_byte_size);
    register_sub_element(info->node, element_cd_bit_size, integer_constant_or_expression_init, type_set_bit_size);
    register_sub_element(info->node, element_cd_little_endian, dummy_init, cd_integer_set_little_endian);
    register_sub_element(info->node, element_cd_native_type, cd_native_type_init, cd_integer_set_read_type);
    register_sub_element(info->node, element_cd_conversion, cd_conversion_init, cd_integer_set_conversion);
    register_sub_element(info->node, element_cd_mapping, cd_mapping_init, cd_integer_add_mapping);
    info->node->finalise_element = cd_integer_finalise;

    return 0;
}

static int cd_named_type_init(parser_info *info, const char **attr)
{
    coda_type *type;
    const char *id;

    id = get_mandatory_attribute_value(attr, "id", info->node->tag);
    if (id == NULL)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    assert(info->product_class != NULL);
    if (get_named_type(info, id, &type) != 0)
    {
        return -1;
    }
    info->node->format = type->format;
    info->node->data = type;

    return 0;
}

static int cd_native_type_finalise(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid native type");
        return -1;
    }

    if (strcmp(info->node->char_data, "int8") == 0)
    {
        info->node->integer_data = coda_native_type_int8;
    }
    else if (strcmp(info->node->char_data, "int16") == 0)
    {
        info->node->integer_data = coda_native_type_int16;
    }
    else if (strcmp(info->node->char_data, "int32") == 0)
    {
        info->node->integer_data = coda_native_type_int32;
    }
    else if (strcmp(info->node->char_data, "int64") == 0)
    {
        info->node->integer_data = coda_native_type_int64;
    }
    else if (strcmp(info->node->char_data, "uint8") == 0)
    {
        info->node->integer_data = coda_native_type_uint8;
    }
    else if (strcmp(info->node->char_data, "uint16") == 0)
    {
        info->node->integer_data = coda_native_type_uint16;
    }
    else if (strcmp(info->node->char_data, "uint32") == 0)
    {
        info->node->integer_data = coda_native_type_uint32;
    }
    else if (strcmp(info->node->char_data, "uint64") == 0)
    {
        info->node->integer_data = coda_native_type_uint64;
    }
    else if (strcmp(info->node->char_data, "float") == 0)
    {
        info->node->integer_data = coda_native_type_float;
    }
    else if (strcmp(info->node->char_data, "double") == 0)
    {
        info->node->integer_data = coda_native_type_double;
    }
    else if (strcmp(info->node->char_data, "char") == 0)
    {
        info->node->integer_data = coda_native_type_char;
    }
    else if (strcmp(info->node->char_data, "string") == 0)
    {
        info->node->integer_data = coda_native_type_string;
    }
    else if (strcmp(info->node->char_data, "bytes") == 0)
    {
        info->node->integer_data = coda_native_type_bytes;
    }
    else
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid native type");
        return -1;
    }

    return 0;
}

static int cd_native_type_init(parser_info *info, const char **attr)
{
    attr = attr;
    info->node->expect_char_data = 1;
    info->node->finalise_element = cd_native_type_finalise;
    return 0;
}

static int cd_mapping_init(parser_info *info, const char **attr)
{
    const char *ascii_string;
    const char *value_string;

    ascii_string = get_mandatory_attribute_value(attr, "string", info->node->tag);
    if (ascii_string == NULL)
    {
        return -1;
    }
    value_string = get_mandatory_attribute_value(attr, "value", info->node->tag);
    if (value_string == NULL)
    {
        return -1;
    }
    if (info->node->parent->tag == element_cd_integer)
    {
        int64_t value;

        if (coda_ascii_parse_int64(value_string, strlen(value_string), &value, 0) < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'value' attribute integer value '%s'", value_string);
            return -1;
        }
        info->node->free_data = (free_data_handler)coda_ascii_integer_mapping_delete;
        info->node->data = coda_ascii_integer_mapping_new(ascii_string, value);
        if (info->node->data == NULL)
        {
            return -1;
        }
    }
    else if (info->node->parent->tag == element_cd_float || info->node->parent->tag == element_cd_time)
    {
        double value;

        if (strcasecmp(value_string, "nan") == 0)
        {
            value = coda_NaN();
        }
        else if (strcasecmp(value_string, "inf") == 0 || strcasecmp(value_string, "+inf") == 0)
        {
            value = coda_PlusInf();
        }
        else if (strcasecmp(value_string, "-inf") == 0)
        {
            value = coda_MinInf();
        }
        else if (sscanf(value_string, "%lf", &value) != 1)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'value' attribute float value '%s'", value_string);
            return -1;
        }
        info->node->free_data = (free_data_handler)coda_ascii_float_mapping_delete;
        info->node->data = coda_ascii_float_mapping_new(ascii_string, value);
        if (info->node->data == NULL)
        {
            return -1;
        }
    }
    else
    {
        assert(0);
        exit(1);
    }

    return 0;
}

static int cd_match_data_finalise(parser_info *info)
{
    long value_length;

    value_length = decode_escaped_string(info->node->char_data);
    if (value_length < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid escape sequence in string");
        return -1;
    }
    if (value_length > 0)
    {
        if (coda_detection_rule_entry_set_value(info->node->data, info->node->char_data, value_length) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int cd_match_data_init(parser_info *info, const char **attr)
{
    const char *offset_string;
    const char *path;
    int64_t offset = -1;

    offset_string = get_attribute_value(attr, "offset");
    if (offset_string != NULL)
    {
        if (coda_ascii_parse_int64(offset_string, strlen(offset_string), &offset, 0) < 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'offset' attribute value '%s'", offset_string);
            return -1;
        }
    }
    path = get_attribute_value(attr, "path");

    info->node->free_data = (free_data_handler)coda_detection_rule_entry_delete;
    if (offset == -1)
    {
        info->node->data = coda_detection_rule_entry_with_path_new(path);
    }
    else
    {
        info->node->data = coda_detection_rule_entry_with_offset_new(offset, 0);
    }
    if (info->node->data == NULL)
    {
        return -1;
    }

    info->node->expect_char_data = 1;
    info->node->finalise_element = cd_match_data_finalise;

    return 0;
}

static int cd_match_filename_finalise(parser_info *info)
{
    long value_length;

    value_length = decode_escaped_string(info->node->char_data);
    if (value_length < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid escape sequence in string");
        return -1;
    }
    if (value_length > 0)
    {
        if (coda_detection_rule_entry_set_value(info->node->data, info->node->char_data, value_length) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int cd_match_filename_init(parser_info *info, const char **attr)
{
    const char *offset_string;
    int64_t offset;

    offset_string = get_mandatory_attribute_value(attr, "offset", info->node->tag);
    if (offset_string == NULL)
    {
        return -1;
    }
    if (coda_ascii_parse_int64(offset_string, strlen(offset_string), &offset, 0) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'offset' attribute value '%s'", offset_string);
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_detection_rule_entry_delete;
    info->node->data = coda_detection_rule_entry_with_offset_new(offset, 1);
    if (info->node->data == NULL)
    {
        return -1;
    }

    info->node->expect_char_data = 1;
    info->node->finalise_element = cd_match_filename_finalise;

    return 0;
}

static int cd_match_size_init(parser_info *info, const char **attr)
{
    const char *size_string;
    int64_t size;

    size_string = get_mandatory_attribute_value(attr, "size", info->node->tag);
    if (size_string == NULL)
    {
        return -1;
    }
    if (coda_ascii_parse_int64(size_string, strlen(size_string), &size, 0) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'size' attribute value '%s'", size_string);
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_detection_rule_entry_delete;
    info->node->data = coda_detection_rule_entry_with_size_new(size);
    if (info->node->data == NULL)
    {
        return -1;
    }

    return 0;
}

static int cd_product_class_set_description(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_product_class_set_description((coda_product_class *)info->node->parent->data, "");
    }
    return coda_product_class_set_description((coda_product_class *)info->node->parent->data, info->node->char_data);
}

static int cd_product_class_add_product_type(parser_info *info)
{
    if (coda_product_class_add_product_type((coda_product_class *)info->node->parent->data,
                                            (coda_product_type *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_class_finalise(parser_info *info)
{
    info->product_class = NULL;
    return 0;
}

static int get_product_class_revision(parser_info *info, int *revision)
{
    za_entry *entry;
    char *buffer;
    long filesize;
    int64_t value;

    entry = za_get_entry_by_name(info->zf, "VERSION");
    if (entry == NULL)
    {
        /* no version number available -> use revision number 0 */
        *revision = 0;
        return 0;
    }
    filesize = za_get_entry_size(entry);
    if (filesize == 0)
    {
        /* no version number available -> use revision number 0 */
        *revision = 0;
        return 0;
    }
    buffer = malloc(filesize + 1);
    if (buffer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)filesize + 1, __FILE__, __LINE__);
        return -1;
    }
    if (za_read_entry(entry, buffer) != 0)
    {
        free(buffer);
        return -1;
    }
    buffer[filesize] = '\0';
    if (coda_ascii_parse_int64(buffer, filesize, &value, 1) < 0)
    {
        /* ignore invalid version numbers and treat them as 0 */
        free(buffer);
        *revision = 0;
        return 0;
    }

    free(buffer);

    *revision = (int)value;

    return 0;
}

static int cd_product_class_init(parser_info *info, const char **attr)
{
    const char *name;
    int revision = 0;

    name = get_mandatory_attribute_value(attr, "name", info->node->tag);
    if (name == NULL)
    {
        return -1;
    }
    if (get_product_class_revision(info, &revision) != 0)
    {
        return -1;
    }
    /* see if there is already a version of this product class in the data dictionary */
    if (coda_data_dictionary_has_product_class(name))
    {
        coda_product_class *product_class;

        /* compare revision numbers */
        product_class = coda_data_dictionary_get_product_class(name);
        if (product_class == NULL)
        {
            return -1;
        }
        if (revision <= coda_product_class_get_revision(product_class))
        {
            /* the current available product class is as new or newer -> ignore this product class and stop parsing */
            info->ignore_file = 1;
            abort_parser(info);
            return 0;
        }
        /* the current available product class is older -> remove it */
        if (coda_data_dictionary_remove_product_class(product_class) != 0)
        {
            return -1;
        }
    }

    info->node->free_data = (free_data_handler)coda_product_class_delete;
    info->product_class = coda_product_class_new(name);
    if (info->product_class == NULL)
    {
        return -1;
    }
    if (coda_product_class_set_definition_file(info->product_class, za_get_filename(info->zf)) != 0)
    {
        coda_product_class_delete(info->product_class);
        info->product_class = NULL;
        return -1;
    }
    if (coda_product_class_set_revision(info->product_class, revision) != 0)
    {
        coda_product_class_delete(info->product_class);
        info->product_class = NULL;
        return -1;
    }
    info->node->data = info->product_class;

    register_sub_element(info->node, element_cd_description, string_data_init, cd_product_class_set_description);
    register_sub_element(info->node, element_cd_product_type, cd_product_type_init, cd_product_class_add_product_type);

    info->node->finalise_element = cd_product_class_finalise;

    return 0;
}

static int cd_product_definition_set_description(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_product_definition_set_description((coda_product_definition *)info->node->parent->data, "");
    }
    return coda_product_definition_set_description((coda_product_definition *)info->node->parent->data,
                                                   info->node->char_data);
}

static int cd_product_definition_add_detection_rule(parser_info *info)
{
    if (coda_product_definition_add_detection_rule((coda_product_definition *)info->node->parent->data,
                                                   (coda_detection_rule *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_definition_set_root_type(parser_info *info)
{
    if (coda_product_definition_set_root_type((coda_product_definition *)info->node->parent->data,
                                              (coda_type *)info->node->data) != 0)
    {
        return -1;
    }
    return 0;
}

static int cd_product_definition_add_product_variable(parser_info *info)
{
    if (coda_product_definition_add_product_variable((coda_product_definition *)info->node->parent->data,
                                                     (coda_product_variable *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_definition_finalise(parser_info *info)
{
    return coda_product_definition_validate((coda_product_definition *)info->node->data);
}

static int cd_product_definition_init(parser_info *info, const char **attr)
{
    const char *id;
    const char *format_string;
    const char *version_string;
    int version;

    id = get_mandatory_attribute_value(attr, "id", info->node->tag);
    if (id == NULL)
    {
        return -1;
    }
    format_string = get_mandatory_attribute_value(attr, "format", info->node->tag);
    if (format_string == NULL)
    {
        return -1;
    }
    if (format_from_string(format_string, &info->node->format) != 0)
    {
        return -1;
    }
    version_string = get_mandatory_attribute_value(attr, "version", info->node->tag);
    if (version_string == NULL)
    {
        return -1;
    }
    if (sscanf(version_string, "%d", &version) != 1)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid 'version' attribute value '%s'", version_string);
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_product_definition_delete;
    info->product_definition = coda_product_definition_new(id, info->node->format, version);
    if (info->product_definition == NULL)
    {
        return -1;
    }
    if (coda_option_read_all_definitions)
    {
        if (parse_entry(info->zf, ze_product, id, info->product_class, info->product_definition) != 0)
        {
            coda_product_definition_delete(info->product_definition);
            info->product_definition = NULL;
            info->add_error_location = 0;
            return -1;
        }
    }
    info->node->data = info->product_definition;

    register_sub_element(info->node, element_cd_description, string_data_init, cd_product_definition_set_description);
    register_sub_element(info->node, element_cd_detection_rule, cd_detection_rule_init,
                         cd_product_definition_add_detection_rule);

    return 0;
}

static int cd_product_definition_sub_init(parser_info *info, const char **attr)
{
    const char *id;
    const char *format_string;

    assert(info->product_definition != NULL);
    info->node->free_data = dummy_free_handler;
    info->node->data = info->product_definition;

    id = get_mandatory_attribute_value(attr, "id", info->node->tag);
    if (id == NULL)
    {
        return -1;
    }
    if (strcmp(info->product_definition->name, id) != 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid id attribute value (%s) for product definition %s", id,
                       info->product_definition->name);
        return -1;
    }
    format_string = get_mandatory_attribute_value(attr, "format", info->node->tag);
    if (format_string == NULL)
    {
        return -1;
    }
    if (format_from_string(format_string, &info->node->format) != 0)
    {
        return -1;
    }
    if (info->product_definition->format != info->node->format)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION,
                       "format for product defintion %s differs between index and product file",
                       info->product_definition->name);
        return -1;
    }

    if (info->product_definition->format == coda_format_xml)
    {
        register_sub_element(info->node, element_cd_record, xml_root_init, cd_product_definition_set_root_type);
    }
    else
    {
        register_type_elements(info->node, cd_product_definition_set_root_type);
    }
    register_sub_element(info->node, element_cd_product_variable, cd_product_variable_init,
                         cd_product_definition_add_product_variable);
    info->node->finalise_element = cd_product_definition_finalise;

    return 0;
}

static int cd_product_type_set_description(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_product_type_set_description((coda_product_type *)info->node->parent->data, "");
    }
    return coda_product_type_set_description((coda_product_type *)info->node->parent->data, info->node->char_data);
}

static int cd_product_type_add_product_definition(parser_info *info)
{
    if (coda_product_type_add_product_definition((coda_product_type *)info->node->parent->data,
                                                 (coda_product_definition *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_type_init(parser_info *info, const char **attr)
{
    const char *name;

    attr = attr;

    name = get_mandatory_attribute_value(attr, "name", info->node->tag);
    if (name == NULL)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_product_type_delete;
    info->node->data = coda_product_type_new(name);
    if (info->node->data == NULL)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, cd_product_type_set_description);
    register_sub_element(info->node, element_cd_product_definition, cd_product_definition_init,
                         cd_product_type_add_product_definition);

    return 0;
}

static int cd_product_variable_set_size_expression(parser_info *info)
{
    if (coda_product_variable_set_size_expression((coda_product_variable *)info->node->parent->data,
                                                  (coda_expression *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_variable_set_init_expression(parser_info *info)
{
    if (coda_product_variable_set_init_expression((coda_product_variable *)info->node->parent->data,
                                                  (coda_expression *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_product_variable_finalise(parser_info *info)
{
    return coda_product_variable_validate((coda_product_variable *)info->node->data);
}

static int cd_product_variable_init(parser_info *info, const char **attr)
{
    const char *name;

    name = get_mandatory_attribute_value(attr, "name", info->node->tag);
    if (name == NULL)
    {
        return -1;
    }

    info->node->free_data = (free_data_handler)coda_product_variable_delete;
    info->node->data = coda_product_variable_new(name);
    if (info->node->data == NULL)
    {
        return -1;
    }
    register_sub_element(info->node, element_cd_dimension, integer_expression_init,
                         cd_product_variable_set_size_expression);
    register_sub_element(info->node, element_cd_init, void_expression_init, cd_product_variable_set_init_expression);

    info->node->finalise_element = cd_product_variable_finalise;

    return 0;
}

static int cd_raw_set_fixed_value(parser_info *info)
{
    long value_length;

    value_length = decode_escaped_string(info->node->char_data);
    if (value_length < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid escape sequence in string");
        return -1;
    }
    if (value_length > 0)
    {
        if (coda_type_raw_set_fixed_value((coda_type_raw *)info->node->parent->data, value_length,
                                          info->node->char_data) != 0)
        {
            return -1;
        }
    }

    return 0;
}

static int cd_raw_finalise(parser_info *info)
{
    return coda_type_raw_validate((coda_type_raw *)info->node->data);
}

static int cd_raw_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_raw_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_bit_size, integer_constant_or_expression_init, type_set_bit_size);
    register_sub_element(info->node, element_cd_fixed_value, string_data_init, cd_raw_set_fixed_value);
    info->node->finalise_element = cd_raw_finalise;

    return 0;
}

static int cd_record_add_field(parser_info *info)
{
    if (coda_type_record_add_field((coda_type_record *)info->node->parent->data,
                                   (coda_type_record_field *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_record_finalise(parser_info *info)
{
    return coda_type_record_validate((coda_type_record *)info->node->data);
}

static int cd_record_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_record_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_bit_size, integer_expression_init, type_set_bit_size);
    register_sub_element(info->node, element_cd_field, cd_field_init, cd_record_add_field);
    register_sub_element(info->node, element_cd_attribute, cd_attribute_init, type_add_attribute);
    info->node->finalise_element = cd_record_finalise;

    if (handle_xml_name(info, attr) != 0)
    {
        return -1;
    }

    return 0;
}

static int cd_scale_factor_set_type(parser_info *info)
{
    info->node->parent->data = info->node->data;
    info->node->data = NULL;

    return 0;
}

static int cd_scale_factor_finalise(parser_info *info)
{
    if (info->node->data == NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "missing type for ScaleFactor");
        return -1;
    }

    return 0;
}

static int cd_scale_factor_init(parser_info *info, const char **attr)
{
    if (get_attribute_value(attr, "format") != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'format' not allowed for ScaleFactor");
        return -1;
    }
    info->node->format = info->node->parent->format;
    if (get_attribute_value(attr, "name") != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'name' not allowed for ScaleFactor");
        return -1;
    }
    if (get_attribute_value(attr, "namexml") != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'namexml' not allowed for ScaleFactor");
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;

    register_type_elements(info->node, cd_scale_factor_set_type);
    info->node->finalise_element = cd_scale_factor_finalise;

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);


    return 0;
}

static int cd_text_set_fixed_value(parser_info *info)
{
    if (decode_escaped_string(info->node->char_data) < 0)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid escape sequence in string");
        return -1;
    }
    return coda_type_text_set_fixed_value((coda_type_text *)info->node->parent->data, info->node->char_data);
}

static int cd_text_set_read_type(parser_info *info)
{
    return coda_type_set_read_type((coda_type *)info->node->parent->data, (int)info->node->integer_data);
}

static int cd_text_finalise(parser_info *info)
{
    return coda_type_text_validate((coda_type_text *)info->node->data);
}

static int cd_text_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_text_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_byte_size, integer_constant_or_expression_init, type_set_byte_size);
    register_sub_element(info->node, element_cd_fixed_value, string_data_init, cd_text_set_fixed_value);
    register_sub_element(info->node, element_cd_native_type, cd_native_type_init, cd_text_set_read_type);
    register_sub_element(info->node, element_cd_attribute, cd_attribute_init, type_add_attribute);
    info->node->finalise_element = cd_text_finalise;

    if (handle_xml_name(info, attr) != 0)
    {
        return -1;
    }

    return 0;
}

static int cd_time_add_mapping(parser_info *info)
{
    if (coda_type_time_add_ascii_float_mapping((coda_type_special *)info->node->parent->data,
                                               (coda_ascii_float_mapping *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_time_init(parser_info *info, const char **attr)
{
    const char *timeformat;

    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    timeformat = get_mandatory_attribute_value(attr, "timeformat", info->node->tag);
    if (timeformat == NULL)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_time_new(info->node->format, timeformat);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_mapping, cd_mapping_init, cd_time_add_mapping);

    return 0;
}

static int cd_type_set_type(parser_info *info)
{
    coda_type *type;

    type = (coda_type *)info->node->parent->data;
    if (type->description != NULL)
    {
        coda_type_set_description((coda_type *)info->node->data, type->description);
    }
    if (type->attributes != NULL)
    {
        assert(((coda_type *)info->node->data)->attributes == NULL);
        ((coda_type *)info->node->data)->attributes = type->attributes;
        type->attributes = NULL;
    }
    coda_type_release(type);
    info->node->parent->data = info->node->data;
    info->node->data = NULL;

    return 0;
}

static int cd_type_init(parser_info *info, const char **attr)
{
    if (get_attribute_value(attr, "name") != NULL)
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "attribute 'name' not allowed for Type");
        return -1;
    }
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    /* create dummy type where a description and attributes can be stored */
    info->node->data = coda_type_text_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    register_type_elements(info->node, cd_type_set_type);
    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_attribute, cd_attribute_init, type_add_attribute);

    if (handle_xml_name(info, attr) != 0)
    {
        return -1;
    }

    return 0;
}

static int cd_union_set_field_expression(parser_info *info)
{
    if (coda_type_record_set_union_field_expression((coda_type_record *)info->node->parent->data,
                                                    (coda_expression *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_union_add_field(parser_info *info)
{
    if (coda_type_record_add_field((coda_type_record *)info->node->parent->data,
                                   (coda_type_record_field *)info->node->data) != 0)
    {
        return -1;
    }
    info->node->data = NULL;
    return 0;
}

static int cd_union_finalise(parser_info *info)
{
    return coda_type_record_validate((coda_type_record *)info->node->data);
}

static int cd_union_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_record_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_sub_element(info->node, element_cd_bit_size, integer_expression_init, type_set_bit_size);
    register_sub_element(info->node, element_cd_field_expression, integer_expression_init,
                         cd_union_set_field_expression);
    register_sub_element(info->node, element_cd_field, cd_field_init, cd_union_add_field);
    info->node->finalise_element = cd_union_finalise;

    if (handle_xml_name(info, attr) != 0)
    {
        return -1;
    }

    return 0;
}

static int cd_vsf_integer_set_type(parser_info *info)
{
    return coda_type_vsf_integer_set_type((coda_type_special *)info->node->parent->data, (coda_type *)info->node->data);
}

static int cd_vsf_integer_set_scale_factor(parser_info *info)
{
    return coda_type_vsf_integer_set_scale_factor((coda_type_special *)info->node->parent->data,
                                                  (coda_type *)info->node->data);
}

static int cd_vsf_integer_set_unit(parser_info *info)
{
    if (info->node->char_data == NULL)
    {
        return coda_type_vsf_integer_set_unit((coda_type_special *)info->node->parent->data, "");
    }
    return coda_type_vsf_integer_set_unit((coda_type_special *)info->node->parent->data, info->node->char_data);
}

static int cd_vsf_integer_finalise(parser_info *info)
{
    return coda_type_vsf_integer_validate((coda_type_special *)info->node->data);
}

static int cd_vsf_integer_init(parser_info *info, const char **attr)
{
    if (handle_format_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }
    info->node->free_data = (free_data_handler)coda_type_release;
    info->node->data = coda_type_vsf_integer_new(info->node->format);
    if (info->node->data == NULL)
    {
        return -1;
    }
    if (handle_name_attribute_for_type(info, attr) != 0)
    {
        return -1;
    }

    register_sub_element(info->node, element_cd_description, string_data_init, type_set_description);
    register_type_elements(info->node, cd_vsf_integer_set_type);
    register_sub_element(info->node, element_cd_scale_factor, cd_scale_factor_init, cd_vsf_integer_set_scale_factor);
    register_sub_element(info->node, element_cd_unit, string_data_init, cd_vsf_integer_set_unit);
    info->node->finalise_element = cd_vsf_integer_finalise;

    return 0;
}

static void XMLCALL whitespace_handler(void *data, const char *s, int len)
{
    parser_info *info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    /* the generic char handler only allows white space (which is ignored) */
    if (!is_whitespace(s, len))
    {
        coda_set_error(CODA_ERROR_DATA_DEFINITION, "non-whitespace character data not allowed here");
        abort_parser(info);
    }
}

static void XMLCALL string_handler(void *data, const char *s, int len)
{
    parser_info *info = (parser_info *)data;

    if (info->unparsed_depth > 0)
    {
        return;
    }

    if (info->node->char_data == NULL)
    {
        info->node->char_data = malloc(len + 1);
        if (info->node->char_data == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)len + 1, __FILE__, __LINE__);
            abort_parser(info);
            return;
        }
        memcpy(info->node->char_data, s, len);
        info->node->char_data[len] = '\0';
    }
    else
    {
        char *char_data;
        long current_length = strlen(info->node->char_data);

        char_data = malloc(current_length + len + 1);
        if (char_data == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           current_length + len + 1, __FILE__, __LINE__);
            abort_parser(info);
            return;
        }
        memcpy(char_data, info->node->char_data, current_length);
        memcpy(&char_data[current_length], s, len);
        char_data[current_length + len] = '\0';
        free(info->node->char_data);
        info->node->char_data = char_data;
    }
}

static int push_node(parser_info *info, xml_element_tag tag, const char **attr)
{
    node_info *node;

#if 0
    printf("push %s\n", xml_element_name(tag));
#endif
    node = malloc(sizeof(node_info));
    assert(node != NULL);
    node->tag = tag;
    node->empty = 0;
    node->data = NULL;
    node->char_data = NULL;
    node->integer_data = -1;
    node->float_data = coda_NaN();
    node->expect_char_data = 0;
    node->finalise_element = NULL;
    node->free_data = NULL;
    node->format = FORMAT_UNDEFINED;
    memset(node->init_sub_element, 0, num_xml_elements * sizeof(init_handler));
    memset(node->add_element_to_parent, 0, num_xml_elements * sizeof(add_element_to_parent_handler));
    node->parent = info->node;
    info->node = node;

    if (node->parent != NULL && node->parent->init_sub_element[tag] != NULL)
    {
        if (node->parent->init_sub_element[tag] (info, attr) != 0)
        {
            return -1;
        }
    }

    if (node->expect_char_data)
    {
        XML_SetCharacterDataHandler(info->parser, string_handler);
    }
    else
    {
        XML_SetCharacterDataHandler(info->parser, whitespace_handler);
    }

    return 0;
}

static int pop_node(parser_info *info)
{
    node_info *node = info->node;

    assert(node != NULL);
#if 0
    printf("pop  %s\n", xml_element_name(node->tag));
#endif
    if (node->finalise_element != NULL)
    {
        if (node->finalise_element(info) != 0)
        {
            return -1;
        }
    }
    if (node->parent != NULL && node->parent->add_element_to_parent[node->tag] != NULL)
    {
        if (node->parent->add_element_to_parent[node->tag] (info) != 0)
        {
            return -1;
        }
    }
    if (node->data != NULL)
    {
        assert(node->free_data != NULL);
        node->free_data(node->data);
    }
    if (node->char_data != NULL)
    {
        free(node->char_data);
    }
    info->node = node->parent;
    free(node);

    if (info->node != NULL && info->node->expect_char_data)
    {
        XML_SetCharacterDataHandler(info->parser, string_handler);
    }
    else
    {
        XML_SetCharacterDataHandler(info->parser, whitespace_handler);
    }

    return 0;
}

static void XMLCALL start_element_handler(void *data, const char *el, const char **attr)
{
    parser_info *info = (parser_info *)data;
    xml_element_tag tag;

    if (info->unparsed_depth > 0)
    {
        /* We are inside an element of another namespace -> ignore this element */
        info->unparsed_depth++;
        return;
    }

    tag = (xml_element_tag)hashtable_get_index_from_name(info->hash_data, el);
    if (tag < 0 && strncmp(el, CODA_DEFINITION_NAMESPACE, strlen(CODA_DEFINITION_NAMESPACE)) != 0)
    {
        /* start of a branch from some other namespace */
        info->unparsed_depth = 1;
        return;
    }
    if (tag < 0 || info->node->init_sub_element[tag] == NULL)
    {
        if (info->node->tag == no_element)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml element '%s' is not allowed as root element",
                           coda_element_name_from_xml_name(el));
        }
        else if (info->node->format != FORMAT_UNDEFINED)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION,
                           "xml element '%s' is not allowed within element '%s'{%s}",
                           coda_element_name_from_xml_name(el), xml_element_name(info->node->tag),
                           coda_type_get_format_name(info->node->format));
        }
        else
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml element '%s' is not allowed within element '%s'",
                           coda_element_name_from_xml_name(el), xml_element_name(info->node->tag));
        }
        abort_parser(info);
        return;
    }

    if (push_node(info, tag, attr) != 0)
    {
        abort_parser(info);
    }
}

static void XMLCALL end_element_handler(void *data, const char *el)
{
    parser_info *info = (parser_info *)data;

    el = el;

    if (info->abort_parser)
    {
        return;
    }

    if (info->unparsed_depth > 0)
    {
        info->unparsed_depth--;
        return;
    }

    if (pop_node(info) != 0)
    {
        abort_parser(info);
    }
}

static void parser_info_init(parser_info *info)
{
    info->node = NULL;
    info->parser = NULL;
    info->hash_data = NULL;
    info->buffer = NULL;
    info->zf = NULL;
    info->product_class = NULL;
    info->product_definition = NULL;
    info->product_class_revision = 0;
    info->abort_parser = 0;
    info->ignore_file = 0;
    info->add_error_location = 1;
    info->unparsed_depth = 0;
}

static void parser_info_delete(parser_info *info)
{
    while (info->node != NULL)
    {
        node_info *node;

        node = info->node;
        if (node->data != NULL)
        {
            assert(node->free_data != NULL);
            node->free_data(node->data);
        }
        if (node->char_data != NULL)
        {
            free(node->char_data);
        }
        info->node = node->parent;
        free(node);
    }
    if (info->parser != NULL)
    {
        XML_ParserFree(info->parser);
    }
    if (info->hash_data != NULL)
    {
        hashtable_delete(info->hash_data);
    }
    if (info->buffer != NULL)
    {
        free(info->buffer);
    }
    info->zf = NULL;
}

static int parse_entry(za_file *zf, zip_entry_type type, const char *name, coda_product_class *current_product_class,
                       coda_product_definition *current_product_definition)
{
    parser_info info;
    char *entry_name = NULL;
    za_entry *entry;
    long filesize;
    int result;
    int i;

    switch (type)
    {
        case ze_index:
            entry_name = strdup("index.xml");
            if (entry_name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                               __LINE__);
            }
            break;
        case ze_type:
            assert(name != NULL);
            entry_name = malloc(6 + strlen(name) + 4 + 1);
            if (entry_name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)6 + strlen(name) + 4 + 1, __FILE__, __LINE__);
            }
            sprintf(entry_name, "types/%s.xml", name);
            break;
        case ze_product:
            assert(name != NULL);
            entry_name = malloc(9 + strlen(name) + 4 + 1);
            if (entry_name == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)9 + strlen(name) + 4 + 1, __FILE__, __LINE__);
            }
            sprintf(entry_name, "products/%s.xml", name);
            break;
    }

    entry = za_get_entry_by_name(zf, entry_name);
    if (entry == NULL)
    {
        switch (type)
        {
            case ze_index:
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid definition file '%s' (index missing)",
                               za_get_filename(zf));
                break;
            case ze_type:
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid definition file '%s' "
                               "(definition for type '%s' missing)", za_get_filename(zf), name);
                break;
            case ze_product:
                coda_set_error(CODA_ERROR_DATA_DEFINITION, "invalid definition file '%s' "
                               "(definition for product '%s' missing)", za_get_filename(zf), name);
                break;
        }
        free(entry_name);
        return -1;
    }
    free(entry_name);

    parser_info_init(&info);
    info.zf = zf;
    info.entry_base_name = name;
    info.product_class = current_product_class;
    info.product_definition = current_product_definition;

    filesize = za_get_entry_size(entry);
    info.buffer = malloc(filesize);
    if (info.buffer == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       (long)filesize, __FILE__, __LINE__);
        parser_info_delete(&info);
        return -1;
    }
    if (za_read_entry(entry, info.buffer) != 0)
    {
        parser_info_delete(&info);
        return -1;
    }

    info.hash_data = hashtable_new(1);
    if (info.hash_data == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate hashtable) (%s:%u)", __FILE__,
                       __LINE__);
        parser_info_delete(&info);
        return -1;
    }
    for (i = 0; i < num_xml_elements; i++)
    {
        if (hashtable_add_name(info.hash_data, xml_full_element_name[i]) != 0)
        {
            assert(0);
        }
    }

    info.parser = XML_ParserCreateNS(NULL, ' ');
    if (info.parser == NULL)
    {
        coda_set_error(CODA_ERROR_XML, "could not create XML parser");
        parser_info_delete(&info);
        return -1;
    }
    XML_SetUserData(info.parser, &info);
    XML_SetElementHandler(info.parser, start_element_handler, end_element_handler);
    push_node(&info, no_element, NULL);
    info.node->format = FORMAT_UNDEFINED;
    switch (type)
    {
        case ze_index:
            register_sub_element(info.node, element_cd_product_class, cd_product_class_init,
                                 data_dictionary_add_product_class);
            break;
        case ze_type:
            register_type_elements(info.node, product_class_add_named_type);
            break;
        case ze_product:
            register_sub_element(info.node, element_cd_product_definition, cd_product_definition_sub_init, NULL);
            break;
    }

    coda_errno = 0;
    result = XML_Parse(info.parser, info.buffer, filesize, 1);
    if ((result == XML_STATUS_ERROR || coda_errno != 0) && !info.ignore_file)
    {
        if (coda_errno == 0)
        {
            coda_set_error(CODA_ERROR_DATA_DEFINITION, "xml parse error: %s",
                           XML_ErrorString(XML_GetErrorCode(info.parser)));
        }
        if ((coda_errno == CODA_ERROR_DATA_DEFINITION || coda_errno == CODA_ERROR_EXPRESSION) &&
            info.add_error_location)
        {
            coda_add_error_message(" (in %s@", za_get_filename(zf));
            switch (type)
            {
                case ze_index:
                    coda_add_error_message("index", NULL);
                    break;
                case ze_type:
                    coda_add_error_message("types/%s", name);
                    break;
                case ze_product:
                    coda_add_error_message("products/%s", name);
                    break;
            }
            coda_add_error_message(", line %lu, byte offset %ld)", (long)XML_GetCurrentLineNumber(info.parser),
                                   (long)XML_GetCurrentByteIndex(info.parser));
        }
        parser_info_delete(&info);
        return -1;
    }
    parser_info_delete(&info);

    return 0;
}

static int read_definition_file(const char *filename)
{
    za_file *zf;

    zf = za_open(filename, handle_ziparchive_error);
    if (zf == NULL)
    {
        return -1;
    }

    if (parse_entry(zf, ze_index, NULL, NULL, NULL) != 0)
    {
        za_close(zf);
        return -1;
    }

    za_close(zf);

    return 0;
}

int coda_read_product_definition(coda_product_definition *product_definition)
{
    coda_product_class *product_class;
    za_file *zf;

    assert(product_definition->root_type == NULL);

    product_class = product_definition->product_type->product_class;

    zf = za_open(product_class->definition_file, handle_ziparchive_error);
    if (zf == NULL)
    {
        return -1;
    }
    if (parse_entry(zf, ze_product, product_definition->name, product_class, product_definition) != 0)
    {
        za_close(zf);
        return -1;
    }
    za_close(zf);

    return 0;
}

int coda_read_definitions(const char *definition_path)
{
#ifdef WIN32
    const char path_separator_char = ';';
#else
    const char path_separator_char = ':';
#endif
    char *path;
    char *path_component;

    path = strdup(definition_path);
    if (path == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate string) (%s:%u)", __FILE__,
                       __LINE__);
        return -1;
    }
    path_component = path;
    while (*path_component != '\0')
    {
        struct stat sb;
        char *p;

        p = path_component;
        while (*p != '\0' && *p != path_separator_char)
        {
            p++;
        }
        if (*p != '\0')
        {
            *p = '\0';
            p++;
        }

        if (stat(path_component, &sb) == 0)
        {
            if (sb.st_mode & S_IFDIR)
            {
#ifdef WIN32
                WIN32_FIND_DATA FileData;
                HANDLE hSearch;
                BOOL fFinished;
                char *pattern;

                pattern = malloc(strlen(path_component) + 10 + 1);
                if (pattern == NULL)
                {
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                                   (long)strlen(path) + 10 + 1, __FILE__, __LINE__);
                    free(path);
                    return -1;
                }
                sprintf(pattern, "%s\\*.codadef", path_component);
                hSearch = FindFirstFile(pattern, &FileData);
                free(pattern);

                if (hSearch == INVALID_HANDLE_VALUE)
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)
                    {
                        /* no files found */
                        continue;
                    }
                    coda_set_error(CODA_ERROR_DATA_DEFINITION, "could not access directory '%s'", path_component);
                    free(path);
                    return -1;
                }

                fFinished = FALSE;
                while (!fFinished)
                {
                    /* skip directories with a .codadef extension */
                    if (!(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        char *filepath;

                        filepath = malloc(strlen(path_component) + 1 + strlen(FileData.cFileName) + 1);
                        if (filepath == NULL)
                        {
                            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) "
                                           "(%s:%u)", (long)strlen(path_component) + 1 + strlen(FileData.cFileName) + 1,
                                           __FILE__, __LINE__);
                            free(path);
                            return -1;
                        }
                        sprintf(filepath, "%s\\%s", path_component, FileData.cFileName);
                        if (read_definition_file(filepath) != 0)
                        {
                            free(filepath);
                            free(path);
                            FindClose(hSearch);
                            return -1;
                        }
                        free(filepath);
                    }

                    if (!FindNextFile(hSearch, &FileData))
                    {
                        if (GetLastError() == ERROR_NO_MORE_FILES)
                        {
                            fFinished = TRUE;
                        }
                        else
                        {
                            FindClose(hSearch);
                            coda_set_error(CODA_ERROR_DATA_DEFINITION, "could not retrieve directory entry");
                            free(path);
                            return -1;
                        }
                    }
                }
                FindClose(hSearch);
#else
                DIR *dirp;
                struct dirent *dp;

                dirp = opendir(path_component);
                if (dirp == NULL)
                {
                    coda_set_error(CODA_ERROR_DATA_DEFINITION, "could not access directory '%s' (%s)", path_component,
                                   strerror(errno));
                    free(path);
                    return -1;
                }

                while ((dp = readdir(dirp)) != NULL)
                {
                    int filename_length;

                    filename_length = strlen(dp->d_name);
                    if (filename_length > 8 && strcmp(&dp->d_name[filename_length - 8], ".codadef") == 0)
                    {
                        char *filepath;

                        filepath = malloc(strlen(path_component) + 1 + filename_length + 1);
                        if (filepath == NULL)
                        {
                            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) "
                                           "(%s:%u)", (long)strlen(path_component) + 1 + filename_length + 1, __FILE__,
                                           __LINE__);
                            closedir(dirp);
                            free(path);
                            return -1;
                        }
                        sprintf(filepath, "%s/%s", path_component, dp->d_name);

                        if (stat(filepath, &sb) != 0)
                        {
                            coda_set_error(CODA_ERROR_DATA_DEFINITION, "could not access file '%s' (%s)", filepath,
                                           strerror(errno));
                            free(filepath);
                            closedir(dirp);
                            free(path);
                            return -1;
                        }
                        if (sb.st_mode & S_IFREG)
                        {
                            if (read_definition_file(filepath) != 0)
                            {
                                free(filepath);
                                closedir(dirp);
                                free(path);
                                return -1;
                            }
                        }
                        free(filepath);
                    }
                }
                closedir(dirp);
#endif
            }
            else if (sb.st_mode & S_IFREG)
            {
                if (read_definition_file(path_component) != 0)
                {
                    free(path);
                    return -1;
                }
            }
        }

        path_component = p;
    }

    free(path);

    return 0;
}
