#include "lpm.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <arpa/inet.h>


/********************************* structs *********************************/
typedef struct lpm_trie_node_s {
    struct lpm_trie_node_s *next[2];
    bool is_prefix;
    char * prefix;
} lpm_trie_node_t;

typedef struct lpm_trie_s {
    lpm_trie_node_t *root;
    lpm_trie_node_t *ipv4_node;
} lpm_trie_t;


/********************************* attributes *********************************/
static const int LPM_MAX_PREFIX = 128;


/***************************** private function declare **********************************/
static int __ip_to_bin_str(const char *cidr, char *addr, size_t *len, unsigned int *prefix_len);

static lpm_trie_node_t * __create_node();

static int __is_empty(lpm_trie_node_t *root);
static lpm_trie_node_t * __recursive_delete(lpm_trie_node_t *root, const char *key, int depth);


/***************************** private function definition **********************************/
static int __ip_to_bin_str(const char *cidr, char *addr, size_t *len, unsigned int *prefix_len)
{
    char *p;
    char buf[INET6_ADDRSTRLEN];
    unsigned int addr_buf[4];
    memset(addr_buf, '\0', sizeof(unsigned int) * sizeof(addr_buf));

    strncpy(buf, cidr, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    // parsing
    if((p = strchr(buf, '/')) != NULL) {
        const ptrdiff_t off = p - buf;
        *prefix_len = atoi(&buf[off + 1]);
        buf[off] = '\0';
    }
    else {
        *prefix_len = LPM_MAX_PREFIX;
    }

    // ipv6
    if(inet_pton(AF_INET6, buf, addr_buf) == 1) {
        int cnt = 0;
        for(int i = 0; i < 4; ++i) {
            addr_buf[i] = ntohl(addr_buf[i]);
            for(int j = 0; j < 32 && cnt < *prefix_len; ++j) {
                addr[cnt++] = ((addr_buf[i] >> (31 - j)) & 1) + '0';
            }
        }

        *len = 16;
        return 0;
    }

    // ipv4
    if(inet_pton(AF_INET, buf, addr_buf) == 1) {
        if(*prefix_len == LPM_MAX_PREFIX) {
            *prefix_len = 32;
        }

        int cnt = 0;
        addr_buf[0] = ntohl(addr_buf[0]);
        for(int i = 0; i < 32 && cnt < *prefix_len; ++i) {
            addr[cnt++] = ((addr_buf[0] >> (31 - i)) & 1) + '0';
        }

        *len = 4;
        return 0;
    }

    return 1;
}

static lpm_trie_node_t * __create_node()
{
    lpm_trie_node_t *node = (lpm_trie_node_t *) malloc(sizeof(lpm_trie_node_t)); 

    if(node) {  
        node->is_prefix = false;
        node->prefix = NULL;
  
        for(int i = 0; i < 2; i++) {
            node->next[i] = NULL;
        }
    }

    return node; 
}

static int __is_empty(lpm_trie_node_t *root)
{
    for (int i = 0; i < 2; ++i) {
        if (root->next[i]) {
            return 0;
        }
    }
    return 1;
}

static lpm_trie_node_t * __recursive_delete(lpm_trie_node_t *root, const char *key, int depth)
{
    if (!root) {
        return NULL;
    }

    // If last character of key is being processed
    if (depth == strlen(key)) {
        // This node is no more end of word after removal of given key
        if (root->is_prefix) {
            free(root->prefix);
            root->prefix = NULL;
            root->is_prefix = false;
        }

        // If given is not prefix of any other word
        if (__is_empty(root)) {
            free(root);
            root = NULL;
        }

        return root;
    }

    // If not last character, recur for the child obtained using ASCII value
    int index = key[depth] - '0';
    root->next[index] = __recursive_delete(root->next[index], key, depth + 1);

    // If root does not have any child (its only child got deleted), and it is not end of another word.
    if (__is_empty(root) && root->is_prefix == false) {
        free(root);
        root = NULL;
    }

    return root;
}


/********************************* APIs *********************************/
void * lpm_trie_init()
{
    lpm_trie_t *trie = (lpm_trie_t *) malloc(sizeof(lpm_trie_t));
    trie->root = __create_node();

    // convert to binary
    char key_binary[129] = {'\0'};
    size_t key_binary_len = 0;
    unsigned int prefix_len;
    __ip_to_bin_str("0:0:0:0:0:ffff::/96", key_binary, &key_binary_len, &prefix_len);

    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *cur_node = trie->root;

    for(int i = 0; i < length; ++i)
    {
        index = key_binary[i] - '0';

        if (!cur_node->next[index]) {
            cur_node->next[index] = __create_node();
        }

        cur_node = cur_node->next[index];
    }

    trie->ipv4_node = cur_node;

    return (void *)trie; 
}

lpm_return_type_e lpm_insert(void *trie_hadnler, const char *key)
{
    if(trie_hadnler == NULL) {
        return LPM_TRIE_INVALID;
    }

    if(key == NULL) {
        return LPM_KEY_INVALID;
    }

    lpm_trie_t *trie = (lpm_trie_t *)trie_hadnler;

    // convert to binary
    char key_binary[129] = {'\0'};
    size_t key_binary_len = 0;
    unsigned int prefix_len;
    if(__ip_to_bin_str(key, key_binary, &key_binary_len, &prefix_len) != 0) {
        return LPM_KEY_INVALID;
    }

    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *cur_node = (key_binary_len == 16) ? trie->root : trie->ipv4_node;

    for(int i = 0; i < length; ++i)
    {
        index = key_binary[i] - '0';
 
        if (!cur_node->next[index]) {
            cur_node->next[index] = __create_node();
        }

        cur_node = cur_node->next[index]; 
    }

    if(cur_node->is_prefix != false || cur_node->prefix != NULL) {
        return LPM_KEY_EXISTS;
    }

    cur_node->is_prefix = true;
    cur_node->prefix = strdup(key);

    return LPM_SUCCESS;
}

lpm_return_type_e lpm_search(void *trie_hadnler, const char *key, const char **value)
{
    if(trie_hadnler == NULL) {
        return LPM_TRIE_INVALID;
    }

    if(key == NULL) {
        return LPM_KEY_INVALID;
    }

    lpm_trie_t *trie = (lpm_trie_t *)trie_hadnler;

    *value = NULL;

    // convert to binary
    char key_binary[129] = {'\0'};
    size_t key_binary_len = 0;
    unsigned int prefix_len;
    if(__ip_to_bin_str(key, key_binary, &key_binary_len, &prefix_len) != 0) {
        return LPM_KEY_INVALID;
    }

    int cnt = 0;
    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *cur_node = (key_binary_len == 16) ? trie->root : trie->ipv4_node;

    while(cur_node != NULL && cnt < length)
    {
        if(cur_node != NULL && cur_node->is_prefix && cur_node->prefix != NULL) {
            *value = cur_node->prefix;
        }

        index = key_binary[cnt++] - '0';

        if (!cur_node->next[index]) {
            break;
        }

        cur_node = cur_node->next[index];
    }

    if(cur_node != NULL && cur_node->is_prefix && cur_node->prefix != NULL) {
        *value = cur_node->prefix;
    }

    if(*value == NULL) {
        return LPM_KEY_NOT_FOUND;
    }

    return LPM_SUCCESS;
}

lpm_return_type_e lpm_delete(void *trie_hadnler, const char *key)
{
    if(trie_hadnler == NULL) {
        return LPM_TRIE_INVALID;
    }

    if(key == NULL) {
        return LPM_KEY_INVALID;
    }

    lpm_trie_t *trie = (lpm_trie_t *)trie_hadnler;

    // convert to binary
    char key_binary[129] = {'\0'};
    size_t key_binary_len = 0;
    unsigned int prefix_len;
    if(__ip_to_bin_str(key, key_binary, &key_binary_len, &prefix_len) != 0) {
        return LPM_KEY_INVALID;
    }

    // determine if key exists
    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *root = (key_binary_len == 16) ? trie->root : trie->ipv4_node;
    lpm_trie_node_t *cur_node = root;
    for(int i = 0; i < length; ++i)
    {
        index = key_binary[i] - '0';
        if (!cur_node->next[index]) {
            return LPM_KEY_NOT_FOUND;
        }
        cur_node = cur_node->next[index];
    }
    if(cur_node == NULL || !cur_node->is_prefix) {
        return LPM_KEY_NOT_FOUND;
    }

    __recursive_delete(root, key_binary, 0);

    return LPM_SUCCESS;
}