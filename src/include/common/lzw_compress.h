/** ----------
 * yuxin tang
 * lzw_compress.h -
 *
 *	Definitions for the builtin LZW compressor
 *
 *  src/include/common/lzw_compress.h
 * ----------
 */

#ifndef _LZW_COMPRESS_H_
#define _LZW_COMPRESS_H_
#define LZW_MAX_OUTPUT(_dlen)			((_dlen) + 4)

// big endian
#define buf_put_int(__bp,__v)                                                   \
do {                                                                            \
    for(int32 seg = 3 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int(__bp)                                           \
({                                                                  \
    int32 value = 0;                                                  \
    for(int32 seg = 3 ; seg >= 0 ; seg --,__bp++){                    \
        value |=  (int32)((*__bp)&0xFF) << (8*seg);                   \
    }                                                               \
    value;                                                          \
})


// big endian
#define buf_put_int8(__bp,__v)                                                  \
do {                                                                            \
    for(int32 seg = 0 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int8(__bp)                                              \
({                                                                      \
    int32 value = 0;                                                      \
    for(int32 seg = 0 ; seg >= 0 ; seg --,__bp++){                        \
        value |=  (int32)((*__bp)&0xFF) << (8*seg);                       \
    }                                                                   \
    value;                                                              \
})


// big endian
#define buf_put_int16(__bp,__v)                                                 \
do {                                                                            \
    for(int32 seg = 1 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int16(__bp)                                                 \
({                                                                          \
    int32 value = 0;                                                          \
    for(int32 seg = 1 ; seg >= 0 ; seg --,__bp++){                            \
        value |=  (int32)((*__bp)&0xFF) << (8*seg);                           \
    }                                                                       \
    value;                                                                  \
})

typedef struct LZW_Strategy
{
    int32 min_input_size;
    int32 max_input_size;
    int32 min_comp_rate;
}LZW_Strategy;
extern PGDLLIMPORT const LZW_Strategy *const LZW_strategy_default;
extern int32 lzw_compress(const char *source, int32 slen, char *dest,
                            const LZW_Strategy *strategy);
extern int32 lzw_decompress(const char *source, int32 slen, char *dest,
				int32 rawsize, bool check_complete);

#endif