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
    key_t key = my_ftok(CLIENT_ORCH, CLIENT_ORCH_KEY);     //cle pour sema client<-->orch
    int semClientOrch = my_semget(key, 1);
    key = my_ftok(CLIENT_ORCH, CLIENT_DONE_KEY);     //cle pour sema pour attendre que le client ferme les tubes orch<-->client
    int ClientDone = my_semget(key, 1);
    

    // entrée en section critique pour communiquer avec l'orchestre
    printf("Acces a la section critique...\n");
    struct sembuf accessOrch = {0, -1, 0};
    my_semop(semClientOrch, accessOrch);
    my_semop(ClientDone, accessOrch);
    printf("Acces a la section critique réussi\n");

    // ouverture des tubes avec l'orchestre
    printf("Connexion a l'orchestre...\n");
    int pipeOrchToClient = my_open(ORCH_TO_CLIENT, O_RDONLY);
    int pipeClientToOrch = my_open(CLIENT_TO_ORCH, O_WRONLY);
    printf("Connexion a l'orchestre réussie\n");

    // envoi à l'orchestre du numéro du service
    my_write(pipeClientToOrch, &numService, sizeof(int));

    // attente code de retour
    int retCode;
    my_read(pipeOrchToClient, &retCode, sizeof(int));

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
            my_read(pipeOrchToClient, &password, sizeof(int));

            int len;

            my_read(pipeOrchToClient, &len, sizeof(int));
            tubeC2S = malloc(sizeof(char) * (len + 1));
            my_read(pipeOrchToClient, tubeC2S, sizeof(char) * len);
            tubeC2S[len] = '\0';

            my_read(pipeOrchToClient, &len, sizeof(int));
            tubeS2C = malloc(sizeof(char) * (len + 1));
            my_read(pipeOrchToClient, tubeS2C, sizeof(char) * len);
            tubeS2C[len] = '\0';
        }
    }

    int code = VALIDATION_CODE;
    my_write(pipeClientToOrch, &code, sizeof(int));

    my_close(pipeClientToOrch);
    my_close(pipeOrchToClient);

    struct sembuf op = {0, 1, 0};
    my_semop(ClientDone, op);
    my_semop(semClientOrch, op);

    if (retCode != ERROR_CODE && numService != -1){
        int pipeServToClient = my_open(tubeS2C, O_RDONLY);     //ouverture tube serv-->client
        int pipeClientToServ = my_open(tubeC2S, O_WRONLY);     //ouverture tube client-->serv
 
        my_write(pipeClientToServ, &password, sizeof(int));

        my_read(pipeServToClient, &retCode, sizeof(int));

        if (retCode == ERROR_CODE){
            printf("Erreur : mot de passe incorrect\n");
        } else {
            switch (numService){
                case SERVICE_SOMME : client_somme(pipeClientToServ, pipeServToClient, argc, argv); break;
                case SERVICE_COMPRESSION : client_compression(pipeClientToServ, pipeServToClient, argc, argv); break;
                case SERVICE_SIGMA : client_sigma(pipeClientToServ, pipeServToClient, argc, argv); break;
                default : myassert(false, "erreur : numero de service incorrect\n");
            }
            my_write(pipeClientToServ, &retCode, sizeof(int));
            
        }
        my_close(pipeServToClient);
        my_close(pipeClientToServ);
    }
    
    // libération éventuelle de ressources
    if (tubeC2S != NULL){   //si l'un des deux est alloué, l'autre aussi logiquement
        free(tubeC2S);
        free(tubeS2C);
    }

    return EXIT_SUCCESS;
}
