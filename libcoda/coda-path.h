/*
 * Copyright (C) 2007-2021 S[&]T, The Netherlands.
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
