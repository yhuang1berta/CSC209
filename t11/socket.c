#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <netinet/in.h>    /* struct sockaddr_in */

#include "socket.h"

void setup_server_socket(struct listen_sock *s) {
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        perror("malloc");
        exit(1);
    }
    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(SERVER_PORT);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("server socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        perror("server: bind");
        close(s->sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(s->sock_fd);
        exit(1);
    }
}

/* Insert Tutorial 10 helper functions here. */

int find_network_newline(const char *buf, int inbuf) {
    int i;
    for (i = 0; i < inbuf-1; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return i+2;
        }
    }
    return -1;
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    int result;
    if (read(sock_fd, NULL, 0) == -1) {
        // socket is closed
        return 1;
    }
    if ((*inbuf) >= BUF_SIZE) {
        return -1;
    }
    if ((result = read(sock_fd, &(buf[*inbuf]), BUF_SIZE - *inbuf)) == -1) {
        perror("read");
        return -1;
    }
    *inbuf += result;
    if (find_network_newline(buf, *inbuf) != -1) {
        return 0;
    }
    return 2;
}

int get_message(char **dst, char *src, int *inbuf) {
    int index = find_network_newline(src, (*inbuf));
    if (index == -1) {
        return 1;
    }
    *dst = malloc(BUF_SIZE);
    if (memset(*dst, 0, BUF_SIZE) == NULL) {
        perror("memset");
        return 1;
    }
    if (memcpy(*dst, src, index-2) == NULL) {
        perror("memcpy");
        return 1;
    }
    char *src_temp = malloc(BUF_SIZE);
    if (memset(src_temp, 0, BUF_SIZE) == NULL) {
	perror("memset");
	return 1;
    }
    if (memmove(src_temp, &(src[index]), *inbuf - index) == NULL) {
	perror("memmove");
	return 1;
    }
    // moving the remaining message forward
    if (memmove(src, src_temp, BUF_SIZE) == NULL) {
        perror("memmove");
        return 1;
    }
    (*inbuf) -= index;
    free(src_temp);

    return 0;
}

/* Helper function to be completed for Tutorial 11. */

int write_to_socket(int sock_fd, char *buf, int len) {
    int result = write(sock_fd, buf, len);
    // write msg to server
    if (result  == -1) {
	perror("write");
        return 1;
    } else if (result == 0) {
	return 2;
    }
    return 0;
}
