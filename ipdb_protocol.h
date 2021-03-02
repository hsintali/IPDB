#ifndef _IPDB_PROTOCOL_H_
#define _IPDB_PROTOCOL_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    char opcode;
    char key[256];
    char value[256];
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