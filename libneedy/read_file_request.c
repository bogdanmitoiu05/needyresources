//
// Created by vscode on 6/7/26.
//
#include "read_file_request.h"
#include <string.h>

needy_read_file_request* needy_read_file_request_new(pid_t pid, const char* file_name) {

    needy_read_file_request* res = new(needy_read_file_request);

    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->file_name = strdup(file_name);
    return res;
}

needy_read_file_request* needy_read_file_request_deserialize(json_t* data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_read_file_request_deserialize: NULL pointer passed");
    pid_t pid = 0;
    const char* file_name = NULL;
    json_error_t error;

    int success = json_unpack_ex(data, &error, JSON_STRICT, "{s:i, s:s}","pid",&pid, "file_name", &file_name);
    if (success<0) {
        fputs("needy_read_file_request_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    needy_read_file_request* client_response = needy_read_file_request_new(pid, file_name);
    return client_response;
}
json_t* needy_read_file_request_serialize(const needy_read_file_request* this) {

    ENSURE_NOTNULL_MSG_RNULL(this, "needy_read_file_request_serialize: NULL pointer passed");

    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:i, s:s}","pid",this->pid, "file_name", this->file_name);
    if (!result) {
        fputs("needy_read_file_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}
void needy_read_file_request_destroy(needy_read_file_request* this) {
    ENSURE_NOTNULL(this);
    free(this->file_name);
    free(this);
}