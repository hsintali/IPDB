#ifndef _LPM_H_
#define _LPM_H_


#include <stdlib.h>
#include <stdbool.h>


typedef struct lpm_trie_node_s {
    struct lpm_trie_node_s *next[2];
    bool is_prefix;
	char * prefix;
    
} lpm_trie_node_t;

typedef enum lpm_return_type_e {
    LPM_SUCCESS = 0,
    LPM_FAILED = 1,
} lpm_return_type_e;


/********************************* APIs *********************************/
lpm_trie_node_t * lpm_create_node();

lpm_return_type_e lpm_insert(lpm_trie_node_t *root, const char *key);

lpm_return_type_e lpm_search(lpm_trie_node_t *root, const char *key, const char **value);

lpm_return_type_e lpm_delete(lpm_trie_node_t *root, const char *key);


#endif