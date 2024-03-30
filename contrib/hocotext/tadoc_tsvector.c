#include "hocotext.h"
#include "postgres.h"
#include "tsearch/ts_utils.h"

#define DEBUG 1

#ifdef DEBUG
    #define debug(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)
#else
    #define debug(...) ((void)0)
#endif

typedef bool uint8;

/**
 * global variables:
 * 
*/
static uint32 num_rules;
static DecompRule* decomp_rules;


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
    decomp_rules = (DecompRule*)palloc(sizeof(DecompRule) * num_rules);

	ParsedText	prs;
	TSVector out;

	// to estimate word's number
	




}

