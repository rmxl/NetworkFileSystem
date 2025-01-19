#ifndef STORAGESERVER_H
#define STORAGESERVER_H
#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#define QUEUE_SIZE 1024
#define NS_PORT 8082
#define NS_IP "127.0.0.1"
extern int queue[QUEUE_SIZE];
extern int front;
extern int rear;
extern int itemcount;
extern int ns_send_socket;
int peek();

int isfull();
int isempty();
int size();
void insert(int data);
int remove_data();
int connect_to_ns(char *ns_ip, int ns_port,char* argv);
#endif
