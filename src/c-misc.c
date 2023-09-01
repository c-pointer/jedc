/*
 *	Miscellaneous utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "slang.h"
#include "c-types.h"

#define BUFSZ			4096	// i386 optimized, one page
#define	TEXTLINESZ		128

#define	CSHELL_ALLOC_PAGE 0x10000

//	Run a command and returns the output as string
//	(output, exit_status, error_n) = cshell ( command );
void c_shell_sl(const char *cmd) {
	FILE *fp;
	char buf[BUFSZ];
	int_t	alloc = CSHELL_ALLOC_PAGE, count = 1, exit_status = -1, err = 0;
	char *retf = (char *) malloc(alloc);

	*retf = '\0';
	if ( (fp = popen(cmd, "r")) != NULL ) {
		while ( fgets(buf, BUFSZ, fp) ) {
			count += strlen(buf);
			if ( count >= alloc ) {
				alloc += CSHELL_ALLOC_PAGE;
				retf = (char *) realloc(retf, alloc);
				}
			strcat(retf, buf);
			}
		exit_status = pclose(fp);
		}
	err = errno;
	
	SLang_start_arg_list();
	SLang_push_string(retf);
	SLang_push_integer(exit_status);
	SLang_push_integer(err);
	SLang_end_arg_list();
	free(retf);
	}

// executes the cmd and returns the result (reversed quotes)
#define PIPE_ALLOC_PAGE 4096
char *pipe_exec(const char *cmd) {
	FILE 	*fp;
	char	buf[LINE_MAX];
	size_t	alloc = PIPE_ALLOC_PAGE, count = 1;
	int_t	status = -1;
	char 	*retf = (char *) malloc(alloc);

	*retf = '\0';
	if ( (fp = popen(cmd, "r")) != NULL ) {
		while ( fgets(buf, LINE_MAX, fp) ) {
			count += strlen(buf);
			if ( count >= alloc ) {
				alloc += PIPE_ALLOC_PAGE;
				retf = (char *) realloc(retf, alloc);
				}
			strcat(retf, buf);
			}
		status = pclose(fp);
		sprintf(buf, "%d", status);
		setenv("status", buf, 1);
		return retf;
		}
	free(retf);
	return NULL;
	}

// --------------------------------------------------------------------------------

static SLang_Intrin_Fun_Type C_MISC_Intrinsics [] = {
	MAKE_INTRINSIC_1("c_shell",     c_shell_sl,         VOID_TYPE, STRING_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_misc_init() {
	if ( SLadd_intrin_fun_table(C_MISC_Intrinsics, NULL) == -1 )	return 0;
	return 1;
	}
