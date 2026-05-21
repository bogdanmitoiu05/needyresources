//
// Created by vscode on 5/10/26.
//

#include "resource_response.h"

#include <string.h>

needy_resource_response_t * needy_resource_response_new(response_code code, size_t noResources, char **resourceNames) {

    needy_resource_response_t* resource_response = new(needy_resource_response_t); // instantiere
    ENSURE_NOTNULL_MSG_RNULL(resource_response, "needy_resource_response_new: could not allocate response");

    //atribuire
    resource_response->code = code;
    resource_response->noResources = noResources;
    resource_response->resourceNames = resourceNames;
    return resource_response;
}

json_t* needy_resource_response_serialize(needy_resource_response_t *this) {
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_resource_response_serialize: this is NULL");// null check

    json_t* res = json_pack("{s:i,s:i}"/*i - int, s - string*/, "code", this->code,"resource_count",this->noResources); //impachetare.
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not pack res");
    json_t* arr = json_array(); // trebuie construit manual vectorul
    ENSURE_NOTNULL_MSG_RNULL(arr, "needy_resource_response_serialize: arr is NULL");
    for (size_t i = 0; i < this->noResources; ++i) { // pentru fiecare string din C, generam un string json
        json_array_append_new(arr, json_string(this->resourceNames[i]));
    }
    json_object_set_new(res, "names", arr);// adaugam vectorul nou creat la obiectul de raspuns
    return res;
}

needy_resource_response_t * needy_resource_response_deserialize(json_t *json) {
    ENSURE_NOTNULL_MSG_RNULL(json, "needy_resource_response_deserialize: json is NULL "); // null check
    needy_resource_response_t* resource_response = new(needy_resource_response_t); // instantiere
    ENSURE_NOTNULL_MSG_RNULL(resource_response, "needy_resource_response_deserialize: could not allocate resource response");
    json_error_t error; //procesul invers celui de mai sus
    const int success = json_unpack_ex(json, &error, 0, "{s:i,s:i}", "code", &resource_response->code,"resource_count",&resource_response->noResources);
    if (success<0) {
        JSON_ERROR_PRINTF(error);
        free(resource_response);
        return NULL;
    }
    json_t* arrayObject = json_object_get(json, "names"); // incepem parsarea manuala a vectorului de nume
    if (!json_is_array(arrayObject)) {
        fputs("needy_resource_response_deserialize: names field is NOT an array",stderr);
        free(resource_response);
        return NULL;
    }

    char** names = calloc(resource_response->noResources,sizeof(char*)); // instantierea vectorului de stringuri (char*)
    ENSURE_NOTNULL_MSG_RNULL(names, "needy_resource_response_deserialize: could not allocate names array");
    for (size_t i = 0; i < resource_response->noResources; ++i) { //pentru fiecare element din vectorul JSON extrage stringul continut
        json_t* currString = json_array_get(arrayObject, i);
        if (!json_is_string(currString)) {
            fprintf(stderr,"needy_resource_response_deserialize: name index %lu not a string", i+1);
            free(names);
            free(resource_response);
            return NULL;
        }
        names[i] = calloc(json_string_length(currString)+1, sizeof(char)); //alocare
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
    if(response->resourceNames != NULL) { // deoarece am alocat o matrice de caractere dinamic, trebuie să eliberăm fiecare rând înainte de a elibera ma
        for (size_t i = 0; i< response->noResources; ++i) {
            free(response->resourceNames[i]);
        }
        free(response->resourceNames);
    }
    free(response);
}