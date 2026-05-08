//
// Created by vscode on 4/9/26.
//

#ifndef NEEDYRESOURCES_REQUEST_H
#define NEEDYRESOURCES_REQUEST_H
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <unistd.h>
#include <jansson.h>
#include <sys/types.h>
#include <string.h>

typedef struct {
    pid_t pid;
    char* workspace_path;
} needy_client_identification_header;

needy_client_identification_header* nr_client_identification_header_new(pid_t pid, const char* workspace_path);
needy_client_identification_header* nr_client_identification_header_deserialize(json_t* data);

json_t *nr_client_identification_header_serialize(needy_client_identification_header *this);
void nr_client_identification_header_destroy(needy_client_identification_header* this);

#endif //NEEDYRESOURCES_REQUEST_H
