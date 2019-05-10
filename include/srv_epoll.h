// srv_epoll.h
#ifndef SRV_EPOLL_H
#define SRV_EPOLL_H

#include <netinet/in.h>

/* ---- Macros ---- */
#define SRVLOGFILE "../data/srv_epoll_log"
#define ARGSNUM 2
#define ARG_PORT 1
#define BACKLOG 100
#define PKTSIZE 1000      // must be same on client side
#define STRINGSIZE 16
#define ARRSIZE 1000
#define MAXEVENTS 50000

/* ---- Structures ---- */
struct srv_nw_var       // server network variables
{
    int sd_listen;                  // socket to listen for new connections
    struct sockaddr_in srv_addr;    // addr of server
    int port;                       // port to bind to
};

/* ---- Function Prototypes ---- */
int valid_args(int arg, char *port);
int run_srv(struct srv_nw_var *nw);
int setup_srv(struct srv_nw_var *nw);
int run_epoll_loop(struct srv_nw_var nw);
int echo(int sd);
int set_SIGINT();
void close_fd();

#endif
