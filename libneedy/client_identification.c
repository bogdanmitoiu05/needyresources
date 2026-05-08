//
// Created by vscode on 4/9/26.
//

#include "client_identification.h"
#include <needy-private.h>
#include <string.h>


needy_client_identification_header * needy_client_identification_header_new(pid_t pid, const char *workspace_path) {
    ENSURE_NOTNULL_MSG_RNULL(workspace_path,"Workspace path can't be empty");
    needy_client_identification_header* header = calloc(1, sizeof(needy_client_identification_header));
    ENSURE_NOTNULL_MSG_RNULL(header, "Could not allocate header");

    header->workspace_path=strdup(workspace_path);
    header->pid = pid;
    return header;

}

needy_client_identification_header * needy_client_identification_header_deserialize(json_t *data) {
    ENSURE_NOTNULL_RNULL(data);
    if (!json_is_object(data)) {
        fputs("Nu am primit obiect",stderr);
    }

    const char* key;
    json_t* value;

    char* workspace_path = NULL;
    pid_t pid = -1;
    json_object_foreach(data,key,value) {
        if (strcmp(key,"pid")==0) {
            if (!json_is_number(value)) {
                fputs("Am gasit pid, dar nu este de tip numar",stderr);
                return NULL;
            }
            pid = (pid_t) json_number_value(value);
        }
        if (strcmp(key,"workspace_path")==0) {
            if (!json_is_string(value)) {
                fputs("Am gasit pid, dar nu este de tip numar",stderr);
                return NULL;
            }
            workspace_path = strdup(json_string_value(value));
        }
    }
    if (pid == -1 || workspace_path == NULL) return NULL;
    needy_client_identification_header* res = needy_client_identification_header_new(pid,workspace_path);
    free(workspace_path);
    return res;
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
