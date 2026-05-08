#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <jansson.h>
#include <needy.h>


#define MAX_MSG_SIZE 8192


int main(int argc, char* const* argv) {
    pid_t my_pid = getpid();
    char client_queue_name[64];
    snprintf(client_queue_name, sizeof(client_queue_name), "/needy_client_mq_%d", my_pid);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t client_mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0644, &attr);
    if (client_mq == (mqd_t)-1) {
        perror("Client: Failed to create client queue");
        exit(1);
    }

    mqd_t server_mq = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_mq == (mqd_t)-1) {
        perror("Client: Failed to open server queue");
        mq_unlink(client_queue_name);
        exit(1);
    }

    printf("Client %d started. Connecting to server...\n", my_pid);
    json_t* conn_payload = json_pack("{s:i, s:s}", "pid", my_pid, "queue", client_queue_name);
    needy_message_t* msg = needy_message_new(CLIENT_CONNECTION_REQUEST,conn_payload);
    send_message(server_mq, msg);

    printf("Client [%d] requesting resources...\n", my_pid);
    json_t* res_payload = json_pack("{s:i, s:s}", "pid", my_pid, "resources", "2");
    // -- vedem cum trimitem resursele
    needy_message_t* msg_req = needy_message_new(RESOURCE_REQUEST,conn_payload);
    send_message(server_mq, msg_req);

    char receive_buffer[MAX_MSG_SIZE];
    printf("Client [%d] waiting for SERVER_ACK...\n", my_pid);

    ssize_t bytes_read = mq_receive(client_mq, receive_buffer, MAX_MSG_SIZE, NULL);
    if (bytes_read >= 0) {

        json_error_t error;
        json_t *received_json = json_loads(receive_buffer, 0, &error);

        if (received_json) {

            json_t *payload_json = json_object_get(received_json, "payload");

            needy_server_ack* ack = needy_server_ack_deserialize(payload_json);

            printf("Client [%d] received ACK from server (Response Code: %zu)\n", my_pid, ack->response);

            needy_server_ack_free(ack);
            json_decref(received_json);
        }
    } else {
        perror("Client: mq_receive failed");
    }

    // simulam
    // --- to be  changed ---
    printf("Client %d holding resources. Processing for 10 seconds...\n", my_pid);
    sleep(10);

    printf("Client %d processing complete. Releasing resources...\n", my_pid);
    json_t* fin_payload = json_pack("{s:i}", "pid", my_pid);
    needy_message_t* msg_fin = needy_message_new(CLIENT_FINALIZE,fin_payload);
    send_message(server_mq, msg_fin);

    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(client_queue_name);
    //nu las cv prin sistem
    printf("Client [%d] shutting down safely.\n", my_pid);
    return 0;
}