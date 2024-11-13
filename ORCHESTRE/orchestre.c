#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "../CONFIG/config.h"
#include "../CLIENT_ORCHESTRE/client_orchestre.h"
#include "../ORCHESTRE_SERVICE/orchestre_service.h"
#include "../SERVICE/service.h"
#include "../UTILS/myassert.h"


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


    // lancement des services, avec pour chaque service :
    // - création d'un tube anonyme pour converser (orchestre vers service)
    // - un sémaphore pour que le service préviene l'orchestre de la
    //   fin d'un traitement
    // - création de deux tubes nommés (pour chaque service) pour les
    //   communications entre les clients et les services
    //creation pipe et sema orch<-->service somme
    ret = pipe(pipeOrchToServ0);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service somme\n");
    key = ftok(ORCH_SERV, ORCH_SERV_SUM_KEY);
    myassert(key != -1, "echec de la creation de la cle pour le semaphore orch<-->serv_somme\n");
    int semOrchServSum = semget(key, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semOrchServSum != -1, "echec de la creation du semaphore orch<-->serv_somme (semOrchServSum)\n");

    //creation pipe et sema orch<-->service compression
    ret = pipe(pipeOrchToServ1);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service compression\n");
    key = ftok(ORCH_SERV, ORCH_SERV_COMP_KEY);
    myassert(key != -1, "echec de la creation de la cle pour le semaphore orch<-->serv_compression\n");
    int semOrchServComp = semget(key, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semOrchServComp != -1, "echec de la creation du semaphore orch<-->serv_compression (semOrchServComp)\n");

    //creation pipe et sema orch<-->service sigma
    ret = pipe(pipeOrchToServ2);
    myassert(ret == 0, "echec de la creation du tube anonyme vers le service sigma\n");
    key = ftok(ORCH_SERV, ORCH_SERV_SIG_KEY);
    myassert(key != -1, "echec de la creation de la cle pour le semaphore orch<-->serv_sigma\n");
    int semOrchServSigma = semget(key, IPC_CREAT | IPC_EXCL | 0641);
    myassert(semOrchServSigma != -1, "echec de la creation du semaphore orch<-->serv_sigma (semOrchServSigma)\n");

    //service 0 : somme
    ret = fork();
    myassert(ret != -1, "echec de la 1ere duplication de l'orchestre\n");
    if (ret == 0){
        char * servArgv[6];
        servArgv[0] = "../SERVICE/service";
        sprintf(servArgv[1], "%d", ORCH_SERV_SUM_KEY);
        sprintf(servArgv[2], "%d", ORCH_SERV_SUM_KEY);
        sprintf(servArgv[3], "%d", );
        sprintf(servArgv[4], "%d", );
        sprintf(servArgv[5], "%d", );
        execv(servArgv[0], servArgv);
        myassert(false, "erreur : retour dans orchestre apres execv(service somme)\n");
    }


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


        // attente d'un accusé de réception du client
        // fermer les tubes vers le client
        close(pipeOrchToClient);
        close(pipeClientToOrch);

        // il peut y avoir un problème si l'orchestre revient en haut de la
        // boucle avant que le client ait eu le temps de fermer les tubes
        // il faut attendre avec un sémaphore.
        // (en attendant on fait une attente d'1 seconde, à supprimer dès
        // que le sémaphore est en place)
        // attendre avec un sémaphore que le client ait fermé les tubes
        sleep(1);   // à supprimer
    }

    // attente de la fin des traitements en cours (via les sémaphores)

    // envoi à chaque service d'un code de fin

    // attente de la terminaison des processus services

    // libération des ressources
    ret = unlink(ORCH_TO_CLIENT);   //destruction du tube orch->client
    myassert(ret != -1, "echec de la destruction du tube orch->client\n");
    ret = unlink(CLIENT_TO_ORCH);   //destruction du tube client->orch
    myassert(ret != -1, "echec de la destruction du tube client->orch\n");

    ret = semctl(semClientOrch, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore client<-->orch\n");

    return EXIT_SUCCESS;
}
