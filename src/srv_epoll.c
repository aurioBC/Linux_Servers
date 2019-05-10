/*------------------------------------------------------------------------------
|   SOURCE:     srv_epoll.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module that represents an asynchronous edge triggered server
|               program. The program takes in 1 additional cmd argument:
|                   - host port
|
|                             Usage: ./clt <PORT>
|
|               The program will then listen on PORT for any incoming
|               connections. Once a connection has been accepted it will be
|               added to the epoll event array where it will be monitored for
|               events.
------------------------------------------------------------------------------*/
#include "../include/srv_epoll.h"
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
#include <sys/epoll.h>
#include <ctype.h>

/* --- Global ---- */
struct srv_nw_var nw_var;

/*==============================================================================
|   FUNCTION:   int main(int argc, char **argv)
|                   argc   : number of cmd args
|                   **argv : array of args
|
|   RETURN:     0 on success
|
|   DATE:       Feb 24, 2018
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
        printf("\nUsage: ./srv_epoll <PORT>\n\n");
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
|   DATE:       Feb 24, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function to run the server. Sets up the server and
|               the SIGINT interupt handler and then runs the epoll loop that
|               listens for incoming connections and monitors socket events.
------------------------------------------------------------------------------*/
int run_srv(struct srv_nw_var *nw)
{
    if(setup_srv(nw) == -1)
        return -1;

    if(set_SIGINT() == -1)
        return -1;

    if(run_epoll_loop(*nw) == -1)
        return -1;

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int setup_srv(struct srv_nw_var *nw)
|                   *nw : pointer to clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 22, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function to setup the server. Uses the networking
|               related variables held in '*nw' to:
|                   - create a socket
|                   - set socket option to reuse address
|                   - set socket to non-blocking
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

    if(set_nonblocking(&(nw->sd_listen)) == -1)
        return -1;

    if(bind_socket(nw->sd_listen, (struct sockaddr *)&(nw->srv_addr), sizeof(nw->srv_addr)) == -1)
        return -1;

    if(listen_socket(nw->sd_listen, BACKLOG) == -1)
        return -1;

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int run_epoll_loop(struct srv_nw_var nw)
|                   nw : clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 24, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Function that runs the epoll loop. Incoming connections are
|               accepted and the new socket is added to the epoll event array.
|               epoll then monitors the array for any socket events and
|               accomodates those events accordingly (echos back data);
------------------------------------------------------------------------------*/
int run_epoll_loop(struct srv_nw_var nw)
{
    struct epoll_event _event;
    struct epoll_event _events[MAXEVENTS];
    struct srv_log_stats _stats[MAXEVENTS];
    char _recv_buff[PKTSIZE];
    int _esd, _ready, _bytes_recv, _bytes_sent, _total_clts = 0;
    int _timeout = (0.1 * 60 * 1000); // set timeout to 15 sec

    for(int i = 0; i < MAXEVENTS; i++) // init stats array
        _stats[i].sd = -1;

    // create epoll socket descriptor
    if((_esd = epoll_create(MAXEVENTS)) == -1)
    {
        printf("\tError creating epoll file descriptor\n");
        printf("\tError code: %s\n\n", strerror(errno));
        return -1;
    }

    // set event of interest and edge trigger on epoll instance _esd
    _event.data.fd = nw.sd_listen;
    _event.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
    if((epoll_ctl(_esd, EPOLL_CTL_ADD, nw.sd_listen, &_event)) == -1)
    {
        printf("\tError adding server sock to epoll event loop\n");
        printf("\tError code: %s\n\n", strerror(errno));
        close(_esd);
        return -1;
    }

    // epoll loop
    while(1)
    {
        // wait for event
        _ready = epoll_wait(_esd, _events, MAXEVENTS, _timeout);
        if(_ready == -1) // error
        {
            printf("\tEPoll Failed\n");
            printf("\tError code: %s\n\n", strerror(errno));
            close(nw.sd_listen);
            close(_esd);
            return -1;
        }

        if(_ready == 0)  // timeout
        {
            printf("\n- Timeout....Terminating\n");
            close(nw.sd_listen);
            close(_esd);
            append_total_clients(SRVLOGFILE, _total_clts);
            return -1;
        }

        // process events
        for(int i = 0; i < _ready; i++)
        {
            if(_events[i].events & (EPOLLHUP & EPOLLERR)) // error on socket
            {
                printf("\tError: epoll EPOLLERR\n");
                close(_events[i].data.fd);
            }
            else if(_events[i].data.fd == nw.sd_listen) // connection request
            {
                while(1)
                {
                    struct sockaddr_in _clt_addr;
                    socklen_t _clt_addr_len;
                    int _new_sd;

                    _clt_addr_len = sizeof(_clt_addr);

                    // accept connection
                    if((_new_sd = accept(nw.sd_listen, (struct sockaddr *)&_clt_addr, &_clt_addr_len)) == -1)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break; // done processing incoming connections
                        }

                        printf("\tError accepting connection\n");
                        printf("\tError code: %s\n\n", strerror(errno));
                        close(nw.sd_listen);
                        break;
                    }

                    if(set_nonblocking(&_new_sd) == -1)
                        return -1;

                    // add new socket to epoll loop
                    _event.data.fd = _new_sd;
                    _event.events = EPOLLIN | EPOLLET;
                    if((epoll_ctl(_esd, EPOLL_CTL_ADD, _new_sd, &_event)) == -1)
                    {
                        printf("\tError adding server sock to epoll event loop\n");
                        printf("\tError code: %s\n\n", strerror(errno));
                        close(_esd);
                        return -1;
                    }

                    _total_clts++;
                    printf("- Client connected: %s\n",  inet_ntoa(_clt_addr.sin_addr));

                    for(int j = 0; j < MAXEVENTS; j++) // save new client to stats array
                        if(_stats[j].sd == -1)
                        {
                            _stats[j].sd = _new_sd;
                            time_t t = time(NULL);
                            _stats[j].tm = *localtime(&t); // time of new connection
                            _stats[j].requests = 0;
                            strcpy(_stats[j].clt_ip, inet_ntoa(_clt_addr.sin_addr));
                            break;
                        }
                }
            }
            else // data ready to be read
            {
                // read socket
                _bytes_recv = recv(_events[i].data.fd, _recv_buff, PKTSIZE, MSG_WAITALL);
                if(_bytes_recv == 0) // client disconnected
                {
                    for(int j = 0; j < MAXEVENTS; j++) // save client stats
                        if(_stats[j].sd == _events[j].data.fd)
                        {
                            _stats[j].sd = -1;
                            printf("- Client disconnected: %s\n", _stats[i].clt_ip);
                            break;
                        }

                    close (_events[i].data.fd);
                    append_srv_data(SRVLOGFILE, _stats[i]); // write to log file
                }
                else    // successfully read socket
                {
                    int j;
                    for(j = 0; j < MAXEVENTS; j++) // save new client to stats array
                        if(_stats[j].sd == _events[i].data.fd)
                        {
                            _stats[j].requests++;   // update client requests
                            break;
                        }

                    // echo data back
                    _bytes_sent = send(_events[i].data.fd, _recv_buff, PKTSIZE, 0);
                    update_bytes_struct(&(_stats[j].bytes), _bytes_sent);
                    bzero(_recv_buff, sizeof(_recv_buff));
                }
            }
        }
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
|   FUNCTION:   void close_fd()
|
|   RETURN:     void
|
|   DATE:       Feb 20, 2018
|
|   AUTHOR:     Aman Abdulla
|
|   DESC:       Function to execute when SIGINT signal is encountered.
|               Terminates server's listening socket
------------------------------------------------------------------------------*/
void close_fd()
{
    printf("\n\n- Terminating\n");
    close(nw_var.sd_listen);
}
