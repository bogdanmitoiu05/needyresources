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
#endif //NEEDYRESOURCES_MESSAGE_FRAME_H
