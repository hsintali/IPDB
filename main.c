#include "hashmap.h"
#include "loader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argv, char **args)
{
    int map_size = 100000;
    void *map_geoid = hashmap_table_create(map_size);
    void *map_ipdb = hashmap_table_create(map_size);

    // load location csv to hashmap
    printf("Loading geoid data ... ");
    char geoid_path[256];
    sprintf(geoid_path, "%s/%s", args[1], args[4]);
    load_location_csv_to_hashmap(geoid_path, map_geoid);
    printf("OK.\n");
    
    // load ipv4 csv to hashmap with mapped geoid
    printf("Loading ipv4 data ... ");
    char ipv4_path[256];
    sprintf(ipv4_path, "%s/%s", args[1], args[2]);
    load_ipdb_csv_to_hashmap(ipv4_path, map_ipdb, map_geoid);
    printf("OK.\n");

    // load ipv6 csv to hashmap with mapped geoid
    printf("Loading ipv6 data ... ");
    char ipv6_path[256];
    sprintf(ipv6_path, "%s/%s", args[1], args[3]);
    load_ipdb_csv_to_hashmap(ipv6_path, map_ipdb, map_geoid);
    printf("OK.\n");
    
    hashmap_table_destroy(map_geoid);
    map_geoid = NULL;

    // hashmap_dump(map_ipdb);
    
    while(1) {
        char cmd;
        scanf("%c", &cmd);
        if(cmd == 'q') {
            break;
        }
        else if(cmd == 'c') {
            printf("Key:");
            char key[64];
            scanf("%s", key);

            printf("Value:");
            char value[64];
            scanf("%s", value);

            printf("insert %s: %d\n", key, hashmap_insert(map_ipdb, key, value));
            printf("\n\n");
        }
        else if(cmd == 'r') {
            printf("Key:");
            char key[64];
            scanf("%s", key);

            const char *value;
            return_type_e status = hashmap_search(map_ipdb, key, &value);
            printf("ibdb[%s]: %s (%d)\n", key, value, status);
            printf("\n\n");
        }
        else if(cmd == 'u') {
            printf("Key:");
            char key[64];
            scanf("%s", key);

            printf("Value:");
            char value[64];
            scanf("%s", value);

            printf("update %s: %d\n", key, hashmap_update(map_ipdb, key, value));
            printf("\n\n");
        }
        else if(cmd == 'd') {
            printf("Key:");
            char key[64];
            scanf("%s", key);

            printf("delete %s: %d\n", key, hashmap_delete(map_ipdb, key));
            printf("\n\n");
        }
    }

    hashmap_table_destroy(map_ipdb);
    map_ipdb = NULL;

    return 0;
}