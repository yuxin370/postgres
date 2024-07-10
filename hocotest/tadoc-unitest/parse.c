#include "tadoc.h"

// #define DEBUG 1

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
static uint32_t num_rules;
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
	// debug("%s: ", "get word");
	while (is_letter(*word_end)) {
		// debug("%c", *word_end);
		word_end++;
	}
	*word_len = word_end - source;
	// debug(" length: %u\n", *word_len);
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
	dest[0].len = source[0].len;
	dest[0].word = source[0].word;
	dest[0].pos.pos = start_text_pos;
	uint32_t cur_text_pos = start_text_pos;
	// memcpy(dest, source, sizeof(ParsedWord) * len);
	for (int i = 1; i < len; ++i) {
		dest[i].len = source[i].len;
		dest[i].word = source[i].word;
		dest[i].pos.pos = cur_text_pos;
		cur_text_pos += dest[i-1].len;
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
	debug("begin to parse rule %u\n", rule_index);
	parsed_rules[rule_index].parse_start = dest;
	
	char* cur_parse_pos = rule_data;
	uint32_t cur_text_pos = start_text_pos;	// position of each word
	ParsedWord* parsed_wpos = dest;
	char* parse_end = rule_data + rule_len;
	
	// clear the bootless characters in front of a rule
	while (!is_letter(*cur_parse_pos) && // not letter
			*cur_parse_pos != RESERVE_CHAR && // not reserve character
			cur_parse_pos != parse_end) {	// not parse end
		cur_parse_pos++;
		cur_text_pos++;
	}
	// each time parse a single word
	// cur_parse_pos indicate the character that currently processed
	while (cur_parse_pos != parse_end) {
		// debug("read character: %c\n", *cur_parse_pos);
		// the character belones to a simple word
		if (is_letter(*cur_parse_pos)) {
			parsed_wpos->pos.pos = cur_text_pos;
			// TODO: write flag of parsed wpos???
			// this flag is not used in make vector, useless now
			// parsed_wpos->flags = 
			// parse a single word
			parsed_wpos->word = cur_parse_pos;
			char* next_parse_pos;
			next_parse_pos = get_word(cur_parse_pos, &parsed_wpos->len, parse_end);
			cur_text_pos += (next_parse_pos - cur_parse_pos);
			cur_parse_pos = next_parse_pos;
			parsed_rules[rule_index].num_words++;
			parsed_wpos++;
		}
		// a non-letter character follows a rule
		else if (*cur_parse_pos != RESERVE_CHAR) {
			cur_parse_pos++;
			cur_text_pos++;
		}
		// the character represents a rule-symbol
		else {
			rule_index_t subrule_index = *((rule_index_t*)(cur_parse_pos+1));
			// check whether the subrule has been parsed
			if (parsed_rules[subrule_index].is_parsed) {
				debug("when parsing rule %u, find parsed rule %u with num_word = %u\n", rule_index, subrule_index, parsed_rules[subrule_index].num_words);
				copy_word_arr(
					parsed_rules[subrule_index].parse_start, 
					parsed_wpos,
					parsed_rules[subrule_index].num_words,
					cur_text_pos
				);
			}
			else {
				debug("when parsing rule %u, find unparsed rule %u\n", rule_index, subrule_index);
				rule_offset_t rule_begin_offset = rule_end_offset[subrule_index-1];
				char* subrule_data = rule_data_start + rule_begin_offset;
				uint32_t subrule_len = rule_end_offset[subrule_index] - rule_begin_offset;
				parse_rule(subrule_index, cur_text_pos, subrule_data, subrule_len, parsed_wpos);
			}
			parsed_wpos += parsed_rules[subrule_index].num_words;
			cur_text_pos += parsed_rules[subrule_index].real_length;
			parsed_rules[rule_index].num_words += parsed_rules[subrule_index].num_words;
			cur_parse_pos += (1 + sizeof(rule_index_t)); // to skip rule symbol
		}
	}
	parsed_rules[rule_index].real_length = cur_text_pos - start_text_pos;
	parsed_rules[rule_index].is_parsed = 1;
	debug("end parse rule %u with num_word = %u\n", rule_index, parsed_rules[rule_index].num_words);
	return;
}

void print_word(ParsedWord pw) {
	// printf("%u: ", pw.pos.pos);
	for (int i = 0; i < pw.len; ++i) {
		printf("%c", pw.word[i]);
	}
	printf("\n");
}


/*
 * Compare two strings by tsvector rules.
 *
 * if prefix = true then it returns zero value iff b has prefix a
 */
static int32_t compare_string(char *a, int lena, char *b, int lenb, int prefix)
{
	int			cmp;

	if (lena == 0)
	{
		if (prefix)
			cmp = 0;			/* empty string is prefix of anything */
		else
			cmp = (lenb > 0) ? -1 : 0;
	}
	else if (lenb == 0)
	{
		cmp = (lena > 0) ? 1 : 0;
	}
	else
	{
		cmp = memcmp(a, b, Min((unsigned int) lena, (unsigned int) lenb));

		if (prefix)
		{
			if (cmp == 0 && lena > lenb)
				cmp = 1;		/* a is longer, so not a prefix of b */
		}
		else if (cmp == 0 && lena != lenb)
		{
			cmp = (lena < lenb) ? -1 : 1;
		}
	}

	return cmp;
}

static int compare_word(const void *a, const void *b)
{
	int			res;

	res = compare_string(((const ParsedWord *) a)->word, ((const ParsedWord *) a)->len,
						  ((const ParsedWord *) b)->word, ((const ParsedWord *) b)->len,
						  0);

	if (res == 0)
	{
		if (((const ParsedWord *) a)->pos.pos == ((const ParsedWord *) b)->pos.pos)
			return 0;

		res = (((const ParsedWord *) a)->pos.pos > ((const ParsedWord *) b)->pos.pos) ? 1 : -1;
	}

	return res;
}

#define MIN_POS_ARRAY_LEN 1000

static HocotextTsvector* hocotext_make_tsvector(ParsedText *parsed_text) {
	uint32_t total_words = parsed_text->curwords;
	ParsedWord* words = parsed_text->words;
	qsort(words, total_words, sizeof(ParsedWord), compare_word);

	// for (int i = 0; i < total_words; ++i) {
	// 	for (int j = 0; j < words[i].len; ++j) {
	// 		printf("%c", words[i].word[j]);
	// 	}
	// 	printf("\n");
	// }

	HocotextTsvector* tsvector = (HocotextTsvector*)malloc(sizeof(HocotextTsvector));
	tsvector->word_entry = words;
	
	tsvector->word_entry[0].pos_arr_len_esti = MIN_POS_ARRAY_LEN;
	uint32_t first_word_pos = words[0].pos.pos;
	tsvector->word_entry[0].pos.pos_arr = (uint32_t*)malloc(sizeof(uint32_t) * tsvector->word_entry[0].pos_arr_len_esti);
	tsvector->word_entry[0].pos.pos_arr[0] = first_word_pos;
	tsvector->word_entry[0].pos_arr_len = 1;
	tsvector->word_entry[0].len = words[0].len;
	tsvector->num_diffwords = 1;

	for (int i = 1; i < total_words; ++i) {
		ParsedWord* cur_word = &(tsvector->word_entry[tsvector->num_diffwords - 1]);
		// printf("curword (len: %d) is ", cur_word->len);
		// print_word(*cur_word);
		// printf("word %d (len: %d) is ", i, words[i].len);
		// print_word(words[i]);
		if (words[i].len == cur_word->len && 
			strncmp(words[i].word, cur_word->word, words[i].len) == 0) {
			// printf("same!\n");
			cur_word->pos.pos_arr[cur_word->pos_arr_len] = words[i].pos.pos;
			cur_word->pos_arr_len += 1;
			if (cur_word->pos_arr_len > cur_word->pos_arr_len_esti / 2) {
				cur_word->pos_arr_len_esti *= 2;
				cur_word->pos.pos_arr = (uint32_t*)realloc(
					cur_word->pos.pos_arr,
					cur_word->pos_arr_len_esti * sizeof(uint32_t)
				);
			}
		}
		else {
			// printf("create a new!\n");
			tsvector->num_diffwords += 1;
			cur_word = &(tsvector->word_entry[tsvector->num_diffwords - 1]);
			cur_word->len = words[i].len;
			cur_word->pos_arr_len_esti = MIN_POS_ARRAY_LEN;
			cur_word->pos.pos_arr = (uint32_t*)malloc(sizeof(uint32_t) * tsvector->word_entry[tsvector->num_diffwords-1].pos_arr_len_esti);
			cur_word->pos.pos_arr[0] = words[i].pos.pos;
			cur_word->pos_arr_len = 1;
			cur_word->word = words[i].word;
		}
	}

	printf("number of total words: %d, number of unique words: %d\n", total_words, tsvector->num_diffwords);
	return tsvector;
}

#define MIN_ALLOC_LEN 19790326

static char* tsvector2text(HocotextTsvector* tsvector, uint32_t* ret_len) {
	uint32_t alloc_len = MIN_ALLOC_LEN;
	char* buffer = (char*)malloc(alloc_len * sizeof(char));
	uint32_t write_bytes = 0;
	for (int i = 0; i < tsvector->num_diffwords; ++i) {
		write_bytes += sprintf(buffer+write_bytes, "'%.*s': ", tsvector->word_entry[i].len, tsvector->word_entry[i].word);
		for (int j = 0; j < tsvector->word_entry[i].pos_arr_len - 1; ++j) {
			write_bytes += sprintf(buffer + write_bytes, "%u,", tsvector->word_entry[i].pos.pos_arr[j]);
		}
		write_bytes += sprintf(buffer + write_bytes, "%u", tsvector->word_entry[i].pos.pos_arr[tsvector->word_entry[i].pos_arr_len - 1]);
		write_bytes += sprintf(buffer + write_bytes, "%c", '\n');
		if (write_bytes > alloc_len / 2) {
			alloc_len *= 2;
			buffer = (char*)realloc(buffer, alloc_len * sizeof(char));
		}
	}
	*ret_len = write_bytes;
	return buffer;
}

/**
 * @brief convert text to tsvector
 * tadoc_comp_data: data compressed by tadoc compression algorithm
 * ts_vector: this variable will be filled in
 * 		    all words in the compressed data will be seperated
 * 			these words will also be sorted
*/
void tadoc_to_tsvector(char* tadoc_comp_data) {
	char* read_pos = tadoc_comp_data;
	uint32_t raw_size  = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32_t); // to skip `rawsize`
	num_rules = *((uint32_t*)read_pos);
	read_pos += sizeof(uint32_t); // to skip `rawsize`
	debug("tadoc_to_tsvector: num_rules is %u\n", num_rules);
	// initialize decompress rules array
    parsed_rules = (ParsedRule*)malloc(sizeof(ParsedRule) * num_rules);

	for (int i = 0; i < num_rules; ++i) {
		parsed_rules[i].is_parsed = 0;
		parsed_rules[i].num_words = 0;
		parsed_rules[i].real_length = 0;
	}

	ParsedText parsed_text;
	// TSVector tsvector_out;

	// parsed_text.lenwords is the length of the word array
	// here we first estimate word's number to alloc memory
	parsed_text.lenwords = raw_size / 6;	
	if (parsed_text.lenwords < 2)
		parsed_text.lenwords = 2;
	else if (parsed_text.lenwords > MaxAllocSize / sizeof(ParsedWord))
		parsed_text.lenwords = MaxAllocSize / sizeof(ParsedWord);
	// parsed_text.words is parsed word array
	parsed_text.words = (ParsedWord*)malloc(sizeof(ParsedWord) * parsed_text.lenwords);
	
	// parse the start rule
	// initialize work
	rule_end_offset = (rule_offset_t*)read_pos;
	rule_data_start = read_pos + sizeof(rule_offset_t) * num_rules;
	parse_rule(0, 0, rule_data_start, rule_end_offset[0], parsed_text.words);
	debug("%s", "end parse rules\n");
	// parsed_text.cur_words is the number of words porcessed
	parsed_text.curwords = parsed_rules[0].num_words;
	// parsed_text.pos is the position of current processing word
	
	// for (int i = 0; i < parsed_text.curwords; ++i) {
	// 	print_word(parsed_text.words[i]);
	// }

	debug("%s", "end make_tsvector\n");

	HocotextTsvector* tsvector = hocotext_make_tsvector(&parsed_text);
	uint32_t ret_size;
	char* result = tsvector2text(tsvector, &ret_size);
	printf("%s", result);
}