# Projet Orchestre

Ce projet est une application **client-serveur multi-processus en C** basée sur le motif d'architecture **Orchestre / Services / Clients**.

Son but est d'illustrer la communication inter-processus (**IPC** - *Inter-Process Communication*) et la synchronisation sous Unix.

---

### Les 3 Acteurs Principaux

#### 1. L'Orchestre (`ORCHESTRE/orchestre.c`)
C'est le **processus central (le démon)**.
* **Initialisation** : Il lit un fichier de configuration (`CONFIG/config.txt`) pour savoir quels services sont actifs (*ouverts* ou *fermés*).
* **Lancement des services** : Il instancie chaque service dans son propre processus enfant via `fork()` et `execv()`.
* **Rôle de Courtier (Broker)** : Il écoute les requêtes des clients, valide si le service demandé est ouvert et disponible, génère un **mot de passe/ticket de session**, puis met en relation le client et le service.

#### 2. Les Services (`SERVICE/service.c`)
Ce sont les **processus de traitement applicatif** (ex: calcul de somme, compression de texte, calcul statistique *Sigma*).
* Chaque service fonctionne de manière autonome et reste en attente d'ordres envoyés par l'Orchestre.
* Lorsqu'un ordre arrive, le service ouvre une liaison directe avec le client (tubes nommés dédiés), vérifie le mot de passe fourni par le client, réalise le calcul et renvoie le résultat.
* Une fois le travail terminé, il prévient l'Orchestre qu'il est à nouveau disponible.

#### 3. Les Clients (`CLIENT/client.c`)
Ce sont les **programmes lancés par l'utilisateur** en ligne de commande (ex: `CLIENT/client 0 35 76 "==> "`).
* Le client contacte d'abord l'Orchestre pour demander l'accès à un service spécifique.
* S'il est accepté, il reçoit un mot de passe et le nom des tubes dédiés.
* Il communique ensuite **directement avec le Service** sans repasser par l'Orchestre pour lui envoyer ses données et récupérer le résultat.

---

### Scénario d'Exécution Pas à Pas

1. **Démarrage de l'Orchestre** : Crée les tubes nommés globaux (`ORCH_TO_CLIENT`, `CLIENT_TO_ORCH`), initialise les sémaphores de synchronisation, et fait `fork()` + `execv()` pour démarrer les services.
2. **Demande Client** : Un client envoie le numéro du service souhaité à l'Orchestre.
3. **Négociation & Ticket** : 
   * L'Orchestre vérifie la disponibilité du service.
   * Si tout est valide, il génère un **mot de passe unique**.
   * Il transmet ce mot de passe au Service (via tube anonyme) et au Client (via tube nommé).
4. **Communication Directe** : Le client ouvre les tubes nommés du service, envoie son mot de passe pour authentification, puis ses arguments de calcul.
5. **Réponse & Libération** : Le service effectue le traitement, renvoie le résultat au client, puis signale sa disponibilité à l'Orchestre via un sémaphore.

---

### Concepts & Primitives Système Utilisés

* **Tubes nommés (FIFO - `mkfifo`)** : Pour la communication entre des processus indépendants qui n'ont pas de lien de parenté (Client $\leftrightarrow$ Orchestre, Client $\leftrightarrow$ Service).
* **Tubes anonymes (`pipe`)** : Pour la communication entre processus parent et enfant (Orchestre $\rightarrow$ Service).
* **Sémaphores (System V IPC - `semget`, `semop`)** :
  * Éviter que plusieurs clients ne s'adressent simultanément à l'Orchestre (exclusion mutuelle).
  * Suivre et synchroniser l'état de disponibilité de chaque service.
* **Gestion des processus (`fork`, `execv`, `wait`)** : Pour la création et le suivi du cycle de vie des processus enfants.