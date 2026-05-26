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
volatile bool shouldQuit = false;
void terminate_handler(__attribute_maybe_unused__ int signal) {
    shouldQuit = true;
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
int buf_in=0;
int buf_out=0;
sem_t sem_full,sem_gol;
pthread_mutex_t mutex_message;
int nr=0;

void* func_receive(void* args) {
    recv_args_t* recv_args = args;
    MQManager* mq_manager = recv_args->mq_manager;
    mqd_t server_mq = recv_args->server_mq;
    char buffer[MAX_MSG_SIZE] = {0,};

    while (!shouldQuit && mq_manager->active_count < mq_manager->queue_size && nr<3) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5;
        ssize_t b_received = mq_timedreceive(server_mq, buffer, MAX_MSG_SIZE, NULL, &timeout);
        if (b_received < 0)
        {
            if (errno == ETIMEDOUT)
            {
                printf("time\n");
                errno = 0;
                continue;
            }
            perror("Something went wrong while reading from message queue: ");
            break; //porneste procesarea

        }
        needy_message_t* message = needy_message_from_string(buffer);
        printf("[SERVER] Received message: %s\n", buffer);
        ///printf("%s\n",message->message_type);
        memset(buffer, 0, MAX_MSG_SIZE);
        if (!message) continue;
        sem_wait(&sem_gol);
        pthread_mutex_lock(&mutex_message);
        share_buff[buf_in] = message;
        buf_in=(buf_in+1)%BUFF_SIZE;
        pthread_mutex_unlock(&mutex_message);
        sem_post(&sem_full);
    }
    free(args);
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

        if (sem_timedwait(&sem_full, &ts) == -1) {
            if (errno == ETIMEDOUT) continue;
            break;
        }

        pthread_mutex_lock(&mutex_message);
        needy_message_t* message = share_buff[buf_out];
        buf_out = (buf_out+1)%BUFF_SIZE;
        pthread_mutex_unlock(&mutex_message);
        sem_post(&sem_gol);
        switch (message->message_type)
        {
            case CLIENT_CONNECTION_REQUEST:;
                needy_client_identification_header* header = needy_client_identification_header_deserialize(message->payload);
                if(!mq_manager_has_space(mq_manager)){
                    needy_server_ack* ack = needy_server_ack_new(header->pid, MAX_CLIENT_LIMIT_EXCEEDED, "Server has reached maximum capacity");
                    client_conn* new_conn = client_conn_new(header);
                    needy_message_t* message = needy_message_new(SERVER_ACK, needy_server_ack_serialize(ack));
                    send_message(new_conn->queue, message);

                    needy_message_destroy(message);
                    needy_server_ack_destroy(ack);
                    client_conn_destroy(new_conn);
                }

                if (!header)
                {
                    break; //eroare
                }

                //printf("got conn req\n");
                client_conn* new_conn = client_conn_new(header);
                mq_manager_add(mq_manager, new_conn);

                needy_server_ack* ack = needy_server_ack_new(header->pid, OK, "");
                needy_message_t* message = needy_message_new(SERVER_ACK, needy_server_ack_serialize(ack));
                send_message(new_conn->queue, message);

                needy_message_destroy(message);
                needy_server_ack_destroy(ack);
                break;
            case RESOURCE_REQUEST:;
                needy_resource_request* request = needy_client_resource_request_deserialize(message->payload);
                if (!request)
                {
                    break;
                }

                if(request->noResources != res_manager->nr_resource_type){ // verifica dimensiunea vectorului sa fie conforma cu ce se asteapta serverul
                    char buff[1024]={0,};
                    needy_resource_response_t* ack = needy_resource_response_new(INCORRECT_NUMBER_OF_RESOURCES, 0, NULL);
                    needy_message_t* message = needy_message_new(RESOURCE_RESPONSE, needy_resource_response_serialize(ack));
                    send_message(findByPid(mq_manager,request->pid),message);

                    needy_client_resource_request_destroy(request);
                    needy_resource_response_destroy(ack);
                }
                //printf("got res req\n");
                resource_manager_add_request(res_manager,request);
                needy_resource_response_t* response = resource_manager_step(res_manager);
                needy_message_t* msg = needy_message_new(RESOURCE_RESPONSE,needy_resource_response_serialize(response));
                break;
            case CLIENT_FINALIZE:;
                //printf("got fin req\n");
                needy_client_finalize* finalizeMsg = needy_client_finalize_deserialize(message->payload);
                if (!finalizeMsg)
                {
                    break;
                }
                resource_manager_release(res_manager, finalizeMsg);
                needy_client_finalize_destroy(finalizeMsg);
                break;
            default:
                fputs("ERROR: Invalid message received", stderr);
                break;
        }
        //needy_message_destroy(message);
    }
    for (int i=0;i<BUFF_SIZE;i++) {
        free(share_buff[i]);
    }
    return NULL;
}
void init_threads(pthread_attr_t* attr_send) {

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
        printf("Needy Resources, version %s\n.Proiect pt SO2\nVersiune protocol needy: %u\nProfesori coordonatori: Florin-Teodor Fortiș, Diogen Babuc\nEchipa:$Name\nMembri:\n1.Alexandru Turculeț\n2.Mitoiu Bogdan Mitoiu\n3.Timeea Tătărușanu",SERVER_VERSION,NEEDY_PROTOCOL_VERSION);
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
            fputs("Failed to index resources",stderr);
            goto quit_noargs;
        }
    }
    //printf("hello\n");
    puts("Server started");
    puts("Server is listenin");
    pthread_mutex_init(&mutex_message, NULL);
    sem_init(&sem_gol, 0, BUFF_SIZE);
    sem_init(&sem_full, 0, 0);
    recv_args_t* args = NULL;
    args = new(send_args_t);
    if (args == NULL) {
        perror("malloc failed recv_args");
        goto quit;
    }
    args->server_mq = server_mq;
    args->mq_manager = mq_manager;

    send_args_t* args1 = NULL;
    args1 = new(send_args_t);
    if (args1 == NULL) {
        perror("malloc failed send_args");
        goto quit;
    }
    args1->res_manager = res_manager;
    args1->mq_manager = mq_manager;
    pthread_create(&th_receive,NULL,func_receive,(void*)args);
    pthread_create(&th_send,NULL,func_send,(void*)args1);

    pthread_join(th_receive,NULL);
    pthread_join(th_send,NULL);



quit:
    puts("Server quitting");
    if (args)
        free(args);
    if (args1)
        free(args1);
quit_noargs:
    sem_destroy(&sem_gol);
    sem_destroy(&sem_full);
    pthread_mutex_destroy(&mutex_message);
    resource_manager_destroy(res_manager);
    mq_manager_close_all(mq_manager);
    mq_manager_destroy(mq_manager);
    mq_unlink(SERVER_QUEUE_NAME);
    free(config_path);
    server_config_destroy(conf);
    return 0;
}