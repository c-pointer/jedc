/*
 *	CBRIEF compatibility module
 * 
 *	Copyright (c) 2017-2022 Nicholas Christopoulos <nereus@freemail.gr>
 *	This file is part of CBRIEF-package / JED editor library source.
 *
 *	You may distribute this file under the terms the GNU General Public
 *	License. See the file COPYING for more information.
 *
 *	Original JED files that changed:
 *	screen.c -- error message color, selection lines/columns inclusive or not colorize
 *	intrin.c -- add cbrief_slang_init()
 *	misc.c -- add set_last_macro()
 */

#include <stdbool.h>	// ISO C
#include <stdint.h>		// ISO C
#include <wchar.h>		// ISO C
#include <limits.h>		// ISO C
#include <stdio.h>		// ISO C
#include <string.h>		// ISO C
#include <ctype.h>		// ISO C
#include <stdlib.h>		// ISO C
#include <unistd.h>		// POSIX
#include <errno.h>		// ISO C / POSIX.1
#include <sys/stat.h>

#include <slang.h>
#include "screen.h"
#include "colors.h"
#include "sysdep.h"
#include "misc.h"

#include "c-types.h"
#define NO_LOGFILE
#include "panic.h"
#include "c-screen.h"
#include "c-getkey.h"
#include "c-string.h"
#include "c-files.h"
#include "c-misc.h"

static inline int_t MIN(int_t a, int_t b) { return ((a<b)?a:b); }
static inline int_t MAX(int_t a, int_t b) { return ((a>b)?a:b); }
static inline bool  VALIDKEY(int c)	{ return (c > 31 && c < 0x100 && c != '' ); }

/* CBRIEF flags:
 * 0x01 = use reversed block for cursor. (visual only)
 * 0x02 = activate BRIEF's inclusive selection. (visual only)
 * 0x04 = activate BRIEF's line-mode selection. (visual only)
 * 0x08 = activate column-mode selection. (visual only)
 */
size_t cbrief_flags = 0;					/* ndc: just an integer to pass parameters */
size_t cbrief_api_ver = 10;					/* ndc: cbrief module version */
size_t cbrief_sel_line = 0;					/* ndc: same as select_column_pos, it will replace it when finish */ 
size_t cbrief_sel_col = 0;					/* ndc: line of selection mark */
extern void set_last_macro(char *);			/* ndc: copies a keystroke string to macro buffer (misc.c) */

#define BUFSZ			4096	// i386 optimized, one page
#define	TEXTLINESZ		NAME_MAX

/*
 * Message/Input Box
 *
 * title: title of the window
 * fmt: printf's style format for the message
 * flags:
 *	0x00 normal
 *  0x04 with text box at the bottom, TODO out of horz limits scroll
 *	0x08 show wrapped text TODO
 *
 * Returns -1 for cancel, 0 for default key
*/
int_t c_msgbox(int_t flags, const char *title, const char *pbuf, const char *fmt, ...) {
	char	*msg = (char *) malloc(BUFSZ), **list, *p;
	int_t	x, y, w, h, i, start_pos = 0;
	int_t	maxc, rv;
	int		rows, cols, c;
	bool	first_time = true;
	size_t	count, wcs_pos = 0;
	wchar_t	*wcs = NULL, wch;
	va_list ap;

	bool	has_editline = flags & 0x4;	// todo: edit text line

	int_t	extra_h = 0;
//	if ( has_buttons  )	extra_h ++;
	if ( has_editline )	{
		extra_h ++;
		wcs     = (wchar_t *) malloc(sizeof(wchar_t) * TEXTLINESZ);
		wcs[0]  = L'\0';
		wcs_pos = 0;
		if ( pbuf && *pbuf )
			wcscpy(wcs, u8towcs(pbuf));
		}
	
	// build text
	va_start(ap, fmt);
	vsnprintf(msg, BUFSZ, fmt, ap);
	va_end(ap);
	list = text_to_strlist(msg, '\n', &count);
	
	/*
	 * calculate the maximum width in columns
	 */
	maxc = 20;
	if ( has_editline )
		maxc = 60;
	
	if ( title ) {
		if ( *title )
			maxc = MAX(maxc, u8strlen(title) + 2);
		}
	for ( i = 0; i < count; i ++ )
		maxc = MAX(maxc, u8strlen(list[i]));

	// calculate size and position
	jed_get_screen_size(&rows, &cols);
	w = MIN(maxc,  cols - 8);
	h = MIN(count, rows - (4 + extra_h));
	h += extra_h;
	y = rows / 2 - h / 2;
	x = cols / 2 - w / 2;

	rv = -1;
	do {
		// draw the whole box
		SLsmg_set_color_in_region(JMENU_SHADOW_COLOR, y, x, h + 2, w + 4);
		SLsmg_set_color(JMENU_POPUP_COLOR);
		SLsmg_draw_box(y - 1, x - 2, h + 2, w + 4);
		if ( title ) {
			if ( *title ) {
				SLsmg_draw_object(y - 1, x, SLSMG_RTEE_CHAR);
				SLsmg_gotorc(y - 1, x + 1);
				SLsmg_write_string((char *) title);
				SLsmg_draw_object(y - 1, x + u8strlen(title) + 1, SLSMG_LTEE_CHAR);
				}
			}
		// clear window
		for ( i = 0; i < h; i ++ ) {
			SLsmg_gotorc(y + i, x - 1);
			SLsmg_write_nstring(NULL, w + 2);
			}
		// print text
		for ( i = start_pos; i < count && i < (h - extra_h); i ++ ) {
			SLsmg_gotorc(y + i, x);
			SLsmg_write_nstring(list[i], w);
			}
		// scroll up indicator
		SLsmg_set_color(JMENU_POPUP_COLOR);
		if ( start_pos > 0 ) {
			SLsmg_gotorc(y, x + w);
			SLsmg_write_string("↑");
			}
		// more text indicator
		if ( start_pos + i < count ) {
			SLsmg_gotorc(y + h - (1 + extra_h), x + w);
			SLsmg_write_string("↓");
			}

		// draw edit box
		if ( has_editline ) {
			int	ey = (h - extra_h);
			SLsmg_set_color(JNORMAL_COLOR);
			SLsmg_gotorc(y + ey, x);
			p = u8wcsto(wcs);
			SLsmg_write_nstring(p, w);
			free(p);
			// put the cursor
			SLsmg_gotorc(y + ey, x + wcs_pos);
			}

		// refresh
		SLsmg_set_color(JNORMAL_COLOR);
		SLsmg_refresh();

		// input
		c = c_getkey();
		if ( has_editline && first_time ) {
			first_time = false;
			if ( c > 31 && c < 256 )
				wcs[0] = L'\0';
			}
		switch ( c ) {
		case SL_KEY_UP: // up
			if ( start_pos > 0 )
				start_pos --;
			break;
		case SL_KEY_DOWN: // down
			if ( start_pos < (count - (h - extra_h)) )
				start_pos ++;
			break;
		case '\n': case '\r': rv = 1; break; // enter
		case -1: case SL_CTL_KEY('Q'):
		case 0x1B: rv = 0; break;
		default:
			if ( has_editline ) {
				switch ( c ) {
				case '\b': case '':	wcs_pos = wcs_backspace(wcs, wcs_pos); break;
				case SL_KEY_LEFT:  if ( wcs_pos ) wcs_pos --; break;
				case SL_KEY_RIGHT: if ( wcs_pos < wcslen(wcs) ) wcs_pos ++; break;
				case SL_KEY_HOME:  case '': wcs_pos = 0; break;
				case SL_KEY_END:   case '': wcs_pos = wcslen(wcs); break;
				case SL_ALT_KEY('k'):	wcs_pos = wcs_deleol(wcs, wcs_pos); break;
				case SL_CTL_KEY('K'):	wcs_pos = wcs_delbol(wcs, wcs_pos); break;
				case SL_KEY_IC:    break;
				case SL_KEY_DELETE:		wcs_pos = wcs_delwc(wcs, wcs_pos); break;
				default:
					// if character is utf8, convert it to wchar
					if ( (c & 0xC0) == 0xC0 ) {
						ungetkey(&c);
						jed_getkey_wchar(&wch);
						}
					else 
						wch = (wchar_t) c;
					wcs_pos = wcs_inswc(wcs, wcs_pos, wch);
					}
				}
			else {
				if ( c == 'q' || c == 'Q' ) {	// close/cancel/quit
					rv = 0;
					break;
					}
				}
			}
		} while ( rv == -1 );
	c_scr_redraw();

	// clean up
	free(msg);
	if ( has_editline ) {
		p = u8wcsto(wcs);		
		SLang_push_string(p);
		free(p);
		}
	if ( wcs ) free(wcs);
	return rv;
	}

/*
 *	Popup menu / List-box dialog box (list or files, with or without search / new text / file)
 *
 * source: is the list of items separated by '\n'
 * defsel: the preselected item (default is 0)
 * footer: message on the bottom window line; can be NULL
 * hook: callback routine for each key (except change position); can be NULL
 *   hookpar: extra parameter to pass to hook function; can be NULL
 *
 * int hook ( user data (hookpar), selected text, selected index, action code, key code )
 * 		action code: 's' for selected (before returns), 'd' for delete (before delete), 'k' for any other key
 * return -1 to quit, 0 for success
 * 
 * Returns the index of the selected item or -1 for cancel
 */

// flags
#define PUM_CAN_DELETE		0x01	// can delete element
#define	PUM_SORT_ELEMENTS	0x02	// sort the elements
#define	PUM_ALLOW_NEW_TEXT	0x04	// allow new element
#define	PUM_ALLOW_NEW_FILE	0x08	// allow new filename
#define	PUM_FILE_SELECT		0x10	// do not mixup file with list selection, this is file selection
#define	PUM_USE_KEYS		0x20	// use normal keys for actions
#define	PUM_HIDE_SEARCH		0x40	// do not show search-bar
#define	PUM_USE_SEARCH		0x80	// use simple search

// fix positions to make 'selected' item always visible
#define PUM_FIX_SCROLL() {\
		if ( selected >= count ) selected = count - 1;\
		if ( selected >= start_pos + menu_items ) start_pos = (selected+1) - menu_items;\
		}

// copy text from s (list[selected]) to text-edit and mark as first-time edit
#define PUM_SET_WCS(s) \
{ wchar_t *w = u8towcs(s); wcscpy(wcs, w); free(w); wcs_pos = wcslen(wcs); first_time = true; }

// qsort string callback
static int_t _sort_str(const void *a, const void *b)
{ return strcmp(*(const char **)a, *(const char **)b); }

// select-file callback
static int dir_hook(const char *slfunc, const char *text, int item, int code, int key) {
	switch ( code ) {
	case 's':
		strcpy((char *)slfunc, text);
		break;
		}
	return 0;
	}

// default C hook for popup
static int pum_hook(const char *slfunc, const char *text, int item, int code, int key) {
	SLang_Name_Type *slhook = NULL;
	int	rv = 0;

	if ( *slfunc )
		slhook = SLang_get_function((char *)slfunc);
	else
		slhook = NULL;
	if ( slhook ) {
		SLang_start_arg_list();
		SLang_push_string((char*)text);
		SLang_push_integer(item);
		SLang_push_integer(code);
		SLang_push_integer(key);
		SLang_end_arg_list();
		SLexecute_function(slhook);
		if ( SLang_get_error () == 0 )
			SLang_pop_integer(&rv);
		}
	return rv;
	}

/*
 * list box - double line - popup window
 * used for buffers
 *
 * title   = title of window or NULL
 * footer  = footer of window or NULL (use NULL for file-search box)
 * hook    = callback routine
 * hookpar = callback routine extra parameter
 */

// call slang hook
static int bufmenu_call_hook(SLang_Name_Type *hook_p, int sel, int code, int key) {
	int	ret = 0;
	if ( hook_p ) {
		SLang_start_arg_list();
		SLang_push_integer(sel);
		SLang_push_integer(code);
		SLang_push_integer(key);
		SLang_end_arg_list();
		SLexecute_function(hook_p);
		if ( SLang_get_error() == 0 )
			SLang_pop_integer(&ret);
		}
	return ret;
	}

// fix positions to make 'selected' item always visible
#define bufmenu_fix_pos() {\
		if ( selected < 0 ) selected = 0;\
		if ( selected >= dcount ) selected = dcount - 1;\
		if ( selected >= start_pos + menu_items ) start_pos = (selected+1) - menu_items;\
		}

// main bufmenu 
int_t dialog_bufmenu_sl(SLang_Array_Type *ac, int_t *defsel,
		const char *title, const char *footer, const char *sl_hook) {
	int		rows, cols, ret;
	size_t	maxc, i, count, dcount;
	int_t	selected, start_pos, key, menu_w, menu_h, menu_x, menu_y, menu_items;
	bool	b_exit = false;
	SLang_Name_Type *hook_p = NULL;
	char **list = get_array_ptrs(ac, &count);

	if ( sl_hook && *sl_hook )
		hook_p = SLang_get_function((char *)sl_hook);

	// calculate the size and position
	jed_get_screen_size(&rows, &cols);
	maxc = MAX(50, cols >> 1); // minimum width
	if ( title )	maxc = MAX(maxc, u8strlen((char *)title ) + 2);
	if ( footer )	maxc = MAX(maxc, u8strlen((char *)footer) + 2);
	for ( i = 0; i < count; i ++ )
		maxc = MAX(maxc, u8strlen(list[i]));

	// calculate menu width, height and position
	menu_w = MIN(maxc,  cols - 8);
	menu_h = MIN(count, rows - 8);
	if ( menu_h % 2 ) menu_h --;
	menu_x = cols / 2 - menu_w / 2;
	menu_y = ((rows - menu_h) - 2) / 2;
	menu_items = menu_h >> 1;
	dcount = count >> 1;

	// first selection
	start_pos = 0;
	selected = *defsel;
	if ( selected < 0 ) selected = 0;
	if ( selected >= dcount ) selected = dcount - 1;
	bufmenu_fix_pos();

	do {
		// draw static items...
		// this was outside of the loop but needed for redraw
		SLsmg_set_color_in_region(JMENU_SHADOW_COLOR, menu_y, menu_x, menu_h + 2, menu_w + 4);
		SLsmg_set_color(JMENU_POPUP_COLOR);
		SLsmg_draw_box(menu_y - 1, menu_x - 2, menu_h + 2, menu_w + 4);
		if ( title && *title ) {
			SLsmg_draw_object(menu_y - 1, menu_x, SLSMG_RTEE_CHAR);
			SLsmg_gotorc(menu_y - 1, menu_x + 1);
			SLsmg_write_string((char *) title);
			SLsmg_draw_object(menu_y - 1, menu_x + u8strlen(title) + 1, SLSMG_LTEE_CHAR);
			}
		if ( footer && *footer ) {
			SLsmg_gotorc(menu_y + menu_h, menu_x + menu_w - u8strlen(footer));
			SLsmg_write_string((char *) footer);
			}
		for ( i = 0; i < menu_items; i ++ ) {
			SLsmg_gotorc(menu_y + (i << 1), menu_x - 1);
			SLsmg_write_nstring(NULL, menu_w + 2);
			SLsmg_gotorc(menu_y + (i << 1) + 1, menu_x - 1);
			SLsmg_write_nstring(NULL, menu_w + 2);
			}
		
		// draw menu
		for ( i = 0; i < menu_items; i ++ ) {
			if ( i + start_pos == selected )
				SLsmg_set_color(JMENU_SELECTION_COLOR);
			else
				SLsmg_set_color(JMENU_POPUP_COLOR);

			SLsmg_gotorc(menu_y + (i << 1), menu_x);
			if ( start_pos + i < dcount ) 
				SLsmg_write_nstring(list[start_pos + (i << 1)], menu_w);
			else 
				SLsmg_write_nstring(NULL, menu_w);
			SLsmg_gotorc(menu_y + (i << 1) + 1, menu_x);
			if ( start_pos + i < dcount )
				SLsmg_write_nstring(list[start_pos + (i << 1) + 1], menu_w);
			else 
				SLsmg_write_nstring(NULL, menu_w);
			}
		SLsmg_set_color(JMENU_POPUP_COLOR);
		// up indicator
		if ( start_pos > 0 ) {
			SLsmg_gotorc(menu_y, menu_x + menu_w);
			SLsmg_write_string("↑");
			}
		// down indicator
		if ( start_pos + i < dcount ) {
			SLsmg_gotorc(menu_y + menu_h - 1, menu_x + menu_w);
			SLsmg_write_string("↓");
			}
		else
			SLsmg_gotorc(menu_y + menu_h, menu_x);
		
		SLsmg_set_color(JNORMAL_COLOR);
 		SLsmg_refresh();

		// get key
		key = c_getkey();
		if ( VALIDKEY(key) ) {
			// curstom user keys
			switch ( key ) {
			case 'd': case 'D': // delete
				key = SL_KEY_DELETE; break;
			case 'q': case 'Q': // quit
				key = SL_CTL_KEY('Q'); break;
			case '?': case 'h': case 'H': // help
				key = SL_ALT_KEY('h'); break;
				}
			}
		
		//
		switch ( key ) {
		case SL_KEY_UP:
			if ( selected > 0 ) {
				selected --;
				if ( start_pos > selected )
					start_pos = selected;
				}
			bufmenu_fix_pos();
			break;
		case SL_KEY_DOWN:
			if ( selected < dcount )
				selected ++;
			bufmenu_fix_pos();
			break;
		case SL_KEY_HOME:
			selected = start_pos = 0;
			break;
		case SL_KEY_END:
			selected = dcount - 1;
			bufmenu_fix_pos();
			break;
		case SL_KEY_PPAGE: // pgup
 			selected -= menu_items;
 			if ( selected < 0 ) selected = 0;
 			start_pos -= menu_items;
			if ( start_pos < 0 ) start_pos = 0;
			bufmenu_fix_pos();
			break;
		case SL_KEY_NPAGE: // pgdn
 			selected += menu_items;
			bufmenu_fix_pos();
			break;
		// -------------------------------------------------------------------------
		case SL_KEY_DELETE:
			if ( (ret = bufmenu_call_hook(hook_p, selected, 'd', key)) < 0 )
				return ret;
			if ( ret == 1 ) { // allow to delete
				list[(selected << 1)] = NULL;
				list[(selected << 1) + 1] = NULL;
				count -= 2;
	 			if ( selected < dcount ) {
					for ( i = (selected << 1); i < count; i += 2 ) {
						list[i]   = list[i+2];
						list[i+1] = list[i+3];
						}
					list[count  ] = NULL;
					list[count+1] = NULL;
					}
				dcount --;
				if ( selected == dcount ) {
					selected --;
					bufmenu_fix_pos();
					}
				}
			break;
		case '\r': case '\n': // ENTER (select)
			if ( (ret = bufmenu_call_hook(hook_p, selected, 's', key)) < 0 )
				return ret;
			b_exit = true;
			break;
		case 0x1B: case -1: case SL_CTL_KEY('Q'): // quit / cancel 
			selected = -1;
			if ( (ret = bufmenu_call_hook(hook_p, selected, 'c', key)) < 0 )
				return ret;
			b_exit = true;
			break;
		default:
			if ( (ret = bufmenu_call_hook(hook_p, selected, 'k', key)) < 0 )
				return ret;
			if ( ret == 1 )
				b_exit = true;
			} // switch
		} while ( b_exit == false );

	return selected;
	}

/*
 * list box popup window
 *
 * flags   = see PUM_* flags above
 * source  = list separated by '\n'
 * defsel  = default selection
 * title   = title of window or NULL
 * footer  = footer of window or NULL (use NULL for file-search box)
 * info    = text buffer for the parent window with the latest selection or NULL
 * hook    = callback routine
 * hookpar = callback routine extra parameter
 */
int_t c_popup_menu(int_t flags, char **list, size_t count, int_t defsel,
				const char *title, const char *footer, char *info,
				int (*hook)(const char*,const char*,int,int,int), const char *hookpar) {
	int_t 	i, maxc, start_pos = 0, selected = 0, n;
	int 	rows, cols, key = 0, prev_key = 0;
	bool	first_time = true, b_exit = false;
	int_t	menu_x, menu_y, menu_w, menu_h, menu_items;
	char	*elem;

	bool	b_delete  = flags & PUM_CAN_DELETE;
	bool	b_sort    = flags & PUM_SORT_ELEMENTS;
	bool	b_newfile = flags & PUM_ALLOW_NEW_FILE;
	bool	b_usrkeys = flags & PUM_USE_KEYS;
 	bool	b_hidesrc = flags & PUM_HIDE_SEARCH;
 	bool	b_filesel = flags & PUM_FILE_SELECT;
	bool	b_newtext = flags & PUM_ALLOW_NEW_TEXT;
	bool	b_search  = flags & PUM_USE_SEARCH;
	
	wchar_t wch, wcs[NAME_MAX];
	wcs[0]  = L'\0';
	size_t wcs_pos = 0;
	char	base[PATH_MAX];
	base[0] = '\0';
	
	if ( b_sort ) // sort
		strlist_sort(list, count);

	// calculate the size and position
	jed_get_screen_size(&rows, &cols);
	maxc = 30; // minimum width
	if ( title )	maxc = MAX(maxc, u8strlen((char *)title ) + 2);
	if ( footer )	maxc = MAX(maxc, u8strlen((char *)footer) + 2);
	for ( i = 0; i < count; i ++ )
		maxc = MAX(maxc, u8strlen(list[i]));

	// calculate menu width, height and position
	menu_w = MIN(maxc,  cols - 8);
	menu_h = MIN(count, rows - 8);
	menu_x = cols / 2 - menu_w / 2;
	menu_y = ((rows - menu_h) - 2) / 2;
	menu_items = menu_h;

	// first selection
	start_pos = 0;
	selected = defsel;
	if ( selected < 0 ) selected = 0;
	if ( selected >= count ) selected = count - 1;
	u8set(wcs, list[selected]); // default selection
	PUM_FIX_SCROLL();

	do {
		// draw static items...
		// this was outside of the loop but needed for redraw
		SLsmg_set_color_in_region(JMENU_SHADOW_COLOR, menu_y, menu_x, menu_h + 2, menu_w + 4);
		SLsmg_set_color(JMENU_POPUP_COLOR);
		SLsmg_draw_box(menu_y - 1, menu_x - 2, menu_h + 2, menu_w + 4);
		if ( title && *title ) {
			SLsmg_draw_object(menu_y - 1, menu_x, SLSMG_RTEE_CHAR);
			SLsmg_gotorc(menu_y - 1, menu_x + 1);
			SLsmg_write_string((char *) title);
			SLsmg_draw_object(menu_y - 1, menu_x + u8strlen(title) + 1, SLSMG_LTEE_CHAR);
			}
		if ( footer && *footer ) {
			SLsmg_gotorc(menu_y + menu_h, menu_x + menu_w - u8strlen(footer));
			SLsmg_write_string((char *) footer);
			}
		for ( i = 0; i < menu_items; i ++ ) {
			SLsmg_gotorc(menu_y + i, menu_x - 1);
			SLsmg_write_nstring(NULL, menu_w + 2);
			}
		
		// draw menu
		for ( i = 0; i < menu_items; i ++ ) {
			if ( i + start_pos == selected )
				SLsmg_set_color(JMENU_SELECTION_COLOR);
			else
				SLsmg_set_color(JMENU_POPUP_COLOR);
			SLsmg_gotorc(menu_y + i, menu_x);
			if ( start_pos + i < count )
				SLsmg_write_nstring(list[start_pos + i], menu_w);
			else 
				SLsmg_write_nstring(NULL, menu_w);
			}
		SLsmg_set_color(JMENU_POPUP_COLOR);
		// up indicator
		if ( start_pos > 0 ) {
			SLsmg_gotorc(menu_y, menu_x + menu_w);
			SLsmg_write_string("↑");
			}
		// down indicator
		if ( start_pos + i < count ) {
			SLsmg_gotorc(menu_y + menu_h - 1, menu_x + menu_w);
			SLsmg_write_string("↓");
			}
		// search string
		if ( !b_hidesrc ) {
			if ( b_newtext || b_newfile || b_search ) {
				SLsmg_set_color(JMENU_CHAR_COLOR);
				SLsmg_gotorc(menu_y + menu_h, menu_x - 1);
				SLsmg_write_nstring(NULL, menu_w + 2);
				SLsmg_gotorc(menu_y + menu_h, menu_x);
				elem = u8wcsto(wcs);
				SLsmg_write_nstring(elem, menu_w);
				free(elem);
				SLsmg_gotorc(menu_y + menu_h, menu_x + wcs_pos);
				}
			else
				SLsmg_gotorc(menu_y + menu_h, menu_x + wcs_pos);
			}
		else
			SLsmg_gotorc(menu_y + menu_h, menu_x);
		
		SLsmg_set_color(JNORMAL_COLOR);
 		SLsmg_refresh();

		// get key
		if ( !VALIDKEY(key) )
			prev_key = key;
		key = c_getkey();
		
		if ( VALIDKEY(key) ) {
			// curstom user keys
			if ( b_usrkeys ) { // use keys 
				switch ( key ) {
				case 'd': case 'D': // delete
					if ( b_delete )
						key = SL_KEY_DELETE;
					break;
				case 'q': case 'Q': // quit
					key = SL_CTL_KEY('Q');
					break;
				case '?': case 'h': case 'H': // help
					key = SL_ALT_KEY('h');
					break;
					}
				}
			else if ( b_newtext || b_newfile || b_search ) {
				// if character is utf8, convert it to wchar
				if ( (key & 0xC0) == 0xC0 ) {
					ungetkey(&key);
					jed_getkey_wchar(&wch);
					}
				else
					wch = (wchar_t) key;

				if ( first_time ) {
					first_time = false;
					wcs[0] = wch;
					wcs[1] = L'\0'; // clear edit text
					wcs_pos = 1;
					}
				else
					wcs_pos = wcs_inswc(wcs, wcs_pos, wch);
				key = 'a'; // any valid key
				}
			}
		
		//
		switch ( key ) {
		case SL_KEY_UP:
			if ( selected > 0 ) {
				selected --;
				if ( start_pos > selected )
					start_pos = selected;
				}
			PUM_FIX_SCROLL();
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_KEY_DOWN:
			if ( selected < count )
				selected ++;
			PUM_FIX_SCROLL();
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_KEY_HOME:
			selected = start_pos = 0;
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_KEY_END:
			selected = count - 1;
			PUM_FIX_SCROLL();
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_KEY_PPAGE: // pgup
 			selected -= menu_items;
 			if ( selected < 0 ) selected = 0;
 			start_pos -= menu_items;
			if ( start_pos < 0 ) start_pos = 0;
			PUM_FIX_SCROLL();
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_KEY_NPAGE: // pgdn
 			selected += menu_items;
			PUM_FIX_SCROLL();
			PUM_SET_WCS(list[selected]);
			break;
			
		case SL_ALT_KEY('d'):
			if ( b_filesel ) {
				struct stat st;
				if ( stat(list[selected], &st) == 0 ) {
					bool succeed;
					if ( st.st_mode & S_IFDIR )
						succeed = (rmdir(list[selected]) == 0);
					else
						succeed = (remove(list[selected]) == 0);
					if ( succeed ) {
						free(list[selected]);
						list[selected] = NULL;
						count --;
			 			if ( selected < count ) {
							for ( i = selected; i < count; i ++ )
								list[i] = list[i+1];
							list[count] = NULL;
							}
						PUM_FIX_SCROLL();
						}
					}
				}
			break;

		case SL_KEY_DELETE:		// delete
			// delete character from search-string buffer
			if ( b_newfile || b_newtext || b_search ) {
				if ( !b_usrkeys && !b_delete ) {
					char file[NAME_MAX];
					wcs_pos = wcs_delwc(wcs, wcs_pos);
					u8get(file, wcs);
					selected = strlist_find_first(file, selected, (const char **) list, count);
					break;
					}
				}

			// delete element, use alt+d if it is blocked
			if ( (!(b_newfile || b_newtext || b_search)) && b_delete ) {
				if ( hook ) {
					int_t r = hook(hookpar, list[selected], selected, 'd', key);
					if ( r < 0 ) {
						selected = r;
						b_exit = true;
						}
					}
				
				if ( !b_exit ) {
					free(list[selected]);
					list[selected] = NULL;
					count --;
		 			if ( selected < count ) {
						for ( i = selected; i < count; i ++ )
							list[i] = list[i+1];
						list[count] = NULL;
						}
					PUM_FIX_SCROLL();
					}
				}
			break;
		// --- editor keys ---------------------------------------------------------
		case '': case '\b':	wcs_pos = wcs_backspace(wcs, wcs_pos); break;
		case SL_CTL_KEY('A'):	wcs_pos = 0; break;
		case SL_CTL_KEY('E'):	wcs_pos = wcslen(wcs); break;
		case SL_ALT_KEY('k'):	wcs_pos = wcs_deleol(wcs, wcs_pos); break;
		case SL_CTL_KEY('K'):	wcs_pos = wcs_delbol(wcs, wcs_pos); break;
		case SL_KEY_LEFT:		if ( wcs_pos ) wcs_pos --; break;
		case SL_KEY_RIGHT:		if ( wcs_pos < wcslen(wcs) ) wcs_pos ++; else PUM_SET_WCS(list[selected]); break;
		case SL_KEY_STAB:
			if ( prev_key != key && strlen(base) == 0 ) {
				u8get(base, wcs);
				n = strlist_find_last(base, selected, (const char **) list, count);
				}
			else  {
				n = strlist_find_prev(base, selected, (const char **) list, count);
				if ( n == selected )
					n = strlist_find_last(base, count, (const char **) list, count);
				}
			
			if ( n != selected ) {
			 	selected = n; 
			 	PUM_FIX_SCROLL(); 
			 	PUM_SET_WCS(list[selected]); 
			 	}
		
			PUM_SET_WCS(list[selected]);
			break;
			
		case '\t':
			if ( prev_key != key ) {
				u8get(base, wcs);
				n = strlist_find_first(base, selected, (const char **) list, count);
				}
			else {
				n = strlist_find_next(base, selected, (const char **) list, count);
				if ( n == selected )
					n = strlist_find_first(base, 0, (const char **) list, count);					
				}
		
			if ( n != selected ) {
			 	selected = n; 
			 	PUM_FIX_SCROLL(); 
			 	PUM_SET_WCS(list[selected]); 
			 	}
		
			PUM_SET_WCS(list[selected]);
			break;
			
		// -------------------------------------------------------------------------
		case '\r': case '\n': { // ENTER (select)
				char file[NAME_MAX];
				u8get(file, wcs);
				if ( info ) strcpy(info, file);

				if ( !b_filesel ) {
					if ( strcmp(file, list[selected]) == 0 ) {
						if ( hook )
							hook(hookpar, list[selected], selected, 's', key);
						b_exit = true;
						}
					// wildcards || given directory
					if ( !b_exit && b_newtext ) {
						selected = -2;
						if ( hook ) 
							hook(hookpar, file, selected, 's', key);
						b_exit = true;
						}
					}

				// selected item from the list
				if ( !b_exit && b_filesel ) {
					if ( strcmp(file, list[selected]) == 0 ) {
						if ( hook )
							hook(hookpar, list[selected], selected, 's', key);
						b_exit = true;
						}
					// wildcards || given directory
					if ( !b_exit && b_newfile ) {
						selected = -2;
						if ( hook ) 
							hook(hookpar, file, selected, 's', key);
						b_exit = true;
						}
					}
				}
			break;
			
		case 0x1B: case -1: case SL_CTL_KEY('Q'): // quit / cancel 
			selected = -1;
			if ( hook ) 
				hook(hookpar, "", selected, 's', key);
			b_exit = true;
			break;
			
		default:
			if ( hook ) {
				int_t r = hook(hookpar, list[selected], selected, 'k', key);
				if ( r < 0 ) {
					selected = r;
					b_exit = true;
					}
				}

			// search
			if ( !b_exit && b_search && VALIDKEY(key) ) {
				if ( key != '\t' && key != SL_KEY_STAB ) {
					if ( wcslen(wcs) ) {
						char name[PATH_MAX];
						u8get(name, wcs);
						n = strlist_find_first(name, selected, (const char **) list, count);
						if ( n != selected )
							selected = n;
						PUM_FIX_SCROLL();
						}
					}
				}
			
			} // switch
		} while ( b_exit == false );

	// cleanup and return
	return selected;
	}

/*
 * Create popup menu (list box) with files and directories
 * 
 * flags   = see PUM_* flags above
 * title   = title of the window or NULL
 * dirp    = default directory with or without wildcards, can be NULL (all of cwd)
 *
 * returns a string with the new / selected file.
 */
void c_popup_files(int flags, const char *title, const char *dirp) {
	char	path[PATH_MAX], wcrd[NAME_MAX];
	char	last[PATH_MAX], retf[PATH_MAX];
	char	cwd[PATH_MAX];
	char	**files = NULL, *p, *ps;
	struct stat st;
	int_t	r, l, df;
	size_t	count = 0;
	
	bool	b_delete  = flags & PUM_CAN_DELETE;
	bool	b_sort    = flags & PUM_SORT_ELEMENTS;
	bool	b_newfile = flags & PUM_ALLOW_NEW_FILE;
 	bool	b_filesel = flags & PUM_FILE_SELECT;
	bool	b_newtext = flags & PUM_ALLOW_NEW_TEXT;

	setlogfile("/tmp/xxx.log");
	getcwd(cwd, PATH_MAX);
	logpf("start=%s\n", cwd);

	// get path and file with/without windcards
	path[0] = wcrd[0] = '\0';
	if ( dirp ) 
		strcpy(path, dirp);
	if ( *path && (stat(path, &st) == 0) ? (st.st_mode & S_IFDIR) : 0 )
		strcpy(wcrd, "*");
	else if ( (p = strrchr(path, '/')) != NULL ) {
		*p = '\0';
		strcpy(wcrd, p + 1); 
		}
	else {
		strcpy(wcrd, path);
		getcwd(path, PATH_MAX);
		}
	if ( *wcrd && (strwc(wcrd) == NULL) )
		strcat(wcrd, "*");

	// main loop
	last[0] = retf[0] = '\0';
	do {
		// find the correct directory
		logpf("path=%s, wcrd=%s\n", path, wcrd);
		if ( chdir(path) != 0 )
			panic("chdir %s", path);
		
		if ( files ) files = strlist_destroy(files, &count);
		files = get_file_list_strlist(path, wcrd, &count);
		if ( count == 0 ) {
			SLang_push_null();
			return;
			}
		
		// show list
		c_scr_redraw(); // clear screen
		r = c_popup_menu(flags, files, count, 0, title, /* footer */ NULL, retf, dir_hook, last);
		// if ( r >= 0 ) the index of the selected file, also its name copied to 'last'
		logpf("R=%d, info=%s\n", r, retf);

		if ( r >= 0 ) {
			// perfect absolute name from the list
			if ( b_filesel ) {
				char file[NAME_MAX];
				strcpy(file, retf);
				l = strlen(file);
				
				// this directory selected from menu, change to it
				if ( file[l-1] == '/' || c_isdir(file) ) {
					if ( file[l-1] == '/' )
						file[l-1] = '\0';
					strcat(path, "/");
					strcat(path, file);
					realpath(path, retf);
					strcpy(path, retf); // copy it back for chdir
					logpf("directory selected and changer to '%s'\n", retf);
					continue;
					}
				
				// this file selected from menu, fill path and return
				else {
					realpath(path, retf);
					strcat(retf, "/");
					strcat(retf, file); // add the actual file
					if ( c_isdir(retf) ) { // its supposed never happens
						strcpy(path, retf);
						continue;
						}
					logpf("file selected and returned '%s'\n", retf);
					break;
					}
				} // file mode
			break;
			}
		
		else if ( r == -1 ) {
			// cancel
			logpf("Popup canceled, return -1 and empty string\n");
			last[0] = retf[0] = '\0';
			break;
			}
		
		else if ( r == -2 ) {
			// a typed text not belong to list	(newfile/newtext)

			if ( !b_filesel && b_newtext ) {
				// not file-box, new text entered
				logpf("new-text selected and returned '%s'\n", retf);
				break; // allow and exit
				}

			if ( b_filesel && b_newfile ) {
				char	file[PATH_MAX];
				char	sdir[PATH_MAX];
				char	*p;

				wcrd[0] = '\0';				// reset wildcards
				
				getcwd(path, PATH_MAX);		// get current path
				strcpy(sdir, retf);			// sdir = input string
				if ( c_isdir(sdir) ) {		// is it directory ?
					if ( sdir[0] == '/' )	// directory from root
						strcpy(path, sdir);	// new path defined
					else { 					// subdirectory/ies
						strcat(path, "/");
						strcat(path, sdir);	// new path defined
						}
					continue;				// rebuild the list-box
					}

				p = strrchr(sdir, '/');		// the input string contains filename
				if ( p ) {
					*p = '\0';
					if ( !c_isdir(sdir) )	// wrong sub-dir, does not exist
						continue;			// rebuild - error
					else {
						strcat(path, "/");
						strcat(path, sdir);	// new path defined
						}
					strcpy(file, p + 1);	// the filename
					}
				else
					strcpy(file, retf);		// the input string is only file name

check_filename:
				logpf("=== Check filename [%s] procedure ===\n", file);
				
				// check for wildcards, if true update wcrd and rebuild
				if ( strwc(file) ) {
					strcpy(wcrd, file);
					logpf("Wildcards found, rebuild\n");
					continue; // rebuild
					}

				snprintf(retf, PATH_MAX, "%s/%s", path, file); // full-path file-name 
				logpf("Filename '%s' allowed\n", file);
				break; // allow and exit
				}
			}
		
		} while ( true );

	chdir(cwd);

	//
	c_scr_touch();	// mark the area as dirty
	c_scr_redraw(); // restore screen

	if ( files )
		files = strlist_destroy(files, &count);
	// push copy of the string in interpreters stack, this is the returned string
	if ( r >= 0 || (b_newfile && r == -2) )
		SLang_push_string(retf);
	else
		SLang_push_null();
	}

// --------------------------------------------------------------------------------
// interface with S-Lang

// file-list which allows new files
void c_pop_edits(const char *title, const char *dirp) // returns string in stack
{ c_popup_files(PUM_FILE_SELECT | PUM_SORT_ELEMENTS | PUM_ALLOW_NEW_FILE | PUM_USE_SEARCH, title, dirp); }

// file-list, just select file
void c_pop_files(const char *title, const char *dirp) // returns string in stack
{ c_popup_files(PUM_FILE_SELECT | PUM_SORT_ELEMENTS | PUM_USE_SEARCH, title, dirp); }

// listbox dialog
int_t dialog_listbox_sl(const char *title, SLang_Array_Type *ac, int_t *defsel, size_t *pflags, const char *footer, const char *hkfunc) {
	char info[LINE_MAX]; // this can be used if user does not use hook
	size_t i, count;
	int_t  r;
	size_t flags = ( pflags == NULL ) ? PUM_USE_SEARCH : *pflags;

	info[0] = '\0';
	char **list = array_to_strlist(ac, &count);
	if ( hkfunc && *hkfunc )
		r = c_popup_menu(flags, list, count, *defsel, title, footer, info, pum_hook, hkfunc);
	else
		r = c_popup_menu(flags, list, count, *defsel, title, footer, info, NULL, NULL);
	list = strlist_destroy(list, &count);
	return r;
	}
int_t dialog_listbox_sl3(const char *title, SLang_Array_Type *ac, int_t *defsel)
{ return dialog_listbox_sl(title, ac, defsel, NULL, NULL, NULL); }
int_t dialog_listbox_sl4(const char *title, SLang_Array_Type *ac, int_t *defsel, size_t *pflags)
{ return dialog_listbox_sl(title, ac, defsel, pflags, NULL, NULL); }

// panic() for slang
void c_exit_error(char *s)
{ exit_error(s, 0); }

// typical messagebox
int_t c_msgbox_sl(const char *title, const char *msg)
{ return c_msgbox(0x00, title, NULL, "%s", msg); }

// message box with text-box
int_t c_txtbox_sl(const char *title, const char *msg, char *text)
{ return c_msgbox(0x04, title, text, "%s", msg); }

/*
 * setup S-Lang interface
 */
static SLang_Intrin_Fun_Type CBRIEF_Intrinsics [] = {
	MAKE_INTRINSIC_1("panic",           c_exit_error,       VOID_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_S("set_last_macro",  set_last_macro,     VOID_TYPE),
	MAKE_INTRINSIC_5("dlg_bufmenu",     dialog_bufmenu_sl,  SWORD_TYPE, ARRAY_TYPE, SWORD_TYPE, STRING_TYPE, STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_6("dlg_listbox",     dialog_listbox_sl,  SWORD_TYPE, STRING_TYPE, ARRAY_TYPE, SWORD_TYPE, WORD_TYPE, STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_3("dlg_listbox3",    dialog_listbox_sl3, SWORD_TYPE, STRING_TYPE, ARRAY_TYPE, SWORD_TYPE),
	MAKE_INTRINSIC_4("dlg_listbox4",    dialog_listbox_sl4, SWORD_TYPE, STRING_TYPE, ARRAY_TYPE, SWORD_TYPE, WORD_TYPE),
	MAKE_INTRINSIC_2("dlg_openfile",    c_pop_edits,        VOID_TYPE,  STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("dlg_selectfile",  c_pop_files,        VOID_TYPE,  STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_2("c_msgbox",    c_msgbox_sl,        SWORD_TYPE,  STRING_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_3("c_txtbox",    c_txtbox_sl,        SWORD_TYPE,  STRING_TYPE, STRING_TYPE, STRING_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

static SLang_Intrin_Var_Type CBRIEF_Variables [] = {
	MAKE_VARIABLE("CBRIEF_API",       &cbrief_api_ver,	WORD_TYPE, 0),
	MAKE_VARIABLE("CBRIEF_FLAGS",     &cbrief_flags,	WORD_TYPE, 0),
	MAKE_VARIABLE("CBRIEF_SEL_LINE",  &cbrief_sel_line, WORD_TYPE, 0),
	MAKE_VARIABLE("CBRIEF_SEL_COL",   &cbrief_sel_col,  WORD_TYPE, 0),
	MAKE_VARIABLE(NULL, NULL, 0, 0)
	};

/*
 *	initialize slang members
 */
extern size_t c_hash_init();	// XXH3 - the best and fast hash by far, works in 64bit...
							// tested latin1 dict 123K words, not one conflict
extern size_t c_osl_init();	// simple and optimized sets; slang already has similar
							// text routines, not actually needed

int_t cbrief_slang_init() {
	if ( !c_hash_init() )	return 0;
	if ( !c_getkey_init() )	return 0;	// getkey and getwckey with alt codes and ESCAPE! yeah!
	if ( !c_files_init() )	return 0;	// nothing special, propably will be removed
	if ( !c_osl_init() )	return 0;
	if ( !c_screen_init() )	return 0;	// just a few SLsmg routines that used in jed,
										// still there is not a sure pattern how to use them
	if ( !c_string_init() )	return 0;	// more string routines, strlist dynamic arrays,
										// not more type-errors, powerfull shell like split...
	if ( !c_misc_init() )	return 0;
	if ( SLadd_intrin_fun_table(CBRIEF_Intrinsics, NULL) == -1 )	return 0;
	if ( SLadd_intrin_var_table(CBRIEF_Variables,  NULL) == -1 )	return 0;
	SLdefine_for_ifdef("CBRIEF_PATCH");
	return 1;
	}

