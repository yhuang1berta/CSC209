#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "socket.h"

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

int main(void) {
    struct server_sock s;
    s.inbuf = 0;
    int exit_status = 0;
    
    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(s.sock_fd);
        exit(1);
    }

    char *buf; // Buffer to read name from stdin
    int name_valid = 0;
    while(!name_valid) {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        size_t name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0) {
            perror("getline");
	        free(buf);
            fprintf(stderr, "Error reading username.\n");
            exit(1);
        }
        if (name_len - 1 > MAX_NAME) { // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
	        free(buf);
        }
        else {
            // added check for space in user name
	    char flag = 0;
            for (int i = 0; i < name_len; i++) {
                if (buf[i] == ' ') {
                    printf("Username cannot contain any space\n");
		    flag = 1;
		    free(buf);
		    break;
                }
            }
	    if (flag) {
		continue;
	   }
            // Replace LF+NULL with CR+LF
	    char *new_buf = malloc(sizeof(char) * name_len+3);
	    memset(new_buf, '1', 1);
	    memcpy(&(new_buf[1]), buf, name_len);
	    memset(&(new_buf[name_len+1]), '\r', 1);
	    memset(&(new_buf[name_len+2]), '\n', 1);
            if (write_to_socket(s.sock_fd, new_buf, name_len+3)) {
                fprintf(stderr, "Error sending username.\n");
		free(buf);
		free(new_buf);
                exit(1);
            }
            name_valid = 1;
	    free(buf);
	    free(new_buf);
        }
    }
    
    /*
     * See here for why getline() is used above instead of fgets():
     * https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152445
     */
    
    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */
    char *msg_buf;
    char newline_flag = 0;
    // int saved_stdin, saved_stdout, saved_stderr;
    int status, emote_bytes_read;
    fd_set all_fds, read_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    FD_SET(s.sock_fd, &all_fds);

    /*
     * Step 2: Using select, monitor the socket for incoming mesages
     * from the server and stdin for data typed in by the user.
     */
    while(1) {
        read_fds = all_fds;
        int nready = select(s.sock_fd + 1, &read_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit_status = 1;
            break;
        }
        
        /*
         * Step 3: Read user-entered message from the standard input
         * stream. We should read at most MAX_USR_MSG bytes at a time.
         * If the user types in a message longer than MAX_USR_MSG,
         * we should leave the rest of the message in the standard
         * input stream so that we can read it later when we loop
         * back around.
         * 
         * In other words, an oversized messages will be split up
         * into smaller messages. For example, assuming that
         * MAX_USR_MSG is 10 bytes, a message of 22 bytes would be
         * split up into 3 messages of length 10, 10, and 2,
         * respectively.
         * 
         * It will probably be easier to do this using a combination of
         * fgets() and ungetc(). You probably don't want to use
         * getline() as was used for reading the user name, because
         * that would read all the bytes off of the standard input
         * stream, even if it exceeds MAX_USR_MSG.
         */
        if (FD_ISSET(STDIN_FILENO, &read_fds) == 1) {
            int bytes_read;
            // write MAX_USR_MSG bytes into t_buf
            char t_buf[BUF_SIZE];
            if ((bytes_read = read(STDIN_FILENO, &(t_buf[1]), MAX_USER_MSG)) < 0) {
                perror("read");
                close(s.sock_fd);
                exit_status = 1;
                break;
            }
            // check for special case
            if (bytes_read == 1) {
                continue;
            }
            if (bytes_read >= 3 && t_buf[1] == '.' && t_buf[2] == 'k' && t_buf[3] == ' ') {
                // sending kick message to user
                if (t_buf[bytes_read] != '\n') {
                    newline_flag = 1;
                } else {
                    newline_flag = 0;
                }
                t_buf[0] = '0';
                memmove(&t_buf[1], &(t_buf[4]), bytes_read-3);
                t_buf[bytes_read-3] = '\r';
                t_buf[bytes_read-2] = '\n';
                bytes_read--;
            } else if (bytes_read >= 3 && t_buf[1] == '.' && t_buf[2] == 'e' && t_buf[3] == ' ') {
                // sending emote message
                if (t_buf[bytes_read] != '\n') {
                    newline_flag = 1;
                } else {
                    newline_flag = 0;
                }
                // get name of the emote
                char *emote_path = malloc(sizeof(char) * (MAX_USER_MSG+1));
                memset(emote_path, 0, MAX_USER_MSG+1);
                memcpy(emote_path, "./emotes/", 9);
                memmove(&(emote_path[9]), &(t_buf[4]), bytes_read-3);
		        memcpy(&(emote_path[bytes_read+5]), ".jpg", 4);
                // saving stdout file descriptor for later use
                // saved_stdout = dup(STDOUT_FILENO);
                // saved_stderr = dup(STDERR_FILENO);
                // build pipe
                int p[2];
                if (pipe(p) < 0) {
                    perror("pipe");
                    continue;
                }
                int r = fork();
                if (r == 0) {
                    // child process
                    close(p[0]);
                    // direct stdout to pipe
                    if (dup2(p[1], STDOUT_FILENO) < 0) {
                        perror("dup2");
                        exit_status = 1;
                        close(p[1]);
                        break;
                    }
                    if (dup2(p[1], STDERR_FILENO) < 0) {
                        perror("dup2");
                        exit_status = 1;
                        close(p[1]);
                        break;
                    }
                    // execl base64
                    execlp("base64", "base64", "-w 0",emote_path, NULL);
                    perror("execlp");
                    exit_status = 1;
                    break;
                } else {
                    // parent process
                    close(p[1]);
                    wait(&status);
                    free(emote_path);
                    // restore stdout and stderr
                    // dup2(saved_stdout, STDOUT_FILENO);
                    // dup2(saved_stderr, STDERR_FILENO);
                    // close(saved_stdout);
                    if (WEXITSTATUS(status) || WIFSIGNALED(status)) {
                        exit_status = 1;
                        close(p[0]);
                        break;
                    }
                    // reading encoded base64 from child process
                    if ((bytes_read = read(p[0], &(t_buf[1]), MAX_IMG_LEN)) < 0) {
                        perror("read");
                        exit_status = 1;
                        close(p[0]);
                        break;
                    }
                    close(p[0]);
                    if (strncmp(&(t_buf[1]), "base64: ", 8) == 0) {
                        fprintf(stderr, "Error: Emote cannot be found!\n");
                        exit_status = 1;
                        close(p[0]);
                        break;
                    }
                    t_buf[0] = '2';
                    t_buf[bytes_read] = '\r';
                    t_buf[bytes_read+1] = '\n';
                    bytes_read += 2;
                }
            } else {
                newline_flag = 0;
                t_buf[0] = '1';
                t_buf[bytes_read] = '\r';
                t_buf[bytes_read+1] = '\n';
                bytes_read += 2;
            }
            if (!newline_flag) {
                if (write_to_socket(s.sock_fd, t_buf, bytes_read) != 0) {
                    close(s.sock_fd);
                    exit_status = 1;
                    break;
                }
            }
        }

        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
        if (FD_ISSET(s.sock_fd, &read_fds) == 1) {
            // read message sent by server
            int result;
            int space_flag = 0;
            if ((result = read_from_socket(s.sock_fd, &(s.buf[s.inbuf]), &(s.inbuf))) == 1) {
                fprintf(stderr, "Socket is closed\n");
                exit_status = 1;
                break;
            } else if (result == -1) {
                fprintf(stderr, "message size exceeds maximum\n");
                exit_status = 1;
                break;
            } else if (result == 2) {
                fprintf(stderr, "CLRF not recieved\n");
                exit_status = 1;
                break;
            }
            while (s.inbuf > 0) {
                if (get_message(&msg_buf, s.buf, &s.inbuf) == -1) {
                    fprintf(stderr, "Something went wrong in get_message helper\n");
                    exit_status = 1;
                    break;
                }
                if (msg_buf[0] == '1') {
                    for (int i = 1; i < strlen(msg_buf); i++){
                        if (space_flag == 0 && msg_buf[i] == ' ') {
                            space_flag = 1;
                            printf("%c", ':');
                        }
                        printf("%c", msg_buf[i]);
                    }
                    printf("%c", '\n');
                } else {
                    char *emote_base64 = malloc(sizeof(char) * (MAX_IMG_LEN+1));
                    memset(emote_base64, 0, MAX_IMG_LEN+1);
                    for (int i = 0; i < strlen(msg_buf); i++) {
                        if (msg_buf[i] == ' ') {
                            memcpy(emote_base64, &(msg_buf[i+1]), MAX_IMG_LEN+1);
                            break;
                        }
                    }
                    int decode_p[2];
                    if (pipe(decode_p) < 0) {
                        perror("pipe");
                        exit_status = 1;
                        break;
                    }
                    //saved_stdin = dup(STDIN_FILENO);
                    //saved_stdout = dup(STDOUT_FILENO);
                    //saved_stderr = dup(STDERR_FILENO);
                    if (fork() == 0) {
                        // child process
                        // decode the base64 message sent from server
			printf("decode child pid: %d\n", getpid());
                        if (dup2(decode_p[1], STDOUT_FILENO) < 0) {
                            perror("dup2");
                            exit_status = 1;
                            break;
                        }
                        if (dup2(decode_p[1], STDERR_FILENO) < 0) {
                            perror("dup2");
                            exit_status = 1;
                            break;
                        }
                        if (dup2(STDIN_FILENO, decode_p[0]) < 0) {
                            perror("dup2");
                            exit_status =1;
                            break;
                        }
			write(decode_p[1], "a", 1);
                        execlp("base64", "base64", "-d", NULL);
                        perror("base64");
                        exit_status = 1;
                        break;
		    }
		    // restore stdin, stdout, stderr
                    //dup2(saved_stdin, STDIN_FILENO);
                    //dup2(saved_stdout, STDOUT_FILENO);
                    //dup2(saved_stderr, STDERR_FILENO);
		    // if (write(decode_p[1], "base64 -d\n",10) < 0) {
		//	perror("write");
		  //      exit_status = 1;
		//	break;
		  //  }
		    // wait till the child process has finished dupping
		    char *dup_flag = malloc(sizeof(char));
		    read(decode_p[0], dup_flag, 1);
		    free(dup_flag);
		    printf("decode parent pid: %d\n", getpid());
		    strncat(emote_base64, "\n", 1);
                    if (write(decode_p[1], emote_base64, strlen(emote_base64)-1) < 0) {
                        perror("write");
                        exit_status = 1;
                        break;
                    }
                    close(decode_p[1]);
		    free(emote_base64);
		    printf("parent done writing\n");
                    char *emote_data = malloc(sizeof(char) * (MAX_IMG_LEN+1));
                    if ((emote_bytes_read = read(decode_p[0], emote_data, MAX_IMG_LEN+1)) < 0) {
                        perror("read");
                        exit_status = 1;
			close(decode_p[0]);
                        break;
                    }
		    printf("parent done reading\n");
		    close(decode_p[0]);
		    printf("%s", emote_data);
                    if (strncmp(emote_data, "base64: ", 8) == 0) {
                        fprintf(stderr, "Error in reading emote sent\n");
                        exit_status = 1;
                        break;
                    }
                    int p = getpid();
                    char *fifo_name = malloc(sizeof(char) * 21);
                    memset(fifo_name, 0, 21);
                    strncat(fifo_name, "./emotepipe", 11);
                    sprintf(&(fifo_name[11]), "%d", p);
                    strncat(fifo_name, ".jpg", 4);
                    int emote = mkfifo(fifo_name, 0666);
                    for (int i = 0; i < strlen(msg_buf); i++) {
                        if (msg_buf[i] == ' ') {
                            printf("%s", ": ");
                            break;
                        }
                        printf("%c", msg_buf[i]);
                    }
                    if (fork() == 0) {
                        // load jpg using catimg
                        if (write(emote, emote_data, emote_bytes_read) < 0) {
                            perror("write");
                            close(emote);
                            exit_status = 1;
                            break;
                        }
                        execlp("catimg", "catimg", "-w80", fifo_name, NULL);
                        perror("execlp");
                        exit_status = 1;
                        break;
                    }
                    wait(&status);
                    if (WEXITSTATUS(status) || WIFSIGNALED(status)) {
                        exit_status = 1;
                        close(emote);
                        break;
                    }
                    free(fifo_name);
                    close(emote);
                    if (unlink(fifo_name) != 0) {
                        perror("unlink");
                        exit_status = 1;
                        break;
                    }
                }
	        }
	        free(msg_buf);
        }
    }
    
    close(s.sock_fd);
    exit(exit_status);
}
