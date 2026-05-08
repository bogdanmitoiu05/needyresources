//
// Created by vscode on 4/9/26.
//

#include "resource_request.h"


__attribute_warn_unused_result__ needy_resource_request* nr_client_resource_request_new(const pid_t pid, const size_t requestedResources) {
    needy_resource_request* res = calloc(1,sizeof(needy_resource_request));
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->requestedResources = requestedResources;
    return res;
}

__attribute_warn_unused_result__ needy_resource_request * nr_client_resource_request_deserialize(json_t *data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "nr_client_resource_deserialize: NULL pointer passed");
    if (!json_is_object(data)) {
        fputs("nr_client_resource_deserialize: Data passed not an object",stderr);
        return NULL;
    }
    pid_t pid = 0;
    size_t requestedResources = 0;
    json_error_t error;
    int success = json_unpack_ex(data, &error, JSON_STRICT, "{s:I,s:I}","pid",&pid, "requested_resources",&requestedResources);
    if (!success) {
        fputs("nr_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    needy_resource_request* resource_request = nr_client_resource_request_new(pid,requestedResources);
    return resource_request;
}

__attribute_warn_unused_result__ json_t* nr_client_resource_request_serialize(const needy_resource_request *this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "nr_client_resource_serialize: NULL pointer passed");

    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:I,s:I}","pid",this->pid, "requested_resources", this->requestedResources);
    if (!result) {
        fputs("nr_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}

void nr_client_resource_request_destroy(needy_resource_request *this) {
    ENSURE_NOTNULL(this);
    free(this);
}
