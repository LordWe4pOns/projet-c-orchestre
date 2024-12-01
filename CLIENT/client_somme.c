#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "client_service.h"
#include "client_somme.h"
#include "../UTILS/myassert.h"
#include "../UTILS/io.h"


/*----------------------------------------------*
 * usage pour le client somme
 *----------------------------------------------*/

static void usage(const char *exeName, const char *numService, const char *message)
{
    fprintf(stderr, "Client somme de deux nombres\n");
    fprintf(stderr, "usage : %s %s <n1> <n2> <prefixe>\n", exeName, numService);
    fprintf(stderr, "        %s         : numéro du service\n", numService);
    fprintf(stderr, "        <n1>      : premier nombre à sommer\n");
    fprintf(stderr, "        <n2>      : deuxième nombre à sommer\n");
    fprintf(stderr, "        <prefixe> : chaîne à afficher avant le résultat\n");
    fprintf(stderr, "exemple d'appel :\n");
    fprintf(stderr, "    %s %s 22 33 \"le résultat est : \"\n", exeName, numService);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

/*----------------------------------------------*
 * fonction de vérification des paramètres
 *----------------------------------------------*/

void client_somme_verifArgs(int argc, char * argv[])
{
    char tabArg2[11];
    int arg2 = atoi(argv[2]);
    sprintf(tabArg2, "%d", arg2);

    char tabArg3[11];
    int arg3 = atoi(argv[3]);
    sprintf(tabArg3, "%d", arg3);

    if (argc != 5)
        usage(argv[0], argv[1], "Nombre d'arguments incorrect.\n");
    if (argv[1][0] != '0')
        usage(argv[0], argv[1],"Ce n'est pas le bon numéro de service.\n" );
    if (strcmp(argv[2], tabArg2))
        usage(argv[0], argv[1],"Le deuxième paramètre doit être un entier.\n" );
    if (strcmp(argv[3], tabArg3))
        usage(argv[0], argv[1],"Le troisième paramètre doit être un entier.\n" );
    if (argv[4][0] == '\0')
    usage(argv[0], argv[1], "La chaîne à afficher est vide.\n");
}


/*----------------------------------------------*
 * fonctions de communication avec le service
 *----------------------------------------------*/

// ---------------------------------------------
// fonction d'envoi des données du client au service
// Les paramètres sont
// - le file descriptor du tube de communication vers le service
// - les deux float dont on veut la somme
static void sendData(int fd_pipe_to_service, int int1, int int2)
{
    my_write(fd_pipe_to_service, &int1, sizeof(int));
    my_write(fd_pipe_to_service, &int2, sizeof(int));
}

// ---------------------------------------------
// fonction de réception des résultats en provenance du service et affichage
// Les paramètres sont
// - le file descriptor du tube de communication en provenance du service
// - le prefixe
// - autre chose si nécessaire
static void receiveResult(int fd_pipe_from_service, const char *prefixe)
{
    int somme_res;
    my_read(fd_pipe_from_service, &somme_res, sizeof(int));
    printf("%s %d\n", prefixe, somme_res);
}


// ---------------------------------------------
// Fonction appelée par le main pour gérer la communications avec le service
// Les paramètres sont
// - les deux file descriptors des tubes nommés avec le service
// - argc et argv fournis en ligne de commande
// Cette fonction analyse argv et en déduit les données à envoyer
//    - argv[2] : premier nombre
//    - argv[3] : deuxième nombre
//    - argv[4] : chaîne à afficher avant le résultat
void client_somme(int fd_pipe_to_service, int fd_pipe_from_service, int argc, char * argv[])
{
    client_somme_verifArgs(argc, argv);
    
    int int1 = atoi(argv[2]);
    int int2 = atoi(argv[3]);
    const char *prefixe = argv[4];
    myassert(prefixe[0] != '\0', "Erreur : il n'y a pas de préfixe.\n");

    sendData(fd_pipe_to_service, int1, int2);
    receiveResult(fd_pipe_from_service, prefixe);
}

