/*
 *	screen related utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#include "slang.h"
#include "screen.h"
#include "c-screen.h"
#include "c-types.h"

void c_scr_touch()		{ touch_screen(); }
void c_scr_save()		{ SLsmg_suspend_smg(); }
void c_scr_restore()	{ SLsmg_resume_smg(); jed_redraw_screen(0); }
void c_scr_redraw()		{ jed_redraw_screen(1); }
size_t c_scr_cols()		{ int r, c; jed_get_screen_size(&r, &c); return c; }
size_t c_scr_rows()		{ int r, c; jed_get_screen_size(&r, &c); return r; }
//void c_scr_shutdown()	{ SLang_reset_smg(); SLang_reset_tty(); }
void c_scr_shutdown()	{ jed_reset_display(); }
int jed_redraw_menus();
void c_scr_redraw_menus()	{ jed_redraw_menus(); }

// --------------------------------------------------------------------------------

static SLang_Intrin_Fun_Type C_SCREEN_Intrinsics [] = {
	MAKE_INTRINSIC_0("scr_shutdown", c_scr_shutdown,	VOID_TYPE),
	MAKE_INTRINSIC_0("scr_touch",	 c_scr_touch,		VOID_TYPE),
	MAKE_INTRINSIC_0("scr_save",	 c_scr_save,		VOID_TYPE),
	MAKE_INTRINSIC_0("scr_restore",	 c_scr_restore,		VOID_TYPE),
	MAKE_INTRINSIC_0("scr_redraw",	 c_scr_redraw,		VOID_TYPE),
	MAKE_INTRINSIC_0("scr_cols",	 c_scr_cols,		WORD_TYPE),
	MAKE_INTRINSIC_0("scr_rows",	 c_scr_rows,		WORD_TYPE),
	MAKE_INTRINSIC_0("scr_redraw_menus", c_scr_redraw_menus, VOID_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_screen_init() {
	if ( SLadd_intrin_fun_table(C_SCREEN_Intrinsics, NULL) == -1 ) return 0;
	return 1;
	}
