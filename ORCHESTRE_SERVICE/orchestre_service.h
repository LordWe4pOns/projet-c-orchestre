#ifndef ORCHESTRE_SERVICE_H
#define ORCHESTRE_SERVICE_H

// Ici toutes les communications entre l'orchestre et les services :
// - le tube anonyme pour que l'orchestre envoie des données au service
// - le sémaphore pour que  le service indique à l'orchestre la fin
//   d'un traitement
int pipeOrchToServ0[2];     //orch-->service somme
int pipeOrchToServ1[2];     //orch-->service compression
int pipeOrchToServ2[2];     //orch-->service sigma

#define ORCH_SERV "../ORCHESTRE_SERVICE/orchestre_service.h"

#define ORCH_SERV_KEY 1337

#endif
