//
// Created by vscode on 4/9/26.
//

#include "client_identification.h"
#include <utils.h>
#include <string.h>


needy_client_identification_header * needy_client_identification_header_new(pid_t pid, const char *workspace_path) {
    ENSURE_NOTNULL_MSG_RNULL(workspace_path,"Workspace path can't be empty");
    needy_client_identification_header* header = calloc(1, sizeof(needy_client_identification_header));
    ENSURE_NOTNULL_MSG_RNULL(header, "Could not allocate header");

    header->workspace_path=strdup(workspace_path);
    header->pid = pid;
    return header;

}

needy_client_identification_header * needy_client_identification_header_deserialize(json_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: msg is NULL");
    needy_client_identification_header* result = calloc(1, sizeof(needy_client_identification_header));
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: could not allocate message");
    json_error_t error;
    int success = json_unpack_ex(msg, &error, JSON_STRICT, "{s:i,s:s}", "pid",&(result->pid), "workspace_path",result->workspace_path);
    if (success<0) {
        JSON_ERROR_PRINTF(error);
        free(result);
        return NULL;
    }
    return result;
}

json_t *needy_client_identification_header_serialize(needy_client_identification_header *this) {
    ENSURE_NOTNULL_RNULL(this);
    json_t* obj = json_pack("{s:n, s:s}","pid",this->pid,"workspace_path",this->workspace_path);
    return obj;
}

void needy_client_identification_header_destroy(needy_client_identification_header *this) {
    ENSURE_NOTNULL(this);
    if (this->workspace_path) {
        free(this->workspace_path);
    }
    free(this);
}
