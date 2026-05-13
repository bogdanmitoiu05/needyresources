//
// Created by vscode on 5/8/26.
//

#include "server_ack.h"

#include <string.h>

__attribute_warn_unused_result__ needy_server_ack* needy_server_ack_new(const pid_t pid, size_t response) {
    needy_server_ack* res = calloc(1,sizeof(needy_server_ack)); //alocare
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->response = response;
    return res;
}

needy_server_ack* needy_server_ack_deserialize(json_t* data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_client_resource_deserialize: NULL pointer passed"); // null guard
    pid_t pid = 0;
    size_t response;
    json_error_t error;
    //despachetare
    int success = json_unpack_ex(data, &error, JSON_STRICT/*verifica respecatarea exacta a formatului*/, "{s:I,s:I}"/*string: long long, string: long long*/,"pid",&pid, "response",&response);
    if (success<0) {
        fputs("needy_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    needy_server_ack* client_response = needy_server_ack_new(pid,response);
    return client_response;
}

json_t* needy_server_ack_serialize(const needy_server_ack* this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_client_resource_serialize: NULL pointer passed");

    //idem, invers
    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:I,s:I}","pid",this->pid, "response", this->response);
    if (!result) {
        fputs("needy_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}

void needy_server_ack_destroy(needy_server_ack* this) {
    ENSURE_NOTNULL(this); //nu apelăm free pe
    free(this);
}
