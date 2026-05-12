#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <needy.h>
#include <stdbool.h>


/***
 * Fiecare client se identifică prin intermediul ID-ului de proces (PID)
 *
 * Implicit, fiecare client va avea "spațiul de manevră" într-un folder "workspace_$PID"
 */
int main(int argc, char* const* argv) {
    pid_t my_pid = getpid();


    // Începem parsarea argumentelor din linia de comandă
    int opt;
    bool version_flag = false; //dacă vedem -v, afișăm autorii și ne oprim
    char* client_workspace_path = calloc(1024, sizeof(char)); // inițializează calea de workspace
    if (client_workspace_path == NULL) {
        fputs("Could not allocate workspace path", stderr);
        exit(EXIT_FAILURE);
    }


    snprintf(client_workspace_path, 1023, "workspace_%d", my_pid); //interpolează PID-ul cu șirul de caractere
    size_t id = 0; // variabilă ajutătoare pentru a defini clienții/procesele

    bool stop_parse = false; //dacă am întâlnit un argument ce determină oprirea: -h sau -v
    size_t noResources = 2; // implicit clientul va solicita 2 resurse
    while ((opt = getopt(argc, argv, "hvi:p:r:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'p': //calea către workspace
                free(client_workspace_path);
                client_workspace_path = strdup(optarg);
                break;
            case 'i': //id
                id = strtol(optarg, NULL, 10); //conversie în baza 10
                if (id == 0) { //functia va returna 0 dacă întâlnește un șir de intrare ce nu reprezintă un număr
                    fputs("Invalid ID specified",stderr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r': //nr. resurse de cerut
                noResources = strtol(optarg, NULL, 10); // vezi mai sus
                if (id == 0) {
                    fputs("Resouruces no invalid",stderr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'v': //versiune
                version_flag = true;
                stop_parse=true;
                break;
            case 'h':  //help/eroare
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i CLIENT_ID (must be at least 1)] [-p /path/to/workspace/folder] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // dacă s-a solicitat afișarea versiunii, afișează credits
    if (version_flag) {
        printf("Needy Resources, version %s\n.Proiect pt SO2\nVersiune protocol needy: %u\nProfesori coordonatori: Florin-Teodor Fortiș, Diogen Babuc\nEchipa:$Name\nMembri:\n1.Alexandru Turculeț\n2.Mitoiu Bogdan Mitoiu\n3.Timeea Tătărușanu",CLIENT_VERSION,NEEDY_PROTOCOL_VERSION);
        exit(EXIT_SUCCESS);
    }

    char client_queue_name[64]; //crează numele de coadă
    snprintf(client_queue_name, sizeof(client_queue_name), "/needy_client_mq_%d", my_pid);


    //definește atributele pentru coada de mesaje a clientului
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    mqd_t client_mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0644, &attr); //pe aici va răspunde serverul
    if (client_mq == (mqd_t)-1) { //dacă nu am reușit să creem coada
        perror("Client: Failed to create client queue");
        exit(1);
    }

    mqd_t server_mq = mq_open(SERVER_QUEUE_NAME, O_WRONLY); //aici va scrie clientul serverului
    if (server_mq == (mqd_t)-1) {
        perror("Client: Failed to open server queue");
        mq_unlink(client_queue_name);
        exit(1);
    }

    printf("Client %d started. Connecting to server...\n", my_pid);

    // instanțiem un antet de verificare. Vezi libneedy/client_identification.h pentru mai multe detalii
    needy_client_identification_header* identification_header = needy_client_identification_header_new(my_pid, client_workspace_path);

    // impachetam antetul de verificare intr-un mesaj needy.
    needy_message_t* msg = needy_message_new(CLIENT_CONNECTION_REQUEST,needy_client_identification_header_serialize(identification_header));
    send_message(server_mq, msg); // trimitem mesajul pe coada de mesaje


    needy_client_identification_header_destroy(identification_header); //eliberăm resursele
    needy_message_destroy(msg);


    printf("Client [%d] requesting resources...\n", my_pid);
    // emitem o cerere de resurse pentru sine
    needy_resource_request* request = needy_client_resource_request_new(my_pid, noResources);

    // impacheteaza si trimite
    needy_message_t* requestMsg = needy_message_new(RESOURCE_REQUEST, needy_client_resource_request_serialize(request));
    send_message(server_mq, requestMsg);
    needy_client_resource_request_destroy(request);


    //asteptam mesajul. Nu continuam fara resurse
    char receive_buffer[MAX_MSG_SIZE];
    printf("Client [%d] waiting for SERVER_ACK...\n", my_pid);

    ssize_t bytes_read = mq_receive(client_mq, receive_buffer, MAX_MSG_SIZE, NULL);
    if (bytes_read >= 0) { //daca am putut receptiona mesajul

        needy_message_t* receivedMessage = needy_message_from_string(receive_buffer); //citeste mesajul
        if (receivedMessage) { //daca am putut citi mesajul
            needy_server_ack* ack = needy_server_ack_deserialize(requestMsg->payload); //deserializaează în ack

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

    // am terminat procesarea
    printf("Client [%d] processing complete. Releasing resources...\n", my_pid);
    needy_client_finalize* finalize = needy_client_finalize_new(my_pid); //trimitem instiintarea de finalziare
    needy_message_t* msg_fin = needy_message_new(CLIENT_FINALIZE,needy_client_finalize_serialize(finalize));
    send_message(server_mq, msg_fin);
    needy_client_finalize_destroy(finalize);


    // curatam si incheiem executia
    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(client_queue_name);
    free(client_workspace_path);
    printf("Client [%d] shutting down safely.\n", my_pid);
    return 0;
}