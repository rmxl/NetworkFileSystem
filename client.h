#ifndef CLIENT_H
#define CLIENT_H

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <errno.h>

typedef struct {
    int sockfd;
    int* ack_received;
} AckThreadArgs;


#endif
