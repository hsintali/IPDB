#ifndef _HASHMAP_H_
#define _HASHMAP_H_

typedef enum return_type_e {
    SUCCESS = 0,
    KEY_NOT_FOUND = -1,
    KEY_EXISTS = -2,
    KEY_INVALID = -3,
    VALUE_INVALID = -4,
    TABLE_INVALID = -5,
} return_type_e;

/** hashmap_table_create
  * Create a hashtable with capacity.
  
  * @para_in    capacity   Capacity of the hashtable. Set to default 256 if capacity is 0
  * @return pointer to the table handler
*/
void* hashmap_table_create(unsigned int capacity);

/** hashmap_table_destroy
  * Destroy the specific hashtable.
  
  * @para_in    table_handler   A pointer to the table handler
  * @return SUCCESS, TABLE_INVALID
*/
return_type_e hashmap_table_destroy(void *table);

/** hashmap_insert
  * Insert a key-value pair to the specific hashtable.
  
  * @para_in    table_handler   pointer to the table handler
  * @para_in    key             pointer to the key string
  * @para_in    value           pointer to the value string
  * @return SUCCESS, KEY_EXISTS, KEY_INVALID, VALUE_INVALID, TABLE_INVALID
*/
return_type_e hashmap_insert(void *table_handler, const char * const key, const char * const value);

/** hashmap_update
  * Update the value of the hashtable with specific key.
  
  * @para_in    table_handler   pointer to the table handler
  * @para_in    key             pointer to the key string
  * @para_in    value           pointer to the value string
  * @return SUCCESS, KEY_NOT_FOUND, KEY_INVALID, VALUE_INVALID, TABLE_INVALID
*/
return_type_e hashmap_update(void *table_handler, const char * const key, const char * const value);

/** hashmap_delete
  * Delete a key-value pair of the specific hashtable.
  
  * @para_in    table_handler   pointer to the table handler
  * @para_in    key             pointer to the key string
  * @return SUCCESS, KEY_NOT_FOUND, KEY_INVALID, TABLE_INVALID
*/
return_type_e hashmap_delete(void *table_handler, const char * const key);

/** hashmap_search
  * Search the value of the hashtable with specific key.
  
  * @para_in    table_handler   pointer to the table handler
  * @para_in    key             pointer to the key string
  * @para_out   value           pointer to the result string of the searching
  * @return SUCCESS, KEY_NOT_FOUND, KEY_INVALID, TABLE_INVALID
*/
return_type_e hashmap_search(void *table_handler, const char * const key, const char **value);

/** hashmap_dump
  * Display all nodes of the hashtable.
  
  * @para_in    table_handler   pointer to the table handler
  * @return SUCCESS, TABLE_INVALID
*/
return_type_e hashmap_dump(void *table_handler);

#endif