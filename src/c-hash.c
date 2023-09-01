/*
 *	XXH3, XXH32, XXH64 - hash encoding functions
 *
 *	See xxhash.h for authors and license details
 */

#include "slang.h"
#include <string.h>

#define XXH_IMPLEMENTATION
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"

int32_t xxh32(const char *s, int32_t seed)	{ return XXH32(s, strlen(s), seed); }
int64_t xxh64(const char *s, int64_t seed)	{ return XXH64(s, strlen(s), seed); }
int64_t xxh3(const char *s)					{ return XXH3_64bits(s, strlen(s)); }
int64_t xxh64_hash_file(const char *file) {
	FILE	*fp = fopen(file, "rb");
	if ( !fp ) return 0;
	
	// Allocate a state struct. Do not just use malloc() or new.
	XXH3_state_t* state = XXH3_createState();
	// Reset the state to start a new hashing session.
	XXH3_64bits_reset(state);
	char buffer[4096];
	size_t count;

	// Read the file in chunks
	while ( (count = fread(buffer, 1, sizeof(buffer), fp)) != 0 ) {
		// Run update() as many times as necessary to process the data
		XXH3_64bits_update(state, buffer, count);
		}
	
	// Retrieve the finalized hash. This will not change the state.
	XXH64_hash_t result = XXH3_64bits_digest(state);
	// Free the state. Do not use free().
	XXH3_freeState(state);
	fclose(fp);
	return result;
	}

// --------------------------------------------------------------------

int32_t xxh32_sl(const char *s, int32_t *seed)	{ return XXH32(s, strlen(s), *seed); }
int64_t xxh64_sl(const char *s, int64_t *seed)	{ return XXH64(s, strlen(s), *seed); }

#include "c-types.h"

static SLang_Intrin_Fun_Type C_HASH_Intrinsics [] = {
	MAKE_INTRINSIC_2("XXH32",       xxh32_sl,           INT32_TYPE,  STRING_TYPE, INT32_TYPE),
	MAKE_INTRINSIC_2("XXH64",       xxh64_sl,           UINT64_TYPE, STRING_TYPE, SLANG_ULONG_TYPE),
	MAKE_INTRINSIC_1("XXH3",        xxh3,               UINT64_TYPE, STRING_TYPE),
	MAKE_INTRINSIC_1("XXH64_hash_file",xxh64_hash_file, UINT64_TYPE, STRING_TYPE),
	SLANG_END_INTRIN_FUN_TABLE
	};

size_t c_hash_init() {
	if ( SLadd_intrin_fun_table(C_HASH_Intrinsics, NULL) == -1 ) return 0;
	return 1;
	}
