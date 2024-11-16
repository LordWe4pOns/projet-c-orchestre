#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "client_service.h"
#include "client_sigma.h"
#include "../UTILS/myassert.h"


/*----------------------------------------------*
 * usage pour le client sigma
 *----------------------------------------------*/

static void usage(const char *exeName, const char *numService, const char *message)
{
    fprintf(stderr, "Client sigma de float\n");
    fprintf(stderr, "usage : %s %s <nbThreads> <f1> <f2> .. <fn>\n", exeName, numService);
    fprintf(stderr, "        %s           : numéro du service\n", numService);
    fprintf(stderr, "        <nbThreads>   : nombre de threads\n");
    fprintf(stderr, "        <f1> ... <fn> : les nombres à tester (au moins un)\n");
    fprintf(stderr, "exemple d'appel :\n");
    fprintf(stderr, "    %s %s 2 6.3 5.8 -2.33 0.0 8.34 12.98\n", exeName, numService);
    fprintf(stderr, "    -> 6 nombres à tester avec 2 threads\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

/*----------------------------------------------*
 * fonction de vérification des paramètres
 *----------------------------------------------*/

void client_sigma_verifArgs(int argc, char * argv[])
{
    int nb_threads = atoi(argv[2]);

    if (argc < 4)
        usage(argv[0], argv[1], "nombre d'arguments incorrect.\n");
    if (argv[1][0] != 2)
        usage(argv[0], argv[1],"Ce n'est pas le bon numéro de service.\n" );
    if (nb_threads > argc - 3)
        usage(argv[0], argv[1], "Il ne peut pas y avoir plus de threads que de cases dans le tableau.\n");

    for (int i = 3; i < argc; i++) {
        char *fin_arg;
        float valeur = strtof(argv[i], &fin_arg);
        if (*fin_arg != '\0') {
            usage(argv[0], argv[1], "Ce n'est pas un tableau exclusivement de flottants.\n");
        }
        if (valeur == 0.0f && fin_arg == argv[i]) { // si valeur = 0.0f c'est que strtof n'a pas pu effectuer une conversion valide
        usage(argv[0], argv[1], "Ce n'est pas un tableau exclusivement de flottants.\n");
        }
    }
} 

/*----------------------------------------------*
 * fonctions de communication avec le service
 *----------------------------------------------*/

// ---------------------------------------------
// fonction d'envoi des données du client au service
// Les paramètres sont
// - le file descriptor du tube de communication vers le service
// - le nombre de threads que doit utiliser le service
// - le tableau de float dont on veut la somme
static void sendData(int fd_pipe_to_service, int nb_threads, float *tab_float, int size)
{
    int envoi_nb_threads = write(fd_pipe_to_service, &nb_threads, sizeof(nb_threads));
    myassert(envoi_nb_threads != sizeof(nb_threads), "Erreur : tous les octets n'ont pas été envoyés.\n");

    int envoi_tab = write(fd_pipe_to_service, tab_float, sizeof(float) * size);
    myassert(envoi_tab != (int)sizeof(float) * size, "Erreur : le tableau ne s'est pas bien envoyé.\n");

    printf("Données envoyées au service : %d threads, %d valeurs.\n", nb_threads, size);
}

// ---------------------------------------------
// fonction de réception des résultats en provenance du service et affichage
// Les paramètres sont
// - le file descriptor du tube de communication en provenance du service
// - autre chose si nécessaire
static void receiveResult(int fd_pipe_from_service)
{
    float res;

    int res_lu = read(fd_pipe_from_service, &res, sizeof(res));
    myassert(res_lu != sizeof(res), "Erreur : problème dans le résultat reçu.\n");

    printf("Résultat reçu du service : %f\n", res);
}


// ---------------------------------------------
// Fonction appelée par le main pour gérer la communications avec le service
// Les paramètres sont
// - les deux file descriptors des tubes nommés avec le service
// - argc et argv fournis en ligne de commande
// Cette fonction analyse argv et en déduit les données à envoyer
//    - argv[2] : nombre de threads
//    - argv[3] à argv[argc-1]: les nombres flottants
void client_sigma(int fd_pipe_to_service, int fd_pipe_from_service, int argc, char * argv[])
{
    client_sigma_verifArgs(argc, argv);

    int nb_threads = atoi(argv[2]);
    int size = argc - 2;
    float tab_float[size];

    for (int i = 0; i < size; i++) {
        tab_float[i] = atof(argv[i + 3]);
    }

    sendData(fd_pipe_to_service, nb_threads, tab_float, size);
    receiveResult(fd_pipe_from_service);
}

