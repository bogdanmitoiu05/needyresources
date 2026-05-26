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
        char msg[64];
        snprintf(msg, sizeof(msg), "[Client %d]: Could not allocate workspace path",my_pid);
        fputs(msg, stderr);
        exit(EXIT_FAILURE);
    }


    snprintf(client_workspace_path, 1023, "workspace_%d", my_pid); //interpolează PID-ul cu șirul de caractere
    size_t id = 0; // variabilă ajutătoare pentru a defini clienții/procesele
    char* json_file_path = NULL; // calea catre fisierul JSON
    bool stop_parse = false; //dacă am întâlnit un argument ce determină oprirea: -h sau -v
    size_t noResources = 2; // implicit clientul va solicita 2 resurse
    while ((opt = getopt(argc, argv, "hvi:p:r:f:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'f': // fișier js
                json_file_path = fixed_strdup(optarg);
                break;
            case 'p': //calea către workspace
                free(client_workspace_path);
                client_workspace_path = fixed_strdup(optarg);
                break;
            case 'i': //id
                id = strtol(optarg, NULL, 10); //conversie în baza 10
                if (id == 0) { //functia va returna 0 dacă întâlnește un șir de intrare ce nu reprezintă un număr
                    char msg[64];
                    snprintf(msg, sizeof(msg), "[Client %d]: Invalid ID specified",my_pid);
                    fputs(msg, stderr);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r': //nr. resurse de cerut
                noResources = strtol(optarg, NULL, 10); // vezi mai sus
                if (id == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "[Client %d]: Resource number (no)invalid",my_pid);
                    fputs(msg, stderr);
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
    mq_unlink(client_queue_name);

    mqd_t client_mq = mq_open(client_queue_name, O_CREAT | O_RDONLY, 0644, &attr); //pe aici va răspunde serverul
    if (client_mq == (mqd_t)-1) { //dacă nu am reușit să creem coada
        char msg[64];
        snprintf(msg, sizeof(msg), "[Client %d]: Failed to open client queue",my_pid);
        perror(msg);
        exit(1);
    }

    mqd_t server_mq = mq_open(SERVER_QUEUE_NAME, O_WRONLY); //aici va scrie clientul serverului
    if (server_mq == (mqd_t)-1) {
        char msg[64];
        snprintf(msg, sizeof(msg), "[Client %d]: Failed to open server queue",my_pid);
        perror(msg);mq_unlink(client_queue_name);
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

    char receive_buffer[MAX_MSG_SIZE];
    printf("[Client %d] waiting for SERVER_ACK...\n", my_pid);

    ssize_t bytes_read = mq_receive(client_mq, receive_buffer, MAX_MSG_SIZE, NULL);
    if (bytes_read >= 0) {
        needy_message_t* receivedMessage = needy_message_from_string(receive_buffer);
        if (receivedMessage) {
            needy_server_ack *msg= needy_server_ack_deserialize(receivedMessage->payload);
            printf("[Client %d] received ack message from server(%d)\n", my_pid, msg->code);
            needy_server_ack_destroy(msg);
        }
    }

    if (json_file_path==NULL) {
        fprintf(stderr, "[Client %d]: missing JSON file parameter, provide -f <file.json>\n", my_pid);
        exit(EXIT_FAILURE);
    }

    json_error_t json_error;
    json_t* fis=json_load_file(json_file_path,0,&json_error);

    if (!fis) {
        fprintf(stderr, "[Client %d]: Error parsing JSON on line %d: %s\n", my_pid, json_error.line, json_error.text);
        exit(EXIT_FAILURE);
    }

    json_t* requests= json_object_get(fis,"requests");
    json_t* nr_resources_json = json_object_get(fis,"nr_resources");

    if (!json_is_array(requests)) {
        fprintf(stderr, "[Client %d]: 'requests' field is missing or is not an array\n", my_pid);
        json_decref(fis);
        exit(EXIT_FAILURE);
    }
    size_t total_resource_types = nr_resources_json ? json_integer_value(nr_resources_json) : noResources;
    size_t req_index;
    json_t *req_array;
    json_array_foreach(requests, req_index, req_array) {
        if (!json_is_array(req_array)) continue;

        // un vectorul de resurse din JSON
        size_t *resource_vector = calloc(total_resource_types, sizeof(int));
        size_t res_index;
        json_t *res_val;

        json_array_foreach(req_array, res_index, res_val) {
            if (res_index < total_resource_types) {
                resource_vector[res_index] = (int)json_integer_value(res_val);
            }
        }

        printf("[Client %d] requesting resources...\n", my_pid);
        // emitem o cerere de resurse pentru sine
        needy_resource_request* request = needy_client_resource_request_new(my_pid, resource_vector, total_resource_types);

        // impacheteaza si trimite
        needy_message_t* requestMsg = needy_message_new(RESOURCE_REQUEST, needy_client_resource_request_serialize(request));
        send_message(server_mq, requestMsg);
        needy_client_resource_request_destroy(request);


        //asteptam mesajul. Nu continuam fara resurse
        printf("[Client %d] waiting for RESOURCE_RESPONSE...\n", my_pid);

        bytes_read = mq_receive(client_mq, receive_buffer, MAX_MSG_SIZE, NULL);
        if (bytes_read >= 0) { //daca am putut receptiona mesajul

            needy_message_t* receivedMessage = needy_message_from_string(receive_buffer); //citeste mesajul
            //puts(receive_buffer);
            if (receivedMessage) { //daca am putut citi mesajul
                needy_resource_response_t* ack = needy_resource_response_deserialize(receivedMessage->payload); //deserializaează în ack

                printf("[Client %d] received code from server (Response Code: %d)\n", my_pid, ack->code);

                needy_resource_response_destroy(ack);
            }
        } else {
            char msg[64];
            snprintf(msg, sizeof(msg), "[Client %d]: mq_receive failed",my_pid);
            perror(msg);
        }
        // simulam
        // --- to be  changed ---
        printf("[Client %d] holding resources. Processing for 10 seconds...\n", my_pid);
        sleep(10);
        free(resource_vector);
    }
    //curat json
    json_decref(fis);

    // am terminat procesarea
    printf("[Client %d] processing complete. Releasing resources...\n", my_pid);
    needy_client_finalize* finalize = needy_client_finalize_new(my_pid); //trimitem instiintarea de finalziare
    needy_message_t* msg_fin = needy_message_new(CLIENT_FINALIZE,needy_client_finalize_serialize(finalize));
    send_message(server_mq, msg_fin);
    needy_client_finalize_destroy(finalize);


    // curatam si incheiem executia
    mq_close(server_mq);
    mq_close(client_mq);
    mq_unlink(client_queue_name);
    free(client_workspace_path);
    if(json_file_path) free(json_file_path);
    printf("[Client %d] shutting down safely.\n", my_pid);
    return 0;
}