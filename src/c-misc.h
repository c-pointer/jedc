/*
 *	Miscellaneous utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#if !defined(__C_MISC_H)
#define __C_MISC_H

#if defined(__cplusplus)
extern "C" {
#endif

char *pipe_exec(const char *cmd);

size_t c_misc_init();

#if defined(__cplusplus)
}
#endif

#endif
