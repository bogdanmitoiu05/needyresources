//
// Created by vscode on 5/12/26.
//

#ifndef NEEDYRESOURCES_RESOURCE_MANAGER_H
#define NEEDYRESOURCES_RESOURCE_MANAGER_H
#include <needy.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <needy.h>
#include "mq_manager.h"
typedef struct
{
    size_t* has;
    size_t* needs;
    char*** individualResourceNames;
    size_t* individualResourceSize;
    pid_t pid;
}process_resource_information;

typedef struct
{
    size_t idx;
    bool is_sorted;
}
resource_manager_state;

typedef struct
{
    process_resource_information** process_resources;
    MQManager* client_manager;
    size_t process_count;
    size_t active_process_count;
    size_t idx;
    resource_manager_state state;
    size_t nr_resource_type;
}resource_manager;

resource_manager* resource_manager_new(MQManager* manager, size_t maxResources, size_t nr_resource_type);
void resource_manager_destroy(resource_manager* manager);
int resource_manager_index(resource_manager* manager, const char* working_directory,size_t type_index);
needy_resource_response_t* resource_manager_step(resource_manager* manager);
int resource_manager_add_request(resource_manager* manager,const needy_resource_request* request,size_t type_index);
int resource_manager_release(resource_manager* manager, needy_client_finalize* finalize_request);


#endif //NEEDYRESOURCES_RESOURCE_MANAGER_H
