/*------------------------------------------------------------------------------
|   SOURCE:     srv_thread.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module that represents the traditional multi-threaded server
|               program. The program takes in 1 additional cmd argument:
|                   - host port
|
|                             Usage: ./clt <PORT>
|
|               The program will then listen on PORT for any incoming
|               connections. Once a connection has been accepted then the server
|               will create a new thread in order to accomodate that new
|               connection. As a result each new connection will have its own
|               thread.
------------------------------------------------------------------------------*/
#include "../include/srv_thread.h"
#include "../include/socket.h"
#include "../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>

/* --- Global ---- */
struct srv_nw_var nw_var;
int total_clts = 0;

/*==============================================================================
|   FUNCTION:   int main(int argc, char **argv)
|                   argc   : number of cmd args
|                   **argv : array of args
|
|   RETURN:     0 on success
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Main entry point of the program.
==============================================================================*/
int main(int argc, char **argv)
{
    if(!valid_args(argc, argv[ARG_PORT]))   // check for valid args
        exit(1);

    nw_var.port = atoi(argv[ARG_PORT]);     // extract port from cmd arg

    if(app_srv_hdr(SRVLOGFILE) == -1)       // append header to server log file
        exit(1);

    if(run_srv(&nw_var) == -1)
        exit(1);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int valid_args(int arg, char *port)
|                   arg   : number of cmd ARGSNUM
|                   *port : port argument
|
|   RETURN:     1 on true, 0 on false
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Checks for valid arguments (valid num of arg and valid port).
|               Returns true (1) if args are valid, otherwise returns false (0).
------------------------------------------------------------------------------*/
int valid_args(int arg, char *port)
{
    // check valid number of args (2)
    if(arg != ARGSNUM)
    {
        printf("\nUsage: ./srv_thread <PORT>\n\n");
        return 0;
    }

    // check for valid port
    for(int i = 0; port[i] != '\0'; i++)
        if(!isdigit(port[i]))
        {
            printf("\nError: Invalid port: %s.\n\n", port);
            return 0;
        }

    return 1;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int run_srv(struct srv_nw_var *nw)
|                   *nw : pointer to clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 15, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function to run the server. Sets up the server and
|               the SIGINT interupt handler and then runs the accept loop that
|               listens for incoming connections.
------------------------------------------------------------------------------*/
int run_srv(struct srv_nw_var *nw)
{
    if(setup_srv(nw) == -1)
        return -1;

    if(set_SIGINT() == -1)
        return -1;

    if(run_accept_loop(*nw) == -1)
        return -1;

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int setup_srv(struct srv_nw_var *nw)
|                   *nw : pointer to clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 15, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function to setup the server. Uses the networking
|               related variables held in '*nw' to:
|                   - create a socket
|                   - set socket option to reuse address
|                   - bind socket
|                   - set socket to listen
------------------------------------------------------------------------------*/
int setup_srv(struct srv_nw_var *nw)
{
    int _optval = 1;

    if(create_socket(&(nw->sd_listen), AF_INET, SOCK_STREAM, 0) == -1)
        return -1;

    bzero((char *)&(nw->srv_addr), sizeof(struct sockaddr_in));
    fill_addr(&(nw->srv_addr), AF_INET, htons(nw->port), htonl(INADDR_ANY));

    setsockopt(nw->sd_listen, SOL_SOCKET, SO_REUSEADDR, &_optval, sizeof(_optval));

    if(bind_socket(nw->sd_listen, (struct sockaddr *)&(nw->srv_addr), sizeof(nw->srv_addr)) == -1)
        return -1;

    if(listen_socket(nw->sd_listen, BACKLOG) == -1)
        return -1;

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int run_accept_loop(struct srv_nw_var nw)
|                   nw : clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 15, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Function that accepts client connections. Once a connection
|               has been established the function will then create a new thread
|               in order to accomodate the new connection.
------------------------------------------------------------------------------*/
int run_accept_loop(struct srv_nw_var nw)
{
    int _sd;
    struct sockaddr_in _clt_addr;
    struct thread_args *_args;
    socklen_t _clt_addr_len;
    pthread_t _threads[THREADS];

    _clt_addr_len = sizeof(_clt_addr);

    // loop on accept
    for(int i = 0; i < THREADS; i++)
    {
        bzero((char *)&(_clt_addr), sizeof(struct sockaddr_in));
        if((_sd = accept(nw.sd_listen, (struct sockaddr *)&_clt_addr, &_clt_addr_len)) == -1)
        {
            printf("\tError accepting connection\n");
            printf("\tError code: %s\n\n", strerror(errno));
            close(nw.sd_listen);
            return -1;
        }

        total_clts++;

        // setup thread args
        _args = (struct thread_args *)malloc(sizeof *_args);
        _args->sd = _sd;
        strcpy(_args->clt_ip, inet_ntoa(_clt_addr.sin_addr));

        // accomodate client connection in seperate thread
        if(pthread_create(&_threads[i], NULL, echo_loop, _args) != 0)
        {
            printf("\n\tError creating thread\n");
            printf("\tError code: %s\n\n", strerror(errno));
            close(nw.sd_listen);
            return -1;
        }
        if(pthread_detach(_threads[i]) != 0)
        {
            printf("\n\tError joining thread\n");
            printf("\tError code: %s\n\n", strerror(errno));
            close(nw.sd_listen);
            return -1;
        }

        printf("- Client connected: %s\n",  inet_ntoa(_clt_addr.sin_addr));
    }

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int set_SIGINT()
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 20, 2018
|
|   AUTHOR:     Aman Abulla
|
|   DESC:       Function to set up SIGINT interupt handler
------------------------------------------------------------------------------*/
int set_SIGINT()
{
    struct sigaction act;
    act.sa_handler = close_fd;
    act.sa_flags = 0;

    if ((sigemptyset (&act.sa_mask) == -1 || sigaction (SIGINT, &act, NULL) == -1))
    {
            printf("\n\tFailed to set SIGINT handler\n");
            return -1;
    }

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void *echo_loop(void *sd)
|
|   RETURN:     void
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinsii
|
|   DESC:       Function that is passed to a thread. This function accomadates
|               a new connection. It reads data coming from client and echos it
|               back. The function terminates once the client has finished
|               sending data and has disconnected.
------------------------------------------------------------------------------*/
void *echo_loop(void *args)
{
    struct thread_args *_args = (struct thread_args *)args; // get function args
    struct srv_log_stats _stats;
    char _recv_buff[PKTSIZE];
    int _bytes_recv;
    int _bytes_sent;
    time_t t = time(NULL);

    // setup stats struct
    _stats.requests = 0;
    _stats.tm = *localtime(&t);     // time of new connection
    strcpy(_stats.clt_ip, _args->clt_ip);
    init_bytes_struct(&(_stats.bytes));

    // echo loop
    while(1)
    {
        // read socket
        _bytes_recv = recv(_args->sd, _recv_buff, PKTSIZE, MSG_WAITALL);
        if(_bytes_recv == -1)
        {
            printf("\tError reading\n");
            printf("\tError code: %s\n\n", strerror(errno));
            break;
        }
        if(_bytes_recv == 0) // client disconnected
        {
            printf("- Client disconnected: %s\n", _args->clt_ip);
            close(_args->sd);
            break;
        }
        else    // successfully read socket
        {
            _stats.requests++;   // update client requests

            // write to socket (echo)
            _bytes_sent = send(_args->sd, _recv_buff, PKTSIZE, 0);
            if(_bytes_sent == -1)
            {
                printf("\tError sending\n");
                printf("\tError code: %s\n\n", strerror(errno));
                printf("\nsent %d bytes\n", _bytes_sent);
                break;
            }
            update_bytes_struct(&_stats.bytes, _bytes_sent);
            bzero(_recv_buff, sizeof(_recv_buff));
        }
    }

    append_srv_data(SRVLOGFILE, _stats);    // write to log file

    free(_args);
    close(_args->sd);
    pthread_exit(NULL);
    return NULL;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void close_fd()
|
|   RETURN:     void
|
|   DATE:       Feb 20, 2018
|
|   AUTHOR:     Aman Abdulla, Alex Zielinski
|
|   DESC:       Function to execute when SIGINT signal is encountered.
|               Terminates server's listening socket
------------------------------------------------------------------------------*/
void close_fd()
{
    printf("\n\n- Terminating\n");
    close(nw_var.sd_listen);

    append_total_clients(SRVLOGFILE, total_clts);
}
