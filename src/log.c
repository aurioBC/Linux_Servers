/*------------------------------------------------------------------------------
|   SOURCE:     log.c
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Module for logging data into a log file.
------------------------------------------------------------------------------*/
#include "../include/log.h"
#include "../include/clt_thread.h"
#include <stdio.h>
#include <unistd.h>


/*------------------------------------------------------------------------------
|   FUNCTION:   int app_srv_hdr(char *filename)
|                   *filename : name of file to write to
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Appends a table header to the server log file specified by
|               '*filename'.
------------------------------------------------------------------------------*/
int app_srv_hdr(char *filename)
{
    FILE *_log;

    if((_log = fopen(filename, "a")) == NULL)
    {
        printf("\n\tFailed to open server's log file\n\n");
        return -1;
    }

    truncate(filename, 0);
    fprintf(_log, "CONNECTION TIME \t\tHOSTNAME\t\tREQUESTS\t\tBYTES TRANSFERRED\n");
    fprintf(_log, "--------------- \t\t--------\t\t--------\t\t-----------------\n");
    fclose(_log);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int app_clt_hdr()
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Appends a table header to the clients log file.
------------------------------------------------------------------------------*/
int app_clt_hdr()
{
    FILE *_log;

    if((_log = fopen(CLTLOGFILE, "a")) == NULL)
    {
        printf("\n\tFailed to open client's log file\n\n");
        return -1;
    }

    if(ftell(_log) == 0) // if no header found in log file
    {
        fprintf(_log, "Packet Size: %d\tTransmission Duration (each client): %d seconds\n\n", PKTSIZE, TIMEOUT);
        fprintf(_log, "CONNECTION TIME \t\tREQUESTS\t\tDATA TRANSFERRED\tAVG RESPONSE TIME\n");
        fprintf(_log, "--------------- \t\t--------\t\t----------------\t-----------------\n");
    }

    fclose(_log);
    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int append_srv_data(char *filename, struct srv_log_stats stats)
|                   *filename : name of file to write to
|                   stats : server statistics to append to server log file
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Appends server statistical data in 'stats' to the server log
|               file specified by '*filename'.
------------------------------------------------------------------------------*/
int append_srv_data(char *filename, struct srv_log_stats stats)
{
    FILE *_log;

    pthread_mutex_lock(&lock);
    if((_log = fopen(filename, "a")) == NULL)
    {
        printf("\n\tFailed to open server's log file\n\n");
        return -1;
    }

    // append time of connection
    fprintf(_log, "%d/%d/%d ", stats.tm.tm_year + 1900, stats.tm.tm_mon + 1, stats.tm.tm_mday);
    fprintf(_log, "%d:%d:%d \t\t", stats.tm.tm_hour, stats.tm.tm_min, stats.tm.tm_sec);

    // append number of client requests
    fprintf(_log, "%s\t\t%d\t\t", stats.clt_ip, stats.requests);

    // append total bytes transferred
    if(stats.bytes.gigabytes > 0)
        fprintf(_log, "\t%.2f GB\n", stats.bytes.gigabytes);
    else if(stats.bytes.megabytes > 0)
        fprintf(_log, "\t%.2f MB\n", stats.bytes.megabytes);
    else if(stats.bytes.kilobytes > 0)
        fprintf(_log, "\t%.2f KB\n", stats.bytes.kilobytes);
    else if(stats.bytes.bytes > 0)
        fprintf(_log, "\t%.2f Bytes\n", stats.bytes.bytes);

    fclose(_log);
    pthread_mutex_unlock(&lock);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int append_clt_data(struct clt_log_stats stats, double t)
|                   stats : server statistics to append to client log file
|                   t : average response time from server
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Appends client statistical data in 'stats' to the client log
|               file as well as the average server response time specified by
|               't'.
------------------------------------------------------------------------------*/
int append_clt_data(struct clt_log_stats stats, double t)
{
    FILE *_log;

    if((_log = fopen(CLTLOGFILE, "a")) == NULL)
    {
        printf("\n\tFailed to open client's log file\n\n");
        return -1;
    }

    // append time of connection
    fprintf(_log, "%d/%d/%d ", stats.tm.tm_year + 1900, stats.tm.tm_mon + 1, stats.tm.tm_mday);
    fprintf(_log, "%d:%d:%d \t\t", stats.tm.tm_hour, stats.tm.tm_min, stats.tm.tm_sec);
    fprintf(_log, "%d\t\t", stats.requests);

    // append number of requests to server
    if(stats.bytes.gigabytes > 0)
        fprintf(_log, "\t%.2f GB\t\t", stats.bytes.gigabytes);
    else if(stats.bytes.megabytes > 0)
        fprintf(_log, "\t%.2f MB\t\t", stats.bytes.megabytes);
    else if(stats.bytes.kilobytes > 0)
        fprintf(_log, "\t%.2f KB\t\t", stats.bytes.kilobytes);
    else if(stats.bytes.bytes > 0)
        fprintf(_log, "\t%.2f Bytes\t\t", stats.bytes.bytes);

    // append average response time from server
    fprintf(_log, "%f ms\n", t);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   int append_total_clients(char *filename, int total)
|                   *filename : name of file to write to
|                   total: total number of connections
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Appends the total amount of clients connected held in 'total'
|               to the server log file specified by '*filename'.
------------------------------------------------------------------------------*/
int append_total_clients(char *filename, int total)
{
    FILE *_log;
    if((_log = fopen(filename, "a")) == NULL)
    {
        printf("\n\tFailed to open server's log file\n\n");
        return -1;
    }

    fprintf(_log, "-------------------------------------------------------------------------------------------\n");
    fprintf(_log, "\nTotal Client Connections: %d", total);

    return 0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void init_bytes_struct(struct Bytes *data)
|                   *data : pointer to bytes struct to initialize
|
|   RETURN:     void
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       initializes the struct variables in '*data' to 0.
------------------------------------------------------------------------------*/
void init_bytes_struct(struct Bytes *data)
{
    data->bytes = 0.0;
    data->kilobytes = 0.0;
    data->megabytes = 0.0;
    data->gigabytes = 0.0;
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void print_bytes_struct(struct Bytes data)
|                   data : bytes struct to print
|
|   RETURN:     0 on success, -1 on failure
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Prints the content of the Bytes struct 'data'.
------------------------------------------------------------------------------*/
void print_bytes_struct(struct Bytes data)
{
    printf("\nB: %.0f\n", data.bytes);
    printf("\nK: %.2f\n", data.kilobytes);
    printf("\nM: %.2f\n", data.megabytes);
    printf("\nG: %.2f\n", data.gigabytes);
}


/*------------------------------------------------------------------------------
|   FUNCTION:   void update_bytes_struct(struct Bytes *data, int bytes)
|                   *data : pointer to bytes struct to update
|                   bytes : number of bytes to add to struct
|
|   RETURN:     void
|
|   DATE:       Feb 19, 2018
|
|   AUTHOR:     Alex Zielinski
|
|   DESC:       Takes 'bytes' and updates the Bytes struct '*data' in order
|               to track the total number of bytes transferred during a
|               connection.
------------------------------------------------------------------------------*/
void update_bytes_struct(struct Bytes *data, int bytes)
{
    // add to bytes
    data->bytes += bytes; // b = 4000

    // check if you can update kilobytes
    if(data->bytes >= KILO)
    {
        data->kilobytes += data->bytes / (float)KILO; // update kilobytes
        data->bytes = 0; // reset bytes

        // check if you can update megabytes
        if(data->kilobytes >= KILO)
        {
            data->megabytes = data->kilobytes / (float)KILO; // update kilobytes

            // check if you can update gigabytes
            if(data->megabytes >= KILO)
            {
                data->gigabytes = data->megabytes / (float)KILO; // update kilobytes
            }
        }
    }
}
