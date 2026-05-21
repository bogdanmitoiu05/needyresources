//
// Created by vscode on 5/12/26.
//

#include "mq_manager.h"
#include "utils.h"
#include <stdbool.h>


client_conn* client_conn_new(needy_client_identification_header* clientInfo)
{
    client_conn* cConn = new(client_conn); //instanțiere
    struct mq_attr attr; // atributele cozii de mesaje
    attr.mq_flags = 0; // fără flags
    attr.mq_maxmsg = 5; // max. 5 mesaje
    attr.mq_msgsize = MAX_MSG_SIZE; // fiecare mesaj max MAX_MSG_SIZE octeți
    attr.mq_curmsgs = 0;
    ENSURE_NOTNULL_MSG_RNULL(cConn, "client_conn_new: could not allocate new client connection object");

    char client_queue_name[64]; //coada clientului cu PID x este /needy_client_mq_$x
    snprintf(client_queue_name, sizeof(client_queue_name), "/needy_client_mq_%d", clientInfo->pid);

    mqd_t client_queue = mq_open(client_queue_name, O_CREAT | O_WRONLY, 0644, &attr); //deschide coada procesului pentru scriere
    if (client_queue < 0) // eroare
    {
        perror("client_conn_new could not open client pipe: ");
        return NULL;
    }
    cConn->queue = client_queue;
    cConn->clientInfo = clientInfo;
    return cConn;
}
mqd_t findByPid(MQManager* mgr,pid_t pid) {
    for (size_t i = 0; i < mgr->queue_size; i++) { // caută prima (și singura) coadă coresp. PID-ului procesului
        if (mgr->queues[i]->clientInfo->pid==pid) {
            return mgr->queues[i]->queue;
        }
    }
}
void client_conn_destroy(client_conn* conn)
{
    ENSURE_NOTNULL(conn); // nu apela free pe null
    mq_close(conn->queue);
    needy_client_identification_header_destroy(conn->clientInfo);
}
MQManager* mq_manager_new(size_t size) {
    MQManager* mgr = new(MQManager); //instanțiere
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
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_add: mgr null", -1); //null guard
    if (mgr->active_count >= mgr->queue_size) {
        return -1;
    }

    for (size_t i = 0; i < mgr->queue_size; i++) { // introduce conexiunea în primul loc liber din vector
        if (mgr->queues[i] == NULL) {
            mgr->queues[i] = mq;
            mgr->active_count++;
            return 0;
        }
    }
    return -1;
}

int mq_manager_remove(MQManager *mgr, client_conn* mq) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_remove: mgr null", -1); //null guard
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] == mq) { //găsește procesul și ștergere-l
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
bool mq_manager_has_space(MQManager* mgr){
    ENSURE_NOTNULL_FULL(mgr, false);
    return mgr->active_count < mgr->queue_size;
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