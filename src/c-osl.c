/*
 *	OR '|' Separated Flag List
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "slang.h"
#include "c-types.h"

// returns true if the flag exists in the list
int_t	osl_isset(const char *list, const char *flag) {
	register const char *p, *ps;
	register size_t l = strlen(flag);

	ps = p = list;
	while ( (p = strchr(ps, '|')) != NULL ) {
		if ( l == (p - ps) && strncmp(ps, flag, l) == 0 )
			return 1;
		ps = p + 1;
		}
	if ( *ps ) {
		if ( strcmp(ps, flag) == 0 )
			return 1;
		}
	return 0;
	}

// adds a flag to the list
// list = osl_set(list, flag);
void osl_set(const char *list, const char *flag) {
	if ( !osl_isset(list, flag) ) {
		char *newlist = (char *) malloc(strlen(list) + strlen(flag) + 2);
		strcpy(newlist, list);
		strcat(newlist, "|");
		strcat(newlist, flag);
		SLang_push_string(newlist);
		free(newlist);
		}
	else
		SLang_push_string((char *)list);
	}

// removes a flag from the list
// list = osl_unset(list, flag);
void osl_unset(const char *list, const char *flag) {
	char *str = strdup(list);
	register char *p = str, *ps = str;
	while ( (p = strchr(ps, '|')) != NULL ) {
		*p = '\0';
		if ( strcmp(ps, flag) == 0 ) {
			strcpy(ps, p + 1);
			continue;
			}
		*p = '|';
		ps = p + 1;
		}
	if ( *ps ) {
		if ( strcmp(ps, flag) == 0 ) {
			ps[0] = '\0';
			if ( ps > str )
				ps[-1] = '\0';
			}
		}
	SLang_push_string(str);
	free(str);
	}

// --------------------------------------------------------------------------------

static SLang_Intrin_Fun_Type C_OSL_Intrinsics [] = {
	MAKE_INTRINSIC_2("osl_isset",   osl_isset,          SWORD_TYPE, STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("osl_set",     osl_set,            VOID_TYPE,  STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("osl_unset",   osl_unset,          VOID_TYPE,  STRING_TYPE, STRING_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_osl_init() {
	if ( SLadd_intrin_fun_table(C_OSL_Intrinsics, NULL) == -1 ) return 0;
	return 1;
	}
