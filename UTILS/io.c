#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#include "myassert.h"

#include "io.h"


/*===================================================================*
 * manipulations de chaînes
 *===================================================================*/

int io_strToInt(const char *s)
{
    myassert(s != NULL, "Erreur chaîne NULL");
    myassert(strlen(s) > 0, "Erreur chaîne vide");

    long int tmp;
    char *end;

    errno = 0;     // voir man strtol
    tmp = strtol(s, &end, 10);
    myassert(errno == 0, "Erreur récupération int");
    myassert(*end == '\0', "Erreur chaîne partiellement lue");
    myassert(tmp >= INT_MIN && tmp <= INT_MAX, "Erreur overflow int");
    
    return (int) tmp;
}

char * io_intToStr(int i)
{
    return io_intToStrFormat("%d", i);
}

char * io_intToStrFormat(const char *format, int i)
{
    myassert(format != NULL, "chaîne inexistante");
    myassert(strstr(format, "%d") != NULL, " la chaîne \"%d\" n'est pas présente");

    char *s = NULL;
    int l = snprintf(NULL, 0, format, i);
    s = malloc((l+1) * sizeof(char));
    myassert(s != NULL, "Erreur allocation");
    sprintf(s, format, i);
    return s;
}

float io_strToFloat(const char *s)
{
    myassert(s != NULL, "Erreur chaîne NULL");
    myassert(strlen(s) > 0, "Erreur chaîne vide");

    float val;
    char *end;

    val = strtof(s, &end);
    //myassert(end != s, "aucun caractère n'a été consommé");
    myassert(! (val ==  HUGE_VALF && errno == ERANGE), "overflow positif");
    myassert(! (val == -HUGE_VALF && errno == ERANGE), "overflow négatif");
    myassert(! (val == 0.f && errno == ERANGE), "underflow");
    myassert(*end == '\0', "Erreur chaîne partiellement lue");
    
    return val;
}

/*===================================================================*
 * mes fonctions avec myassert
 *===================================================================*/

void my_mkfifo(const char * filename){
    int ret = mkfifo(filename, 0644);
    myassert(ret == 0, "echec de la creation du tube");
}

void my_unlink(const char *pathname){
    int ret = unlink(pathname);
    myassert(ret == 0, "echec de la destruction du tube");
}


void my_pipe(int pipefd[2]){
    int ret = pipe(pipefd);
    myassert(ret == 0, "echec de la creation du tube anonyme");
}


key_t my_ftok(const char * pathname, int proj_id){
    key_t key = ftok(pathname, proj_id);
    myassert(key != -1, "echec de la creation de la cle");
    return key;
}

int my_semCreate(key_t key, int nsems){
    int sema = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0641);
    myassert(sema != -1, "echec de la creation du semaphore");
    return sema;
}

int my_semget(key_t key, int nsems){
    int sema = semget(key, nsems, 0);
    myassert(sema != -1, "echec de la recuperation du semaphore");
    return sema;
}

void my_semop(int semid, struct sembuf sops){
    int ret = semop(semid, &sops, 1);
    myassert(ret != -1, "echec de l'operation sur le semaphore");
}

int my_semGetVal(int semid, int semnum){
    int val = semctl(semid, semnum, GETVAL);
    myassert(val != -1, "echec de la recuperation de la valeur du semaphore");
    return val;
}

void my_semSetVal(int semid, int semnum, int val){
    int ret = semctl(semid, semnum, SETVAL, val);
    myassert(ret != -1, "echec de l'initialisation du semaphore");
}

void my_semDestroy(int semid){
    int ret = semctl(semid, -1, IPC_RMID);
    myassert(ret != -1, "echec de la destruction du semaphore");
}


void my_write(int fd, const void* buf, size_t count){
    int ret = write(fd, buf, count);
    myassert(ret != -1, "echec de l'ecriture");
    myassert(ret == (int)count, "erreur durant l'ecriture : nombre d'octets invalide");
}

void my_read(int fd, void *buf, size_t count){
    int ret = read(fd, buf, count);
    myassert(ret != -1, "echec de la lecture");
    myassert(ret == (int)count, "erreur durant la lecture : nombre d'octets invalide");
}


int my_open(const char *pathname, int flags){
    int ret = open(pathname, flags);
    myassert(ret != -1, "echec de l'ouverture du fichier");
    return ret;
}

void my_close(int fd){
    int ret = close(fd);
    myassert(ret == 0, "echec de la fermeture du fichier\n");
}



// uncomment to test
//#define IO_TESTING
#ifdef IO_TESTING

int main()
{
    int i;
    const char *s;

    s = "14";
    i = io_strToInt(s);
    printf("%d (%s)\n", i, s);
    s = "-14";
    i = io_strToInt(s);
    printf("%d (%s)\n", i, s);
    s = "2147483647";
    i = io_strToInt(s);
    printf("%d (%s)\n", i, s);
    s = "-2147483648";
    i = io_strToInt(s);
    printf("%d (%s)\n", i, s);
    
    //s = NULL;
    //i = io_strToInt(s);
    //printf("%d (%s)\n", i, s);
    
    //s = "";
    //i = io_strToInt(s);
    //printf("%d (%s)\n", i, s);

    //s = "14a";
    //i = io_strToInt(s);
    //printf("%d (%s)\n", i, s);

    //s = "2147483648";
    //i = io_strToInt(s);
    //printf("%d (%s)\n", i, s);

    //s = "9223372036854775808";
    //i = io_strToInt(s);
    //printf("%d (%s)\n", i, s);
    
    
    char *r;
    //r = io_intToStrFormat(NULL, 33);
    //r = io_intToStrFormat("====", 33);
    r = io_intToStrFormat("==%d==", 33);
    printf("chaîne : >>%s<<\n", r);
    free(r);
    r = io_intToStr(33);
    printf("chaîne : >>%s<<\n", r);
    free(r);
    
    printf("\n");
    float f;
    s = "3.14";
    f = io_strToFloat(s);
    printf("%g (%s)\n", f, s);
    s = "10.2e4";
    f = io_strToFloat(s);
    printf("%g (%s)\n", f, s);
    //s = NULL;         // chaine NULL
    //s = "";           // chaîne vide
    //s = "3.14aa";     // chaîne avec des caractère en trop
    //s = "1e+1000";    // overflow positif
    //s = "-1e+1000";   // overflow négatif
    //s = "1e-1000";    // underflow
    //f = io_strToFloat(s);
    //printf("%g (%s)\n", f, s);

    return EXIT_SUCCESS;
}

#endif
