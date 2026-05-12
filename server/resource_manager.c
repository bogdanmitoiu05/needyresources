//
// Created by vscode on 5/12/26.
//

#include "resource_manager.h"

//Aici executam algoritmul bancherului efectiv

int resource_manager_grant_resource(process_resource_information* pri, const char* resource_name)
{
    //symlink
}

int resource_manager_index(resource_manager* manager, const char* working_directory)
{

}
resource_manager* resource_manager_new(MQManager* manager, size_t maxResources)
{

}
void resource_manager_destroy(resource_manager* manager)
{

}
needy_resource_response_t* resource_manager_step(resource_manager* manager)
{

}
int resource_manager_add_request(const needy_resource_request* request)
{

}
int resource_manager_release(resource_manager* manager, needy_client_finalize* finalize_request)
{

}