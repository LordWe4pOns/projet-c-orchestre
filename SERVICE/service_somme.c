#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "../UTILS/myassert.h"
#include "../UTILS/io.h"

#include "service_somme.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int *int1, int *int2)
{
    my_read(fd_pipe_from_client, int1, sizeof(int));
    my_read(fd_pipe_from_client, int2, sizeof(int));
}

// fonction de traitement des données
static void computeResult(int int1, int int2, int *somme)
{
    *somme = int1 + int2;
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, int somme)
{
    my_write(fd_pipe_to_client, &somme, sizeof(int));
}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_somme(int fd_pipe_from_client, int fd_pipe_to_client)
{
    int int1;
    int int2;
    int somme;
    
    receiveData(fd_pipe_from_client, &int1, &int2);
    computeResult(int1, int2, &somme);
    sendResult(fd_pipe_to_client, somme);
}
