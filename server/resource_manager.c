//
// Created by vscode on 5/12/26.
//

#include "resource_manager.h"

//Aici executam algoritmul bancherului efectiv

#define INITIAL_RESOURCE_SLOTS 10

/**
 * creeaza symlink pe resursa respectiva pentru un anume client
 * @param pri proces information pointer
 * @param resource_name nume resursa
 * @return
 */
int resource_manager_grant_resource(process_resource_information* pri, const char* resource_name, size_t type_index)
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

    pri->has[type_index]++;
    return 0;
}

/**
 * find proces index in the current pool
 * of allocated resources
 * @param manager
 * @param pid
 * @return
 */
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

/**
 * gaseste numele unei resurse din lista de resurse a unui proces
 * @param pri
 * @param resource_name
 * @return 0 OK,-1 error
 */
static int find_resource_in_process(process_resource_information* pri, const char* resource_name, size_t type_index)
{
    for (size_t i = 0; i < pri->individualResourceSize[type_index]; i++)
    {
        if (pri->individualResourceNames[type_index][i] &&
            strcmp(pri->individualResourceNames[type_index][i], resource_name) == 0)
            return (int)i;
    }
    return -1;
}


/**
 * indexeaza un director pentru un manager de resurse
 *
 * @param manager Managerul de resurse
 * @param working_directory Directorul 
 * @param type_index 
 * @return numar de resurse gasite in acel director
 */
int resource_manager_index(resource_manager* manager, const char* working_directory, size_t type_index)
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

        pool->has = calloc(manager->nr_resource_type, sizeof(size_t));
        pool->needs = calloc(manager->nr_resource_type, sizeof(size_t));
        pool->individualResourceNames = calloc(manager->nr_resource_type, sizeof(char**));
        pool->individualResourceSize = calloc(manager->nr_resource_type, sizeof(size_t));

        for (size_t i = 0; i < manager->nr_resource_type; i++) {
            pool->individualResourceNames[i] =calloc(INITIAL_RESOURCE_SLOTS, sizeof(char*));
            pool->individualResourceSize[i] = INITIAL_RESOURCE_SLOTS;
        }
        if (!pool->individualResourceNames)
        {
            free(pool);
            closedir(dir);
            return -1;
        }
        pool->pid = 0;
        manager->process_resources[0] = pool;
        manager->process_count = 1;
    }

    for (size_t i = 0; i < pool->individualResourceSize[type_index]; i++)
    {
        free(pool->individualResourceNames[type_index][i]);
        pool->individualResourceNames[type_index][i] = NULL;
    }
    pool->has[type_index] = 0;

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

        if ((size_t)count >= pool->individualResourceSize[type_index])
        {
            size_t new_size = pool->individualResourceSize[type_index] * 2;
            char** tmp = realloc(pool->individualResourceNames[type_index],
                                 new_size * sizeof(char*));
            if (!tmp) break;
            memset(tmp + pool->individualResourceSize[type_index], 0,(new_size - pool->individualResourceSize[type_index]) * sizeof(char*));
            pool->individualResourceNames[type_index] = tmp;
            pool->individualResourceSize[type_index] = new_size;
        }

        pool->individualResourceNames[type_index][count] = fixed_strdup(full_path);
        if (!pool->individualResourceNames[type_index][count]) break;
        count++;
    }

    pool->has[type_index] = (size_t)count;
    //printf("--------\n\n\n%d\n\n\n",count);
    closedir(dir);

    return count;
}
/**
 * aloc o instanta manager de resurse ce are un numar maxim de resurse de asignat
 * @param manager message queue manager
 * @param maxResources
 * @return pointer manager de resurse
 */
resource_manager* resource_manager_new(MQManager* manager, size_t maxResources, size_t nr_resource_type)
{
    //printf("poc\n");

    if (!manager || maxResources == 0 || nr_resource_type == 0) return NULL;
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
/**
 * dau free la fiecare nume de resursa per proces,apoi manager
 * @param manager
 */
void resource_manager_destroy(resource_manager* manager)
{
    if (!manager) return;

    for (size_t i = 0; i < manager->process_count; i++)
    {
        process_resource_information* pri = manager->process_resources[i];
        if (!pri) continue;

        for (size_t k = 0; k < manager->nr_resource_type; k++) {
            for (size_t j = 0; j < pri->individualResourceSize[k]; j++)
                free(pri->individualResourceNames[k][j]);
            free(pri->individualResourceNames[k]);
        }

        free(pri->individualResourceNames);
        free(pri->individualResourceSize);
        free(pri->has);
        free(pri->needs);
        free(pri);
    }

    free(manager->process_resources);
    free(manager);

}

/**
 * realizeaza un pas din algoritmul bancherului
 * verifica daca poate aloca pentru fiecare proces valoarea ceruta
 *
 * @param manager
 * @return starea finala pentru cererile curente:OK , DEADLOCK
 */
needy_resource_response_t* resource_manager_step(resource_manager* manager)
{
    if (!manager) return NULL;

    size_t n = manager->process_count;
    if (n <= 1) return NULL;

    process_resource_information* pool = manager->process_resources[0];
    if (!pool) return NULL;

    process_resource_information* target = NULL;
    for (size_t i = 1; i < n; i++)
    {
        process_resource_information* p = manager->process_resources[i];
        if (!p) continue;
        bool needs = false;
        bool pool_has_enough = true;
        for (size_t j = 0; j < manager->nr_resource_type; j++) {
            if (p->needs[j] > 0) needs = true;
            if (pool->has[j] < p->needs[j]) {
                pool_has_enough = false;
                break;
            }
        }
        if (needs && pool_has_enough) {
            target = p;
            break;
        }
    }

    if (!target) return NULL;

    size_t* work = calloc(manager->nr_resource_type, sizeof(size_t));
    if (!work) return NULL;

    for (size_t i = 0; i < manager->nr_resource_type; i++) {
        work[i] = pool->has[i] - target->needs[i];
    }

    bool* finish = calloc(n, sizeof(bool));
    if (!finish) {
        free(work);
        return NULL;
    }
    finish[0] = true;

    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < n; i++) {
            process_resource_information* p = manager->process_resources[i];
            if (!p || finish[i]) continue;
            bool can_fin=true;
            for (size_t j = 0; j < manager->nr_resource_type; j++) {
                size_t curr_needs = p->needs[j];
                if (curr_needs > work[i]) {
                    can_fin = false;
                    break;
                }
            }
            if (can_fin) {
                finish[i] = true;
                for (size_t i = 0; i < manager->nr_resource_type; i++) {
                    size_t curr_allocation = (p == target) ? (p->has[i] + p->needs[i]) : p->has[i];
                    work[i] += curr_allocation;
                }
                changed = true;
            }
        }
    }

    bool safe = true;
    for (size_t i = 1; i < n; i++) {
        if (!finish[i]) { safe = false; break; }
    }
    free(finish);
    free(work);

    needy_resource_response_t* response = calloc(1, sizeof(needy_resource_response_t));
    if (!response) return NULL;
    if (safe)
    {
        response->code = OK;

        size_t total_needs = 0;
        for(size_t t = 0; t < manager->nr_resource_type; t++) {
            total_needs += target->needs[t];
        }

        response->noResources = total_needs;
        response->resourceNames = calloc(total_needs, sizeof(char*));

        size_t granted_all = 0;
        for (int i = 0; i < manager->nr_resource_type; i++) {
            size_t granted = 0;
            size_t needed = target->needs[i];

            for (size_t p = 0; p < pool->individualResourceSize[i] && granted < needed; p++)
            {
                if (!pool->individualResourceNames[i][p]) continue;

                char* rname = pool->individualResourceNames[i][p];

                size_t free_slot = target->individualResourceSize[i];
                for (size_t j = 0; j < target->individualResourceSize[i]; j++) {
                    if (!target->individualResourceNames[i][j]) { free_slot = j; break; }
                }

                if (free_slot == target->individualResourceSize[i])
                {
                    size_t new_size = target->individualResourceSize[i] * 2;
                    char** tmp = realloc(target->individualResourceNames[i], new_size * sizeof(char*));
                    memset(tmp + target->individualResourceSize[i], 0, (new_size - target->individualResourceSize[i]) * sizeof(char*));
                    target->individualResourceNames[i] = tmp;
                    target->individualResourceSize[i] = new_size;
                }

                target->individualResourceNames[i][free_slot] = rname;
                pool->individualResourceNames[i][p] = NULL;
                pool->has[i]--;

                response->resourceNames[granted_all] = strdup(rname);

                resource_manager_grant_resource(target, rname, i);

                target->needs[i]--;
                granted++;
                granted_all++;
            }
        }
    }
    else
    {
        response->code = DEADLOCK;
        response->noResources = 0;
        response->resourceNames = NULL;
    }

    return response;
}

/**
 * adauga cererea facuta de un client in situatia curenta a managerului:
 * client needs
 * @param manager
 * @param request
 * @return
 */
int resource_manager_add_request(resource_manager* manager, const needy_resource_request* request)
{
    if (!request || !manager ) return -1;

    pid_t pid = request->pid;
    size_t* wanted = request->requestedResources;
    int idx = find_process_index(manager, pid);

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

        pri->has   = calloc(manager->nr_resource_type, sizeof(size_t));
        pri->needs = calloc(manager->nr_resource_type, sizeof(size_t));
        pri->individualResourceSize = calloc(manager->nr_resource_type, sizeof(size_t));
        pri->individualResourceNames = calloc(manager->nr_resource_type, sizeof(char**));

        for (size_t i = 0; i < manager->nr_resource_type; i++) {
            pri->individualResourceNames[i] = calloc(INITIAL_RESOURCE_SLOTS, sizeof(char*));
            pri->individualResourceSize[i] = INITIAL_RESOURCE_SLOTS;
        }

        if (!pri->individualResourceNames) {
            free(pri->individualResourceSize);
            free(pri->needs);
            free(pri->has);
            free(pri);
            return -1;
        }

        pri->pid   = pid;


        manager->process_resources[slot] = pri;
        manager->active_process_count++;
        idx = (int)slot;
    }
    for (int i = 0; i < manager->nr_resource_type; i++) {
        manager->process_resources[idx]->needs[i] += wanted[i];
    }


    return 0;
}

/**
 * da free la resursele alocate unui client,cand clientul cere fin
 * @param manager resource manager
 * @param finalize_request
 * @return 0 OK,-1 eroare
 */
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

    for (size_t i = 0; i < manager->nr_resource_type; i++) {
        for (size_t j = 0; j < pri->individualResourceSize[i]; j++)
        {
            if (!pri->individualResourceNames[i][j]) continue;

            const char* resource_name = pri->individualResourceNames[i][j];
            const char* basename = strrchr(resource_name, '/');
            basename = basename ? basename + 1 : resource_name;

            char link_name[512];
            snprintf(link_name, sizeof(link_name), ".grant_%d_%s",
                     (int)pid, basename);
            unlink(link_name);

            free(pri->individualResourceNames[i][j]);
            pri->individualResourceNames[i][j] = NULL;
        }
        free(pri->individualResourceNames[i]);
    }
    free(pri->individualResourceNames);
    free(pri->individualResourceSize);
    free(pri->has);
    free(pri->needs);
    free(pri);
    manager->process_resources[idx] = NULL;

    if (manager->active_process_count > 0)
        manager->active_process_count--;

    return 0;
}
