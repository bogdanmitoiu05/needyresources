//
// Created by vscode on 5/12/26.
//

#include "resource_manager.h"

//Aici executam algoritmul bancherului efectiv

#define INITIAL_RESOURCE_SLOTS 10

int resource_manager_grant_resource(process_resource_information* pri, const char* resource_name)
{
    if (!pri || !resource_name) return -1;

    const char* basename = strrchr(resource_name, '/');
    basename = basename ? basename + 1 : resource_name;

    char link_name[512];
    snprintf(link_name, sizeof(link_name), ".grant_%d_%s", (int)pri->pid, basename);

    unlink(link_name);

    if (symlink(resource_name, link_name) < 0)
    {
        perror("resource_manager_grant_resource: symlink");
        return -1;
    }

    pri->has++;
    return 0;
}


int resource_manager_index(resource_manager* manager, const char* working_directory)
{
    if (!working_directory) return -1;

    if (!manager) return -1;


    DIR* dir = opendir(working_directory);
    if (!dir)
    {
        printf("resource_manager_index: opendir");
        return -1;
    }

    process_resource_information* pool = manager->process_resources[0];
    if (!pool)
    {
        pool = calloc(1, sizeof(process_resource_information));
        if (!pool) {
            closedir(dir);
            return -1;
        }
        pool->individualResourceNames =calloc(INITIAL_RESOURCE_SLOTS, sizeof(char*));
        if (!pool->individualResourceNames)
        {
            free(pool);
            closedir(dir);
            return -1;
        }
        pool->individualResourcesSize = INITIAL_RESOURCE_SLOTS;
        pool->pid = 0;
        manager->process_resources[0] = pool;
        manager->process_count = 1;
    }

    for (size_t i = 0; i < pool->individualResourcesSize; i++)
    {
        free(pool->individualResourceNames[i]);
        pool->individualResourceNames[i] = NULL;
    }
    pool->has = 0;

    struct dirent* entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 working_directory, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) < 0) continue;
        if (!S_ISREG(st.st_mode)) continue;

        if ((size_t)count >= pool->individualResourcesSize)
        {
            size_t new_size = pool->individualResourcesSize * 2;
            char** tmp = realloc(pool->individualResourceNames,
                                 new_size * sizeof(char*));
            if (!tmp) break;
            memset(tmp + pool->individualResourcesSize, 0,(new_size - pool->individualResourcesSize) * sizeof(char*));
            pool->individualResourceNames = tmp;
            pool->individualResourcesSize = new_size;
        }

        pool->individualResourceNames[count] = strdup(full_path);
        if (!pool->individualResourceNames[count]) break;
        count++;
    }

    pool->has = (size_t)count;
    closedir(dir);

    return count;
}

resource_manager* resource_manager_new(MQManager* manager, size_t maxResources)
{
    //printf("poc\n");

    if (!manager || maxResources == 0) return NULL;
    resource_manager* rm = calloc(1, sizeof(resource_manager));
    if (!rm) return NULL;

    rm->process_resources = calloc(maxResources + 1, sizeof(process_resource_information*));
    if (!rm->process_resources)
    {
        free(rm);
        return NULL;
    }

    rm->client_manager       = manager;
    rm->process_count        = 0;
    rm->active_process_count = 0;
    rm->idx                  = 0;
    rm->state.idx            = 0;
    rm->state.is_sorted      = false;

    return rm;
}

void resource_manager_destroy(resource_manager* manager)
{
    if (!manager) return;

    for (size_t i = 0; i < manager->process_count; i++)
    {
        process_resource_information* pri = manager->process_resources[i];
        if (!pri) continue;

        for (size_t j = 0; j < pri->individualResourcesSize; j++)
            free(pri->individualResourceNames[j]);

        free(pri->individualResourceNames);
        free(pri);
    }

    free(manager->process_resources);
    free(manager);

}

needy_resource_response_t* resource_manager_step(resource_manager* manager)
{
    if (!manager) return NULL;

    size_t n = manager->process_count;
    if (n <= 1) return NULL;

    process_resource_information* pool = manager->process_resources[0];
    if (!pool) return NULL;

    size_t allocated = 0;
    for (size_t i = 1; i < n; i++)
    {
        if (manager->process_resources[i])
            allocated += manager->process_resources[i]->has;
    }
    size_t work = (pool->has >= allocated) ? pool->has - allocated : 0;

    bool* finish = calloc(n, sizeof(bool));
    if (!finish) return NULL;
    finish[0] = true;

    bool changed = true;
    while (changed)
    {
        changed = false;
        for (size_t i = 1; i < n; i++)
        {
            process_resource_information* p = manager->process_resources[i];
            if (!p || finish[i]) continue;
            if (p->needs <= work)
            {
                finish[i] = true;
                work += p->has;
                changed = true;
            }
        }
    }


    bool safe = true;
    for (size_t i = 1; i < n; i++)
    {
        if (!finish[i]) { safe = false; break; }
    }
    free(finish);

    process_resource_information* target = NULL;
    for (size_t i = 1; i < n; i++)
    {
        process_resource_information* p = manager->process_resources[i];
        if (p && p->needs > 0) { target = p; break; }
    }

    if (!target) return NULL;

    needy_resource_response_t* response = calloc(1, sizeof(needy_resource_response_t));
    if (!response) return NULL;


    if (safe && pool->has > allocated)
    {
        process_resource_information* pool_entry = manager->process_resources[0];
        const char* resource_to_grant = NULL;
        for (size_t j = 0; j < pool_entry->individualResourcesSize; j++)
        {
            if (pool_entry->individualResourceNames[j])
            {
                resource_to_grant = pool_entry->individualResourceNames[j];
                break;
            }
        }

        if (resource_to_grant &&
            resource_manager_grant_resource(target, resource_to_grant) == 0)
        {
            response->code = OK;
            target->needs--;
        }
        else
        {
            response->code = DEADLOCK;
        }
    }
    else
    {
        response->code = DEADLOCK;
    }

    return response;
}

static int find_process_index(resource_manager* manager, pid_t pid)
{
    for (size_t i = 0; i < manager->process_count; i++)
    {
        if (manager->process_resources[i] &&
            manager->process_resources[i]->pid == pid)
            return (int)i;
    }
    return -1;
}
static int find_resource_in_process(process_resource_information* pri, const char* resource_name)
{
    for (size_t i = 0; i < pri->individualResourcesSize; i++)
    {
        if (pri->individualResourceNames[i] &&
            strcmp(pri->individualResourceNames[i], resource_name) == 0)
            return (int)i;
    }
    return -1;
}


int resource_manager_add_request(resource_manager* manager, const needy_resource_request* request)
{
    if (!request || !manager) return -1;

    pid_t pid = request->pid;
    size_t wanted   = request->requestedResources;
    int idx   = find_process_index(manager, pid);

    if (idx < 0)
    {
        size_t slot = 0;
        for (size_t i = 1; i < manager->process_count; i++)
        {
            if (!manager->process_resources[i]) {
                slot = i; break;
            }
        }
        if (slot == 0)
        {
            slot = manager->process_count;
            manager->process_count++;
        }

        process_resource_information* pri = calloc(1, sizeof(process_resource_information));
        if (!pri) return -1;

        pri->individualResourceNames = calloc(INITIAL_RESOURCE_SLOTS, sizeof(char*));
        if (!pri->individualResourceNames) {
            free(pri);
            return -1;
        }

        pri->individualResourcesSize = INITIAL_RESOURCE_SLOTS;
        pri->pid   = pid;
        pri->has   = 0;
        pri->needs = 0;

        manager->process_resources[slot] = pri;
        manager->active_process_count++;
        idx = (int)slot;
    }

    process_resource_information* pri = manager->process_resources[idx];
    process_resource_information* pool = manager->process_resources[0];

    size_t assigned = 0;

    if (pool)
    {
        for (size_t p = 0;
             p < pool->individualResourcesSize && assigned < wanted;
             p++)
        {
            if (!pool->individualResourceNames[p]) continue;

            const char* rname = pool->individualResourceNames[p];

            if (find_resource_in_process(pri, rname) >= 0) continue;

            size_t free_slot = pri->individualResourcesSize;
            for (size_t j = 0; j < pri->individualResourcesSize; j++)
            {
                if (!pri->individualResourceNames[j]) { free_slot = j; break; }
            }

            if (free_slot == pri->individualResourcesSize)
            {
                size_t new_size = pri->individualResourcesSize * 2;
                char** tmp = realloc(pri->individualResourceNames,
                                     new_size * sizeof(char*));
                if (!tmp) break;
                memset(tmp + pri->individualResourcesSize, 0,
                       (new_size - pri->individualResourcesSize) * sizeof(char*));
                pri->individualResourceNames    = tmp;
                pri->individualResourcesSize    = new_size;
            }

            pri->individualResourceNames[free_slot] = strdup(pool->individualResourceNames[p]);

            free(pool->individualResourceNames[p]);
            pool->individualResourceNames[p] = NULL;
            if (pool->has > 0)
                pool->has--;

            assigned++;
        }
    }


    pri->needs += assigned;

    return 0;
}


int resource_manager_release(resource_manager* manager, needy_client_finalize* finalize_request)
{
    if (!manager || !finalize_request) return -1;

    pid_t pid = finalize_request->pid;
    int idx   = find_process_index(manager, pid);

    if (idx < 0)
    {
        return -1;
    }

    process_resource_information* pri = manager->process_resources[idx];

    for (size_t j = 0; j < pri->individualResourcesSize; j++)
    {
        if (!pri->individualResourceNames[j]) continue;

        const char* resource_name = pri->individualResourceNames[j];
        const char* basename = strrchr(resource_name, '/');
        basename = basename ? basename + 1 : resource_name;

        char link_name[512];
        snprintf(link_name, sizeof(link_name), ".grant_%d_%s",
                 (int)pid, basename);
        unlink(link_name);

        free(pri->individualResourceNames[j]);
        pri->individualResourceNames[j] = NULL;
    }

    free(pri->individualResourceNames);
    free(pri);
    manager->process_resources[idx] = NULL;

    if (manager->active_process_count > 0)
        manager->active_process_count--;

    return 0;
}
