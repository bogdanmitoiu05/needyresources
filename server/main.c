#include <errno.h>
#include <stdio.h>
#include <needy.h>
#include <stdbool.h>
#include <needy.h>
#include <pthread.h>
#include <string.h>
#include "server_config.h"
#include "mq_manager.h"
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
//TODO
bool safeState(void) {
    return true;
}
//TODO
bool canAssign(void) {
    return true;
}
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
    char* config_path = strdup("config.json");

    bool stop_parse = false;
    while ((opt = getopt(argc, argv, "hvc:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'c':
                config_path = strdup(optarg);
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

    puts("Server started");


    char buffer[1024] = {0,};
    struct timespec timeout;
    timeout.tv_sec = 5;
    while (!shouldQuit && mq_manager->active_count < mq_manager->queue_size)
    {
        ssize_t b_received = mq_timedreceive(server_mq, buffer, 1023, NULL, &timeout);
        if (b_received < -1)
        {
            if (errno != ETIMEDOUT)
            {
                perror("Something went wrong while reading from message queue: ");
                errno = 0;
                continue;
            }
            break; //porneste procesarea

        }

        needy_message_t* message = needy_message_from_string(buffer);
        memset(buffer, 0, 1024);
        if (!message) continue;
        switch (message->message_type)
        {
            case CLIENT_CONNECTION_REQUEST:;
                needy_client_identification_header* header = needy_client_identification_header_deserialize(message->payload);
                if (!header)
                {
                    break; //eroare
                }

                break;
            case RESOURCE_REQUEST:;
                break;
            case CLIENT_FINALIZE:;
                break;
            default:
                break;
        }
        needy_message_destroy(message);

    }

    puts("Server quitting");
    mq_manager_close_all(mq_manager);
    mq_manager_destroy(mq_manager);
    mq_unlink(SERVER_QUEUE_NAME);
    free(config_path);
    server_config_destroy(conf);
    return 0;
}