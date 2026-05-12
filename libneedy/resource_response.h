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
 * @param code
 * @param noResources
 * @param resourceNames
 * @return
 */
needy_resource_response_t* needy_resource_response_new(response_code code, size_t noResources, char** resourceNames);
/**
 *
 * @param this
 * @return
 */
json_t* needy_resource_response_serialize(needy_resource_response_t* this);
/**
 *
 * @param json
 * @return
 */
needy_resource_response_t* needy_resource_response_deserialize(json_t* json);
/**
 *
 * @param response
 */
void needy_resource_response_destroy(needy_resource_response_t* response);
#endif //NEEDYRESOURCES_RESOURCE_RESPONSE_H
