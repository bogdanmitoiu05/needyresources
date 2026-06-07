//
// Created by vscode on 5/8/26.
//
#include "file_request_response.h"
#include <string.h>

needy_file_request_response* needy_file_request_response_new(response_code code, const char* file_contents) {

    needy_file_request_response* res = new(needy_file_request_response);

    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate needy_file_request_response");
    res->code = code;
    if (file_contents) {
        res->file_contents = strdup(file_contents);
    } else {
        res->file_contents = NULL;
    }
    return res;
}

needy_file_request_response* needy_file_request_response_deserialize(json_t* data) {

    ENSURE_NOTNULL_MSG_RNULL(data, "needy_file_request_response_deserialize: NULL pointer passed");
    int code = 0;
    const char* file_contents = NULL;
    json_error_t error;

    int success = json_unpack_ex(data, &error, JSON_STRICT, "{s:i, s:s}","response_code",&code, "file_contents", &file_contents);
    if (success<0) {
        fputs("needy_file_request_response_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }

    needy_file_request_response* response = needy_file_request_response_new((response_code)code, file_contents);
    return response;
}
json_t* needy_file_request_response_serialize(const needy_file_request_response* this) {


    ENSURE_NOTNULL_MSG_RNULL(this, "needy_file_request_response_serialize: NULL pointer passed");

    json_error_t error;

    json_t* result = json_pack_ex(&error, 0, "{s:i, s:s}","response_code", (int)this->code, "file_contents", this->file_contents ? this->file_contents : "");
    if (!result) {
        fputs("needy_file_request_response_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}
void needy_file_request_response_destroy(needy_file_request_response* this) {
    ENSURE_NOTNULL(this);
    if (this->file_contents) {
        free(this->file_contents);
    }
    free(this);
}