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

#define NB_SERV_DIF 3    //puisqu'il n'y a que 3 services differents

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
        free(pipesClientServNames[i][0]);
        free(pipesClientServNames[i][1]);
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
    myassert(nb_serv <= 3, "Erreur : il n'existe (pour l'instant) que 3 services");

    // Pour la communication avec les clients
    // - création de 2 tubes nommés pour converser avec les clients
    // - création d'un sémaphore pour que deux clients ne
    //   ne communiquent pas en même temps avec l'orchestre
    srand(time(NULL));
    int ret;

    ret = mkfifo(ORCH_TO_CLIENT, 0644);     //creation pipe orch->client
    myassert(ret == 0, "echec de la creation du tube orch->client\n");
    ret = mkfifo(CLIENT_TO_ORCH, 0644);     //creation pipe client->orch
    myassert(ret == 0, "echec de la creation du tube client->orch\n");

    key_t key = ftok(CLIENT_ORCH, CLIENT_ORCH_KEY);     //cle pour sema client<-->orch
    myassert(key != -1, "echec de la creation de la cle pour le semaphore client<-->orch\n");

    int semClientOrch = semget(key, 1, IPC_CREAT | IPC_EXCL | 0641);    //creation sema client<-->orch
    myassert(semClientOrch != -1, "echec de la creation du semaphore client<-->orch (semClientOrch)\n");
    ret = semctl(semClientOrch, 0, SETVAL, 1);
    myassert(ret != -1, "echec de l'initialisation du semaphore client<-->orch (semClientOrch)\n");


    key = ftok(CLIENT_ORCH, CLIENT_DONE_KEY);     //cle pour sema attendre que le client ferme les tubes orch<-->client
    myassert(key != -1, "echec de la creation de la cle pour le semaphore client<-->orch\n");

    int ClientDone = semget(key, 1, IPC_CREAT | IPC_EXCL | 0641);    //creation sema client<-->orch
    myassert(ClientDone != -1, "echec de la creation du semaphore client<-->orch (semClientOrch)\n");
    ret = semctl(ClientDone, 0, SETVAL, 1);
    myassert(ret != -1, "echec de l'initialisation du semaphore client<-->orch (semClientOrch)\n");


    // lancement des services, avec pour chaque service :
    // - création d'un tube anonyme pour converser (orchestre vers service)
    // - un sémaphore pour que le service préviene l'orchestre de la
    //   fin d'un traitement
    // - création de deux tubes nommés (pour chaque service) pour les
    //   communications entre les clients et les services

    
    int pipesOrchToServ[nb_serv][2];     //orch-->services

    //creation pipe orch<-->service 
    for (int i = 0; i < nb_serv; i++){
        ret = pipe(pipesOrchToServ[i]);
        myassert(ret == 0, "echec de la creation du tube anonyme vers un service\n");
    }
    printf("Creation pipe orch<-->service OK\n");

    //creation tubes client<-->service
    for (int i = 0; i < nb_serv; i++){
        ret = mkfifo(pipesClientServNames[i][0], 0644);
        myassert(ret == 0, "echec de la creation d'un tube client-->service\n");
        ret = mkfifo(pipesClientServNames[i][1], 0644);
        myassert(ret == 0, "echec de la creation d'un tube client<--service\n");
    }
    printf("Creation tubes client<-->service OK\n");

    //creation semaphores orch<-->service
    key = ftok(ORCH_SERV, ORCH_SERV_KEY);
    myassert(key != -1, "echec de la creation de la cle pour le semaphore orch<-->serv\n");
    int semOrchServ = semget(key, nb_serv, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semOrchServ != -1, "echec de la creation du semaphore orch<-->serv\n");
    //initialisation sema orch<-->service
    for (int i = 0; i < nb_serv; i++){
        ret = semctl(semOrchServ, i, SETVAL, 1);
        myassert(ret != -1, "echec de l'iniitialisation du semaphore semOrchServ\n");
    }
    printf("Creation et init sema orch<-->service OK\n");

    //lancement des services
    for (int i = 0; i < nb_serv; i++){
        ret = fork();
        myassert(ret != -1, "echec de la 1ere duplication de l'orchestre\n");
        char * servArgv[7];
        servArgv[0] = NULL;
        servArgv[1] = NULL;   //num service
        servArgv[2] = NULL;   //cle sema serv<-->orch
        servArgv[3] = NULL;   //fd tube ano orch-->serv
        servArgv[4] = NULL;   //nom pour mkfifo serv-->client
        servArgv[5] = NULL;   //nom pour mkfifo client-->serv
        servArgv[6] = NULL;
        if (ret == 0){
            servArgv[0] = name;
            servArgv[1] = io_intToStr(i);   //num service
            servArgv[2] = io_intToStr(ORCH_SERV_KEY);   //cle sema serv<-->orch
            servArgv[3] = io_intToStr(pipesOrchToServ[i][0]);   //fd tube ano orch-->serv
            servArgv[4] = pipesClientServNames[i][1];   //nom pour mkfifo serv-->client
            servArgv[5] = pipesClientServNames[i][0];   //nom pour mkfifo client-->serv
            servArgv[6] = NULL;
            execv(servArgv[0], servArgv);
            myassert(false, "erreur : retour dans orchestre apres execv()\n");
        }
        ret = close(pipesOrchToServ[i][0]);    //fermeture entree lecture
        myassert(ret == 0, "echec de la fermeture de l'entree lecture de pipeOrchToServ\n");
        for (int i = 0; i < 4; i++)
            free(servArgv[i]);
        printf("Lancement du service %d OK\n", i);
    }
    printf("Lancement des services OK\n");

    while (! fin)
    {
        // ouverture ici des tubes nommés avec un client
        // attente d'une demande de service du client
        printf("Entrée dans la boucle de l'orch OK\n");

        int pipeOrchToClient = open(ORCH_TO_CLIENT, O_WRONLY);
        myassert(pipeOrchToClient != -1, "echec de l'ouverture du tube pipeOrchToClient en ecriture\n");
        printf("Ouverture pipeOrchToClient OK\n");

        int pipeClientToOrch = open(CLIENT_TO_ORCH, O_RDONLY);
        myassert(pipeClientToOrch != -1, "echec de l'ouverture du tube pipeClientToOrch en lecture\n");
        printf("Ouverture pipeClientToOrch OK\n");

        int serv;
        ret = read(pipeClientToOrch, &serv, sizeof(int));
        myassert(ret != 0, "echec de la lecture de la demande de service dans le tube pipeClientToOrch\n");
        myassert(ret == sizeof(int), "erreur dans la lecture de la demande de service\n");
        printf("Pas de client, on n'est pas censé arriver ici pour l'instant...\n");

        // détecter la fin des traitements lancés précédemment via
        // les sémaphores dédiés (attention on n'attend pas la
        // fin des traitement, on note juste ceux qui sont finis)
        bool servDone[nb_serv];

        for (int i = 0; i < nb_serv; i++){
            ret = semctl(semOrchServ, i, GETVAL);
            myassert(ret != -1, "echec de la recuperation de la valeur du semaphore semOrchServ\n");
            servDone[i] = ret == 1;
        }
        printf("recup etat des services OK\n");

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
        int send;
        switch (serv){
            case SERVICE_ARRET :
                send = VALIDATION_CODE;
                fin = true;
                break;
            case SERVICE_SOMME :
                if (servDone[SERVICE_SOMME] && config_isServiceOpen(SERVICE_SOMME)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            case SERVICE_COMPRESSION :
                if (servDone[SERVICE_COMPRESSION] && config_isServiceOpen(SERVICE_COMPRESSION)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            case SERVICE_SIGMA :
                if (servDone[SERVICE_SIGMA] && config_isServiceOpen(SERVICE_SIGMA)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            default : send = ERROR_CODE;
        }
        ret = write(pipeOrchToClient, &send, sizeof(int));
        myassert(ret != -1, "echec de l'envoi du code au client\n");
        printf("envoi code service OK\n");

        if (send == VALIDATION_CODE){
            int password = rand();
            ret = write(pipesOrchToServ[serv][1], &serv, sizeof(int));
            myassert(ret != -1, "echec de l'envoi du code de travail\n");
            ret = write(pipesOrchToServ[serv][1], &password, sizeof(int));
            myassert(ret != -1, "echec de l'envoi du mot de passe\n");

            ret = write(pipeOrchToClient, &password, sizeof(int));
            myassert(ret != -1, "echec de l'envoi du mot de passe au client\n");

            int len = strlen(pipesClientServNames[serv][0]);

            ret = write(pipeOrchToClient, &len, sizeof(int));
            myassert(ret != -1, "echec de l'envoi de la taille du nom du tube");
            ret = write(pipeOrchToClient, &(pipesClientServNames[serv][0]), sizeof(char) * len);
            myassert(ret != -1, "echec de l'envoi du tube client-->serv\n");

            len = strlen(pipesClientServNames[serv][0]);

            ret = write(pipeOrchToClient, &len, sizeof(int));
            myassert(ret != -1, "echec de l'envoi de la taille du nom du tube");
            ret = write(pipeOrchToClient, &(pipesClientServNames[serv][1]), sizeof(char) * len);
            myassert(ret != -1, "echec de l'envoi du tube client<--serv\n");
        }


        // attente d'un accusé de réception du client
        int recep;
        ret = read(pipeClientToOrch, &recep, sizeof(int));
        myassert(ret != -1, "echec accuse de reception\n");
        // fermer les tubes vers le client
        close(pipeOrchToClient);
        close(pipeClientToOrch);

        // il peut y avoir un problème si l'orchestre revient en haut de la
        // boucle avant que le client ait eu le temps de fermer les tubes
        // il faut attendre avec un sémaphore.
        // (en attendant on fait une attente d'1 seconde, à supprimer dès
        // que le sémaphore est en place)
        // attendre avec un sémaphore que le client ait fermé les tubes
        struct sembuf op = {0, -1, 0};
        ret = semop(ClientDone, &op, 1);
        myassert(ret != -1, "echec de l'operation sur le semaphore semClientOrch\n");
    }

    // attente de la fin des traitements en cours (via les sémaphores)
    for (int i = 0; i < SERVICE_NB; i++){
        struct sembuf op = {i, -1, 0};
        ret = semop(semClientOrch, &op, SERVICE_NB);
        myassert(ret != -1, "echec de l'operation sur le semaphore semClientOrch\n");
    }

    // envoi à chaque service d'un code de fin
    int end = SERVICE_ARRET;
    for (int i = 0; i < nb_serv; i++){
        ret = write(pipesOrchToServ[i][1], &end, sizeof(int));
        myassert(ret != -1, "echec de l'envoi du code de fin a un des serv\n");
    }

    // attente de la terminaison des processus services
    for (int i = 0; i < SERVICE_NB; i++){
        wait(NULL);
    }

    // libération des ressources
    free(name);

    ret = unlink(ORCH_TO_CLIENT);   //destruction du tube orch->client
    myassert(ret != -1, "echec de la destruction du tube orch->client\n");
    ret = unlink(CLIENT_TO_ORCH);   //destruction du tube client->orch
    myassert(ret != -1, "echec de la destruction du tube client->orch\n");
    ret = unlink(CLIENT_TO_SERV_SUM);
    myassert(ret != -1, "echec de la destruction du tube client->serv_sum\n");
    ret = unlink(SERV_SUM_TO_CLIENT);
    myassert(ret != -1, "echec de la destruction du tube serv_sum->client\n");
    ret = unlink(CLIENT_TO_SERV_COMP);
    myassert(ret != -1, "echec de la destruction du tube client->serv_comp\n");
    ret = unlink(SERV_COMP_TO_CLIENT);
    myassert(ret != -1, "echec de la destruction du tube serv_comp->client\n");
    ret = unlink(CLIENT_TO_SERV_SIG);
    myassert(ret != -1, "echec de la destruction du tube client->serv_sig\n");
    ret = unlink(SERV_SIG_TO_CLIENT);
    myassert(ret != -1, "echec de la destruction du tube serv_sig->client\n");

    ret = semctl(semClientOrch, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore client<-->orch\n");
    
    ret = semctl(ClientDone, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore client<-->client\n");
    
    ret = semctl(semOrchServ, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore orch<-->serv\n");

    destroy_pipesClientServNames();

    return EXIT_SUCCESS;
}
