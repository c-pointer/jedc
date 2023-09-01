/*
 *	file related utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#if !defined(__C_FILES_H)
#define __C_FILES_H

#include <stdbool.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

// returns true if the file is a directory
int_t c_isdir(const char *file);

// returns true if the file is a regular file
int_t c_isreg(const char *file);

// untested
char *get_file_list_string(const char *directory, const char *pattern);
char **get_file_list_strlist(const char *directory, const char *pattern, size_t *count);
char *file_complete(const char *root);

// returns true if file is in directory
int_t find_in_dir(const char *dir, const char *file);

// returns a newly allocated full-path filename, if file is in path;
// otherwise returns NULL. The c_del is the character that separates
// the path directories (:).
char *find_in_path(const char *path, const char *file, char c_del);

// unit initialization
size_t c_files_init();

#if defined(__cplusplus)
}
#endif

#endif
