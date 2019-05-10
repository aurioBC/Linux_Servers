//clt_thread.h
#ifndef CLT_THREAD_H
#define CLT_THREAD_H

#include <sys/socket.h>
#include <netinet/in.h>

/* ---- Macros ---- */
#define ARGSNUM 4
#define ARG_IP 1
#define ARG_PORT 2
#define ARG_CLTS 3
#define STRINGSIZE 16
#define PKTSIZE 1000
#define TIMEOUT 20
#define CLTLOGFILE "../data/clt_log"

/* ---- Structures ---- */
struct clt_nw_var                   // client network variables
{
    int sd;                         // socket to connect to host with
    struct sockaddr_in h_addr;      // addr of host to connect to
    in_port_t h_port;               // hosts port
    unsigned long h_ip;             // hosts ip
};

/* ---- Function Prototypes ---- */
int valid_args(int arg, char *port, char *clients);
int connect_to_host(struct clt_nw_var *nw);
int send_loop(struct clt_nw_var nw);
void spawn_clients(char *ip, char *port);
void get_host_info(struct clt_nw_var *nw, char *ip, char *port);
void print_nw_struct(struct clt_nw_var nw);

#endif
