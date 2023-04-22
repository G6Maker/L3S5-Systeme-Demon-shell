#include "sema.h"

// definition de la structure
struct fifo {
  sem_t mutex;   // Controle l'accès
  sem_t vide;    // Compte les emplacements vides
  sem_t plein;   // Compte les emplacemens occupés
  int tete;      // Position d'ajout dans le tampon
  int queue;     // Position de suppression dans le tampon
  int buffer[]; // Le tampon contenant les données
};

// L'en-tête du segment de mémoire partagée
struct fifo *fifo_p = NULL;

//crée l'espace de memoire partage et le semaphore puis le
int semaphore_serv_init(int n) { //prendre en parametre un int n
  if(n < 1){
    n = N;
  }
  int taille_shm = ((int)(sizeof(struct fifo)) + (int)(n*(int)sizeof(int)));
  // Création d'un segment de mémoire partagée
  int shm_fd = shm_open(NOM_SHM, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("shm_open");
    return EXIT_FAILURE;
  }
  if (ftruncate(shm_fd, taille_shm) == -1) {
    perror("ftruncate");
    return EXIT_FAILURE;
  }
  char *shm_ptr = mmap(NULL, (size_t)taille_shm, PROT_READ | PROT_WRITE, MAP_SHARED,
      shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("sem_open");
    return EXIT_FAILURE;
  }
  fifo_p = (struct fifo *) shm_ptr;
  if (sem_init(&fifo_p->mutex, 1, 1) == -1) {
    perror("sem_init");
    return EXIT_FAILURE;
  }
  if (sem_init(&fifo_p->vide, 1, (unsigned int)n) == -1) {
    perror("sem_init");
    return EXIT_FAILURE;
  }
  if (sem_init(&fifo_p->plein, 1, 0) == -1) {
    perror("sem_init");
    return EXIT_FAILURE;
  }
  fifo_p->tete = 0;
  fifo_p->queue = 0;
  return 0;
}

int semaphore_serv_close(void) {
  // Attention: être sûr que les sémaphores sont détruits à la fin du processus
  // (gestion des signaux).
  if (shm_unlink(NOM_SHM) == -1) {
    perror("shm_unlink");
    return EXIT_FAILURE;
  }
  if (sem_destroy(&fifo_p->mutex) == -1) {
    perror("sem_destroy");
    return EXIT_FAILURE;
  }
  if (sem_destroy(&fifo_p->plein) == -1) {
    perror("sem_destroy");
    return EXIT_FAILURE;
  }
  if (sem_destroy(&fifo_p->vide) == -1) {
    perror("sem_destroy");
    return EXIT_FAILURE;
  }
  exit(EXIT_SUCCESS);
}

int semaphore_client_init(void) {
  // Création d'un segment de mémoire partagée
  int shm_fd = shm_open(NOM_SHM, O_RDWR, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(EXIT_SUCCESS);
  }
  if (ftruncate(shm_fd, TAILLE_SHM) == -1) {
    perror("ftruncate");
    exit(EXIT_FAILURE);
  }
  char *shm_ptr = mmap(NULL, TAILLE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED,
      shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }
  fifo_p = (struct fifo *) shm_ptr;
  return EXIT_SUCCESS;
}

void producteur(void) {
  ssize_t n;
  char c;
  while ((n = read(STDIN_FILENO, &c, 1)) > 0) {
    ajouter(c);
  }
  if (n == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }
  ajouter(0);
}

void consommateur(void) {
  pid_t pid;
  do {
    pid = retirer();
    errno = 0;
    if (write(STDOUT_FILENO, &pid, 10) < 1) {
      if (errno != 0) {
        perror("write");
        exit(EXIT_FAILURE);
      } else {
        fprintf(stderr, "write: impossible d'écrire les données demandées");
        exit(EXIT_FAILURE);
      }
    }
  } while (pid != 0);
}

//ajoute un pid a l'espace de memoire partagé
void ajouter(pid_t c) {
  if (sem_wait(&fifo_p->vide) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  if (sem_wait(&fifo_p->mutex) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  fifo_p->buffer[fifo_p->tete] = c;
  fifo_p->tete = (fifo_p->tete + 1) % N;
  if (sem_post(&fifo_p->mutex) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  if (sem_post(&fifo_p->plein) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
}

//retire un pid de l'espace partage sans rendre le jeton à vide
pid_t retirer(void) {
  if (sem_wait(&fifo_p->plein) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  if (sem_wait(&fifo_p->mutex) == -1) {
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
  pid_t c = fifo_p->buffer[fifo_p->queue];
  fifo_p->queue = (fifo_p->queue + 1) % N;
  if (sem_post(&fifo_p->mutex) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
  return c;
}

//redonne le jeton de vide.
int post(void){
  if (sem_wait(&fifo_p->mutex) == -1) {
    perror("sem_wait");
    return EXIT_FAILURE;
  }
  if (sem_post(&fifo_p->vide) == -1) {
    perror("sem_post");
    return EXIT_FAILURE;
  }
  if (sem_post(&fifo_p->mutex) == -1) {
    perror("sem_post");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
