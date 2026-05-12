//
// Created by vscode on 5/12/26.
//

#include "mq_manager.h"



MQManager* mq_manager_new(size_t size) {
    MQManager* mgr = new(MQManager);
    ENSURE_NOTNULL_MSG_RNULL(mgr,"mq_manager_new: could not allocate MQManager");
    mgr->queues = calloc(size, sizeof(mqd_t));
    mgr->queue_size = size;

    mgr->active_count = 0;
    for (size_t i = 0; i < mgr->queue_size; i++) {
        mgr->queues[i] = (mqd_t)-1;
    }
    return mgr;
}

int mq_manager_add(MQManager *mgr, mqd_t mq) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_add: mgr null", -1);
    if (mgr->active_count >= mgr->queue_size) {
        return -1;
    }

    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] == (mqd_t)-1) {
            mgr->queues[i] = mq;
            mgr->active_count++;
            return 0;
        }
    }
    return -1;
}

int mq_manager_remove(MQManager *mgr, mqd_t mq) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_remove: mgr null", -1);
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] == mq) {
            mgr->queues[i] = (mqd_t)-1;
            mgr->active_count--;
            return 0;
        }
    }
    return -1;
}

void mq_manager_close_all(MQManager *mgr) {
    ENSURE_NOTNULL_MSG_RETVAL(mgr, "mq_manager_close_all: mgr null",);
    for (size_t i = 0; i < mgr->queue_size; i++) {
        if (mgr->queues[i] != (mqd_t)-1) {
            mq_close(mgr->queues[i]);
            mgr->queues[i] = (mqd_t)-1;
        }
    }
    mgr->active_count = 0;

}
void mq_manager_destroy(MQManager* mgr) {
    ENSURE_NOTNULL(mgr);
    ENSURE_NOTNULL(mgr->queues);
    free(mgr->queues);
    free(mgr);
}