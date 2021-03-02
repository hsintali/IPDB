#include "ipdb_protocol.h"

#include <string.h>
#include <stdio.h>


void ipdb_serialized_message(ipdb_message_t *message, char opcode, char *key, char *value)
{
    message->opcode = opcode;
    strcpy(message->key, key);
    strcpy(message->value, value);
}

void ipdb_deserialized_message(ipdb_message_t *message, char *opcode, char *key, char *value)
{
    *opcode = message->opcode;
    strcpy(key, message->key);
    strcpy(value, message->value);
}