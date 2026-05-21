//
// Created by vscode on 4/9/26.
//

#ifndef NEEDYRESOURCES_RESOURCE_REQUEST_H
#define NEEDYRESOURCES_RESOURCE_REQUEST_H
#include "utils.h"
#include <unistd.h>
#include <jansson.h>
#include <sys/types.h>

/**
*  * ```json
 * {
 *      "pid":$pid,
 *      "requestedResources":[$noRequestedResourcesType1,$noRequestedResourcesType2,...]
 * }
 *
 * ```
 */
typedef struct {
    pid_t pid;
    size_t* requestedResources;
    size_t noResources;
} needy_resource_request;

/**
 * Instanțiază o nouă cerere de resurse
 * @param pid PID-ul doritorului
 * @param requestedResources numărul de resurse cerute
 * @return Instanța nou creată de cerere de resursă
 */
needy_resource_request* needy_client_resource_request_new(pid_t pid, size_t* requestedResources, size_t noResources);
/**
 * Deserializează cererea de resurse
 * @param data
 * @return
 */
needy_resource_request* needy_client_resource_request_deserialize(json_t* data);
/**
 *
 * @param this
 * @return
 */
json_t* needy_client_resource_request_serialize(const needy_resource_request* this);
/**
 *
 * @param this
 */
void needy_client_resource_request_destroy(needy_resource_request* this);
#endif //NEEDYRESOURCES_RESOURCE_REQUEST_H
