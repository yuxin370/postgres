#include "hocotext.h"
#include "postgres.h"

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
static rule_offset_t* rule_end_offset;
static char* rule_data_start;

/**
 * @brief check whether a character is a letter or not
*/
static inline uint32_t is_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/**
 * @brief: to get a single word
 * parse_ptr points to the parse position
 * word_len is the length of a word
*/
char* get_word(char* source, uint16_t* word_len, char* parse_end) {
	char* word_end = source;
	while (is_letter(*source)) {
		word_end++;
	}
	*word_len = word_end - source;
	char* cur_ptr = word_end;
	while (1) {
		// if read a reserve char or a new letter or the end of string, stop
		if (*cur_ptr == RESERVE_CHAR || is_letter(*cur_ptr) || cur_ptr == parse_end) {
			return cur_ptr;
		}
		else {
			// move cur_ptr to a new position
			cur_ptr++;
		}
	}
}

void copy_word_arr(ParsedWord* source, ParsedWord* dest, uint32_t len, uint32_t distance) {
	memcpy(dest, source, sizeof(ParsedWord) * len);
	for (int i = 0; i < len; ++i) {
		dest[i].pos.pos = source[i].pos.pos + distance;
	}
}

/**
 * @brief parse_rule
*/
void parse_rule(rule_index_t rule_index, uint32_t text_pos_base, char* txtrule_len, uint32_t txt_rule_len, ParsedWord* dest) {
	parsed_rules[rule_index].parse_start = dest;
	parsed_rules[rule_index].text_pos_base = text_pos_base;
	char* cur_parse_pos = txtrule_len;
	ParsedWord* parsed_wpos = dest;
	char* parse_end = txtrule_len + txt_rule_len;
	uint32_t local_text_pos = 0;	// position of each word
	// each time parse a single word
	// cur_parse_pos indicate the character that currently processed
	while (cur_parse_pos != parse_end) {
		// the character belones to a simple word
		if (*cur_parse_pos != RESERVE_CHAR) {
			parsed_wpos->alen = 0; // not a word array, so make alen=0 
			parsed_wpos->nvariant = 0; // no consideration to nvariant
			parsed_wpos->pos.pos = local_text_pos + text_pos_base;
			// TODO: write flag of parsed wpos
			// parsed_wpos->flags = 
			// get a single word
			char* next_parse_pos = get_word(cur_parse_pos, &parsed_wpos->len, parse_end);
			local_text_pos += (next_parse_pos - cur_parse_pos);
		}
		// the character represents a rule-symbol
		else {
			rule_index_t subrule_index = *((rule_index_t*)(cur_parse_pos+1));
			// check whether the subrule has been parsed
			if (parsed_rules[subrule_index].is_parsed) {
				copy_word_arr(
					parsed_rules[subrule_index].parse_start, 
					parsed_rules[subrule_index].num_words,
					parsed_wpos,
					local_text_pos + text_pos_base - parsed_rules[subrule_index].text_pos_base
				);
				local_text_pos += parsed_rules[subrule_index].real_length;
			}
			else {
				rule_offset_t rule_begin_offset = subrule_index==0 ? 0 : rule_end_offset[subrule_index-1];
				char* subtxtrule_len = rule_data_start + rule_begin_offset;
				uint32_t subtxt_rule_len = rule_end_offset[subrule_index] - rule_begin_offset;
				parse_rule(subrule_index, local_text_pos, subtxtrule_len, subtxt_rule_len, cur_parse_pos);
				local_text_pos += parsed_rules[subrule_index].real_length;
			}
		}
	}
	parsed_rules[rule_index].real_length = local_text_pos - text_pos_base;
	return;
}

/**
 * @brief convert text to tsvector
 * tadoc_comp_data: data compressed by tadoc compression algorithm
 * ts_vector: this variable will be filled in
 * 		    all words in the compressed data will be seperated
 * 			these words will also be sorted
*/
uint8 __tadoc_to_tsvector(char* tadoc_comp_data, TSVector* ts_vector) {
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
	// initialize work
	rule_end_offset = (rule_offset_t*)read_pos;
	rule_data_start = read_pos + sizeof(rule_offset_t) * num_rules;
	parse_rule(0, 0, rule_data_start, rule_end_offset[0], parsed_text.words);
	tsvector_out = make_tsvector(&parsed_text);
	PG_RETURN_TSVECTOR(tsvector_out);
}