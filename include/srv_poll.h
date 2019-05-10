// srv_poll.h
#ifndef SRV_POLL_H
#define SRV_POLL_H

#include <netinet/in.h>

/* ---- Macros ---- */
#define SRVLOGFILE "../data/srv_poll_log"
#define ARGSNUM 2
#define ARG_PORT 1
#define BACKLOG 100
#define PKTSIZE 1000        // must be same on client side
#define STRINGSIZE 16
#define ARRSIZE 1000
#define MAXCLIENTS 15000

/* ---- Structures ---- */
struct srv_nw_var           // server network variables
{
    int sd_listen;                  // socket to listen for new connections
    struct sockaddr_in srv_addr;    // addr of server
    int port;                       // port to bind to
};

struct thread_args          // arguments to pass into threaded function
{
    int sd;
    char clt_ip[STRINGSIZE];
};

/* ---- Function Prototypes ---- */
int valid_args(int arg, char *port);
int run_srv(struct srv_nw_var *nw);
int setup_srv(struct srv_nw_var *nw);
int run_poll_loop(struct srv_nw_var nw);
int set_SIGINT();
void *echo_loop(void *args);
void close_fd();

#endif
