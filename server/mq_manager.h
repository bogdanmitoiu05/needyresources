//
// Created by vscode on 5/12/26.
//

#ifndef NEEDYRESOURCES_MQ_MANAGER_H
#define NEEDYRESOURCES_MQ_MANAGER_H
#include <needy.h>

typedef struct
{
    mqd_t queue;
    needy_client_identification_header* clientInfo;
} client_conn;
typedef struct {
    client_conn** queues;
    size_t queue_size;
    size_t active_count;
} MQManager;

mqd_t findByPid(MQManager* mgr,pid_t pid);
client_conn* client_conn_new(needy_client_identification_header* clientInfo);
void client_conn_destroy(client_conn* conn);
MQManager* mq_manager_new(size_t size);
int mq_manager_add(MQManager *mgr, client_conn* conn);
int mq_manager_remove(MQManager *mgr, client_conn* conn);
void mq_manager_close_all(MQManager *mgr);
void mq_manager_destroy(MQManager* mgr);
#endif //NEEDYRESOURCES_MQ_MANAGER_H
