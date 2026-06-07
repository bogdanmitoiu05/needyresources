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
#include "read_file_request.h"
#include "write_file_request.h"
#include "file_request_response.h"

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

#define RW_BUFF_SIZE 10
needy_message_t* read_write_buff[RW_BUFF_SIZE];
int rw_read_head = 0;
int rw_write_head = 0;
pthread_mutex_t rw_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rw_bufferHasItems = PTHREAD_COND_INITIALIZER;
pthread_cond_t rw_bufferCanBeFilled = PTHREAD_COND_INITIALIZER;

typedef enum {
    RW_SIMULTANEOUS,
    RW_READERS_PRIORITY,
    RW_WRITERS_PRIORITY
} rw_policy_t;

rw_policy_t rw_policy = RW_READERS_PRIORITY; // Default policy

typedef struct {
    pthread_mutex_t rw_lock;
    pthread_mutex_t read_count_lock;
    int read_count;
    // for writers priority
    pthread_mutex_t priority_mutex;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
    int waiting_writers;
    int active_writers;
} file_lock_t;

file_lock_t global_file_lock;

void reader_lock(file_lock_t* lock) {
    if (rw_policy == RW_SIMULTANEOUS) {
        // No lock access
        return;
    } else if (rw_policy == RW_READERS_PRIORITY) {
        pthread_mutex_lock(&lock->read_count_lock);
        lock->read_count++;
        if (lock->read_count == 1) {
            pthread_mutex_lock(&lock->rw_lock);
        }
        pthread_mutex_unlock(&lock->read_count_lock);
    } else if (rw_policy == RW_WRITERS_PRIORITY) {
        pthread_mutex_lock(&lock->priority_mutex);
        while (lock->waiting_writers > 0 || lock->active_writers > 0) {
            pthread_cond_wait(&lock->can_read, &lock->priority_mutex);
        }
        lock->read_count++;
        pthread_mutex_unlock(&lock->priority_mutex);
    }
}

void reader_unlock(file_lock_t* lock) {
    if (rw_policy == RW_SIMULTANEOUS) {
        // No lock access
        return;
    } else if (rw_policy == RW_READERS_PRIORITY) {
        pthread_mutex_lock(&lock->read_count_lock);
        lock->read_count--;
        if (lock->read_count == 0) {
            pthread_mutex_unlock(&lock->rw_lock);
        }
        pthread_mutex_unlock(&lock->read_count_lock);
    } else if (rw_policy == RW_WRITERS_PRIORITY) {
        pthread_mutex_lock(&lock->priority_mutex);
        lock->read_count--;
        if (lock->read_count == 0 && lock->waiting_writers > 0) {
            pthread_cond_signal(&lock->can_write);
        }
        pthread_mutex_unlock(&lock->priority_mutex);
    }
}

void writer_lock(file_lock_t* lock) {
    if (rw_policy == RW_SIMULTANEOUS) {
        // No lock access
        return;
    } else if (rw_policy == RW_READERS_PRIORITY) {
        pthread_mutex_lock(&lock->rw_lock);
    } else if (rw_policy == RW_WRITERS_PRIORITY) {
        pthread_mutex_lock(&lock->priority_mutex);
        lock->waiting_writers++;
        while (lock->read_count > 0 || lock->active_writers > 0) {
            pthread_cond_wait(&lock->can_write, &lock->priority_mutex);
        }
        lock->waiting_writers--;
        lock->active_writers = 1;
        pthread_mutex_unlock(&lock->priority_mutex);
    }
}

void writer_unlock(file_lock_t* lock) {
    if (rw_policy == RW_SIMULTANEOUS) {
        // No lock access
        return;
    } else if (rw_policy == RW_READERS_PRIORITY) {
        pthread_mutex_unlock(&lock->rw_lock);
    } else if (rw_policy == RW_WRITERS_PRIORITY) {
        pthread_mutex_lock(&lock->priority_mutex);
        lock->active_writers = 0;
        if (lock->waiting_writers > 0) {
            pthread_cond_signal(&lock->can_write);
        } else {
            pthread_cond_broadcast(&lock->can_read);
        }
        pthread_mutex_unlock(&lock->priority_mutex);
    }
}

typedef struct {
    needy_message_t* message;
    MQManager* mq_manager;
} thread_args_t;

void* reader_thread(void* args) {
    thread_args_t* thread_args = (thread_args_t*)args;
    needy_read_file_request* request = needy_read_file_request_deserialize(thread_args->message->payload);

    file_lock_t* lock = &global_file_lock;
    reader_lock(lock);

    // Critical section for reading
    printDbg("Reading file: %s", request->file_name);
    FILE* f = fopen(request->file_name, "r");
    char* contents = NULL;
    if (f) {
        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        fseek(f, 0, SEEK_SET);
        contents = malloc(length + 1);
        if (contents) {
            fread(contents, 1, length, f);
            contents[length] = '\0';
        }
        fclose(f);
    }

    needy_file_request_response* response = needy_file_request_response_new(OK, contents);
    needy_message_t* response_message = needy_message_new(FILE_RESPONSE, needy_file_request_response_serialize(response));
    send_message(findByPid(thread_args->mq_manager, request->pid), response_message);

    if (contents) {
        free(contents);
    }
    needy_file_request_response_destroy(response);
    needy_message_destroy(response_message);

    reader_unlock(lock);

    needy_read_file_request_destroy(request);
    free(thread_args);
    return NULL;
}

void* writer_thread(void* args) {
    thread_args_t* thread_args = (thread_args_t*)args;
    needy_write_file_request* request = needy_write_file_request_deserialize(thread_args->message->payload);

    file_lock_t* lock = &global_file_lock;
    writer_lock(lock);

    // Critical section for writing
    printDbg("Writing to file: %s", request->file_name);

    const char* mode_str = (request->mode == WRITE_MODE_APPEND) ? "a" : "w";
    FILE* f = fopen(request->file_name, mode_str);
    if (f) {
        if (request->content) {
            fwrite(request->content, 1, strlen(request->content), f);
        }
        fclose(f);
    } else {
        printErr("Failed to open file %s for writing", request->file_name);
    }


    writer_unlock(lock);

    needy_write_file_request_destroy(request);
    free(thread_args);
    return NULL;
}

void process_file_requests(MQManager* mq_manager) {
    pthread_t thread_pool[RW_BUFF_SIZE];
    int thread_count = 0;

    pthread_mutex_lock(&rw_mutex);
    for (int i = 0; i < RW_BUFF_SIZE; i++) {
        if (read_write_buff[i] != NULL) {
            thread_args_t* args = malloc(sizeof(thread_args_t));
            args->message = read_write_buff[i];
            args->mq_manager = mq_manager;

            if (args->message->message_type == READ_REQUEST) {
                pthread_create(&thread_pool[thread_count++], NULL, reader_thread, args);
            } else if (args->message->message_type == WRITE_REQUEST) {
                pthread_create(&thread_pool[thread_count++], NULL, writer_thread, args);
            }
            read_write_buff[i] = NULL;
        }
    }
    rw_read_head = 0;
    rw_write_head = 0;
    pthread_cond_broadcast(&rw_bufferCanBeFilled);
    pthread_mutex_unlock(&rw_mutex);

    for (int i = 0; i < thread_count; i++) {
        pthread_join(thread_pool[i], NULL);
    }
}


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
        if (message != NULL) {
            printDbg("Got: %s",needy_message_to_string(message));
        }
        else {
            printDbg("Got NULL");
        }
        share_buff[read_head] = NULL;
        pthread_cond_broadcast(&bufferCanBeFilled);
        if (message && shouldQuit) {
            needy_message_destroy(message);
        }
        read_head = (read_head+1)%BUFF_SIZE;
        pthread_mutex_unlock(&mutex_message);
        if (shouldQuit) {
            if (message) needy_message_destroy(message);
            return NULL;
        }
        if (!message) continue;

        switch (message->message_type) {
            case READ_REQUEST:
            case WRITE_REQUEST:
                pthread_mutex_lock(&rw_mutex);
                while(read_write_buff[rw_write_head] != NULL && !shouldQuit) {
                    pthread_cond_wait(&rw_bufferCanBeFilled, &rw_mutex);
                }
                if(shouldQuit) {
                    pthread_mutex_unlock(&rw_mutex);
                    needy_message_destroy(message);
                    break;
                }
                read_write_buff[rw_write_head] = message;
                rw_write_head = (rw_write_head + 1) % RW_BUFF_SIZE;

                if (rw_write_head == rw_read_head) { // Buffer is full
                    process_file_requests(mq_manager);
                }
                pthread_cond_broadcast(&rw_bufferHasItems);
                pthread_mutex_unlock(&rw_mutex);
                break;
            case CLIENT_CONNECTION_REQUEST:;
                needy_client_identification_header* header = needy_client_identification_header_deserialize(message->payload);
                if(!mq_manager_has_space(mq_manager)){
                    needy_server_ack* ack = needy_server_ack_new(header->pid, MAX_CLIENT_LIMIT_EXCEEDED, "Server has reached maximum capacity");
                    client_conn* new_conn = client_conn_new(header);
                    needy_message_t* ack_message = needy_message_new(SERVER_ACK, needy_server_ack_serialize(ack));
                    send_message(new_conn->queue, ack_message);

                    needy_message_destroy(ack_message);
                    needy_server_ack_destroy(ack);
                    client_conn_destroy(new_conn);
                    printDbg("noSpace");
                    break;
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
                needy_message_destroy(message);
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
                    needy_message_t* msg = needy_message_new(RESOURCE_RESPONSE, needy_resource_response_serialize(ack));
                    send_message(findByPid(mq_manager,request->pid),msg);

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
                needy_message_destroy(message);
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
                needy_message_destroy(message);
                break;
            default:
                printErr("ERROR: Invalid message received");
                needy_message_destroy(message);
                break;
        }
    }
    for (int i=0;i<BUFF_SIZE;i++) {
        if(share_buff[i]) free(share_buff[i]);
    }
    return NULL;
}
static server_config_t* conf = NULL;

void cleanup_locks() {
    pthread_mutex_destroy(&global_file_lock.rw_lock);
    pthread_mutex_destroy(&global_file_lock.read_count_lock);
    pthread_mutex_destroy(&global_file_lock.priority_mutex);
    pthread_cond_destroy(&global_file_lock.can_read);
    pthread_cond_destroy(&global_file_lock.can_write);
}

int main(int argc, char* const* argv)
{
    signal(SIGINT, terminate_handler);
    signal(SIGTERM, terminate_handler);

    int opt;

    bool version_flag = false;
    char* config_path = calloc(64, sizeof(char));
    strcpy(config_path, "/IdeaProjects/needyresources/files/config.json");

    bool stop_parse = false;
    while ((opt = getopt(argc, argv, "hvc:p:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'c':
                strcpy(config_path, optarg);
                break;
            case 'v':
                version_flag = true;
                stop_parse=true;
                break;
            case 'p':
                if (strcmp(optarg, "simultaneous") == 0) {
                    rw_policy = RW_SIMULTANEOUS;
                } else if (strcmp(optarg, "readers") == 0) {
                    rw_policy = RW_READERS_PRIORITY;
                } else if (strcmp(optarg, "writers") == 0) {
                    rw_policy = RW_WRITERS_PRIORITY;
                } else {
                    fprintf(stderr, "Invalid policy: %s. Available policies: simultaneous, readers, writers.\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-c /path/to/config.json] [-p <policy>]\n",
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

    pthread_mutex_init(&global_file_lock.rw_lock, NULL);
    pthread_mutex_init(&global_file_lock.read_count_lock, NULL);
    global_file_lock.read_count = 0;
    pthread_mutex_init(&global_file_lock.priority_mutex, NULL);
    pthread_cond_init(&global_file_lock.can_read, NULL);
    pthread_cond_init(&global_file_lock.can_write, NULL);
    global_file_lock.waiting_writers = 0;
    global_file_lock.active_writers = 0;

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
    pthread_cond_broadcast(&rw_bufferHasItems);
    pthread_join(th_send,NULL);
    printDbg("Sending stopped");
    printDbg("Server quitting");
    if (args)
        free(args);
    if (args1)
        free(args1);
quit_noargs:
    cleanup_locks();
    resource_manager_destroy(res_manager);
    mq_manager_close_all(mq_manager);
    mq_manager_destroy(mq_manager);
    mq_unlink(SERVER_QUEUE_NAME);
    free(config_path);
    server_config_destroy(conf);
    return 0;
}