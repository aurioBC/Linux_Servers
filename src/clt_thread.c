/*------------------------------------------------------------------------------
|   SOURCE:     clt_thread.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module that represents the client program. The program takes
|               3 additional cmd arguments:
|                   - host IP
|                   - host port
|                   - number of clients/threads to create
|
|                   Usage: ./clt <HOST IP> <PORT> <NUM OF CLIENTS>
|
|               The user must specify the number of clients the program will
|               create. The program will then create a seperate thread for each
|               client in order to simulate multiple client connections to the
|               server.
------------------------------------------------------------------------------*/
#include "../include/clt_thread.h"
#include "../include/socket.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <pthread.h>

/*==============================================================================
|   FUNCTION:   int main(int argc, char **argv)
|                   argc   : number of cmd args
|                   **argv : array of args
|
|   RETURN:     0 on success
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Main entry point of the program.
==============================================================================*/
int main(int argc, char **argv)
{
    if(!valid_args(argc, argv[ARG_PORT], argv[ARG_CLTS]))  // check for valid args
        exit(1);

    if(app_clt_hdr() == -1)  // append header to client log file
        exit(1);

    struct clt_nw_var nw_var;
    get_host_info(&nw_var, argv[ARG_IP], argv[ARG_PORT]);

    int num_of_clts = atoi(argv[ARG_CLTS]); // get number of client to create
    omp_set_num_threads(num_of_clts);

    #pragma omp parallel
    {
        spawn_clients(argv[ARG_IP], argv[ARG_PORT]);
    }

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int valid_args(int arg, char *port, char *clients)
|                   arg   : number of cmd args
|                   *port : port arg
|                   *clients : number of clients arg
|
|   RETURN:     1 on true, 0 on false
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Checks for valid arguments (valid number of args, port and
|               number of clients). Returns true (1) if args are valid,
|               otherwise returns false (0).
------------------------------------------------------------------------------*/
int valid_args(int arg, char *port, char *clients)
{
    // check valid number of args (4)
    if(arg != ARGSNUM)
    {
        printf("\nUsage: ./clt <HOST IP> <PORT> <NUM OF CLIENTS>\n\n");
        return 0;
    }

    // check for valid port
    for(int i = 0; port[i] != '\0'; i++)
    {
        if(!isdigit(port[i]))
        {
            printf("\nError: Invalid port: %s.\n\n", port);
            return 0;
        }
    }

    // check for valid num of clients
    for(int i = 0; clients[i] != '\0'; i++)
    {
        if(!isdigit(clients[i]))
        {
            printf("\nError: Invalid number of clients: %s.\n\n", port);
            return 0;
        }
    }

    return 1;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int connect_to_host(struct clt_nw_var *nw)
|                   *nw : pointer to clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function that connects the client program to a host
|               machine. Uses variables held in '*nw' to create a socket and
|               connect the socket to a host machine.
------------------------------------------------------------------------------*/
int connect_to_host(struct clt_nw_var *nw)
{
    if(create_socket(&(nw->sd), AF_INET, SOCK_STREAM, 0) == -1)
        return -1;

    bzero((char *)&(nw->h_addr), sizeof(struct sockaddr_in));
    fill_addr(&(nw->h_addr), AF_INET, nw->h_port, nw->h_ip);

    if(connect_socket(nw->sd, (struct sockaddr *)&(nw->h_addr), sizeof(nw->h_addr)) == -1)
        return -1;

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int send_loop(struct clt_nw_var nw)
|                   nw : clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Function to initiate send loop. Clients keeps sending a packet
|               of PKTSIZE and reading the echo from the server until TIMEOUT
|               has occured.
------------------------------------------------------------------------------*/
int send_loop(struct clt_nw_var nw)
{
    struct timeval _tt1;
    struct timeval _tt2;
    struct clt_log_stats _stats;
    char _send_buff[PKTSIZE];
    char _recv_buff[PKTSIZE];
    double _elapsed_time;
    double _avg_time = 0;
    int _bytes_recv;
    int _bytes_sent;
    time_t _t = time(NULL);
    time_t _t1;
    time_t _t2;

    memset(_send_buff, 'A', PKTSIZE);
    time(&_t1);                      // get current time (for timeout)
    _stats.tm = *localtime(&_t);     // time of new connection
    _stats.requests = 0;
    init_bytes_struct(&(_stats.bytes));

    // send loop (unitl timeout)
    while(1)
    {
        gettimeofday(&_tt1, NULL); // start timer

        // send oacket
        if ((_bytes_sent = send(nw.sd, _send_buff, PKTSIZE, 0)) == -1)
        {
            printf("\tError sending\n");
      	    printf("\tError code: %s\n\n", strerror(errno));
            return -1;
        }

        _stats.requests++; // update client requests

        // read echo
        if((_bytes_recv = recv(nw.sd, _recv_buff, PKTSIZE, MSG_WAITALL)) == -1)
        {
            printf("\tClient %d error reading\n", omp_get_thread_num());
      	    printf("\tError code: %s\n\n", strerror(errno));
            return -1;
        }
        else if(_bytes_recv == 0) // server shutdown
        {
            printf("\nServer shutdown\n\n");
            break;
        }
        else // success
        {
            update_bytes_struct(&_stats.bytes, _bytes_recv);
            bzero(_recv_buff, sizeof(_recv_buff));
        }

        gettimeofday(&_tt2, NULL); // stop timer
        _elapsed_time = (_tt2.tv_sec - _tt1.tv_sec) * 1000.0;
        _elapsed_time += (_tt2.tv_usec - _tt1.tv_usec) / 1000.0; // in milliseconds
        _avg_time += _elapsed_time;

        // check for timeout
        time(&_t2);
        if(difftime(_t2, _t1) > TIMEOUT)
            break;
    }

    _avg_time = _avg_time / _stats.requests;
    printf("- Client %d: Disconnecting\n", omp_get_thread_num());
    close(nw.sd);
    append_clt_data(_stats, _avg_time);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void spawn_clients(char *ip, char *port)
|                   *ip : cmd arg that holds the servers IP
|                   *port : cmd arg that holds teh servers listening port
|
|   RETURN:     void
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       high level function that is called by openmp. connects to a host
|               specified by '*ip' and '*port'. Once connected it calls the
|               send_loop function in order to initiate data transfer.
------------------------------------------------------------------------------*/
void spawn_clients(char *ip, char *port)
{
    struct clt_nw_var _nw;
    get_host_info(&_nw, ip, port);

    if(connect_to_host(&_nw) == -1)
        return;

    if(send_loop(_nw) == -1)
        return;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void get_host_info(struct clt_nw_var *nw, char *ip, char *port)
|                   *nw : pointer to clients network variables
|                   *ip : cmd arg that hold the servers IP
|                   *port : cmd arg that holds the servers listening port
|
|   RETURN:     void
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Copies the cmd args holding the servers ip and listening port
|               ('*ip' and '*port' respectivly) into the appropriate fields
|               of the clients network data struct pointed to by '*nw'.
------------------------------------------------------------------------------*/
void get_host_info(struct clt_nw_var *nw, char *ip, char *port)
{
    nw->h_ip = inet_addr(ip);
    nw->h_port = htons(atoi(port));
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void print_nw_struct(struct clt_nw_var nw)
|                   nw : client network variables struct to print
|
|   RETURN:     void
|
|   DATE:       Feb 13, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Funcion used for testing. Simply prints the contents of 'nw'
------------------------------------------------------------------------------*/
void print_nw_struct(struct clt_nw_var nw)
{
    printf("\nsock: %d\nport: %d\nip: %lu\n\n", nw.sd, nw.h_port, nw.h_ip);
}
