//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_SERVER_ACK_H
#define NEEDYRESOURCES_SERVER_ACK_H
#include <jansson.h>
#include <unistd.h>
#include "private/needy-private.h"

typedef struct {
    pid_t pid;
    size_t response;
}needy_server_ack;

needy_server_ack* needy_server_ack_new(pid_t pid, size_t response);
needy_server_ack* needy_server_ack_deserialize(json_t* data);
json_t* needy_server_ack_serialize(const needy_server_ack* this);
void needy_server_ack_free(needy_server_ack* this);
#endif //NEEDYRESOURCES_SERVER_ACK_H
