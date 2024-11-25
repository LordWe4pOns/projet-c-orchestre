/*
 * Indications (à respecter) :
 * - Les erreurs sont gérées avec des assert ; les erreurs traitées sont :
 *    . appel trop tôt ou trop tard d'une méthode (cf. config.h)
 *    . fichier de configuration inaccessible
 *    . une position erronée
 * - Le fichier (si on arrive à l'ouvrir) est considéré comme bien
 *   formé sans qu'il soit nécessaire de le vérifier
 *
 * Un code minimal est fourni et permet d'utiliser le module "config" dès
 * le début du projet ; il faudra le remplacer par l'utilisation du fichier
 * de configuration.
 * Il est inutile de faire plus que ce qui est demandé
 *
 * Dans cette partie vous avez le droit d'utiliser les entrées-sorties
 * de haut niveau (fopen, fgets, ...)
 */


// TODO include des .h système*
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "myassert.h"

#include "config.h"

// TODO Définition des données ici
bool init = false;
int nb_serv;
bool* open = NULL;
char* name = NULL;


void config_init(const char *filename)
{
    // TODO erreur si la fonction est appelée deux fois
    myassert(!init, "erreur : config_init est appelée deux fois\n");

    // TODO code vide par défaut, à remplacer
    //      il faut lire le fichier et stocker toutes les informations en
    //      mémoire
    FILE * fd = fopen(filename, "r");
    myassert(fd != NULL, "echec de l'ouverture du config file\n");

    size_t ret = fread(&nb_serv, sizeof(int), 1, fd);
    myassert(ret == sizeof(int), "echec de la recuperetion du nombre de service dans le config file\n");

    name = malloc(sizeof(char) * 50);
    fgets(name, 50, fd);

    int numServ;
    open = malloc(sizeof(bool) * nb_serv);
    for (int i = 0; i < nb_serv; i++){
        ret = fread(&numServ, sizeof(int), 1, fd);
        myassert(ret == sizeof(int), "echec de la recuperation d'un numero de service\n");

        char isOpen[7];
        fgets(isOpen, 7, fd);

        char* tmp = "ouvert";
        open[numServ] = strcmp(isOpen, tmp) == 0;
    }
    ret = fclose(fd);
    myassert(ret == 0, "echec de la fermeture du fd config\n");
    init = true;
}

void config_exit()
{
    // TODO erreur si la fonction est appelée avant config_init
    myassert(init, "erreur : config_exit appelee avant config_init\n");

    // TODO code vide par défaut, à remplacer
    //      libération des ressources
    init = false;
    free(open);
    open = NULL;
    free(name);
    name = NULL;
}

int config_getNbServices()
{
    // erreur si la fonction est appelée avant config_init
    // erreur si la fonction est appelée après config_exit
    myassert(init, "erreur : appel de la fonction config_getNbServices avant config_init ou apres config_exit\n");

    return nb_serv;
}

const char * config_getExeName()
{
    // TODO erreur si la fonction est appelée avant config_init
    // TODO erreur si la fonction est appelée après config_exit
    myassert(init, "erreur : appel de la fonction config_getExeName avant config_init ou apres config_exit\n");

    // TODO code par défaut, à remplacer
    return (const char*)name;
}

bool config_isServiceOpen(int pos)
{
    // TODO erreur si la fonction est appelée avant config_init
    // TODO erreur si la fonction est appelée après config_exit
    // TODO erreur si "pos" est incorrect
    myassert(init, "erreur : appel de la fonction config_isServiceOpen avant config_init ou apres config_exit\n");
    myassert(pos >= 0 && pos < nb_serv, "erreur : pos doit etre compris entre 0 et nb_serv - 1\n");

    // TODO code par défaut, à remplacer
    return open[pos];
}
