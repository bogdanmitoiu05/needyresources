#include <errno.h>
#include <stdio.h>
#include <needy.h>
#include <stdbool.h>
#include <needy.h>
#include <pthread.h>
#include <string.h>
#include "server_config.h"
#include "mq_manager.h"
#include "resource_manager.h"
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
        perror("Client: Failed to open server queue");
        mq_unlink(SERVER_QUEUE_NAME);
        exit(EXIT_FAILURE);
    }
    MQManager* mq_manager = mq_manager_new(conf->maximumClients);
    resource_manager* res_manager = resource_manager_new(mq_manager, conf->maximumAllowedResources);
    //printf(conf->workingDirectory)
    if ( resource_manager_index(res_manager, conf->workingDirectory) < 0)
    {
        goto quit;
    }
    //printf("hello\n");
    puts("Server started");
    puts("Server is listenin");

    char buffer[MAX_MSG_SIZE] = {0,};

    int nr=0;
    while (!shouldQuit && mq_manager->active_count < mq_manager->queue_size && nr<3)
    {
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
        printf("Received message: %s\n", buffer);
        //printf("%s\n",message->message_type);
        memset(buffer, 0, MAX_MSG_SIZE);
        if (!message) continue;
        switch (message->message_type)
        {
            case CLIENT_CONNECTION_REQUEST:;
                needy_client_identification_header* header = needy_client_identification_header_deserialize(message->payload);
                if (!header)
                {
                    break; //eroare
                }
                printf("got conn req\n");
                client_conn* new_conn = client_conn_new(header);
                mq_manager_add(mq_manager, new_conn);
                break;
            case RESOURCE_REQUEST:;
                needy_resource_request* request = needy_client_resource_request_deserialize(message->payload);
                if (!request)
                {
                    break;
                }
                printf("got res req\n");
                resource_manager_add_request(res_manager,request);
                needy_resource_response_t* response = resource_manager_step(res_manager);
                needy_message_t* msg = needy_message_new(RESOURCE_RESPONSE,needy_resource_response_serialize(response));

                send_message(findByPid(mq_manager,request->pid),msg);

                needy_client_resource_request_destroy(request);
                nr++;
                break;
            case CLIENT_FINALIZE:;
                printf("got fin req\n");
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
        needy_message_destroy(message);
    }

quit:
    puts("Server quitting");
    resource_manager_destroy(res_manager);
    mq_manager_close_all(mq_manager);
    mq_manager_destroy(mq_manager);
    mq_unlink(SERVER_QUEUE_NAME);
    free(config_path);
    server_config_destroy(conf);
    return 0;
}