#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int check_args(int argc);
int break_system(char *comando);
void sequential_execute(char *comandos);
void *parallel_execute(void *comando);
void file_execute(const char *filename);

int main(int argc, char *argv[]) {
  size_t tamanho_arg = 0;
  ssize_t tamanho_linha;
  if (check_args(argc) == 0) {
    char *comando;
    char style[4] = "seq";
    while (1) {
      printf("pmc3 %s>", style);
      fflush(stdout);

      getline(&comando, &tamanho_arg, stdin);

      if (comando[strlen(comando) - 1] == '\n') {
        comando[strlen(comando) - 1] = '\0';
      }

      if (break_system(comando)) {
        break;
      }

      if (strcmp(comando, "style parallel") == 0) {
        strcpy(style, "par");
      } else if (strcmp(comando, "style sequential") == 0) {
        strcpy(style, "seq");
      }

      if (strcmp(comando, "style parallel") == 0 ||
          strcmp(comando, "style sequential") == 0) {
        continue;
      }

      if (strcmp(style, "seq") == 0) {
        sequential_execute(comando);
      } else if (strcmp(style, "par") == 0) {
        pthread_t thread;
        pthread_create(&thread, NULL, parallel_execute, (void *)comando);
        pthread_join(thread, NULL);
      }
    }
  } else if (check_args(argc) == 1) {
    file_execute(argv[1]);
  } else {
    printf("Too much arguments.\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

int check_args(int argc) {
  if (argc == 1) {
    return 0;
  } else if (argc == 2) {
    return 1;
  } else {
    return -1;
  }
}

int break_system(char *comando) {
  if (feof(stdin)) {
    printf("Ctrl + D\n");
    return 1;
  }
  if (strcmp(comando, "exit") == 0) {
    return 1;
  } else {
    return 0;
  }
}

void sequential_execute(char *comandos) {
  pid_t child = fork();
  if (child == -1) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }

  if (child == 0) {
    char *comando;
    char *aux = strtok(comandos, ";");
    while (aux != NULL) {
      comando = aux;
      while (*comando == ' ' || *comando == '\t') {
        comando++;
      }

      if (strlen(comando) > 0) {
        pid_t grandson = fork();

        if (grandson == -1) {
          perror("Fork failed");
          exit(EXIT_FAILURE);
        }

        if (grandson == 0) {
          execlp("/bin/sh", "sh", "-c", comando, NULL);
          perror("execlp");
          exit(1);
        } else {
          wait(NULL);
        }
      }
      aux = strtok(NULL, ";");
    }
    exit(1);
  } else {
    wait(NULL);
  }
}

void *parallel_execute(void *comandos) {
  char *comando = (char *)comandos;
  char *aux;
  char *saveptr;

  aux = strtok_r(comando, ";", &saveptr);

  while (aux != NULL) {
    char *command = aux;
    while (*command == ' ' || *command == '\t') {
      command++;
    }

    if (strlen(command) > 0) {
      pthread_t thread;
      int *status = malloc(sizeof(int));
      if (status == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
      }

      *status = system(command);

      if (*status != 0) {
        fprintf(stderr, "Error while executing command: %s\n", command);
      }

      free(status);
    }

    aux = strtok_r(NULL, ";", &saveptr);
  }

  pthread_exit(NULL);
}

void file_execute(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, file)) != -1) {
    if (line[read - 1] == '\n') {
      line[read - 1] = '\0';
    }

    if (break_system(line)) {
      break;
    }

    sequential_execute(line);
  }

  free(line);
  fclose(file);
}