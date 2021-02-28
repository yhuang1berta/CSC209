#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: pass in a string as the file name of the test case");
        return 1;
    }
    char *file_name = argv[1];
    FILE *fp;
    if ((fp = fopen(file_name, "wb")) == NULL) {
        perror("fopen");
        return 1;
    }
    int *int_arr = malloc(sizeof(int) * 100);
    int i;
    for (i = 0; i < 100; i++) {
        int r = (rand() % 100);
        int_arr[i] = r;
    }
    if (fwrite(int_arr, sizeof(int), 100, fp) == 0) {
        perror("fwrite");
        return 1;
    }
    if (fclose(fp) != 0) {
        perror("fclose");
        return 1;
    }
    free(int_arr);
    printf("test case generated\n");
    return 0;
}