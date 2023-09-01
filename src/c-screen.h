/*
 *	screen related utilities
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 */

#if !defined(__C_SCREEN_H)
#define __C_SCREEN_H

#if defined(__cplusplus)
extern "C" {
#endif

void c_scr_touch();
void c_scr_save();
void c_scr_restore();
void c_scr_redraw();
size_t  c_scr_cols();
size_t  c_scr_rows();
void c_scr_shutdown(); // close smgr, return to tty; use it for panic errors only

// init module
size_t  c_screen_init();

#if defined(__cplusplus)
}
#endif

#endif
