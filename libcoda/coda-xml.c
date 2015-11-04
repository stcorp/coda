/*
 * Copyright (C) 2007-2009 S&T, The Netherlands.
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

#include "coda-xml-internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coda-definition.h"

int coda_xml_recognize_file(const char *filename, int64_t size, coda_ProductDefinition **definition)
{
    int fd;
    int open_flags;

    size = size;

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    fd = open(filename, open_flags);
    if (fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", filename, strerror(errno));
        return -1;
    }

    if (coda_xml_parse_for_detection(fd, filename, definition) != 0)
    {
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

int coda_xml_open(const char *filename, int64_t file_size, coda_ProductFile **pf)
{
    coda_xmlProductFile *product_file = (coda_xmlProductFile *)pf;
    int open_flags;

    product_file = (coda_xmlProductFile *)malloc(sizeof(coda_xmlProductFile));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_xmlProductFile), __FILE__, __LINE__);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = file_size;
    product_file->format = coda_format_xml;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->use_mmap = 0;
    product_file->fd = -1;

    product_file->filename = strdup(filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_xml_close((coda_ProductFile *)product_file);
        return -1;
    }

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    product_file->fd = open(product_file->filename, open_flags);
    if (product_file->fd < 0)
    {
        coda_set_error(CODA_ERROR_FILE_OPEN, "could not open file %s (%s)", product_file->filename, strerror(errno));
        coda_xml_close((coda_ProductFile *)product_file);
        return -1;
    }

    if (coda_xml_parse_for_detection(product_file->fd, product_file->filename, &product_file->product_definition) != 0)
    {
        coda_xml_close((coda_ProductFile *)product_file);
        return -1;
    }

    lseek(product_file->fd, 0, SEEK_SET);
    if (product_file->product_definition != NULL)
    {
        if (product_file->product_definition->root_type == NULL)
        {
            if (coda_read_product_definition(product_file->product_definition) != 0)
            {
                coda_xml_close((coda_ProductFile *)product_file);
                return -1;
            }
        }
        if (coda_xml_parse_with_definition(product_file) != 0)
        {
            coda_xml_close((coda_ProductFile *)product_file);
            return -1;
        }
    }
    else
    {
        if (coda_xml_parse_and_interpret(product_file) != 0)
        {
            coda_xml_close((coda_ProductFile *)product_file);
            return -1;
        }
    }

    *pf = (coda_ProductFile *)product_file;

    return 0;
}

int coda_xml_close(coda_ProductFile *pf)
{
    coda_xmlProductFile *product_file = (coda_xmlProductFile *)pf;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_release_dynamic_type(product_file->root_type);
    }
    if (product_file->fd >= 0)
    {
        close(product_file->fd);
    }

    free(product_file);

    return 0;
}

int coda_xml_get_type_for_dynamic_type(coda_DynamicType *dynamic_type, coda_Type **type)
{
    switch (((coda_xmlDynamicType *)dynamic_type)->tag)
    {
        case tag_xml_root_dynamic:
            *type = (coda_Type *)((coda_xmlRootDynamicType *)dynamic_type)->type;
            break;
        case tag_xml_record_dynamic:
        case tag_xml_text_dynamic:
        case tag_xml_ascii_type_dynamic:
            *type = (coda_Type *)((coda_xmlElementDynamicType *)dynamic_type)->type;
            break;
        case tag_xml_array_dynamic:
            *type = (coda_Type *)((coda_xmlArrayDynamicType *)dynamic_type)->type;
            break;
        case tag_xml_attribute_dynamic:
            *type = (coda_Type *)((coda_xmlAttributeDynamicType *)dynamic_type)->type;
            break;
        case tag_xml_attribute_record_dynamic:
            *type = (coda_Type *)((coda_xmlAttributeRecordDynamicType *)dynamic_type)->type;
            break;
    }

    return 0;
}

coda_xmlDetectionNode *coda_xml_get_detection_tree(void)
{
    return (coda_xmlDetectionNode *)coda_data_dictionary->xml_detection_tree;
}
