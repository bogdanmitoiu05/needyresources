//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_CLIENT_FINALIZE_H
#define NEEDYRESOURCES_CLIENT_FINALIZE_H
#include <unistd.h>
#include "utils.h"

typedef struct {
    pid_t pid;
}needy_client_finalize;

needy_client_finalize* needy_client_finalize_new(pid_t pid);
needy_client_finalize* needy_client_finalize_deserialize(json_t* data);
json_t* needy_client_finalize_serialize(const needy_client_finalize* this);
void needy_client_finalize_destroy(needy_client_finalize* this);
#endif //NEEDYRESOURCES_CLIENT_FINALIZE_H
