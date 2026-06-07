//
// Created by vscode on 5/8/26.
//

#ifndef NEEDYRESOURCES_FILE_REQUEST_RESPONSE_H
#define NEEDYRESOURCES_FILE_REQUEST_RESPONSE_H
#include <unistd.h>
#include "utils.h"
#include "response_codes.h"

/**
 * Structura ce definește urmatorul format JSON
 *
 * {
 *      "response_code": $code,
 *      "file_contents": "$file_contents"
 * }
 *
 *
 * Această structură este necesară pentru a defini ce fel de mesaj s-a transmis
 */
typedef struct {
    response_code code;
    char* file_contents;
} needy_file_request_response;

/**
 * Instanțiază un nou obiect de tipul needy_file_request_response
 * @param code Codul de răspuns
 * @param file_contents Conținutul fișierului
 * @return needy_file_request_response nou sau NULL dacă alocarea a eșuat
 */
needy_file_request_response* needy_file_request_response_new(response_code code, const char* file_contents);

/**
 *
 * @param data
 * @return
 */
needy_file_request_response* needy_file_request_response_deserialize(json_t* data);
/**
 *
 * @param this
 * @return
 */
json_t* needy_file_request_response_serialize(const needy_file_request_response* this);
/**
 *
 * @param this
 */
void needy_file_request_response_destroy(needy_file_request_response* this);
#endif //NEEDYRESOURCES_FILE_REQUEST_RESPONSE_H