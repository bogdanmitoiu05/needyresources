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

json_t * needy_message_serialize(needy_message_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_serialize: msg is NULL");
    if (msg->version != NEEDY_PROTOCOL_VERSION) {
        fprintf(stderr,"Invalid protocol version %lu. Protocol version %lu needed", msg->version, (size_t) NEEDY_PROTOCOL_VERSION);
        return NULL;
    }
    json_t* result = json_pack("{s:I,s:s,s:O}","version",msg->version,"type",msg->message_type,"payload",msg->payload);
    ENSURE_NOTNULL_MSG_RNULL(result, "needy_message_serialize: Failed to serialize message");
    return result;
}

needy_message_t * needy_message_deserialize(json_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: msg is NULL");
    needy_message_t* result = calloc(1, sizeof(needy_message_t));
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: could not allocate message");
    json_error_t error;
    const char* type = NULL;
    int success = json_unpack_ex(msg, &error, JSON_STRICT, "{s:I,s:s,s:O}", "version",&(result->version), "type",type, "payload",&(result->payload));
    if (!success) {
        JSON_ERROR_PRINTF(error);
        free(result);
        return NULL;
    }
    result->message_type = needy_message_type_from_string(type);
    return result;

}

char* needy_message_to_string(needy_message_t* msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg,"needy_message_to_string: msg is NULL");
    json_t* root = needy_message_serialize(msg);
    return json_dumps(root, JSON_COMPACT);
}

needy_message_t* needy_message_from_string(char* str) {
    json_error_t error;
    json_t* msgJson = json_loads(str, JSON_REJECT_DUPLICATES, &error);
    if (!msgJson) {
        JSON_ERROR_PRINTF(error);
        return NULL;
    }
    return needy_message_deserialize(msgJson);
}
void needy_message_destroy(needy_message_t *msg) {
    ENSURE_NOTNULL(msg);
    if (msg->payload)
        json_decref(msg->payload);
    free(msg);
}
