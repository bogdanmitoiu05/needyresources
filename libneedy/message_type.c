#include "message_type.h"

#include <stdio.h>
#include <string.h>
needy_message_type needy_message_type_from_string(const char* str) {
    if (strcmp(str,"resource_reequest") == 0) {
        return RESOURCE_REQUEST;
    }
    if (strcmp(str,"client_connection_request") == 0) {
        return CLIENT_CONNECTION_REQUEST;
    }
    if (strcmp(str, "server_ack") == 0) {
        return SERVER_ACK;
    }
    if (strcmp(str, "client_finalize") == 0) {
        return CLIENT_FINALIZE;
    }
    return MESSAGE_TYPE_UNKNOWN;
}
char* needy_message_type_to_string(needy_message_type type) {
    switch (type) {
        case RESOURCE_REQUEST:
            return strdup("resource_request");
        case CLIENT_CONNECTION_REQUEST:
            return strdup("client_connection_request");
        case SERVER_ACK:
            return strdup("server_ack");
        case CLIENT_FINALIZE:
            return strdup("client_finalize");
        default:
            fputs("needy_message_type_to_string: Invalid type", stderr);
            return NULL;
    }
}