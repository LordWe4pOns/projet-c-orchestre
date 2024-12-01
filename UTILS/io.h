#ifndef IO_H
#define IO_H

#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

/*********************************************************************
 * manipulations générales sur les entrées/sorties :
 * - file descriptors
 * - conversions diverses
 * - ...
 *********************************************************************/

/*===================================================================*
 * manipulations de chaînes
 *===================================================================*/

int io_strToInt(const char *s);
// la chaîne renvoyée est allouée dynamiquement
char * io_intToStr(int i);
// la chaine format doit contenir "%d" à l'endroit où doit être inséré i
// la chaîne renvoyée est allouée dynamiquement
char * io_intToStrFormat(const char *format, int i);

float io_strToFloat(const char *s);


void my_mkfifo(const char * filename);  // mkfifo(filename, 0644)
void my_unlink(const char *pathname);

void my_pipe(int pipefd[2]);

key_t my_ftok(const char * pathname, int proj_id);
int my_semCreate(key_t key, int nsems);    // semget(key, nsems, IPC_CREAT | IPC_EXCL | 0641)
int my_semget(key_t key, int nsems);    // semget(key, nsems, 0)
void my_semop(int semid, struct sembuf sops);
int my_semGetVal(int semid, int semnum);    //semctl(semid, semnum, GETVAL)
void my_semSetVal(int semid, int semnum, int val);  // semctl(semid, semnum, SETVAL, val)
void my_semDestroy(int semid);

void my_write(int fd, const void* buf, size_t count);
void my_read(int fd, void *buf, size_t count);

int my_open(const char *pathname, int flags);
void my_close(int fd);


#endif
