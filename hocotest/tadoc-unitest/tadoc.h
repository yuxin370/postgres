#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define Min(x,y) ((x) < (y) ? (x) : (y))

/**
 * tadoc_(de)compress()
 * internal compression module
*/


typedef struct ParsedWord {
	uint32_t		len;
	uint32_t		pos_arr_len;
	uint32_t		pos_arr_len_esti;
	union {
		uint32_t pos;
		uint32_t* pos_arr;
	} pos;
	char	   *word;
} ParsedWord;

typedef struct ParsedRule {
	// whether parsed or not
	uint32_t is_parsed;
	// length of the word array of this rule
	uint32_t num_words;
	// the real length of this rule
	uint32_t real_length;
	// start position in the array
	ParsedWord* parse_start;
} ParsedRule;

typedef struct ParsedText {
	ParsedWord *words;
	int32_t		lenwords;
	int32_t		curwords;
} ParsedText;

typedef struct HocotextTsvector {
	uint32_t num_diffwords;
	ParsedWord* word_entry;
} HocotextTsvector;
/**
 * TADOC Decompressed Rule Entry
*/
struct DecompRule {
    // o means the rule have not been finish decompression step yet
    uint8_t finished;  
    char* decomp_start;
    uint32_t decomp_len;
};

#define MaxAllocSize 0x3fffffff

typedef struct ParsedRule ParsedRule;
typedef struct DecompRule DecompRule;
// type to indicate the id of rule
#ifdef MINI_NUM_RULES
	typedef uint16_t rule_index_t;
#else
	typedef uint32_t rule_index_t;
#endif
typedef uint32_t rule_offset_t;
#define RESERVE_CHAR '^'

extern void tadoc_to_tsvector(char* tadoc_comp_dat);