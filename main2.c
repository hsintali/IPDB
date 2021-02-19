#include "hashmap.h"
#include "loader.h"
#include "lpm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char **args)
{
    if(argc != 5) {
        printf("Parameter error!\n");
        return 1;
    }

    // hashmap
    int map_size = 100000;
    void *map_geoid = hashmap_table_create(map_size);
    void *map_ipdb = hashmap_table_create(map_size);
    void *map_mydb = hashmap_table_create(map_size);

    // trie
    void *trie = lpm_trie_init();

    // load location csv to hashmap
    printf("Loading geoid data ...");
    char geoid_path[256];
    sprintf(geoid_path, "%s/%s", args[1], args[4]);
    if(load_location_csv_to_hashmap(geoid_path, map_geoid) != LOADER_SUCCESS) {
        printf("GeoID: %s - PATH NOT FOUND!\n", geoid_path);
    }
    printf("OK\n");
    
    // load ipv4 csv to hashmap with mapped geoid
    printf("Loading ipv4 data ...");
    char ipv4_path[256];
    sprintf(ipv4_path, "%s/%s", args[1], args[2]);
    if(load_ipdb_csv_to_hashmap(ipv4_path, map_ipdb, map_geoid, trie) != LOADER_SUCCESS) {
        printf("IPv4: %s - PATH NOT FOUND!\n", ipv4_path);
    }
    printf("OK\n");

    // load ipv6 csv to hashmap with mapped geoid
    // printf("Loading ipv6 data ...");
    // char ipv6_path[256];
    // sprintf(ipv6_path, "%s/%s", args[1], args[3]);
    // if(load_ipdb_csv_to_hashmap(ipv6_path, map_ipdb, map_geoid, trie) != LOADER_SUCCESS) {
    //     printf("IPv6: %s - PATH NOT FOUND!\n", ipv6_path);
    // }
    // printf("OK\n");
    
    hashmap_table_destroy(map_geoid);
    map_geoid = NULL;


    char res[1999];
    char *addr = "161.184.137.0";
    const char *search_value = NULL; 
    
    
    lpm_search(trie, addr, &search_value);
    sprintf(res, "%s\n", search_value);
    printf("%s", res);

    lpm_insert(trie, "161.184.137.0/26");

    search_value = NULL;
    lpm_search(trie, addr, &search_value);
    sprintf(res, "%s\n", search_value);
    printf("%s", res);

    // printf("delete:%d\n", lpm_delete(trie, "161.181.253.24/29"));
    // printf("insert:%d\n", lpm_insert(trie, "161.181.253.0/24"));
    // printf("insert:%d\n", lpm_insert(trie, "161.181.253.0/24"));

    // search_value = NULL;
    // lpm_search(trie, addr, &search_value);
    // sprintf(res, "%s\n", search_value);
    // printf("%s\n", res);
    
    return 0;
}