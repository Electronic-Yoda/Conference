#ifndef CONNECTION_H
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "fcntl.h"

/* ================== SHARED ==================== */
void *get_in_addr(struct sockaddr *sa);
int tcp_sendAll(int sockfd, char *buf, int *len);
bool set_blocking_mode(int socket, bool is_blocking);


/* =================== SERVER =======================*/


int tcp_server_listener_start(char* port, int backlog);


/* =================== CLIENT =======================*/


int tcp_client_connect(char* host, char* port);

#endif