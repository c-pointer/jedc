/*
 *	c_getkey / c_getwckey - waits and returns a key
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#if !defined(__C_GETKEY_H)
#define __C_GETKEY_H

#if defined(__cplusplus)
extern "C" {
#endif

// ASCII version
#define SPC_MASK		0x700	// 0x1xx special keys, 0x2xx function keys
#define ALT_MASK		0x400	// 0x4xx ALT keys xx = the character
#define SL_ALT_KEY(x)	(ALT_MASK | (x))
#define SL_KEY_CTRL_Q	0x11
#define SL_CTL_KEY(x)	(x-'A'+1)
#define SL_KEY_STAB		(ALT_MASK | 9)
	
// UTF8/UNICODE version (wchar_t)
#include <wchar.h>
#define SPC_WC_MASK		0xE0000000
#define ALT_WC_MASK		0x80000000
#define SL_ALT_WCKEY(x)	(ALT_WC_MASK | (x))
#define SL_CTL_WCKEY(x)	(x-'A'+1)
#define SL_KEY_WCSTAB	(ALT_WC_MASK | 9)

/* reads the next character from input stream and returns its code */
int_t	c_getkey();

/* reads the next character from input stream and returns its code */
wchar_t	c_getwckey();

/* init */
size_t	c_getkey_init();

#if defined(__cplusplus)
}
#endif

#endif
