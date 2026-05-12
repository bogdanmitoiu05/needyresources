//
// Created by vscode on 5/10/26.
//
/**
*  * ```json
 * {
 *      "response_code": $code,
 *      "resource_count": $count,
 *      "resources": [
 *          "./res1",
 *          "./res2"
 *       ]
 * }
 *
 * ```
 */
#ifndef NEEDYRESOURCES_RESOURCE_RESPONSE_H
#define NEEDYRESOURCES_RESOURCE_RESPONSE_H
#include <jansson.h>
#include <utils.h>
typedef enum {
    OK = 200,
    DEADLOCK = 507
} response_code;

typedef struct {
    response_code code;
    size_t noResources;
    char** resourceNames;
} needy_resource_response_t;

/**
 * Instanțiază un nou răspuns server
 * @param code Codul de răspuns din partea serverului
 * @param noResources Numărul de resurse atașate
 * @param resourceNames Numele fișierelor aferente resurselor
 * @return
 */
needy_resource_response_t* needy_resource_response_new(response_code code, size_t noResources, char** resourceNames);
/**
 * Serializează un răspuns de resurse în format AST JSON
 * @param this obiectul de serializat
 * @return forma serializată
 */
json_t* needy_resource_response_serialize(needy_resource_response_t* this);
/**
 * Deserializează un răspuns în format json AST și instanțiază un obiect cu informațiile conținute în acesta
 * @param json AST-ul JSON
 * @return un răspuns sau NULL pentru eroare
 */
needy_resource_response_t* needy_resource_response_deserialize(json_t* json);
/**
 * Distruge un răspuns
 * @param response răspunsul de distrus
 */
void needy_resource_response_destroy(needy_resource_response_t* response);
#endif //NEEDYRESOURCES_RESOURCE_RESPONSE_H
