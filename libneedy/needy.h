//
// Created by vscode on 4/8/26.
//

#ifndef NEEDYRESOURCES_NEEDY_H
#define NEEDYRESOURCES_NEEDY_H


#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <constants.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdlib.h>
#include <mqueue.h>
#include "client_identification.h"
#include "resource_request.h"
#include "client_finalize.h"
#include "message.h"
#include "message_type.h"
#include "server_ack.h"
#include "message_queue.h"

#endif //NEEDYRESOURCES_NEEDY_H
