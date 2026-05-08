//
// Created by vscode on 4/11/26.
//

#ifndef NEEDYRESOURCES_MESSAGE_FRAME_H
#define NEEDYRESOURCES_MESSAGE_FRAME_H
#include <stddef.h>
#include <message_type.h>
#include <jansson.h>
// invelim fiecare mesaj in urmatoarea structura
/**
 * ```json
 * {
 *      "version":1,
 *      "type":"identification_request",
 *      "payload":{
 *      //...
 *      }
 * }
 *
 * ```
 */
typedef struct message {
    size_t version;
    needy_message_type message_type;
    json_t *payload;
}needy_message_t;

needy_message_t* needy_message_new(needy_message_type type, json_t* payload);
json_t* needy_message_serialize(needy_message_t* msg);
needy_message_t* needy_message_deserialize(json_t* msg);
char* needy_message_to_string(needy_message_t* msg);
needy_message_t* needy_message_from_string(char* str);

/**
 * Destroys a message.
 * @param msg message to be destroyed
 */
void needy_message_destroy(needy_message_t* msg);
#endif //NEEDYRESOURCES_MESSAGE_FRAME_H
