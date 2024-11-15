#ifndef CLIENT_SIGMA
#define CLIENT_SIGMA

// on ne déclare ici que les fonctions appelables par le main

// vérifie les arguments : arrêt du programme en cas d'erreur
// - argc doit valoir 4 ou plus
// - argv[1] est le numéro du service
// - argv[2] est le nombre de threads que le service doit utiliser
//   *   (il peut pas y avoir plus de threads que de cases dans le tableau)
//   *   (tableau d'argc -2) 
//   *   (prendre threads un par un et les mettres dans les cases du tableau un par un pour vérifier)
// - argv[3] à argv[argc-1] : les float à envoyer au service
void client_sigma_verifArgs(int argc, char * argv[]);


// fonction pour gérer la communication avec le service
void client_sigma(int fd_pipe_to_service, int fd_pipe_from_service, int argc, char * argv[]);

#endif
