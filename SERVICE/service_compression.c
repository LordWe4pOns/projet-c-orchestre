#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "../UTILS/myassert.h"
#include "../UTILS/io.h"

#include "service_compression.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int *len, char **chaine_recup)
{
    my_read(fd_pipe_from_client, len, sizeof(int));
    *chaine_recup = (char *)malloc(sizeof(char) *(*len + 1)); // +1 pour '\0'
    (*chaine_recup)[*len] = '\0';
    myassert(*chaine_recup != NULL, "Erreur : allocation mémoire échouée.\n");
    my_read(fd_pipe_from_client, *chaine_recup, sizeof(char) *(*len));
}

// fonction de traitement des données
static void computeResult(char *chaine_recup, char **chaine_res)
{
    int len = strlen(chaine_recup);
    int res_len = len * 2 + 1;

    *chaine_res = (char *)malloc(sizeof(char) * res_len); // +1 pour '\0'
    myassert(*chaine_res != NULL, "Erreur : allocation mémoire échouée.\n");
    (*chaine_res)[0] = '\0';

    int count = 1;
    for (int i = 0; i < len; i++){
        char* current_char = malloc(sizeof(char) * 2);
        current_char[0] = chaine_recup[i];
        current_char[1] = '\0';
        if (i + 1 < len && chaine_recup[i + 1] == *current_char){
            count++;
        } else {
            char* chaine_count;
            chaine_count = io_intToStr(count);

            *chaine_res = strcat(*chaine_res, chaine_count);
            *chaine_res = strcat(*chaine_res, current_char);
            count = 1;
            free(chaine_count);
        }
        free(current_char);
    }
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, char *chaine_res)
{
    int len_res = (int)strlen(chaine_res);
    my_write(fd_pipe_to_client, &len_res, sizeof(int));
    my_write(fd_pipe_to_client, chaine_res, sizeof(char) *len_res);
}



/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_compression(int fd_pipe_from_client, int fd_pipe_to_client)
{
    int len;
    char *chaine_recup = NULL;
    char *chaine_res = NULL;
    
    receiveData(fd_pipe_from_client, &len, &chaine_recup);
    computeResult(chaine_recup, &chaine_res);
    sendResult(fd_pipe_to_client, chaine_res);

    free(chaine_recup);
    free(chaine_res);
}
