#ifndef SEMA__H
#define SEMA__H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**
 * Un nom de segment de mémoire partagé avec un identifiant pour nous assurer de
 * son unicité.
 */
#define NOM_SHM "/non_shm_21804939"

// Taillec du tampon
#define N 10

typedef struct fifo fifo;
/**
 * On met dans notre shm :
 * - une ent-ête contenant toutes les variables permettant la manipulation du
 *   tampon ;
 * - un tampon de N caractères.
 */
#define TAILLE_SHM (sizeof(struct fifo) + N*sizeof(int))

void ajouter(pid_t c);
pid_t retirer(void);

/**
 * Envoie au consommateur les caractères qu'il lit sur l'entrée standard.
 */
void producteur(void);

/**
 * Affiche ce que le producteur lui envoie sur la sortie standard.
 * S'arrete dès qu'il lit un 0.
 */
void consommateur(void);

int semaphore_serv_init(int n);

int semaphore_serv_close(void);

int semaphore_client_init(void);

int post(void);

#endif
