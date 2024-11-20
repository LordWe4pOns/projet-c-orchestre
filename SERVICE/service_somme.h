#ifndef SERVICE_SOMME_H
#define SERVICE_SOMME_H

// on ne déclare ici que les fonctions appelables par le main

// fonction pour gérer la communication avec le client
void service_somme(int fd_pipe_form_client, int fd_pipe_to_client);

#endif

