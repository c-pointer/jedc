/*
 *	panic & logfile routines
 *
 *	Copyright (C) 1991-2022 Free Software Foundation, Inc.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#if !defined(__PANIC_H) 
#define __PANIC_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#if defined(__linux__)
	#include <syslog.h>
#endif
#include <assert.h>
#include "c-types.h"
#include "c-screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

// the most famous function in whole universe; created at
// 1 Jan 1970 at 0 seconds since the big bang started :P
#if defined(__linux__)
	#define	panic(FMT, ...) \
		{ int e = errno; \
		syslog(0, FMT, ##__VA_ARGS__); \
		c_scr_shutdown();\
		fprintf(stderr, "[%16jd] %s:%d errno=%d = %s\n", time(NULL), __FILE__, __LINE__, e, strerror(e)); \
		fprintf(stderr, FMT, ##__VA_ARGS__); \
		abort(); }
#else
	#define	panic(FMT, ...) \
		{ int e = errno; \
		c_scr_shutdown();\
		fprintf(stderr, "[%16jd] %s:%d errno=%d = %s\n", time(NULL), __FILE__, __LINE__, e, strerror(e)); \
		fprintf(stderr, FMT, ##__VA_ARGS__); \
		abort(); }
#endif

// if expression "EXP" has true value runs panic
#define	panic_if(EXP, FMT, ...) \
	{ if ( EXP ) panic(FMT ##__VA_ARGS__); }

// if expression "EXP" has true value runs panic
#define	panic_ifnot(EXP, FMT, ...) \
	{ if ( EXP ) panic(FMT ##__VA_ARGS__); }

// while the panic was a small routine in all unix program,
// more advanced version wanted preprocessor; so that is the
// upper name of panic.
#define PANIC(...) panic(__VA_ARGS__)

// standard log-file routines
#if defined(NO_LOGFILE)
	#define setlogfile(s);
	#define logpf(f, ...);
#else
	void setlogfile(const char *);
	void logpf(const char *fmt, ...);
#endif

#if defined(__cplusplus)
	}
#endif

#endif
