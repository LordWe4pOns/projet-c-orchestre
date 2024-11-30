#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
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
        case SERVICE_ARRET : client_arret_verifArgs(argc, argv); break;
        case SERVICE_SOMME : client_somme_verifArgs(argc, argv); break;
        case SERVICE_COMPRESSION : client_compression_verifArgs(argc, argv); break;
        case SERVICE_SIGMA : client_sigma_verifArgs(argc, argv); break;
        default : myassert(false, "numero de service incorrect\n");
    }

    // initialisations diverses s'il y a lieu
    int ret;

    key_t key = ftok(CLIENT_ORCH, CLIENT_ORCH_KEY);     //cle pour sema client<-->orch
    myassert(key != -1, "echec de la creation de la cle pour le semaphore client<-->orch\n");

    int semClientOrch = semget(key, 1, 0);    //recup sema client<-->orch
    myassert(semClientOrch != -1, "echec de la recuperation du semaphore client<-->orch (semClientOrch)\n");


    key = ftok(CLIENT_ORCH, CLIENT_DONE_KEY);     //cle pour sema pour attendre que le client ferme les tubes orch<-->client
    myassert(key != -1, "echec de la creation de la cle pour le semaphore attente fermeture des tubes\n");

    int ClientDone = semget(key, 1, 0);    //creation sema pour attendre que le client ferme les tubes orch<-->client
    myassert(ClientDone != -1, "echec de la recuperation du semaphore attente fermeture des tubes\n");


    // entrée en section critique pour communiquer avec l'orchestre
    printf("Acces a la section critique...\n");
    struct sembuf accessOrch = {0, -1, 0};
    ret = semop(semClientOrch, &accessOrch, 1);
    myassert(ret != -1, "echec de l'acces au semaphore client<-->orch (semClientOrch)");
    printf("Acces a la section critique...\n");
    ret = semop(ClientDone, &accessOrch, 1);
    myassert(ret != -1, "echec de l'acces au semaphore client<-->orch (semClientOrch)");
    printf("Acces réussi\n");

    // ouverture des tubes avec l'orchestre
    printf("Connexion a l'orchestre...\n");
    int pipeOrchToClient = open(ORCH_TO_CLIENT, O_RDONLY);
    myassert(pipeOrchToClient != -1, "echec de l'ouverture du tube pipeOrchToClient en lecture\n");

    int pipeClientToOrch = open(CLIENT_TO_ORCH, O_WRONLY);
    myassert(pipeClientToOrch != -1, "echec de l'ouverture du tube pipeClientToOrch en ecriture\n");


    // envoi à l'orchestre du numéro du service
    ret = write(pipeClientToOrch, &numService, sizeof(int));
    myassert(ret != -1, "echec de l'ecriture de la demande de service dans le tube pipeClientToOrch\n");
    myassert(ret == sizeof(int), "erreur dans l'ecriture de la demande de service\n");

    // attente code de retour
    int retCode;
    ret = read(pipeOrchToClient, &retCode, sizeof(int));
    myassert(ret != -1, "echec de la recuperation du code de retour\n");

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
    int password;
    char* tubeC2S = NULL; 
    char* tubeS2C = NULL;
    if (retCode == ERROR_CODE){
        printf("Erreur : service non disponible\n");
    } else {
        if (numService == -1){
            printf("Demande d'arret de l'orchestre validé\n");
        } else {
            ret = read(pipeOrchToClient, &password, sizeof(int));
            myassert(ret != -1, "echec de la reception du mot de passe\n");

            int len;

            ret = read(pipeOrchToClient, &len, sizeof(int));
            myassert(ret == sizeof(int), "echec de la reception de la taille du nom du tube C2S");
            printf("len = %d\n", len);
            tubeC2S = malloc(sizeof(char) * (len + 1));
            ret = read(pipeOrchToClient, tubeC2S, sizeof(char) * len);
            myassert(ret != -1, "echec de la reception du tube client-->serv\n");
            printf("tubeC2S = **%s**\n", tubeC2S);
            tubeC2S[len] = '\0';

            ret = read(pipeOrchToClient, &len, sizeof(int));
            myassert(ret == sizeof(int), "echec de la reception de la taille du nom du tube S2C");
            tubeS2C = malloc(sizeof(char) * (len + 1));
            ret = read(pipeOrchToClient, tubeS2C, sizeof(char) * len);
            myassert(ret != -1, "echec de la reception du tube client<--serv\n");
            printf("tubeS2C = **%s**\n", tubeS2C);
            tubeS2C[len] = '\0';
        }
    }

    int code = VALIDATION_CODE;
    ret = write(pipeClientToOrch, &code, sizeof(int));
    myassert(ret != -1, "echec de l'envoi de l'accuse de reception\n");

    ret = close(pipeClientToOrch);
    myassert(ret != -1, "echec de la fermeture du tube pipeClientToOrch\n");
    ret = close(pipeOrchToClient);
    myassert(ret != -1, "echec de la fermeture du tube pipeOrchToClient\n");

    struct sembuf op = {0, 1, 0};
    ret = semop(ClientDone, &op, 1);
    myassert(ret != -1, "echec de l'acces au semaphore client<-->orch (semClientOrch)");
    ret = semop(semClientOrch, &op, 1);
    myassert(ret != -1, "echec de l'acces au semaphore client<-->orch (semClientOrch)");

    if (retCode != ERROR_CODE && numService != -1){
        int pipeServToClient = open(tubeS2C, O_RDONLY);     //ouverture tube serv-->client
        myassert(pipeServToClient != -1, "echec de l'ouverture du tube pipeServToClient");
        int pipeClientToServ = open(tubeC2S, O_WRONLY);     //ouverture tube client-->serv
        myassert(pipeClientToServ != -1, "echec de l'ouverture du tube pipeClientToServ");
        printf("pipeServToClient = %d\n", pipeServToClient);
        printf("pipeClientToServ = %d\n", pipeClientToServ);
 
        ret = write(pipeClientToServ, &password, sizeof(int));
        myassert(ret == sizeof(int), "echec de l'envoi du mot de passe au service\n");

        ret = read(pipeServToClient, &retCode, sizeof(int));
        myassert(ret == sizeof(int), "echec de la reception de l'accuse de reception\n");

        if (retCode == ERROR_CODE){
            printf("Erreur : mot de passe incorrect\n");
        } else {
            switch (numService){
                case SERVICE_SOMME : client_somme(pipeClientToServ, pipeServToClient, argc, argv); break;
                case SERVICE_COMPRESSION : client_compression(pipeClientToServ, pipeServToClient, argc, argv); break;
                case SERVICE_SIGMA : client_sigma(pipeClientToServ, pipeServToClient, argc, argv); break;
                default : myassert(false, "erreur : numero de service incorrect\n");
            }
            
            printf("Client : en attente de l'Accusé de reception\n");
            ret = write(pipeClientToServ, &retCode, sizeof(int));
            myassert(ret != -1, "echec de l'envoi de l'accusé de reception\n");
            printf("Accusé de reception %d envoyé au service\n", retCode);
            
        }
        ret = close(pipeServToClient);
        myassert(ret != -1, "echec de la fermeture du tube pipeServToClient\n");
        ret = close(pipeClientToServ);
        myassert(ret != -1, "echec de la fermeture du tube pipeClientToServ\n");
    }
    
    // libération éventuelle de ressources
    if (tubeC2S != NULL){   //si l'un des deux est alloué, l'autre aussi logiquement
        free(tubeC2S);
        free(tubeS2C);
    }

    return EXIT_SUCCESS;
}
