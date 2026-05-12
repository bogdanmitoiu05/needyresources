//
// Created by vscode on 5/10/26.
//

#include "resource_response.h"

#include <string.h>

needy_resource_response_t * needy_resource_response_new(response_code code, size_t noResources, char **resourceNames) {
    ENSURE_NOTNULL_MSG_RNULL(resourceNames, "needy_resource_response_new: resourceNames is NULL");

    needy_resource_response_t* resource_response = new(needy_resource_response_t);
    ENSURE_NOTNULL_MSG_RNULL(resource_response, "needy_resource_response_new: could not allocate response");

    resource_response->code = code;
    resource_response->noResources = noResources;
    resource_response->resourceNames = resourceNames;
    return resource_response;
}

json_t* needy_resource_response_serialize(needy_resource_response_t *this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_resource_response_serialize: this is NULL");

    json_t* res = json_pack("{s:i,s:i}", "code", this->code,"resource_count",this->noResources);
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not pack res")
    json_t* arr = json_array();
    ENSURE_NOTNULL_MSG_RNULL(arr, "needy_resource_response_serialize: arr is NULL");
    for (size_t i = 0; i < this->noResources; ++i) {
        json_array_append_new(arr, json_string(this->resourceNames[i]));
    }
    json_object_set_new(res, "names", arr);
    return res;
}

needy_resource_response_t * needy_resource_response_deserialize(json_t *json) {
    ENSURE_NOTNULL_MSG_RNULL(json, "needy_resource_response_deserialize: json is NULL ");
    needy_resource_response_t* resource_response = new(needy_resource_response_t);
    ENSURE_NOTNULL_MSG_RNULL(resource_response, "needy_resource_response_deserialize: could not allocate resource response");
    json_error_t error;
    const int success = json_unpack_ex(json, &error, 0, "{s:i,s:i}", "code", &resource_response->code,"resource_count",&resource_response->noResources);
    if (success<0) {
        JSON_ERROR_PRINTF(error);
        free(resource_response);
        return NULL;
    }
    json_t* arrayObject = json_object_get(json, "names");
    if (!json_is_array(arrayObject)) {
        fputs("needy_resource_response_deserialize: names field is NOT an array",stderr);
        free(resource_response);
        return NULL;
    }

    char** names = calloc(resource_response->noResources,sizeof(char*));
    ENSURE_NOTNULL_MSG_RNULL(names, "needy_resource_response_deserialize: could not allocate names array");
    for (size_t i = 0; i < resource_response->noResources; ++i) {
        json_t* currString = json_array_get(arrayObject, i);
        if (!json_is_string(currString)) {
            fprintf(stderr,"needy_resource_response_deserialize: name index %lu not a string", i+1);
            free(names);
            free(resource_response);
            return NULL;
        }
        names[i] = calloc(json_string_length(currString)+1, sizeof(char));
        ENSURE_NOTNULL_MSG_RNULL(names[i], "needy_resource_response_deserialize: could not allocate string")
        strcpy(names[i],json_string_value(currString));
        //json_decref(currString);
    }
    //json_decref(arrayObject);

    resource_response->resourceNames = names;
    return resource_response;

}

void needy_resource_response_destroy(needy_resource_response_t *response) {
    ENSURE_NOTNULL(response);
    if(response->resourceNames != NULL) {
        for (size_t i = 0; i< response->noResources; ++i) {
            free(response->resourceNames[i]);
        }
        free(response->resourceNames);
    }
    free(response);
}