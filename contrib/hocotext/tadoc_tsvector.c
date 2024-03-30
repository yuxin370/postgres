#include "hocotext.h"
#include "postgres.h"
#include "tsearch/ts_utils.h"

#define DEBUG 1

#ifdef DEBUG
    #define debug(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)
#else
    #define debug(...) ((void)0)
#endif

struct ParsedRule;

/**
 * global variables:
 * 
*/
static uint32 num_rules;
static ParsedRule *parsed_rules;

void 

/**
 * @brief convert text to tsvector
 * tadoc_comp_data: data compressed by tadoc compression algorithm
 * ts_vector: this variable will be filled in
 * 		    all words in the compressed data will be seperated
 * 			these words will also be sorted
*/
bool __tadoc_to_tsvector(char* tadoc_comp_data, TSVector* ts_vector) {
	char* read_pos = tadoc_comp_data;
	uint32 raw_size  = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32); // to skip `rawsize`
	num_rules = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32); // to skip `rawsize`
	debug("tadoc decompress: num_rules is %d\n", num_rules);
	// initialize decompress rules array
    parsed_rules = (ParsedRule*)palloc(sizeof(ParsedRule) * num_rules);

	ParsedText parsed_text;
	TSVector tsvector_out;

	// parsed_text.lenwords is the length of the word array
	// here we first estimate word's number to alloc memory
	parsed_text.lenwords = raw_size / 6;	
	if (parsed_text.lenwords < 2)
		parsed_text.lenwords = 2;
	else if (parsed_text.lenwords > MaxAllocSize / sizeof(ParsedWord))
		parsed_text.lenwords = MaxAllocSize / sizeof(ParsedWord);
	// parsed_text.words is parsed word array
	parsed_text.words = (ParsedWord*)palloc(sizeof(ParsedWord) * parsed_text.lenwords);
	// parsed_text.pos is the position of current processing word
	parsed_text.pos = 0;
	// parsed_text.cur_words is the number of words porcessed
	parsed_text.curwords = 0;
	// parse the start rule

}

