#include "hashmap.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/********************************* structs *********************************/
typedef struct hashmap_node_s {
    struct hashmap_node_s *next;
    char *key;
    char *value;
    int str_length;
} hashmap_node_t;

typedef struct {
    int capacity;
    int size;
    hashmap_node_t **buckets;
} hashmap_table_t;

/***************************** private members **********************************/
static hashmap_node_t* __hashmap_node_create(const char * const key, const char * const value);
static void __hashmap_node_delete(hashmap_node_t *node);

// return NULL: if not found
static hashmap_node_t* __hashmap_find_item_node(hashmap_table_t *table, const char * const key);

static unsigned long long int __fast_pow(const int x, const unsigned int n);
static int __hashmap_hash(const char * const str, const unsigned int prime, const unsigned int capacity);

/********************************* attributes *********************************/
const static int HASH_PRIME = 37;
const static int DEFAULT_CAPACITY = 256;

/********************************* APIs *********************************/
void* hashmap_table_create(unsigned int capacity)
{
    if(capacity == 0) {
        capacity = DEFAULT_CAPACITY;
    }
    
    hashmap_table_t *table = malloc(sizeof(hashmap_table_t));
    table->capacity = capacity;
    table->size = 0;

    table->buckets = calloc(capacity, sizeof(hashmap_node_t *));
    for(unsigned int i = 0; i < table->capacity; ++i) {
        table->buckets[i] = __hashmap_node_create(NULL, NULL);
    }

    return (void*)table;
}

return_type_e hashmap_table_destroy(void *table_handler)
{   
    if(table_handler == NULL) {
        return TABLE_INVALID;
    }

    hashmap_table_t *table = (hashmap_table_t *)table_handler;

    for(unsigned int i = 0; i < table->capacity; ++i) {
        hashmap_node_t *node = table->buckets[i];
        hashmap_node_t *nextNode = NULL;
        while(node != NULL) {
            nextNode = node->next;
            __hashmap_node_delete(node);
            node = nextNode;
        }
    }

    if(table->buckets) {
        free(table->buckets);
    }

    if(table) {
        free(table);
    }

    return SUCCESS;
}

return_type_e hashmap_insert(void *table_handler, const char * const key, const char * const value)
{
    if(table_handler == NULL) {
        return TABLE_INVALID;
    }

    if(key == NULL) {
        return KEY_INVALID;
    }

    if(value == NULL) {
        return VALUE_INVALID;
    }

    hashmap_table_t *table = (hashmap_table_t *)table_handler;

    hashmap_node_t *node = __hashmap_find_item_node(table, key);
    if(node != NULL) {
        return KEY_EXISTS;
    }

    int index = __hashmap_hash(key, HASH_PRIME, table->capacity);
    node = table->buckets[index];
    hashmap_node_t *nextNode = node->next;
    node->next = __hashmap_node_create(key, value);
    node->next->next = nextNode;
    ++table->size;

    return SUCCESS;
}

return_type_e hashmap_update(void *table_handler, const char * const key, const char * const value)
{
    if(table_handler == NULL) {
        return TABLE_INVALID;
    }

    if(key == NULL) {
        return KEY_INVALID;
    }

    if(value == NULL) {
        return VALUE_INVALID;
    }

    hashmap_table_t *table = (hashmap_table_t *)table_handler;

    hashmap_node_t *node = __hashmap_find_item_node(table, key);
    if(node == NULL) {
        return KEY_NOT_FOUND;
    }

    free(node->value);
    node->value = strdup(value);
    node->str_length = strlen(value);

    return SUCCESS;
}

return_type_e hashmap_delete(void *table_handler, const char * const key)
{
    if(table_handler == NULL) {
        return TABLE_INVALID;
    }

    if(key == NULL) {
        return KEY_INVALID;
    }

    hashmap_table_t *table = (hashmap_table_t *)table_handler;

    int index = __hashmap_hash(key, HASH_PRIME, table->capacity);
    hashmap_node_t *cur = table->buckets[index];

    hashmap_node_t *pre = NULL;
    hashmap_node_t *next = NULL;
    
    while(cur != NULL) {
        next = cur->next;
        if(cur->key != NULL && strcmp(cur->key, key) == 0) {
            break;
        }
        pre = cur;
        cur = cur->next;
    }

    if(cur == NULL) {
        return KEY_NOT_FOUND;
    }

    __hashmap_node_delete(cur);
    pre->next = next;
    --table->size;

    return SUCCESS;
}

return_type_e hashmap_search(void *table_handler, const char * const key, const char **value)
{
    if(table_handler == NULL) {
        return TABLE_INVALID;
    }

    if(key == NULL) {
        return KEY_INVALID;
    }

    hashmap_table_t *table = (hashmap_table_t *)table_handler;

    hashmap_node_t *node = __hashmap_find_item_node(table, key);
    if(node == NULL) {
        return KEY_NOT_FOUND;
    }

    *value = node->value;
    return SUCCESS;
}

return_type_e hashmap_dump(void *table_handler)
{
    hashmap_table_t *table = (hashmap_table_t *)table_handler;
    if(table == NULL) {
        return TABLE_INVALID;
    }

    printf("---------- table with size: %d ----------\n", table->size);
    for(int i = 0; i < table->capacity; ++i) {
        hashmap_node_t *node = table->buckets[i];
        if(node->next == NULL) continue;
        while(node != NULL) {
            printf("index: %d | key: %s | value: %s | value_length: %d \n", i, node->key, node->value, node->str_length);
            node = node->next;
        }       
    }

    return SUCCESS;
}

return_type_e hashmap_get_bucket(void *table_handler, int index, char *buffer)
{
    hashmap_table_t *table = (hashmap_table_t *)table_handler;
    if(table == NULL) {
        return TABLE_INVALID;
    }

    if(index < 0 || index >= table->capacity) {
        return INDEX_ERROR;
    }

    hashmap_node_t *node = table->buckets[index];
    if(node->next == NULL) {
        return SUCCESS;
    }

    node = node->next;

    while(node != NULL) {
        sprintf(buffer, "%s%s %s\n", buffer, node->key, node->value);
        node = node->next;
    }       

    return SUCCESS;
}

int hashmap_get_size(void *table_handler)
{
    return ((hashmap_table_t *)table_handler)->size;
}
/***************************** private members **********************************/
static hashmap_node_t* __hashmap_node_create(const char * const key, const char * const value)
{
    hashmap_node_t *node = malloc(sizeof(hashmap_node_t));
    node->next = NULL;
    node->key = NULL;
    node->value = NULL;
    node->str_length = 0;

    if(key == NULL) {
        return node;
    }
    
    node->key = strdup(key);
    node->value = strdup(value);
    node->str_length = strlen(value);

    return node;
}

static void __hashmap_node_delete(hashmap_node_t *node)
{
    if(node->key) {
        free(node->key);
    }

    if(node->value) {
        free(node->value);
    }

    if(node) {
        free(node);
    }
}

static hashmap_node_t* __hashmap_find_item_node(hashmap_table_t *table, const char * const key)
{
    int index = __hashmap_hash(key, HASH_PRIME, table->capacity);
    hashmap_node_t *node = table->buckets[index];

    while(node != NULL) {
        if(node->key != NULL && strcmp(node->key, key) == 0) {
            break;
        }
        node = node->next;
    }

    return node;
}

static unsigned long long int __fast_pow(int x, const unsigned int n)
{
    unsigned int N = n;    
    unsigned long long int ans = 1;
    unsigned long long int product = x;

    while(N > 0) {
        if(N % 2 == 1) {
            ans *= product;
        }
        
        N /= 2;
        product *= product;
    }

    return ans;
}

static int __hashmap_hash(const char * const s, const unsigned int prime, const unsigned int capacity)
{
    unsigned long long int hash = 0;
    const int str_length = strlen(s);

    for(unsigned int i = 0; i < str_length; ++i) {
        hash += (unsigned long long int) __fast_pow(prime, str_length - i - 1) * s[i];
        hash %= capacity;
    } 
    
    return hash;
}