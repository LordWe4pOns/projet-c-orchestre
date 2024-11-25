#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "../UTILS/myassert.h"

#include "service_compression.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int *len, char *chaine_recup)
{
    int taille_chaine = read(fd_pipe_from_client, &len, sizeof(int));
    myassert(taille_chaine == sizeof(len), "Erreur : erreur de longueur de la chaîne reçue.\n");
    
    chaine_recup = (char *)malloc(sizeof(char) *(*len + 1)); // +1 pour '\0'
    chaine_recup[*len] = '\0';
    myassert(chaine_recup != NULL, "Erreur : allocation mémoire échouée.\n");

    int chaine_lu = read(fd_pipe_from_client, chaine_recup, sizeof(char) *(*len));
    myassert(chaine_lu == *len, "Erreur : la chaîne n'a pas été bien reçue.\n");

    printf("Chaine du client reçue : %s\n", chaine_recup);
}

// fonction de traitement des données
static void computeResult(char *chaine_recup, char *chaine_res)
{
    int len = strlen(chaine_recup);
    int i = 0;

    chaine_res = (char *)malloc(sizeof(char) *((len * 2) + 1)); // +1 pour '\0'
    myassert(chaine_res != NULL, "Erreur : allocation mémoire échouée.\n");

    while (i < len) {
        char current_char = chaine_recup[i];
        int count = 1;
        
        while (i + 1 < len && chaine_recup[i + 1] == current_char) {
            i++;
            count++;
        }
        char chaine_count[15];
        sprintf(chaine_count, "%d", count);

        strcat(chaine_res, chaine_count);
        strcat(chaine_res, &current_char);
        i++;
    }
    chaine_recup[i] = '\0';

    int len2 = strlen(chaine_res);
    chaine_res = (char *)realloc(chaine_res, len2);
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, char *chaine_res)
{
    int len_res = (int)strlen(chaine_res);

    int envoi_len_res = write(fd_pipe_to_client, &len_res, sizeof(int));
    myassert(envoi_len_res == sizeof(len_res), "Erreur : la longueur de la chaîne résultat n'a pas été bien envoyée.\n");

    int envoi_res = write(fd_pipe_to_client, chaine_res, sizeof(char) *len_res);
    myassert((int)envoi_res == len_res, "Erreur : la chaîne résultat n'a pas été bien renvoyée.\n");
    
    printf("Données renvoyées au client : %s\n", chaine_res);
}



/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_compression(int fd_pipe_from_client, int fd_pipe_to_client)
{
    int len;
    char *chaine_recup = NULL;
    char *chaine_res = NULL;
    
    receiveData(fd_pipe_from_client, &len, chaine_recup);
    computeResult(chaine_recup, chaine_res);
    sendResult(fd_pipe_to_client, chaine_res);

    free(chaine_recup);
    free(chaine_res);
}
