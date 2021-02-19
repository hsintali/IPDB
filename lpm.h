#ifndef _LPM_H_
#define _LPM_H_

#include <stdlib.h>
#include <stdbool.h>

typedef enum lpm_return_type_e {
    LPM_SUCCESS = 0,
    LPM_FAILED = 1,
} lpm_return_type_e;

/********************************* APIs *********************************/
void * lpm_trie_init();

lpm_return_type_e lpm_insert(void *trie_hadnler, const char *key);

lpm_return_type_e lpm_search(void *trie_hadnler, const char *key, const char **value);

lpm_return_type_e lpm_delete(void *trie_hadnler, const char *key);

#endif