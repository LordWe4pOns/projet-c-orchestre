#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "../UTILS/io.h"
#include "../UTILS/memory.h"
#include "../UTILS/myassert.h"

#include "../SERVICE/service.h"
#include "../CLIENT_ORCHESTRE/client_orchestre.h"
#include "../CLIENT_SERVICE/client_service.h"

#include "../CLIENT/client_arret.h"
#include "../CLIENT/client_somme.h"
#include "../CLIENT/client_sigma.h"
#include "../CLIENT/client_compression.h"


static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <num_service> ...\n", exeName);
    fprintf(stderr, "        <num_service> : entre -1 et %d\n", SERVICE_NB - 1);
    fprintf(stderr, "                        -1 signifie l'arrêt de l'orchestre\n");
    fprintf(stderr, "        ...           : les paramètres propres au service\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

int main(int argc, char * argv[])
{
    if (argc < 2)
        usage(argv[0], "nombre paramètres incorrect");

    int numService = io_strToInt(argv[1]);
    if (numService < -1 || numService >= SERVICE_NB)
        usage(argv[0], "numéro service incorrect");

    // appeler la fonction de vérification des arguments
    //     une fct par service selon numService
    //            . client_arret_verifArgs
    //         ou . client_somme_verifArgs
    //         ou . client_compression_verifArgs
    //         ou . client_sigma_verifArgs
    switch (numService){
        case SERVICE_ARRET : client_arret_verifArgs(argc, argv);
        case SERVICE_SOMME : client_somme_verifArgs(argc, argv);
        case SERVICE_COMPRESSION : client_compression_verifArgs(argc, argv);
        case SERVICE_SIGMA : client_sigma_verifArgs(argc, argv);
    }

    // initialisations diverses s'il y a lieu
    int ret;

    key_t key = ftok(CLIENT_ORCH, CLIENT_ORCH_KEY);     //cle pour sema client<-->orch
    myassert(key != -1, "echec de la creation de la cle pour le semaphore client<-->orch\n");

    int semClientOrch = semget(key, 1, 0);    //recup sema client<-->orch
    myassert(semClientOrch != -1, "echec de la recuperation du semaphore client<-->orch (semClientOrch)\n");


    // entrée en section critique pour communiquer avec l'orchestre
    struct sembuf accessOrch = {0, -1, 0};
    ret = semop(semClientOrch, &accessOrch, 1);
    myassert(ret != -1, "echec de l'acces au semaphore client<-->orch (semClientOrch)");
    printf("Mise en place de la communication avec l'orchestre...\n");

    // ouverture des tubes avec l'orchestre
    int pipeOrchToClient = open(ORCH_TO_CLIENT, 'r');
    myassert(pipeOrchToClient != -1, "echec de l'ouverture du tube pipeOrchToClient en lecture\n");

    int pipeClientToOrch = open(CLIENT_TO_ORCH, 'w');
    myassert(pipeClientToOrch != -1, "echec de l'ouverture du tube pipeClientToOrch en ecriture\n");


    // envoi à l'orchestre du numéro du service

    // attente code de retour
    // si code d'erreur
    //     afficher un message erreur
    // sinon si demande d'arrêt (i.e. numService == -1)
    //     afficher un message
    // sinon
    //     récupération du mot de passe et des noms des 2 tubes
    // finsi
    //
    // envoi d'un accusé de réception à l'orchestre
    // fermeture des tubes avec l'orchestre
    // on prévient l'orchestre qu'on a fini la communication (cf. orchestre.c)
    // sortie de la section critique
    //
    // si pas d'erreur et service normal
    //     ouverture des tubes avec le service
    //     envoi du mot de passe au service
    //     attente de l'accusé de réception du service
    //     si mot de passe non accepté
    //         message d'erreur
    //     sinon
    //         appel de la fonction de communication avec le service :
    //             une fct par service selon numService :
    //                    . client_somme
    //                 ou . client_compression
    //                 ou . client_sigma
    //         envoi d'un accusé de réception au service
    //     finsi
    //     fermeture des tubes avec le service
    // finsi

    // libération éventuelle de ressources

    return EXIT_SUCCESS;
}
