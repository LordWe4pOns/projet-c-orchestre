#ifndef CLIENT_SERVICE_H
#define CLIENT_SERVICE_H

// Ici toutes les communications entre les services et les clients :
// - les deux tubes nommés pour la communication bidirectionnelle

#define CLIENT_TO_SERV_SUM "pipe_c2s_0"
#define SERV_SUM_TO_CLIENT "pipe_s2c_0"

#define CLIENT_TO_SERV_COMP "pipe_c2s_1"
#define SERV_COMP_TO_CLIENT "pipe_s2c_1"

#define CLIENT_TO_SERV_SIG "pipe_c2s_2"
#define SERV_SIG_TO_CLIENT "pipe_s2c_2"

#define ERROR_CODE -1
#define VALIDATION_CODE 0

//void init_pipeClientServNames(char *** pipesClientServNames);

#endif
