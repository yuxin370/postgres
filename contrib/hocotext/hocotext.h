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
#include "portability/instr_time.h"
#include "tsearch/ts_utils.h"
#include <sys/time.h>

/**
 * utility functions
*/

#define buf_copy_ctrl(__buf,__sp) \
do { \
    *__buf = *__sp;                     \
    __buf ++;                           \
    __sp ++;                            \
}while(0)

#define repeat_buf_copy(__dp,__str,__count) \
do{ \
    for(int i = 0; i < __count; i++){       \
        *__dp = __str;                      \
        __dp ++;                            \
    }\
}while(0)

// big endian
#define buf_get_int(__bp) \
({ \
    int32 value = 0;                                                  \
    for(int seg = 3 ; seg >= 0 ; seg --,__bp++){                     \
        value |=  (int32)((*__bp)&0xFF) << (8*seg);                 \
    }                                                               \
    value;                                                          \
})

// big endian
#define buf_put_int(__bp,__v) \
do { \
    for(int seg = 3 ; seg >= 0 ; seg --,__bp++){      \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                               \
}while(0)


#define RAWDATA_BYTE 4

static double
elapsed_time(instr_time *starttime)
{
	instr_time	endtime;

	INSTR_TIME_SET_CURRENT(endtime);
	INSTR_TIME_SUBTRACT(endtime, *starttime);
	return INSTR_TIME_GET_DOUBLE(endtime);
}


/**
 * ************************************************************
 *                  HLEP HANDLE FUNCTIONS                     *
 * ************************************************************
*/
extern int32 hocotext_hoco_cmp_helper(struct varlena * left, struct varlena * right, Oid collid);
extern text * hocotext_hoco_extract_helper(struct varlena * source,int32 offset,int32 len,Oid collid);
extern text * hocotext_hoco_insert_helper(struct varlena * source,int32 offset,text * str,Oid collid);
extern text * hocotext_hoco_overlay_helper(struct varlena * source,int32 offset,int32 len,text * str,Oid collid);
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
 * Definitions
 */

#define MAX_REPEATED_SIZE 130
#define MAX_SINGLE_STORE_SIZE 127
#define THRESHOLD 3


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

extern int32 rle_compress_ctrl(unsigned char *sp,unsigned char *srcend,unsigned char *dp);
extern text * rle_compress(struct varlena *source, const RLE_Strategy *strategy, Oid collid);
extern text * rle_decompress(struct varlena *source, Oid collid);

/*
 * hocotext_rle_*_cmp()
 * Internal comparison function for hocotext_rle strings (and common text strings).
 * Returns int32 negative, zero, or positive.
 */

extern int32 hocotext_rle_hoco_cmp(struct varlena * left, struct varlena * right, Oid collid);

/**
 * hocotext_rle_*_*()
 * internal operation functions
*/
extern text * hocotext_rle_hoco_extract(struct varlena * source,int32 offset,int32 len,Oid collid);
extern text * hocotext_rle_hoco_insert(struct varlena * source,int32 offset,text *str,Oid collid);
extern text * hocotext_rle_hoco_overlay(struct varlena * source,int32 offset,int32 len,text *str,Oid collid);
extern text * hocotext_rle_hoco_delete(struct varlena * source,int32 offset,int32 len,Oid collid);
extern int32 hocotext_rle_hoco_char_length(struct varlena * source,Oid collid);
extern text * hocotext_rle_hoco_concat(struct varlena * left,struct varlena * right,Oid collid);

/**
 * tadoc_(de)compress()
 * internal compression module
*/

/**
 * tadoc to_tsvector
*/
typedef struct ParsedRule {
	// whether parsed or not
	uint8_t is_parsed;
	// length of the word array of this rule
	uint32_t num_words;
	// start position in the array
	ParsedWord* parse_start;
	// the real length of this rule
	uint32_t real_length;
};

/**
 * TADOC Decompressed Rule Entry
*/
struct DecompRule {
    // o means the rule have not been finish decompression step yet
    uint8_t finished;  
    char* decomp_start;
    uint32_t decomp_len;
};

typedef struct ParsedRule ParsedRule;
// typedef ParsedRule ParsedRule;
typedef struct DecompRule DecompRule;
// type to indicate the id of rule
#ifdef MINI_NUM_RULES
	typedef uint16_t rule_index_t;
#else
	typedef uint32_t rule_index_t;
#endif
typedef uint32_t rule_offset_t;
#define RESERVE_CHAR '^'

extern text * tadoc_compress(struct varlena *source, Oid collid);
extern text * tadoc_decompress(struct varlena *source, Oid collid);
extern TSVector tadoc_to_tsvector(char* tadoc_comp_data);


#endif          /*HOCOTEXT_H*/