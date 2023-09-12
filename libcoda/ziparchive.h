/*
 * Copyright (C) 2007-2023 S[&]T, The Netherlands.
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

#ifndef ZIPARCHIVE_H
#define ZIPARCHIVE_H

#define za_open coda_za_open
#define za_get_filename coda_za_get_filename
#define za_get_num_entries coda_za_get_num_entries
#define za_get_entry_by_index coda_za_get_entry_by_index
#define za_get_entry_by_name coda_za_get_entry_by_name
#define za_get_entry_size coda_za_get_entry_size
#define za_get_entry_name coda_za_get_entry_name
#define za_read_entry coda_za_read_entry
#define za_close coda_za_close

typedef struct za_entry_struct za_entry;
typedef struct za_file_struct za_file;

za_file *za_open(const char *filename, void (*error_handler)(const char *, ...));

const char *za_get_filename(za_file *zf);
long za_get_num_entries(za_file *zf);
za_entry *za_get_entry_by_index(za_file *zf, long index);
za_entry *za_get_entry_by_name(za_file *zf, const char *name);

long za_get_entry_size(za_entry *entry);
const char *za_get_entry_name(za_entry *entry);
int za_read_entry(za_entry *entry, char *buffer);

void za_close(za_file *zf);

#endif
