/*
 * Copyright (C) 2007-2018 S[&]T, The Netherlands.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "coda-cdf-internal.h"
#include "coda-mem-internal.h"
#include "coda-read-bytes.h"
#include "coda-swap2.h"
#include "coda-swap4.h"
#include "coda-swap8.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "zlib.h"

static void rtrim(char *str)
{
    long length;

    length = strlen(str);
    while (length > 0 && str[length - 1] == ' ')
    {
        str[length - 1] = '\0';
        length--;
    }
}

static int read_attribute_sub(coda_cdf_product *product_file, int64_t offset, int32_t byte_size, coda_type *definition,
                              coda_dynamic_type **attribute)
{
    if (definition->type_class == coda_text_class)
    {
        char *buffer;

        buffer = malloc(byte_size + 1);
        if (buffer == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)byte_size + 1, __FILE__, __LINE__);
            return -1;
        }
        if (read_bytes(product_file->raw_product, offset, byte_size, buffer) != 0)
        {
            free(buffer);
            return -1;
        }
        buffer[byte_size] = '\0';
        *attribute = (coda_dynamic_type *)coda_mem_string_new((coda_type_text *)definition, NULL,
                                                              (coda_product *)product_file, buffer);
        free(buffer);
    }
    else
    {
#ifdef WORDS_BIGENDIAN
        coda_endianness other_endianness = coda_little_endian;
#else
        coda_endianness other_endianness = coda_big_endian;
#endif
        union
        {
            int8_t as_int8[8];
            uint8_t as_uint8[8];
            int16_t as_int16[4];
            uint16_t as_uint16[4];
            int32_t as_int32[2];
            uint32_t as_uint32[2];
            int64_t as_int64[1];
            float as_float[2];
            double as_double[1];
        } buffer;

        if (read_bytes(product_file->raw_product, offset, byte_size, buffer.as_int8) != 0)
        {
            return -1;
        }
        if (product_file->endianness == other_endianness)
        {
            switch (byte_size)
            {
                case 1:
                    break;
                case 2:
                    swap2(buffer.as_int16);
                    break;
                case 4:
                    swap4(buffer.as_int32);
                    break;
                case 8:
                    swap8(buffer.as_int64);
                    break;
                default:
                    assert(0);
                    exit(1);
            }
        }
        switch (definition->read_type)
        {
            case coda_native_type_int8:
                *attribute = (coda_dynamic_type *)coda_mem_int8_new((coda_type_number *)definition, NULL,
                                                                    (coda_product *)product_file, buffer.as_int8[0]);
                break;
            case coda_native_type_uint8:
                *attribute = (coda_dynamic_type *)coda_mem_uint8_new((coda_type_number *)definition, NULL,
                                                                     (coda_product *)product_file, buffer.as_uint8[0]);
                break;
            case coda_native_type_int16:
                *attribute = (coda_dynamic_type *)coda_mem_int16_new((coda_type_number *)definition, NULL,
                                                                     (coda_product *)product_file, buffer.as_int16[0]);
                break;
            case coda_native_type_uint16:
                *attribute = (coda_dynamic_type *)coda_mem_uint16_new((coda_type_number *)definition, NULL,
                                                                      (coda_product *)product_file,
                                                                      buffer.as_uint16[0]);
                break;
            case coda_native_type_int32:
                *attribute = (coda_dynamic_type *)coda_mem_int32_new((coda_type_number *)definition, NULL,
                                                                     (coda_product *)product_file, buffer.as_int32[0]);
                break;
            case coda_native_type_uint32:
                *attribute = (coda_dynamic_type *)coda_mem_uint32_new((coda_type_number *)definition, NULL,
                                                                      (coda_product *)product_file,
                                                                      buffer.as_uint32[0]);
                break;
            case coda_native_type_int64:
                *attribute = (coda_dynamic_type *)coda_mem_int64_new((coda_type_number *)definition, NULL,
                                                                     (coda_product *)product_file, buffer.as_int64[0]);
                break;
            case coda_native_type_float:
                *attribute = (coda_dynamic_type *)coda_mem_float_new((coda_type_number *)definition, NULL,
                                                                     (coda_product *)product_file, buffer.as_float[0]);
                break;
            case coda_native_type_double:
                *attribute = (coda_dynamic_type *)coda_mem_double_new((coda_type_number *)definition, NULL,
                                                                      (coda_product *)product_file,
                                                                      buffer.as_double[0]);
                break;
            default:
                assert(0);
                exit(1);
        }
    }
    if (*attribute == NULL)
    {
        return -1;
    }
    return 0;
}

static int read_attribute(coda_cdf_product *product_file, int64_t offset, int32_t data_type,
                          int32_t num_elems, coda_dynamic_type **attribute)
{
    coda_type_class type_class;
    coda_native_type native_type;
    coda_type *definition;
    int32_t byte_size;

    switch (data_type)
    {
        case 1:        /* INT1 */
        case 41:       /* BYTE */
            type_class = coda_integer_class;
            native_type = coda_native_type_int8;
            byte_size = 1;
            break;
        case 2:        /* INT2 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int16;
            byte_size = 2;
            break;
        case 4:        /* INT4 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int32;
            byte_size = 4;
            break;
        case 8:        /* INT8 */
        case 33:       /* CDF_TIME_TT2000 */
            type_class = coda_integer_class;
            native_type = coda_native_type_int64;
            byte_size = 8;
            break;
        case 11:       /* UINT1 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint8;
            byte_size = 1;
            break;
        case 12:       /* UINT2 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint16;
            byte_size = 2;
            break;
        case 14:       /* UINT4 */
            type_class = coda_integer_class;
            native_type = coda_native_type_uint32;
            byte_size = 4;
            break;
        case 21:       /* REAL4 */
        case 44:       /* FLOAT */
            type_class = coda_real_class;
            native_type = coda_native_type_float;
            byte_size = 4;
            break;
        case 22:       /* REAL8 */
        case 31:       /* CDF_EPOCH */
        case 45:       /* DOUBLE */
            type_class = coda_real_class;
            native_type = coda_native_type_double;
            byte_size = 8;
            break;
        case 51:       /* CHAR */
        case 52:       /* UCHAR */
            type_class = coda_text_class;
            native_type = coda_native_type_string;
            byte_size = num_elems;
            break;
        default:
            coda_set_error(CODA_ERROR_INVALID_ARGUMENT, "invalid CDF data type (%d)", data_type);
            return -1;
    }
    switch (type_class)
    {
        case coda_integer_class:
        case coda_real_class:
            definition = (coda_type *)coda_type_number_new(coda_format_cdf, type_class);
            break;
        case coda_text_class:
            definition = (coda_type *)coda_type_text_new(coda_format_cdf);
            break;
        default:
            assert(0);
            exit(1);
    }
    if (definition == NULL)
    {
        return -1;
    }
    if (coda_type_set_read_type(definition, native_type) != 0)
    {
        coda_type_release(definition);
        return -1;
    }
    if (type_class != coda_text_class)
    {
        if (coda_type_set_byte_size(definition, byte_size) != 0)
        {
            coda_type_release(definition);
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        if (coda_type_number_set_endianness((coda_type_number *)definition, coda_little_endian) != 0)
        {
            coda_type_release(definition);
            return -1;
        }
#endif
    }

    if (num_elems != 1 && data_type != 51 && data_type != 52)
    {
        coda_type_array *array_definition;
        coda_mem_array *array = NULL;
        int i;

        /* create an array */
        array_definition = coda_type_array_new(coda_format_cdf);
        if (array_definition == NULL)
        {
            coda_type_release(definition);
            return -1;
        }
        if (coda_type_array_set_base_type(array_definition, definition) != 0)
        {
            coda_type_release((coda_type *)array_definition);
            coda_type_release(definition);
            return -1;
        }
        coda_type_release(definition);
        if (coda_type_array_add_variable_dimension(array_definition, NULL) != 0)
        {
            coda_type_release((coda_type *)array_definition);
            return -1;
        }
        array = coda_mem_array_new(array_definition, NULL);
        if (array == NULL)
        {
            coda_type_release((coda_type *)array_definition);
            return -1;
        }
        coda_type_release((coda_type *)array_definition);
        for (i = 0; i < num_elems; i++)
        {
            if (read_attribute_sub(product_file, offset + i * byte_size, byte_size, definition, attribute) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                return -1;
            }
            if (coda_mem_array_add_element(array, *attribute) != 0)
            {
                coda_dynamic_type_delete((coda_dynamic_type *)array);
                coda_dynamic_type_delete(*attribute);
                return -1;
            }
        }
    }
    else
    {
        if (read_attribute_sub(product_file, offset, byte_size, definition, attribute) != 0)
        {
            coda_type_release(definition);
            return -1;
        }
        coda_type_release(definition);
    }

    return 0;
}

static int read_AEDR(coda_cdf_product *product_file, int64_t offset, const char *name, int32_t scope)
{
    coda_dynamic_type *attribute;
    int32_t record_type;
    int64_t aedr_next;
    int32_t attr_num;
    int32_t data_type;
    int32_t num;
    int32_t num_elems;

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 5 && record_type != 9)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for AEDR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, offset + 12, 8, &aedr_next) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 20, 4, &attr_num) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 24, 4, &data_type) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 28, 4, &num) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 32, 4, &num_elems) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int64(&aedr_next);
    swap_int32(&attr_num);
    swap_int32(&data_type);
    swap_int32(&num);
    swap_int32(&num_elems);
#endif
    offset += 56;

    if (data_type == 32)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "CDF EPOCH16 data type is not supported");
        return -1;
    }

    if (read_attribute(product_file, offset, data_type, num_elems, &attribute) != 0)
    {
        return -1;
    }
    if (record_type == 5 && (scope & 1))
    {
        if (coda_mem_type_add_attribute((coda_mem_type *)product_file->root_type, name, attribute, 1) != 0)
        {
            coda_dynamic_type_delete(attribute);
            return -1;
        }
    }
    else
    {
        if (num < 0 || num >= product_file->root_type->num_fields)
        {
            coda_set_error(CODA_ERROR_PRODUCT,
                           "CDF Attribute entry number (%d) is outside range of available variables " "[0,%ld]", num,
                           product_file->root_type->num_fields - 1);
            return -1;
        }

        assert(((coda_cdf_variable *)product_file->root_type->field_type[num])->backend == coda_backend_cdf);
        if (coda_cdf_variable_add_attribute((coda_cdf_variable *)product_file->root_type->field_type[num], name,
                                            attribute, 1) != 0)
        {
            coda_dynamic_type_delete(attribute);
            return -1;
        }
    }

    if (read_AEDR(product_file, aedr_next, name, scope) != 0)
    {
        return -1;
    }

    return 0;
}

static int read_ADR(coda_cdf_product *product_file, int64_t offset)
{
    int32_t record_type;
    int64_t adr_next;
    int64_t agredr_head;
    int32_t scope;
    int32_t num;
    int32_t ngr_entries;
    int32_t maxgr_entry;
    int64_t azedr_head;
    int32_t nz_entries;
    int32_t maxz_entry;
    int64_t aedr_head;
    char name[257];

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 4)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for ADR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, offset + 12, 8, &adr_next) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 20, 8, &agredr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 28, 4, &scope) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 32, 4, &num) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 36, 4, &ngr_entries) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 40, 4, &maxgr_entry) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 48, 8, &azedr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 56, 4, &nz_entries) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 60, 4, &maxz_entry) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 68, 256, name) < 0)
    {
        return -1;
    }
    name[256] = '\0';
    rtrim(name);
#ifndef WORDS_BIGENDIAN
    swap_int64(&adr_next);
    swap_int64(&agredr_head);
    swap_int32(&scope);
    swap_int32(&num);
    swap_int32(&ngr_entries);
    swap_int32(&maxgr_entry);
    swap_int64(&azedr_head);
    swap_int32(&nz_entries);
    swap_int32(&maxz_entry);
#endif

    aedr_head = agredr_head;
    if (scope & 1)
    {
        if (nz_entries != 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "gADR record has non-zero NzEntries (%d)", nz_entries);
            return -1;
        }
    }
    else if (ngr_entries == 0)
    {
        aedr_head = azedr_head;
    }

    if (read_AEDR(product_file, aedr_head, name, scope) != 0)
    {
        return -1;
    }

    if (read_ADR(product_file, adr_next) != 0)
    {
        return -1;
    }

    return 0;
}

static int read_VXR(coda_cdf_product *product_file, coda_cdf_variable *variable, int64_t offset, int32_t first,
                    int32_t last);

static int read_VR(coda_cdf_product *product_file, coda_cdf_variable *variable, int64_t offset, int32_t first,
                   int32_t last)
{
    int32_t record_type;

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type == 6)
    {
        return read_VXR(product_file, variable, offset, first, last);
    }
    if (record_type == 7)
    {
        int i;

        if (last >= variable->num_records)
        {
            last = variable->num_records - 1;
        }
        for (i = first; i <= last; i++)
        {
            variable->offset[i] = offset + 12 + (i - first) * variable->num_values_per_record * variable->value_size;
        }
    }
    else if (record_type == 13)
    {
        int64_t csize;
        Bytef *buffer;
        z_stream zs;
        int partial_read = 0;
        int result;
        int i;

        if (first >= variable->num_records)
        {
            /* completely skip this record */
            return 0;
        }

        if (variable->data == NULL)
        {
            variable->data = malloc(variable->num_records * variable->num_values_per_record * variable->value_size);
            if (variable->data == NULL)
            {
                coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                               (long)variable->num_records * variable->num_values_per_record * variable->value_size,
                               __FILE__, __LINE__);
                return -1;
            }
        }

        if (read_bytes(product_file->raw_product, offset + 16, 8, &csize) < 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int64(&csize);
#endif
        offset += 24;

        if (csize < 20)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "Invalid compressed data block for CDF variable");
            return -1;
        }
        buffer = malloc((size_t)csize);
        if (buffer == NULL)
        {
            coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                           (long)csize, __FILE__, __LINE__);
            return -1;
        }
        if (read_bytes(product_file->raw_product, offset, csize, buffer) < 0)
        {
            free(buffer);
            return -1;
        }
        zs.next_in = Z_NULL;
        zs.avail_in = 0;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.msg = NULL;
        /* windowBits is 15 + 16 (adding 16 means that gzip headers are parsed automatically) */
        if (inflateInit2(&zs, 31) != Z_OK)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "could not intialize zip decompression");
            if (zs.msg != NULL)
            {
                coda_add_error_message(" (%s)", zs.msg);
            }
            free(buffer);
            return -1;
        }
        zs.next_in = buffer;
        zs.avail_in = (int)csize;
        if (last >= variable->num_records)
        {
            last = variable->num_records - 1;
            partial_read = 1;
        }
        zs.next_out = (Bytef *)&variable->data[first * variable->num_values_per_record * variable->value_size];
        zs.avail_out = (last - first + 1) * variable->num_values_per_record * variable->value_size;
        result = inflate(&zs, Z_FINISH);
        assert(result != Z_STREAM_ERROR);
        if (result < 0 && !(result == Z_BUF_ERROR && partial_read))
        {
            switch (result)
            {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                    coda_set_error(CODA_ERROR_FILE_READ, "invalid or incomplete compressed data for CDF variable");
                    if (zs.msg != NULL)
                    {
                        coda_add_error_message(" (%s)", zs.msg);
                    }
                    break;
                case Z_MEM_ERROR:
                    coda_set_error(CODA_ERROR_OUT_OF_MEMORY, NULL);
                    break;
                default:
                    coda_set_error(CODA_ERROR_FILE_READ, "error during decompression of CDF variable");
                    if (zs.msg != NULL)
                    {
                        coda_add_error_message(" (%s)", zs.msg);
                    }
            }
            inflateEnd(&zs);
            free(buffer);
            return -1;
        }
        free(buffer);

        if (inflateEnd(&zs) != Z_OK)
        {
            coda_set_error(CODA_ERROR_FILE_READ, "zlib error");
            if (zs.msg != NULL)
            {
                coda_add_error_message(" (%s)", zs.msg);
            }
            return -1;
        }
        for (i = first; i <= last; i++)
        {
            variable->offset[i] = i * variable->num_values_per_record * variable->value_size;
        }
    }
    else
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for VVR record", record_type);
        return -1;
    }

    return 0;
}

static int read_VXR(coda_cdf_product *product_file, coda_cdf_variable *variable, int64_t offset, int32_t first,
                    int32_t last)
{
    int32_t record_type;
    int64_t vxr_next;
    int32_t n_entries;
    int32_t nused_entries;
    int i;

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 6)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for VXR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, offset + 12, 8, &vxr_next) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 20, 4, &n_entries) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 24, 4, &nused_entries) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int64(&vxr_next);
    swap_int32(&n_entries);
    swap_int32(&nused_entries);
#endif
    offset += 28;

    for (i = 0; i < nused_entries; i++)
    {
        int32_t vr_first;
        int32_t vr_last;
        int64_t vr_offset;

        if (read_bytes(product_file->raw_product, offset + i * 4, 4, &vr_first) < 0)
        {
            return -1;
        }
        if (read_bytes(product_file->raw_product, offset + (i + n_entries) * 4, 4, &vr_last) < 0)
        {
            return -1;
        }
        if (read_bytes(product_file->raw_product, offset + (i + n_entries) * 8, 8, &vr_offset) < 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&vr_first);
        swap_int32(&vr_last);
        swap_int64(&vr_offset);
#endif
        if (read_VR(product_file, variable, vr_offset, vr_first, vr_last) != 0)
        {
            return -1;
        }
    }

    if (read_VXR(product_file, variable, vxr_next, first, last) != 0)
    {
        return -1;
    }

    return 0;
}

static int read_CPR(coda_cdf_product *product_file, int64_t offset)
{
    int32_t record_type;
    int32_t ctype;

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 11)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for CPR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, offset + 12, 4, &ctype) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&ctype);
#endif
    if (ctype != 5)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "Unsupported compression method (%d) for CDF variable", ctype);
        return -1;
    }

    return 0;
}

static int read_VDR(coda_cdf_product *product_file, int64_t offset, int is_zvar)
{
    coda_dynamic_type *variable_type;
    coda_cdf_variable *variable;
    int32_t record_type;
    int64_t vdr_next;
    int32_t data_type;
    int32_t max_rec;
    int64_t vxr_head;
    int64_t vxr_tail;
    int32_t flags;
    int32_t srecords;
    int32_t num_elems;
    int32_t num;
    int64_t cpr_spr_offset;
    int32_t blocking_factor;
    char name[257];
    int32_t num_dims;
    int32_t zdim_sizes[CODA_MAX_NUM_DIMS];
    int32_t dim_varys[CODA_MAX_NUM_DIMS];
    int record_varys;
    int has_compression;
    int i;

    if (offset == 0)
    {
        return 0;
    }

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (is_zvar)
    {
        if (record_type != 8)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for zVDR record", record_type);
            return -1;
        }
    }
    else
    {
        if (record_type != 3)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for rVDR record", record_type);
            return -1;
        }
    }

    if (read_bytes(product_file->raw_product, offset + 12, 8, &vdr_next) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 20, 4, &data_type) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 24, 4, &max_rec) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 28, 8, &vxr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 36, 8, &vxr_tail) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 44, 4, &flags) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 48, 4, &srecords) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 64, 4, &num_elems) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 68, 4, &num) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 72, 8, &cpr_spr_offset) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 80, 4, &blocking_factor) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 84, 256, name) < 0)
    {
        return -1;
    }
    name[256] = '\0';
    rtrim(name);
#ifndef WORDS_BIGENDIAN
    swap_int64(&vdr_next);
    swap_int32(&data_type);
    swap_int32(&max_rec);
    swap_int64(&vxr_head);
    swap_int64(&vxr_tail);
    swap_int32(&flags);
    swap_int32(&srecords);
    swap_int32(&num_elems);
    swap_int32(&num);
    swap_int64(&cpr_spr_offset);
    swap_int32(&blocking_factor);
#endif

    if (data_type == 32)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "CDF EPOCH16 data type is not supported");
        return -1;
    }
    if (is_zvar)
    {
        if (read_bytes(product_file->raw_product, offset + 340, 4, &num_dims) < 0)
        {
            return -1;
        }
#ifndef WORDS_BIGENDIAN
        swap_int32(&num_dims);
#endif
        if (num_dims < 0)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "CDF variable '%s' has invalid number of dimensions (%d)", name,
                           num_dims);
            return -1;
        }
        if (num_dims > CODA_MAX_NUM_DIMS)
        {
            coda_set_error(CODA_ERROR_PRODUCT, "CDF variable '%s' has too many dimensions (%d)", name, num_dims);
            return -1;
        }
        offset += 344;
        if (num_dims > 0)
        {
            for (i = 0; i < num_dims; i++)
            {
                if (read_bytes(product_file->raw_product, offset + i * 4, 4, &zdim_sizes[i]) < 0)
                {
                    return -1;
                }
#ifndef WORDS_BIGENDIAN
                swap_int32(&zdim_sizes[i]);
#endif
            }
            offset += num_dims * 4;
        }
    }
    else
    {
        num_dims = product_file->rnum_dims;
    }

    if (num_dims > 0)
    {
        for (i = 0; i < num_dims; i++)
        {
            if (read_bytes(product_file->raw_product, offset + i * 4, 4, &dim_varys[i]) < 0)
            {
                return -1;
            }
#ifndef WORDS_BIGENDIAN
            swap_int32(&dim_varys[i]);
#endif
        }
        offset += num_dims * 4;
    }
    record_varys = flags & 1;
    /* int has_pad_value = flags & 2; */
    has_compression = flags & 4;
    /* int64_t pad_value_offset = offset; */
    if (!record_varys && max_rec != 0)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF variable '%s' has non-varying record dimension but number of records "
                       "(%d) is not equal to 1", name, max_rec + 1);
        return -1;
    }

    if (has_compression && cpr_spr_offset != -1)
    {
        if (read_CPR(product_file, cpr_spr_offset) != 0)
        {
            return -1;
        }
    }
    if (product_file->root_type->num_fields != num)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF variable has invalid number '%d', expected '%ld'", num,
                       product_file->root_type->num_fields);
        return -1;
    }

    if (is_zvar)
    {
        variable_type = coda_cdf_variable_new(data_type, max_rec, record_varys, num_dims, zdim_sizes, dim_varys,
                                              product_file->array_ordering, num_elems, srecords, &variable);
    }
    else
    {
        variable_type = coda_cdf_variable_new(data_type, max_rec, record_varys, num_dims, product_file->rdim_sizes,
                                              dim_varys, product_file->array_ordering, num_elems, srecords, &variable);
    }
    if (variable_type == NULL)
    {
        return -1;
    }
    if (coda_mem_record_add_field(product_file->root_type, name, variable_type, 1) != 0)
    {
        coda_cdf_type_delete((coda_dynamic_type *)variable_type);
        return -1;
    }

    if (read_VXR(product_file, variable, vxr_head, 0, -1) != 0)
    {
        return -1;
    }

    if (read_VDR(product_file, vdr_next, is_zvar) != 0)
    {
        return -1;
    }

    return 0;
}

static int read_GDR(coda_cdf_product *product_file, int64_t offset)
{
    int32_t record_type;
    int64_t rvdr_head;
    int64_t zvdr_head;
    int64_t adr_head;
    int64_t eof;
    int32_t nr_vars;
    int32_t num_attr;
    int32_t nz_vars;

    if (read_bytes(product_file->raw_product, offset + 8, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 2)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for GDR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, offset + 12, 8, &rvdr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 20, 8, &zvdr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 28, 8, &adr_head) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 36, 8, &eof) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 44, 4, &nr_vars) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 48, 4, &num_attr) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 56, 4, &product_file->rnum_dims) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, offset + 60, 4, &nz_vars) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int64(&rvdr_head);
    swap_int64(&zvdr_head);
    swap_int64(&adr_head);
    swap_int64(&eof);
    swap_int32(&nr_vars);
    swap_int32(&num_attr);
    swap_int32(&product_file->rnum_dims);
    swap_int32(&nz_vars);
#endif

    if (eof != product_file->file_size)
    {
        char s1[21];
        char s2[21];

        coda_str64(eof, s1);
        coda_str64(product_file->file_size, s2);
        coda_set_error(CODA_ERROR_PRODUCT, "CDF end of file position (%s) does not match file size (%s)", s1, s2);
        return -1;
    }

    if (rvdr_head != 0)
    {
        if (read_VDR(product_file, rvdr_head, 0) != 0)
        {
            return -1;
        }
    }

    if (zvdr_head != 0)
    {
        if (read_VDR(product_file, zvdr_head, 1) != 0)
        {
            return -1;
        }
    }

    if (read_ADR(product_file, adr_head) != 0)
    {
        return -1;
    }

    return 0;
}

static int read_file(coda_cdf_product *product_file)
{
    int32_t record_type;
    int64_t gdr_offset;
    int32_t encoding;
    int32_t flags;

    /* CDF Descriptor Record */
    if (read_bytes(product_file->raw_product, 16, 4, &record_type) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int32(&record_type);
#endif
    if (record_type != 1)
    {
        coda_set_error(CODA_ERROR_PRODUCT, "CDF file has invalid record type (%d) for CDR record", record_type);
        return -1;
    }

    if (read_bytes(product_file->raw_product, 20, 8, &gdr_offset) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, 28, 4, &product_file->cdf_version) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, 32, 4, &product_file->cdf_release) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, 36, 4, &encoding) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, 40, 4, &flags) < 0)
    {
        return -1;
    }
    if (read_bytes(product_file->raw_product, 52, 4, &product_file->cdf_increment) < 0)
    {
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_int64(&gdr_offset);
    swap_int32(&product_file->cdf_version);
    swap_int32(&product_file->cdf_release);
    swap_int32(&encoding);
    swap_int32(&flags);
    swap_int32(&product_file->cdf_increment);
#endif

    switch (encoding)
    {
        case 1:        /* NETWORK */
        case 2:        /* SUN */
        case 5:        /* SGi */
        case 7:        /* IBMRS */
        case 9:        /* MAC */
        case 11:       /* HP */
        case 12:       /* NeXT */
            product_file->endianness = coda_big_endian;
            break;
        case 4:        /* DECSTATION */
        case 6:        /* IBMPC */
        case 13:       /* ALPHAOSF1 */
        case 16:       /* ALPHAVMSi */
            product_file->endianness = coda_little_endian;
            break;
        case 3:        /* VAX */
        case 14:       /* ALPHAVMSd */
        case 15:       /* ALPHAVMSg */
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "CDF file contains unsupported floating point format "
                           "(only IEEE 754 floating point format is supported)");
            return -1;
        default:
            coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "CDF file has unsupported encoding %d", encoding);
            return -1;
    }
    if (flags & 1)
    {
        /* row-major ordering */
        product_file->array_ordering = coda_array_ordering_c;
    }
    else
    {
        /* column-major ordering */
        product_file->array_ordering = coda_array_ordering_fortran;
    }
    if (!(flags & 2))
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "multi-file CDF is not supported");
        return -1;
    }
    product_file->has_md5_chksum = ((flags & 4) && (flags & 8));

    /* Global Descriptor Record */
    if (read_GDR(product_file, gdr_offset) != 0)
    {
        return -1;
    }

    return 0;
}

int coda_cdf_reopen(coda_product **product)
{
    coda_cdf_product *product_file;
    coda_type_record *root_definition;
    uint32_t magic[2];

    product_file = (coda_cdf_product *)malloc(sizeof(coda_cdf_product));
    if (product_file == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not allocate %lu bytes) (%s:%u)",
                       sizeof(coda_cdf_product), __FILE__, __LINE__);
        coda_close(*product);
        return -1;
    }
    product_file->filename = NULL;
    product_file->file_size = (*product)->file_size;
    product_file->format = coda_format_cdf;
    product_file->root_type = NULL;
    product_file->product_definition = NULL;
    product_file->product_variable_size = NULL;
    product_file->product_variable = NULL;
    product_file->mem_size = 0;
    product_file->mem_ptr = NULL;

    product_file->raw_product = *product;

    product_file->filename = strdup((*product)->filename);
    if (product_file->filename == NULL)
    {
        coda_set_error(CODA_ERROR_OUT_OF_MEMORY, "out of memory (could not duplicate filename string) (%s:%u)",
                       __FILE__, __LINE__);
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }

    /* magic */
    if (read_bytes(product_file->raw_product, 0, 8, magic) < 0)
    {
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }
#ifndef WORDS_BIGENDIAN
    swap_uint32(&magic[0]);
    swap_uint32(&magic[1]);
#endif
    if (magic[0] == 0x0000FFFF || magic[0] == 0xCDF26002)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "CDF format version older than 3.0 is not supported");
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }
    if (magic[1] == 0xCCCC0001)
    {
        coda_set_error(CODA_ERROR_UNSUPPORTED_PRODUCT, "full file compression not supported for CDF files");
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }
    assert(magic[0] == 0xCDF30001 && magic[1] == 0x0000FFFF);

    /* create root type */
    root_definition = coda_type_record_new(coda_format_cdf);
    if (root_definition == NULL)
    {
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }
    product_file->root_type = coda_mem_record_new(root_definition, NULL);
    if (product_file->root_type == NULL)
    {
        coda_cdf_close((coda_product *)product_file);
        coda_type_release((coda_type *)root_definition);
        return -1;
    }
    coda_type_release((coda_type *)root_definition);

    if (read_file(product_file) != 0)
    {
        coda_cdf_close((coda_product *)product_file);
        return -1;
    }

    *product = (coda_product *)product_file;

    return 0;
}

int coda_cdf_close(coda_product *product)
{
    coda_cdf_product *product_file = (coda_cdf_product *)product;

    if (product_file->filename != NULL)
    {
        free(product_file->filename);
    }
    if (product_file->root_type != NULL)
    {
        coda_dynamic_type_delete((coda_dynamic_type *)product_file->root_type);
    }
    if (product_file->mem_ptr != NULL)
    {
        free(product_file->mem_ptr);
    }
    if (product_file->raw_product != NULL)
    {
        coda_bin_close((coda_product *)product_file->raw_product);
    }

    free(product_file);

    return 0;
}
