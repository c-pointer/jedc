/*
 *	CBRIEF string library
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#if !defined(__C_STRING_H)
#define __C_STRING_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <stdlib.h>
#include "slang.h"
#include "c-types.h"

uint_t	xdigit(uint_t ch);					// value of hex digit character
const char *strwc(const char *source);		// NULL if no wildcard character
size_t count_char(const char *str, char c);	// count character c in string

// wc <-> u8, dynamic allocated buffer
wchar_t*	u8towcs(const char *str);
char*		u8wcsto(const wchar_t *str);
size_t		u8strlen(const char *s);

// preallocated buffer
char		*u8get(char *dest, const wchar_t *source);
wchar_t		*u8set(wchar_t *dest, const char *source);

/* ----------------------------------------------------------------------
 * wchar_t string edit, all returns the new position
 * ------------------------------------------------------------------- */
size_t wcs_inswc    (wchar_t *dest, size_t pos, wchar_t wch); 		// insert character in pos
size_t wcs_backspace(wchar_t *dest, size_t pos);					// backspace at pos
size_t wcs_delwc    (wchar_t *dest, size_t pos);					// delete character at pos
size_t wcs_delbol   (wchar_t *dest, size_t pos);					// delete to begin of line
size_t wcs_deleol   (wchar_t *dest, size_t pos);					// delete to end of line
size_t wcs_insert   (wchar_t *dest, size_t pos, wchar_t *substr);	// insert string at pos
size_t wcs_delete   (wchar_t *dest, size_t pos, size_t len);		// delete len characters at pos

/* ----------------------------------------------------------------------
 * string list routines (dynamic allocation array)
 * ------------------------------------------------------------------- */
char **strlist_create();
char **strlist_destroy(char **list, size_t *count);
char **strlist_append(char **list, const char *source, size_t *p_count);
char **text_to_strlist(const char *source, char ch_delim, size_t *count);
char **strlist_append_text(char **list, const char *source, char cdel, size_t *p_count);
char *strlist_to_text (const char **list, char ch_delim, size_t count);
void strlist_sort(char **list, size_t count);
size_t strlist_find(const char *name, const char **list, size_t count);
size_t strlist_bsearch(const char *name, const char **list, size_t count);

// for completion
size_t	strlist_find_first(const char *name, size_t selected, const char **list, size_t count);
size_t	strlist_find_last (const char *name, size_t selected, const char **list, size_t count);
size_t	strlist_find_next (const char *name, size_t selected, const char **list, size_t count);
size_t	strlist_find_prev (const char *name, size_t selected, const char **list, size_t count);

// copy a S-Lang array to a new strlist
char **array_to_strlist(SLang_Array_Type *at, size_t *pcount);
// returns a S-Lang array char**data; DO NOT FREED
char **get_array_ptrs(SLang_Array_Type *at, size_t *pcount);
// converts a strlist to S-Lang array type
SLang_Array_Type *strlist_to_array(const char **list, size_t count);
// S-Lang intern version, pushing the results to SL's stack
void strlist_to_array_sl(size_t *plist, size_t *pcount);

// init module
size_t c_string_init();

#if defined(__cplusplus)
}
#endif

#endif
