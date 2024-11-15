#ifndef ORCHESTRE_SERVICE_H
#define ORCHESTRE_SERVICE_H

// Ici toutes les communications entre l'orchestre et les services :
// - le tube anonyme pour que l'orchestre envoie des données au service
// - le sémaphore pour que  le service indique à l'orchestre la fin
//   d'un traitement
int pipeOrchToServ0[2];
int pipeOrchToServ1[2];
int pipeOrchToServ2[2];

#define ORCH_SERV "../ORCHESTRE_SERVICE/orchestre_service.h"

//service 0 : somme
#define ORCH_SERV_SUM_KEY 1

//service 1 : compression
#define ORCH_SERV_COMP_KEY 2

//service 2 : sigma
#define ORCH_SERV_SIG_KEY 3

#endif
