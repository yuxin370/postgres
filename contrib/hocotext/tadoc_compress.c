#include "hocotext.h"

#define DEBUG 1

#ifdef DEBUG
    #define debug(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)
#else
    #define debug(...) ((void)0)
#endif

/**
 * struct declarations
*/
struct Symbol;
struct Rule;
union SymbolData;
typedef struct Symbol Symbol;
typedef struct Rule Rule;
typedef union SymbolData SymbolData;
#define MAX_WORD_LENGTH 64
#define PRIME 9999991 // a magical prime number
#define HER_BIRTHDAY 19790326 // her birthday
// #define UPBOUND 9 // the decimal number to represent the number of rules should not larger than this number 

/**
 * static function declarations
*/
static void gen_rule(Rule**);
static void destroy_rule(Rule*);
static void reuse_rule(Rule*);
static void deuse_rule(Rule*);
static void gen_rulesymbol(Rule*, Symbol**);
static void gen_symbol(char*, uint32_t, Symbol**);
static Symbol* first(Rule* rule);
static Symbol* last(Rule* rule);
static Rule* get_rule(Symbol* rulesymbol);
static void destroy_symbol(Symbol* symbol);
static uint32_t non_terminal(Symbol*);
static uint32_t is_guard(Symbol*);
static void join_symbols(Symbol*, Symbol*);
static uint32_t cmp_symbol(Symbol*, Symbol*);
static void delete_digram(Symbol*);
static void insert_after(Symbol*, Symbol*);
static uint32_t check_digram(Symbol*);
static void expand_rulesymbol(Symbol*);
static void match_digram(Symbol*, Symbol*);
static void replace_digram(Symbol*, Rule*);
static void init_digram_table(int32_t);
static void clear_digram_table();
static Symbol** find_digram(Symbol*);
static void discover(Rule*);
static inline uint32_t write_rulesymbol(Symbol*, char*);
static inline uint32_t write_symbol(Symbol*, char*);
static uint32_t write_compressed_data(char*, uint32_t );
static void decompress_rule(rule_index_t, char*, uint32_t, char*);
static inline uint32_t is_letter(char);
static uint32_t __tadoc_compress(char*, uint32_t, char*);
static uint32_t __tadoc_decompress(char*, char*);

/**
 * The key points in TADOC compression algorithm：
 * 1. rule-symbol is an abstract symbol, not a real character
 * 2. rule-symbol is the instance of a Rule
 * 3. Rule consists of other Rules and Symbols 
*/

// A symbol represents either a word or a rule
union SymbolData
{
    char word [MAX_WORD_LENGTH];
    Rule* rule;
};

struct Symbol {
    Symbol *n, *p;
    // 0 means this is not a Rule-Symbol
    uint8_t is_rulesymbol;
    SymbolData data;
};

struct Rule {
    /** -----------------------------------------------------------------------------------------------------------------
    * Struct of Rule:
    * Rule: Symbol1 -> Symbol2 -> Symbol3 -> ....... ->Symboln
    *        |                                    |           
    *        -----------<< n <<---------- guard -------->> p >>-----
    * Guard is a special non_terminal whose key can be converted to correspond Rule
    */
    /** guard:
     * Guard shoud point forward to the first symbol in the rule and backward to the last symbol in the rule 
     * Its own value points to the rule data structure, so that Symbol can find out which rule they're in
     * ----------------------------------------------------------------------------------------------------------------- */
    Symbol* guard;
    // Reference count: Keeps track of the number of times the rule is used in the grammar
    int count;
    // To number the Rule for printing
    rule_index_t index;
};

static Symbol** digram_table;
static Rule** rule_array;
static uint32_t num_rules;
static uint32_t num_detected_rules;
static rule_offset_t* rule_end_offset;
static char* rule_data_start;
static DecompRule* decomp_rules;

/** -------------------------------------------------------------------------------
 * RULE FUNCTIONS
*/

/**
 * @brief generate a new rule
 * will generate a symbol as its guard
*/
static void gen_rule(Rule** new_rule) {
    *new_rule = (Rule*)palloc(sizeof(Rule));
    num_rules++;
    (*new_rule)->count = 0;
    (*new_rule)->index = 0;  // index==0 means the rule has not been appended to rules array
    gen_rulesymbol(*new_rule, &((*new_rule)->guard));
    join_symbols((*new_rule)->guard, (*new_rule)->guard); // make guard points to itself
}

/**
 * @brief destory a rule
*/
static void destroy_rule(Rule* rule) {
    num_rules--;
    // guard is the last ruyle-symbol of the correspoding rule
    destroy_symbol(rule->guard);
    // free(rule);
}

/**
 * @brief each time reuse a rule-symbol should call this function to increace its reference count
*/
static inline void reuse_rule(Rule* rule) {
    rule->count++;
}

/**
 * @brief each time delete a rule-symbol should call this function to decreace its reference count
*/
static inline void deuse_rule(Rule* rule) {
    rule->count--;
}

/**
 * @brief generate a rule-symbol according to an existing rule
*/
static void gen_rulesymbol(Rule *rule, Symbol** new_symbol) {
    *new_symbol = (Symbol*)palloc(sizeof(Symbol));
    (*new_symbol)->data.rule = rule;
    (*new_symbol)->is_rulesymbol = 1;
    (*new_symbol)->p = (*new_symbol)->n = NULL;
    // when generating a rule symbol, that means the rule is reused:
    reuse_rule(rule);
}  

/**
 * @brief genrate a common symbol according to a word
*/
static void gen_symbol(char* word, uint32_t word_len, Symbol** new_symbol) {
    *new_symbol = (Symbol*)palloc(sizeof(Symbol));
    memset((*new_symbol)->data.word, '\0', MAX_WORD_LENGTH);
    strncpy((*new_symbol)->data.word, word, word_len);
    (*new_symbol)->is_rulesymbol = 0;
    (*new_symbol)->p = (*new_symbol)->n = NULL;
}

/**
 * @brief get the first symbol of a rule
*/
static inline Symbol* first(Rule* rule) {
    // the guard node's next-pointer points to the first symbol of a rule
    return rule->guard->n;
}

/**
 * @brief get the last symbol of a rule
*/
static inline Symbol* last(Rule* rule) {
    // the guard node's prevent-pointer points to the last symbol of a rule
    return rule->guard->p;
}

/**
 * @brief get the rule of a rule-symbol
*/
static inline Rule* get_rule(Symbol* rulesymbol) {
    // only the rule-symbol can call this function
    // assert(rulesymbol->is_rulesymbol);
    return rulesymbol->data.rule;
}

/**
 * @brief destroy a symbol
*/
static void destroy_symbol(Symbol* symbol) {
    join_symbols(symbol->p, symbol->n);
    if (!is_guard(symbol)) {
        delete_digram(symbol);
        if (non_terminal(symbol)) {
            Rule* rule = get_rule(symbol);
            deuse_rule(rule);
        }
    }
    // free(symbol);
}

/**
 * @brief whether the symbol is a non-terminal
*/
static inline uint32_t non_terminal(Symbol* symbol) {
    return symbol->is_rulesymbol;
}

/**
 * @brief whether the symbol is a guard of a rule
*/
static uint32_t is_guard(Symbol* symbol) {
    if (non_terminal(symbol)) {
        Rule* rule = get_rule(symbol);
        Symbol* last_symbol = last(rule);
        // if the symbol is a guard, then its last symbol's next pointer should points to it.
        return last_symbol->n == symbol;
    }
    else 
        return 0;
}

/**
 * @brief links two symbols together
 * consider the following situation:
 * before joining operation, the left symbol has been linked to another symbol
 * we should unlink the left symbol and its 
*/
void join_symbols(Symbol* left, Symbol* right) {
    if (left->n) {
        delete_digram(left);
        /**
         * the following guidance is from the origin author of sequitur
         * This is to deal with triples, where we only record the second pair of the overlapping digrams. 
         * When we delete the second pair, we insert the first pair into the hash table 
         * so that we don't forget about it.  e.g. abbbabcbb
        */
        // deal with three continous same numbers
        if (right->p && right->n && cmp_symbol(right->p, right) && cmp_symbol(right, right->n)) {
            *find_digram(right) = right;
        }
        if (left->p && left->n && cmp_symbol(left, left->n) && cmp_symbol(left->p, left)) {
            *find_digram(left->p) = left;
        }
    }
    left->n = right;
    right->p = left;
}

/**
 * @brief whether the left symbol equals to right symbol
*/
uint32_t cmp_symbol(Symbol* left, Symbol* right) {
    // first, rule-symbol and common symbol must be not equal
    if (left->is_rulesymbol != right->is_rulesymbol) 
        return 0;
    // both left symbol and right symbol are common symbols
    else if (left->is_rulesymbol == 0) {
        return strcmp(left->data.word, right->data.word) == 0;
    }
    // both left symbol and right symbol are rule-symbols
    else if (left->is_rulesymbol == 1) {
        // if they represent a same rule, then they are equal
        return left->data.rule == right->data.rule;
    }
}

/**
 * @brief delete a digram from digrams table
*/
void delete_digram(Symbol* symbol) {
    if (is_guard(symbol) || is_guard(symbol->n)) {
        return;
    }
    Symbol** target = find_digram(symbol);
    if (*target == symbol) 
        *target = (Symbol*)1; // 1 is a special flag of deleted element in digram table
}

/**
 * @brief insert right symbol after left symbol
*/
void insert_after(Symbol* left, Symbol* right) {
    join_symbols(right, left->n);   // seg fault here
    join_symbols(left, right);
}

/**
 * @brief expand a rule symbol to its sub-symbols
*/
void expand_rulesymbol(Symbol* rulesymbol) {
    // assert(rulesymbol->is_rulesymbol);
    Symbol* prev_sym = rulesymbol->p;
    Symbol* next_sym = rulesymbol->n;
    Rule* rule = get_rule(rulesymbol);
    Symbol* first_sym = first(rule);
    Symbol* last_sym = last(rule);

    destroy_rule(rule);
    Symbol** target = find_digram(rulesymbol);
    if (*target == rulesymbol) 
        *target = (Symbol*)1;
    rulesymbol->is_rulesymbol = 0;
    destroy_symbol(rulesymbol);
    // expand and delete a rule will generates two new digrams
    join_symbols(prev_sym, first_sym);
    join_symbols(last_sym, next_sym);
    *find_digram(last_sym) = (Symbol*)1;
}

/**
 * @brief match new_digram and old_digram, discuss whether to create a new rule or not
 * if a digram has been emerged before
    * if has a rule to record the digram, use the rule to substitude the digram
    * else generate a new rule
 * new_digram: new coming digram 
 * old_digram: digram which has been existed
 */ 
void match_digram(Symbol* new_digram, Symbol* old_digram) {
    Rule* rule;
    // if old_digram has been recorded as a rule
    if (is_guard(old_digram->p) && is_guard(old_digram->n->n)) {
        rule = get_rule(old_digram->p);  // old_digram->p is the guard of the rule
        replace_digram(new_digram, rule);  
    }
    // the rule is not recorded
    else {
        // generate a new rule
        gen_rule(&rule);    
        // generate symbols to insert into rule, origin symbols are deleted  
        // if new_digram is a nonTerminal
        if (non_terminal(new_digram)) { 
            Symbol* new_symbol;
            gen_rulesymbol(get_rule(new_digram), &new_symbol);
            insert_after(last(rule), new_symbol);
        }
        else {
            Symbol* new_symbol;
            gen_symbol(new_digram->data.word, strlen(new_digram->data.word), &new_symbol);
            insert_after(last(rule), new_symbol);
        }
        if (non_terminal(new_digram->n)) {
            Symbol* new_symbol;
            gen_rulesymbol(get_rule(new_digram->n), &new_symbol);
            insert_after(last(rule), new_symbol);
        }
        else {
            Symbol* new_symbol;
            gen_symbol(new_digram->n->data.word, strlen(new_digram->n->data.word), &new_symbol);
            insert_after(last(rule), new_symbol);
        }
        replace_digram(old_digram, rule);    
        replace_digram(new_digram, rule);
        //  insert digram that has been converted to rule into table
        *find_digram(first(rule)) = first(rule);  
        // the symbol digram in the rule is not the origin symbol any more
    } 
    Symbol* first_sym = first(rule);
    // if a rule is deused to 1 times, expand it and delete the rule
    if (non_terminal(first_sym) && get_rule(first_sym)->count == 1) {
        expand_rulesymbol(first_sym);
    }
}

/**
 * @brief A digram can be replaced with a non-terminal rule
 * parameter digram is the first symbol of the digram
*/
void replace_digram(Symbol* digram, Rule* rule) {
    // prev_sym is the symbol before the digram
    Symbol *prev_sym = digram->p;
    // destroy two symbols(which makes up of a digram)
    // destroy origin symbols is OK, because we will create new symbols to the rule in match function 
    destroy_symbol(prev_sym->n); // delete the first symbol of the digram
    destroy_symbol(prev_sym->n); // delete the second symbol of the digram
    // generate a new rule symbol
    // generate a new symbol with rule
    Symbol* new_symbol;
    gen_rulesymbol(rule, &new_symbol);
    insert_after(prev_sym, new_symbol);
    if (!check_digram(prev_sym))
        check_digram(prev_sym->n);
}

/**
 * @brief to check whether a new digram began with parameter new_digram has emerged in digram_table or not
 * if not emerged in digram_table, put it
 * else call function match_digram to generate a new rule or reuse an old rule
 */
uint32_t check_digram(Symbol* new_digram) {
    if (is_guard(new_digram) || is_guard(new_digram->n)) {
        return 0;
    }
    Symbol** target = find_digram(new_digram);
    if ((uint64_t)*target <= 1) { // an empty place or a deleted element
        *target = new_digram;  // the digram has not emerged before, insert it
        return 0;
    }
    // if find a existed digram, return 1
    if ((uint64_t)*target > 1 && (*target)->n != new_digram)
        match_digram(new_digram, *target);
    return 1;
}

/** --------------------------------------------------------------------------------------
 * @brief Hash table structure:
 * key -> a memory pointer
 * Hash table function:
*/

/**
 * @brief alloc memory and initialize digram table
*/
static inline void init_digram_table(int32_t size) {
    digram_table = (Symbol**)palloc(sizeof(Symbol*) * size);
    for (int i = 0; i < size; ++i) {
        digram_table[i] = NULL;
    }
}

/**
 * @brief clear digram table data
*/
static inline void clear_digram_table() {
    // free(digram_table);
}

/**
 * @brief murmurhash function, a open-source C version
 * copy from https://github.com/jwerle/murmurhash.c
*/
uint32_t murmurhash (const char *key, uint32_t len, uint32_t seed) {
    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;
    uint32_t r1 = 15;
    uint32_t r2 = 13;
    uint32_t m = 5;
    uint32_t n = 0xe6546b64;
    uint32_t h = 0;
    uint32_t k = 0;
    uint8_t *d = (uint8_t *) key; // 32 bit extract from `key'
    const uint32_t *chunks = NULL;
    const uint8_t *tail = NULL; // tail - last 8 bytes
    int i = 0;
    int l = len / 4; // chunk length

    h = seed;

    chunks = (const uint32_t *) (d + l * 4); // body
    tail = (const uint8_t *) (d + l * 4); // last 8 byte chunk of `key'

    // for each 4 byte chunk of `key'
    for (i = -l; i != 0; ++i) {
        // next 4 byte chunk of `key'
        k = chunks[i];

        // encode next 4 byte chunk of `key'
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        // append to hash
        h ^= k;
        h = (h << r2) | (h >> (32 - r2));
        h = h * m + n;
    }

    k = 0;

    // remainder
    switch (len & 3) { // `len % 4'
        case 3: k ^= (tail[2] << 16);
        case 2: k ^= (tail[1] << 8);
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h ^= k;
    }

    h ^= len;

    h ^= (h >> 16);
    h *= 0x85ebca6b;
    h ^= (h >> 13);
    h *= 0xc2b2ae35;
    h ^= (h >> 16);

    return h;
}

/**
 * @brief A function to hash rule symbol, this acctually hashes a uint64_t data
*/
static inline uint32_t hash_rulesymbol(Symbol* rulesymbol) {
    // use murmurhash to hash the rule pointer
    return murmurhash(
        (char*)rulesymbol->data.rule, 
        sizeof(Rule*), 
        HER_BIRTHDAY
    );
}

/**
 * @brief A function to hash common symbol, this hashed a string
*/
static inline uint32_t hash_symbol(Symbol* symbol) {
    return murmurhash(
        symbol->data.word, 
        strlen(symbol->data.word), 
        HER_BIRTHDAY
    );
}

static inline uint32_t hash(Symbol* symbol) {
    if (symbol->is_rulesymbol) {
        return hash_rulesymbol(symbol);
    } 
    else {
        return hash_symbol(symbol);
    }
}

static inline uint32_t get_init_place(Symbol* symbol1, Symbol* symbol2) {
    uint32_t one = hash(symbol1);
    uint32_t two = hash(symbol2);
    return (one<<16 | two) % PRIME;
}

static inline uint32_t get_jump(Symbol* symbol) {
    uint32_t one = hash(symbol);
    return 17 - one % 17;
}

/**
 * @brief find a digram began with symbol
*/
static Symbol** find_digram(Symbol* digram) {
    Symbol* first = digram;
    Symbol* second = digram->n;
    uint32_t cur_place = get_init_place(first, second);
    uint32_t jump = get_jump(first);
    uint32_t target_place = -1;
    while (1) {
        Symbol *m = digram_table[cur_place];
        // not happen hash coincidence / can't find anything
        if (!m) {
            if (target_place == -1) {
                target_place = cur_place;
            }
            return &digram_table[target_place];
        }
        // if the digram here has been deleted
        else if ((uint64_t)m == 1) {
            target_place = cur_place;
        }
        else if (cmp_symbol(m, first) && cmp_symbol(m->n, second)) {
            return &digram_table[cur_place];
        }
        cur_place = (cur_place + jump) % PRIME;
    }
}

/**
 * @brief Depth First Search algorithm based discover function
 * The main purpose of this function is to collect all the rules and assign index for them
*/
static void discover(Rule* rule) {
    rule->index = num_detected_rules;
    rule_array[num_detected_rules] = rule;
    num_detected_rules++;
    int num_subrules = 0;
    for (Symbol* s = first(rule); !is_guard(s); s = s->n) {
        if (non_terminal(s)) {
            num_subrules++;
        }
    }
    for (Symbol* s = first(rule); !is_guard(s); s = s->n) {
        // if this is a rule-symbol
        if (non_terminal(s)) {
            if (get_rule(s)->index == 0) {
                discover(get_rule(s));
            }
        }
    }
}

/**
 * @brief write a rule-symbol to target place
*/
static inline uint32_t write_rulesymbol(Symbol* rulesymbol, char* buffer) {
    Rule* rule = get_rule(rulesymbol);
    *buffer = RESERVE_CHAR;
    memcpy(buffer+1, &rule->index, sizeof(rule_index_t));
    return 1 + sizeof(rule_index_t);
}

/**
 * @brief write a common symbol to target place
*/
static inline uint32_t write_symbol(Symbol* symbol, char* buffer) {
    return sprintf(buffer, "%s", symbol->data.word);
}

/**
 * @brief write compressed data to dest
 * compressed data format:
 *                          ________________________________________________
 *                          |                                   +
 *  ____________________________________________________________________________________________________
 * | num rules(4 bytes) | rule0 offset(4 bytes) | rule1 offset |  ......   | rule0 | rule1 | ......   |
 * |_______________________|_________________________|_______________|__________|________|________|_________|
*/
static uint32_t write_compressed_data(char *dest, uint32_t raw_size) {
    char* write_pos = dest;
    // write raw data size
	*((uint32_t*)write_pos) = raw_size;
	write_pos += sizeof(uint32_t);
	// write number of rules (4 bytes):
    *((uint32_t*)write_pos) = num_rules;
    write_pos += sizeof(uint32_t);
    // write offset of each rule（4 bytes per rule）: 
    rule_end_offset = (rule_offset_t*)write_pos;
    write_pos = rule_data_start = write_pos + sizeof(rule_offset_t) * num_rules;
    for (int i = 0; i < num_rules; ++i) {
        Rule* rule = rule_array[i];
        // if (i != 0) {
        //     rule_end_offset[i-1] = write_pos - rule_data_start;
        // }
        for (Symbol* s = first(rule); !is_guard(s); s = s->n) {
            if (non_terminal(s)) {
                write_pos += write_rulesymbol(s, write_pos);
            }
            else {
                write_pos += write_symbol(s, write_pos);
            }
        }
        rule_end_offset[i] = write_pos - rule_data_start;
    }
    // rule_end_offset[num_rules-1] = write_pos - rule_data_start;
    return write_pos - dest;
}

/**
 * @brief decompress a rule using a Depth-First-Search way
*/
static void decompress_rule(rule_index_t index, char* rule_start, uint32_t rule_len, char* dest) {
    decomp_rules[index].decomp_start = dest;
    char* decomp_pos = rule_start;
    char* write_pos = dest;
    // debug("begin to decompress rule %d\n", index);
    while (decomp_pos != rule_start + rule_len) {
        // the character belones to a simple word
        if (*decomp_pos != RESERVE_CHAR) {
            *write_pos = *decomp_pos;
            write_pos++;
            decomp_pos++;
        }
        // the character represents a rule-symbol
        else {  
            rule_index_t subrule_index = *((rule_index_t*)(decomp_pos+1)); // +1 is used to jump reserve type
            // check whether the subrule has been decompressed
            if (decomp_rules[subrule_index].finished) {
                strncpy(write_pos, decomp_rules[subrule_index].decomp_start, decomp_rules[subrule_index].decomp_len);
                write_pos += decomp_rules[subrule_index].decomp_len;
            }
            else {
                rule_offset_t rule_begin_offset = subrule_index==0 ? 0 : rule_end_offset[subrule_index-1];
                char* subrule_start = rule_data_start + rule_begin_offset;
                uint32_t subrule_len = rule_end_offset[subrule_index] - rule_begin_offset;
                decompress_rule(subrule_index, subrule_start, subrule_len, write_pos);
                write_pos += decomp_rules[subrule_index].decomp_len;
            }
            decomp_pos += 1 + sizeof(rule_index_t);
        }
    }
    decomp_rules[index].decomp_len = write_pos - dest;
    decomp_rules[index].finished = 1;
}

/**
 * @brief check whether a character is a letter or not
*/
static inline uint32_t is_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

/**
 * @brief TADOC compression algorithm implement
 * source is uncompressed string
 * slen is the length of uncompressed string
 * dest is compressed string
*/
uint32_t __tadoc_compress(char *source, uint32_t slen, char *dest) {
    // initialize global varible
    num_rules = 0;
    num_detected_rules = 0;

    // initialize hash table
    init_digram_table(PRIME);
    // the start of a word
    char* start_ptr = source;
    // the end of a word
    char* cur_ptr = source;

    Rule* start_rule;
    gen_rule(&start_rule);

    /**
     * @brief the definition of "word":
     * A string of letters
     * Any single character which is not letter
    */

    // to traverse the source string, each time locate a single word
    // use double-pointer window to solve the problem, Time Complexity: O(n), n is the length of the string
    while(cur_ptr != source + slen) { // loop until read to EOF
        // if current reading character is a letter   
        if (is_letter(*cur_ptr)) {
            cur_ptr++;  // move the pointer
        }
        else {
            int word_len;
            // word_len = start_ptr==cur_ptr ? 1 : cur_ptr-start_ptr;
            word_len = start_ptr==cur_ptr ? 1 : cur_ptr-start_ptr+1;
            Symbol* new_symbol;
            gen_symbol(start_ptr, word_len, &new_symbol);
            insert_after(last(start_rule), new_symbol); // segmentatio fault here
            check_digram(last(start_rule)->p);
            // start_ptr = start_ptr==cur_ptr ? ++cur_ptr : cur_ptr;
            start_ptr = ++cur_ptr;
        }
        // printf("detected character id: %d, current number of rules: %d\n", num_ch, num_rules);
        // if (num_ch > 12852860) {
        //     printf("%c\n", *cur_ptr);
        // }
        // printf("%c", *cur_ptr);
        // num_ch++;
    }
    debug("%s\n", "finish reading input data");

    // all rules are stored in memory now, print to dest
    rule_array = (Rule**)palloc(sizeof(Rule*) * num_rules);
    // use a depth-first-search way to discover all rules
    discover(start_rule);
    debug("num_rules: %d, num_detected_rules: %d\n", num_rules, num_detected_rules);
    return write_compressed_data(dest, slen);
}

uint32_t __tadoc_decompress(char *source, char *dest) {
	char* decomp_pos = source;
	decomp_pos += sizeof(uint32_t); // to skip `raw_size`
    num_rules = *((uint32_t*)decomp_pos);
	decomp_pos += sizeof(uint32_t); // to skip `num_rules`
    debug("tadoc decompress: num_rules is %d\n", num_rules);
    // initialize decompress rules array
    decomp_rules = (DecompRule*)palloc(sizeof(DecompRule) * num_rules);
    for (int i = 0; i < num_rules; ++i) {
        decomp_rules[i].finished = 0;
    }
    rule_end_offset = (rule_offset_t*)decomp_pos;
    rule_data_start = decomp_pos + sizeof(rule_offset_t) * num_rules;
    decompress_rule(0, rule_data_start, rule_end_offset[0], dest);
    return decomp_rules[0].decomp_len;
}

#define MIN_BUF_SIZE 100000
text* tadoc_compress(struct varlena *source, Oid collid) {
	if (!OidIsValid(collid)) {
		/*
		 * This typically means that the parser could not resolve a conflict
		 * of implicit collations, so report it that way.
		 */
		ereport(ERROR, (
				errcode(ERRCODE_INDETERMINATE_COLLATION),
				errmsg("could not determine which collation to use for %s function", "tadoc_compess()"),
				errhint("Use the COLLATE clause to set the collation explicitly.")
			)
		);
	}

	char* raw_data_start = VARDATA_ANY(source);
	uint32_t raw_data_len = VARSIZE_ANY_EXHDR(source);
	// WARN: sometimes compressed data will be longer than origin data, so here we alloc 2 times of origin data
	uint32 estim_comp_len = VARSIZE_ANY_EXHDR(source) > MIN_BUF_SIZE ? VARSIZE_ANY_EXHDR(source) : MIN_BUF_SIZE * 2;
	text *dest = (text *)palloc(estim_comp_len);
	char* comp_data_start = VARDATA_ANY(dest);
	uint32 comp_size = __tadoc_compress(raw_data_start, raw_data_len, comp_data_start);

	printf("raw size: %d, compressed size: %d\n", VARSIZE_ANY_EXHDR(source), comp_size);
	SET_VARSIZE(dest, comp_size);
	return dest;
}

text * tadoc_decompress(struct varlena *source, Oid collid) {
	if (!OidIsValid(collid)) {
		/*
		 * This typically means that the parser could not resolve a conflict
		 * of implicit collations, so report it that way.
		 */
		ereport(ERROR, (
				errcode(ERRCODE_INDETERMINATE_COLLATION),
				errmsg("could not determine which collation to use for %s function", "tadoc_decompress()"),
				errhint("Use the COLLATE clause to set the collation explicitly.")
			)
		);
	}

	char* comp_data_start = VARDATA_ANY(source);
	// uint32_t comp_size = VARSIZE_ANY_EXHDR(source);
	uint32_t raw_size = *((uint32_t*)comp_data_start);
	text* dest = (text*)palloc(raw_size);
	char* raw_data_start = VARDATA_ANY(dest);
	__tadoc_decompress(comp_data_start, raw_data_start);
	return dest;
}