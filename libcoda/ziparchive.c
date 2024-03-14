/*
 * Copyright (C) 2007-2024 S[&]T, The Netherlands.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ziparchive.h"
#include "hashtable.h"
#ifdef WORDS_BIGENDIAN
#include "coda-swap2.h"
#include "coda-swap4.h"
#endif

#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "zlib.h"

/* for definition of uint16_t and uint32_t */
#include "coda.h"

#define BUFFSIZE 8192

static void default_error_handler(const char *message, ...)
{
    va_list ap;

    fprintf(stderr, "ERROR: ");
    va_start(ap, message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

struct za_entry_struct
{
    uint32_t localheader_offset;
    uint16_t compression;
    uint16_t modification_time;
    uint16_t modification_date;
    uint32_t attributes;

    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;

    uint16_t filename_length;
    uint16_t extrafield_length;

    int ascii;  /* is it an ASCII file */

    char *filename;

    struct za_file_struct *zf;
};

struct za_file_struct
{
    int fd;
    int num_entries;
    char *filename;
    int64_t file_size;
    za_entry *entry;
    hashtable *hash_data;
    void (*handle_error)(const char *, ...);
};

static int get_entries(za_file *zf)
{
    union
    {
        int8_t as_int8[48];
        uint16_t as_uint16[24];
        uint32_t as_uint32[12];
    } buffer;
    uint32_t signature;
    uint32_t offset;
    uint16_t num_entries;
    long result;
    int i;

    /* read the 'end of central directory record' */
    if (lseek(zf->fd, -22, SEEK_END) < 0)
    {
        zf->handle_error(strerror(errno));
        return -1;
    }

    if (read(zf->fd, buffer.as_int8, 22) < 0)
    {
        zf->handle_error(strerror(errno));
        return -1;
    }

    signature = buffer.as_uint32[0];
#ifdef WORDS_BIGENDIAN
    swap_uint32(&signature);
#endif
    if (signature != 0x06054b50)
    {
        /* There is probably a zipfile comment at the end -> bail out */
        /* In the future, if needed, we could implement an approach that just reads all local file headers */
        zf->handle_error("could not locate package index in zip file '%s'. There is probably a 'zip file comment' at "
                         "the end of the file (which is not supported)", zf->filename);
        return -1;
    }

    num_entries = buffer.as_uint16[4];  /* offset 8 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&num_entries);
#endif

    offset = buffer.as_uint32[4];       /* offset 16 */
#ifdef WORDS_BIGENDIAN
    swap_uint32(&offset);
#endif

    zf->num_entries = num_entries;
    zf->entry = malloc(num_entries * sizeof(za_entry));
    if (zf->entry == NULL)
    {
        zf->handle_error("could not allocate %ld bytes", num_entries * sizeof(za_entry));
        return -1;
    }
    for (i = 0; i < num_entries; i++)
    {
        zf->entry[i].filename = NULL;
        zf->entry[i].zf = zf;
    }

    if (lseek(zf->fd, offset, SEEK_SET) < 0)
    {
        zf->handle_error(strerror(errno));
        return -1;
    }

    for (i = 0; i < num_entries; i++)
    {
        uint16_t comment_length;
        uint16_t internal_attributes;
        za_entry *entry;

        entry = &zf->entry[i];

        /* read the constant length part of the central directory 'file header' */
        result = read(zf->fd, buffer.as_int8, 46);
        if (result < 0)
        {
            zf->handle_error(strerror(errno));
            return -1;
        }
        if (result != 46)
        {
            entry->zf->handle_error("unexpected end in zip file");
            return -1;
        }

        signature = buffer.as_uint32[0];
#ifdef WORDS_BIGENDIAN
        swap_uint32(&signature);
#endif
        if (signature != 0x02014b50)
        {
            /* not a central directory file header */
            zf->handle_error("invalid file header signature in zip file '%s'", zf->filename);
            return -1;
        }

        /* uint16_t version1 = buffer.as_uint16[2]; */
        /* uint16_t version2 = buffer.as_uint16[3]; */
        /* uint16_t bitflag = buffer.as_uint16[4]; */

        entry->compression = buffer.as_uint16[5];       /* offset 10 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&entry->compression);
#endif
        if (entry->compression != 0 && entry->compression != 8)
        {
            zf->handle_error("unsupported compression for entry in zip file '%s'", zf->filename);
            return -1;
        }

        entry->modification_time = buffer.as_uint16[6]; /* offset 12 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&entry->modification_time);
#endif

        entry->modification_date = buffer.as_uint16[7]; /* offset 14 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&entry->modification_date);
#endif

        entry->crc = buffer.as_uint32[4];       /* offset 16 */
#ifdef WORDS_BIGENDIAN
        swap_uint32(&entry->crc);
#endif

        entry->compressed_size = buffer.as_uint32[5];   /* offset 20 */
#ifdef WORDS_BIGENDIAN
        swap_uint32(&entry->compressed_size);
#endif

        entry->uncompressed_size = buffer.as_uint32[6]; /* offset 24 */
#ifdef WORDS_BIGENDIAN
        swap_uint32(&entry->uncompressed_size);
#endif

        entry->filename_length = buffer.as_uint16[14];  /* offset 28 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&entry->filename_length);
#endif

        entry->extrafield_length = buffer.as_uint16[15];        /* offset 30 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&entry->extrafield_length);
#endif

        comment_length = buffer.as_uint16[16];  /* offset 32 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&comment_length);
#endif

        /* skip disk number start (2 bytes) */

        internal_attributes = buffer.as_uint16[18];     /* offset 36 */
#ifdef WORDS_BIGENDIAN
        swap_uint16(&internal_attributes);
#endif
        entry->ascii = internal_attributes & 0x1;

        /* we can't directly access 32 bit values if they are not on a 32 bit boundary with regard to the start of
         * 'buffer'. We therefore use memcpy for these cases. */
        memcpy(&entry->attributes, &buffer.as_int8[38], 4);
#ifdef WORDS_BIGENDIAN
        swap_uint32(&entry->attributes);
#endif

        memcpy(&entry->localheader_offset, &buffer.as_int8[42], 4);
#ifdef WORDS_BIGENDIAN
        swap_uint32(&entry->localheader_offset);
#endif

        entry->filename = malloc(entry->filename_length + 1);
        if (entry->filename == NULL)
        {
            zf->handle_error("could not allocate %d bytes", (int)entry->filename_length + 1);
            return -1;
        }

        result = read(zf->fd, entry->filename, entry->filename_length);
        if (result < 0)
        {
            zf->handle_error(strerror(errno));
            return -1;
        }
        if (((uint32_t)result) != entry->filename_length)
        {
            entry->zf->handle_error("unexpected end in zip file");
            return -1;
        }
        entry->filename[entry->filename_length] = '\0';

        if (hashtable_add_name(zf->hash_data, entry->filename) != 0)
        {
            zf->handle_error("zip file '%s' contains two entries with the same name '%s'", zf->filename,
                             entry->filename);
            return -1;
        }

        if (lseek(zf->fd, entry->extrafield_length + comment_length, SEEK_CUR) < 0)
        {
            zf->handle_error(strerror(errno));
            return -1;
        }

        /* perform some quick sanity checks on the offsets and sizes */
        if (entry->localheader_offset > zf->file_size)
        {
            entry->zf->handle_error("invalid zip file (local header offset exceeds file size)");
            return -1;
        }
        if (entry->compression != 0)
        {
            if (entry->localheader_offset + entry->compressed_size > zf->file_size)
            {
                entry->zf->handle_error("invalid zip file (entry size exceeds file size)");
                return -1;
            }
            /* for zlib the theoretical maximum compression factor is 1032 */
            if (entry->uncompressed_size / 1032 > entry->compressed_size + 1)
            {
                entry->zf->handle_error("invalid uncompressed size in zip file");
                return -1;
            }
        }
        else
        {
            if (entry->compressed_size != entry->uncompressed_size)
            {
                entry->zf->handle_error("invalid zip file (compressed and uncompressed sizes should be equal for "
                                        "uncompressed entry)");
                return -1;
            }
            if (entry->localheader_offset + entry->uncompressed_size > zf->file_size)
            {
                entry->zf->handle_error("invalid zip file (entry size exceeds file size)");
                return -1;
            }
        }
    }

    return 0;
}

za_file *za_open(const char *filename, void (*error_handler)(const char *, ...))
{
    struct stat statbuf;
    za_file *zf;
    char buffer[2];
    int open_flags;

    if (stat(filename, &statbuf) != 0)
    {
        if (errno == ENOENT)
        {
            error_handler("could not find %s", filename);
        }
        else
        {
            error_handler("could not open %s (%s)", filename, strerror(errno));
        }
        return NULL;
    }
    if ((statbuf.st_mode & S_IFREG) == 0)
    {
        error_handler("could not open %s (not a regular file)", filename);
        return NULL;
    }
    if (statbuf.st_size < 22)
    {
        error_handler("could not open %s (not a zip file)", filename);
        return NULL;
    }

    zf = malloc(sizeof(za_file));
    if (zf == NULL)
    {
        if (error_handler != NULL)
        {
            error_handler("could not allocate %ld bytes", sizeof(za_file));
        }
        return NULL;
    }
    zf->filename = strdup(filename);
    if (zf->filename == NULL)
    {
        free(zf);
        if (error_handler != NULL)
        {
            error_handler("could not duplicate string");
        }
        return NULL;
    }
    zf->file_size = statbuf.st_size;
    zf->num_entries = 0;
    zf->entry = NULL;
    zf->hash_data = NULL;
    if (error_handler != NULL)
    {
        zf->handle_error = error_handler;
    }
    else
    {
        zf->handle_error = default_error_handler;
    }

    open_flags = O_RDONLY;
#ifdef WIN32
    open_flags |= _O_BINARY;
#endif
    zf->fd = open(filename, open_flags);
    if (zf->fd < 0)
    {
        zf->handle_error("could not open file '%s' (%s)\n", filename, strerror(errno));
        free(zf->filename);
        free(zf);
        return NULL;
    }

    if (read(zf->fd, buffer, 2) < 0)
    {
        zf->handle_error(strerror(errno));
        za_close(zf);
        return NULL;
    }
    if (buffer[0] != 'P' || buffer[1] != 'K')
    {
        error_handler("could not open %s (not a zip file)", filename);
        za_close(zf);
        return NULL;
    }

    zf->hash_data = hashtable_new(1);
    if (get_entries(zf) != 0)
    {
        za_close(zf);
        return NULL;
    }

    return zf;
}

long za_get_num_entries(za_file *zf)
{
    return zf->num_entries;
}

const char *za_get_filename(za_file *zf)
{
    return zf->filename;
}

za_entry *za_get_entry_by_index(za_file *zf, long index)
{
    if (index < 0 || index >= zf->num_entries)
    {
        return NULL;
    }
    return &zf->entry[index];
}

za_entry *za_get_entry_by_name(za_file *zf, const char *name)
{
    long index;

    index = hashtable_get_index_from_name(zf->hash_data, name);
    if (index < 0)
    {
        return NULL;
    }

    return &zf->entry[index];
}

long za_get_entry_size(za_entry *entry)
{
    return entry->uncompressed_size;
}

const char *za_get_entry_name(za_entry *entry)
{
    return entry->filename;
}

int za_read_entry(za_entry *entry, char *out_buffer)
{
    union
    {
        int8_t as_int8[32];
        uint16_t as_uint16[16];
        uint32_t as_uint32[8];
    } buffer;
    uint32_t signature;
    uint16_t compression;
    uint16_t modification_time;
    uint16_t modification_date;
    uint32_t crc;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_length;
    uint16_t extrafield_length;
    long result;

    if (lseek(entry->zf->fd, entry->localheader_offset, SEEK_SET) < 0)
    {
        entry->zf->handle_error(strerror(errno));
        return -1;
    }

    /* read the constant length part of the 'local file header' */
    result = read(entry->zf->fd, buffer.as_int8, 30);
    if (result < 0)
    {
        entry->zf->handle_error(strerror(errno));
        return -1;
    }
    if (result != 30)
    {
        entry->zf->handle_error("unexpected end in zip file");
        return -1;
    }

    signature = buffer.as_uint32[0];
#ifdef WORDS_BIGENDIAN
    swap_uint32(&signature);
#endif
    if (signature != 0x04034b50)
    {
        /* Not a local header! */
        entry->zf->handle_error("error in zip file (local header has incorrect signature)");
        return -1;
    }

    /* uint16_t version2 = buffer.as_uint16[2];  */
    /* uint16_t bitflag = buffer.as_uint16[3]; */

    compression = buffer.as_uint16[4];  /* offset 8 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&compression);
#endif
    if (compression != entry->compression)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(compression)");
        return -1;
    }

    modification_time = buffer.as_uint16[5];    /* offset 10 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&modification_time);
#endif
    if (modification_time != entry->modification_time)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(modification_time)");
        return -1;
    }

    modification_date = buffer.as_uint16[6];    /* offset 12 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&modification_date);
#endif
    if (modification_date != entry->modification_date)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(modification_date)");
        return -1;
    }

    /* we can't directly access 32 bit values if they are not on a 32 bit boundary with regard to the start of
     * 'buffer'. We therefore use memcpy for these cases. */
    memcpy(&crc, &buffer.as_int8[14], 4);
#ifdef WORDS_BIGENDIAN
    swap_uint32(&crc);
#endif
    if (crc != entry->crc)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file (crc)");
        return -1;
    }

    memcpy(&compressed_size, &buffer.as_int8[18], 4);
#ifdef WORDS_BIGENDIAN
    swap_uint32(&compressed_size);
#endif
    if (compressed_size != entry->compressed_size)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(compressed_size)");
        return -1;
    }

    memcpy(&uncompressed_size, &buffer.as_int8[22], 4);
#ifdef WORDS_BIGENDIAN
    swap_uint32(&uncompressed_size);
#endif
    if (uncompressed_size != entry->uncompressed_size)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(uncompressed_size)");
        return -1;
    }

    filename_length = buffer.as_uint16[13];     /* offset 26 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&filename_length);
#endif
    if (filename_length != entry->filename_length)
    {
        entry->zf->handle_error("inconsistency between local file header and central directory in zip file "
                                "(filename_length)");
        return -1;
    }

    extrafield_length = buffer.as_uint16[14];   /* offset 28 */
#ifdef WORDS_BIGENDIAN
    swap_uint16(&extrafield_length);
#endif
    /* the extra field information is allowed to be different between the local file header and central directory! */

    if (lseek(entry->zf->fd, filename_length + extrafield_length, SEEK_CUR) < 0)
    {
        entry->zf->handle_error(strerror(errno));
        return -1;
    }

    if (entry->compression == 0)
    {
        result = read(entry->zf->fd, out_buffer, entry->uncompressed_size);
        if (result < 0)
        {
            entry->zf->handle_error(strerror(errno));
            return -1;
        }
        if (((uint32_t)result) != entry->uncompressed_size)
        {
            entry->zf->handle_error("unexpected end in zip file");
            return -1;
        }
    }
    else
    {
        Bytef *in_buffer;
        z_stream zs;
        int result;

        in_buffer = malloc(entry->compressed_size);
        if (in_buffer == NULL)
        {
            entry->zf->handle_error("could not allocate %ld bytes", (long)entry->compressed_size);
            return -1;
        }

        result = read(entry->zf->fd, in_buffer, entry->compressed_size);
        if (result < 0)
        {
            entry->zf->handle_error(strerror(errno));
            free(in_buffer);
            return -1;
        }
        if (((uint32_t)result) != entry->compressed_size)
        {
            entry->zf->handle_error("unexpected end in zip file");
            free(in_buffer);
            return -1;
        }

        zs.next_in = Z_NULL;
        zs.avail_in = 0;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.msg = NULL;

        /* the negative value for windowBits is an undocumented zlib option: it instructs zlib to bypass the zlib header */
        if (inflateInit2(&zs, -15) != Z_OK)
        {
            if (zs.msg != NULL)
            {
                entry->zf->handle_error("%s", zs.msg);
            }
            else
            {
                entry->zf->handle_error("could not intialize zip decompression");
            }
            free(in_buffer);
            return -1;
        }

        zs.next_in = in_buffer;
        zs.avail_in = entry->compressed_size;
        zs.next_out = (Bytef *)out_buffer;
        zs.avail_out = entry->uncompressed_size;
        result = inflate(&zs, Z_FINISH);
        assert(result != Z_STREAM_ERROR);
        switch (result)
        {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
                entry->zf->handle_error("invalid or incomplete deflate data for zip entry");
                inflateEnd(&zs);
                free(in_buffer);
                return -1;
            case Z_MEM_ERROR:
                entry->zf->handle_error("out of memory");
                inflateEnd(&zs);
                free(in_buffer);
                return -1;
        }
        free(in_buffer);

        if (inflateEnd(&zs) != Z_OK)
        {
            if (zs.msg != NULL)
            {
                entry->zf->handle_error("zlib error (%s)", zs.msg);
            }
            else
            {
                entry->zf->handle_error("unknown zlib error");
            }
            return -1;
        }
        if (zs.avail_in != 0 || zs.avail_out != 0)
        {
            entry->zf->handle_error("invalid compressed/uncompressed size for zip entry");
            return -1;
        }
    }

    return 0;
}

void za_close(za_file *zf)
{
    close(zf->fd);
    if (zf->entry != NULL)
    {
        int i;

        for (i = 0; i < zf->num_entries; i++)
        {
            if (zf->entry[i].filename != NULL)
            {
                free(zf->entry[i].filename);
            }
        }
        free(zf->entry);
    }
    if (zf->filename != NULL)
    {
        free(zf->filename);
    }
    if (zf->hash_data != NULL)
    {
        hashtable_delete(zf->hash_data);
    }
    free(zf);
}
