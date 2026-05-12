//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_MESSAGE_QUEUE_H
#define NEEDYRESOURCES_MESSAGE_QUEUE_H
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <constants.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdlib.h>
#include <mqueue.h>
#define SERVER_QUEUE_NAME "/needy_server_mq"
#define PROJECT_ID 'M'
#define QUEUE_PERMISSIONS 0660
#define MAX_MSG_SIZE 8192

/**
 * Funcție helper pentru a trimite mesaje. Execută transformarea în string din AST-ul JSON și curățarea bufferului temporar
 * @param server_mq coada de transmisie
 * @param msg mesajul de transmis
 */
void send_message(mqd_t server_mq, needy_message_t* msg);

#endif //NEEDYRESOURCES_MESSAGE_QUEUE_H
