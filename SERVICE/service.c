#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "service.h"
#include "service_somme.h"
#include "service_compression.h"
#include "service_sigma.h"
#include "../UTILS/myassert.h"


static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <num_service> <clé_sémaphore> <fd_tube_anonyme> "
            "<nom_tube_service_vers_client> <nom_tube_client_vers_service>\n",
            exeName);
    fprintf(stderr, "        <num_service>     : entre 0 et %d\n", SERVICE_NB - 1);
    fprintf(stderr, "        <clé_sémaphore>   : entre ce service et l'orchestre (clé au sens ftok)\n");
    fprintf(stderr, "        <fd_tube_anonyme> : entre ce service et l'orchestre\n");
    fprintf(stderr, "        <nom_tube_...>    : noms des deux tubes nommés reliés à ce service\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/*----------------------------------------------*
 * fonction main
 *----------------------------------------------*/
int main(int argc, char * argv[])
{
    if (argc != 6)
        usage(argv[0], "nombre paramètres incorrect");

    // initialisations diverses : analyse de argv
    int servNum = atoi(argv[1]);    //numero du service

    key_t key = ftok(ORCH_SERV, atoi(argv[2]));     //cle sema serv<-->orch
    myassert(key != -1, "echec de la creation de la cle pour le sema serv<-->orch\n");
    int semOrchServ = semget(key, SERVICE_NB, 0);
    myassert(semOrchServ != -1, "echec de la recuperation du sema serv<-->orch\n");

    int pipeFromOrch = atoi(argv[3]);     //fd orch-->serv

    char* servToClient = argv[4];

    char* ClientToServ = argv[5];

    int ret;
    bool fin = false;


    while (!fin)
    {
        // attente d'un code de l'orchestre (via tube anonyme)
        int code;
        ret = read(pipeFromOrch, &code, sizeof(int));
        myassert(ret != -1, "echec de la reception du code par un service\n");

        struct sembuf op ={servNum, -1, 0};
        ret = semop(semOrchServ, &op, SERVICE_NB);
        myassert(ret != -1, "echec du changement de valeur de semOrchServ\n");

        // si code de fin
        //    sortie de la boucle
        // sinon
        //    réception du mot de passe de l'orchestre
        //    ouverture des deux tubes nommés avec le client
        //    attente du mot de passe du client
        //    si mot de passe incorrect
        //        envoi au client d'un code d'erreur
        //    sinon
        //        envoi au client d'un code d'acceptation
        //        appel de la fonction de communication avec le client :
        //            une fct par service selon numService (cf. argv[1]) :
        //                   . service_somme
        //                ou . service_compression
        //                ou . service_sigma
        //        attente de l'accusé de réception du client
        //    finsi
        //    fermeture ici des deux tubes nommés avec le client
        //    modification du sémaphore pour prévenir l'orchestre de la fin
        // finsi
        if (code == SERVICE_ARRET){
            fin = true;
        } else {
            int password;
            ret = read(pipeFromOrch, &password, sizeof(int));
            myassert(ret != -1, "echec de la reception du mot de passe\n");

            int pipeServToClient = open(argv[4], O_WRONLY);     //ouverture tube serv-->client
            int pipeClientToServ = open(argv[5], O_RDONLY);     //ouverture tube client-->serv
            
            int val;
            ret = read(pipeClientToServ, &val, sizeof(int));
            myassert(ret != -1, "echec de la reception du mot de passe client\n");
            if (password != fromClient){
                val = ERROR_CODE;
                ret = write(pipeServToClient, &val, sizeof(int));
                myassert(ret != -1, "echec de l'envoi du code d'erreur au client\n");
            } else {
                val = VALIDATION_CODE;
                ret = write(pipeServToClient, &val, sizeof(int));
                myassert(ret != -1, "echec de l'envoi du code d'acceptation au client\n");
                switch (servNum){
                    case SERVICE_SOMME : service_somme(pipeServToClient, pipeClientToServ); break;
                    case SERVICE_COMPRESSION : service_compression(pipeServToClient, pipeClientToServ); break;
                    case SERVICE_SIGMA : service_sigma(pipeServToClient, pipeClientToServ); break;
                    default : myassert(false, "erreur : numero de service incorrect\n");
                }
                ret = read(pipeClientToServ, &val, sizeof(int));
                myassert(ret != -1, "echec de la reception de l'accusé\n");
            }
            ret = close(pipeClientToServ);
            myassert(ret != -1, "echec de la fermeture du tube pipeClientToServ");
            ret = close(pipeServToClient);
            myassert(ret != -1, "echec de la fermeture du tube pipeServToClient");
            op ={servNum, 1, 0};
            ret = semop(semOrchServ, &op, SERVICE_NB);
            myassert(ret != -1, "echec du changement de valeur de semOrchServ\n");
        }
    }

    // libération éventuelle de ressources

    return EXIT_SUCCESS;
}
