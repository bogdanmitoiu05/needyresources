//
// Created by vscode on 5/8/26.
//

#include "server_ack.h"

#include <string.h>

__attribute_warn_unused_result__ needy_server_ack* needy_server_ack_new(const pid_t pid, size_t response) {
    needy_server_ack* res = calloc(1,sizeof(needy_server_ack));
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->response = response;
    return res;
}

needy_server_ack* needy_server_ack_deserialize(json_t* data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_client_resource_deserialize: NULL pointer passed");
    if (!json_is_object(data)) {
        fputs("needy_client_resource_deserialize: Data passed not an object",stderr);
        return NULL;
    }
    pid_t pid = 0;
    size_t response;
    json_error_t error;
    int success = json_unpack_ex(data, &error, JSON_STRICT, "{s:I,s:I}","pid",&pid, "response",&response);
    if (!success) {
        fputs("needy_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    needy_server_ack* client_response = needy_server_ack_new(pid,response);
    return client_response;
}

json_t* needy_server_ack_serialize(const needy_server_ack* this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_client_resource_serialize: NULL pointer passed");

    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:I,s:I}","pid",this->pid, "response", this->response);
    if (!result) {
        fputs("needy_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}

void needy_server_ack_free(needy_server_ack* this) {
    ENSURE_NOTNULL(this);
    free(this);
}
