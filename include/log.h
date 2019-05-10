//log.h
#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <pthread.h>

/* ---- Macros ---- */
#define STRINGSIZE 16
#define KILO 1000


/* ---- Structures ---- */
struct Bytes            // hold total bytes transferred
{
    float bytes;
    float kilobytes;
    float megabytes;
    float gigabytes;
};

struct srv_log_stats    // hold server logging info
{
    struct tm tm;                   // time of connection
    struct Bytes bytes;             // total amount of data transferred
    char clt_ip[STRINGSIZE];        // host name of client
    int requests;                   // total client requests
    int sd;                         // used to track epoll client sockets
};

struct clt_log_stats    // hold client logging info
{
    struct tm tm;                   // time of connection
    struct Bytes bytes;             // total amount of data transferred
    int requests;                   // total client requests
    float avg_time;                 // average server response time
};

/* ---- Macros ---- */
#define STRINGSIZE 16

/* ---- Function Prototypes ---- */
int app_srv_hdr();
int app_clt_hdr();
int append_srv_data(char *filename, struct srv_log_stats stats);
int append_clt_data(struct clt_log_stats stats, double t);
int append_total_clients(char *filename, int total);
void init_bytes_struct(struct Bytes *data);
void send_pkt(struct Bytes *data);
void update_bytes_struct(struct Bytes *data, int bytes);
void print_bytes_struct(struct Bytes data);

/* --- Variables ---- */
pthread_mutex_t lock;

#endif
