	# compilation variables
CC = gcc
CFLAGS = -W -Wall -pedantic

# client program variables
CLT_FILES = src/clt_thread.c src/socket.c src/log.c
CLT_EXE = bin/clt_thread

# threaded server variables
SRV_THREAD_FILES = src/srv_thread.c src/socket.c src/log.c
SRV_THREAD_EXE = bin/srv_thread

# multiplexed server (poll) variables
SRV_POLL_FILES = src/srv_poll.c src/socket.c src/log.c
SRV_POLL_EXE = bin/srv_poll

# Asynchoronous server (epoll) variables
SRV_EPOLL_FILES = src/srv_epoll.c src/socket.c src/log.c
SRV_EPOLL_EXE = bin/srv_epoll

#------------------------------------------------------------------------------
all: clt_thread srv_thread srv_poll srv_epoll

clt_thread: $(CLT_FILES)
	$(CC) $(CFLAGS) -o $(CLT_EXE) $(CLT_FILES) -fopenmp

srv_thread: $(SRV_THREAD_FILES)
	$(CC) $(CFLAGS) -o $(SRV_THREAD_EXE) $(SRV_THREAD_FILES) -fopenmp

srv_poll: $(SRV_POLL_FILES)
	$(CC) $(CFLAGS) -o $(SRV_POLL_EXE) $(SRV_POLL_FILES) -fopenmp

srv_epoll: $(SRV_EPOLL_FILES)
	$(CC) $(CFLAGS) -o $(SRV_EPOLL_EXE) $(SRV_EPOLL_FILES) -fopenmp

clean:
	rm -f $(CLT_EXE)
	rm -f $(SRV_THREAD_EXE)
	rm -f $(SRV_POLL_EXE)
#------------------------------------------------------------------------------
