//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_MESSAGE_QUEUE_H
#define NEEDYRESOURCES_MESSAGE_QUEUE_H

#define SERVER_QUEUE_NAME "/needy_server_mq"
#define PROJECT_ID 'M'
#define QUEUE_PERMISSIONS 0660
#define MAX_MSG_SIZE 8192

void send_message(mqd_t server_mq, needy_message_t* msg);

#endif //NEEDYRESOURCES_MESSAGE_QUEUE_H
