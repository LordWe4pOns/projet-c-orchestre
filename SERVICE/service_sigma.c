#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "orchestre_service.h"
#include "client_service.h"
#include "../UTILS/myassert.h"
#include "../UTILS/io.h"

#include "service_sigma.h"

// définition éventuelle de types pour stocker les données


/*----------------------------------------------*
 * fonctions appelables par le service
 *----------------------------------------------*/

// fonction de réception des données
static void receiveData(int fd_pipe_from_client, int* size, int* nbThreads, float** values)
{
    my_read(fd_pipe_from_client, nbThreads, sizeof(int));
    my_read(fd_pipe_from_client, size, sizeof(int));

    *values = malloc(sizeof(float) * (*size));
    my_read(fd_pipe_from_client, *values, sizeof(float) * (*size));
}

typedef struct {
    int indMin;
    int nb_val;
    float* values;
    float* res;
} ThreadData;

static void * computeAux(void * arg){
    ThreadData* data = (ThreadData *) arg;
    float my_res = 0.0;
    for (int i = data->indMin; i < data->indMin + data->nb_val; i++){
        my_res += data->values[i];
    }
    *(data->res) = *(data->res) + my_res;
    return NULL;
}

// fonction de traitement des données
static void computeResult(int size, int nbThreads, float* values, float* res)
{
    pthread_t thArr[nbThreads];
    int ret;
    int nb_val = size / nbThreads;
    ThreadData datas[nbThreads];

    for (int i = 0; i < nbThreads - 1; i++){
        datas[i].indMin = i * nb_val;
        datas[i].nb_val = nb_val;
        datas[i].values = values;
        datas[i].res = res;
    }

    datas[nbThreads - 1].indMin = (nbThreads - 1) * nb_val;
    datas[nbThreads - 1].nb_val = nb_val + (size % nbThreads);
    datas[nbThreads - 1].values = values;
    datas[nbThreads - 1].res = res;

    for (int i = 0; i < nbThreads - 1; i++){
        ret = pthread_create(&(thArr[i]), NULL, computeAux, &(datas[i]));
        myassert(ret == 0, "Erreur : echec de la creation d'un thread.\n");
    }
    ret = pthread_create(&(thArr[nbThreads - 1]), NULL, computeAux, &(datas[nbThreads - 1]));
    myassert(ret == 0, "Erreur : echec de la creation d'un thread.\n");

    for (int i = 0; i < nbThreads; i++){
        ret = pthread_join(thArr[i], NULL);
        myassert(ret == 0, "Erreur : echec de l'attente d'un thread fils.\n");
    }
}

// fonction d'envoi du résultat
static void sendResult(int fd_pipe_to_client, float res)
{
    my_write(fd_pipe_to_client, &res, sizeof(float));
}


/*----------------------------------------------*
 * fonction appelable par le main
 *----------------------------------------------*/
void service_sigma(int fd_pipe_from_client, int fd_pipe_to_client)
{
    // initialisations diverses
    int size, nbThreads;
    float* values = NULL;
    float res = 0.0;

    receiveData(fd_pipe_from_client, &size, &nbThreads, &values);
    computeResult(size, nbThreads, values, &res);
    sendResult(fd_pipe_to_client, res);

    // libération éventuelle de ressources
    free(values);
}
