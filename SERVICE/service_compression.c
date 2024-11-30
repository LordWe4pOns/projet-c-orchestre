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
    int taille_chaine = read(fd_pipe_from_client, len, sizeof(int));
    myassert(taille_chaine == sizeof(int), "Erreur : erreur de longueur de la chaîne reçue.\n");
    printf("receiveData : len = %d\n", *len);
    
    *chaine_recup = (char *)malloc(sizeof(char) *(*len + 1)); // +1 pour '\0'
    chaine_recup[*len] = '\0';
    myassert(*chaine_recup != NULL, "Erreur : allocation mémoire échouée.\n");

    int chaine_lu = read(fd_pipe_from_client, *chaine_recup, sizeof(char) *(*len));
    myassert(chaine_lu == *len, "Erreur : la chaîne n'a pas été bien reçue.\n");

    printf("Chaine du client reçue : %s\n", *chaine_recup);
}

// fonction de traitement des données
static void computeResult(char *chaine_recup, char **chaine_res)
{
    printf("entrée dans computeResult OK\n");
    int len = strlen(chaine_recup);
    //int i = 0;
    int res_len = len * 2 + 1;
    printf("entrée dans computeResult OK\n");

    *chaine_res = (char *)malloc(sizeof(char) * res_len); // +1 pour '\0'
    myassert(*chaine_res != NULL, "Erreur : allocation mémoire échouée.\n");
    printf("alloc dans computeResult OK\n");

    int count = 1;
    for (int i = 0; i < len; i++){
        char* current_char = malloc(sizeof(char) * 2);
        strcpy(current_char, &(chaine_recup[i]));
        current_char[1] = '\0';
        if (i + 1 < len && chaine_recup[i + 1] == *current_char){
            count++;
        } else {
            char* chaine_count;
            chaine_count = io_intToStr(count);

            *chaine_res = strcat(*chaine_res, chaine_count);
            printf("Apres strcat 1 : %s\n", *chaine_res);
            *chaine_res = strcat(*chaine_res, current_char);
            printf("Apres strcat 2 : %s\n", *chaine_res);
            count = 1;
            free(chaine_count);
        }
        free(current_char);
    }
/*
    while (i < len) {
        char* current_char = malloc(sizeof(char) * 2);
        strcpy(current_char, &(chaine_recup[i]));
        current_char[1] = '\0';
        printf("current_char = %s\n", current_char);
        int count = 1;
        
        while (i + 1 < len && chaine_recup[i + 1] == *current_char) {
            i++;
            count++;
            printf("tour n°%d\n", i);
        }
        char* chaine_count;
        chaine_count = io_intToStr(count);

        *chaine_res = strcat(*chaine_res, chaine_count);
        printf("Apres strcat 1 : %s\n", *chaine_res);
        *chaine_res = strcat(*chaine_res, current_char);
        printf("Apres strcat 2 : %s\n", *chaine_res);
        i++;
        free(chaine_count);
        free(current_char);
    }
*/
    printf("test chaine_res \n");
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, char *chaine_res)
{
    int len_res = (int)strlen(chaine_res);
    printf("len_res = %d\n", len_res);

    int envoi_len_res = write(fd_pipe_to_client, &len_res, sizeof(int));
    myassert(envoi_len_res == sizeof(int), "Erreur : la longueur de la chaîne résultat n'a pas été bien envoyée.\n");

    int envoi_res = write(fd_pipe_to_client, chaine_res, sizeof(char) *len_res);
    myassert((int)envoi_res == len_res, "Erreur : la chaîne résultat n'a pas été bien renvoyée.\n");
    
    printf("Données renvoyées au client : %s\n", chaine_res);
}



/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_compression(int fd_pipe_from_client, int fd_pipe_to_client)
{
    printf("=====================fd_pipe_to_client : %d\n", fd_pipe_to_client);
    int len;
    char *chaine_recup = NULL;
    char *chaine_res = NULL;
    
    receiveData(fd_pipe_from_client, &len, &chaine_recup);
    printf("receiveData OK\nchaine_recup = **%s**\n", chaine_recup);
    computeResult(chaine_recup, &chaine_res);
    printf("computeResult OK\n");
    sendResult(fd_pipe_to_client, chaine_res);
    printf("sendResult OK\n");

    free(chaine_recup);
    free(chaine_res);
    printf("Service : free comp OK\n");
}
