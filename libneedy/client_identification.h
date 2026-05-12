//
// Created by vscode on 4/9/26.
//

#ifndef NEEDYRESOURCES_REQUEST_H
#define NEEDYRESOURCES_REQUEST_H
#include <stdint.h>
#include <ctype.h>
#include <stddef.h>
#include <unistd.h>
#include <jansson.h>
#include <sys/types.h>
#include <string.h>

/**
 * Structura ce definește urmatorul format JSON
 *
 * {
 *      "pid: $pid,
 *      "workspace_path": $workspace_path
 * }
 *
 *
 * Această structură este necesară pentru a defini ce fel de mesaj s-a transmis
 */
typedef struct {
    pid_t pid;
    char* workspace_path;
} needy_client_identification_header;

/**
 * Construiește un nou obiect de tipul needy_client_identification_header
 * @param pid PID-ul procesului
 * @param workspace_path Zona de ownership a procesului (folder)
 * @return obiect nou sau NULL
 */
needy_client_identification_header* needy_client_identification_header_new(pid_t pid, const char* workspace_path);
/**
 * Deserializează datele dintr-un AST JSON
 * @param data AST json
 * @return antetul conținut sau NULL dacă a apărut o eroare
 */
needy_client_identification_header* needy_client_identification_header_deserialize(json_t* data);

/**
 * Serializează un obiect de tipul needy_client_identification_header
 * @param this obiectul de serializat
 * @return AST JSON corespunzător
 */
json_t *needy_client_identification_header_serialize(needy_client_identification_header *this);
/**
 * Destructor pentru antentul de client
 * @param this antetul de distrus
 */
void needy_client_identification_header_destroy(needy_client_identification_header* this);

#endif //NEEDYRESOURCES_REQUEST_H
