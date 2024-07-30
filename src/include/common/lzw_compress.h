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
#define LZW_MAX_OUTPUT(_dlen)			((_dlen)+2048)
#include "common/uthash.h"
#include <limits.h>

#define MAX_WORDS_COUNT 99999999
#define MAX_ENTRY_COUNT 9999999
#define MAX_ENTRY_SIZE 999999

#define move_ptr(cur_buf,word_size)     \
    cur_buf += word_size;               \
    *cur_buf = '\0'

#define buf_put_dict_entry(__bp,__len,__key,__id)                       \
do{                                                                     \
    buf_put_int(__bp,__len);                                           \
    memcpy(__bp,__key,__len);                                           \
    __bp+=__len;                                                        \
    buf_put_int(__bp,__id);                                            \
}while(0)

#define buf_get_dict_entry(__bp)                                        \
do{                                                                     \
    int32 len = buf_get_int(__bp);                                     \
    char tmp[MAX_ENTRY_SIZE];                                           \
    memcpy(tmp,__bp,len);                                               \
    tmp[len] = '\0';                                                    \
    __bp+=len;                                                          \
    int32 id = buf_get_int(__bp);                                      \
    hash_insert_rev_without_check(tmp,id,tmp);                          \
}while(0)

typedef struct hash_entry {
    char *key;                 /* key */
    int32 id;
    UT_hash_handle hh1;         /* makes this structure hashable, hh1 for key*/
}hash_entry;


typedef struct hash_entry_rev {
    int32 id;                     /* key */
    char *key;   
    char *first;                  /* P */
    char *id_seq;                 /* id sequence*/
    UT_hash_handle hh2;         /* makes this structure hashable, hh1 for key*/
}hash_entry_rev;


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


/**
 * @brief parse and print char-array as int32-array 
*/
extern void print_int(char * dest,char *end);

extern void print_int8(char * dest,char *end);

/***
 * @brief search an entry in dict.
 * return hash for success and NULL for failure.
*/
struct hash_entry* hash_find(char* ikey);

struct hash_entry_rev* hash_find_rev(int32 id);

/***
 * @brief insert an entry to dic.
 * insert a new entry if not in dict yet, or just modify the value.
*/
extern void hash_insert(char* ikey, int32 id);


extern void hash_insert_rev(char* ikey, int32 id, char * ifirst);

// extern void hash_insert_rev_fill_seq(char* ikey, char* id_seqs,int32 id, char * ifirst);

// extern void hash_insert_rev_without_check_fill_seq(char* ikey, char* id_seqs,int32 id, char * ifirst);

extern void hash_insert_rev_without_check(char* ikey, int32 id, char * ifirst);

/***
 * @brief delete an entry with ikey from hash_table;
*/
int32 hash_delete(char* ikey);

int32 rev_hash_delete(int32 id);

/**
 * @brief clear the hash_table
*/
extern void hash_clear();

extern void hash_clear_rev();

/**
 * @brief print the hash_table
*/
extern void hash_print(int32 type);

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