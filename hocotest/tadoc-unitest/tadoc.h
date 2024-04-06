#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
	uint16_t		flags;			/* currently, only TSL_PREFIX */
	uint16_t		len;
	uint16_t		nvariant;
	uint16_t		alen;
	union
	{
		uint32_t		pos;

		/*
		 * When apos array is used, apos[0] is the number of elements in the
		 * array (excluding apos[0]), and alen is the allocated size of the
		 * array.  We do not allow more than MAXNUMPOS array elements.
		 */
		uint32_t	   *apos;
	}			pos;
	char	   *word;
} ParsedWord;

typedef struct
{
	ParsedWord *words;
	int32_t		lenwords;
	int32_t		curwords;
	int32_t		pos;
} ParsedText;

/**
 * tadoc_(de)compress()
 * internal compression module
*/

/**
 * tadoc to_tsvector
*/
struct ParsedRule {
	// whether parsed or not
	uint32_t is_parsed;
	// length of the word array of this rule
	uint32_t num_words;
	// the real length of this rule
	uint32_t real_length;
	// start position in the array
	ParsedWord* parse_start;
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