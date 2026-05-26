//
// Created by vscode on 4/9/26.
//

#include "resource_request.h"


__attribute_warn_unused_result__ needy_resource_request* needy_client_resource_request_new(pid_t pid, size_t* requestedResources, size_t noResources){
    needy_resource_request* res = calloc(1,sizeof(needy_resource_request)); // instanțiere și verificare
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    res->noResources = noResources;
    res->requestedResources = requestedResources;
    return res;
}

__attribute_warn_unused_result__ needy_resource_request * needy_client_resource_request_deserialize(json_t *data) {
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_client_resource_deserialize: NULL pointer passed"); // null guard
    if (!json_is_object(data)) {
        fputs("needy_client_resource_deserialize: Data passed not an object",stderr);
        return NULL;
    }
    pid_t pid = 0;
    json_error_t error; // deserializare
    int success = json_unpack_ex(data, &error, 0, "{s:I}"/*I = long int*/,"pid",&pid);
    if (success<0) {
        fputs("needy_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }

    json_t* arrayObject = json_object_get(data, "requestedResources"); // incepem parsarea manuala a vectorului de resurse cerute
    if (!json_is_array(arrayObject)) {
        fputs("needy_resource_resource_deserialize: requestedResources field is NOT an array",stderr);
        printf("%s",json_dumps(arrayObject,JSON_INDENT(4)));
        return NULL;
    }

    size_t* resources = calloc(json_array_size(arrayObject),sizeof(size_t)); // instantierea vectorului de numere
    ENSURE_NOTNULL_MSG_RNULL(resources, "needy_resource_response_deserialize: could not allocate names array");
    for (size_t i = 0; i < json_array_size(arrayObject); ++i) { //pentru fiecare element din vectorul JSON extrage numarul
        json_t* currItem = json_array_get(arrayObject,i);
        if (!json_is_number(currItem)) {
            fputs("needy_client_resource_request_deserialize: Not a number",stderr);
            resources[i] = 0;
        }
        else
            resources[i] = json_number_value(currItem);
        //json_decref(currString);
    }
    //json_decref(arrayObject);
    // instanțiere cu noile date
    needy_resource_request* resource_request = needy_client_resource_request_new(pid,resources,json_array_size(arrayObject));
    return resource_request;
}

__attribute_warn_unused_result__ json_t* needy_client_resource_request_serialize(const needy_resource_request *this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_client_resource_serialize: NULL pointer passed"); //procesul de mai sus, invers

    json_error_t error;
    json_t* result = json_pack_ex(&error, 0, "{s:I}"/*idem*/,"pid",this->pid);
    if (!result) {
        fputs("needy_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }

    json_t* arr = json_array(); // trebuie construit manual vectorul
    ENSURE_NOTNULL_MSG_RNULL(arr, "needy_resource_response_serialize: arr is NULL");
    for (size_t i = 0; i < this->noResources; ++i) { // pentru fiecare string din C, generam un string json
        json_array_append_new(arr, json_integer(this->requestedResources[i]));
    }
    json_object_set_new(result, "requestedResources", arr);// adaugam vectorul nou creat la obiectul de raspuns
    return result;
}

void needy_client_resource_request_destroy(needy_resource_request *this) {
    ENSURE_NOTNULL(this); // null
    if (this->requestedResources)
        free(this->requestedResources);
    free(this);
}
