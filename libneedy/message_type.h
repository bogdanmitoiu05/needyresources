#ifndef NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
#define NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
typedef enum {
    RESOURCE_REQUEST,
    CLIENT_CONNECTION_REQUEST,
    SERVER_ACK,
    CLIENT_FINALIZE,
    N_TYPES,
    MESSAGE_TYPE_UNKNOWN
} needy_message_type;

needy_message_type needy_message_type_from_string(const char* str);
char* needy_message_type_to_string(needy_message_type type);
#endif //NEEDYRESOURCES_MESSAGE_FRAME_TYPE_H
