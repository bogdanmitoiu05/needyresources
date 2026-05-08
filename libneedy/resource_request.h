//
// Created by vscode on 4/9/26.
//

#ifndef NEEDYRESOURCES_RESOURCE_REQUEST_H
#define NEEDYRESOURCES_RESOURCE_REQUEST_H
#include <needy-private.h>
#include <unistd.h>
#include <jansson.h>
#include <sys/types.h>
typedef struct {
    pid_t pid;
    size_t requestedResources;
} nr_client_resource_request;

nr_client_resource_request* nr_client_resource_request_new(pid_t pid, size_t requestedResources);
nr_client_resource_request* nr_client_resource_request_deserialize(json_t* data);
json_t* nr_client_resource_request_serialize(const nr_client_resource_request* this);
void nr_client_resource_request_destroy(nr_client_resource_request* this);
#endif //NEEDYRESOURCES_RESOURCE_REQUEST_H
