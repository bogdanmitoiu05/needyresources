//
// Created by vscode on 4/8/26.
//

#ifndef NEEDYRESOURCES_NEEDY_H
#define NEEDYRESOURCES_NEEDY_H

#define SERVER_KEY_PATHNAME "/tmp/mqueue_server_key"
#define PROJECT_ID 'M'
#define QUEUE_PERMISSIONS 0660
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <constants.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdlib.h>
#include "client_identification.h"
#include "resource_request.h"
#endif //NEEDYRESOURCES_NEEDY_H
