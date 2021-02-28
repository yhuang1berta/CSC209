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

    char *buf = NULL; // Buffer to read name from stdin
    int name_valid = 0;
    while(!name_valid) {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        size_t name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0) {
            perror("getline");
            fprintf(stderr, "Error reading username.\n");
            free(buf);
            exit(1);
        }
        if (name_len - 1 > MAX_NAME) { // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
        }
        else {
            // added check for space in user name
            for (int i = 0; i < name_len; i++) {
                if (buf[i] == ' ') {
                    printf("Username cannot contain any space\n");
                }
            }
            // Replace LF+NULL with CR+LF
            buf[name_len-1] = '\r';
            buf[name_len] = '\n';
            if (write_to_socket(s.sock_fd, buf, name_len+1)) {
                fprintf(stderr, "Error sending username.\n");
                free(buf);
                exit(1);
            }
            name_valid = 1;
            free(buf);
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
	    char t_buf[MAX_USER_MSG+2];
            if ((bytes_read = read(STDIN_FILENO, t_buf, MAX_USER_MSG)) < 0) {
                perror("read");
                close(s.sock_fd);
                exit_status = 1;
                break;
            }
            // check for special case
            if (bytes_read == 1) {
                continue;
            }
	    t_buf[bytes_read] = '\r';
	    t_buf[bytes_read+1] = '\n';
            if (write_to_socket(s.sock_fd, t_buf, bytes_read+2) != 0) {
                close(s.sock_fd);
                exit_status = 1;
                break;
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

            	for (int i = 0; i < strlen(msg_buf); i++){
                	if (space_flag == 0 && msg_buf[i] == ' ') {
                    	space_flag = 1;
                    	printf("%c", ':');
                	}
                	printf("%c", msg_buf[i]);
            	}
            	printf("%c", '\n');
	    }
	    free(msg_buf);
        }
    }
    
    close(s.sock_fd);
    exit(exit_status);
}
