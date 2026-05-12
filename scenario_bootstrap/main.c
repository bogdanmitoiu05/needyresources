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

#define CLIENT_COUNT 3
#define SERVER_START_DELAY_SECONDS 1
#define CLIENT_TIMEOUT_SECONDS 20

typedef struct {
    int id;
    int requested_resources;
    const char *workspace_path;
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
        "../server/nr_server",
        "-c",
        "/IdeaProjects/needyresources/files/config.json",
        NULL
    };

    printf("[BOOTSTRAP] Starting server...\n");
    pid_t server_pid = start_process(server_argv);

    if (server_pid < 0) {
        return EXIT_FAILURE;
    }

    sleep(SERVER_START_DELAY_SECONDS);

    client_scenario_t scenario[CLIENT_COUNT] = {
        {1, 2, "workspace_client_1"},
        {2, 2, "workspace_client_2"},
        {3, 1, "workspace_client_3"}
    };

    pid_t client_pids[CLIENT_COUNT];

    for (int i = 0; i < CLIENT_COUNT; i++) {
        char id_arg[16];
        char resources_arg[16];

        snprintf(id_arg, sizeof(id_arg), "%d", scenario[i].id);
        snprintf(resources_arg, sizeof(resources_arg), "%d", scenario[i].requested_resources);

        char *client_argv[] = {
            "../client/nr_client",
            "-i",
            id_arg,
            "-p",
            (char *)scenario[i].workspace_path,
            "-r",
            resources_arg,
            NULL
        };

        printf(
            "[BOOTSTRAP] Starting client %d requesting %d resources...\n",
            scenario[i].id,
            scenario[i].requested_resources
        );

        client_pids[i] = start_process(client_argv);

        if (client_pids[i] < 0) {
            printf("[BOOTSTRAP] Could not start client %d\n", scenario[i].id);
        }

        sleep(1);
    }

    for (int i = 0; i < CLIENT_COUNT; i++) {
        if (client_pids[i] > 0) {
            wait_for_client(client_pids[i], scenario[i].id);
        }
    }

    printf("[BOOTSTRAP] Stopping server...\n");
    kill(server_pid, SIGTERM);
    waitpid(server_pid, NULL, 0);


    printf("[BOOTSTRAP] Scenario finished.\n");

    return EXIT_SUCCESS;
}