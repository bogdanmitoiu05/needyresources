//
// Created by vscode on 4/11/26.
//

#include "message.h"

#include "utils.h"
#include "constants.h"

needy_message_t* needy_message_new(needy_message_type type, json_t *payload) {
    ENSURE_NOTNULL_MSG_RNULL(payload, "needy_message_new: payload passed was a NULL pointer");
    if (type > N_TYPES) { // dacă a fost introdus un tip de date invalid -> raportează
        printf("needy_message_new: Invalid type %u\n", type);
        return NULL;
    }
    //alocare
    needy_message_t* message = calloc(1,  sizeof(needy_message_t));
    ENSURE_NOTNULL_MSG_RNULL(message, "needy_message_new: could not allocate message");
    //inițializare
    message->version = NEEDY_PROTOCOL_VERSION;
    message->message_type = type;
    message->payload = json_incref(payload); //vom marca faptul că aces obiect are o referință către încărcătură
    return message;

}

json_t * needy_message_serialize(needy_message_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_serialize: msg is NULL"); //null guard
    // verificare versiune protocol
    if (msg->version != NEEDY_PROTOCOL_VERSION) {
        fprintf(stderr,"Invalid protocol version %lu. Protocol version %lu needed", msg->version, (size_t) NEEDY_PROTOCOL_VERSION);
        return NULL;
    }
    // împachetare mesaj conform structurii din message.h
    json_t* result = json_pack("{s:I,s:s,s:O}"/*„O” indică faptul că obiectului i se va incrementa usage counterul*/,"version",msg->version,"type",needy_message_type_to_string(msg->message_type)/*obține stringul asociat valorii message type*/,"payload",msg->payload);
    ENSURE_NOTNULL_MSG_RNULL(result, "needy_message_serialize: Failed to serialize message"); //verificare eroare
    return result;
}

needy_message_t * needy_message_deserialize(json_t *msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: msg is NULL"); // null guard
    needy_message_t* result = calloc(1, sizeof(needy_message_t)); // instantiere
    ENSURE_NOTNULL_MSG_RNULL(msg, "needy_message_deserialize: could not allocate message");
    json_error_t error; // pregatire despachetare
    const char* type = NULL; //vezi mai sus
    int success = json_unpack_ex(msg, &error, JSON_STRICT, "{s:I,s:s,s:O}", "version",&(result->version), "type",type, "payload",&(result->payload));
    if (success<0) {
        JSON_ERROR_PRINTF(error);
        free(result);
        return NULL;
    }
    result->message_type = needy_message_type_from_string(type); //extrage tipul de mesaj din sirul de caractere aferent
    return result;

}

char* needy_message_to_string(needy_message_t* msg) {
    ENSURE_NOTNULL_MSG_RNULL(msg,"needy_message_to_string: msg is NULL"); // null guard
    json_t* root = needy_message_serialize(msg); // serializează mesajul
    return json_dumps(root, JSON_COMPACT); // scrie îm format JSON compact (fără \n)
}

needy_message_t* needy_message_from_string(char* str) {
    ENSURE_NOTNULL_MSG_RNULL(str, "needy_message_from_string: str is NULL"); // null guard
    json_error_t error; // eroare
    json_t* msgJson = json_loads(str, JSON_REJECT_DUPLICATES, &error); // încarcă din string
    if (!msgJson) { // la eroare, NULL
        JSON_ERROR_PRINTF(error);
        return NULL;
    }
    return needy_message_deserialize(msgJson); //returnea
}
void needy_message_destroy(needy_message_t *msg) {
    ENSURE_NOTNULL(msg);
    //if (msg->payload)
        //json_decref(msg->payload);
    free(msg);
}
