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
#include "../UTILS/io.h"


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
    printf("Lancement du service %d OK\n", servNum);

    key_t key = my_ftok(ORCH_SERV, atoi(argv[2]));     //cle sema serv<-->orch
    int semOrchServ = my_semget(key, SERVICE_NB);

    int pipeFromOrch = atoi(argv[3]);     //fd orch-->serv
    char* servToClient = argv[4];
    char* clientToServ = argv[5];
    bool fin = false;

    while (!fin)
    {
        // attente d'un code de l'orchestre (via tube anonyme)
        int code;
        my_read(pipeFromOrch, &code, sizeof(int));

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
            printf("Service %d : Demande d'arret reçue\n", servNum);
            fin = true;
        } else {
            struct sembuf op ={servNum, -1, 0};
            my_semop(semOrchServ, op);
            printf("Service %d : en attente du mot de passe venant de l'orchestre\n", servNum);
            int password;
            my_read(pipeFromOrch, &password, sizeof(int));

            int pipeServToClient = my_open(servToClient, O_WRONLY);     //ouverture tube serv-->client
            int pipeClientToServ = my_open(clientToServ, O_RDONLY);     //ouverture tube client-->serv
            
            int val;
            my_read(pipeClientToServ, &val, sizeof(int));
            if (password != val){
                val = ERROR_CODE;
                my_write(pipeServToClient, &val, sizeof(int));
            } else {
                val = VALIDATION_CODE;
                my_write(pipeServToClient, &val, sizeof(int));
                switch (servNum){
                    case SERVICE_SOMME : service_somme(pipeClientToServ, pipeServToClient); break;
                    case SERVICE_COMPRESSION : service_compression(pipeClientToServ, pipeServToClient); break;
                    case SERVICE_SIGMA : service_sigma(pipeClientToServ, pipeServToClient); break;
                    default : myassert(false, "erreur : numero de service incorrect\n");
                }
                my_read(pipeClientToServ, &val, sizeof(int));
                
            }
            my_close(pipeClientToServ);
            my_close(pipeServToClient);
            printf("Service %d : fin des opérations\n", servNum);
            struct sembuf endOp = {servNum, 1, 0};
            my_semop(semOrchServ, endOp);
        }
    }

    // libération éventuelle de ressources
    printf("Service %d : Arret\n", servNum);

    return EXIT_SUCCESS;
}
