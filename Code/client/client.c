//#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
//#include <semaphore.h>
#include "sema.h"

#define MAX_TN 30

int namegenerator(pid_t pid, char *result, int n);
int tube_delete(void);
void gestionnaire(int signum);

#define TUBE_RPS "/tmp/tube_reponse_"
#define TUBE_RQT "/tmp/tube_requete_"
#define TAILLE_TUBE 4096

int main(void) {
  struct sigaction action;
  action.sa_handler = gestionnaire;
  action.sa_flags = 0;
  if (sigfillset(&action.sa_mask) == -1) {
    perror("sigfillset");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (semaphore_client_init() != EXIT_SUCCESS) {
    printf("lancer le demon");
    exit(EXIT_FAILURE);
  }
  int fd1, fd2;
  char *buf1 = malloc(MAX_TN);
  if (namegenerator(getpid(), buf1, 0) != EXIT_SUCCESS) {
    exit(EXIT_FAILURE);
  }
  if (mkfifo(buf1, S_IRUSR | S_IWUSR) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }
  char *buf2 = malloc(MAX_TN);
  if (namegenerator(getpid(), buf2, 1) != EXIT_SUCCESS) {
    exit(EXIT_FAILURE);
  }
  if (mkfifo(buf2, S_IRUSR | S_IWUSR) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }
  pid_t pid = getpid();
  ajouter(pid);
  fd1 = open(buf1, O_WRONLY);
  if (fd1 == -1) {
    unlink(buf1);
    unlink(buf2);
    perror("open");
    exit(EXIT_FAILURE);
  }
  if (unlink(buf1) == -1) {
    unlink(buf2);
    perror("unlink");
    exit(EXIT_FAILURE);
  }
  //ecrire la commande dans le tube.
  char buf[TAILLE_TUBE];
  char buff[TAILLE_TUBE];
  ssize_t n;
  while ((n = read(STDIN_FILENO, buf, TAILLE_TUBE)) > 0) {
    *(buf + n - 1) = '\0';
    if (write(fd1, buf, (size_t) n ) != n) {
      unlink(buf2);
      perror("write");
      exit(EXIT_FAILURE);
    }
    fd2 = open(buf2, O_RDONLY);
    if (fd2 == -1) {
      unlink(buf2);
      perror("open");
      exit(EXIT_FAILURE);
    }
    //ecrire ce qui est lu dans fd[1] sur la sorti standard
    while ((n = read(fd2, buff, TAILLE_TUBE)) > 0) {
      if (write(STDOUT_FILENO, buff, (size_t) n) != n) {
        unlink(buf2);
        perror("write");
        exit(EXIT_FAILURE);
      }
    }
  }
  if (unlink(buf2) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }
  //fermeture des tubes
  close(fd1);
  close(fd2);
  free(buf1);
  free(buf2);
  return 0;
}

//fonction qui genere les nom des tubes, n = 0 pour le tube de requete
// n != 0 pour le tube de reponse
int namegenerator(pid_t pid, char *result, int n) {
  char mypid[9];
  if (sprintf(mypid, "%d", pid) < 0) {
    return EXIT_FAILURE;
  }
  if (n == 0) {
    strcpy(result, TUBE_RQT);
  } else {
    strcpy(result, TUBE_RPS);
  }
  strcat(result, mypid);
  return EXIT_SUCCESS;
}

//fonction de destruction des tubes.
int tube_delete() {
  pid_t pid = getpid();
  char buffer1[MAX_TN];
  char buffer2[MAX_TN];
  if (namegenerator(pid, buffer1, 0) != 0) {
    exit(EXIT_FAILURE);
  }
  if (namegenerator(pid, buffer2, 1) != 0) {
    exit(EXIT_FAILURE);
  }
  if (unlink(buffer1) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }
  if (unlink(buffer2) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}

//fonction gestionnaire pour les signaux
void gestionnaire(int signum) {
  if (signum < 0) {
    printf("\nprobleme\n");
  }
  char *buf1 = malloc(MAX_TN);
  namegenerator(getpid(), buf1, 1);
  unlink(buf1);
  free(buf1);
  printf("\nMerci\n");
  exit(EXIT_SUCCESS);
}
