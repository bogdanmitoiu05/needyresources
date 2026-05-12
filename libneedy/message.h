//
// Created by vscode on 4/11/26.
//

#ifndef NEEDYRESOURCES_MESSAGE_FRAME_H
#define NEEDYRESOURCES_MESSAGE_FRAME_H
#include <stddef.h>
#include <message_type.h>
#include <jansson.h>
// invelim fiecare mesaj in urmatoarea structura
/**
 * ```json
 * {
 *      "version":1,
 *      "type":"identification_request",
 *      "payload":{
 *      //...
 *      }
 * }
 *
 * ```
 */
typedef struct message {
    size_t version;
    needy_message_type message_type;
    json_t *payload;
}needy_message_t;

/**
 * Crează o instață de mesaj
 * @param type tipul de mesaj (vezi message_type.h)
 * @param payload mesajul efectiv din interior
 * @return Un nou mesaj sau NULL la eșec
 */
needy_message_t* needy_message_new(needy_message_type type, json_t* payload);
/**
 * Serializează un mesaj în format JSON AST
 * @param msg mesajul de serializat
 * @return AST-ul JSON corespunzător sau NULL la eroare
 */
json_t* needy_message_serialize(needy_message_t* msg);
/**
 * Decodifică un mesaj din AST-ul JSON
 * @param msg mesajul de decodificat
 * @return instanța de mesaj corespunzătoare sau NULL la eroare
 */
needy_message_t* needy_message_deserialize(json_t* msg);
/**
 * Transformă mesajul în string
 * @param msg mesaj
 * @return Șir de caractere sau NULL la eroare
 */
char* needy_message_to_string(needy_message_t* msg);
/**
 * Decodează mesajul dintr-un string
 * @param str string
 * @return mesajul stocat în interior sau NULL la eroare
 */
needy_message_t* needy_message_from_string(char* str);

/**
 * Distruge un mesaj
 * @param msg Mesajul ce trebuie distrus
 */
void needy_message_destroy(needy_message_t* msg);

char* fixed_strdup(const char* in);
#endif //NEEDYRESOURCES_MESSAGE_FRAME_H
