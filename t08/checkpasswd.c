#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  // initialize pipe
  int fd[2];
  if (pipe(fd) == -1) {
    perror("pipe");
  }
  // fork
  int n;
  n = fork();
  if (n == 0) {
    // child
    close(fd[1]);
    // set stdin to pipe's read end
    dup2(fd[0], STDIN_FILENO);
    execl("./validate", "./validate", NULL);
    perror("execl");
    exit(1);
  } else {
    // parent
    int status;
    close(fd[0]);
    if (write(fd[1], user_id, MAX_PASSWORD) == -1) {
      perror("write");
    }
    if (write(fd[1], password, MAX_PASSWORD) == -1) {
      perror("write");
    }
    wait(&status);
    if (!WIFEXITED(status)) {
      fprintf(stderr, "An error occurred\n");
      return 1;
    }
    if (WEXITSTATUS(status) == 0) {
      printf(SUCCESS);
    } else if (WEXITSTATUS(status) == 2) {
      printf(INVALID);
    } else if (WEXITSTATUS(status) == 3) {
      printf(NO_USER);
    } else {
      fprintf(stderr, "An error occurred in validate\n");
      return 1;
    }
  }

  return 0;
}
