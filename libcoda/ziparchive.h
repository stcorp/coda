/*
 * Copyright (C) 2007-2016 S[&]T, The Netherlands.
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

za_file *za_open(const char *filename, void (*error_handler) (const char *, ...));

const char *za_get_filename(za_file *zf);
long za_get_num_entries(za_file *zf);
za_entry *za_get_entry_by_index(za_file *zf, long index);
za_entry *za_get_entry_by_name(za_file *zf, const char *name);

long za_get_entry_size(za_entry *entry);
const char *za_get_entry_name(za_entry *entry);
int za_read_entry(za_entry *entry, char *buffer);

void za_close(za_file *zf);

#endif
