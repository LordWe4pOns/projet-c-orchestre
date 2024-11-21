#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include "orchestre_service.h"
#include "client_service.h"

#include "service_sigma.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int* size, int* nbThreads, float* values)
{
    int ret = read(fd_pipe_from_client, nbThreads, sizeof(int));
    myassert(ret == sizeof(int), "echec de la reception du nombre de threads\n");

    ret = read(fd_pipe_from_client, size, sizeof(int));
    myassert(ret == sizeof(int), "echec de la reception de la taille du tableau de float\n");

    values = malloc(sizeof(float) * (*size));
    ret = read(fd_pipe_from_client, values, sizeof(float) * (*size));
    myassert(ret == sizeof(float) * (*size));
}

static void * computeAux(int indMin, int nb_val, float* values, float* res){
    float my_res = 0;
    for (int i = indMin; i < nb_val; i++){
        my_res += values[i];
    }
    res = *res + my_res;
}

// fonction de traitement des données
static void computeResult(int size, int nbThreads, float* values, float* res)
{
    pthread_t thArr[nbThreads];
    int ret;
    int nb_val = size / nbThreads;

    for (int i = 0; i < nbThreads - 1; i++){
        ret = pthread_create(&(thArr[i]), NULL, computeAux, (i * nb_val, nb_val, values, res));
        myassert(ret == 0, "echec de la creation d'un thread\n");
    }
    ret = pthread_create(&(thArr[nbThreads - 1]), NULL, computeAux, ((nbThreads - 1), nb_val + (size % nbThreads), values, res));
    myassert(ret == 0, "echec de la creation d'un thread\n");

    for (int i = 0; i < nbThreads; i++){
        ret = pthread_join(thArr[i], NULL);
        myassert(ret == 0, "echec de l'attente d'un thread fils\n");
    }
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, float res)
{
    int ret = write(fd_pipe_to_client, &res, sizeof(float));
    myassert(ret == sizeof(float));
}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_sigma(int fd_pipe_to_client, int fd_pipe_from_client)
{
    // initialisations diverses
    int size, nbThreads;
    float* values;
    float res;

    receiveData(fd_pipe_from_client, &size, &nbThreads, values);
    computeResult(nbThreads, values, &res);
    sendResult(fd_pipe_to_client, res);

    // libération éventuelle de ressources
    free(values);
}
