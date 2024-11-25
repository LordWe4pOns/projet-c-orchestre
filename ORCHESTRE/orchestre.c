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

#include "../CLIENT_SERVICE/client_service.h"


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

    // lecture du fichier de configuration
    config_init(argv[1]);

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

    //creation pipe orch<-->service somme
    ret = pipe(pipeOrchToServ0);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service somme\n");
    //creation pipe orch<-->service compression
    ret = pipe(pipeOrchToServ1);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service compression\n");
    //creation pipe orch<-->service sigma
    ret = pipe(pipeOrchToServ2);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service sigma\n");

    //creation tubes client<-->service_somme
    ret = mkfifo(CLIENT_TO_SERV_SUM, 0644);
    myassert(ret == 0, "echec de la creation du tube client-->service_somme\n");
    ret = mkfifo(SERV_SUM_TO_CLIENT, 0644);
    myassert(ret == 0, "echec de la creation du tube client<--service_somme\n");
    //creation tubes client<-->service_compression
    ret = mkfifo(CLIENT_TO_SERV_COMP, 0644);
    myassert(ret == 0, "echec de la creation du tube client-->service_compression\n");
    ret = mkfifo(SERV_COMP_TO_CLIENT, 0644);
    myassert(ret == 0, "echec de la creation du tube client<--service_compression\n");
    //creation tubes client<-->service_sigma
    ret = mkfifo(CLIENT_TO_SERV_SIG, 0644);
    myassert(ret == 0, "echec de la creation du tube client-->service_sigma\n");
    ret = mkfifo(SERV_SIG_TO_CLIENT, 0644);
    myassert(ret == 0, "echec de la creation du tube client<--service_sigma\n");

    //creation semaphores orch<-->service
    key = ftok(ORCH_SERV, ORCH_SERV_KEY);
    myassert(key != -1, "echec de la creation de la cle pour le semaphore orch<-->serv\n");
    int semOrchServ = semget(key, config_getNbServices(), IPC_CREAT | IPC_EXCL | 0641);
    myassert(semOrchServ != -1, "echec de la creation du semaphore orch<-->serv\n");
    //initialisation sema orch<-->service
    ret = semctl(semOrchServ, 0, SETALL, 1);
    myassert(ret != -1, "echec de l'iniitialisation du semaphore semOrchServ\n");

    //service 0 : somme
    ret = fork();
    myassert(ret != -1, "echec de la 1ere duplication de l'orchestre\n");
    if (ret == 0){
        char * servArgv[7];
        servArgv[0] = config_getExeName();
        sprintf(servArgv[1], "%d", SERVICE_SOMME);   //num service
        sprintf(servArgv[2], "%d", ORCH_SERV_KEY);   //cle sema serv<-->orch
        sprintf(servArgv[3], "%d", pipeOrchToServ0[0]);   //fd tube ano orch-->serv
        servArgv[4] = SERV_SUM_TO_CLIENT;   //mkfifo serv-->client
        servArgv[5] = CLIENT_TO_SERV_SUM;   //mkfifo client-->serv
        servArgv[6] = NULL;
        execv(servArgv[0], servArgv);
        myassert(false, "erreur : retour dans orchestre apres execv(service somme)\n");
    }
    ret = close(pipeOrchToServ0[0]);    //fermeture entree lecture
    myassert(ret == 0, "echec de la fermeture de l'entree lecture de pipeOrchToServ0\n");

    //service 1 : compression
    ret = fork();
    myassert(ret != -1, "echec de la 2eme duplication de l'orchestre\n");
    if (ret == 0){
        char * servArgv[7];
        servArgv[0] = config_getExeName();
        sprintf(servArgv[1], "%d", SERVICE_COMPRESSION);   //num service
        sprintf(servArgv[2], "%d", ORCH_SERV_KEY);   //cle sema serv<-->orch
        sprintf(servArgv[3], "%d", pipeOrchToServ1[0]);   //fd tube ano orch-->serv
        servArgv[4] = SERV_COMP_TO_CLIENT;   //mkfifo serv-->client
        servArgv[5] = CLIENT_TO_SERV_COMP;   //mkfifo client-->serv
        servArgv[6] = NULL;
        execv(servArgv[0], servArgv);
        myassert(false, "erreur : retour dans orchestre apres execv(service compression)\n");
    }
    ret = close(pipeOrchToServ1[0]);    //fermeture entree lecture
    myassert(ret == 0, "echec de la fermeture de l'entree lecture de pipeOrchToServ0\n");

    //service 2 : sigma
    ret = fork();
    myassert(ret != -1, "echec de la 3eme duplication de l'orchestre\n");
    if (ret == 0){
        char * servArgv[7];
        servArgv[0] = config_getExeName();
        sprintf(servArgv[1], "%d", SERVICE_SIGMA);   //num service
        sprintf(servArgv[2], "%d", ORCH_SERV_KEY);   //cle sema serv<-->orch
        sprintf(servArgv[3], "%d", pipeOrchToServ2[0]);   //fd tube ano orch-->serv
        servArgv[4] = SERV_SIG_TO_CLIENT;   //mkfifo serv-->client
        servArgv[5] = CLIENT_TO_SERV_SIG;   //mkfifo client-->serv
        servArgv[6] = NULL;
        execv(servArgv[0], servArgv);
        myassert(false, "erreur : retour dans orchestre apres execv(service sigma)\n");
    }
    ret = close(pipeOrchToServ2[0]);    //fermeture entree lecture
    myassert(ret == 0, "echec de la fermeture de l'entree lecture de pipeOrchToServ0\n");

    while (! fin)
    {
        // ouverture ici des tubes nommés avec un client
        // attente d'une demande de service du client
        int pipeOrchToClient = open(ORCH_TO_CLIENT, 'w');
        myassert(pipeOrchToClient != -1, "echec de l'ouverture du tube pipeOrchToClient en ecriture\n");

        int pipeClientToOrch = open(CLIENT_TO_ORCH, 'r');
        myassert(pipeClientToOrch != -1, "echec de l'ouverture du tube pipeClientToOrch en lecture\n");

        int serv;
        ret = read(pipeClientToOrch, &serv, sizeof(int));
        myassert(ret != 0, "echec de la lecture de la demande de service dans le tube pipeClientToOrch\n");
        myassert(ret == sizeof(int), "erreur dans la lecture de la demande de service\n");

        // détecter la fin des traitements lancés précédemment via
        // les sémaphores dédiés (attention on n'attend pas la
        // fin des traitement, on note juste ceux qui sont finis)
        bool serv0, serv1, serv2;

        ret = semctl(semOrchServ, 0, GETVAL);
        myassert(ret != -1, "echec de la recuperation de la valeur du semaphore semOrchServ[0]\n");
        serv0 = ret == 1;
        ret = semctl(semOrchServ, 1, GETVAL);
        myassert(ret != -1, "echec de la recuperation de la valeur du semaphore semOrchServ[1]\n");
        serv1 = ret == 1;
        ret = semctl(semOrchServ, 2, GETVAL);
        myassert(ret != -1, "echec de la recuperation de la valeur du semaphore semOrchServ[2]\n");
        serv2 = ret == 1;

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
                if (serv0 && config_isServiceOpen(SERVICE_SOMME)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            case SERVICE_COMPRESSION :
                if (serv1 && config_isServiceOpen(SERVICE_COMPRESSION)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            case SERVICE_SIGMA :
                if (serv2 && config_isServiceOpen(SERVICE_SIGMA)){
                    send = VALIDATION_CODE;
                } else {
                    send = ERROR_CODE;
                }
                break;
            default : send = ERROR_CODE;
        }
        ret = write(pipeOrchToClient, &send, sizeof(int));
        myassert(ret != -1, "echec de l'envoi du code au client\n");

        if (send == VALIDATION_CODE){
            int password = rand();
            char tubeC2S[SIZE_FD + 1], tubeS2C[SIZE_FD + 1];
            switch (serv){
                case SERVICE_SOMME :
                    ret = write(pipeOrchToServ0[1], &serv, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du code de travail au serv0\n");
                    ret = write(pipeOrchToServ0[1], &password, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du mot de passe\n");
                    strcpy(tubeC2S, CLIENT_TO_SERV_SUM);
                    strcpy(tubeS2C, SERV_SUM_TO_CLIENT);
                    break;
                case SERVICE_COMPRESSION :
                    ret = write(pipeOrchToServ1[1], &serv, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du code de travail au serv1\n");
                    ret = write(pipeOrchToServ1[1], &password, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du mot de passe\n");
                    strcpy(tubeC2S, CLIENT_TO_SERV_COMP);
                    strcpy(tubeS2C, SERV_COMP_TO_CLIENT);
                    break;
                case SERVICE_SIGMA :
                    ret = write(pipeOrchToServ2[1], &serv, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du code de travail au serv2\n");
                    ret = write(pipeOrchToServ2[1], &password, sizeof(int));
                    myassert(ret != -1, "echec de l'envoi du mot de passe\n");
                    strcpy(tubeC2S, CLIENT_TO_SERV_SIG);
                    strcpy(tubeS2C, SERV_SIG_TO_CLIENT);
                    break;
                default : myassert(false, "erreur : numero de service incorrect\n");
            }
            ret = write(pipeOrchToClient, &password, sizeof(int));
            myassert(ret != -1, "echec de l'envoi du mot de passe au client\n");
            ret = write(pipeOrchToClient, &tubeC2S, sizeof(char) * SIZE_FD);
            myassert(ret != -1, "echec de l'envoi du tube client-->serv\n");
            ret = write(pipeOrchToClient, &tubeS2C, sizeof(char) * SIZE_FD);
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
    ret = write(pipeOrchToServ0[1], &end, sizeof(int));
    myassert(ret != -1, "echec de l'envoi du code de fin au serv0\n");
    ret = write(pipeOrchToServ1[1], &end, sizeof(int));
    myassert(ret != -1, "echec de l'envoi du code de fin au serv1\n");
    ret = write(pipeOrchToServ2[1], &end, sizeof(int));
    myassert(ret != -1, "echec de l'envoi du code de fin au serv2\n");

    // attente de la terminaison des processus services
    for (int i = 0; i < SERVICE_NB; i++){
        wait(NULL);
    }

    // libération des ressources
    ret = unlink(ORCH_TO_CLIENT);   //destruction du tube orch->client
    myassert(ret != -1, "echec de la destruction du tube orch->client\n");
    ret = unlink(CLIENT_TO_ORCH);   //destruction du tube client->orch
    myassert(ret != -1, "echec de la destruction du tube client->orch\n");

    ret = semctl(semClientOrch, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore client<-->orch\n");

    return EXIT_SUCCESS;
}
