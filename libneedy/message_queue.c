//
// Created by vscode on 5/8/26.
//

#include "needy.h"

void send_message(mqd_t server_mq, needy_message_t* msg)
{
    char* buffer = needy_message_to_string(msg);

    if (mq_send(server_mq, buffer, strlen(buffer) + 1, 0) == -1) {
        perror("Client: mq_send failed");
    }

    free(buffer);
    needy_message_destroy(msg);
}