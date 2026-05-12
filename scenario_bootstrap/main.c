#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <needy.h>

int main(int argc, char *argv[]) {
    int opt;
    size_t c;
    while ((opt = getopt(argc, argv, "hp:r:")) != -1) {
        switch (opt) {
            case 's':
                c = strtol(optarg, NULL, 10);
                break;
            case 'h':
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-i CLIENT_ID (must be at least 1)] [-p /path/to/workspace/folder] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    char commm[40];
    snprintf(commm, sizeof(commm),
        "echo \"case = %ld\"",c);
    system(commm);
}