/* This file contains functions that are not part of the visible "interface".
 * They are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if(fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}

int find_empty_fentry(fentry *files) {
    // finds the first empty fentry in a fs, then returns the index of the empty fentry or -1 if no empty fentry can be found
    int i;
    for (i = 0; i < MAXFILES; i++) {
        if (strlen(files[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

int find_empty_fnode(fnode *fnodes) {
    // finds the first empty fentry in a fs, then returns the index of the empty fentry or -1 if no empty fentry can be found
    int i;
    for (i = 0; i < MAXBLOCKS; i++) {
        if (fnodes[i].blockindex < 0) {
            return i;
        }
    }
    return -1;
}

int find_fentry(fentry *files, char *entry_name) {
    // returns the index of the fentry with <entry_name>, or -1 if <entry_name> is not in fentry of the provided file
    int i;
    for (i = 0; i < MAXFILES; i++) {
        if (strncmp(files[i].name, entry_name, 12) == 0) {
            return i;
        }
    }
    return -1;
}

int block_required(fnode *fnodes, int length) {
    // returns the number of blocks required to store <length> byte of data, or -1 if there isn't enough blocks to store it
    int count = 0;
    while (length > 0) {
        length -= BLOCKSIZE;
        count++;
    }
    int i;
    int empty_count = 0;
    // searches for empty fnode
    for (i = 0; i < MAXBLOCKS; i++) {
        if (fnodes[i].blockindex < 0) {
            empty_count++;
        }
    }
    if (empty_count >= count) {
        return count;
    }
    return -1;
}

int find_block_used(int bytes) {
    int count = 0;
    while (bytes > 0) {
        bytes -= BLOCKSIZE;
        count++;
    }
    return count;
}

/* File system operations: creating, deleting, reading, and writing to files.
 */

void createfile(char *filename, char *new_file) {
    // opening filename
    FILE *file = openfs(filename, "r+b");
     if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }
    // initialize metadata
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    // read the fentry
    if ((fread(files, sizeof(fentry), MAXFILES, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // check if file already exists
    if (find_fentry(files, new_file) != -1) {
        fprintf(stderr, "Error: file already exists in this file system\n");
        closefs(file);
        exit(1);
    }
    // read the fnodes
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // initialize new fentry to be written
    fentry new;
    new.name[0] = '\0';
    new.size = 0;
    new.firstblock = -1;
    strncat(new.name, new_file, strlen(new_file));

    int index = find_empty_fentry(files);
    if (index == -1) {
        fprintf(stderr, "Error: No empty fentry could be found\n");
        closefs(file);
        exit(1);
    }
    if (fseek(file, sizeof(fentry) * index, SEEK_SET) != 0) {
        fprintf(stderr, "Error: fseek to empty fentry failed\n");
        closefs(file);
        exit(1);
    }
    if(fwrite(&new, sizeof(fentry), 1, file) != 1) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(file);
        exit(1);
    }
    closefs(file);
}

void writefile(char *filename, char *new_file, int start, int length) {
    // checking for edge cases
    if (length < 0) {
        fprintf(stderr, "Error: invalid writing length");
        exit(1);
    } else if (length == 0) {
        exit(0);
    }
    // opening file
    FILE *file = openfs(filename, "r+b");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }
        // initializes metadata
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    // calculate the space taken by zero to fill in the current block
    int bytes_used = sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS;
    int bytes_to_write = (BLOCKSIZE - (bytes_used % BLOCKSIZE)) % BLOCKSIZE;
    char zerobuf[BLOCKSIZE] = {0};
    // read the fentry
    if ((fread(files, sizeof(fentry), MAXFILES, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // read the fnodes
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // check if file already exists
    int file_index = find_fentry(files, new_file);
    if (file_index == -1) {
        fprintf(stderr, "Error: writing to non-existing file\n");
        closefs(file);
        exit(1);
    }
    // check the specified fentry
    fentry file_entry = files[file_index];
    // checking for block indices
    if (file_entry.firstblock >= MAXBLOCKS || file_entry.firstblock < -1) {
        fprintf(stderr, "Error: corrupted file system");
        closefs(file);
        exit(1);
    }
    if ((file_entry.size > 0 && file_entry.size < start-1) || (file_entry.size == 0 && start != 0)) {
        fprintf(stderr, "Error: starting position exceeds file size\n");
        closefs(file);
        exit(1);
    }
    short curr_node_index = file_entry.firstblock;
    // find the number of blocks used to store metadata
    int block_used = find_block_used(sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS);
    // initialize buffers
    char *buffer = malloc(sizeof(char) * (BLOCKSIZE + 1));
    buffer[0] = '\0';
    char temp_buf[length+10];
    temp_buf[0] = '\0';
    // initialize memory buffer array
    char *block_arr[MAXBLOCKS - block_used];
    int j = block_used;
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        block_arr[j] = malloc(sizeof(char) * (BLOCKSIZE + 1));
        block_arr[j][0] = '\0';
    }
    // store all existing blocks
    j = block_used;
    while (j < MAXBLOCKS) {
        if (fnodes[j].blockindex >= 0) {
            fseek(file, BLOCKSIZE * j, SEEK_SET);
            fread(buffer, BLOCKSIZE, 1, file);
            strncpy(block_arr[j - block_used], buffer, BLOCKSIZE);
            buffer[0] = '\0';
        }
        j++;
    }
    // initialize new block for data
    if (curr_node_index == -1) {
        curr_node_index = find_empty_fnode(fnodes);
        if (curr_node_index == -1) {
            fprintf(stderr, "Error: cannot initialize a new block to store data\n");
            closefs(file);
            exit(1);
        }
        file_entry.firstblock = curr_node_index;
        (fnodes[file_entry.firstblock]).blockindex *= -1;
    }
    // find the block we will write in and the starting place of the string we will start writing at 
    int temp_start = start;
    while (temp_start >= BLOCKSIZE && fnodes[curr_node_index].nextblock != -1) {
        temp_start -= BLOCKSIZE;
        curr_node_index = fnodes[curr_node_index].nextblock;
    }
    int curr_block_space = BLOCKSIZE - temp_start;
    // check if more blocks need to be allocated
    if (start + length > file_entry.size && length > curr_block_space) {
        int block_count = block_required(fnodes, length - curr_block_space);
        if (block_count == -1) {
            fprintf(stderr, "Error: not enough space to store the data with %d bytes\n", length);
            closefs(file);
            exit(1);
        } else {
            // move to the starting address of data
            if (fseek(file, BLOCKSIZE * block_used, SEEK_SET) != 0) {
                fprintf(stderr, "Error: fseek to empty fentry failed\n");
                closefs(file);
                exit(1);
            }
            char *heap_buf = malloc(sizeof(char) * (length+10));
            heap_buf[0] = '\0';
            // receiving inputs
            if (fread(temp_buf, 1, length+10, stdin) != length) {
                fprintf(stderr, "Error: input size does not match indicated length (note: if input is from stdin, enter counts as one byte)\n");
                closefs(file);
                exit(1);
            }
            strncat(heap_buf, temp_buf, length);
            // fill in the first not-full block
            int i = 0;
            int concat_len;
            int dummy_start = start;
            while (dummy_start < BLOCKSIZE || fnodes[curr_node_index].nextblock != -1) {
                temp_buf[0] = '\0';
                block_arr[curr_node_index - block_used][temp_start] = '\0';
                if (curr_node_index == file_entry.firstblock) {
                    strncat(block_arr[curr_node_index - block_used], heap_buf, BLOCKSIZE - temp_start);
                    strncpy(temp_buf, block_arr[curr_node_index - block_used], BLOCKSIZE);
                    strncpy(block_arr[curr_node_index - block_used], temp_buf, BLOCKSIZE);
                    i += BLOCKSIZE - temp_start;
                    temp_start = 0;
                } else {
                    temp_start = 0;
                    concat_len = BLOCKSIZE - strlen(block_arr[curr_node_index - block_used]);
                    strncat(block_arr[curr_node_index - block_used], &(heap_buf[i]), concat_len);
                    i += concat_len;
                }
                if (fnodes[curr_node_index].nextblock == -1) {
                    break;
                }
                curr_node_index = fnodes[curr_node_index].nextblock;
                dummy_start += BLOCKSIZE;
            }
            // store the rest of the data in separate blocks
            while (i < length) {
                if (temp_start > BLOCKSIZE) {
                    temp_start -= BLOCKSIZE;
                }
                buffer[0] = '\0';
                short next_block_index = find_empty_fnode(fnodes);
                (fnodes[curr_node_index]).nextblock = next_block_index;
                (fnodes[next_block_index]).blockindex *= -1;
                strncat(buffer, &(heap_buf[i]), BLOCKSIZE);
                strncpy(block_arr[next_block_index - block_used], buffer, BLOCKSIZE);
                i += strlen(buffer);
                curr_node_index = next_block_index;
            }
            if (start + length > file_entry.size) {
                file_entry.size = start + length;
            }
            files[file_index] = file_entry;
            // write changes to disk
            rewind(file);
            if(fwrite(files, sizeof(fentry), MAXFILES, file) < MAXFILES) {
                fprintf(stderr, "Error: re-write fentry failed on writefile\n");
                closefs(file);
                exit(1);
            }
            if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, file) < MAXBLOCKS) {
                fprintf(stderr, "Error: re-write fnode failed on writefile\n");
                closefs(file);
                exit(1);
            }
            /* Write buffer bytes to the file to fill the current block. */
            if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, file) < 1) {
                fprintf(stderr, "Error: write failed on init\n");
                closefs(file);
                exit(1);
            }
            // re-write the data blocks back into disk
            for (j = 0; j < MAXBLOCKS - block_used; j++) {
                if (strlen(block_arr[j]) > 0) {
                    if(fwrite(block_arr[j], BLOCKSIZE, 1, file) == 0) {
                        fprintf(stderr, "Error: write block failed on writefile\n");
                        closefs(file);
                        exit(1);
                    }
                }
            }
            // freeing heap-buffer and block_buffers
            for (j = 0; j < MAXBLOCKS - block_used; j++) {
                free(block_arr[j]);
            }
            free(heap_buf);
            free(buffer);
            closefs(file);
            exit(0);
        }
    }
    // don't need to allocate new block
    temp_buf[0] = '\0';
    int error = fread(temp_buf, 1, length+10, stdin);
    if (error != length) {
        fprintf(stderr, "Error: input size does not match indicated length (note: if input is from stdin, enter counts as one byte)\n");
        closefs(file);
        exit(1);
    }
    temp_buf[length] = '\0';
    int dummy_length = length;
    int buffer_start = 0;
    while (dummy_length > 0) {
        char after_buffer[BLOCKSIZE] = {0};
        strncpy(after_buffer, block_arr[curr_node_index - block_used], BLOCKSIZE);
        block_arr[curr_node_index - block_used][temp_start] = '\0';
        strncat(block_arr[curr_node_index - block_used], &(temp_buf[buffer_start]), BLOCKSIZE - temp_start);
        for (j = strlen(block_arr[curr_node_index - block_used]); j < BLOCKSIZE; j++) {
            block_arr[curr_node_index - block_used][j] = after_buffer[j];
        }
        dummy_length -= BLOCKSIZE - temp_start;
        buffer_start += BLOCKSIZE - temp_start;
        curr_node_index = fnodes[curr_node_index].nextblock;
        temp_start = 0;
    }
    if (start + length > file_entry.size) {
        file_entry.size = start + length;
    }
    files[file_index] = file_entry;
    rewind(file);
    if(fwrite(files, sizeof(fentry), MAXFILES, file) < MAXFILES) {
        fprintf(stderr, "Error: re-write fentry failed on writefile\n");
        closefs(file);
        exit(1);
    }
    if (fwrite(fnodes, sizeof(fnode), MAXBLOCKS, file) < MAXBLOCKS) {
        fprintf(stderr, "Error: re-write fnode failed on writefile\n");
        closefs(file);
        exit(1);
    }
    /* Write buffer bytes to the file to fill the current block. */
    if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, file) < 1) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(file);
        exit(1);
    }
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        if (strlen(block_arr[j]) > 0) {
            if(fwrite(block_arr[j], BLOCKSIZE, 1, file) == 0) {
                fprintf(stderr, "Error: write block failed on writefile\n");
                closefs(file);
                exit(1);
            }
        }
    }
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        free(block_arr[j]);
    }
    free(buffer);
    closefs(file);
    exit(0);
}

void readfile(char *filename, char *new_file, int start, int length) {
    // opening file
    FILE *file = openfs(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }
    // initializes metadata
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    // read the fentry
    if ((fread(files, sizeof(fentry), MAXFILES, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // read the fnodes
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // check if file already exists
    int file_index = find_fentry(files, new_file);
    if (file_index == -1) {
        fprintf(stderr, "Error: trying to read non-existing file\n");
        closefs(file);
        exit(1);
    }
    // check the specified fentry
    fentry file_entry = files[file_index];
    // checking for block indices
    if (file_entry.firstblock >= MAXBLOCKS || file_entry.firstblock < -1) {
        fprintf(stderr, "Error: corrupted file system");
        closefs(file);
        exit(1);
    }
    if ((file_entry.size > 0 && file_entry.size < start-1) || (file_entry.size == 0 && start != 0)) {
        fprintf(stderr, "Error: starting position exceeds file size\n");
        closefs(file);
        exit(1);
    }
    // checking for length edge cases
    if (length < 0 || (file_entry.size - start < length)) {
        fprintf(stderr, "Error: invalid reading length\n");
        exit(1);
    } else if (length == 0) {
        exit(0);
    } else if (start + length > file_entry.size) {
        fprintf(stderr, "Error: invalid reading length\n");
        exit(1);
    }
    // find the number of blocks used to store metadata
    int block_used = find_block_used(sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS);
    char buffer[BLOCKSIZE+1];
    buffer[0] = '\0';
    // store all blocks data to block_arr
    char *block_arr[MAXBLOCKS - block_used];
    int j;
    char zerobuf[BLOCKSIZE] = {0};
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        block_arr[j] = malloc(sizeof(char) * (BLOCKSIZE + 1));
        strncpy(block_arr[j], zerobuf, BLOCKSIZE);
    }
    // store all existing blocks
    j = block_used;
    while (j < MAXBLOCKS) {
        if (fnodes[j].blockindex >= 0) {
            fseek(file, BLOCKSIZE * j, SEEK_SET);
            fread(buffer, BLOCKSIZE, 1, file);
            strncpy(block_arr[j - block_used], buffer, BLOCKSIZE);
            buffer[0] = '\0';
        }
        j++;
    }
    char print_buffer[file_entry.size+1];
    for (j = 0; j < file_entry.size+1; j++) {
        print_buffer[j] = '\0';
    }
    int curr_node_index = file_entry.firstblock;
    while (curr_node_index != -1 && fnodes[curr_node_index].blockindex != -1) {
        strncat(print_buffer, block_arr[curr_node_index - block_used], BLOCKSIZE);
        curr_node_index = fnodes[curr_node_index].nextblock;
    }
    for (j = start; j < start+ length; j++) {
        if (fwrite(&(print_buffer[j]), 1, 1, stdout) != 1) {
            fprintf(stderr, "Error: writing result to standard out failed\n");
            closefs(file);
            exit(1);
        }
    }
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        free(block_arr[j]);
    }
    closefs(file);
    exit(0);
}

void deletefile(char *filename, char *new_file) {
    // opening file
    FILE *file = openfs(filename, "r+b");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", filename);
        exit(1);
    }
    // initializes metadata
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];
    // read the fentry
    if ((fread(files, sizeof(fentry), MAXFILES, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // read the fnodes
    if ((fread(fnodes, sizeof(fnode), MAXBLOCKS, file)) == 0) {
        fprintf(stderr, "Error: could not read file entries\n");
        closefs(file);
        exit(1);
    }
    // check if file already exists
    int file_index = find_fentry(files, new_file);
    if (file_index == -1) {
        fprintf(stderr, "Error: trying to read non-existing file\n");
        closefs(file);
        exit(1);
    }
    // check the specified fentry
    fentry file_entry = files[file_index];
    // checking for block indices
    if (file_entry.firstblock >= MAXBLOCKS || file_entry.firstblock < -1) {
        fprintf(stderr, "Error: corrupted file system");
        closefs(file);
        exit(1);
    }
    // find the number of blocks used to store metadata
    int block_used = find_block_used(sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS);
    char buffer[BLOCKSIZE+1];
    buffer[0] = '\0';
    // store all blocks data to block_arr
    char *block_arr[MAXBLOCKS - block_used];
    int j;
    char zerobuf[BLOCKSIZE] = {0};
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        block_arr[j] = malloc(sizeof(char) * (BLOCKSIZE + 1));
        strncpy(block_arr[j], zerobuf, BLOCKSIZE);
    }
    // store all existing blocks
    j = block_used;
    while (j < MAXBLOCKS) {
        if (fnodes[j].blockindex >= 0) {
            fseek(file, BLOCKSIZE * j, SEEK_SET);
            fread(buffer, BLOCKSIZE, 1, file);
            strncpy(block_arr[j - block_used], buffer, BLOCKSIZE);
            buffer[0] = '\0';
        }
        j++;
    }
    int temp;
    int curr_node_index = file_entry.firstblock;
    while (curr_node_index != -1 && fnodes[curr_node_index].blockindex != -1) {
        strncpy(block_arr[curr_node_index - block_used], zerobuf, BLOCKSIZE);
        fnodes[curr_node_index].blockindex *= -1;
        if (fnodes[curr_node_index].nextblock != -1) {
            temp = curr_node_index;
            curr_node_index = fnodes[curr_node_index].nextblock;
            fnodes[temp].nextblock = -1;
        } else {
            break;
        }
    }
    for (j = 0; j < 12; j++) {
        file_entry.name[j] = 0;
    }
    file_entry.firstblock = -1;
    file_entry.size = 0;
    files[file_index] = file_entry;
    // writeback to disk
    int bytes_used = sizeof(fentry) * MAXFILES + sizeof(fnode) * MAXBLOCKS;
    int bytes_to_write = (BLOCKSIZE - (bytes_used % BLOCKSIZE)) % BLOCKSIZE;
    rewind(file);
    if(fwrite(files, sizeof(fentry), MAXFILES, file) < MAXFILES) {
        fprintf(stderr, "Error: re-write fentry failed on writefile\n");
        closefs(file);
        exit(1);
    }
    if (fwrite(fnodes, sizeof(fnode), MAXBLOCKS, file) < MAXBLOCKS) {
        fprintf(stderr, "Error: re-write fnode failed on writefile\n");
        closefs(file);
        exit(1);
    }
    /* Write buffer bytes to the file to fill the current block. */
    if (bytes_to_write != 0  && fwrite(zerobuf, bytes_to_write, 1, file) < 1) {
        fprintf(stderr, "Error: write failed on init\n");
        closefs(file);
        exit(1);
    }
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        if (fwrite(block_arr[j], BLOCKSIZE, 1, file) == 0) {
            fprintf(stderr, "Error: write block failed on writefile\n");
            closefs(file);
            exit(1);
        }
    }
    for (j = 0; j < MAXBLOCKS - block_used; j++) {
        free(block_arr[j]);
    }
    closefs(file);
    exit(0);
}
// Signatures omitted; design as you wish.