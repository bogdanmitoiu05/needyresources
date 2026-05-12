//
// Created by vscode on 5/12/26.
//

#include "mq_manager.h"


client_conn* client_conn_new(needy_client_identification_header* clientInfo)
{
    client_conn* cConn = new(client_conn);
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 5;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;
    ENSURE_NOTNULL_MSG_RNULL(cConn, "client_conn_new: could not allocate new client connection object");

    char client_queue_name[64];
    snprintf(client_queue_name, sizeof(client_queue_name), "/needy_client_mq_%d", clientInfo->pid);

    mqd_t client_queue = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    if (client_queue < 0)
    {
        perror("client_conn_new could not open client pipe: ");
        return NULL;
    }
    cConn->queue = client_queue;
    cConn->clientInfo = clientInfo;
    return cConn;
}
mqd_t findByPid(MQManager* mgr,pid_t pid) {
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i]->clientInfo->pid==pid) {
            return mgr->queues[i]->queue;
        }
    }
}
void client_conn_destroy(client_conn* conn)
{
    ENSURE_NOTNULL(conn);
    mq_close(conn->queue);
    needy_client_identification_header_destroy(conn->clientInfo);
}
MQManager* mq_manager_new(size_t size) {
    MQManager* mgr = new(MQManager);
    ENSURE_NOTNULL_MSG_RNULL(mgr,"mq_manager_new: could not allocate MQManager");
    mgr->queues = calloc(size, sizeof(client_conn*));
    mgr->queue_size = size;

    mgr->active_count = 0;
    for (size_t i = 0; i < mgr->queue_size; i++) {

        mgr->queues[i] = NULL;
    }
    return mgr;
}

int mq_manager_add(MQManager *mgr, client_conn* mq) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_add: mgr null", -1);
    if (mgr->active_count >= mgr->queue_size) {
        return -1;
    }

    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] == NULL) {
            mgr->queues[i] = mq;
            mgr->active_count++;
            return 0;
        }
    }
    return -1;
}

int mq_manager_remove(MQManager *mgr, client_conn* mq) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_remove: mgr null", -1);
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] == mq) {
            client_conn_destroy(mgr->queues[i]);
            mgr->queues[i] = NULL;
            mgr->active_count--;
            return 0;
        }
    }
    return -1;
}

void mq_manager_close_all(MQManager *mgr) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_close_all: mgr null",);
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] != NULL) {
            client_conn_destroy(mgr->queues[i]);
            mgr->queues[i] = NULL;
        }
    }
    mgr->active_count = 0;

}
void mq_manager_destroy(MQManager* mgr) {
    ENSURE_NOTNULL(mgr);
    ENSURE_NOTNULL(mgr->queues);
    for (size_t i = 0; i < mgr->active_count; ++i)
    {
        if (mgr->queues[i] != NULL) {
            client_conn_destroy(mgr->queues[i]);
            mgr->queues[i] = NULL;
        }
    }
    free(mgr->queues);
    free(mgr);
}