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
static char* rule_data_end;

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
static char* get_word(char* source, uint16_t* word_len, char* parse_end) {
	char* word_end = source;
	debug("%s: ", "get word");
	while (is_letter(*word_end)) {
		debug("%c", *word_end);
		word_end++;
	}
	*word_len = word_end - source;
	debug(" length: %d\n", *word_len);
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

static void copy_word_arr(ParsedWord* source, ParsedWord* dest, uint32_t len, uint32_t start_text_pos) {
	memcpy(dest, source, sizeof(ParsedWord) * len);
	for (int i = 0; i < len; ++i) {
		dest[i].pos.pos = start_text_pos + i;
	}
}

/**
 * @brief parse_rule parse a rule to seperate the word in the rule
 * rule_index: rule id
 * text_pos: the global character base of the rule
 * rule_data: the compressed data of the rule
 * rule_len: the length of rule_data
 * dest: write position of parsed word in this rule
*/
static void parse_rule(rule_index_t rule_index, uint32_t start_text_pos, char* rule_data, uint32_t rule_len, ParsedWord* dest) {
	debug("begin to parse rule %d\n", rule_index);
	parsed_rules[rule_index].parse_start = dest;
	parsed_rules[rule_index].num_words = 0;
	
	char* cur_parse_pos = rule_data;
	uint32_t cur_text_pos = start_text_pos;	// position of each word
	ParsedWord* parsed_wpos = dest;
	char* parse_end = rule_data + rule_len;
	
	// clear the bootless characters in front of a rule
	while (!is_letter(*cur_parse_pos) && cur_parse_pos != parse_end) {
		cur_parse_pos++;
		cur_text_pos++;
	}
	// each time parse a single word
	// cur_parse_pos indicate the character that currently processed
	while (cur_parse_pos != parse_end) {
		// debug("read character: %c\n", *cur_parse_pos);
		// the character belones to a simple word
		if (*cur_parse_pos != RESERVE_CHAR) {
			parsed_wpos->alen = 0; // not a word array, so make alen=0 
			parsed_wpos->nvariant = 0; // no consideration to nvariant
			parsed_wpos->pos.pos = cur_text_pos;
			// TODO: write flag of parsed wpos???
			// this flag is not used in make vector, useless now
			// parsed_wpos->flags = 
			// parse a single word
			parsed_wpos->word = cur_parse_pos;
			char* next_parse_pos = get_word(cur_parse_pos, &parsed_wpos->len, parse_end);
			cur_text_pos += (next_parse_pos - cur_parse_pos);
			cur_parse_pos = next_parse_pos;
			parsed_rules[rule_index].num_words++;
			parsed_wpos++;
		}
		// the character represents a rule-symbol
		else {
			rule_index_t subrule_index = *((rule_index_t*)(cur_parse_pos+1));
			// check whether the subrule has been parsed
			if (parsed_rules[subrule_index].is_parsed) {
				copy_word_arr(
					parsed_rules[subrule_index].parse_start, 
					parsed_wpos,
					parsed_rules[subrule_index].num_words,
					cur_text_pos
				);
			}
			else {
				rule_offset_t rule_begin_offset = subrule_index==0 ? 0 : rule_end_offset[subrule_index-1];
				char* subrule_data = rule_data_start + rule_begin_offset;
				uint32_t subrule_len = rule_end_offset[subrule_index] - rule_begin_offset;
				parse_rule(subrule_index, cur_text_pos, subrule_data, subrule_len, parsed_wpos);
				parsed_rules[subrule_index].is_parsed = 1;
			}
			parsed_wpos += parsed_rules[subrule_index].num_words;
			cur_text_pos += parsed_rules[subrule_index].real_length;
			parsed_rules[rule_index].num_words += parsed_rules[subrule_index].num_words;
			cur_parse_pos += (1 + sizeof(rule_index_t)); // to skip rule symbol
		}
	}
	parsed_rules[rule_index].real_length = cur_text_pos - start_text_pos;
	return;
}

/**
 * @brief convert text to tsvector
 * tadoc_comp_data: data compressed by tadoc compression algorithm
 * ts_vector: this variable will be filled in
 * 		    all words in the compressed data will be seperated
 * 			these words will also be sorted
*/
TSVector tadoc_to_tsvector(char* tadoc_comp_data) {
	char* read_pos = tadoc_comp_data;
	uint32 raw_size  = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32); // to skip `rawsize`
	num_rules = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32); // to skip `rawsize`
	debug("tadoc_to_tsvector: num_rules is %d\n", num_rules);
	// initialize decompress rules array
    parsed_rules = (ParsedRule*)palloc(sizeof(ParsedRule) * num_rules);

	for (int i = 0; i < num_rules; ++i) {
		parsed_rules[i].is_parsed = 0;
	}

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
	
	// parse the start rule
	// initialize work
	rule_end_offset = (rule_offset_t*)read_pos;
	rule_data_start = read_pos + sizeof(rule_offset_t) * num_rules;
	parse_rule(0, 0, rule_data_start, rule_end_offset[0], parsed_text.words);
	debug("%s", "end parse rules\n");
	// parsed_text.cur_words is the number of words porcessed
	parsed_text.curwords = parsed_rules[0].num_words;
	// parsed_text.pos is the position of current processing word
	parsed_text.pos = parsed_rules[0].real_length;
	tsvector_out = make_tsvector(&parsed_text);
	debug("%s", "end make_tsvector\n");
	return tsvector_out;
}