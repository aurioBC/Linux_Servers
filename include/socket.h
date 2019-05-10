//socket.h
#include <netinet/in.h>

#ifndef SOCKET_H
#define SOCKET_H

/* ---- Functions Prototypes ---- */
int create_socket(int *sd, int domain, int type, int protocol);
int bind_socket(int sd, const struct sockaddr *addr, socklen_t len);
int listen_socket(int sd, int backlog);
int connect_socket(int sd, const struct sockaddr *addr, socklen_t len);
int set_nonblocking(int *sd);
int set_blocking(int *sd);
void fill_addr(struct sockaddr_in *addr, int domain, unsigned short port, unsigned long ip);

#endif
