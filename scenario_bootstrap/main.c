#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <needy.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <jansson.h>

#define MAX_CLIENTS 10
#define SERVER_START_DELAY_SECONDS 1
#define CLIENT_TIMEOUT_SECONDS 120

typedef struct {
    int id;
    char workspace[256];
    char request_file[256];
} client_scenario_t;

static pid_t start_process(char *const argv[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("[BOOTSTRAP] fork failed");
        return -1;
    }

    if (pid == 0) {
        execv(argv[0], argv);
        perror("[BOOTSTRAP] execv failed");
        exit(EXIT_FAILURE);
    }

    return pid;
}
static void wait_for_client(pid_t pid, int client_id) {
    int status = 0;
    time_t start_time = time(NULL);

    while (1) {
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid) {
            if (WIFEXITED(status)) {
                printf(
                    "[BOOTSTRAP] Client %d finished with exit code %d\n",
                    client_id,
                    WEXITSTATUS(status)
                );
            } else if (WIFSIGNALED(status)) {
                printf(
                    "[BOOTSTRAP] Client %d was terminated by signal %d\n",
                    client_id,
                    WTERMSIG(status)
                );
            }
            return;
        }

        if (result == -1) {
            perror("[BOOTSTRAP] waitpid failed");
            return;
        }

        if (time(NULL) - start_time >= CLIENT_TIMEOUT_SECONDS) {
            printf(
                "[BOOTSTRAP] Client %d timeout. Killing process...\n",
                client_id
            );

            kill(pid, SIGTERM);
            sleep(1);

            if (waitpid(pid, &status, WNOHANG) == 0) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }

            return;
        }

        sleep(1);
    }
}

int main(void) {
    printf("[BOOTSTRAP] Starting beta scenario...\n");

    char *server_argv[] = {
        "/IdeaProjects/needyresources/cmake-build-debug/server/nr_server",
        "-c",
        "/IdeaProjects/needyresources/files/config.json",
        NULL
    };

    printf("[BOOTSTRAP] Starting server...\n");
    pid_t server_pid = start_process(server_argv);
    if (server_pid <= 0) {
    fprintf(stderr, "[BOOTSTRAP] Could not start server\n");
    return EXIT_FAILURE;
}

    if (server_pid < 0) {
        return EXIT_FAILURE;
    }

    sleep(SERVER_START_DELAY_SECONDS);

        json_error_t error;
    json_t *scenario_json = json_load_file(
        "/IdeaProjects/needyresources/files/scenarios/scenario_1.json",
        0,
        &error
    );

    if (!scenario_json) {
        fprintf(stderr,
                "[BOOTSTRAP] Failed to load scenario file: %s\n",
                error.text);
        return EXIT_FAILURE;
    }

    json_t *clients = json_object_get(scenario_json, "clients");

    if (!json_is_array(clients)) {
        fprintf(stderr, "[BOOTSTRAP] Invalid clients array\n");
        json_decref(scenario_json);
        return EXIT_FAILURE;
    }

    size_t client_count = json_array_size(clients);
    if (client_count > MAX_CLIENTS) {
    fprintf(stderr,
            "[BOOTSTRAP] Too many clients (%zu). Maximum is %d\n",
            client_count,
            MAX_CLIENTS);
    json_decref(scenario_json);
    return EXIT_FAILURE;
}

    client_scenario_t scenario[MAX_CLIENTS];
    pid_t client_pids[MAX_CLIENTS];

    size_t index;
    json_t *client_json;

    json_array_foreach(clients, index, client_json) {

        scenario[index].id =
            json_integer_value(json_object_get(client_json, "id"));

        strcpy(
            scenario[index].workspace,
            json_string_value(
                json_object_get(client_json, "workspace")
            )
        );

        strcpy(
            scenario[index].request_file,
            json_string_value(
                json_object_get(client_json, "request_file")
            )
        );

        char id_arg[16];

        snprintf(
            id_arg,
            sizeof(id_arg),
            "%d",
            scenario[index].id
        );

        char *client_argv[] = {
            "../client/nr_client",
            "-i",
            id_arg,
            "-p",
            scenario[index].workspace,
            "-f",
            scenario[index].request_file,
            NULL
        };

        printf(
            "[BOOTSTRAP] Starting client %d...\n",
            scenario[index].id
        );

        client_pids[index] = start_process(client_argv);
        if (client_pids[index] <= 0) {
            fprintf(stderr,
            "[BOOTSTRAP] Failed to start client %d\n",
            scenario[index].id);
}

        
    }

    for (size_t i = 0; i < client_count; i++) {
        wait_for_client(client_pids[i], scenario[i].id);
    }

    json_decref(scenario_json);

    printf("[BOOTSTRAP] Stopping server...\n");
    if (kill(server_pid, SIGTERM) < 0) {
    perror("[BOOTSTRAP] Failed to stop server");
    }

    waitpid(server_pid, NULL, 0);

    printf("[BOOTSTRAP] Scenario finished.\n");

    return EXIT_SUCCESS;
}