#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include "../CONFIG/config.h"
#include "../CLIENT_ORCHESTRE/client_orchestre.h"
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../SERVICE/service.h"
#include "../UTILS/myassert.h"
#include "../UTILS/io.h"

#include "../CLIENT_SERVICE/client_service.h"

#define NB_SERV_DIF 3    //puisqu'il n'y a que 3 services differents (pour l'instant)

char*** pipesClientServNames = NULL;

void init_pipeClientServNames(){
    myassert(pipesClientServNames == NULL, "Erreur : pipesClientServNames deja initialisé");
    pipesClientServNames = malloc(sizeof(char**) * NB_SERV_DIF);
    for (int i = 0; i < NB_SERV_DIF; i++){
        pipesClientServNames[i] = malloc(sizeof(char*) * 2);
    }
    pipesClientServNames[0][0] = CLIENT_TO_SERV_SUM;
    pipesClientServNames[0][1] = SERV_SUM_TO_CLIENT;
    pipesClientServNames[1][0] = CLIENT_TO_SERV_COMP;
    pipesClientServNames[1][1] = SERV_COMP_TO_CLIENT;
    pipesClientServNames[2][0] = CLIENT_TO_SERV_SIG;
    pipesClientServNames[2][1] = SERV_SIG_TO_CLIENT;
}

void destroy_pipesClientServNames(){
    myassert(pipesClientServNames != NULL, "Erreur : pipesClientServNames n'est pas initialisé");
    for (int i = 0; i < NB_SERV_DIF; i++){
        free(pipesClientServNames[i]);
    }
    free(pipesClientServNames);
    pipesClientServNames = NULL;
}



static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <fichier config>\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if (argc != 2)
        usage(argv[0], "nombre paramètres incorrect");

    bool fin = false;

    // init des noms des fifo client<-->serv
    init_pipeClientServNames();

    // lecture du fichier de configuration
    config_init(argv[1]);
    char* name = malloc(sizeof(char) * strlen(config_getExeName()) + 1);
    strcpy(name, config_getExeName());
    int nb_serv = config_getNbServices();

    // Pour la communication avec les clients
    // - création de 2 tubes nommés pour converser avec les clients
    // - création d'un sémaphore pour que deux clients ne
    //   ne communiquent pas en même temps avec l'orchestre
    srand(time(NULL));
    int ret;

    my_mkfifo(ORCH_TO_CLIENT);     //creation pipe orch->client
    my_mkfifo(CLIENT_TO_ORCH);     //creation pipe client->orch

    key_t key = my_ftok(CLIENT_ORCH, CLIENT_ORCH_KEY);     //cle pour sema client<-->orch
    int semClientOrch = my_semCreate(key, 1);    //creation sema client<-->orch
    my_semSetVal(semClientOrch, 0, 1);

    key = my_ftok(CLIENT_ORCH, CLIENT_DONE_KEY);     //cle pour sema attendre que le client ferme les tubes orch<-->client
    int ClientDone = my_semCreate(key, 1);    //creation sema client<-->orch
    my_semSetVal(ClientDone, 0, 1);
    

    // lancement des services, avec pour chaque service :
    // - création d'un tube anonyme pour converser (orchestre vers service)
    // - un sémaphore pour que le service préviene l'orchestre de la
    //   fin d'un traitement
    // - création de deux tubes nommés (pour chaque service) pour les
    //   communications entre les clients et les services

    
    int pipesOrchToServ[nb_serv][2];     // orch-->services

    // creation pipe orch<-->service 
    for (int i = 0; i < nb_serv; i++){
        my_pipe(pipesOrchToServ[i]);
    }

    // creation tubes client<-->service
    for (int i = 0; i < nb_serv; i++){
        my_mkfifo(pipesClientServNames[i][0]);
        my_mkfifo(pipesClientServNames[i][1]);
    }

    // creation semaphores orch<-->service
    key = my_ftok(ORCH_SERV, ORCH_SERV_KEY);
    int semOrchServ = my_semCreate(key, nb_serv);
    // initialisation sema orch<-->service
    for (int i = 0; i < nb_serv; i++){
        my_semSetVal(semOrchServ, i, 1);
    }

    // lancement des services
    for (int i = 0; i < nb_serv; i++){
        ret = fork();
        myassert(ret != -1, "echec de la duplication de l'orchestre\n");
        if (ret == 0){
            char * servArgv[7];
            servArgv[0] = name;
            servArgv[1] = io_intToStr(i);   // num service
            servArgv[2] = io_intToStr(ORCH_SERV_KEY);   // cle sema serv<-->orch
            servArgv[3] = io_intToStr(pipesOrchToServ[i][0]);   // fd tube ano orch-->serv
            servArgv[4] = pipesClientServNames[i][1];   // nom pour mkfifo serv-->client
            servArgv[5] = pipesClientServNames[i][0];   // nom pour mkfifo client-->serv
            servArgv[6] = NULL;
            execv(servArgv[0], servArgv);
            myassert(false, "erreur : retour dans orchestre apres execv()\n");
        }
        my_close(pipesOrchToServ[i][0]);    // fermeture entree lecture
    }

    while (! fin)
    {
        // ouverture ici des tubes nommés avec un client
        // attente d'une demande de service du client

        printf("Orchestre : En attente d'un Client...\n");
        int pipeOrchToClient = my_open(ORCH_TO_CLIENT, O_WRONLY);
        int pipeClientToOrch = my_open(CLIENT_TO_ORCH, O_RDONLY);
        
        int serv;
        my_read(pipeClientToOrch, &serv, sizeof(int));
        
        // détecter la fin des traitements lancés précédemment via
        // les sémaphores dédiés (attention on n'attend pas la
        // fin des traitement, on note juste ceux qui sont finis)
        bool servDone[nb_serv];

        for (int i = 0; i < nb_serv; i++){
            servDone[i] = my_semGetVal(semOrchServ, i);
        }

        // analyse de la demande du client
        // si ordre de fin
        //     envoi au client d'un code d'acceptation (via le tube nommé)
        //     marquer le booléen de fin de la boucle
        // sinon si service non ouvert
        //     envoi au client d'un code d'erreur (via le tube nommé)
        // sinon si service déjà en cours de traitement
        //     envoi au client d'un code d'erreur (via le tube nommé)
        // sinon
        //     envoi au client d'un code d'acceptation (via le tube nommé)
        //     génération d'un mot de passe
        //     envoi d'un code de travail au service (via le tube anonyme)
        //     envoi du mot de passe au service (via le tube anonyme)
        //     envoi du mot de passe au client (via le tube nommé)
        //     envoi des noms des tubes nommés au client (via le tube nommé)
        // finsi
        int code;
        switch (serv){
            case SERVICE_ARRET :
                code = VALIDATION_CODE;
                fin = true;
                printf("Orchestre : demande d'arret reçue\n");
                break;
            case SERVICE_SOMME :
                if (servDone[SERVICE_SOMME] && config_isServiceOpen(SERVICE_SOMME)){
                    code = VALIDATION_CODE;
                } else {
                    code = ERROR_CODE;
                }
                break;
            case SERVICE_COMPRESSION :
                if (servDone[SERVICE_COMPRESSION] && config_isServiceOpen(SERVICE_COMPRESSION)){
                    code = VALIDATION_CODE;
                } else {
                    code = ERROR_CODE;
                }
                break;
            case SERVICE_SIGMA :
                if (servDone[SERVICE_SIGMA] && config_isServiceOpen(SERVICE_SIGMA)){
                    code = VALIDATION_CODE;
                } else {
                    code = ERROR_CODE;
                }
                break;
            default : code = ERROR_CODE;
        }
        my_write(pipeOrchToClient, &code, sizeof(int));

        if (code == VALIDATION_CODE && serv >= 0){
            int password = rand();
            my_write(pipesOrchToServ[serv][1], &serv, sizeof(int));
            my_write(pipesOrchToServ[serv][1], &password, sizeof(int));
            my_write(pipeOrchToClient, &password, sizeof(int));

            int len = strlen(pipesClientServNames[serv][0]);
            my_write(pipeOrchToClient, &len, sizeof(int));
            my_write(pipeOrchToClient, pipesClientServNames[serv][0], sizeof(char) * len);

            len = strlen(pipesClientServNames[serv][0]);
            my_write(pipeOrchToClient, &len, sizeof(int));
            my_write(pipeOrchToClient, pipesClientServNames[serv][1], sizeof(char) * len);
        }


        // attente d'un accusé de réception du client
        int recep;
        my_read(pipeClientToOrch, &recep, sizeof(int));
        // fermer les tubes vers le client
        my_close(pipeOrchToClient);
        my_close(pipeClientToOrch);

        // il peut y avoir un problème si l'orchestre revient en haut de la
        // boucle avant que le client ait eu le temps de fermer les tubes
        // il faut attendre avec un sémaphore.
        // (en attendant on fait une attente d'1 seconde, à supprimer dès
        // que le sémaphore est en place)
        // attendre avec un sémaphore que le client ait fermé les tubes
        
        struct sembuf op = {0, -1, 0};
        my_semop(ClientDone, op);
        struct sembuf op_done = {0, 1, 0};
        my_semop(ClientDone, op_done);
        printf("Orchestre : fin de l'opération\n");
    }

    // attente de la fin des traitements en cours (via les sémaphores)
    for (int i = 0; i < nb_serv; i++){
        struct sembuf op = {i, -1, 0};
        my_semop(semOrchServ, op);
        printf("Orchestre : le Service %d a fini\n", i);
    }

    // envoi à chaque service d'un code de fin
    int end = SERVICE_ARRET;
    for (int i = 0; i < nb_serv; i++){
        my_write(pipesOrchToServ[i][1], &end, sizeof(int));
        printf("Orchestre : demande d'arret envoyée au service %d\n", i);
    }

    // attente de la terminaison des processus services
    for (int i = 0; i < nb_serv; i++){
        printf("Orchestre : attente de la fin de %d service(s)\n", nb_serv - i);
        wait(NULL);
    }

    // libération des ressources
    free(name);

    my_unlink(ORCH_TO_CLIENT);
    my_unlink(CLIENT_TO_ORCH);
    my_unlink(CLIENT_TO_SERV_SUM);
    my_unlink(SERV_SUM_TO_CLIENT);
    my_unlink(CLIENT_TO_SERV_COMP);
    my_unlink(SERV_COMP_TO_CLIENT);
    my_unlink(CLIENT_TO_SERV_SIG);
    my_unlink(SERV_SIG_TO_CLIENT);

    my_semDestroy(semClientOrch);
    my_semDestroy(ClientDone);
    my_semDestroy(semOrchServ);

    destroy_pipesClientServNames();
    printf("Orchestre : Arret\n");

    return EXIT_SUCCESS;
}
