/**
 * 
 * contrib/hocotext/compress_lzw.c
 * 
 *    an implementation of LZW for PostgreSQL
 * 
*/

#include <limits.h>
#include <string.h>
#include "uthash.h"
#include<stdio.h>

/* ----------
 * Local definitions
 * ----------
 */
#define MAX_WORDS_COUNT 99999
#define MAX_ENTRY_COUNT 9999
#define MAX_ENTRY_SIZE 999

#define move_ptr(cur_buf,word_size)     \
    cur_buf += word_size;               \
    *cur_buf = '\0'

// big endian
#define buf_put_int(__bp,__v)                                                   \
do {                                                                            \
    for(int seg = 3 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int(__bp)                                           \
({                                                                  \
    int value = 0;                                                  \
    for(int seg = 3 ; seg >= 0 ; seg --,__bp++){                    \
        value |=  (int)((*__bp)&0xFF) << (8*seg);                   \
    }                                                               \
    value;                                                          \
})


// big endian
#define buf_put_int8(__bp,__v)                                                  \
do {                                                                            \
    for(int seg = 0 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int8(__bp)                                              \
({                                                                      \
    int value = 0;                                                      \
    for(int seg = 0 ; seg >= 0 ; seg --,__bp++){                        \
        value |=  (int)((*__bp)&0xFF) << (8*seg);                       \
    }                                                                   \
    value;                                                              \
})


// big endian
#define buf_put_int16(__bp,__v)                                                 \
do {                                                                            \
    for(int seg = 1 ; seg >= 0 ; seg --,__bp++){                                \
        (*__bp) = (char)( ( ((unsigned)__v) >> ( 8 * seg ) ) & 0xFF );          \
    }                                                                           \
}while(0)

// big endian
#define buf_get_int16(__bp)                                                 \
({                                                                          \
    int value = 0;                                                          \
    for(int seg = 1 ; seg >= 0 ; seg --,__bp++){                            \
        value |=  (int)((*__bp)&0xFF) << (8*seg);                           \
    }                                                                       \
    value;                                                                  \
})

#define buf_put_dict_entry(__bp,__len,__key,__id)                       \
do{                                                                     \
    buf_put_int8(__bp,__len);                                           \
    memcpy(__bp,__key,__len);                                           \
    __bp+=__len;                                                        \
    buf_put_int8(__bp,__id);                                            \
}while(0)

#define buf_get_dict_entry(__bp)                                        \
do{                                                                     \
    int len = buf_get_int8(__bp);                                       \
    char tmp[len+1];                                                    \
    memcpy(tmp,__bp,len);                                               \
    tmp[len] = '\0';                                                    \
    __bp+=len;                                                          \
    int id = buf_get_int8(__bp);                                        \
    hash_insert_rev(tmp,id,tmp);                                                \
}while(0)

typedef struct hash_entry {
    char *key;                 /* key */
    int id;
    UT_hash_handle hh1;         /* makes this structure hashable, hh1 for key*/
}hash_entry;


typedef struct hash_entry_rev {
    int id;                     /* key */
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

int getWord(char *buf,char *res) {
    char *ptr = res;
    char c;                        // current char
    static int indicator = 0; // flags
    static char old;               // last char
    char* myStr = (char *)malloc(5);
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
 * @brief parse and print char-array as int-array 
*/
void print_int(char * dest,char *end){
    while(dest < end){
        printf("%d ",buf_get_int(dest));
    }
    printf("\n");
}

void print_int8(char * dest,char *end){
    while(dest < end){
        printf("%d ",buf_get_int8(dest));
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

struct hash_entry_rev* hash_find_rev(int id) {
    struct hash_entry_rev* tmp;
    HASH_FIND(hh2, dict_rev, &id, sizeof(int), tmp);
    return tmp;
}


/***
 * @brief insert an entry to dic.
 * insert a new entry if not in dict yet, or just modify the value.
*/
void hash_insert(char* ikey, int id) {
    struct hash_entry* it;

    it = hash_find(ikey);
    if (it == NULL) {
        struct hash_entry* tmp = (struct hash_entry *)malloc(sizeof *tmp);
        tmp->key = (char *)malloc(strlen(ikey));
        strcpy(tmp->key,ikey);
        tmp->id = id;
        HASH_ADD_KEYPTR(hh1, dict, tmp->key, strlen(tmp->key), tmp);
    } else {
        it->id = id;
    }
}


void hash_insert_rev(char* ikey, int id, char * ifirst) {
    struct hash_entry_rev* it_rev;

    it_rev =  hash_find_rev(id);
    if (it_rev == NULL) {
        struct hash_entry_rev* tmp = (struct hash_entry_rev *)malloc(sizeof *tmp);
        tmp->key = (char *)malloc(strlen(ikey));
        strcpy(tmp->key,ikey);
        tmp->id = id;
        tmp->first = (char *)malloc(strlen(ifirst));
        strcpy(tmp->first,ifirst);
        HASH_ADD_KEYPTR(hh2, dict_rev, &(tmp->id), sizeof(int), tmp);
    } else {
        strcpy(it_rev->key,ikey);
        strcpy(it_rev->first,ifirst);
    }
}

/***
 * @brief delete an entry with ikey from hash_table;
*/
int hash_delete(char* ikey) {
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

int rev_hash_delete(int id) {
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
void hash_print(int type){
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
int lzw_compress_ctrl(char *sp,char *srcend,char *dp){
	char *dstart = dp;                         //start of compressed data
    char *stp = sp;
    char* cur_word = (char *)malloc(MAX_ENTRY_SIZE);
    int word_size;

    char* cur_buf = (char *)malloc(MAX_ENTRY_COUNT);
    memset(cur_buf,0,MAX_ENTRY_COUNT);
    char* buf_base = cur_buf;

    char **input_word =  (char **)malloc(MAX_ENTRY_COUNT);
    for(int i =  0 ; i < MAX_ENTRY_COUNT;i++){
        input_word[i] = (char *)malloc(MAX_ENTRY_SIZE);
    }
    int id_no = 0,last_id = -1;
    int word_count = 0,word_no =0;
    int parse_word_size = 0;
    struct hash_entry* tmp = NULL;
    // construct dics;
    dp+=1; // reserve 1 byte for record size

    while(stp<srcend){
        parse_word_size =getWord(stp,cur_word);
        stp += parse_word_size;
        tmp = hash_find(cur_word);
        
        if(tmp==NULL){
            hash_insert(cur_word,id_no);
            buf_put_dict_entry(dp,parse_word_size,cur_word,id_no);
            id_no++;            
        }
        strcpy(input_word[word_no],cur_word);
        word_no++;
    }
    stp = dstart;
    buf_put_int8(stp,id_no); // fill basic entry count to dest
    word_count = word_no;

    strcpy(cur_word,input_word[0]);
    word_size = strlen(input_word[0]);
    memcpy(cur_buf,cur_word,word_size);
    move_ptr(cur_buf,word_size);
    tmp = hash_find(buf_base);
    if(tmp == NULL){
        return -1;
    }
    last_id = tmp->id;
    for(word_no = 1;word_no < word_count;word_no++){
        strcpy(cur_word,input_word[word_no]);
        word_size = strlen(input_word[word_no]);
        strncat(cur_buf,cur_word,word_size);
        move_ptr(cur_buf,word_size);

        tmp = hash_find(buf_base);
        if(tmp!=NULL){
            // is still in dict;
            last_id = tmp->id;
        }else{
            hash_insert(buf_base,id_no);
            id_no++;            
            
            buf_put_int8(dp,last_id);

            memcpy(buf_base,cur_word,word_size);
            cur_buf = buf_base + word_size;
            *cur_buf = '\0';
        
            tmp = hash_find(buf_base);
            last_id =  tmp->id;
        }
    }
    
    // put last word entry in buf
    buf_put_int8(dp,last_id);
    *dp = '\0';

    return (int)(dp - dstart);
}


/**
 * lzw_decompress -
 * 
 * 
*/
int
lzw_decompress(char* sp, int slen, char* dest, int rawsize){
    char * srcend = sp + slen;
    // rawsize = buf_get_int(sp) & 0x3fffffff;
	char *dp = dest;
	char *destend = dp + rawsize;
    int entry_count = buf_get_int8(sp);
    int cur_id;
    int word_size = 0;
    char *pw = (char *)malloc(MAX_ENTRY_SIZE);
    char *cw = (char *)malloc(MAX_ENTRY_SIZE);
    char c;
    struct hash_entry_rev* tmp = NULL;
    struct hash_entry_rev* prev = NULL;
    for(int i = 0 ; i < entry_count; i ++){
        buf_get_dict_entry(sp);
    }
    print_int8(sp,srcend);

    cur_id = buf_get_int8(sp);
    tmp = hash_find_rev(cur_id);
    prev = tmp;
    strcpy(cw,tmp->key);
    strcpy(pw,cw);
    word_size = strlen(cw);
    strncpy(dp,tmp->key,word_size);
    dp += word_size;
    
    while(sp < srcend){
        int cur_id = buf_get_int8(sp);
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

    hash_print(2);
	return dp-dest;
}

int main(int argc, char *argv[])
{
    //char text[] = "TOTOB";
    char text[] = "hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world hello world";
    char *dest = (char *)malloc(MAX_WORDS_COUNT);
    char *decomp_dest = (char *)malloc(MAX_WORDS_COUNT);
    memset(dest,0,MAX_WORDS_COUNT);
    memset(decomp_dest,0,MAX_WORDS_COUNT);
    char* srcend = text + strlen(text);

    int reslen = lzw_compress_ctrl(text,srcend,dest);
    printf(" === compressing completed ==== \ncompressed len = %d, raw text len = %ld\n",reslen,strlen(text));
    hash_clear();
    hash_clear_rev();
    int rawlen = lzw_decompress(dest,reslen,decomp_dest,strlen(text));
    printf(" === decompressing completed ==== \n compressed len = %d, raw text len = %ld\n decompressed data : %s\n",reslen,rawlen,decomp_dest);
    hash_clear();
    return 0;
}