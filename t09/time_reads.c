#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>


// Message to print in the signal handling function. 
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed.
 * These have to be global so that signal handler to be written will have
 * access.
 */
long num_reads = 0, seconds;
// defining global variable to use to store the number of times read has been called
long int read_times = 0;

// defining signal handler for sigaction
void handler(int signal) {
  struct itimerval curr_timer;
  long int seconds_spent;
  getitimer(ITIMER_PROF, &curr_timer);
  seconds_spent = curr_timer.it_value.tv_sec;
  fprintf(stderr, MESSAGE, read_times, seconds - seconds_spent);
  exit(0);
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);

    FILE *fp;
    if ((fp = fopen(argv[2], "r+")) == NULL) {    // Read+Write for later ...
      perror("fopen");
      exit(1);
    }

    // install new action for SIGPROF
    struct sigaction newaction;
    newaction.sa_handler = handler;
    newaction.sa_flags = 0;
    sigemptyset(&newaction.sa_mask);
    sigaction(SIGPROF, &newaction, NULL);

    // set timer for the infinite loop
    struct timeval start_time = {seconds, 0};
    struct timeval stop_time = {0, 0};
    struct itimerval timer = {stop_time, start_time};
    if (setitimer(ITIMER_PROF, &timer, NULL) != 0) {
      perror("setitimer");
      exit(1);
    }
    /* In an infinite loop, read an int from a random location in the file
     * and print it to stderr.
     */
    for (;;) {
      int offset = (rand() % 100) * 4;
      int read_data;
      if (fseek(fp, offset, SEEK_SET) != 0) {
        perror("fseek");
        return 1;
      }
      if (fread(&read_data, sizeof(int), 1, fp) == 0) {
        perror("fread");
        return 1;
      }
      read_times++;
      printf("%d\n", read_data);
    }

    return 1;  //something is wrong if we ever get here!
}

