//
// Created by vscode on 5/10/26.
//

#ifndef NEEDYRESOURCES_SERVER_CONFIG_H
#define NEEDYRESOURCES_SERVER_CONFIG_H
#include <stdbool.h>
#include <stddef.h>
#include <needy.h>
#define SERVER_CONFIG_FILE_VERSION 2
typedef struct {
    char* workingDirectory;
    bool liveUpdate;
    size_t maximumAllowedResources;
    size_t maximumClients;
    size_t typesOfResources;

}server_config_t;

void server_config_destroy(server_config_t* this);
server_config_t* load_from_file(const char* file);
int dump_to_file(server_config_t* config, const char* file);
#endif //NEEDYRESOURCES_SERVER_CONFIG_H
