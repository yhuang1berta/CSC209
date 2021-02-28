#include "closest_parallel.h"
#include "closest_serial.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int curr_depth = 0;

double _closest_parallel(struct Point P[], size_t n, int pdmax, int *pcount)
{
    static int num_forks = 0;
    // step 1).
    // int pdepth = pdmax;
    // step 2). base case
    if (n < 4 || pdmax == 0) {
        return _closest_serial(P, n);
    }
    // step 3). create 2 child processes
    // initialize 2 pipes for 2 child processes
    int pipe1[2];
    int pipe2[2];
    // forking first child
    if (pipe(pipe1) == -1) {
        perror("pipe");
        exit(1);
    }
    int result1 = fork();
    if (result1 < 0) {
        perror("fork");
        exit(1);
    } else if (result1 == 0) {
        // child process
        // close read-end of pipe for child1
        if (close(pipe1[0]) != 0) {
            perror("close");
            exit(1);
        }
        // call _closest_parallel on the left half of the array
        double left_closest = _closest_parallel(P, n/2, pdmax-1, pcount);
        // write result to pipe1
        if (write(pipe1[1], &left_closest, sizeof(double)) == -1) {
            perror("write");
            exit(1);
        }
        // close wirte end
        if (close(pipe1[1]) != 0) {
            perror("close");
            exit(1);
        }
        // exit
        exit(num_forks);
    }
    // forking second child
    if (pipe(pipe2) == -1) {
        perror("pipe");
    }
    int result2 = fork();
    if (result2 < 0) {
        perror("pipe");
        exit(1);
    } else if (result2 == 0) {
        // child process
        // close read-end of pipe of child2
        if (close(pipe1[0]) != 0 || close(pipe1[1]) != 0 || close(pipe2[0]) != 0) {
            perror("close");
            exit(1);
        }
        // call closest_parallel on the right half of the array
        double right_closest = _closest_parallel(&(P[n/2]), n-(n/2), pdmax-1, pcount);
        // write result of pipe2
        if (write(pipe2[1], &right_closest, sizeof(double)) == -1) {
            perror("write");
            exit(1);
        }
        // close write end
        if (close(pipe2[1]) != 0) {
            perror("close");
            exit(1);
        }
        //exit
        exit(num_forks);
    }
    // parent process
    if (result1 > 0 && result2 > 0) {
        // close both write ends for pipe1 and pipe2
        if (close(pipe1[1]) != 0 || close(pipe2[1]) != 0) {
            perror("close");
            exit(1);
        }
        // wait for both children
        int status1, status2;
        if (wait(&status1) == -1 || wait(&status2) == -1) {
            perror("wait");
            exit(1);
        }
        // check if children exitted correctly
        if (WIFEXITED(status1) && WIFEXITED(status2)) {
            if (WEXITSTATUS(status1) == -1 || WEXITSTATUS(status2) == -1) {
                exit(1);
            }
            num_forks += WEXITSTATUS(status1) + WEXITSTATUS(status2) + 2;
            // update pcount
            (*pcount) += num_forks;
        } else {
            exit(1);
        }
        // read the children's return value
        double child1_r, child2_r;
        if (read(pipe1[0], &child1_r, sizeof(double)) == -1 || read(pipe2[0], &child2_r, sizeof(double)) == -1) {
            perror("read");
            exit(1);
        }
        // close read ends of pipe1 and pipe2
        if (close(pipe1[0]) != 0 || close(pipe2[0]) != 0) {
            perror("close");
            exit(1);
        }
        // compare the distance between child1_r and child2_r
        double d;
        if (child1_r > child2_r) {
            d = child2_r;
        } else {
            d = child1_r;
        }
        // call combine_lr from closest_serial.c to find the closest distance
        return combine_lr(P, n, P[n/2], d);
    } else {
        // something's wrong with fork 1 or fork 2
        perror("fork");
        exit(1);
    }
}

double closest_parallel(struct Point P[], size_t n, int pdmax, int *pcount)
{
    qsort(P, n, sizeof(struct Point), compare_x);
    *pcount = 0;
    return _closest_parallel(P, n, pdmax, pcount);
}
