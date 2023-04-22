#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <semaphore.h>
#include <ctype.h>
#include "sema.h"
#include <signal.h>
#include <pthread.h>
#include <pwd.h>

#define FILENAME_CONFIG "config.txt"

void gestionnaire(int signum);

#define MAX_TN 30
#define SEM_PARALLELE 10
#define STR_PARA "--parallele="


#define TUBE_RPS "/tmp/tube_reponse_"
#define TUBE_RQT "/tmp/tube_requete_"
#define TAILLE_TUBE 4096

int namegenerator(pid_t pid, char *result, int n);
int main_demon(void);
void info_proc(pid_t pid);
void info_user_name(char *name);
void info_user_uid(uid_t uid);

int main() {
  //gestion des signaux
  struct sigaction action2;
  action2.sa_handler = SIG_DFL;
  action2.sa_flags = SA_NOCLDWAIT;
  if (sigfillset(&action2.sa_mask) == -1) {
    perror("sigfillset");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGCHLD, &action2, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  struct sigaction action;
  action.sa_handler = gestionnaire;
  action.sa_flags = SA_SIGINFO;
  if (sigfillset(&action.sa_mask) == -1) {
    perror("sigfillset");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction1");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGTERM, &action, NULL) == -1) {
    perror("sigaction3");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGQUIT, &action, NULL) == -1) {
    perror("sigaction4");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGTSTP, &action, NULL) == -1) {
    perror("sigaction5");
    exit(EXIT_FAILURE);
  }
  //lancement du demon
  return main_demon();
}

//partie thread du graph
void *demon_hand(void *client) {
  pid_t pid = *(int *)&client; //on recupere le numero du PID
  char *buf1 = malloc(MAX_TN);
  char *buf2 = malloc(MAX_TN);
  if (buf1 == NULL || buf2 == NULL) {
    semaphore_serv_close();
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  if (namegenerator(pid, buf1, 0) != 0) {
    semaphore_serv_close();
    free(buf1);
    free(buf2);
    exit(EXIT_FAILURE);
  }
  if (namegenerator(pid, buf2, 1) != 0) {
    semaphore_serv_close();
    free(buf1);
    free(buf2);
    exit(EXIT_FAILURE);
  }
  int fd1 = open(buf1, O_RDONLY);
  if (fd1 == -1) { //FD SPECIAL A LA LECTURE
    semaphore_serv_close();
    free(buf1);
    free(buf2);
    perror("open");
    exit(EXIT_FAILURE);
  }
  char rbuf[BUFSIZ];
  ssize_t tmp;
  while ((tmp = read(fd1, rbuf, BUFSIZ)) > 0) {
    char *rbuf2 = malloc((size_t)tmp + 1);
    if(rbuf2 == NULL){
      semaphore_serv_close();
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    strncpy(rbuf2, rbuf, (size_t)tmp);
    int fd2 = open(buf2, O_WRONLY);
    if (fd2 == -1) { //BUF SPECIAL A L'ECRITURE
      free(rbuf2);
      semaphore_serv_close();
      perror("open");
      exit(EXIT_FAILURE);
    }
    //creation du processus pour lancer la CMD
    switch (fork()) {
      case -1:
        free(rbuf2);
        perror("fork");
        semaphore_serv_close();
        exit(EXIT_FAILURE);
      case 0:
        if (close(fd1) == -1) {
          free(rbuf2);
          semaphore_serv_close();
          perror("close");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd2, STDOUT_FILENO) == -1) {
          free(rbuf2);
          semaphore_serv_close();
          perror("dup2");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd2, STDERR_FILENO) == -1) {
          free(rbuf2);
          semaphore_serv_close();
          perror("dup2");
          exit(EXIT_FAILURE);
        }
        //partie non SHELL, fonction defini dans le code
        if (strncmp(rbuf2, "info_proc", 9) == 0) {
          long pid_2 = strtol((rbuf + 9), NULL, 10);
          info_proc((pid_t) pid_2);
          free(rbuf2);
          exit(EXIT_SUCCESS);
        }
        if (strncmp(rbuf2, "info_user", 9) == 0) {
          long uid = strtol((rbuf2 + 9), NULL, 10);
          if (uid == 0L) {
            info_user_name(rbuf2 + 10);
            free(rbuf2);
            exit(EXIT_SUCCESS);
          } else {
            info_user_uid((uid_t) uid);
            free(rbuf2);
            exit(EXIT_SUCCESS);
          }
          free(rbuf2);
          exit(EXIT_FAILURE);
        }
        //partie SHELL
        execlp("/bin/sh", "/bin/sh", "-c", rbuf2, (char *) NULL);
        free(rbuf2);
        perror("exec");
        semaphore_serv_close();
        exit(EXIT_FAILURE);
      default:
        break;
    }
    free(rbuf2);
    if (close(fd2) == -1) {
      free(buf1);
      free(buf2);
      semaphore_serv_close();
      perror("close");
      exit(EXIT_FAILURE);
    }
  }
  if (tmp != 0) { //lecture de read vide
    free(buf1);
    free(buf2);
    semaphore_serv_close();
    close(fd1);
    perror("read fd1");
    exit(EXIT_FAILURE);
  }
  //liberation des ressources utilisé
  if (close(fd1) == -1) {
    free(buf1);
    free(buf2);
    semaphore_serv_close();
    perror("close");
    exit(EXIT_FAILURE);
  }
  if (post() == EXIT_FAILURE) {
    free(buf1);
    free(buf2);
    semaphore_serv_close();
    exit(EXIT_FAILURE);
  }
  free(buf1);
  free(buf2);
  return 0;
}

//fonction demon
int main_demon() {
  //definition paramettre de base
  int parallele = SEM_PARALLELE;
  //recherche des paramettre fihier de config.
  int fconfig = open(FILENAME_CONFIG, O_RDONLY);
  if(fconfig == -1){
    printf("Configuration par défault\n");
  } else {
    char config[50];
    ssize_t sc;
    if((sc = read(fconfig, config, 50)) > 0){
      char *cp = strstr(config, STR_PARA);
      if(cp != NULL){
        int tmp = (int) strtol((cp + strlen(STR_PARA)), NULL, 10);
        if(tmp != 0L) {
          parallele = tmp;
          printf("Parallele = %d\n", parallele);
        } else {
          printf("Configuration par défault\n");
        }
      }
    } else {
      printf("Configuration par défault\n");
    }
  }
  //initialisation avec paramettre
  if (semaphore_serv_init(parallele) != EXIT_SUCCESS) {
    semaphore_serv_close();
    printf("Redemarrer l'application, merci");
    exit(EXIT_FAILURE);
  }
  char *pid;
  pthread_t ph;
  //definition des attributs des Threads
  int errnum;
  pthread_attr_t attr;
  if ((errnum = pthread_attr_init(&attr)) != 0) {
    fprintf(stderr, "pthread_attr_init: %s\n", strerror(errnum));
    exit(EXIT_FAILURE);
  }
  if ((errnum = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    fprintf(stderr, "pthread_attr_setdetachstate: %s\n", strerror(errnum));
    exit(EXIT_FAILURE);
  }
  //boucle de creation des threads (fait au moins 1 fois)
  do {
    pid_t pidtmp = retirer();
    pid = ((char *)NULL + (int)pidtmp);
    if (pid == NULL) {
      semaphore_serv_close();
      exit(EXIT_FAILURE);
    }
    if ((errnum = pthread_create(&ph, &attr, demon_hand, pid)) != 0) {
      fprintf(stderr, "pthread_create: %s\n", strerror(errnum));
      return -1;
    }
  } while (pid != 0);
  //fermeture du semaphore
  if (semaphore_serv_close() == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }
  return 0;
}

//fonction qui genere les nom des tubes, n = 0 pour le tube de requete
// n != 0 pour le tube de reponse
int namegenerator(pid_t pid, char *result, int n) {
  char mypid[sizeof(pid_t) + 1];
  if (sprintf(mypid, "%d", pid) < 0) {
    return EXIT_FAILURE;
  }
  if (n == 0) {
    strcpy(result, TUBE_RQT);
  } else {
    strcpy(result, TUBE_RPS);
  }
  strcat(result, mypid);
  printf("%s\n", result);
  return 0;
}

//fonction gestionnaire pour les signaux
void gestionnaire(int signum) {
  if (signum < 0) {
    printf("\nprobleme\n");
  }
  printf("\nMerci\n");
  if (semaphore_serv_close() == EXIT_FAILURE) {
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

//fonction info_proc
void info_proc(pid_t pid) {
  char chemin[1000];
  snprintf(chemin, 999, "/proc/%d/cmdline", (int) pid);
  FILE *f = fopen(chemin, "r");
  if (f == NULL) {
    printf("Cannot open file %s\n", chemin);
    return;
  }
  char buffer[1000];
  char buf[500];
  while (fgets(buf, 499, f) != NULL) {
    snprintf(buffer, 999, "[%d] %s\n", (int) pid, buf);
    printf("%s", buffer);
  }
  fclose(f);
  snprintf(chemin, 999, "/proc/%d/status", (int) pid);
  FILE *f1 = fopen(chemin, "r");
  if (f1 == NULL) {
    printf("Cannot open file %s\n", chemin);
    return;
  }
  while (fgets(buf, 499, f1) != NULL) {
    if (strncmp(buf, "State", 5) == 0) {
      snprintf(buffer, 999, "[%d] %s", (int) pid, buf);
      printf("%s", buffer);
    }
    if (strncmp(buf, "Tgid", 4) == 0) {
      snprintf(buffer, 999, "[%d] %s", (int) pid, buf);
      printf("%s", buffer);
    }
    if (strncmp(buf, "PPid", 4) == 0) {
      snprintf(buffer, 999, "[%d] %s", (int) pid, buf);
      printf("%s", buffer);
    }
  }
}

//fonction info_user avec paramettre NOM
void info_user_name(char *name) {
  struct passwd *result;
  result = getpwnam(name);
  if (result == NULL) {
    if (errno == 0) {
      fprintf(stderr, "le login %s n'est pas trouvé", name);
    } else {
      perror("info_user_name");
    }
    return;
  }
  printf("Login: %s;\n", result->pw_name);
  printf("Nom réel: %s;\n", result->pw_gecos);
  printf("UID: %d;\n", result->pw_uid);
  printf("Login: %d;\n", result->pw_gid);
  printf("Répertoire dédié: %s;\n", result->pw_dir);
  printf("Shell: %s;\n", result->pw_shell);
}

//fonction info_user avec paramettre uid
void info_user_uid(uid_t uid) {
  struct passwd *result;
  result = getpwuid(uid);
  if (result == NULL) {
    perror("info_user_uid");
    return;
  }
  printf("Login: %s;\n", result->pw_name);
  printf("Nom réel: %s;\n", result->pw_gecos);
  printf("UID: %d;\n", result->pw_uid);
  printf("Login: %d;\n", result->pw_gid);
  printf("Répertoire dédié: %s;\n", result->pw_dir);
  printf("Shell: %s;\n", result->pw_shell);
}
