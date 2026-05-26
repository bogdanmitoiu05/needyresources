#include <errno.h>
#include <stdio.h>
#include <needy.h>
#include <stdbool.h>
#include <needy.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include "message.h"
#include "message_queue.h"
#include "message_type.h"
#include "resource_response.h"
#include "server_ack.h"
#include "server_config.h"
#include "mq_manager.h"
#include "resource_manager.h"
#include <response_codes.h>
/**
 * Sistemul se va baza pe o coadă de mesaje ce va conține toate cererile pt procesare
 *
 * Dpdv al arhitecturii, serverul este capabil de a se adapta în timp real la ANUMITE evenimente externe.
 * În particular, serverul își va putea actualiza dinamic lista de fișiere aflate în gestiune prin interogarea repetată
 * a sistemului de fișiere la intervale regulate.
 *
 * Sistemul va acționa, după caz:
 * 1. Dacă un fișier este adăugat în directorul de lucru, programul va vedea și își va actualiza baza sa de resurse
 * 2. Dacă un fișier este eliminat din directorul de lucru, programul va opri clientul ce deține fișierul și va curăța încă odată fișierul în cazul în care mai apare ca
 * urmare a unor operații de scriere din cadrul procesului client.
 *
 * Evident, acest lucru nu protejează împotriva unor potențiale pierderi de date dacă fișierul este șters și readăugat între momentul de actualizare a bazei de resurse și finalizarea
 * curățării.
 */
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

volatile bool shouldQuit = false;
void terminate_handler(__attribute_maybe_unused__ int signal) {
    shouldQuit = true;
    printDbg("quit received");
}
pthread_t th_receive,th_send;
typedef struct {
    MQManager *mq_manager;
    mqd_t server_mq;
} recv_args_t;
typedef struct {
    MQManager *mq_manager;
    resource_manager* res_manager;
} send_args_t;
#define BUFF_SIZE 12
needy_message_t* share_buff[BUFF_SIZE];
int read_head=0;
int write_head=0;
pthread_mutex_t mutex_message = PTHREAD_MUTEX_INITIALIZER;
int nr=0;
pthread_cond_t bufferHasItems = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferCanBeFilled = PTHREAD_COND_INITIALIZER;



void* func_receive(void* args) {
    recv_args_t* recv_args = args;
    MQManager* mq_manager = recv_args->mq_manager;
    mqd_t server_mq = recv_args->server_mq;
    char buffer[MAX_MSG_SIZE] = {0,};

    int idle_seconds = 0;

    while (!shouldQuit) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 1;
        ssize_t b_received = mq_timedreceive(server_mq, buffer, MAX_MSG_SIZE, NULL, &timeout);
        if (b_received < 0)
        {
            if (errno == ETIMEDOUT)
            {
                printDbg("time");

                if (shouldQuit)
                    break;
                if (mq_manager->active_count == 0) {
                    idle_seconds++;
                    printDbg("No clients connected. Idle for %d second(s)...", idle_seconds);

                    if (idle_seconds >= 5) {
                        printDbg("5 seconds of inactivity reached. Shutting down.");
                        shouldQuit = true;
                        break;
                    }
                } else {
                    idle_seconds = 0;
                }
                errno = 0;
                continue;
            }
            perror("Something went wrong while reading from message queue: ");
            break; //porneste procesarea

        }
        idle_seconds = 0;
        needy_message_t* message = needy_message_from_string(buffer);
        printDbg("[SERVER] Received message: %s", buffer);
        ///printDbg("%s",message->message_type);
        memset(buffer, 0, MAX_MSG_SIZE);
        if (!message) {
            fprintf(stderr,"[SERVER] Received NULL message");
            continue;
        }

        pthread_mutex_lock(&mutex_message);

        while (share_buff[write_head] != NULL && !shouldQuit) {
            pthread_cond_wait(&bufferCanBeFilled, &mutex_message);
        }
        if (shouldQuit) {
            needy_message_destroy(message);
            pthread_mutex_unlock(&mutex_message);
            return NULL;
        }
        share_buff[write_head] = message;
        write_head=(write_head+1)%BUFF_SIZE;
        pthread_cond_broadcast(&bufferHasItems);
        pthread_mutex_unlock(&mutex_message);
    }
    return NULL;
}

void* func_send(void* args) {
    send_args_t* send_args = (send_args_t*)args;
    MQManager* mq_manager = send_args->mq_manager;
    resource_manager* res_manager = send_args->res_manager;
    while (!shouldQuit) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // 1-second timeout



        pthread_mutex_lock(&mutex_message);
        while (share_buff[read_head] == NULL && !shouldQuit) {
            pthread_cond_wait(&bufferHasItems, &mutex_message);
        }

        needy_message_t* message = share_buff[read_head];
        share_buff[read_head] = NULL;
        pthread_cond_broadcast(&bufferCanBeFilled);
        if (message && shouldQuit) {
            needy_message_destroy(message);
        }
        read_head = (read_head+1)%BUFF_SIZE;
        pthread_mutex_unlock(&mutex_message);
        if (shouldQuit) {
            return NULL;
        }
        printDbg("Got message");
        switch (message->message_type) {
            case CLIENT_CONNECTION_REQUEST:;
                needy_client_identification_header* header = needy_client_identification_header_deserialize(message->payload);
                if(!mq_manager_has_space(mq_manager)){
                    needy_server_ack* ack = needy_server_ack_new(header->pid, MAX_CLIENT_LIMIT_EXCEEDED, "Server has reached maximum capacity");
                    client_conn* new_conn = client_conn_new(header);
                    needy_message_t* ack_message = needy_message_new(SERVER_ACK, needy_server_ack_serialize(ack));
                    send_message(new_conn->queue, message);

                    needy_message_destroy(ack_message);
                    needy_server_ack_destroy(ack);
                    client_conn_destroy(new_conn);
                }

                if (!header)
                {
                    break; //eroare
                }

                //printDbg("got conn req");
                client_conn* new_conn = client_conn_new(header);
                mq_manager_add(mq_manager, new_conn);

                needy_server_ack* ack = needy_server_ack_new(header->pid, OK, "");
                needy_message_t* ack_message = needy_message_new(SERVER_ACK, needy_server_ack_serialize(ack));
                printDbg("ack");
                send_message(new_conn->queue, ack_message);

                needy_message_destroy(ack_message);
                needy_server_ack_destroy(ack);
                break;
            case RESOURCE_REQUEST:;
                printDbg("rr");
                needy_resource_request* request = needy_client_resource_request_deserialize(message->payload);
                if (!request)
                {
                    break;
                }

                if(request->noResources != res_manager->resource_type_count){ // verifica dimensiunea vectorului sa fie conforma cu ce se asteapta serverul
                    needy_resource_response_t* ack = needy_resource_response_new(INCORRECT_NUMBER_OF_RESOURCES, 0, NULL);
                    needy_message_t* message = needy_message_new(RESOURCE_RESPONSE, needy_resource_response_serialize(ack));
                    send_message(findByPid(mq_manager,request->pid),message);

                    needy_client_resource_request_destroy(request);
                    needy_resource_response_destroy(ack);
                }
                //printDbg("got res req");
                int res = resource_manager_add_request(res_manager,request);
                needy_resource_response_t* response;
                if (res == -2) {
                    response = needy_resource_response_new(MAX_CLIENT_LIMIT_EXCEEDED, 0, NULL);
                }
                if (res == 1) {
                    response = needy_resource_response_new(MAX_RESOURCE_NEED_REGISTERED, 0, NULL);
                }
                else if (res == 2) {
                    response = needy_resource_response_new(RESOURCE_LIMIT_EXCEEDED, 0, NULL);
                }
                else {
                    response = resource_manager_step(res_manager);
                }

                needy_message_t* msg = needy_message_new(RESOURCE_RESPONSE,needy_resource_response_serialize(response));
                send_message(findByPid(mq_manager,request->pid), msg);

                break;
            case CLIENT_FINALIZE:;
                printDbg("cf");
                //printDbg("got fin req");
                needy_client_finalize* finalizeMsg = needy_client_finalize_deserialize(message->payload);
                if (!finalizeMsg)
                {
                    break;
                }
                resource_manager_release(res_manager, finalizeMsg);
                needy_client_finalize_destroy(finalizeMsg);
                break;
            default:
                printErr("ERROR: Invalid message received");
                break;
        }
    }
    for (int i=0;i<BUFF_SIZE;i++) {
        free(share_buff[i]);
    }
    return NULL;
}
static server_config_t* conf = NULL;
int main(int argc, char* const* argv)
{
    signal(SIGINT, terminate_handler);
    signal(SIGTERM, terminate_handler);

    int opt;

    bool version_flag = false;
    char* config_path = calloc(64, sizeof(char));
    strcpy(config_path, "/IdeaProjects/needyresources/files/config.json");

    bool stop_parse = false;
    while ((opt = getopt(argc, argv, "hvc:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'c':
                strcpy(config_path, optarg);
                break;
            case 'v':
                version_flag = true;
                stop_parse=true;
                break;
            case 'h':
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-c /path/to/config.json] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }


    if (version_flag) {
        printDbg("Needy Resources, version %s\n.Proiect pt SO2\nVersiune protocol needy: %u\nProfesori coordonatori: Florin-Teodor Fortiș, Diogen Babuc\nEchipa:$Name\nMembri:\n1.Alexandru Turculeț\n2.Mitoiu Bogdan Mitoiu\n3.Timeea Tătărușanu",SERVER_VERSION,NEEDY_PROTOCOL_VERSION);
        exit(EXIT_SUCCESS);
    }

    conf = load_from_file(config_path);
    if (conf == NULL)
    {
        exit(EXIT_FAILURE);
    }
    /***
     * Vom procesa toate cereile ce vin pe coada de mesaje aferentă serverului.
     *
     * Când coada este goală, serverul va rula algoritmul bancherului și va onora, dacă este posibil,cererile astfel încât să nu se creeze
     * o stare de impas. Dacă acest lucru nu este posibil, se va transmite la toate procesele că cererile nu pot fi onorate și programul se va închide.
     */
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = (long) conf->maximumClients;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t server_mq = mq_open(SERVER_QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (server_mq == (mqd_t)-1) {
        perror("Server: Failed to open server queue");
        mq_unlink(SERVER_QUEUE_NAME);
        exit(EXIT_FAILURE);
    }
    MQManager* mq_manager = mq_manager_new(conf->maximumClients);
    resource_manager* res_manager = resource_manager_new(mq_manager, conf->maximumAllowedResources, conf->typesOfResources);
    for(size_t i = 0; i < conf->typesOfResources; ++i){
        if ( resource_manager_index(res_manager, conf->workingDirectory,i) < 0)//loop index folders
        {
            printErr("Failed to index resources");
            goto quit_noargs;
        }
    }
    //printDbg("hello");
    printDbg("Server started");
    printDbg("Server is listening");
    recv_args_t* args = new(send_args_t);
    if (args == NULL) {
        perror("malloc failed recv_args");
        goto quit_noargs;
    }
    args->server_mq = server_mq;
    args->mq_manager = mq_manager;

    send_args_t* args1 = new(send_args_t);
    if (args1 == NULL) {
        perror("malloc failed send_args");
        free(args);
        goto quit_noargs;
    }
    args1->res_manager = res_manager;
    args1->mq_manager = mq_manager;
    pthread_create(&th_receive,NULL,func_receive,(void*)args);
    pthread_create(&th_send,NULL,func_send,(void*)args1);

    pthread_join(th_receive,NULL);
    printDbg("Receive stopped");
    pthread_cond_broadcast(&bufferHasItems);
    pthread_join(th_send,NULL);
    printDbg("Sending stopped");
    printDbg("Server quitting");
    if (args)
        free(args);
    if (args1)
        free(args1);
quit_noargs:
    resource_manager_destroy(res_manager);
    mq_manager_close_all(mq_manager);
    mq_manager_destroy(mq_manager);
    mq_unlink(SERVER_QUEUE_NAME);
    free(config_path);
    server_config_destroy(conf);
    return 0;
}