#ifndef CLIENT_ORCHESTRE_H
#define CLIENT_ORCHESTRE_H

// Ici toutes les communications entre l'orchestre et les clients :
// - le sémaphore pour que 2 clients ne conversent pas en même
//   temps avec l'orchestre
// - les deux tubes nommés pour la communication bidirectionnelle

#define CLIENT_ORCH "CLIENT_ORCHESTRE/client_orchestre.h"
#define CLIENT_ORCH_KEY 5 //5 normalement
#define CLIENT_DONE_KEY 10
#define CLIENT_TO_ORCH "pipe_o2c"
#define ORCH_TO_CLIENT "pipe_c2o"

#define ERROR_CODE -1
#define VALIDATION_CODE 0

#endif
