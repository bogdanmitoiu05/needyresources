//
// Created by vscode on 4/11/26.
//

#include "message.h"

#include "needy-private.h"
#include "constants.h"

needy_message_t* needy_message_new(needy_message_type type, json_t *payload) {
    ENSURE_NOTNULL_MSG_RNULL(payload, "needy_message_new: payload passed was a NULL pointer");
    if (type > N_TYPES) {
        printf("needy_message_new: Invalid type %u\n", type);
        return NULL;
    }
    needy_message_t* message = calloc(1,  sizeof(needy_message_t));
    ENSURE_NOTNULL_MSG_RNULL(message, "needy_message_new: could not allocate message");
    message->version = NEEDY_PROTOCOL_VERSION;
    message->message_type = type;
    message->payload = json_incref(payload);
    return message;

}

void needy_message_destroy(needy_message_t *msg) {
    ENSURE_NOTNULL(msg);
    if (msg->payload)
        json_decref(msg->payload);
    free(msg);
}
