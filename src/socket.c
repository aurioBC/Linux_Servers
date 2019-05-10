/*------------------------------------------------------------------------------
|   SOURCE:     socket.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module that provides socket wrapper function calls
------------------------------------------------------------------------------*/
#include "../include/socket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <omp.h>


/*------------------------------------------------------------------------------
|   FUNCTION:   int create_socket(int *sd, int domain, int type, int protocol)
|                   *sd : pointer to socket descriptor
|                   domain : communication domain
|                   type : communication semantics
|                   protocol : protocol to use
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Sept 20, 2017
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to create socket
------------------------------------------------------------------------------*/
int create_socket(int *sd, int domain, int type, int protocol)
{
    if((*sd = socket(domain, type, protocol)) == -1)
    {
	     printf("\tError creating socket\n");
   	     printf("\tError code: %s\n\n", strerror(errno));
         return -1;
    }

    printf("- Socket created\n");
	printf("- Socket Num: %i\n", *sd );
    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int bind_socket(int sd, const struct sockaddr *addr, socklen_t len)
|                   sd    : socket to bind to
|                   *addr : address info that binds to socket
|                   len   : length of addr struct
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Sept 20, 2017
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to bind socket
------------------------------------------------------------------------------*/
int bind_socket(int sd, const struct sockaddr *addr, socklen_t len)
{
    if(bind(sd, addr, len) == -1)
    {
        printf("\tError binding socket\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    printf("- Socket bound\n");
    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int listen_socket(int sd, int backlog)
|                   sd : socket to set to listen
|                   backlog : backlog incase of simultaneous connections
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Sept 20, 2017
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to set socket to listen
------------------------------------------------------------------------------*/
int listen_socket(int sd, int backlog)
{
    if(listen(sd, backlog) == -1)
    {
        printf("\tError setting socket to listen\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    printf("- Socket listening...\n\n");
    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int connect_socket(int sd, const struct sockaddr *addr, socklen_t len)
|                   sd  : socket descriptor to use for connection
|                   *addr : addr to connect to
|                   len   : size of addr
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Sept 20, 2017
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to connect socket to server
------------------------------------------------------------------------------*/
int connect_socket(int sd, const struct sockaddr *addr, socklen_t len)
{
    if(connect(sd, addr, len) != 0)
    {
        printf("\tError connecting to host\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    printf("- Client %d: Connected to host\n", omp_get_thread_num());
    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int set_nonblocking(int *sd)
|                   *sd : pointer to the socket to make non-blocking
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 12, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to make socket non-blocking
------------------------------------------------------------------------------*/
int set_nonblocking(int *sd)
{
    int _flags = fcntl(*sd, F_GETFL, 0);

    if(_flags == -1)
    {
        printf("\tError getting socket flags\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    _flags |= O_NONBLOCK;

    if(fcntl(*sd, F_SETFL, _flags) == -1)
    {
        printf("\tError setting non-blocking\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    printf("- Successfully set socket to non-blocking\n");

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int set_blocking(int *sd)
|                   *sd : pointer to the socket to make blocking
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 12, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Wrapper function to make socket non-blocking
------------------------------------------------------------------------------*/
int set_blocking(int *sd)
{
    int _flags = fcntl(*sd, F_GETFL, 0);

    if(_flags == -1)
    {
        printf("\tError getting socket flags\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    _flags &= ~O_NONBLOCK;

    if(fcntl(*sd, F_SETFL, _flags) == -1)
    {
        printf("\tError setting non-blocking\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    printf("- Successfully set socket to blocking\n");

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void fill_addr(struct sockaddr_in *addr, int domain, unsigned short port, unsigned long ip)
|                   *addr  : addr struct to fill in
|                   domain : COMM domain
|                   port   : port
|                   ip     : ip address
|
|   RETURN:     void
|
|   DATE:       Sept 20, 2017
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Fills out sockaddr_in struct
------------------------------------------------------------------------------*/
void fill_addr(struct sockaddr_in *addr, int domain, unsigned short port, unsigned long ip)
{
    addr->sin_family = domain;
    addr->sin_port = port;
    addr->sin_addr.s_addr = ip;
}
