#include <stdio.h>
#include <needy.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
/**
 * Sistemul se va baza pe o coadă de mesaje ce va conține toate cererile pt procesare
 *
 * Dpdv al arhitecturii, serverul este capabil de a se adapta în timp real la ANUMITE evenimente externe.
 * În particular, serverul își va putea actualiza dinamic lista de fișiere aflate în gestiune prin interogarea repetată
 * a sistemului de fișiere la intervale regulate.
 *
 * Sistemul va acționa, după caz:
 * 1. Dacă un fișier este adăugat în directorul de lucru, programul va vedea și își va actualiza baza sa de resurse
 * 2. Dacă un fișier este eliminat din directorul de lucru, programul va opri clientul ce deține fișierul și va curăța încă odată fișierul în cazul în care mai apare ca
 * urmare a unor operații de scriere din cadrul procesului client.
 *
 * Evident, acest lucru nu protejează împotriva unor potențiale pierderi de date dacă fișierul este șters și readăugat între momentul de actualizare a bazei de resurse și finalizarea
 * curățării.
 */

volatile bool shouldQuit = false;
void terminate_handler(__attribute_maybe_unused__ int signal) {
    shouldQuit = true;
}

int main(int argc, char* const* argv)
{
    //signal(SIGINT, terminate_handler);
    //signal(SIGTERM, terminate_handler);

    int opt;
    bool version_flag = false;
    char* config_path = strdup("config.json");

    bool stop_parse = false;
    while ((opt = getopt(argc, argv, "hvc:")) != -1 && !stop_parse) {
        switch (opt) {
            case 'c':
                config_path = strdup(optarg);
                break;
            case 'v':
                version_flag = true;
                stop_parse=true;
                break;
            case 'h':
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-c /path/to/config.json] \n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }


    if (version_flag) {
        printf("Needy Resources, version %s\n.Proiect pt SO2\nVersiune protocol needy: %u\nProfesori coordonatori: Florin-Teodor Fortiș, Diogen Babuc\nEchipa:$Name\nMembri:\n1.Alexandru Turculeț\n2.Mitoiu Bogdan Mitoiu\n3.Timeea Tătărușanu",SERVER_VERSION,NEEDY_PROTOCOL_VERSION);
        exit(EXIT_SUCCESS);
    }


    free(config_path);
    return 0;
}