#include "hashmap.h"
#include "loader.h"
#include "ipdb_protocol.h"
#include "lpm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/time.h>
#include <signal.h>
#include <ctype.h>

#include <librdkafka/rdkafka.h>


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

static volatile sig_atomic_t run = 1;
static const char *brokers = "localhost";
static const char *input_topic = "prefixes";
static const char *output_topic = "location";

static lpm_trie_node_t *trie;


/***************************** private function declare **********************************/
static int connection_handler(int client_socket);
static int receive_message(int socket_id, char *opcode, char *key, char *value);
static void operate(char opcode, char *key, char *value, char *response);

static void stop(int sig);

static rd_kafka_t* create_input_consumer(const char *brokers, const char *input_topic);
static const char* process_message(rd_kafka_message_t *message);
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *message, void *opaque);
static rd_kafka_t* create_output_producer(const char *brokers, const char *output_topic);


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
            if(status == SUCCESS) {
                printf("lpm_insert:%d\n", lpm_insert(trie, key));
            }

            break;
        }
        case 'R': {
            const char *lpm_value = NULL;
            printf("lpm_search:%d\n", lpm_search(trie, key, &lpm_value));
            printf("Longest Prefix Match Value: %s\n", lpm_value);

            const char *search_value = NULL;
            status = hashmap_search(map_mydb, lpm_value, &search_value);
            if(status == SUCCESS) {
                sprintf(response, "Search with key(%s) - %s! The value is: \"%s\"", key, return_message[status], search_value);
                return;
            }

            status = hashmap_search(map_ipdb, lpm_value, &search_value);
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
            
            printf("lpm_delete:%d\n", lpm_delete(trie, key));
 
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

static void stop (int sig)
{
    run = 0;
}

static rd_kafka_t *create_input_consumer(const char *brokers, const char *input_topic)
{
    rd_kafka_t *kafka_handler;
    rd_kafka_conf_t *config = rd_kafka_conf_new();
    rd_kafka_topic_partition_list_t *subscription;
    rd_kafka_resp_err_t err;
    char err_str[512];
    const char *groupid = "ipdb_group";

    if(rd_kafka_conf_set(config, "bootstrap.servers", brokers, err_str, sizeof(err_str)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", err_str);
        rd_kafka_conf_destroy(config);
        return NULL;
    }
    if(rd_kafka_conf_set(config, "group.id", groupid, err_str, sizeof(err_str)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", err_str);
        rd_kafka_conf_destroy(config);
        return NULL;
    }
    if(rd_kafka_conf_set(config, "auto.offset.reset", "earliest", err_str, sizeof(err_str)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", err_str);
        rd_kafka_conf_destroy(config);
        return NULL;
    }

    kafka_handler = rd_kafka_new(RD_KAFKA_CONSUMER, config, err_str, sizeof(err_str));
    if (!kafka_handler) {
        rd_kafka_conf_destroy(config);
        fprintf(stderr, "Failed to create consumer: %s", err_str);
    }

    config = NULL;

    rd_kafka_poll_set_consumer(kafka_handler);

    subscription = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(subscription, input_topic, RD_KAFKA_PARTITION_UA);

    err = rd_kafka_subscribe(kafka_handler, subscription);
    if(err) {
        fprintf(stderr, "%% Failed to subscribe to %d topics: %s\n", subscription->cnt, rd_kafka_err2str(err));
        rd_kafka_topic_partition_list_destroy(subscription);
        rd_kafka_destroy(kafka_handler);
        return NULL;
    }
    rd_kafka_topic_partition_list_destroy(subscription);

    return kafka_handler;
}

static const char* process_message(rd_kafka_message_t *message)
{
    if(!message) {
        return NULL;
    }

    if (message->err) {
        fprintf(stderr, "%% Consumer error: %s\n", rd_kafka_message_errstr(message));
        rd_kafka_message_destroy(message);
        return NULL;
    }
    printf("%% Message on %s [%"PRId32"] at offset %"PRId64":\n", rd_kafka_topic_name(message->rkt), message->partition, message->offset);

    if(message->key) {
        printf("%% Key: %.*s (%d bytes)\n", (int)message->key_len, (const char *)message->key, (int)message->key_len);
    }
    if(message->payload) {
        printf("%% Value: %.*s (%d bytes)\n", (int)message->len, (const char *)message->payload, (int)message->len);
    }

    // lookup hashmap
    const char *lpm_value = NULL;
    lpm_search(trie, (const char *)message->payload, &lpm_value);
    printf("%% Longest Prefix Match Value: %s\n", lpm_value);

    const char *search_value = NULL;
    return_type_e status;
    status = hashmap_search(map_mydb, lpm_value, &search_value);
    if(status != SUCCESS) {
        status = hashmap_search(map_ipdb, lpm_value, &search_value);
    }

    return search_value;
}

static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *message, void *opaque) {
    if (message->err) {
        fprintf(stderr, "%% Message delivery failed: %s\n", rd_kafka_err2str(message->err));
    }
    else {
        fprintf(stderr, "%% Message delivered (%zd bytes, " "partition %"PRId32")\n", message->len, message->partition);
    }
}

static rd_kafka_t* create_output_producer(const char *brokers, const char *output_topic)
{
    rd_kafka_t *kafka_handler;
    rd_kafka_conf_t *config = rd_kafka_conf_new();
    rd_kafka_resp_err_t err;
    char err_str[512];

    if(rd_kafka_conf_set(config, "bootstrap.servers", brokers, err_str, sizeof(err_str)) != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%s\n", err_str);
        rd_kafka_conf_destroy(config);
        return NULL;
    }

    rd_kafka_conf_set_dr_msg_cb(config, dr_msg_cb);

    kafka_handler = rd_kafka_new(RD_KAFKA_PRODUCER, config, err_str, sizeof(err_str));
    if (!kafka_handler) {
        rd_kafka_conf_destroy(config);
        fprintf(stderr, "Failed to create consumer: %s", err_str);
    }

    config = NULL;

    return kafka_handler;
}
/***************************** main **********************************/
int main(int argc, char **args)
{
    if(argc != 5) {
        printf("Parameter error!\n");
        return 1;
    }

    // hashmap
    int map_size = 100000;
    void *map_geoid = hashmap_table_create(map_size);
    map_ipdb = hashmap_table_create(map_size);
    map_mydb = hashmap_table_create(map_size);

    // trie
    trie = lpm_create_node();

    // load location csv to hashmap
    printf("Loading geoid data ...");
    char geoid_path[256];
    sprintf(geoid_path, "%s/%s", args[1], args[4]);
    if(load_location_csv_to_hashmap(geoid_path, map_geoid) != LOADER_SUCCESS) {
        printf("GeoID: %s - PATH NOT FOUND!\n", geoid_path);
    }
    else {
        printf("OK\n");
    }
    
    // load ipv4 csv to hashmap with mapped geoid
    printf("Loading ipv4 data ...");
    char ipv4_path[256];
    sprintf(ipv4_path, "%s/%s", args[1], args[2]);
    if(load_ipdb_csv_to_hashmap(ipv4_path, map_ipdb, map_geoid, trie) != LOADER_SUCCESS) {
        printf("IPv4: %s - PATH NOT FOUND!\n", ipv4_path);
    }
    else {
        printf("OK\n");
    }

    // load ipv6 csv to hashmap with mapped geoid
    printf("Loading ipv6 data ...");
    char ipv6_path[256];
    sprintf(ipv6_path, "%s/%s", args[1], args[3]);
    if(load_ipdb_csv_to_hashmap(ipv6_path, map_ipdb, map_geoid, trie) != LOADER_SUCCESS) {
        printf("IPv6: %s - PATH NOT FOUND!\n", ipv6_path);
    }
    else {
        printf("OK\n");
    }
    
    hashmap_table_destroy(map_geoid);
    map_geoid = NULL;

    // load mydb
    printf("Loading mydb data ...");
    sprintf(mydb_path, "%s/mydb.csv", args[1]);
    if(load_mydb_csv_to_hashmap(mydb_path, map_mydb, trie) != LOADER_SUCCESS) {
        printf("mydb: PATH NOT FOUND!\n");
    }
    else {
        printf("OK\n");
    }
    
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
    struct timeval timeout = {0, 1};

    // client socket
    int client_socket = 0;
    struct sockaddr_in client_address;
    socklen_t client_address_size;

    // kafka
    rd_kafka_t *consumer, *producer;
    consumer = create_input_consumer(brokers, input_topic);
    producer = create_output_producer(brokers, output_topic);
 
    /* Signal handler for clean shutdown */
    signal(SIGINT, stop);

    while(run) {
        // kafka consume/process message
        rd_kafka_message_t *input_message;
        input_message = rd_kafka_consumer_poll(consumer, 1);
        const char *out_message = process_message(input_message);

        // kafka produce message
        rd_kafka_resp_err_t err = RD_KAFKA_RESP_ERR_INVALID_MSG;
        while(err != RD_KAFKA_RESP_ERR_NO_ERROR && out_message != NULL) {
            err = rd_kafka_producev(producer, RD_KAFKA_V_TOPIC(output_topic),
                                              RD_KAFKA_V_KEY(input_message->payload, input_message->len),
                                              RD_KAFKA_V_VALUE(out_message, strlen(out_message)),
                                              RD_KAFKA_V_END);
            if(err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
                rd_kafka_poll(producer, 100);
                continue;
            }
            else if(err) {
                fprintf(stderr, "%% Failed to produce to topic %s: %s\n", output_topic, rd_kafka_err2str(err));
                break;
            }
            else {
                fprintf(stderr, "%% Enqueued message %s (%zd bytes) for topic %s\n", out_message, strlen(out_message), output_topic);
            }
        }
        rd_kafka_poll(producer, 0);
        if(input_message) rd_kafka_message_destroy(input_message);
        
        // set select read_fd_set
        read_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
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

    /* Close the consumer: commit final offsets and leave the group. */
    fprintf(stderr, "%% Closing consumer\n");
    rd_kafka_consumer_close(consumer);

    // Destroy the consumer and producer
    rd_kafka_destroy(consumer);
    rd_kafka_destroy(producer);

    // close socket
    close(server_socket);

    // delete hashmap
    hashmap_table_destroy(map_ipdb);
    map_ipdb = NULL;
    hashmap_table_destroy(map_mydb);
    map_mydb = NULL;
    
    return 0;
}