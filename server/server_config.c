//
// Created by vscode on 5/10/26.
//

#include "server_config.h"
#include <jansson.h>

void server_config_destroy(server_config_t* this) {
    ENSURE_NOTNULL(this);
    if (this->workingDirectory)
        free(this->workingDirectory);

    free(this);
}
json_t* server_config_serialize(server_config_t* config) {
    json_error_t error;
    ENSURE_NOTNULL_MSG_RNULL(config, "server_config_serialize: config is NULL")
    json_t* confObj = json_pack_ex(&error, 0, "{s:s,s:b,s:i,s:i}",
        "workingDirectory", &(config->workingDirectory),
        "enableLiveUpdate", &(config->liveUpdate),
        "maximumAllowedResources", &(config->maximumAllowedResources),
        "maximumClients", &(config->maximumClients));
    if (!confObj) {
        JSON_ERROR_PRINTF(error);
        return NULL;
    }

    json_t* confFileObj = json_pack_ex(&error, 0, "{s:i,s:o}", "version", 1, "config", confObj);
    if (!confFileObj) {
        JSON_ERROR_PRINTF(error);
        return NULL;
    }

    return confFileObj;
}

server_config_t* server_config_deserialize(json_t* json) {
    json_error_t error;
    ENSURE_NOTNULL_MSG_RNULL(json, "server_config_deserialize: json is NULL")
    int version;
    json_t* obj;
    const int success1 = json_unpack_ex(json,&error, 0, "{s:i,s:o}", "version",&version, "config", &obj);
    if (success1 < 0) {
        JSON_ERROR_PRINTF(error);
        return NULL;
    }

    server_config_t* conf = new(server_config_t);
    if (!conf) {
        fputs("load_from_file: could not allocate config object",stderr);
        //json_decref(json);
        return NULL;
    }

    const int success2 = json_unpack_ex(obj, &error, 0, "{s:s,s:b,s:i,s:i}",
        "workingDirectory", &(conf->workingDirectory),
        "enableLiveUpdate", &(conf->liveUpdate),
        "maximumAllowedResources", &(conf->maximumAllowedResources),
        "maximumClients", &(conf->maximumClients));
    if (success2 < 0) {
        JSON_ERROR_PRINTF(error);
        free(conf);
        //json_decref(obj);
        return NULL;
    }

    return conf;
}
server_config_t * load_from_file(const char *file) {
    ENSURE_NOTNULL_MSG_RNULL(file, "load_from_file: file is NULL");
    FILE* confFd = fopen(file, "r");
    ENSURE_NOTNULL_MSG_RNULL(confFd, "load_from_file: config file not found");
    json_error_t error;
    json_t* configFileJson = json_loadf(confFd,0, &error);


    if (!configFileJson) {
        JSON_ERROR_PRINTF(error);
        return NULL;
    }
    fclose(confFd);
    server_config_t* conf = server_config_deserialize(configFileJson);
    return conf;
}


int dump_to_file(server_config_t *config, const char *file) {
    ENSURE_NOTNULL_MSG_RETVAL(config, "dump_to_file: config is NULL",-1);
    ENSURE_NOTNULL_MSG_RETVAL(file, "dump_to_file: file is NULL",-1);
    json_t* fileJson = server_config_serialize(config);
    ENSURE_NOTNULL_MSG_RETVAL(fileJson, "dump_to_file: fileJson is NULL", -1);
    FILE* f = fopen(file, "w");
    json_dumpf(fileJson, f, JSON_INDENT(4));
    fclose(f);
    return 0;
}
