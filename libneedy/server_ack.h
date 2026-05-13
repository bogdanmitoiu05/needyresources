//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_SERVER_ACK_H
#define NEEDYRESOURCES_SERVER_ACK_H
#include <jansson.h>
#include <unistd.h>
#include "utils.h"
/// FUTURE USE
/**
*  * ```json
 * {
 *      "pid":$pid,
 *      "response": $response
 *
 * }
 *
 * ```
 */
typedef struct {
    pid_t pid;
    size_t response;
}needy_server_ack;

/**
 * Instanțiază un nou ACK de server
 * @param pid PID destinatar
 * @param response cod răspuns
 * @return ACK sau NULL la eroare
 */
needy_server_ack* needy_server_ack_new(pid_t pid, size_t response);

/**
 * Extrage un ACK server din AST JSON
 * @param data ACK-ul în format JSON AST
 * @return ACK-ul preluat sau NULL la eroare
 */
needy_server_ack* needy_server_ack_deserialize(json_t* data);

/**
 * Serializează un ACK în format JSON AST
 * @param this obiectul de serializat
 * @return AST-ul coresp. sau NULL la eroare
 */
json_t* needy_server_ack_serialize(const needy_server_ack* this);

/**
 * Destructor needy server ack
 * @param this obiectul de distrus
 */
void needy_server_ack_destroy(needy_server_ack* this);
#endif //NEEDYRESOURCES_SERVER_ACK_H
