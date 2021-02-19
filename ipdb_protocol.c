#include "ipdb_protocol.h"

#include <string.h>
#include <stdio.h>


void ipdb_serialized_message(ipdb_message_t *message, char opcode, char *key, char *value)
{
    char buffer[1024];
    uint8_t key_length = strlen(key);
    uint8_t value_length = strlen(value);
    message->packet_length = sizeof(ipdb_message_t) + key_length + value_length;
    message->opcode = opcode;
    message->key_length = key_length;
    message->value_length = value_length;
    sprintf(buffer, "%s%s", key, value);
    memcpy(message->data, buffer, key_length + value_length);
}

void ipdb_deserialized_message(ipdb_message_t *message, char *opcode, char *key, char *value)
{
    *opcode = message->opcode;
    memcpy(key, message->data, message->key_length);
    key[message->key_length] = '\0';
    memcpy(value, message->data + message->key_length, message->value_length);
    value[message->value_length] = '\0';
}