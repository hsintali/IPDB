#include "ipdb_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


int main(int argc, char **argv)
{
    // create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = inet_addr("192.168.11.246");

    if(connect( client_socket, (struct sockaddr *) &server_address, sizeof(server_address) ) != 0)
    {
        printf("Connection error!\n");
        return 1;
    }
    printf("Connected\n");

    // reveive data from the server
    char server_response[1024];
    recv(client_socket, server_response, sizeof(server_response), 0);
    printf("(Server):%s \n", server_response);

    char opcode;
    char key[256];
    char value[256];
    ipdb_message_t *message = malloc(sizeof(ipdb_message_t) + 1024);

    while(1) {
        printf("Opcode:");
        scanf("%s", &opcode);
        getchar();

        if(opcode == 'q') {
            break;
        }

        memset(key, '\0', 256);
        if(opcode != 'S') {
            printf("Key:");
            fgets(key, 256, stdin);
            key[strlen(key) - 1] = '\0';
        }

        memset(value, '\0', 256);
        if(opcode == 'C' || opcode == 'U') {
            printf("value:");
            fgets(value, 256, stdin);
            value[strlen(value) - 1] = '\0';
        }

        ipdb_serialized_message(message, opcode, key, value);

        // send data to the server
        send(client_socket, message, message->packet_length, 0);
        printf("send length: %d\n", message->packet_length);

        // reveive data from the server
        recv(client_socket, server_response, sizeof(server_response), 0);
        printf("(Server): %s\n\n", server_response);
    }

    // close the socket
    close(client_socket);
    printf("Connection closed\n");

    return 0;
}