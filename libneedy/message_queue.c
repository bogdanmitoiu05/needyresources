//
// Created by vscode on 5/8/26.
//

#include "needy.h"
#define printDbg(...){\
char __msg[1024];\
snprintf(__msg, sizeof(__msg), __VA_ARGS__);\
char __msg2[2000]; \
snprintf(__msg2, sizeof(__msg2),"[Server]: %s\n", __msg);\
fputs(__msg2, stdout);\
}
#define printErr(...) {\
char __msg[1024];\
snprintf(__msg, sizeof(__msg), __VA_ARGS__);\
char __msg2[2000]; \
snprintf(__msg2, sizeof(__msg2),"[Server]: %s\n", __msg);\
fputs(__msg2, stderr);\
}
void send_message(mqd_t server_mq, needy_message_t* msg)
{
    if (server_mq == -1)
        return;
    //transformă mesajul din obiect în string
    char* buffer = needy_message_to_string(msg);
    printDbg("Sending to %d: %s",server_mq, buffer);
    //trimite
    if (mq_send(server_mq, buffer, strlen(buffer) + 1/* atenție la \0 final*/, 0) == -1) {
        printErr("mq_send could not send to %d", server_mq);
        perror("Client: mq_send failed"); //error handling
    }

    //cleanup
    free(buffer);
    //needy_message_destroy(msg);
}