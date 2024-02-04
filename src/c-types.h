/*
 * For jed/slang, define int size to CPU's word size
 */

#if !defined(__INT_IS_WORD__)
#define __INT_IS_WORD__

/*
 * CPU x86_64 "Integer" Models
 * 
 *		Type           ILP64   LP64   LLP64
 *		-----------------------------------
 *		char              8      8       8
 *		short            16     16      16
 *		int              64     32      32
 *		long             64     64      32
 *		long long        64     64      64
 *		pointer          64     64      64
 */

#include <stdint.h>
#include <limits.h>
#include "slang.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(LINE_MAX)
	#define LINE_MAX	4096
#endif

#define INT32_TYPE	INT_TYPE
#define UINT32_TYPE	UINT_TYPE
#define INT64_TYPE	LONG_TYPE
#define UINT64_TYPE	SLANG_ULONG_TYPE

/*
 *	integer size = register size = CPU word size
 *	This is true on any case by hardware definition!
 * 
 *  Also, register size it must be equal to pointer size,
 *	at least in most cases.
 */

/* long == big int */
#if defined(__GNUC__) && (__WORDSIZE == 64)
	typedef __int128			long_t;
	typedef unsigned __int128	ulong_t;
#else
	typedef intmax_t		long_t;
	typedef uintmax_t		ulong_t;
#endif

/* int == word, real = word, big real = double word */
#if defined(_LP64) || defined(_LLP64)
	typedef intptr_t		int_t;
//	typedef uintptr_t		uint_t;
	typedef double			real_t;
	typedef long double		bigr_t;

	#define __WORDSIZE64		1
	#define	INTFMT			"%jd"
	#define	UINTFMT			"%ju"
	#define WORD_TYPE		SLANG_ULONG_TYPE
	#define SWORD_TYPE		SLANG_LONG_TYPE
#else
	#if defined(_ILP64)
		#define __WORDSIZE64	1
	#endif
	typedef int			int_t;
//	typedef unsigned	uint_t;
	typedef double		real_t;	/* 32/64 x86 */
	typedef long double	bigr_t;	/* 32/64 x86 */

	#define	INTFMT			"%d"
	#define	UINTFMT			"%u"
	#define WORD_TYPE		SLANG_UINT_TYPE
	#define SWORD_TYPE		SLANG_INT_TYPE
#endif

/* print information */
#define _INT_PRINT()	{\
	printf("sizeof(int)=%d, sizeof(long)=%d, sizeof(long long)=%d, sizeof(void*)=%d\n",\
		sizeof(int), sizeof(long), sizeof(long long), sizeof(void*));\
	printf("sizeof(int_t)=%d, sizeof(long_t)=%d, sizeof(real_t)=%d, sizeof(bigr_t)=%d\n",\
		sizeof(int_t), sizeof(long_t), sizeof(real_t), sizeof(bigr_t));\
	}
	
#if defined(__cplusplus)
	}
#endif

#endif
