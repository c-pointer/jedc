/*
 *	CBRIEF string library
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include <stdint.h>		// ISO C
#include <wchar.h>		// ISO C
#include <limits.h>		// ISO C
#include <stdio.h>		// ISO C
#include <string.h>		// ISO C
#include <ctype.h>		// ISO C
#include <stdlib.h>		// ISO C

#include "c-types.h"
#include "c-string.h"
#include "c-misc.h"

#define BUFSZ			4096	// i386 optimized, one page
#define	TEXTLINESZ		128

#define LIST_PREALLOC	1024	// auto-icreasing block for string lists
								// does not matter, uinitialized pointers are if not used

static inline int_t  min(int_t a, int_t b)    { return (a < b) ? a : b; }
static inline size_t umin(size_t a, size_t b) { return (a < b) ? a : b; }
static inline size_t umax(size_t a, size_t b) { return (a > b) ? a : b; }

// hexadecimal character to number
uint_t xdigit(uint_t c) {
	if ( c >= '0' && c <= '9' ) return c - '0';
	if ( c >= 'A' && c <= 'F' ) return 10 + (c - 'A');
	if ( c >= 'a' && c <= 'f' ) return 10 + (c - 'a');
	return 0;
	}

// returns the pointer to wildcard if found in the string 'source';
// otherwise returns NULL
const char *strwc(const char *source) {
	const char *p = source;
	while ( *p ) {
		if ( *p == '*' || *p == '?' || *p == '[' || *p == ']' ) return p;
		p ++;
		}
	return NULL;
	}

//* returns the size of UTF8 string in wchar
size_t u8strlen(const char *s)
{ return mbstowcs(NULL, s, 0); }

//	convert utf8 string to wchar string
wchar_t *u8towcs(const char *str) {
	size_t  wlen = mbstowcs(NULL, str, 0);
	wchar_t *wcs;
	
	if ( wlen == (size_t) -1 )
		wlen = 0;
	wcs = (wchar_t *) malloc(sizeof(wchar_t) * (wlen + 1));
	wcs[wlen] = L'\0';
	if ( wlen ) {
		if ( mbstowcs(wcs, str, wlen+1) == (size_t) -1 )
		wcs[0] = L'\0';
		}
	return wcs;
	}

// convert wchar string to utf8 string
char	*u8wcsto(const wchar_t *str) {
	size_t  len = wcstombs(NULL, str, 0);
	char	*cs;
	
	if ( len == (size_t) -1 )
		len = 0;
	cs = (char *) malloc(sizeof(char) * (len + 1));
	cs[len] = '\0';
	if ( len ) {
		if ( wcstombs(cs, str, len+1) == (size_t) -1 )
		cs[0] = '\0';
		}
	return cs;
	}

// convert wcs to utf8 string, preallocated buffer 
char *u8get(char *dest, const wchar_t *source) {
	char *p = u8wcsto(source);
	strcpy(dest, p);
	free(p);
	return dest;
	}

// convert utf8 to wcs string, preallocated buffer
wchar_t *u8set(wchar_t *dest, const char *source) {
	wchar_t *p = u8towcs(source);
	wcscpy(dest, p);
	free(p);
	return dest;
	}

// insert character in the string
size_t wcs_inswc(wchar_t *dest, size_t pos, wchar_t wch) {
	if ( pos > wcslen(dest) ) pos = wcslen(dest);
	wchar_t *temp = wcsdup(&(dest[pos]));
	dest[pos] = wch; pos ++;
	wcscpy(&(dest[pos]), temp);
	free(temp);
	return pos;
	}

// insert substring in the string
size_t wcs_insert(wchar_t *dest, size_t pos, wchar_t *wcs) {
	if ( pos > wcslen(dest) ) pos = wcslen(dest);
	wchar_t *temp = wcsdup(&(dest[pos]));
	wcscpy(&(dest[pos]), wcs);
	wcscat(dest, temp);
	free(temp);
	return pos;
	}

// backspace at pos
size_t wcs_backspace(wchar_t *dest, size_t pos) {
	if ( pos ) {
		wchar_t *temp = NULL;
		if ( pos < wcslen(dest) )
			temp = wcsdup(&(dest[pos]));
		dest[-- pos] = L'\0';
		if ( temp ) {
			wcscat(dest, temp);
			free(temp);
			}
		}
	return pos;
	}

// delete to end-of-line
size_t wcs_deleol(wchar_t *dest, size_t pos) {
	dest[pos] = L'\0';
	return pos;
	}

// delete to begin-of-line
size_t wcs_delbol(wchar_t *dest, size_t pos) {
	if ( pos ) {
		if ( pos > wcslen(dest) ) pos = wcslen(dest);
		wchar_t *temp = wcsdup(&(dest[pos]));
		wcscpy(dest, temp);
		pos = 0;
		free(temp);
		}
	return pos;
	}

// delete character at position
size_t wcs_delwc(wchar_t *dest, size_t pos) {
	if ( pos < wcslen(dest) ) {
		wchar_t *temp = wcsdup(dest);
		if ( pos )
			wcsncpy(dest, temp, pos);
		dest[pos] = L'\0';
		wcscat(dest, &(temp[pos+1]));
		free(temp);
		}
	return pos;
	}

// delete wcs at position
size_t wcs_delete(wchar_t *dest, size_t pos, size_t len) {
	if ( pos < wcslen(dest) ) {
		wchar_t *temp = wcsdup(dest);
		if ( pos )
			wcsncpy(dest, temp, pos);
		dest[pos] = L'\0';
		if ( wcslen(&(temp[pos])) > len )
			wcscat(dest, &(temp[pos+len]));
		free(temp);
		}
	return pos;
	}

// returns the number of character c in string src
size_t count_char(const char *src, char c) {
	size_t count = 0;
	const char *p = src;
	while ( *p ) {
		if ( *p == c )
			count ++;
		p ++;
		}
	return count;
	}

//
static char *cspl_getvar(const char *name, void *arg)
{ return strdup(getenv(name)); }

// returns true if c is part of source
static inline bool str_contains(const char *source, char c) {
	register const char *p = source;
	while ( *p ) {
		if ( *p == c )
			return true;
		p ++;
		}
	return false;
	}

// copy source to dp and returns the moved pointer
static inline char *cspl_add_text(char *dp, const char *source) {
	if ( source ) {
		const char *p = source;
		while ( *p )
			*dp ++ = *p ++;
		}
	return dp;
	}

// o is the result of expanded object, resize buffer
#define cspl_resize(o) \
	if ( o && (l = strlen(o)) > 0 ) {\
		dif = b - buf;\
		alloc += l;\
		buf = (char *) realloc(buf, alloc);\
		b = buf + dif; }

// c_split flags
#define CSPL_KEEP_SQ	0x01	// Keep the single quotes; otherwise remove them automatically
#define CSPL_KEEP_RQ	0x02	// --//--   reversed single quotes   --//--
#define CSPL_KEEP_DQ	0x04	// --//--   double quotes  --//--
#define CSPL_EXEC_RQ	0x08	// Execute strings inside of reverse quotes and replace with the result
#define CSPL_EXP_ENV	0x10	// Expand environment variables
#define CSPL_KEEP_DEL	0x20	// Keep delimiters

/*
 * advanced string split
 *
 * priorities
 * 1. slash
 * 2. single quotes
 * 3. reversed single quotes (& execute)
 * 4. environment variables (& expand)
 * 5. double quotes
 * 6. delimiters
 */
size_t c_split(const char *source, const char *delim, int_t flags, char *(*getvar)(const char *, void *), void *arg) {
	char *buf, *output, *rq_ps, *rq_e;
	register char *b, *p, *v;
	int_t	level, n, count = 0;
	size_t	dif, l, alloc = strlen(source) + 1;
	bool b_sq = false, b_rq = false, b_dq = false;
	if ( getvar == NULL )
		getvar = cspl_getvar;

	buf = (char *) malloc(alloc);
	b = buf; *b = '\0';
	
	p = (char *) source;
	while ( isblank(*p) ) p ++;
	
	while ( *p ) {

		// escape codes
		if ( *p == '\\' ) {
			p ++;
			switch ( *p ) {
			case 't':	*b ++ = '\t'; p ++; break;
			case 'n':	*b ++ = '\n'; p ++; break;
			case 'r':	*b ++ = '\r'; p ++; break;
			case 'b':	*b ++ = '\b'; p ++; break;
			case 'v':	*b ++ = '\v'; p ++; break;
			case 'f':	*b ++ = '\f'; p ++; break;
			case 'a':	*b ++ = '\a'; p ++; break;
			case 'e':	*b ++ = '\033'; p ++; break;
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
				n = *p ++ - '0';
				if ( isdigit(*p) )	n = (n << 3) | *p ++ - '0';
				if ( isdigit(*p) )	n = (n << 3) | *p ++ - '0';
				*b ++ = n;
				break;
			case 'x':
				p ++;
				n = xdigit(*p); p ++;
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				break;
			case 'u':
				p ++;
				n = xdigit(*p); p ++;
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				if ( isxdigit(*p) )	{ n = xdigit(*p); p ++; }
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				break;
			case 'U':
				p ++;
				n = xdigit(*p); p ++;
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				if ( isxdigit(*p) )	{ n = xdigit(*p); p ++; }
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				if ( isxdigit(*p) )	{ n = xdigit(*p); p ++; }
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				if ( isxdigit(*p) )	{ n = xdigit(*p); p ++; }
				if ( isxdigit(*p) )	{ n = (n << 4) | xdigit(*p); p ++; }
				*b ++ = n;
				break;
			default:
				*b ++ = *p ++;
				}
			continue;
			}

		// single quotes
		if ( b_sq ) {
			if ( *p == '\'') {
				b_sq = false;
				if ( flags & CSPL_KEEP_SQ ) *b ++ = *p;
				p ++;
				}
			else *b ++ = *p ++;
			continue;
			}

		// reverse single quotes
		if ( b_rq ) {
			if ( *p == '`') {
				b_rq = false;
				rq_e = b;
				if ( flags & CSPL_KEEP_RQ ) *b ++ = *p;
				p ++;
				// execute reverse quote code
				if ( flags & CSPL_EXEC_RQ ) {
					size_t l = (rq_e - rq_ps) + 1;
					char *cmd = (char *) malloc(l+1);
					strncpy(cmd, rq_ps, l);
					cmd[l] = '\0';
					if ( (v = pipe_exec(cmd)) != NULL ) {
						cspl_resize(v);
						b = cspl_add_text(b, v);
						if ( *(b-1) == '\n' ) b --;
						free(v);
						}
					free(cmd);
					}
				}
			else *b ++ = *p ++;
			continue;
			}

		// environment variable
		if ( (*p == '$') && (flags & CSPL_EXP_ENV) ) {
			char name[LINE_MAX], *np;
			if ( isalpha(p[1]) ) {
				p ++;
				np = name;
				while ( isalnum(*p) || *p == '_' )
					*np ++ = *p ++;
				*np = '\0';
				if ( (v = (char *) getvar(name, arg)) != NULL ) {
					cspl_resize(v);
					b = cspl_add_text(b, v);
					free(v);
					}
				continue;
				}
			else if ( p[1] == '{' ) {
				p += 2;
				np = name;
				level = 1;
				while ( *p ) {
					if ( *p == '{' ) level ++;
					if ( *p == '}' ) if ( -- level == 0 ) break;
					*np ++ = *p ++;
					}
				if ( *p == '}' ) p ++;
				*np = '\0';
				if ( (v = (char *) getvar(name, arg)) != NULL ) {
					cspl_resize(v);
					b = cspl_add_text(b, v);
					free(v);
					}
				continue;
				}
			else if ( p[1] == '(' ) {
				p += 2;
				np = name;
				level = 1;
				while ( *p ) {
					if ( *p == '(' ) level ++;
					if ( *p == ')' ) if ( -- level == 0 ) break;
					*np ++ = *p ++;
					}
				if ( *p == ')' ) p ++;
				*np = '\0';
				if ( (v = pipe_exec(name)) != NULL ) {
					cspl_resize(v);
					b = cspl_add_text(b, v);
					if ( *(b-1) == '\n' ) b --;
					free(v);
					}
				continue;
				}
			}

		// double quotes
		if ( b_dq ) {
			if ( *p == '"') {
				b_dq = false;
				if ( flags & CSPL_KEEP_DQ ) *b ++ = *p;
				p ++;
				}
			else *b ++ = *p ++;
			continue;
			}

		// delimiters
		if ( delim && str_contains(delim, *p) ) {
			if ( flags & CSPL_KEEP_DEL ) *b ++ = *p;
			*b = '\0';
			b = buf;
			SLang_push_string(buf);
			count ++;
			p ++;
			continue;
			}
		else {
			switch ( *p ) {
			case '\'': b_sq = true; if (flags & CSPL_KEEP_SQ) *b ++ = *p; break;
			case '`':  b_rq = true; if (flags & CSPL_KEEP_RQ) *b ++ = *p; rq_ps = b; break;
			case '"':  b_dq = true; if (flags & CSPL_KEEP_DQ) *b ++ = *p; break;
			default:
				*b ++ = *p;
				}
			}
		p ++;
		}
	
	if ( b - buf ) { // remains
		*b = '\0';
		SLang_push_string(buf);
		count ++;
		}

	free(buf);
	return count;
	}

// create an array of options (break source at 'cdel')
char **text_to_strlist(const char *source, char cdel, size_t *p_count) {
	const char *ps, *p;
	char	*elem, **list;
	size_t	alloc = LIST_PREALLOC, count = 0;
	
	count = 0;
	list = (char **) malloc(sizeof(char *) * (alloc + 1));
	ps = p = source;
	while ( *p ) {
		if ( *p == cdel ) {
			elem = (char *) malloc((p - ps) + 1);
			strncpy(elem, ps, p - ps);
			elem[p - ps] = '\0';
			list[count ++] = elem;
			if ( count == alloc ) {
				alloc += LIST_PREALLOC;
				list = (char **) realloc(list, sizeof(char *) * (alloc + 1));
				}
			ps = p + 1;
			}
		p ++;
		}
	if ( *ps )
		list[count ++] = strdup(ps);
	*p_count = count;
	return list;
	}

// append one string to list
char **strlist_append(char **list, const char *source, size_t *p_count) {
	size_t count = *p_count, alloc = 0;
	if ( count )
		alloc = ((count / LIST_PREALLOC) + 1) * LIST_PREALLOC;
	list[count ++] = strdup(source);
	if ( count == alloc ) {
		alloc += LIST_PREALLOC;
		list = (char **) realloc(list, sizeof(char *) * (alloc + 1));
		}
	*p_count = count;
	return list;
	}

// append text-lines to strlist
char **strlist_append_text(char **list, const char *source, char cdel, size_t *p_count) {
	const char *ps, *p;
	char	*elem;
	size_t	alloc = 0, count = *p_count;

	if ( count )
		alloc = ((count / LIST_PREALLOC) + 1) * LIST_PREALLOC;
	
	ps = p = source;
	while ( *p ) {
		if ( *p == cdel ) {
			elem = (char *) malloc((p - ps) + 1);
			strncpy(elem, ps, p - ps);
			elem[p - ps] = '\0';
			list[count ++] = elem;
			if ( count == alloc ) {
				alloc += LIST_PREALLOC;
				list = (char **) realloc(list, sizeof(char *) * (alloc + 1));
				}
			ps = p + 1;
			}
		p ++;
		}
	if ( *ps )
		list[count ++] = strdup(ps);
	*p_count = count;
	return list;
	}

// create a text from array of strings (join with 'cdel')
char *strlist_to_text(const char **list, char cdel, size_t count) {
	size_t size = 0, len = 1, i;
	char *result = NULL, sdel[2];
	bool z;

	sdel[0] = cdel;
	sdel[1] = '\0';
	for ( i = 0; i < count; i ++ ) {
		len += strlen(list[i]) + 1;
		if ( len >= size ) {
			size += 8 * (PATH_MAX + 1);
			z = (result == NULL);
			result = (char *) realloc(result, size);
			if ( z ) *result = '\0';
			}
		strcat(result, list[i]);
		strcat(result, sdel);
		}
	return result;
	}

// create an empty string list (dynamic string array)
char **strlist_create() {
	size_t	alloc = LIST_PREALLOC;
	return (char **) malloc(sizeof(char *) * (alloc + 1));
	}

// free a string list
char **strlist_destroy(char **list, size_t *pcount) {
	for ( size_t i = 0; i < *pcount; i ++ ) {
		if ( list[i] )
			free(list[i]);
		}
	free(list);
	*pcount = 0;
	return NULL;
	}

// resize back to actual size, this is no needed,
// the rest allocation space are unintialized pointers
char **strlist_finalize(char **list, size_t *alloc, size_t count) {
	if ( *alloc != count ) {
		*alloc = count;
		list = realloc(list, sizeof(char*) * *alloc);
		}
	return list;
	}

// qsort string callback
static int strlist_sort_str(const void *a, const void *b)
{ return strcmp(*(const char **)a, *(const char **)b); }

// sort
void strlist_sort(char **list, size_t count)
{ qsort(list, count, sizeof(char*), strlist_sort_str); }

// find with binary search in sorted array
size_t strlist_bsearch(const char *name, const char **list, size_t count) {
	int_t mid, cmp, lo = 0, hi = count - 1;
	
	while ( hi - lo > 1 ) {
		mid = (hi + lo) >> 1;
		if ( (cmp = strcmp(list[mid], name)) == 0 ) return mid;
        if ( cmp < 0 )
			lo = mid + 1;
		else
			hi = mid;
		}
	if ( strcmp(list[lo], name) == 0 )	return lo;
	if ( strcmp(list[hi], name) == 0 )	return hi;
	}

// find string
size_t strlist_find(const char *name, const char **list, size_t count) {
	if ( name && *name ) {
		for ( size_t i = 0; i < count; i ++ ) {
			if ( strcmp(list[i], name) == 0 )
				return i;
			}
		}
	return count;
	}

// find the first element that matches with file
size_t strlist_find_first(const char *name, size_t selected, const char **list, size_t count) {
	size_t	 n = strlen(name), i;

	if ( n ) {
		for ( i = selected; i < count; i ++ ) {
			if ( strncmp(list[i], name, n) == 0 )
				return i;
			}
		for ( i = umin(selected, count); i > 0; i -- ) {
			if ( strncmp(list[i-1], name, n) == 0 )
				return i-1;
			}
		}
	return selected;
	}

// find the last element that matches with file
size_t strlist_find_last(const char *name, size_t selected, const char **list, size_t count) {
	size_t	n = strlen(name), i;

	if ( n ) {
		for ( i = umin(selected, count); i > 0; i -- ) {
			if ( strncmp(list[i-1], name, n) == 0 )
				return i-1;
			}
		}
	return selected;
	}

// find the next element that match with file
size_t	strlist_find_next(const char *name, size_t selected, const char **list, size_t count) {
	size_t	n = strlen(name), i;
	
	if ( n ) {
		for ( i = selected+1; i < count; i ++ ) {
			if ( strncmp(list[i], name, n) == 0 )
				return i;
			}
		}
	return selected;
	}

// find the previous element that match with file
size_t	strlist_find_prev(const char *name, size_t selected, const char **list, size_t count) {
	size_t	n = strlen(name), i;

	if ( n ) {
		for ( i = umin(selected, count); i > 0; i -- ) {
			if ( strncmp(list[i-1], name, n) == 0 )
				return i-1;
			}
		}
	return selected;
	}

// --------------------------------------------------------------------------------
unsigned xdigit_sl(unsigned *c) { return xdigit(*c); }

// strlist to SLarray; slist_to_array(list, count) -> array
SLang_Array_Type *strlist_to_array(const char **list, size_t count) {
	SLang_Array_Type *at;
	SLindex_Type icount = count;	
	size_t	i;

	if ( NULL == (at = SLang_create_array(SLANG_STRING_TYPE, 0, NULL, &icount, 1)) )
		return NULL;
	char **data = (char **) at->data;
	for ( i = 0; i < count; i ++ ) {
		if ( list[i] == NULL ) {
			data[i] = NULL;
			continue;
			}
	    if ( NULL == (data[i] = SLang_create_slstring((char*) list[i])) ) {
			SLang_free_array(at);
			return NULL;
			}
		}
	return at;
	}

// S-Lang intern version, pushing the results to SL's stack
void strlist_to_array_sl(size_t *plist, size_t *pcount) {
	char **list = (char **) *plist;
	SLang_Array_Type *at;
	size_t	i;
	SLindex_Type count = (SLindex_Type) *pcount;

	if ( NULL == (at = SLang_create_array(SLANG_STRING_TYPE, 0, NULL, &count, 1)) ) {
		SLang_push_null();
		return;
		}
	char **data = (char **) at->data;
	for ( i = 0; i < count; i ++ ) {
		if ( list[i] == NULL ) {
			data[i] = NULL;
			continue;
			}
	    if ( NULL == (data[i] = SLang_create_slstring(list[i])) ) {
			SLang_free_array(at);
			SLang_push_null();
			return;
			}
		}
	SLang_push_array(at, 1);
	}

// copy a S-Lang array to strlist
char **array_to_strlist(SLang_Array_Type *at, size_t *pcount) {
	size_t i, count;
	char **data = (char **) at->data;
	char **list = NULL;
	
	if ( at->data_type != SLANG_STRING_TYPE ) return NULL;
	count = *pcount = at->num_elements;
	size_t alloc = ((count / LIST_PREALLOC) + 1) * LIST_PREALLOC;
	list = (char **) malloc(sizeof(char*) * (alloc + 1));
	for ( i = 0; i < count; i ++ )
		list[i] = strdup(data[i]);
	return list;
	}

// returns a S-Lang array char**data; DO NOT FREED
char **get_array_ptrs(SLang_Array_Type *at, size_t *pcount) {
	char **data = (char **) at->data;
	if ( at->data_type != SLANG_STRING_TYPE ) return NULL;
	*pcount = at->num_elements;
	return data;
	}

// resize an array
void array_resize_sl(SLang_Array_Type *at, size_t newsize) {
	size_t count, i;
	if ( at->data_type != SLANG_STRING_TYPE ) goto unchanged;
 	if ( at->num_dims  != 1 ) goto unchanged;
	count = at->num_elements;
	if ( count > newsize )
		for ( i = newsize; i < count; i ++ )
			if ( ((char **) at->data)[i] )
				SLfree(((char **) at->data)[i]);
	at->data = (char **) SLrealloc(at->data, sizeof(char*) * newsize); 
	at->num_elements = newsize;
	if ( count < newsize )
		for ( i = count; i < newsize; i ++ )
			((char **) at->data)[i] = NULL;
unchanged:
	SLang_push_array(at, 1);
	}

// strlist
void *strlist_create_sl()
{ return (void *) strlist_create(); }
void strlist_free_sl(void *ptr, size_t *count)
{ strlist_destroy((char **) ptr, count); }

//
size_t	c_split_sl(const char *source, const char *delim, int_t *flags)
{ return c_split(source, delim, *flags, NULL, NULL); }
static char *vc_getvar(const char *name, void *data) {
	char *s = NULL;
	SLang_Name_Type *slfunc = (SLang_Name_Type *) data;

	SLang_start_arg_list();					
	SLang_push_string((char*) name);
	SLang_end_arg_list();					
	if ( SLexecute_function(slfunc) == -1 )
		return NULL;
	int t = SLang_peek_at_stack();
	if ( t == -1 ) return NULL;
	if ( t == SLANG_NULL_TYPE )		{ SLdo_pop(); return NULL; }
	if ( SLpop_string(&s) == -1 )	return NULL;
	return s; // freed by caller
	}
size_t	vc_split_sl(const char *source, const char *delim, int_t *flags, const char *getvar_cb) {
	SLang_Name_Type *slfunc = NULL;
	if ( getvar_cb && *getvar_cb ) 
		slfunc = SLang_get_function((char *)getvar_cb);
	if ( slfunc )
		return c_split(source, delim, *flags, vc_getvar, (void *) slfunc);
	return 0;
	}

static SLang_Intrin_Fun_Type C_STRING_Intrinsics [] = {
	MAKE_INTRINSIC_1("xdigit",        xdigit_sl,               WORD_TYPE, WORD_TYPE),
	MAKE_INTRINSIC_3("csplit",        c_split_sl,              WORD_TYPE, STRING_TYPE, STRING_TYPE, SWORD_TYPE),
	MAKE_INTRINSIC_4("vcsplit",       vc_split_sl,             WORD_TYPE, STRING_TYPE, STRING_TYPE, SWORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("__array_resize_1s", array_resize_sl,     VOID_TYPE, ARRAY_TYPE,  WORD_TYPE), // testing
//	MAKE_INTRINSIC_0("slist_create",      strlist_create_sl,   VOID_STAR),
//	MAKE_INTRINSIC_2("slist_free",        strlist_free_sl,     VOID_TYPE, VOID_STAR, WORD_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_string_init() {
	if ( SLadd_intrin_fun_table(C_STRING_Intrinsics, NULL) == -1 )	return 0;
	return 1;
	}
