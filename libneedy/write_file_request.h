//
// Created by vscode on 6/7/26.
//

#ifndef NEEDYRESOURCES_WRITE_FILE_REQUEST_H
#define NEEDYRESOURCES_WRITE_FILE_REQUEST_H
#include <unistd.h>
#include "utils.h"

typedef enum {
    WRITE_MODE_WRITE,
    WRITE_MODE_APPEND
} needy_write_mode;

/**
 * Structura ce definește urmatorul format JSON
 *
 * {
 *      "pid": $pid,
 *      "file_name": "$file_name",
 *      "content": "$content",
 *      "mode": $mode
 * }
 *
 *
 * Această structură este necesară pentru a defini ce fel de mesaj s-a transmis
 */
typedef struct {
    pid_t pid;
    char* file_name;
    char* content;
    needy_write_mode mode;
} needy_write_file_request;

/**
 * Instanțiază un nou obiect de tipul needy_write_file_request
 * @param pid PID-ul procesului
 * @param file_name Numele fișierului
 * @param content Continutul de scris
 * @param mode Modul (write sau append)
 * @return needy_write_file_request nou sau NULL dacă alocarea a eșuat
 */
needy_write_file_request* needy_write_file_request_new(pid_t pid, const char* file_name, const char* content, needy_write_mode mode);

/**
 *
 * @param data
 * @return
 */
needy_write_file_request* needy_write_file_request_deserialize(json_t* data);
/**
 *
 * @param this
 * @return
 */
json_t* needy_write_file_request_serialize(const needy_write_file_request* this);
/**
 *
 * @param this
 */
void needy_write_file_request_destroy(needy_write_file_request* this);
#endif //NEEDYRESOURCES_WRITE_FILE_REQUEST_H