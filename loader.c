#include "loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

loader_return_type_e load_location_csv_to_hashmap(char *path, void *map_geoid)
{
    FILE *fp = fopen(path, "r");
    if(fp == NULL) {
        return LOADER_PATH_NOT_FOUND;
    }

    size_t key_length = 10;
    char *key = malloc(sizeof(char) * key_length);

    size_t value_length = 80;
    char *value = malloc(sizeof(char) * value_length);

    char *line_buf = NULL;
    size_t line_buf_size = 0;

    // skip first line
    getline(&line_buf, &line_buf_size, fp);
    
    while(!feof(fp)) {
        getline(&line_buf, &line_buf_size, fp);
        if(strlen(line_buf) == 0) {
            continue;
        }

        // extract key
        {
            // calculate id_length
            char *id_end = strchr(line_buf, ',');
            size_t id_length = id_end - line_buf + 1;

            // in case of the length greater than memory size
            if(key_length < id_length) {
                key_length = id_length;
                free(key);
                key = malloc(sizeof(char) * key_length);
            }

            strncpy(key, line_buf, id_end - line_buf);
            key[id_length] = '\0';
        }

        // extract value
        {
            // calculate county_length
            char *country_start = line_buf;
            for(int i = 0; i < 5; ++i) {
                country_start = strchr(country_start + 1, ',');
            }
            char *country_end = strchr(country_start + 1, ',');
            size_t country_length = country_end - country_start - 1;

            // calculate timezon_length
            char *delim = line_buf;
            for(int i = 0; i < 12; ++i) {
                delim = strchr(delim + 1, ',');
            }
            size_t timezone_length = line_buf + strlen(line_buf) - delim;
            
            // in case of the length greater than memory size
            size_t total_length = country_length + timezone_length;
            if(value_length < total_length) {
                value_length = total_length;
                free(value);
                value = malloc(sizeof(char) * value_length);
            }

            strncpy(value, country_start + 1, country_length);
            value[country_length] = '\0'; 

            if(timezone_length >= 3) {
                strncat(value, delim, timezone_length);
                char *last_newline = strchr(value, '\n');
                if(last_newline != NULL) {
                    *last_newline = '\0';
                }            
            }
        }
        
        hashmap_insert(map_geoid, key, value);
        fflush(fp);
    }

    free(value);
    free(key);
    free(line_buf);
    fclose(fp);

    return LOADER_SUCCESS;
}

loader_return_type_e load_ipdb_csv_to_hashmap(char *path, void *map_ipdb, void *map_geoid)
{
    FILE *fp = fopen(path, "r");
    if(fp == NULL) {
        return LOADER_PATH_NOT_FOUND;
    }

    size_t key_length = 10;
    char *key = malloc(sizeof(char) * key_length);

    size_t value_length = 80;
    char *value = malloc(sizeof(char) * value_length);

    char *line_buf = NULL;
    size_t line_buf_size = 0;

    // skip first line
    getline(&line_buf, &line_buf_size, fp);
    
    while(!feof(fp)) {
        getline(&line_buf, &line_buf_size, fp);
        if(strlen(line_buf) == 0) {
            continue;
        }

        // extract key
        {
            // calculate ip_length
            char *ip_end = strchr(line_buf, ',');
            size_t ip_length = ip_end - line_buf + 1;

            // in case of the length greater than memory size
            if(key_length < ip_length) {
                key_length = ip_length;
                free(key);
                key = malloc(sizeof(char) * key_length);
            }
            
            strncpy(key, line_buf, ip_length);
            key[ip_length - 1] = '\0';
        }

        // extract value
        {
            // calculate geoid_length
            char *geoid_start = strchr(line_buf, ',') + 1;
            char *geoid_end = strchr(geoid_start + 1, ',');
            size_t geoid_length = geoid_end - geoid_start;

            // in case of the length greater than memory size
            if(value_length < geoid_length) {
                value_length = geoid_length;
                free(value);
                value = malloc(sizeof(char) * value_length);
            }

            // get geoid
            strncpy(value, geoid_start, geoid_length);
            value[geoid_length] = '\0';

            // get country_timezone
            const char *country_timezone;
            hashmap_search(map_geoid, value, &country_timezone);
            if(country_timezone == NULL ) {
                continue;
            }

            // get country_timezone_length
            size_t country_timezone_length = strlen(country_timezone);

            // in case of the length greater than memory size
            if(value_length < country_timezone_length) {
                value_length = country_timezone_length;
                free(value);
                value = malloc(sizeof(char) * value_length);
            }

            // set country_timezone to value
            strncpy(value, country_timezone, country_timezone_length);
            value[country_timezone_length] = '\0';
        }
        
        hashmap_insert(map_ipdb, key, value);
        fflush(fp);
    }

    free(value);
    free(key);
    free(line_buf);
    fclose(fp);

    return LOADER_SUCCESS;
}