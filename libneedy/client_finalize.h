//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_CLIENT_FINALIZE_H
#define NEEDYRESOURCES_CLIENT_FINALIZE_H
#include <unistd.h>
#include "utils.h"

/**
 * Structura ce definește urmatorul format JSON
 *
 * {
 *      "pid: $pid
 * }
 *
 *
 * Această structură este necesară pentru a defini ce fel de mesaj s-a transmis
 */
typedef struct {
    pid_t pid;
}needy_client_finalize;

/**
 * Instanțiază un nou obiect de tipul needy_client_finalize
 * @param pid PID-ul procesului
 * @return needy_client_finalize nou sau NULL dacă alocarea a eșuat
 */
needy_client_finalize* needy_client_finalize_new(pid_t pid);

/**
 * Crează un no
 * @param data
 * @return
 */
needy_client_finalize* needy_client_finalize_deserialize(json_t* data);
/**
 *
 * @param this
 * @return
 */
json_t* needy_client_finalize_serialize(const needy_client_finalize* this);
/**
 *
 * @param this
 */
void needy_client_finalize_destroy(needy_client_finalize* this);
#endif //NEEDYRESOURCES_CLIENT_FINALIZE_H
