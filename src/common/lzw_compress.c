/**
 * 2023.9.30
 * Yuxin Tang
 * 
 * lzw_compress.c
 * 
 *    an implementation of LZW for PostgreSQL
 * 
 * 
*/

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif


#include <limits.h>
#include <string.h>
#include "varatt.h"
#include "common/lzw_compress.h"
#include "common/uthash.h"

/* ----------
 * Local definitions
 * ----------
 */

static const LZW_Strategy lzw_default_strategy = {
	32,							/* Data chunks less than 32 bytes are not
								 * compressed */
	INT_MAX,					/* No upper limit on what we'll try to
								 * compress */
	25							/* Require 25% compression rate, or not worth
								 * it */
};
const LZW_Strategy *const LZW_strategy_default = &lzw_default_strategy;


#define MAX_WORDS_COUNT 99999999
#define MAX_ENTRY_COUNT 9999999
#define MAX_ENTRY_SIZE 999999

#define move_ptr(cur_buf,word_size)     \
    cur_buf += word_size;               \
    *cur_buf = '\0'

#define buf_put_dict_entry(__bp,__len,__key,__id)                       \
do{                                                                     \
    buf_put_int16(__bp,__len);                                           \
    memcpy(__bp,__key,__len);                                           \
    __bp+=__len;                                                        \
    buf_put_int16(__bp,__id);                                            \
}while(0)

#define buf_get_dict_entry(__bp)                                        \
do{                                                                     \
    int32 len = buf_get_int16(__bp);                                     \
    char tmp[MAX_ENTRY_SIZE];                                           \
    memcpy(tmp,__bp,len);                                               \
    tmp[len] = '\0';                                                    \
    __bp+=len;                                                          \
    int32 id = buf_get_int16(__bp);                                      \
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
    UT_hash_handle hh2;         /* makes this structure hashable, hh1 for key*/
}hash_entry_rev;

hash_entry *dict = NULL;
hash_entry_rev *dict_rev = NULL;


/*
 * Just get a word....
 * When met a non-boundary char just store it in a buffer
 * Else return the word in buffe and then return the boundary char
 */

int32 getWord(char *buf,char *res) {
    char *ptr = res;
    char c;                        // current char
    static int32 indicator = 0; // flags
    static char old;               // last char
    char* myStr = (char *)palloc(MAX_ENTRY_SIZE);
    memset(myStr,0,5);
    if (indicator == 1) { // last time meet a normal char
        indicator = 0;
        *res = old;
        *(res+1) = '\0';
        return 1; // return the old char as a string
    }
    // every time get a single char
    while ((c = *buf++) != 0) {

        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' ||
              c == '.' || c == ';' || c == '@' || c == '?' || c == '"' || c == '\'')) {
            // if not a boundary just return the char
            *ptr++ = c;       // add c to the buffer
            indicator = 1; // set the indicator
        } else {
            // arrive the boundary c between words, last time not meet a normal
            // char
            if (indicator == 0){
                *res = c;
                *(res+1) = '\0';
                return 1;
            }
            // if have met a boundary, last time meet a normal char
            old = c;
            *ptr = '\0'; // end the string (not include the boundary chars)
            return (ptr-res) ;
        }
    }

    if(ptr-res != 0){
        // arrive the boundary c between words, last time not meet a normal char
        if (indicator == 0){
            *res = c;
            *(res+1) = '\0';
            return 1;
        }
        *ptr = '\0'; // end the string (not include the boundary chars)
        return (ptr-res) ;
    }

    return 0; // when meet the end, return a NULL char
}

/**
 * @brief parse and print char-array as int32-array 
*/
void print_int(char * dest,char *end){
    while(dest < end){
        printf("%d ",buf_get_int(dest));
    }
    printf("\n");
}

void print_int16(char * dest,char *end){
    while(dest < end){
        printf("%d ",buf_get_int16(dest));
    }
    printf("\n");
}


/***
 * @brief search an entry in dict.
 * return hash for success and NULL for failure.
*/
struct hash_entry* hash_find(char* ikey) {
    struct hash_entry* tmp;
    HASH_FIND(hh1, dict, ikey, strlen(ikey), tmp);
    return tmp;
}

struct hash_entry_rev* hash_find_rev(int32 id) {
    struct hash_entry_rev* tmp;
    HASH_FIND(hh2, dict_rev, &id, sizeof(int32), tmp);
    return tmp;
}


/***
 * @brief insert an entry to dic.
 * insert a new entry if not in dict yet, or just modify the value.
*/
void hash_insert(char* ikey, int32 id) {
    struct hash_entry* it;

    it = hash_find(ikey);
    if (it == NULL) {
        struct hash_entry* tmp = (struct hash_entry *)palloc(sizeof *tmp);
        tmp->key = (char *)palloc(strlen(ikey));
        strcpy(tmp->key,ikey);
        tmp->id = id;
        HASH_ADD_KEYPTR(hh1, dict, tmp->key, strlen(tmp->key), tmp);
    } else {
        it->id = id;
    }
}


void hash_insert_rev(char* ikey, int32 id, char * ifirst) {
    struct hash_entry_rev* it_rev;

    it_rev =  hash_find_rev(id);
    if (it_rev == NULL) {
        hash_insert_rev_without_check(ikey,id,ifirst);
    } else {
        strcpy(it_rev->key,ikey);
        strcpy(it_rev->first,ifirst);
    }
}

void hash_insert_rev_without_check(char* ikey, int32 id, char * ifirst) {
    struct hash_entry_rev* tmp = (struct hash_entry_rev *)palloc(sizeof *tmp);
    tmp->key = (char *)palloc(strlen(ikey));
    tmp->first = (char *)palloc(strlen(ifirst));
    strcpy(tmp->key,ikey);
    strcpy(tmp->first,ifirst);
    tmp->id = id;
    HASH_ADD_KEYPTR(hh2, dict_rev, &(tmp->id), sizeof(int32), tmp);
}

/***
 * @brief delete an entry with ikey from hash_table;
*/
int32 hash_delete(char* ikey) {
	struct hash_entry* it = hash_find(ikey);
	if (it != NULL) {
		HASH_DELETE(hh1, dict, it);
	    free(it);
	    it = NULL;
	    return 1;
	}else{
		return 0;
	}
}

int32 rev_hash_delete(int32 id) {
	struct hash_entry_rev* it = hash_find_rev(id);
	if (it != NULL) {
		HASH_DELETE(hh2, dict_rev, it);
	    free(it);
	    it = NULL;
	    return 1;
	}else{
		return 0;
	}
}

/**
 * @brief clear the hash_table
*/
void hash_clear(){
    // free the hash table contents 
    struct hash_entry *s, *tmp;
    HASH_ITER(hh1, dict, s, tmp) {
      HASH_DELETE(hh1, dict, s);
      free(s);
    }
}

void hash_clear_rev(){
    // free the hash table contents 
    struct hash_entry_rev *s, *tmp;
    HASH_ITER(hh2, dict_rev, s, tmp) {
      HASH_DELETE(hh2, dict_rev, s);
      free(s);
    }
}

/**
 * @brief print the hash_table
*/
void hash_print(int32 type){
    // print the hash table contents 
    if(type == 1){
        printf("------------------------\n");
        printf("|        key   |   id  |\n");
        printf("------------------------\n");
        struct hash_entry *s, *tmp;
        HASH_ITER(hh1, dict, s, tmp) {
        printf("|%14s|%7d|\n",s->key,s->id);
        }
        printf("------------------------\n");

    }else{
        printf("--------------------------------\n");
        printf("|        key   |   id  | first |\n");
        printf("--------------------------------\n");
        struct hash_entry_rev *s, *tmp;
        HASH_ITER(hh2, dict_rev, s, tmp) {
        printf("|%14s|%7d|%7s|\n",s->key,s->id,s->first);
        }
        printf("--------------------------------\n");
    
    }

}

/***
 * 
 * dictionary format:
 * -------------------------------------------------------------------------
 * |  entry_count  |    key_len   |     key      |      id      |   ...
 * ------------------------------------------------------------------------- 
 * |    int8       |     int8     |    char[]    |     int8     |   ...
 * -------------------------------------------------------------------------
 * 
*/
int32 lzw_compress_ctrl(char *sp,char *srcend,char *dp){
    printf("---------------- using compression !\n");
	char *dstart = dp;                         //start of compressed data
    char *stp = sp;
    char* cur_word = (char *)malloc(MAX_ENTRY_SIZE);
    int word_size;
    int cur_id;

    char* cur_buf = (char *)malloc(MAX_ENTRY_COUNT);
    memset(cur_buf,0,MAX_ENTRY_COUNT);
    char* buf_base = cur_buf;

    int *input_word =  (int *)malloc(MAX_ENTRY_COUNT);
    int id_no = 0,last_id = -1;
    int word_count = 0,word_no =0;
    int parse_word_size = 0;
    struct hash_entry* tmp = NULL;
    struct hash_entry_rev* tmp_rev = NULL;
    // construct dics;
    dp+=1; // reserve 1 byte for record size
    while(stp<srcend){
        parse_word_size =getWord(stp,cur_word);
        stp += parse_word_size;
        tmp = hash_find(cur_word);
        if(tmp==NULL){
            hash_insert(cur_word,id_no);
            hash_insert_rev(cur_word,id_no," ");
            buf_put_dict_entry(dp,parse_word_size,cur_word,id_no);
            input_word[word_no] = id_no;
            id_no++;            
        }else{
            input_word[word_no] = tmp->id;
        }
        word_no++;
    }
    stp = dstart;
    buf_put_int16(stp,id_no); // fill basic entry count to dest
    word_count = word_no;

    cur_id = input_word[0];
    tmp_rev = hash_find_rev(cur_id);
    strcpy(cur_word,tmp_rev->key);
    word_size = strlen(cur_word);
    memcpy(cur_buf,cur_word,word_size);
    move_ptr(cur_buf,word_size);
    last_id = cur_id;
    for(word_no = 1;word_no < word_count;word_no++){
        cur_id = input_word[word_no];
        tmp_rev = hash_find_rev(cur_id);
        
        strcpy(cur_word,tmp_rev->key);
        word_size = strlen(cur_word);
        
        strncat(cur_buf,cur_word,word_size);
        move_ptr(cur_buf,word_size);

        tmp = hash_find(buf_base);
        if(tmp!=NULL){
            // is still in dict;
            last_id = tmp->id;
        }else{
            hash_insert(buf_base,id_no);
            hash_insert_rev(buf_base,id_no," ");
            id_no++;            
            
            buf_put_int16(dp,last_id);

            memcpy(buf_base,cur_word,word_size);
            cur_buf = buf_base + word_size;
            *cur_buf = '\0';
        
            tmp = hash_find(buf_base);
            last_id =  tmp->id;
        }
    }
    
    // put last word entry in buf
    buf_put_int16(dp,last_id);
    *dp = '\0';
    
    dict = NULL;
    dict_rev = NULL;

    return (int)(dp - dstart);
}

/**
 * lzw_compress -
 * 
 *            Compress Source into dest. 
 *            Return the number of bytes written in buffer dest, 
 *            or -1 if compression fails.
*/
int32 lzw_compress(const char *source, int32 slen, char *dest,
                    const LZW_Strategy *strategy){
    int32 need_rate;
    int32 result_max;
    int32 result_size;

	char * sp = source; //uncompressed data
    char * srcend = source + slen;//end of uncompressed data

	char *dp = (unsigned char *) dest;           //compressed data


    buf_put_int(dp, slen | 0xc0000000);     // record rawsize

    /**
     * Our fallback strategy is default.
    */
    if (strategy == NULL)
        strategy = LZW_strategy_default;

    if(slen < strategy->min_input_size || 
        slen > strategy->max_input_size)
    {
        return -1;
    }
        

    need_rate = strategy->min_comp_rate;
    if(need_rate < 0 ) need_rate = 0;
    else if(need_rate > 99) need_rate = 99;

	/*
	 * Compute the maximum result size allowed by the strategy, namely the
	 * input size minus the minimum wanted compression rate.  This had better
	 * be <= slen, else we might overrun the provided output buffer.
	 */
	if (slen > (INT_MAX / 100))
	{
		/* Approximate to avoid overflow */
		result_max = (slen / 100) * (100 - need_rate);
	}
	else
		result_max = (slen * (100 - need_rate)) / 100;

    result_size = lzw_compress_ctrl(sp,srcend,dp);

    if(result_size >= result_max) return -1;
    return result_size + 4; // 1 for header 
}

/**
 * lzw_decompress -
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
lzw_decompress(const char *source, int32 slen, char *dest,
				int32 rawsize, bool check_complete){
    printf("---------------- using decompression !\n");
	const unsigned char *sp;
	const unsigned char *srcend;
	unsigned char *dp;
	unsigned char *destend;

    sp = (const unsigned char *) source;
	srcend = sp + slen;
	dp = (unsigned char *) dest;
	destend = dp + rawsize;


	int32 rawsize_read = buf_get_int(sp) & 0x3fffffff;

    if(rawsize != rawsize_read){
		// ereport(ERROR,(errmsg("rawsize = %d while record rowsize = %d.",rawsize,rawsize_read)));
		pg_printf("rawsize = %d while record rowsize = %d.\n",rawsize,rawsize_read);
	}

    int32 entry_count = buf_get_int16(sp);
    int32 cur_id;
    int32 word_size = 0;
    char *pw = (char *)palloc(MAX_ENTRY_SIZE);
    char *cw = (char *)palloc(MAX_ENTRY_SIZE);
    char c;
    struct hash_entry_rev* tmp = NULL;
    struct hash_entry_rev* prev = NULL;



    for(int32 i = 0 ; i < entry_count; i ++){
        buf_get_dict_entry(sp);
    }

    cur_id = buf_get_int16(sp);
    tmp = hash_find_rev(cur_id);
    prev = tmp;
    strcpy(cw,tmp->key);
    strcpy(pw,cw);
    word_size = strlen(cw);
    strncpy(dp,tmp->key,word_size);
    dp += word_size;
    
    while(sp < srcend){
        int32 cur_id = buf_get_int16(sp);
        tmp = hash_find_rev(cur_id);
        if(tmp){
            strcpy(cw,tmp->key);
            word_size = strlen(cw);
            strncpy(dp,cw,word_size);
            dp += word_size;
            strcat(pw,tmp->first);             
            hash_insert_rev(pw,entry_count++,prev->first);
            prev = tmp;
        }else{
            strcat(pw,prev->first); 
            hash_insert_rev(pw,entry_count++,prev->first);
            strcpy(cw,pw); 
            word_size = strlen(cw);          
            strncpy(dp,cw,word_size);
            dp += word_size;
        }
        strcpy(pw,cw);
    }

    *dp = '\0';

    dict = NULL;
    dict_rev = NULL;

	/*
	 * If requested, check we decompressed the right amount.
	 */
	if (check_complete && (dp != destend || sp != srcend))
		return -1;
	/*
	 * That's it.
	 */
	return (char *) dp - (char *)dest;

}