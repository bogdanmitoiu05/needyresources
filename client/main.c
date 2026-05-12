#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <needy.h>
#include <stdbool.h>



int main(int argc, char* const* argv) {
    pid_t my_pid = getpid();

    int opt;
    bool version_flag = false;
    char* client_workspace_path = calloc(1024, sizeof(char));
    if (client_workspace_path == NULL) {
        fputs("Could not allocate workspace path", stderr);
        exit(EXIT_FAILURE);
    }
    snprintf(client_workspace_path, 1023, "workspace_%d", my_pid);
    size_t id = 0;

    bool stop_parse = false;
    size_t noResources = 2;
    while ((opt = getopt(argc, argv, "hvi:p:r:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'p':
                free(client_workspace_path);
                client_workspace_path = strdup(optarg);
                break;
            case 'i':
                id = strtol(optarg, NULL, 10);
                if (id == 0) {
                    fputs("Invalid ID specified",stderr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r':
                noResources = strtol(optarg, NULL, 10);
                if (id == 0) {
                    fputs("Resouruces no invalid",stderr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'v':
                version_flag = true;
                stop_parse=true;
                break;
            case 'h':
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i CLIENT_ID (must be at least 1)] [-p /path/to/workspace/folder] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }


    if (version_flag) {
        printf("Needy Resources, version %s\n.Proiect pt SO2\nVersiune protocol needy: %u\nProfesori coordonatori: Florin-Teodor Fortiș, Diogen Babuc\nEchipa:$Name\nMembri:\n1.Alexandru Turculeț\n2.Mitoiu Bogdan Mitoiu\n3.Timeea Tătărușanu",CLIENT_VERSION,NEEDY_PROTOCOL_VERSION);
        exit(EXIT_SUCCESS);
    }

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
    needy_client_identification_header* identification_header = needy_client_identification_header_new(my_pid, client_workspace_path);
    needy_message_t* msg = needy_message_new(CLIENT_CONNECTION_REQUEST,needy_client_identification_header_serialize(identification_header));
    send_message(server_mq, msg);
    needy_client_identification_header_destroy(identification_header);
    needy_message_destroy(msg);


    printf("Client [%d] requesting resources...\n", my_pid);
    needy_resource_request* request = needy_client_resource_request_new(my_pid, noResources);

    needy_message_t* requestMsg = needy_message_new(RESOURCE_REQUEST, needy_client_resource_request_serialize(request));
    send_message(server_mq, requestMsg);
    needy_client_resource_request_destroy(request);

    char receive_buffer[MAX_MSG_SIZE];
    printf("Client [%d] waiting for SERVER_ACK...\n", my_pid);

    ssize_t bytes_read = mq_receive(client_mq, receive_buffer, MAX_MSG_SIZE, NULL);
    if (bytes_read >= 0) {

        needy_message_t* receivedMessage = needy_message_from_string(receive_buffer);
        if (receivedMessage) {
            needy_server_ack* ack = needy_server_ack_deserialize(requestMsg->payload);

            printf("Client [%d] received ACK from server (Response Code: %zu)\n", my_pid, ack->response);

            needy_server_ack_destroy(ack);
        }
    } else {
        perror("Client: mq_receive failed");
    }

    // simulam
    // --- to be  changed ---
    printf("Client [%d] holding resources. Processing for 10 seconds...\n", my_pid);
    sleep(10);

    printf("Client [%d] processing complete. Releasing resources...\n", my_pid);
    needy_client_finalize* finalize = needy_client_finalize_new(my_pid);
    needy_message_t* msg_fin = needy_message_new(CLIENT_FINALIZE,needy_client_finalize_serialize(finalize));
    send_message(server_mq, msg_fin);
    needy_client_finalize_destroy(finalize);


    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(client_queue_name);
    free(client_workspace_path);
    //nu las cv prin sistem
    printf("Client [%d] shutting down safely.\n", my_pid);
    return 0;
}