//
// Created by vscode on 5/8/26.
//
#include "client_finalize.h"

needy_client_finalize* needy_client_finalize_new(pid_t pid, size_t message) {
    needy_client_finalize* res = calloc(1,sizeof(needy_client_finalize));
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->message = message;
    return res;
}

needy_client_finalize* needy_client_finalize_deserialize(json_t* data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_client_resource_deserialize: NULL pointer passed");
    if (!json_is_object(data)) {
        fputs("needy_client_resource_deserialize: Data passed not an object",stderr);
        return NULL;
    }
    pid_t pid = 0;
    size_t message= 0;
    json_error_t error;
    int success = json_unpack_ex(data, &error, JSON_STRICT, "{s:I,s:I}","pid",&pid, "response",&message);
    if (!success) {
        fputs("needy_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    needy_client_finalize* client_response = needy_client_finalize_new(pid,message);
    return client_response;
}
json_t* needy_client_finalize_serialize(const needy_client_finalize* this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_client_resource_serialize: NULL pointer passed");

    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:I,s:I}","pid",this->pid, "requestedResources", this->message);
    if (!result) {
        fputs("needy_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}
void needy_client_finalize_free(needy_client_finalize* this) {
    ENSURE_NOTNULL(this);
    free(this);
}