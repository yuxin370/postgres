/*
 * contrib/hocotext/hocotext.h
 */
#ifndef HOCOTEXT_H
#define HOCOTEXT_H

#include "postgres.h"
#include "catalog/pg_collation.h"
#include "common/hashfn.h"
#include "utils/builtins.h"
#include "utils/formatting.h"

#include "access/table.h"
#include "access/tableam.h"
#include "utils/varlena.h"
#include "varatt.h"
#include "fmgr.h"
#include "access/detoast.h"
#include "access/toast_internals.h"
#include "postgres_ext.h"

/**
 * utility functions
*/

#define buf_copy_ctrl(__buf,__dp) \
do { \
    *__buf = *__dp;                     \
    __buf ++;                           \
    __dp ++;                            \
}while(0)

#define repeat_buf_copy(__dp,__str,__count) \
do{ \
    for(int i = 0; i < __count; i++){       \
        *__dp = __str;                      \
        __dp ++;                            \
    }\
}while(0)

// little endian
#define buf_get_int(__bp) \
({ \
    int32 value = 0;                                                  \
    for(int seg = 0 ; seg < 4 ; seg ++,__bp++){                     \
        value |=  (int32)((*__bp)&0xFF) << (8*seg);                 \
    }                                                               \
    value;                                                          \
})

// little endian
#define buf_put_int(__bp,__v) \
do { \
    for(int seg = 0 ; seg < 4 ; seg ++,__bp++){      \
        (*__bp) = (char)( ( __v >> ( 8 * seg ) ) & 0xFF );          \
    }                                                               \
}while(0)


#define RAWDATA_BYTE 4
/**
 * ************************************************************
 *                  HLEP HANDLE FUNCTIONS                     *
 * ************************************************************
*/
extern int32 hocotext_hoco_cmp_helper(struct varlena * left, struct varlena * right, Oid collid);
extern text * hocotext_hoco_extract_helper(struct varlena * source,int32 offset,int32 len,Oid collid);
extern text * hocotext_hoco_insert_helper(struct varlena * source,int32 offset,text * str,Oid collid);
extern text * hocotext_hoco_delete_helper(struct varlena * source,int32 offset,int32 len,Oid collid);
/**
 * ************************************************************
 *                 COMMON UTILITY FUNCTIONS                   *
 * ************************************************************
*/
extern int32 hocotext_common_cmp(struct varlena * left, struct varlena * right, Oid collid);
extern text * hocotext_common_extract(struct varlena * source,int32 offset,int32 len,Oid collid);

/**
 * ************************************************************
 *                    RLE UTILITY FUNCTIONS                   *
 * ************************************************************
*/

/**
 * rle_(de)compressi()
 * internal compression module
*/
typedef struct RLE_Strategy
{
    int32 min_input_size;
    int32 max_input_size;
    int32 min_comp_rate;
} RLE_Strategy;
extern const RLE_Strategy *const RLE_strategy_default;
extern text * rle_compress(struct varlena *source, const RLE_Strategy *strategy,Oid collid);
extern text * rle_decompress(struct varlena *source,Oid collid);

/*
 * hocotext_rle_*_cmp()
 * Internal comparison function for hocotext_rle strings (and common text strings).
 * Returns int32 negative, zero, or positive.
 */

extern int32 hocotext_rle_hoco_cmp(struct varlena * left, struct varlena * right, Oid collid);
extern int32 hocotext_rle_mixed_cmp(struct varlena * left, struct varlena * right, Oid collid);

/**
 * hocotext_rle_*_*()
 * internal operation functions
*/
extern text * hocotext_rle_hoco_extract(struct varlena * source,int32 offset,int32 len,Oid collid);
extern text * hocotext_rle_hoco_insert(struct varlena * source,int32 offset,text *str,Oid collid);
extern text * hocotext_rle_hoco_delete(struct varlena * source,int32 offset,int32 len,Oid collid);


/**
 * ************************************************************
 *                    LZ4 UTILITY FUNCTIONS                   *
 * ************************************************************
*/
extern text * hocotext_lz4_hoco_decompression(struct varlena * source,int32 offset,int32 len,Oid collid);
extern text * hocotext_lz4_hoco_extract(struct varlena * source,int32 offset,int32 len,Oid collid);
extern int32 hocotext_lz4_hoco_cmp(struct varlena * left, struct varlena * right, Oid collid);
// extern int32 hocotext_lz4_get_next_group(char *sp,char *srcend,char *buffer);
extern text * hocotext_lz4_hoco_extract_naive(struct varlena * source,int32 offset,int32 len,Oid collid);

#endif          /*HOCOTEXT_H*/