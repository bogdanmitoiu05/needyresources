#include "message_type.h"

#include <stdio.h>
#include <string.h>
#include "message.h"
needy_message_type needy_message_type_from_string(const char* str) {
    if (strcmp(str,"resource_request") == 0) {
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
    if (strcmp(str, "resource_response") == 0)
    {
        return RESOURCE_RESPONSE;
    }
    if (strcmp(str, "read_request") == 0)
    {
        return READ_REQUEST;
    }
    if (strcmp(str, "write_request") == 0)
    {
        return WRITE_REQUEST;
    }
    if (strcmp(str, "file_response") == 0)
    {
        return FILE_RESPONSE;
    }
    return MESSAGE_TYPE_UNKNOWN;
}
char* needy_message_type_to_string(needy_message_type type) {
    switch (type) {
    case RESOURCE_REQUEST:
        return fixed_strdup("resource_request");
    case CLIENT_CONNECTION_REQUEST:
        return fixed_strdup("client_connection_request");
    case SERVER_ACK:
        return fixed_strdup("server_ack");
    case CLIENT_FINALIZE:
        return fixed_strdup("client_finalize");
    case RESOURCE_RESPONSE:
        return fixed_strdup("resource_response");
    case READ_REQUEST:
        return fixed_strdup("read_request");
    case WRITE_REQUEST:
        return fixed_strdup("write_request");
    case FILE_RESPONSE:
        return fixed_strdup("file_response");
    default:
        fputs("needy_message_type_to_string: Invalid type", stderr);
        return NULL;
    }
}