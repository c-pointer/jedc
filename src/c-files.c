/*
 *	file related utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <time.h>
#if defined(__linux__)
	#include <syslog.h>
#endif
#include "slang.h"
#include "c-types.h"
#include "c-misc.h"
#include "c-string.h"
//#define __AVL_IMPL
//#include "c-avltree.c"
#define __BTREE_IMPL
#include "c-btree.c"

#if defined(IBMPC_SYSTEM)
	#define DIR_SEP_STR "\\"
	#define DIR_SEP_CHAR '\\'
#else
	#define DIR_SEP_STR "/"
	#define DIR_SEP_CHAR '/'
#endif
#define DIR_SEP_LEN 1

static inline size_t umax(size_t a, size_t b) { return (a > b) ? a : b; }

// Returns the file name without directory nor file extension
void c_basename(const char *src) {
	char *buf, *p;

	p = (char*) strrchr(src, DIR_SEP_CHAR);
	buf = ( p ) ? strdup(p+1) : strdup(src);
	if ( (p = strrchr(buf, '.')) != NULL )
		*p = '\0';
	SLang_push_string(buf);
	free(buf);
	}

// Returns the directory name (parent of src, if its directory)
void c_dirname(const char *src) {
	char *buf, *p;
	size_t l = strlen(src);

	buf = strdup(src);
	if ( buf[l-1] == DIR_SEP_CHAR )	buf[l-1] = '\0';
	p = (char*) strrchr(buf, DIR_SEP_CHAR);
	if ( p ) *p = '\0';
	else buf[0] = '\0';
	SLang_push_string(buf);
	free(buf);
	}

// Returns the file extension starting from dot
void c_ext(const char *src) {
	char *buf, *p;

	if ( src == NULL )  { SLang_push_string(""); return; }
	if ( *src == '\0' ) { SLang_push_string(""); return; }
	p = (char*) strrchr(src, DIR_SEP_CHAR);
	buf = ( p ) ? strdup(p+1) : strdup(src);
	p = strrchr(buf, '.');
	if ( p )
		SLang_push_string(p);
	else {
		buf[0] = '\0';
		SLang_push_string(buf);
		}
	free(buf);
	}

// Returns the file extension after the dot
void c_type(const char *src) {
	char *buf, *p;

	if ( src == NULL )  { SLang_push_string(""); return; }
	if ( *src == '\0' ) { SLang_push_string(""); return; }
	p = (char*) strrchr(src, DIR_SEP_CHAR);
	buf = ( p ) ? strdup(p+1) : strdup(src);
	p = strrchr(buf, '.');
	buf[0] = '\0';
	SLang_push_string((p) ? p+1 : buf);
	free(buf);
	}

//	returns true if the file is directory
int_t c_isdir(const char *file) {
	struct stat st;

	if ( file == NULL ) return 0;
	if ( *file == '\0' ) return 0;
	if ( stat(file, &st) == 0 )
		if ( st.st_mode & S_IFDIR ) // DIR & LINK allowed as true
			return 1;
	return 0;
	}

// returns true if the file is regular file
int_t c_isreg(const char *file) {
	struct stat st;

	if ( file == NULL ) return 0;
	if ( *file == '\0' ) return 0;
	if ( stat(file, &st) == 0 )
		if ( st.st_mode & S_IFREG ) // LINK allowed, check for other flags
			return 1;
	return 0;
	}

// returns the files of the path in string separated by \n
char *get_file_list_string(const char *path, const char *pat) {
	DIR		*dir;
	struct dirent *sd;
	int_t	alloc = 1, pass;
	char	*list = (char *) malloc(alloc);
	struct stat st;
	char	old_path[PATH_MAX];

	*list = '\0';
	getcwd(old_path, PATH_MAX);
	chdir(path);
	if ( (dir = opendir(path)) != NULL ) {
		while ( (sd = readdir(dir)) != NULL ) {
			if ( stat(sd->d_name, &st) != 0 ) continue;
			if ( st.st_mode & S_IFDIR ) {
				// add directory
				if ( strcmp(sd->d_name, ".") != 0 ) {
					alloc += strlen(sd->d_name) + 2;
					list = (char *) realloc(list, alloc);
					strcat(list, sd->d_name);
					strcat(list, DIR_SEP_STR"\n");
					}
				}
			else if ( st.st_mode & S_IFREG ) {
				// add regular file
				if ( pat && *pat && fnmatch(pat, sd->d_name, FNM_PERIOD) != 0 )
					continue;
				alloc += strlen(sd->d_name) + 1;
				list = (char *) realloc(list, alloc);
				strcat(list, sd->d_name);
				strcat(list, "\n");
				}
			}
		closedir(dir);
		}
	chdir(old_path);
	return list;
	}

// returns the files of the path
#define PREALLOC_LIST	1024
char **get_file_list_strlist(const char *path, const char *pat, size_t *pcount) {
	DIR		*dir;
	struct dirent *sd;
	struct stat st;
	int_t	alloc = 0, count = 0;
	char	**list = NULL;
	char	old_path[PATH_MAX];

	getcwd(old_path, PATH_MAX);
	chdir(path);
	if ( (dir = opendir(path)) != NULL ) {
		while ( (sd = readdir(dir)) != NULL ) {
			if ( stat(sd->d_name, &st) != 0 ) continue;
			if ( count == alloc ) {
				alloc += PREALLOC_LIST;
				list = (char **) malloc(sizeof(char*) * alloc);
				}
			if ( st.st_mode & S_IFDIR ) {
				if ( strcmp(sd->d_name, ".") == 0 )	continue;
				list[count] = (char *) malloc(strlen(sd->d_name) + 2);
				strcpy(list[count], sd->d_name);
				strcat(list[count], DIR_SEP_STR);
				count ++;
				}
			else if ( st.st_mode & S_IFREG ) {
				if ( pat && *pat && fnmatch(pat, sd->d_name, FNM_PERIOD) != 0 )
					continue;
				list[count ++] = strdup(sd->d_name);
				}
			}
		closedir(dir);
		}
	chdir(old_path);
	*pcount = count;
	return list;
	}

// find solutions
char *file_complete(const char *root) {
	char dir[PATH_MAX];
	char common[NAME_MAX], pat[NAME_MAX];
	int_t i, n, l, sol = 0; 		// number of solution
	DIR *dp;
	struct dirent *e;
	struct stat st;

	pat[0] = '\0';
	common[0] = '\0';
	strcpy(dir, root);
	if ( stat(dir, &st) != 0 ) return NULL;
	if ( (st.st_mode & S_IFDIR) == 0 ) { // not dir
		char *p = strrchr(dir, DIR_SEP_CHAR);	// find if it has part of finename
		if ( p ) {
			char c = p[1];
			p[1] = '\0';
			if ( stat(dir, &st) == 0 ) {
				p[1] = c;
				if ( st.st_mode & S_IFDIR )
					strcpy(pat, p + 1);
				else return NULL;
				p[0] = '\0';
				}
			else return NULL;
			}
		else return NULL;
		}

	if ( chdir(dir) != 0 )
		return NULL;
	if ( (dp = opendir(dir)) == NULL )
		return NULL;
	n = strlen(pat);
	while ( (e = readdir(dp)) != NULL) {
		if ( l && strncmp(pat, e->d_name, n) == 0 ) { // solution found
			sol ++;
			if ( sol == 1 ) // the first solution
				strcpy(common, e->d_name);
			else { // next solution
				l = strlen(common);
				for ( i = n; i < l; i ++ ) {
					if ( common[i] != e->d_name[i] ) {
						common[i] = '\0';
						break;
						}
					}
				} // many solutions, calculate the common part
			} // solution found
		} // dir
	closedir(dp);
	
	// there is at least one solution
	if ( sol )
		return strdup(common);
	return NULL;
	}

// same as dircat() but with unlimited parameters
// vdircat("a", "b", "c") -> "a/b/c"
void c_dircat() {
	size_t	_NARGS = SLang_Num_Function_Args;
	size_t	last, i, *lens = NULL, idx, len = 1;
	char	*str = NULL, *result = NULL, **list = NULL;

	lens = (size_t*) malloc(_NARGS * sizeof(size_t));
	list = (char **) malloc(_NARGS * sizeof(char *));
	memset(lens, 0, _NARGS * sizeof(size_t));
	memset(list, 0, _NARGS * sizeof(char *));
	for ( i = 0; i < _NARGS; i ++ ) {
		str = NULL;
		if ( SLang_pop_slstring(&str) != -1 ) {
			idx = (_NARGS - i) - 1;
			list[idx] = str;
			lens[idx] = strlen(str);
			len += lens[idx];
			}
		}
	result = (char *) malloc(len + _NARGS * DIR_SEP_LEN);
	*result = '\0';
	last = _NARGS - 1;
	for ( i = 0; i < _NARGS; i ++ ) {
		if ( list[i] ) {
			if ( i && list[i][0] == DIR_SEP_CHAR )
				strcat(result, list[i]+1);
			else
				strcat(result, list[i]);
			if ( i < last ) {
				if ( list[i][lens[i]-1] != DIR_SEP_CHAR )
					strcat(result, DIR_SEP_STR);
				}
			}
		}

	// cleanup
	if ( list ) {
		for ( i = 0; i < _NARGS; i ++ )
			if ( list[i] )
				SLang_free_slstring(list[i]);
		}
	if ( result ) {
		SLang_push_string(result);
		free(result);
		}
	if ( list ) free(list);
	if ( lens ) free(lens);
	}

// returns true if file is in directory
int_t find_in_dir(const char *dir, const char *file) {
	DIR *dp = opendir(dir);
	struct dirent *e;
	if ( !dp ) return 0;
	while ( e = readdir(dp) ) {
		if ( strcmp(file, e->d_name) == 0 )
			return 1;
		}
	return 0;
	}

// returns a newly allocated full-path filename, if file is in path;
// otherwise returns NULL. The c_del is the character that separates
// the path directories (:).
char *find_in_path(const char *path, const char *file, char c_del) {
	char	*path_cp = strdup(path), *p, *ps;
	char	c_dir[PATH_MAX], s_del[2];

	s_del[0] = c_del; s_del[1] = '\0';
	ps = path_cp;
	strcat(ps, s_del);
	while ( *ps ) {
		if ( (p = strchr(ps, c_del)) == NULL )
			break;
		*p = '\0';
		strcpy(c_dir, ps);
		ps = p + 1;
		if ( find_in_dir(c_dir, file) ) {
			strcat(c_dir, DIR_SEP_STR);
			strcat(c_dir, file);
			free(path_cp);
			return strdup(c_dir);
			}
		}
	free(path_cp);
	return NULL;
	}

// count = dirlist([[dir[+pat]]|NULL] [, st_mode-mask]);
// list  = __pop_list(count);
size_t dirlist() {
	size_t count;
	char **list, *p, *dir = NULL;
	char path[PATH_MAX], pat[NAME_MAX];
	struct stat st;
	int mode_mask = 0;

	path[0] = pat[0] = '\0';
	if ( SLang_Num_Function_Args > 0 ) {
		if ( SLpop_string(&dir) == -1 )
			{ SLang_push_null(); return 0; }
		else {
			strcpy(path, dir);
			SLfree(dir);
			}
		if ( SLang_Num_Function_Args > 1 ) {
			if ( SLang_pop_integer(&mode_mask) == -1 )
				{ SLang_push_null(); return 0; }
			}
		}

	if ( *path && (stat(path, &st) == 0) ? (st.st_mode & S_IFDIR) : 0 )
		strcpy(pat, "*");
	else if ( (p = strrchr(path, DIR_SEP_CHAR)) != NULL ) {
		*p = '\0';
		strcpy(pat, p + 1); 
		}
	else {
		strcpy(pat, path);
		getcwd(path, PATH_MAX);
		}
	if ( !strwc(pat) ) strcat(pat, "*");

	
	list = get_file_list_strlist(path, pat, &count);
	for ( size_t i = 0; i < count; i ++ ) {
		if ( mode_mask ) {
			if ( stat(list[i], &st) == 0 ) {
				if ( (st.st_mode & mode_mask) == 0 )
					continue;
				}
			else
				continue;
			}
		SLang_push_string(list[i]);
		}
	size_t items = count;
	list = strlist_destroy(list, &count);
	return items;
	}

// --- rc files -------------------------------------------------------------------
// files with rc variables, environment variables and shell backquotes

typedef struct rc_s {
	const char *file;
	btree_node_t *root;
	} rc_t;
static rc_t **rc_table = NULL;
static size_t	rc_count = 0;
static size_t	rc_alloc = 0;

// clear the nodes of the tree
static btree_node_t *rc_clear_tree(btree_node_t *root) {
	if ( root ) {
		rc_clear_tree(root->left);
		rc_clear_tree(root->right);
		free((char*)root->key);
		free(root->data);
		free(root);
		}
	return NULL;
	}

// clears all memory allocated by varfile subsys
void rc_shutdown() {
	if ( rc_table ) {
		for ( size_t i = 0; i < rc_count; i ++ ) {
			if ( rc_table[i] ) {
				free((char*)rc_table[i]->file);
				rc_table[i]->root = rc_clear_tree(rc_table[i]->root);
				free(rc_table[i]);
				}
			}
		free(rc_table);
		}
	}

// create a slot (binary tree) for file and returns the ID
size_t rc_init(const char *file) {
	size_t i, id = 0;

	// search for an empty slot
	for ( i = 0; i < rc_count; i ++ ) {
		if ( rc_table[id] == NULL ) {
			id = i;
			break;
			}
		}
	if ( i == rc_count ) // not found, get a new
		id = rc_count ++;
	if ( rc_count >= rc_alloc ) { // resize required
		if ( rc_table == NULL ) // first time, setup atexit
			atexit(rc_shutdown);
		rc_alloc += 8;
		rc_table = (rc_t **) realloc(rc_table, sizeof(rc_t *) * rc_alloc);
		}
	// fill slot
	rc_table[id] = (rc_t *) malloc(sizeof(rc_t));
	rc_table[id]->file = strdup(file);
	rc_table[id]->root = NULL;
	// return the handle
	return id;
	}

// copy source to dp and returns the moved pointer
static inline char *vexp_add_text(char *dp, const char *source) {
	if ( source ) {
		const char *p = source;
		while ( *p )
			*dp ++ = *p ++;
		}
	return dp;
	}

// get more space for expand
#define vexp_resize(o) \
		if ( (l = strlen(o)) > 0 ) {\
			dif = b - buf;\
			alloc += l;\
			buf = realloc(buf, alloc);\
			b = buf + dif; }

//
char *rc_expand(btree_node_t *root, char *source) {
	char *buf, *rq_ps, *rq_e, *vp;
	register char *b, *p;
	size_t	alloc = strlen(source) + 1, dif, l;
	int_t	level;
	bool	b_sq = false, b_rq = false, b_dq = false;
	btree_node_t *node;

	buf = (char *) malloc(alloc);
	b = buf; *b = '\0';
	
	p = (char *) source;
	while ( isblank(*p) ) p ++;
	
	while ( *p ) {
			
		// escape codes
		if ( *p == '\\' ) {
			int_t	n;
			
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
			if ( *p == '\'') { b_sq = false; p ++; }
			else *b ++ = *p ++;
			continue;
			}

		// reverse single quotes
		if ( b_rq ) {
			if ( *p == '`') {
				b_rq = false;
				rq_e = b;
				p ++;
				// get the command
				size_t l = (rq_e - rq_ps) + 1;
				char *cmd = (char *) malloc(l+1);
				strncpy(cmd, rq_ps, l);
				cmd[l] = '\0';
				// execute the command and copy the results
				if ( (vp = pipe_exec(cmd)) != NULL ) {
					vexp_resize(vp);
					b = vexp_add_text(b, vp);
					if ( *(b-1) == '\n' ) b --;
					free(vp);
					}
				free(cmd);
				}
			else *b ++ = *p ++;
			continue;
			}

		// environment variable
		if ( *p == '$' ) {
			char name[LINE_MAX], *np;
			if ( isalpha(p[1]) || p[1] == '_' ) {
				p ++;
				np = name;
				while ( isalnum(*p) || *p == '_' )
					*np ++ = *p ++;
				*np = '\0';
				vp = ( (node = btree_search(root, name)) != NULL ) ?
					(char *) rc_expand(root, node->data) :
					strdup(getenv(name));
				if ( vp ) {
					vexp_resize(vp);
					b = vexp_add_text(b, vp);
					free(vp);
					}
				continue;
				}
			else if ( p[1] == '(' ) {
				p += 2;
				np = name;
				level = 1;
				while ( *p  ) {
					if ( *p == '(' ) level ++;
					if ( *p == ')' ) if ( -- level == 0 ) break;
					*np ++ = *p ++;
					}
				if ( *p == ')' ) p ++;
				*np = '\0';
				// execute the command and copy the results
				if ( (vp = pipe_exec(name)) != NULL ) {
					vexp_resize(vp);
					b = vexp_add_text(b, vp);
					if ( *(b-1) == '\n' ) b --;
					free(vp);
					}
				continue;
				}
			else if ( p[1] == '{' ) {
				p += 2;
				np = name;
				level = 1;
				while ( *p  ) {
					if ( *p == '{' ) level ++;
					if ( *p == '}' ) if ( -- level == 0 ) break;
					*np ++ = *p ++;
					}
				if ( *p == '}' ) p ++;
				*np = '\0';
				vp = ( (node = btree_search(root, name)) != NULL ) ?
					(char *) rc_expand(root, node->data) :
					strdup(getenv(name));
				if ( vp ) { 
					vexp_resize(vp);
					b = vexp_add_text(b, vp);
					free(vp);
					}
				continue;
				}
			}

		// double quotes
		if ( b_dq ) {
			if ( *p == '"') { b_dq = false; p ++; }
			else *b ++ = *p ++;
			continue;
			}

		switch ( *p ) {
		case '\'': b_sq = true; break;
		case '`':  b_rq = true; rq_ps = b; break;
		case '"':  b_dq = true; break;
		default: *b ++ = *p; }
		
		p ++;
		}
	*b = '\0';
	return buf;
	}

// right trim
static char *vexp_rtrim(char *val) {
	size_t i, l = strlen(val);
	for ( i = l; i > 0; i -- ) {
		if ( isspace(val[i-1]) )
			val[i-1] = '\0';
		else
			break;
		}
	return val;
	}

// set the variable name of the table id, to value
void rc_set(size_t *pid, const char *name, const char *value) {
	size_t id = *pid;
	if ( id < rc_count ) {
		btree_node_t *node = btree_search(rc_table[id]->root, name);
		if ( node ) {
			if ( node->data ) free(node->data);
			node->data = ( value ) ? strdup(value) : NULL;
			}
		else
			rc_table[id]->root =
				btree_insert(rc_table[id]->root,
					strdup(name), (value) ? strdup(value) : NULL );
		}
	}

// returns the value of the variable name of the table id or NULL
char *rc_get(size_t *pid, const char *name) {
	size_t id = *pid;
	if ( id < rc_count ) {
		btree_node_t *node = btree_search(rc_table[id]->root, name);
		if ( node ) 
			return rc_expand(rc_table[id]->root, node->data);
		}
	return NULL;
	}

// removes a node from tree
void rc_del(size_t *pid, const char *name) {
	size_t id = *pid;
	if ( id < rc_count ) {
		btree_node_t *node = btree_search(rc_table[id]->root, name);
		if ( node ) {
			char *key = (char *) node->key;
			free(node->data);
			rc_table[id]->root = btree_delete(rc_table[id]->root, name, NULL);
			free(key);
			}
		}
	}

// returns true if this rc is memory file
static int_t rc_is_memfile(size_t id) {
	if ( rc_table[id]->file == NULL )		return 1;
	if ( rc_table[id]->file[0] == '\0' )	return 1;
	if ( rc_table[id]->file[0] == '*' )		return 1;
	return 0;
	}

// loads the file
void rc_load(size_t *pid) {
	size_t id = *pid;
	FILE *fp;
	char	buf[LINE_MAX], *p, name[LINE_MAX], *n, *value;
	size_t	l, line = 0, count = 0;

	if ( id < rc_count ) {
		if ( !rc_is_memfile(id) ) { // valid filename, or memory only
			if ( (fp = fopen(rc_table[id]->file, "r")) != NULL ) {
				while ( fgets(buf, LINE_MAX, fp) ) {
					line ++;
					p = buf;
					while ( isspace(*p) ) p ++;
					if ( *p == '\0' )	continue;
					if ( *p == '#' )	continue;
					n = name;
					while ( isalnum(*p) || *p == '_' )
						*n ++ = *p ++;
					*n = '\0';
					while ( isspace(*p) ) p ++;
					if ( *p != '=' ) {
						fprintf(stderr, "%s:%jd missing assign symbol (=)\n", rc_table[id]->file, line);
						continue;
						}
					p ++;
					while ( isblank(*p) ) p ++;
					value = strdup(p);
					vexp_rtrim(value);
					
					// append lines -- begin
					while ( value[strlen(value)-1] == '\\' ) {
						size_t slashes = 0, i, l = strlen(value);
						// count slashes at end of value
						for ( i = l; i > 0; i -- ) {
							if ( value[i-1] == '\\' ) { slashes ++; continue; }
							break;
							}
						// if remains one slash (i.e. \[new-line])
						if ( slashes % 2 ) {
							value[l-1] = '\0'; // remove the last slash
							if ( fgets(buf, LINE_MAX, fp) ) { // read next line into buf
								line ++;
								char *newln = vexp_rtrim(buf);	// right trim (removes white chars)
								value = (char*) realloc(value, strlen(value) + strlen(newln) + 1);
								strcat(value, newln);	// append to value, return to loop for more lines or not
								continue;
								}
							}
						break; // exit loop
						}
					// append lines -- end
					
					rc_set(&id, name, value); // store it
					free(value);
					}
				fclose(fp);
				rc_table[id]->root = btree_rebalance(rc_table[id]->root);
				}
			}
		}
	}

// recursed write function
static void rc_save_tree(FILE *out, btree_node_t *root) {
	if ( root ) {
		if ( root->data )
			fprintf(out, "%s=%s\n", root->key, (char *) root->data);
		rc_save_tree(out, root->left);
		rc_save_tree(out, root->right);
		}
	}

// writes the file
void rc_save(size_t *pid) {
	size_t id = *pid;
	if ( id < rc_count ) {
		if ( !rc_is_memfile(id) ) { // valid filename, or memory only
			FILE *fp;
			if ( (fp = fopen(rc_table[id]->file, "w")) != NULL ) {
				rc_save_tree(fp, rc_table[id]->root);
				fclose(fp);
				}
			}
		}
	}

// callback for each variable
void rc_enum(size_t *pid, const char *callback) {
	size_t id = *pid;
	SLang_Name_Type *slfunc = NULL;

	if ( id < rc_count ) {
		if ( callback && *callback ) 
			slfunc = SLang_get_function((char *)callback);
		if ( slfunc ) {
			btree_node_t *cur = rc_table[id]->root;
			size_t tail, stack_alloc = 512;
			btree_node_t **stack = (btree_node_t **) malloc(sizeof(btree_node_t*) * stack_alloc);

			/* in order */
			tail = 0;
			while ( 1 ) {
				if ( cur ) {
					stack[tail ++] = cur;
					cur = cur->left;
					if ( tail == stack_alloc )
						stack = (btree_node_t **) realloc(stack,
							sizeof(btree_node_t*) * (stack_alloc += 512));
					}
				else if ( tail ) {
					cur = stack[-- tail];
					SLang_start_arg_list();					
					SLang_push_string((char*)cur->key);
					SLang_push_string((char*)cur->data);
					SLang_end_arg_list();					
					if ( SLexecute_function(slfunc) == -1 ) {
						free(stack);
						return;
						}
					cur = cur->right;
					}
				else
					break;
				}
			free(stack);
			}
		}
	}

// --------------------------------------------------------------------------------
void find_in_path_sl(const char *path, const char *file, int *c_del) {
	char *p = find_in_path(path, file, *c_del);
	if ( p ) { SLang_push_string(p); free(p); }
	else SLang_push_null();
	}
void rc_get_sl(size_t *pid, const char *name) {
	char *v = rc_get(pid, name);
	if ( v == NULL ) SLang_push_null();
	else { SLang_push_string(v); free(v); }
	}
void rc_gets_sl(size_t *pid, const char *name, const char *defval) {
	char *v = rc_get(pid, name);
	if ( v == NULL ) SLang_push_string((char *)defval);
	else { SLang_push_string(v); free(v); }
	}
void rc_geti_sl(size_t *pid, const char *name, size_t *defval) {
	char *v = rc_get(pid, name);
	if ( v == NULL ) SLang_push_long(*defval);
	else { SLang_push_long(atol(v)); free(v); }
	}
void rc_getf_sl(size_t *pid, const char *name, double *defval) {
	char *v = rc_get(pid, name);
	if ( v == NULL ) SLang_push_double(*defval);
	else { SLang_push_double(atof(v)); free(v); }
	}

static SLang_Intrin_Fun_Type C_FILES_Intrinsics [] = {
	MAKE_INTRINSIC_1("basename"  ,   c_basename,        VOID_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_1("dirname"   ,   c_dirname,         VOID_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_1("file_ext",     c_ext,             VOID_TYPE, STRING_TYPE),
//	MAKE_INTRINSIC_1("file_type",    c_type,            VOID_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_0("vdircat",      c_dircat,          VOID_TYPE),
	MAKE_INTRINSIC_1("is_dir",       c_isdir,           SWORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_1("is_file",      c_isreg,           SWORD_TYPE, STRING_TYPE),
//	MAKE_INTRINSIC_3("find_in_path", find_in_path_sl,   VOID_TYPE, STRING_TYPE, STRING_TYPE, INT_TYPE),
	MAKE_INTRINSIC_0("dirlist",      dirlist,       	WORD_TYPE),
	MAKE_INTRINSIC_1("rc_init",      rc_init,           WORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("rc_get",       rc_get_sl,         VOID_TYPE, WORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_3("rc_gets",      rc_gets_sl,        VOID_TYPE, WORD_TYPE, STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_3("rc_geti",      rc_geti_sl,        VOID_TYPE, WORD_TYPE, STRING_TYPE, SWORD_TYPE),
	MAKE_INTRINSIC_3("rc_getf",      rc_getf_sl,        VOID_TYPE, WORD_TYPE, STRING_TYPE, SLANG_DOUBLE_TYPE),
	MAKE_INTRINSIC_3("rc_set",       rc_set,            VOID_TYPE, WORD_TYPE, STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("rc_del",       rc_del,            VOID_TYPE, WORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("rc_enum",      rc_enum,           VOID_TYPE, WORD_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_1("rc_load",      rc_load,           VOID_TYPE, WORD_TYPE),
	MAKE_INTRINSIC_1("rc_save",      rc_save,           VOID_TYPE, WORD_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_files_init() {
	if ( SLadd_intrin_fun_table(C_FILES_Intrinsics, NULL) == -1 ) return 0;
	return 1;
	}
