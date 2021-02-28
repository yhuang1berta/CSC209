/* This program simulates a file system within an actual file.  The
 * simulated file system has only one directory, and two types of
 * metadata structures defined in simfstypes.h.
 */

/* Simfs is run as:
 * simfs -f myfs command args
 * where the -f option specifies the actual file that the simulated file system
 * occupies, the command is the command to be run on the simulated file system,
 * and a list of arguments for the command is represented by args.  Note that
 * different commands take different numbers of arguments.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

// We using the ops array to match the file system command entered by the user.
#define MAXOPS  6
char *ops[MAXOPS] = {"initfs", "printfs", "createfile", "readfile",
                     "writefile", "deletefile"};
int find_command(char *);

int
main(int argc, char **argv)
{
    int oc;       /* option character */
    char *cmd;    /* command to run on the file system */
    char *fsname; /* name of the simulated file system file */

    char *usage_string = "Usage: simfs -f file cmd arg1 arg2 ...\n";

    /* Get and check the arguments */
    if(argc < 4) {
        fputs(usage_string, stderr);
        exit(1);
    }

    while((oc = getopt(argc, argv, "f:")) != -1) {
        switch(oc) {
        case 'f' :
            fsname = optarg;
            break;
        default:
            fputs(usage_string, stderr);
            exit(1);
        }
    }

    /* Get the command name */
    cmd = argv[optind];
    optind++;

    // initalize arguments to pass into commands
    int start;
    int length;
    char *start_leftover;
    char *length_leftover;

    switch((find_command(cmd))) {
    case 0: /* initfs */
        initfs(fsname);
        break;
    case 1: /* printfs */
        printfs(fsname);
        break;
    case 2: /* createfile */
        // fprintf(stderr, "Error: createfile not yet implemented\n");
        if(argc != 5) {
            fprintf(stderr, "Error: incorrect number of arguments\n");
            exit(1);
        } else if (strlen(argv[4]) > 12) {
            fprintf(stderr, "Error: name too long\n");
            exit(1);
        } else if (strlen(argv[4]) == 0) {
            fprintf(stderr, "Error: name cannot be empty\n");
            exit(1);
        }
        createfile(fsname, argv[4]);
        break;
    case 3: /* readfile */
        // fprintf(stderr, "Error: readfile not yet implemented\n");
        if(argc != 7) {
            fprintf(stderr, "Error: incorrect number of arguments\n");
            exit(1);
        }
        start = (int)strtol(argv[5], &start_leftover, 10);
        if (start < 0) {
            fprintf(stderr, "Error: invalid start value\n");
            exit(1);
        }
        if (strlen(start_leftover) > 0) {
            fprintf(stderr, "Error: trying to pass non-numeric value as start\n");
            exit(1);
        }
        length = (int)strtol(argv[6], &length_leftover, 10);
        if (strlen(length_leftover) > 0) {
            fprintf(stderr, "Error: trying to pass non-numeric value as length\n");
            exit(1);
        }
        readfile(fsname, argv[4], start, length);
        break;
    case 4: /* writefile */
        // fprintf(stderr, "Error: writefile not yet implemented\n");
        if(argc != 7) {
            fprintf(stderr, "Error: missing argument\n");
            exit(1);
        }
        start = (int)strtol(argv[5], &start_leftover, 10);
        if (start < 0) {
            fprintf(stderr, "Error: invalid start value\n");
            exit(1);
        }
        if (strlen(start_leftover) > 0) {
            fprintf(stderr, "Error: trying to pass non-numeric value as start\n");
            exit(1);
        }
        length = (int)strtol(argv[6], &length_leftover, 10);
        if (strlen(length_leftover) > 0) {
            fprintf(stderr, "Error: trying to pass non-numeric value as length\n");
            exit(1);
        }
        if (length != 0) {
            writefile(fsname, argv[4], start, length);
        }
        break;
    case 5: /* deletefile */
        // fprintf(stderr, "Error: deletefile not yet implemented\n");
        if(argc != 5) {
            fprintf(stderr, "Error: missing argument\n");
            exit(1);
        }
        deletefile(fsname, argv[4]);
        break;
    default:
        fprintf(stderr, "Error: Invalid command\n");
        exit(1);
    }

    return 0;
}

/* Returns a integer corresponding to the file system command that
 * is to be executed
 */
int
find_command(char *cmd)
{
    int i;
    for(i = 0; i < MAXOPS; i++) {
        if ((strncmp(cmd, ops[i], strlen(ops[i]))) == 0) {
            return i;
        }
    }
    fprintf(stderr, "Error: Command %s not found\n", cmd);
    return -1;
}