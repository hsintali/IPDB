#include "lpm.h"

#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include <arpa/inet.h>


/********************************* structs *********************************/



/********************************* attributes *********************************/
const int LPM_MAX_PREFIX = 128;


/***************************** private members **********************************/
static unsigned int char_to_index(char c) {
	return (int)c - (int)'a';
}

static int ip_to_bin_str(const char *cidr, char *addr, unsigned int *prefix_len)
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

		return 0;
	}

	return 1;
}

static int is_empty(lpm_trie_node_t *root)
{
    for (int i = 0; i < 2; ++i) {
		if (root->next[i]) {
			return 0;
		}
	}
    return 1;
}

lpm_trie_node_t * recursive_delete(lpm_trie_node_t *root, const char *key, int depth)
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
        if (is_empty(root)) {
            free(root);
            root = NULL;
        }

        return root;
    }

    // If not last character, recur for the child obtained using ASCII value
    int index = key[depth] - '0';
    root->next[index] = recursive_delete(root->next[index], key, depth + 1);

    // If root does not have any child (its only child got deleted), and it is not end of another word.
    if (is_empty(root) && root->is_prefix == false) {
        free(root);
        root = NULL;
    }

    return root;
}


/********************************* APIs *********************************/
lpm_trie_node_t * lpm_create_node()
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

lpm_return_type_e lpm_insert(lpm_trie_node_t *root, const char *key)
{
	// convert to binary
	char key_binary[129] = {'\0'};
	unsigned int prefix_len;
    if(ip_to_bin_str(key, key_binary, &prefix_len) != 0) {
		return LPM_FAILED;
	}

    int length = strlen(key_binary); 
    int index;
    lpm_trie_node_t *cur_node = root; 

    for(int i = 0; i < length; ++i) 
    {
		index = key_binary[i] - '0';
 
        if (!cur_node->next[index]) {
			cur_node->next[index] = lpm_create_node();
		}

        cur_node = cur_node->next[index]; 
    }

	if(cur_node->is_prefix != false || cur_node->prefix != NULL) {
		return LPM_FAILED;
	}

	cur_node->is_prefix = true;
	cur_node->prefix = strdup(key);

	return LPM_SUCCESS;
}

lpm_return_type_e lpm_search(lpm_trie_node_t *root, const char *key, const char **value)
{
	*value = NULL;

	// convert to binary
	char key_binary[129] = {'\0'};
	unsigned int prefix_len;
	if(ip_to_bin_str(key, key_binary, &prefix_len) != 0) {
		return LPM_FAILED;
	}

	int cnt = 0;
    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *cur_node = root;

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
		return LPM_FAILED;
	}

	return LPM_SUCCESS;
}

lpm_return_type_e lpm_delete(lpm_trie_node_t *root, const char *key)
{
	// convert to binary
	char key_binary[129] = {'\0'};
	unsigned int prefix_len;
    if(ip_to_bin_str(key, key_binary, &prefix_len) != 0) {
		return LPM_FAILED;
	}

	// determine if key exists
    int length = strlen(key_binary);
    int index;
    lpm_trie_node_t *cur_node = root;
    for(int i = 0; i < length; ++i)
    {
        index = key_binary[i] - '0';
        if (!cur_node->next[index]) {
			return LPM_FAILED;
		}
        cur_node = cur_node->next[index];
    }
	if(cur_node == NULL || !cur_node->is_prefix) {
		return LPM_FAILED;
	}

	recursive_delete(root, key_binary, 0);

	return LPM_SUCCESS;
}