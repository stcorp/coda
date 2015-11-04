/*
 * Copyright (C) 2007-2010 S[&]T, The Netherlands.
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

typedef struct zaEntry_struct zaEntry;
typedef struct zaFile_struct zaFile;

zaFile *za_open(const char *filename, void (*error_handler) (const char *, ...));

const char *za_get_filename(zaFile *zf);
long za_get_num_entries(zaFile *zf);
zaEntry *za_get_entry_by_index(zaFile *zf, long index);
zaEntry *za_get_entry_by_name(zaFile *zf, const char *name);

long za_get_entry_size(zaEntry *entry);
const char *za_get_entry_name(zaEntry *entry);
int za_read_entry(zaEntry *entry, char *buffer);

void za_close(zaFile *zf);

#endif
