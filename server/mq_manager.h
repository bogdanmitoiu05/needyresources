//
// Created by vscode on 5/12/26.
//

#ifndef NEEDYRESOURCES_MQ_MANAGER_H
#define NEEDYRESOURCES_MQ_MANAGER_H
#include <needy.h>
#include <stdbool.h>
/**
 * Structura ce reprezinta o conexiune
 * O conexiune se defineste in server ca fiind o pereche (antet, queue_fd)
 */
typedef struct
{
    mqd_t queue;
    needy_client_identification_header* clientInfo;
} client_conn;

/**
 * Structura gestiune cozi
 *
 */
typedef struct {
    client_conn** queues; //cozi
    size_t queue_size; // nr. cozi
    size_t active_count; //cate procese sunt in asteptare
} MQManager;

/**
 * Gaseste conexiunea dupa PID
 * @param mgr obiectul manager
 * @param pid identificator proces
 * @return ID-ul găsit sau -1 dacă nu există
 */
mqd_t findByPid(MQManager* mgr,pid_t pid);
/**
 * Instanțiază o conexiune nouă de client
 * @param clientInfo antet client
 * @return conexiunea nouă sau NULL
 */
client_conn* client_conn_new(needy_client_identification_header* clientInfo);
/**
 * Destructor
 * @param conn conexiunea de distrus
 */
void client_conn_destroy(client_conn* conn);
/**
 * Instanțiere manager cozi
 * @param size nr. maxim de procese admise
 * @return manager sau NULL la eroare
 */
MQManager* mq_manager_new(size_t size);
/**
 * Adaugă un proces în gestiune
 * @param mgr manager
 * @param conn conexiune
 * @return 0 la OK, -1 la eroare
 */
int mq_manager_add(MQManager *mgr, client_conn* conn);
/**
 * Șterge un proces din gestiune
 * @param mgr manager
 * @param conn conexiunea aferentă
 * @return 0 la OK, -1 la eroare
 */
int mq_manager_remove(MQManager *mgr, client_conn* conn);
/**
 * Închide toate conexiunile
 * @param mgr manager
 */
void mq_manager_close_all(MQManager *mgr);
/**
 * Destructor manager
 * @param mgr obiectul de distrus
 */
void mq_manager_destroy(MQManager* mgr);
/**
 * @brief Functie care precizeaza daca mai avem loc 
 * 
 * @param mgr 
 * @return true 
 * @return false 
 */
bool mq_manager_has_space(MQManager* mgr);
#endif //NEEDYRESOURCES_MQ_MANAGER_H
