#ifndef _LOADER_H_
#define _LOADER_H_

#include "hashmap.h"
#include "lpm.h"

typedef enum loader_return_type_e {
    LOADER_SUCCESS = 0,
    LOADER_PATH_NOT_FOUND = -1,
} loader_return_type_e;

/** load_location_csv_to_hashmap
  * Load the geo location data and extract (geoid, country-timezone) as a key-value pair inserted to the map_geoid table.
  
  * @para_in    path        A pointer to the csv path
  * @para_in    map_geoid   A pointer to the geoid table handler
  * @return LOADER_SUCCESS, LOADER_PATH_NOT_FOUND
*/
loader_return_type_e load_location_csv_to_hashmap(char *path, void *map_geoid);

/** load_ipdb_csv_to_hashmap
  * Load ipv4 and ipv6 data and extract (ip, country-timezone) as a key-value pair inserted to the map_ipdb table.
  * The country-timezone information is extract from map_geoid table.
  
  * @para_in    path        A pointer to the csv path
  * @para_in    map_ipdb    A pointer to the ipdb table handler
  * @para_in    map_geoid   A pointer to the geoid table handler
  * @return LOADER_SUCCESS, LOADER_PATH_NOT_FOUND
*/
loader_return_type_e load_ipdb_csv_to_hashmap(char *path, void *map_ipdb, void *map_geoid, void *trie);

/** load_mydb_csv_to_hashmap
  * 
  
  * @para_in    path        A pointer to the csv path
  * @para_in    map_mydb    A pointer to the mydb table handler
  * @return LOADER_SUCCESS, LOADER_PATH_NOT_FOUND
*/
loader_return_type_e load_mydb_csv_to_hashmap(char *path, void *map_mydb, void *trie);

/** save_hashmap_mydb_to_csv
  * 
  
  * @para_in    path        A pointer to the csv path
  * @para_in    map_mydb    A pointer to the mydb table handler
  * @return LOADER_SUCCESS, LOADER_PATH_NOT_FOUND
*/
loader_return_type_e save_hashmap_mydb_to_csv(char *path, void *map_mydb);

#endif