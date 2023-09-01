/*
 *	c_getkey / c_getwckey - waits and returns a key
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "slang.h"
#include "jedlimit.h"
#include "sysdep.h"
#include "keymap.h"
#include "misc.h"
#include "screen.h"

#include "c-types.h"
#include "c-getkey.h"
#include "c-screen.h"

// JED's keymap
static SLKeyMap_List_Type *cbkmap = NULL;

// build keymap for CBRIEF TUI
static int_t c_build_kmap() {
	unsigned char esc[16], i;

	memset(esc, 0, sizeof(esc));
	if ( NULL == (cbkmap = SLang_create_keymap ("cbkmap", NULL)) ) return 0;
	cbkmap->functions = NULL;
	
	esc[0] = 27;
	for ( i = 'a'; i <= 'z'; i ++ ) {
		esc[1] = (unsigned char) i;
		SLkm_define_keysym(esc, ALT_MASK | i, cbkmap);
		}
	for ( i = '0'; i <= '9'; i ++ ) {
		esc[1] = (unsigned char) i;
		SLkm_define_keysym(esc, ALT_MASK | i, cbkmap);
		}
	SLkm_define_keysym("OA",  SL_KEY_UP, cbkmap);
	SLkm_define_keysym("[A",  SL_KEY_UP, cbkmap);
	SLkm_define_keysym("OB",  SL_KEY_DOWN, cbkmap);
	SLkm_define_keysym("[B",  SL_KEY_DOWN, cbkmap);
	SLkm_define_keysym("OD",  SL_KEY_LEFT, cbkmap);
	SLkm_define_keysym("[D",  SL_KEY_LEFT, cbkmap);
	SLkm_define_keysym("OC",  SL_KEY_RIGHT, cbkmap);
	SLkm_define_keysym("[C",  SL_KEY_RIGHT, cbkmap);
	SLkm_define_keysym("[5~", SL_KEY_PPAGE, cbkmap);
	SLkm_define_keysym("[6~", SL_KEY_NPAGE, cbkmap);
	SLkm_define_keysym("[1~", SL_KEY_HOME, cbkmap);
	SLkm_define_keysym("[4~", SL_KEY_END, cbkmap);
	SLkm_define_keysym("[2~", SL_KEY_IC, cbkmap);
	SLkm_define_keysym("[3~", SL_KEY_DELETE, cbkmap);
	SLkm_define_keysym("[Z",  SL_KEY_STAB, cbkmap);
	for ( i = 1; i < 27; i ++ ) {
		sprintf(esc, "%c", i);
		SLkm_define_keysym(esc, i, cbkmap); // does not solve the problem
		}
	return 1;
	}

// returns the keysym if it has...
static int_t c_do_key() {
	SLang_Key_Type *key = SLang_do_key(cbkmap, jed_getkey); // <- this stores Last_Key_Char from getkey
	if ( key )
		return key->f.keysym;
	return SLang_Last_Key_Char;
	}

// wait and returns a char or special keys (key & SPC_MASK)
int_t c_getkey() {
	if ( Executing_Keyboard_Macro || ( Read_This_Character != NULL ) )
		return c_do_key();
	int ch = my_getkey();
	if ( SLKeyBoard_Quit || SLang_get_error() ) return -1;
	if ( ch == 27 ) {
		int tsecs = 1;
		if ( 0 == input_pending(&tsecs) )
			return 27;
		}
	ungetkey(&ch);
	return c_do_key();
	}

// wait and returns a wchar_t or special keys (key & SPC_WC_MASK)
wchar_t c_getwckey() {
	int c = c_getkey();
	wchar_t wch;

	if ( c & SPC_MASK )
		wch = (wchar_t) ( (SPC_WC_MASK & (c << 21)) | (c & 0x7F) );
	else {
		if ( (c & 0xC0) == 0xC0 ) { // UTF-8
			ungetkey(&c);
			jed_getkey_wchar(&wch);
			}
		else 
			wch = (wchar_t) c;
		}
	return wch;
	}

// --------------------------------------------------------------------------------

static SLang_Intrin_Fun_Type C_GETKEY_Intrinsics [] = {
	MAKE_INTRINSIC_0("c_getkey",    c_getkey,           SWORD_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_getkey_init() {
	if ( !c_build_kmap() ) return 0;
	if ( SLadd_intrin_fun_table(C_GETKEY_Intrinsics, NULL) == -1 ) return 0;
	return 1;
	}
