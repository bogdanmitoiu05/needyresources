//
// Created by vscode on 4/9/26.
//

#include "client_identification.h"
#include <utils.h>
#include <string.h>


needy_client_identification_header * needy_client_identification_header_new(pid_t pid, const char *workspace_path) {
    ENSURE_NOTNULL_MSG_RNULL(workspace_path,"Workspace path can't be empty"); // nu admite zonă de deținere vidă
    needy_client_identification_header* header = new(needy_client_identification_header); // instanțiere via calloc()
    ENSURE_NOTNULL_MSG_RNULL(header, "Could not allocate header"); // verificare alocare antet

    header->workspace_path=strdup(workspace_path); // duplicare string pt calea workspace
    header->pid = pid; //asignare PID
    return header;

}

needy_client_identification_header * needy_client_identification_header_deserialize(json_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: msg is NULL"); // verificare anti null
    needy_client_identification_header* result = new(needy_client_identification_header); //instantiere
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: could not allocate message"); // verificare instantiere
    json_error_t error; // spațiu de eroare extragere din AST
    // extrage în format strict
    int success = json_unpack_ex(msg, &error, JSON_STRICT, "{s:I,s:s}"/*string: long_int, string:string*/, "pid",&(result->pid), "workspace_path",result->workspace_path);
    if (success<0) { //eroare
        JSON_ERROR_PRINTF(error);
        free(result);
        return NULL;
    }
    return result;
}

json_t *needy_client_identification_header_serialize(needy_client_identification_header *this) {
    ENSURE_NOTNULL_RNULL(this); //verificare anti null
    // obiect -> AST
    json_t* obj = json_pack("{s:I, s:s}"/*string:long int, string: string*/,"pid",this->pid,"workspace_path",this->workspace_path);
    return obj;
}

void needy_client_identification_header_destroy(needy_client_identification_header *this) {
    ENSURE_NOTNULL(this); //nu apela free pe null
    if (this->workspace_path) {
        free(this->workspace_path);
    }
    free(this);
}
