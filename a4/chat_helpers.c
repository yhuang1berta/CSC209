#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"
#include "chat_helpers.h"

int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    // To be completed.    
    if (len + 1 > MAX_PROTO_MSG) {
        return 1;
    }
    buf[len] = '\r';
    buf[len+1] = '\n';
    return write_to_socket(c->sock_fd, buf, len+2);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
    struct client_sock *cur_node = *clients;
    // check if head is node we wish to remove
    if (cur_node == *curr) {
	free(((*curr)->username)-sizeof(char));
	struct client_sock *temp = *clients;
        *clients = (*clients)->next;
	*curr = *clients;
	close(temp->sock_fd);
        return 0;
    }
    while (cur_node != NULL) {
    	if (cur_node->next == *curr) {
		free(((*curr)->username)-sizeof(char));
		close((*curr)->sock_fd);
        	cur_node->next = (*curr)->next;
		*curr = (*curr)->next;
        	return 0;
	}
	cur_node = cur_node->next;
    }
    return 1; // Couldn't find the client in the list, or empty list
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

int set_username(struct client_sock *curr) {
    // To be completed. Hint: Use get_message().
    char *msg = malloc(BUF_SIZE);
    int msg_len = MAX_NAME;
    if (get_message(&msg, curr->buf, &msg_len) == 1) {
        return 1;
    }
    if (msg[0] == '1') {
        msg++;
    }
    if (msg_len > MAX_NAME) {
	return 1;
    }
    for (int i = 0; i < strlen(msg)-1; i++) {
        if (msg[i] == ' ') {
            return 1;
        }
    }
    msg[strlen(msg)-1] = '\0';
    // copy name to curr
    curr->username = msg;
    // update inbuf
    curr->inbuf = 0;
    return 0;

}
