#include "hashmap.h"
#include "loader.h"
#include "ipdb_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


/***************************** private attribute **********************************/
static void *map_ipdb;
static void *map_mydb;
static char mydb_path[256];
static const int buffer_size = 256;
static const int message_size = 1024;
static char return_message[][30] = {"SUCCESS",
                                    "KEY NOT FOUND",
                                    "KEY EXISTS",
                                    "KEY INVALID",
                                    "VALUE INVALID",
                                    "TABLE INVALID"};


/***************************** private function declare **********************************/
static int connection_handler(int client_socket);
static int receive_message(int socket_id, char *opcode, char *key, char *value);
static void operate(char opcode, char *key, char *value, char *response);


/***************************** private function definition **********************************/
static int connection_handler(int client_socket)
{
    char opcode = '\0';
    char key[buffer_size];
    char value[buffer_size];
   
    if(receive_message(client_socket, &opcode, key, value) != 0) {
        return -1;
    }
    printf("(Client - %d): opcode:%c, key:%s(%d), value:%s(%d)\n", client_socket, opcode, key, strlen(key), value, strlen(value));
    
    char response[message_size];

    operate(opcode, key, value, response);

    send(client_socket, response, sizeof(response), 0);
    printf("(Client - %d): response: %s\n", client_socket, response);

    return 0;
}

static int receive_message(int socket_id, char *opcode, char *key, char *value)
{
    char buffer[buffer_size];

    size_t header_size = sizeof(ipdb_message_t);
    if(recv(socket_id, buffer, header_size, 0) <= 0) {
        return -1;
    };

    size_t data_size = ((ipdb_message_t *)buffer)->packet_length - header_size;
    if(data_size > 0 && recv(socket_id, buffer + header_size, data_size, 0) <= 0) {
        return -1;
    };
  
    ipdb_deserialized_message((ipdb_message_t *)buffer, opcode, key, value);
        
    return 0;
}

static void operate(char opcode, char *key, char *value, char *response)
{
    return_type_e status, status_mydb;

    switch(opcode) {
        case 'C': {
            const char *search_value = NULL;
            status = hashmap_search(map_ipdb, key, &search_value);
            if(status == SUCCESS) {
                sprintf(response, "Insert with key(%s) and value(%s) - %s!", key, value, return_message[2]);
                return;
            }
            status = hashmap_insert(map_mydb, key, value);
            sprintf(response, "Insert with key(%s) and value(%s) - %s!", key, value, return_message[status]);
            break;
        }
        case 'R': {
            const char *search_value = NULL;
            status = hashmap_search(map_mydb, key, &search_value);
            if(status == SUCCESS) {
                sprintf(response, "Search with key(%s) - %s! The value is: \"%s\"", key, return_message[status], search_value);
                return;
            }
            status = hashmap_search(map_ipdb, key, &search_value);
            sprintf(response, "Search with key(%s) - %s! The value is: \"%s\"", key, return_message[status], search_value);
            break;
        }
        case 'U': {
            status = hashmap_update(map_mydb, key, value);
            if(status == SUCCESS) {
                sprintf(response, "Update with key(%s) and value(%s) - %s!", key, value, return_message[status]);
                return;
            }
            status = hashmap_update(map_ipdb, key, value);
            if(status == SUCCESS) {
                hashmap_insert(map_mydb, key, value);
            }
            sprintf(response, "Update with key(%s) and value(%s) - %s!", key, value, return_message[status]);
            break;
        }
        case 'D': {
            status_mydb = hashmap_delete(map_mydb, key);
            status = hashmap_delete(map_ipdb, key);
            if(status_mydb < status) {
                sprintf(response, "Delete with key(%s) - %s!", key, return_message[status_mydb]);
            }
            else {
                sprintf(response, "Delete with key(%s) - %s!", key, return_message[status]);
            }
            break;
        }
        case 'S': {
            if(save_hashmap_mydb_to_csv(mydb_path, map_mydb) != LOADER_SUCCESS) {
                sprintf(response, "Save failed!");
            }
            sprintf(response, "Save success!");
            break;
        }
        default: {
            sprintf(response, "Invalid operation!");
            break;
        }
    }
}

/***************************** main **********************************/
int main(int argc, char **args)
{
    if(argc != 5) {
        printf("Parameter error!\n");
        return 1;
    }

    int map_size = 100000;
    void *map_geoid = hashmap_table_create(map_size);
    map_ipdb = hashmap_table_create(map_size);
    map_mydb = hashmap_table_create(map_size);

    // load location csv to hashmap
    printf("Loading geoid data ...");
    char geoid_path[256];
    sprintf(geoid_path, "%s/%s", args[1], args[4]);
    if(load_location_csv_to_hashmap(geoid_path, map_geoid) != LOADER_SUCCESS) {
        printf("GeoID: PATH NOT FOUND!\n");
    }
    printf("OK\n");
    
    // load ipv4 csv to hashmap with mapped geoid
    printf("Loading ipv4 data ...");
    char ipv4_path[256];
    sprintf(ipv4_path, "%s/%s", args[1], args[2]);
    if(load_ipdb_csv_to_hashmap(ipv4_path, map_ipdb, map_geoid) != LOADER_SUCCESS) {
        printf("IPv4: PATH NOT FOUND!\n");
    }
    printf("OK\n");

    // load ipv6 csv to hashmap with mapped geoid
    printf("Loading ipv6 data ...");
    char ipv6_path[256];
    sprintf(ipv6_path, "%s/%s", args[1], args[3]);
    if(load_ipdb_csv_to_hashmap(ipv6_path, map_ipdb, map_geoid) != LOADER_SUCCESS) {
        printf("IPv6: PATH NOT FOUND!\n");
    }
    printf("OK\n");
    
    hashmap_table_destroy(map_geoid);
    map_geoid = NULL;

    // load mydb
    printf("Loading mydb data ...");
    sprintf(mydb_path, "%s/mydb.csv", args[1]);
    if(load_mydb_csv_to_hashmap(mydb_path, map_mydb) != LOADER_SUCCESS) {
        printf("mydb: PATH NOT FOUND!\n");
    }
    printf("OK\n");

    printf("Total %d records in ipdb\n", hashmap_get_size(map_ipdb) + hashmap_get_size(map_mydb));

    // create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1) {
        printf("Socket creating failed!\n");
        return 1;
    }
    printf("Server created\n");

    // define server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    if(bind( server_socket, (struct sockaddr*) &server_address, sizeof(server_address) ) != 0) {
        printf("Binding failed!\n");
        return 2;
    };
    printf("Binding success\n");

    // listen
    listen(server_socket, 5);
    printf("Listening for incoming connections ...\n");

    // select
    fd_set active_fd_set, read_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket,&active_fd_set);

    // client socket
    int client_socket = 0;
    struct sockaddr_in client_address;
    socklen_t client_address_size;

    while(1) {
        read_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            printf("\"select\" error!\n");
            break;
        }

        for(int i = 0; i < FD_SETSIZE; ++i) {
            if(FD_ISSET(i, &read_fd_set)) {
                // accept incoming socket
                if(i == server_socket) {
                    client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_size);
                    if(client_socket < 0) {
                        printf("INVALID socket!\n");
                        continue;
                    }
                    printf("(Client - %d): connection accepted\n", client_socket);
                    printf("(Server): connect from host (%s), port (%d)\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    FD_SET(client_socket, &active_fd_set);

                     // send welcome message to the client
                    char *welcome_message = "Hi there! I am your connection handler.\nPlease send me the operating message to operate IPDB.\n";
                    send(client_socket, welcome_message, strlen(welcome_message), 0);
                }
                else { // processing received message
                    if(connection_handler(i) != 0) {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                        printf("(Client - %d): close connection\n", i);
                    }
                }
            }
        }
    }

    close(server_socket);

    hashmap_table_destroy(map_ipdb);
    map_ipdb = NULL;

    hashmap_table_destroy(map_mydb);
    map_mydb = NULL;
    
    return 0;
}