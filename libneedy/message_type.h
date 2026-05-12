#ifndef NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
#define NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
typedef enum {
    RESOURCE_REQUEST, //cerere resurse
    CLIENT_CONNECTION_REQUEST, // cerere conectare
    SERVER_ACK, // ack conectare
    CLIENT_FINALIZE, // clientul a terminat
    N_TYPES, //truc -> valoarea se va actualiza automat în funcție de câte valori sunt înaintea sa
    MESSAGE_TYPE_UNKNOWN // -> invalid
} needy_message_type;

/**
 * Returnează tipul de mesaj din string
 * @param str string
 * @return tipul de mesaj
 */
needy_message_type needy_message_type_from_string(const char* str);
/**
 * Returnează varianta string a enumerației
 * @param type tip
 * @return șirul de caractere aferent
 */
char* needy_message_type_to_string(needy_message_type type);
#endif //NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
