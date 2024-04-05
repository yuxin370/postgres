#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/**
 * TADOC Decompressed Rule Entry
*/
struct DecompRule {
    // o means the rule have not been finish decompression step yet
    uint8_t finished;  
    char* decomp_start;
    uint32_t decomp_len;
};


typedef struct DecompRule DecompRule;
// type to indicate the id of rule
#ifdef MINI_NUM_RULES
	typedef uint16_t rule_index_t;
#else
	typedef uint32_t rule_index_t;
#endif
typedef uint32_t rule_offset_t;
#define RESERVE_CHAR '^'