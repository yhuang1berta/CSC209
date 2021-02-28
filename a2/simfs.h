#include <stdio.h>
#include "simfstypes.h"

/* File system operations */
void printfs(char *);
void initfs(char *);
void createfile(char *, char *);
void writefile(char *, char *, int, int);
void readfile(char *, char *, int, int);
void deletefile(char *, char *);

/* Internal functions */
FILE *openfs(char *filename, char *mode);
void closefs(FILE *fp);

/* Helper functions */
int find_empty_fentry(fentry *);
int find_empty_fnode(fnode *);
int find_fentry(fentry *, char *);
int block_required(fnode *, int);
int find_block_used(int);