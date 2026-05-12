//
// Created by vscode on 5/12/26.
//

#ifndef NEEDYRESOURCES_MQ_MANAGER_H
#define NEEDYRESOURCES_MQ_MANAGER_H
#include <needy.h>
typedef struct {
    mqd_t* queues;
    size_t queue_size;
    size_t active_count;
} MQManager;

MQManager* mq_manager_new(size_t size);
int mq_manager_add(MQManager *mgr, mqd_t mq);
int mq_manager_remove(MQManager *mgr, mqd_t mq);
void mq_manager_close_all(MQManager *mgr);
void mq_manager_destroy(MQManager* mgr);
#endif //NEEDYRESOURCES_MQ_MANAGER_H
