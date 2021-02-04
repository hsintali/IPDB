#ifndef _IPDB_PROTOCOL_H_
#define _IPDB_PROTOCOL_H_

#include <stdlib.h>
#include <stdint.h>


typedef struct {
    size_t packet_length;
    char opcode;
    uint8_t key_length;
    uint8_t value_length;
    char data[0];
} ipdb_message_t;


/** ipdb_serialized_message
  * 
  
  * @para_in
  * @return
*/
void ipdb_serialized_message(ipdb_message_t *message, char opcode, char *key, char *value);

/** ipdb_serialized_message
  * 
  
  * @para_in
  * @return
*/
void ipdb_deserialized_message(ipdb_message_t *message, char *opcode, char *key, char *value);




#endif