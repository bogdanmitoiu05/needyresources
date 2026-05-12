//
// Created by vscode on 5/8/26.
//
#include "client_finalize.h"

needy_client_finalize* needy_client_finalize_new(pid_t pid) {

    // instanțiază un nou obiect de tipul needy_client_finalize
    needy_client_finalize* res = new(needy_client_finalize);

    // verifică dacă instanțierea a avut succes
    ENSURE_NOTNULL_MSG_RNULL(res, "Could not allocate resource_request");
    res->pid = pid;
    return res;
}

needy_client_finalize* needy_client_finalize_deserialize(json_t* data) {
    //verifică dacă pointerul la date nu este null
    ENSURE_NOTNULL_MSG_RNULL(data, "needy_client_resource_deserialize: NULL pointer passed");
    pid_t pid = 0; // stocare temporară pentru PID
    json_error_t error; //structură de depozitare a erorilor JSON

    // vom extrage din AST-ul JSON PID-ul.
    int success = json_unpack_ex(data, &error, JSON_STRICT /*nu accepta JSON ce nu respecta stringul de format*/, "{s:I}"/**format {string: long_int/pid_t}*/,"pid",&pid);
    if (success<0) { // error handling
        fputs("needy_client_resource_deserialize: invalid JSON",stderr);
        JSON_ERROR_PRINTF(error);
    }
    // instanțiază un răspuns client și introdu datele în acesta
    needy_client_finalize* client_response = needy_client_finalize_new(pid);
    return client_response;
}
json_t* needy_client_finalize_serialize(const needy_client_finalize* this) {

    // verificare pointer null
    ENSURE_NOTNULL_MSG_RNULL(this, "needy_client_resource_serialize: NULL pointer passed");

    json_error_t error;
    // vom împacheta rezultatul
    json_t* result = json_pack_ex(&error/*error handling*/, 0/*fara flags*/, "{s:I}"/*vezi mai sus*/,"pid",&this->pid);
    if (!result) { //error handling
        fputs("needy_client_resource_request_serialize: JSON Error",stderr);
        JSON_ERROR_PRINTF(error);
    }
    return result;
}
void needy_client_finalize_destroy(needy_client_finalize* this) {
    ENSURE_NOTNULL(this); // nu executa free pe pointeri null
    free(this);
}
