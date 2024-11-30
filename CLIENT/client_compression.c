#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "../CLIENT_SERVICE/client_service.h"
#include "client_compression.h"
#include "../UTILS/myassert.h"


/*----------------------------------------------*
 * usage pour le client compression
 *----------------------------------------------*/

static void usage(const char *exeName, const char *numService, const char *message)
{
    fprintf(stderr, "Client compression de chaîne\n");
    fprintf(stderr, "usage : %s %s <chaîne>\n", exeName, numService);
    fprintf(stderr, "        %s      : numéro du service\n", numService);
    fprintf(stderr, "        <chaine> : chaîne à compresser\n");
    fprintf(stderr, "exemple d'appel :\n");
    fprintf(stderr, "    %s %s \"aaabbcdddd\"\n", exeName, numService);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/*----------------------------------------------*
 * fonction de vérification des paramètres
 *----------------------------------------------*/

void client_compression_verifArgs(int argc, char * argv[])
{
    if (argc != 3)
        usage(argv[0], argv[1], "nombre d'arguments incorrect.\n");
    if (argv[1][0] != '1')
        usage(argv[0], argv[1],"Ce n'est pas le bon numéro de service.\n" );
    if (argv[2][0] == '\0')
        usage(argv[0], argv[1], "la chaîne à compresser est vide.\n");
}


/*----------------------------------------------*
 * fonctions de communication avec le service
 *----------------------------------------------*/

// ---------------------------------------------
// fonction d'envoi des données du client au service
// Les paramètres sont
// - le file descriptor du tube de communication vers le service
// - la chaîne devant être compressée
static void sendData(int fd_pipe_to_service, const char *chaine_a_envoyer)
{
    int len = (int)strlen(chaine_a_envoyer);

    int envoi_len = write(fd_pipe_to_service, &len, sizeof(int));
    myassert(envoi_len == sizeof(len), "Erreur : la longueur de la chaîne n'a pas été envoyée.\n");

    int envoi = write(fd_pipe_to_service, chaine_a_envoyer, sizeof(char) *len);
    myassert(envoi == len, "Erreur : tous les octets n'ont pas été envoyés.\n");
    
    printf("Données envoyées au service : %s\n", chaine_a_envoyer);
}


// ---------------------------------------------
// fonction de réception des résultats en provenance du service et affichage
// Les paramètres sont
// - le file descriptor du tube de communication en provenance du service
// - autre chose si nécessaire
static void receiveResult(int fd_pipe_from_service)
{
    int len;

    int taille_chaine_res = read(fd_pipe_from_service, &len, sizeof(int));
    printf("client comp : len = %d, taille_chaine_res = %d\n", len, taille_chaine_res);
    myassert(taille_chaine_res == sizeof(int), "Erreur : la chaîne résultat n'a pas été bien reçue.\n");
    
    char *chaine_res = (char *)malloc(sizeof(char) *(len + 1)); // +1 pour '\0'
    chaine_res[len] = '\0';
    myassert(chaine_res != NULL, "Erreur : allocation mémoire échouée.\n");

    int chaine_res_lue = read(fd_pipe_from_service, chaine_res, sizeof(char) * len);
    myassert(chaine_res_lue == len, "Erreur : la chaine résultat n'a pas été bien reçue.\n");

    printf("Chaine du service reçue : %s\n", chaine_res);
    free(chaine_res);
}

// ---------------------------------------------
// Fonction appelée par le main pour gérer la communications avec le service
// Les paramètres sont
// - les deux file descriptors des tubes nommés avec le service
// - argc et argv fournis en ligne de commande
// Cette fonction analyse argv et en déduit les données à envoyer
//    - argv[2] : la chaîne à compresser
void client_compression(int fd_pipe_to_service, int fd_pipe_from_service, int argc, char * argv[])
{
    client_compression_verifArgs(argc, argv);
    printf("=====================fd_pipe_from_service : %d\n", fd_pipe_from_service);

    const char *chaine = argv[2];
    myassert(chaine[0] != '\0', "Erreur : la chaine à compresser est vide.\n");

    sendData(fd_pipe_to_service, chaine);
    printf("Client comp : sendData OK\n");
    receiveResult(fd_pipe_from_service);
}