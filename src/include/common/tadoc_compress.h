/** ----------
 * tadoc_compress.h -
 *
 *	Definitions for the builtin TADOC compressor
 *
 *  src/include/common/tadoc_compress.h
 * ----------
 */

#ifndef _TADOC_COMPRESS_H_
#define _TADOC_COMPRESS_H_
#define TADOC_MAX_OUTPUT(_dlen)			((_dlen) + 4)

typedef struct TADOC_Strategy
{
    int32 min_input_size;
    int32 max_input_size;
    int32 min_comp_rate;
}TADOC_Strategy;
extern PGDLLIMPORT const TADOC_Strategy *const TADOC_strategy_default;
extern int32 tadoc_compress(const char *source, int32 slen, char *dest,
                            const TADOC_Strategy *strategy);
extern int32 tadoc_decompress(const char *source, int32 slen, char *dest,
				int32 rawsize, bool check_complete);

#endif