#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "../UTILS/myassert.h"

#include "service_somme.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int *int1, int *int2)
{
    int int1_lu = read(fd_pipe_from_client, &int1, sizeof(int));
    myassert(int1_lu == sizeof(int), "Erreur : problème dans le premier entier reçu.\n");

    int int2_lu = read(fd_pipe_from_client, &int2, sizeof(int));
    myassert(int2_lu == sizeof(int), "Erreur : problème dans le second entier reçu.\n");

    printf("Données reçues par le client : %d, %d\n", *int1, *int2);
}

// fonction de traitement des données
static void computeResult(int int1, int int2, int *somme)
{
    *somme = int1 + int2;
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, int somme)
{
    int envoi_somme = write(fd_pipe_to_client, &somme, sizeof(int));
    myassert(envoi_somme == sizeof(int), "Erreur : la somme n'a pas été bien envoyée.\n");

    printf("Données renvoyées au client : %d\n", somme);
}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_somme(int fd_pipe_form_client, int fd_pipe_to_client)
{
    int int1;
    int int2;
    int somme;
    
    receiveData(fd_pipe_form_client, &int1, &int2);
    computeResult(int1, int2, &somme);
    sendResult(fd_pipe_to_client, somme);
}
