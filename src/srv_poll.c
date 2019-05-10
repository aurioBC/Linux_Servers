/*------------------------------------------------------------------------------
|   SOURCE:     srv_poll.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module that represents a multiplexed level triggered server
|               program. The program takes in 1 additional cmd argument:
|                   - host port
|
|                             Usage: ./clt <PORT>
|
|               The program will then listen on PORT for any incoming
|               connections. Once a connection has been accepted it will be
|               added to the poll array where it will be monitored for events.
------------------------------------------------------------------------------*/
#include "../include/srv_poll.h"
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
#include <sys/poll.h>
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
|   DATE:       Feb 22, 2018
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
        printf("\nUsage: ./srv_poll <PORT>\n\n");
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
|   DATE:       Feb 22, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       High level function to run the server. Sets up the server and
|               the SIGINT interupt handler and then runs the poll loop that
|               listens for incoming connections and monitors socket events.
------------------------------------------------------------------------------*/
int run_srv(struct srv_nw_var *nw)
{
    if(setup_srv(nw) == -1)
        return -1;

    if(set_SIGINT() == -1)
        return -1;

    if(run_poll_loop(*nw) == -1)
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
|   FUNCTION:   int run_poll_loop(struct srv_nw_var nw)
|                   nw : clients network variables
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 22, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Function that runs the poll loop. Incoming connections are
|               accepted and the new socket is added to the poll array. poll
|               then monitors the array for any socket events and accomodates
|               those events accordingly (echos back data);
------------------------------------------------------------------------------*/
int run_poll_loop(struct srv_nw_var nw)
{
    struct pollfd _clts[MAXCLIENTS];
    struct sockaddr_in _clt_addr;
    struct srv_log_stats _stats[MAXCLIENTS];
    socklen_t _clt_addr_len;
    char _recv_buff[PKTSIZE];
    int _bytes_recv, _bytes_sent, _sd, _ready, _size, _total_clts = 0;
    int _timeout = (0.1 * 60 * 1000); // set timeout to 10 sec

    // set listening socket
    _clts[0].fd = nw.sd_listen;
    _clts[0].events = POLLIN;

    // indicate available space
    for(int i = 1; i < MAXCLIENTS; i++)
        _clts[i].fd = -1;
    _size = 1; // increase array size

    bzero((char *)&(_stats), sizeof(struct srv_log_stats));
    _clt_addr_len = sizeof(_clt_addr);

    // poll loop
    while(1)
    {
        // wait for event
        _ready = poll(_clts, _size, _timeout);
        if(_ready == -1) // error
        {
            printf("\tPoll Failed\n");
            printf("\tError code: %s\n\n", strerror(errno));
            close(nw.sd_listen);
            return -1;
        }

        if(_ready == 0)  // timeout
        {
            printf("\n- Timeout....Terminating\n");
            close(nw.sd_listen);
            append_total_clients(SRVLOGFILE, _total_clts);
            return -1;
        }

        if(_clts[0].revents == POLLIN)  // connection request
        {
            if((_sd = accept(nw.sd_listen, (struct sockaddr *)&_clt_addr, &_clt_addr_len)) == -1)
            {
                printf("\tError accepting connection\n");
                printf("\tError code: %s\n\n", strerror(errno));
                close(nw.sd_listen);
                return -1;
            }

            for(int i = 1; i < MAXCLIENTS; i++)  // save new socket descriptor
                if(_clts[i].fd == -1)
                {
                    _clts[i].fd = _sd;
                    _clts[i].events = POLLIN;
                    strcpy(_stats[i].clt_ip, inet_ntoa(_clt_addr.sin_addr));
                    _stats[i].requests = 0;
                    time_t t = time(NULL);
                    _stats[i].tm = *localtime(&t); // time of new connection
                    _total_clts++;
                    printf("- Client connected: %s\n",  _stats[i].clt_ip);
                    break;
                }
            _size++;
            _ready--;
        }

        if(_ready > 0)  // check for more events
        {
            for(int i = 1; i < MAXCLIENTS; i++)
            {
                if(_clts[i].fd != -1)
                {
                    if(_clts[i].revents == POLLIN)  // data ready to read
                    {
                        // read socket
                        _bytes_recv = recv(_clts[i].fd, _recv_buff, PKTSIZE, MSG_WAITALL);
                        if(_bytes_recv == 0) // client disconnected
                        {
                            printf("- Client disconnected: %s\n", _stats[i].clt_ip);
                            close(_clts[i].fd);
                            _clts[i].fd = -1;
                            append_srv_data(SRVLOGFILE, _stats[i]); // write to log file
                        }
                        else    // successfully read socket
                        {
                            _stats[i].requests++;   // update client requests

                            // echo data back
                            _bytes_sent = send(_clts[i].fd, _recv_buff, PKTSIZE, 0);
                            update_bytes_struct(&(_stats[i].bytes), _bytes_sent);
                            bzero(_recv_buff, sizeof(_recv_buff));
                        }
                        _ready--;
                    }

                    if(_ready <= 0) // no more events
                        break;
                }
            }
        }
    }

    close(nw.sd_listen);
    close(_sd);
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
