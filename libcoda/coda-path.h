/*
 * Copyright (C) 2007-2015 S[&]T, The Netherlands.
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

#ifndef CODA_PATH_H
#define CODA_PATH_H

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C"
{
#endif
/* *INDENT-ON* */

/* Returns the full path (including program name) for your application.
 * Pass argv[0] of main() as first parameter.
 * The function will search PATH if it can't determine the path from argv0.
 * If the path could not be determined, location is set to NULL.
 */
int coda_path_for_program(const char *argv0, char **location);

/* Returns the path (including filename) for the given filename by searching the searchpath
 * searchpath is a string containing a list of paths to search (';' separator on Windows, ':' separator otherwise).
 * The returned path will be the concatenation of the searchpath location + '/' + the filename.
 * If the path could not be determined, location is set to NULL.
 */
int coda_path_find_file(const char *searchpath, const char *filename, char **location);

/* This function creates a new path from existing path components.
 * if is_filepath is set to 1 this means that initialpath is a path to a file and the filename will first be removed
 * before the 'appendpath' part is appended. If initialpath is not a filepath use is_filepath = 0
 * Example:
 *   coda_path_from_path("/usr/local/bin/foo", 1, "../share/foo/doc", &path)
 * This will give: path = "/usr/local/share/foo/doc"
 *
 * appendpath may be NULL in which case resultpath will be a (newly allocated) clean version of initialpath
 */
int coda_path_from_path(const char *initialpath, int is_filepath, const char *appendpath, char **resultpath);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
