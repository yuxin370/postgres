/**
 *
 * 
 * tadoc_compress.c
 * 
 *    an implementation of TADOC for PostgreSQL
*/

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif


#include <limits.h>
#include <string.h>
#include "varatt.h"
#include "common/tadoc_compress.h"


/* ----------
 * Local definitions
 * ----------
 */

/**
 * provided standard strategies
 * basic compression restriction
*/
static const TADOC_Strategy tadoc_default_strategy = {
	32,							/* Data chunks less than 32 bytes are not
								 * compressed */
	INT_MAX,					/* No upper limit on what we'll try to
								 * compress */
	25							/* Require 25% compression rate, or not worth
								 * it */
};
const TADOC_Strategy *const TADOC_strategy_default = &tadoc_default_strategy;


int32 tadoc_compress(const char *source, int32 slen, char *dest,
                    const TADOC_Strategy *strategy){
    
    //todo
    return 0;//return compressed data size;
}

/**
 * tadoc_decompress -
 * 
 * 
 *      Decompress source into dest.
 *      Return the number of bytes decompressed into the destination buffer, 
 *      or -1 if the compressed data is corrupted
 *  	
 *      If check_complete is true, the data is considered corrupted
 *		if we don't exactly fill the destination buffer.  Callers that
 *		are extracting a slice typically can't apply this check.
*/

int32
tadoc_decompress(const char *source, int32 slen, char *dest,
				int32 rawsize, bool check_complete){
    // todo
    return 0;

}