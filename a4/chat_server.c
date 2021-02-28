#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "socket.h"
#include "chat_helpers.h"

int sigint_received = 0;

void sigint_handler(int code) {
    sigint_received = 1;
}

/*
 * Wait for and accept a new connection.
 * Return -1 if the accept call failed.
 */
int accept_connection(int fd, struct client_sock **clients) {
    struct sockaddr_in peer;
    unsigned int peer_len = sizeof(peer);
    peer.sin_family = AF_INET;

    int num_clients = 0;
    struct client_sock *curr = *clients;
    while (curr != NULL && num_clients < MAX_CONNECTIONS && curr->next != NULL) {
        curr = curr->next;
        num_clients++;
    }

    int client_fd = accept(fd, (struct sockaddr *)&peer, &peer_len);
    
    if (client_fd < 0) {
        perror("server: accept");
        close(fd);
        exit(1);
    }

    if (num_clients == MAX_CONNECTIONS) {
        close(client_fd);
        return -1;
    }

    struct client_sock *newclient = malloc(sizeof(struct client_sock));
    newclient->sock_fd = client_fd;
    newclient->inbuf = newclient->state = 0;
    newclient->username = NULL;
    newclient->next = NULL;
    memset(newclient->buf, 0, BUF_SIZE);
    if (*clients == NULL) {
        *clients = newclient;
    }
    else {
        curr->next = newclient;
    }

    return client_fd;
}

/*
 * Close all sockets, free memory, and exit with specified exit status.
 */
void clean_exit(struct listen_sock s, struct client_sock *clients, int exit_status) {
    struct client_sock *tmp;
    while (clients) {
        tmp = clients;
        close(tmp->sock_fd);
        clients = clients->next;
        free(tmp->username);
        free(tmp);
    }
    close(s.sock_fd);
    free(s.addr);
    exit(exit_status);
}

int main(void) {
    // This line causes stdout not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    
    /*
     * Turn off SIGPIPE: write() to a socket that is closed on the other
     * end will return -1 with errno set to EPIPE, instead of generating
     * a SIGPIPE signal that terminates the process.
     */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        exit(1);
    }

    // Linked list of clients
    struct client_sock *clients = NULL;
    
    struct listen_sock s;
    setup_server_socket(&s);
    
    // Set up SIGINT handler
    struct sigaction sa_sigint;
    memset (&sa_sigint, 0, sizeof (sa_sigint));
    sa_sigint.sa_handler = sigint_handler;
    sa_sigint.sa_flags = 0;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);
    
    int exit_status = 0;
    
    int max_fd = s.sock_fd;

    fd_set all_fds, listen_fds;
    
    FD_ZERO(&all_fds);
    FD_SET(s.sock_fd, &all_fds);

    do {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (sigint_received) break;
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("server: select");
            exit_status = 1;
            break;
        }

        /* 
         * If a new client is connecting, create new
         * client_sock struct and add to clients linked list.
         */
        if (FD_ISSET(s.sock_fd, &listen_fds)) {
            int client_fd = accept_connection(s.sock_fd, &clients);
            if (client_fd < 0) {
                printf("Failed to accept incoming connection.\n");
                continue;
            }
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection\n");
        }

        if (sigint_received) break;

        /*
         * Accept incoming messages from clients,
         * and send to all other connected clients.
         */
        struct client_sock *curr = clients;
        while (curr) {
            if (!FD_ISSET(curr->sock_fd, &listen_fds)) {
                curr = curr->next;
                continue;
            }
            int client_closed = read_from_client(curr);
            
            // If error encountered when receiving data
            if (client_closed == -1) {
                perror("read");
                client_closed = 1; // Disconnect the client
            }
            
            // If received at least one complete message
            // and client is newly connected: Get username
            if (client_closed == 0 && curr->username == NULL) {
                if (set_username(curr)) {
                    printf("Error processing user name from client %d.\n", curr->sock_fd);
                    client_closed = 1; // Disconnect the client
                }
                else {
                    // check if username already exists
                    char name_flag = 0;
                    struct client_sock *name_check = clients;
                    if (strncmp(curr->username, "SERVER", MAX_NAME) == 0) {		    
                        name_flag = 1;
                    }
                    while (!name_flag && name_check) {
                        if (name_check != curr && strncmp(name_check->username, curr->username, MAX_NAME) == 0) {
                            name_flag = 1;
                            break;
                        }
			name_check = name_check->next;
                    }
                    if (name_flag) {
                        char *invalid_username = malloc(sizeof(char) * 44);
                        invalid_username[0] = '\0';
                        strncat(invalid_username, "1SERVER Username invalid or already taken.", 42);
                        write_buf_to_client(curr, invalid_username, 42);
			            free(invalid_username);
                        client_closed = 1;
                    } else {
                        printf("Client %d user name is %s.\n", curr->sock_fd, curr->username);
		    }
                }
            }
                
            char *msg;
            // Loop through buffer to get complete message(s)
            while (client_closed == 0 && !get_message(&msg, curr->buf, &(curr->inbuf))) {
                //check for proto msg code
                if (msg[0] == '0' && curr == clients) {
                    // kick command from admin received
                    struct client_sock *kick_sock = clients;
                    while (kick_sock) {
                        if (strncmp(kick_sock->username, &(msg[1]), MAX_NAME) == 0) {
                        curr = kick_sock;
                        client_closed = 1;
                        break;
                    }
		                kick_sock = kick_sock->next;
		            }
                } else if (msg[0] == '1' || msg[0] == '2') {
                    printf("Echoing message from %s.\n", curr->username);
                    char write_buf[BUF_SIZE];
                    if (msg[0] == '1') {
                        // sending text message
                        write_buf[0] = '1';
                    } else {
                        // sending emote message
                        if (strlen(msg)-2 > MAX_IMG_LEN) {
                            client_closed = 1;
                            continue;
                        }
                        write_buf[0] = '2';
                    }
                    write_buf[1] = '\0';
                    strncat(write_buf, curr->username, MAX_NAME);
                    strncat(write_buf, " ", 1);
                    strncat(write_buf, &(msg[1]), MAX_IMG_LEN);
                    free(msg);
                    int data_len = strlen(write_buf);
                    struct client_sock *dest_c = clients;
                    while (dest_c) {
                        if (dest_c != curr) {
                            int ret = write_buf_to_client(dest_c, write_buf, data_len);
                            if (ret == 0) {
                                printf("Sent message from %s (%d) to %s (%d).\n",
                                    curr->username, curr->sock_fd,
                                    dest_c->username, dest_c->sock_fd);
                            }
                            else {
                                printf("Failed to send message to user %s (%d).\n", dest_c->username, dest_c->sock_fd);
                                if (ret == 2) {
                                    printf("User %s (%d) disconnected.\n", dest_c->username, dest_c->sock_fd);
                                    assert(remove_client(&dest_c, &clients) == 0); // If this fails we have a bug
                                    continue;
                                }
                            }
                        }
                        dest_c = dest_c->next;
                    }
                } else {
			        client_closed = 1;
		        }
            }
            
            if (client_closed == 1) { // Client disconnected
                // Note: Never reduces max_fd when client disconnects
                FD_CLR(curr->sock_fd, &all_fds);
                close(curr->sock_fd);
                printf("Client %d disconnected\n", curr->sock_fd);
                assert(remove_client(&curr, &clients) == 0); // If this fails we have a bug
            }
            else {
                curr = curr->next;
            }
        }
    } while(!sigint_received);
    
    clean_exit(s, clients, exit_status);
}
